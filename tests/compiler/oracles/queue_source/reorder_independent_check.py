# Standalone checker reports are intentional command-line output.
# ruff: noqa: T201
"""A25-T standalone checker for the frozen ``pyc_reorder_pipeline`` contract.

The existing queue lit driver invokes this separate checker on its generated
artifacts. It is the A25-T independent oracle: it re-derives the frozen contract's observable
consequences without using the delivered source, the delivered ``reorder`` oracle
mode or any DUT output as a golden, and it tries to falsify both.

Run it with the artifacts of the frozen four-command build::

    python3 tests/compiler/oracles/queue_source/reorder_independent_check.py \\
        --design-ac <build>/design.ac \\
        --rtl-dir   <build>/verilog \\
        --cpp-dir   <build>/cpp \\
        --iverilog  "$(command -v iverilog)" \\
        --vvp       "$(command -v vvp)"

The causal mutants and start probe always run. By default they use the same
public ``python -m pycircuit.cli`` flow; ``--pycircuit`` may select its frozen
installed wrapper. The nightly owner supplies the native-tool environment.

What is checked, and where the expectations come from
-----------------------------------------------------
``stream_contract``  decodes the 131-bit ``expected`` words of the
    ``reorder_independent`` vector mode *from the bits* and re-checks the frozen
    contract on them.  The absolute epoch tables in it are re-typed here from the
    four contract clauses of ``A25/oracle.md`` sect. 3 -- out-of-order arrival is
    accepted and buffered, a full store backpressures, only a key equal to the
    committed next key is released, duplicate / already-retired keys are invalid
    -- plus one edge of latency per queue.  They are NOT read out of the vector
    module, so a wrong expectation there is caught here.

``artifact_structure``  re-derives V08-style facts from ``design.ac``, the
    emitted Verilog and the emitted C++: two queues with the frozen depths and
    ``local_occupancy``, one ``@ac.rule``, no ``ac.reorder``, a sixteen-entry
    key-addressed table, a 64-bit pointer register, and a 131-bit result whose
    fields are ``{ready, available, head, next_key, fault}``.

``observation_cone``  is the structural half of ``C-R1``: a field-precise
    transitive fan-out over the emitted Verilog assigns, from the net that drives
    ``result.fault``.  Nothing the data path is made of -- no ``fifo`` control, no
    ``dffe`` enable, data or reset -- may be reachable from it.  It also checks
    that ``result.next_key`` is the ``q`` port of the pointer register itself and
    not a recomputation.

``mutant_check``  is the causal half of ``C-R1``: two scratch copies of the
    source, one with the ``fault`` output forced to a constant and one with the
    ``next_key`` output forced to a constant, are built through the frozen
    toolchain and run through Icarus on the same stimulus.  Every result bit
    except the mutated observation must be bit-identical.

``four_state_check``  is the V07 delivery and uses **Icarus + vvp only**
    (Verilator two-state-folds x and, on 5.044, miscompiles the generated row file
    as soon as it contains a ``z`` literal).  Three probes: an unknown payload on
    a NON-effective epoch leaves every output bit known; an unknown that IS an
    effective control is a run-terminating framework failure; and a known
    duplicate key is a fail-closed ``fault`` with no run termination, i.e. a
    different expectation class.

``rival_check``  reads the rival table the vector mode computed and independently
    re-derives the counterfactual for ``key_window`` from the stimulus alone:
    if a far key were rejected and NOT consumed, the input queue would reach its
    depth-eight bound sixteen edges earlier than the delivered ``ready`` curve
    says it does.

Every assertion prints ``ok <id>: <what>`` on success and raises
``AssertionError`` with the offending value on failure.
"""

import argparse
import importlib.util
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

INPUT_BITS = 64
OUTPUT_BITS = 131
SEQ_BITS = 32
SEQ_MASK = (1 << SEQ_BITS) - 1
VALUE_MASK = (1 << 32) - 1
WORD_MASK = (1 << 64) - 1
CAPACITY = 16
IN_DEPTH = 8
OUT_DEPTH = 4
FAR_KEY = 1000
# MSB -> LSB of `ReorderResult`: ready(1) available(1) head(64) next_key(64) fault(1)
FAULT_BIT = 0
NEXT_KEY_LOW, NEXT_KEY_HIGH = 1, 64
HEAD_LOW, HEAD_HIGH = 65, 128
AVAILABLE_BIT = 129
READY_BIT = 130
PAYLOAD_BITS = HEAD_HIGH - HEAD_LOW + 1  # 64 == Token{sequence, value}

SECTION_NAMES = (
    "T1_permutation",
    "T2_hole_then_fill",
    "T3_far_fill_occupancy",
    "T4_far_slot_blocks_in_window",
    "T4b_full_store_duplicate_head",
    "T4c_high_key_field_full_width",
    "T5_duplicate_then_stale_stall",
    "T6_stale_on_arrival",
    "T7_reset_clears_full_store",
    "T8_backpressure_then_drain",
    "T9_masked_unknown",
)


def ok(identifier, what):
    print(f"ok {identifier}: {what}")


def decode(word):
    """Decode one packed ``ReorderResult`` word; every field is a plain integer."""
    return {
        "ready": bool((word >> READY_BIT) & 1),
        "available": bool((word >> AVAILABLE_BIT) & 1),
        "head": (word >> HEAD_LOW) & WORD_MASK,
        "next_key": (word >> NEXT_KEY_LOW) & WORD_MASK,
        "fault": bool((word >> FAULT_BIT) & 1),
        "sequence": (word >> (HEAD_LOW + SEQ_BITS)) & SEQ_MASK,
        "value": (word >> HEAD_LOW) & VALUE_MASK,
    }


# ---------------------------------------------------------------------------
# 1. the contract, re-derived from the delivered rows' bits
# ---------------------------------------------------------------------------

def load_vectors(path):
    spec = importlib.util.spec_from_file_location("q4_a25_vectors", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def epochs_of(rows):
    """Split the row stream into epochs and verify the sampling convention."""
    assert len(rows) % 2 == 0, f"row count {len(rows)} is not an even number of epochs"
    epochs = []
    for index in range(0, len(rows), 2):
        low, high = rows[index], rows[index + 1]
        assert low["clk"] == 0 and high["clk"] == 1, (index, low["clk"], high["clk"])
        for field in ("rst", "valid", "take", "data"):
            assert low[field] == high[field], (index, field, low[field], high[field])
        # The convention is that both rows of an epoch carry the committed
        # pre-edge state read together with that epoch's inputs.
        assert low["expected"] == high["expected"], (index, "expected")
        epochs.append(
            {
                "epoch": index // 2 + 1,
                "rst": bool(low["rst"]),
                "valid": bool(low["valid"]),
                "take": bool(low["take"]),
                "data": low["data"],
                "data_known": low.get("data_known", (1 << INPUT_BITS) - 1),
                "data_z": low.get("data_z", 0),
                "driven_sequence": (low["data"] >> 32) & SEQ_MASK,
                "driven_value": low["data"] & VALUE_MASK,
                **decode(low["expected"]),
            }
        )
    return epochs


def sections_of(epochs):
    """The ten hand-authored sections, reconstructed from the reset edges.

    Every section opens with a reset epoch, and T7 additionally contains one
    internal reset epoch.  The reset epoch's own sample is the PRE-reset state, so
    it closes the previous section rather than opening the next; a section is the
    run of epochs after its opening reset.  T7's internal reset is folded back
    into T7, which is what makes the section-local epoch numbers below the same
    numbers the vector mode's hand tables use.
    """
    boundaries = [index for index, entry in enumerate(epochs) if entry["rst"]]
    assert len(boundaries) == 12, (
        f"expected 11 section-opening resets plus T7's internal one, got "
        f"{len(boundaries)} at {[epochs[i]['epoch'] for i in boundaries]}"
    )
    runs = []
    for order, start in enumerate(boundaries):
        stop = boundaries[order + 1] if order + 1 < len(boundaries) else len(epochs)
        runs.append(epochs[start + 1:stop])
    assert all(runs), "a reset edge was not followed by any epoch"
    folded = (runs[:8] + [runs[8] + [epochs[boundaries[9]]] + runs[9]]
              + runs[10:])
    assert len(folded) == len(SECTION_NAMES), (len(folded), len(SECTION_NAMES))
    out = {}
    for name, run in zip(SECTION_NAMES, folded):  # noqa: B905 - preserve the extracted reference algorithm
        out[name] = [dict(entry, local=index + 1) for index, entry in enumerate(run)]
    return out


def sinks(entries):
    return {entry["local"]: entry["sequence"]
            for entry in entries if entry["available"] and entry["take"]}


def stream_contract(record):
    rows = record["rows"]
    assert record["input_bits"] == INPUT_BITS, record["input_bits"]
    assert record["output_bits"] == OUTPUT_BITS, record["output_bits"]
    assert record["four_state"] is False, "the plain mode carries no unknown payload"
    assert record["masked_unknown_rows"] == 0, record["masked_unknown_rows"]
    assert all(0 <= row["expected"] < (1 << OUTPUT_BITS) for row in rows)
    epochs = epochs_of(rows)
    sections = sections_of(epochs)
    ok("T1-rows", f"{len(rows)} rows = {len(epochs)} epochs in {len(sections)} sections, "
                  f"{INPUT_BITS} -> {OUTPUT_BITS} bits, all expectations two-state")

    # (a) clause 6 on the delivered bits.  The sampled `head` is the token sitting
    #     in the output queue, while the sampled `next_key` has already advanced
    #     past it (the pointer increments on the retirement that pushed it), so the
    #     falsifiable statement is: a token can only reach the sink AFTER the
    #     pointer has named it (`sequence < next_key`), and the number of pops
    #     equals the number of pointer advances -- nothing invented, nothing lost.
    popped = [entry for entry in epochs if entry["available"] and entry["take"]]
    assert popped, "no token is ever popped"
    ahead = [entry for entry in popped if entry["sequence"] >= entry["next_key"]]
    assert not ahead, [(e["epoch"], e["sequence"], e["next_key"]) for e in ahead[:4]]
    advances, previous = 0, 0
    for entry in epochs:
        if entry["rst"]:
            previous = 0
            continue
        assert entry["next_key"] >= previous
        advances += entry["next_key"] - previous
        previous = entry["next_key"]
    assert advances == len(popped), (advances, len(popped))
    ok("T2-pointer", f"all {len(popped)} pops carry a sequence strictly behind the "
                     f"sampled `next_key`, and the {advances} pointer advances are "
                     f"exactly the {len(popped)} retirements that reached the sink")

    # (a2) RT-01 / BD-07 on the delivered bits: a released token carries exactly the
    #      (sequence, value) pair that was driven for that sequence.  A swapped or
    #      truncated payload cannot satisfy this, because no driven value equals its
    #      own sequence in that case.
    driven = {}
    for entry in epochs:
        if entry["valid"]:
            driven.setdefault(entry["driven_sequence"], set()).add(entry["driven_value"])
    for entry in popped:
        assert entry["value"] in driven[entry["sequence"]], (entry["epoch"], entry)
    assert any(entry["value"] > 0x80000000 for entry in popped), "no high payload bit"
    assert any(entry["value"] & 1 for entry in popped), "no low payload bit"
    assert all(entry["value"] != entry["sequence"] for entry in popped if entry["sequence"])
    ok("T2b-payload", f"all {len(popped)} released tokens carry the exact driven "
                      f"(sequence, value) pair, high and low payload bits included")

    # (b) clause 1 on the delivered bits: the pointer never moves backwards and
    #     never jumps.
    previous, resets = 0, 0
    for entry in epochs:
        if entry["rst"]:
            resets += 1
            previous = 0
            continue
        assert entry["next_key"] >= previous, (entry["epoch"], entry["next_key"], previous)
        assert entry["next_key"] - previous <= 1, (entry["epoch"], entry["next_key"], previous)
        previous = entry["next_key"]
    ok("T3-monotone", f"`next_key` is monotone and moves by at most one per edge "
                      f"({resets} reset edges restart it at 0)")

    # (c) no token is lost: every advance of the pointer is a real retirement, and
    #     the count of releases equals the final pointer of the section.
    for name, entries in sections.items():
        sequence = [entry["sequence"] for entry in entries
                    if entry["available"] and entry["take"]]
        assert all(a < b for a, b in zip(sequence, sequence[1:])), (name, sequence)  # noqa: B905 - preserve the extracted reference algorithm
        assert sequence == list(range(len(sequence))), (name, sequence)
        assert entries[-1]["next_key"] >= len(sequence), (name, entries[-1]["next_key"])
    ok("T4-conservation", "inside every section the released stream is strictly "
                          "increasing and contiguous from 0, and never exceeds the pointer")

    # (d) HAND-COMPUTED ABSOLUTE EPOCH TABLES, re-derived here from the clauses.
    #     "a token handed over on edge E is visible on E+1, a retirement on E is
    #     pushed with deadline E+1" -- plus, for T1, the fact that key 1 cannot be
    #     retired before the edge it is admitted into the store.
    hand = {
        "T1_permutation": {7: 0, 10: 1, 11: 2, 12: 3, 13: 4, 14: 5, 15: 6, 16: 7,
                           17: 8, 18: 9},
        "T2_hole_then_fill": {7: 0, 8: 1, 9: 2, 10: 3, 11: 4, 12: 5},
        "T6_stale_on_arrival": {4: 0, 5: 1, 6: 2},
        "T7_reset_clears_full_store": {21: 0, 22: 1, 23: 2, 24: 3, 25: 4, 26: 5,
                                       27: 6, 28: 7},
        "T8_backpressure_then_drain": {41 + index: index for index in range(28)},
        "T9_masked_unknown": {4: 0, 6: 1, 8: 2, 10: 3, 12: 4, 14: 5},
    }
    for name, table in hand.items():
        actual = sinks(sections[name])
        assert actual == table, (name, actual, table)
    ok("T5-absolute-tables", "the six hand-computed absolute epoch tables reproduce "
                             "bit-for-bit: " + ", ".join(sorted(hand)))

    # (e) clause 6 negative half: a hole WAITS.  T2 offers 5,3,4 before 0 exists
    #     and nothing may leave before the edge key 0 becomes releasable.
    t2 = sections["T2_hole_then_fill"]
    assert all(not entry["available"] for entry in t2 if entry["local"] <= 6), (
        "T2 released something before key 0 was available"
    )
    assert sinks(t2) and min(sinks(t2)) == 7, sinks(t2)
    ok("T6-hole-waits", "T2 holds keys 5/3/4 with 0 missing and emits nothing for "
                        "six edges; the pointer stays at 0 throughout")

    # (f) clause 5 + PM correction 2: `capacity` counts OCCUPIED ENTRIES, so a far
    #     key is accepted and holds a slot.  The proof is an epoch, not a value:
    #     the store comes full only if all sixteen far keys were admitted, which
    #     pushes the first `ready`-low edge out to local 25 in T3, T4 and T4b.
    for name in ("T3_far_fill_occupancy", "T4_far_slot_blocks_in_window",
                 "T4b_full_store_duplicate_head", "T4c_high_key_field_full_width"):
        entries = sections[name]
        low = [entry["local"] for entry in entries if not entry["ready"]]
        assert low[:1] == [25], (name, low[:4])
        assert entries[23]["local"] == 24 and entries[23]["ready"] is True, \
            (name, entries[23]["local"], entries[23]["ready"])
        assert sinks(entries) == {}, (name, sinks(entries))
        assert all(entry["fault"] == 0 for entry in entries), name
    ok("T7-far-key-accepted", "in T3/T4/T4b `ready` stays high through local 24 and "
                              "falls first at local 25: sixteen far keys really are "
                              "stored (a key-window reading falls at local 9)")

    # (g) the counterfactual of (f), computed from the STIMULUS alone so the
    #     `key_window` negative control is not taken on trust: if a far key were
    #     rejected AND not consumed, the rule would never pop the input queue and
    #     the queue (depth 8) would be full from local 9 on.
    t3 = sections["T3_far_fill_occupancy"]
    held, broken = 0, None
    for entry in t3:
        if not entry["valid"]:
            continue
        if held >= IN_DEPTH:
            broken = entry["local"]
            break
        held += 1
    assert broken == 9, broken
    assert t3[8]["local"] == 9 and t3[8]["ready"] is True, t3[8]
    ok("T8-key-window-counterfactual",
       "a reject-and-hold DUT is full at local 9 by the stimulus alone, while the "
       "delivered bits stay ready until local 25: the rival is genuinely separated")

    # (h) clause 7 shape (PM appendix sect. 5): duplicate and already-retired keys
    #     are fail-closed.  `fault` is the observation, it is confined to the two
    #     illegal-key sections, it never clears once raised, and `ready` -- pure
    #     input-queue capacity -- is never lowered by it.
    faulted = sorted(name for name, entries in sections.items()
                     if any(entry["fault"] for entry in entries))
    assert faulted == ["T5_duplicate_then_stale_stall", "T6_stale_on_arrival"], faulted
    for name, first in (("T5_duplicate_then_stale_stall", 4),
                        ("T6_stale_on_arrival", 5)):
        entries = sections[name]
        raised = [entry["local"] for entry in entries if entry["fault"]]
        assert raised == list(range(first, len(entries) + 1)), (name, raised)
        assert {entry["ready"] for entry in entries} == {True}, name
        # The offending token is at the input head from the first fault edge on;
        # the pointer may still complete the retirement it had already committed
        # on that edge, and is frozen from the NEXT edge to the end of the section.
        frozen = {entry["next_key"] for entry in entries[first:]}
        assert len(frozen) == 1, (name, sorted(frozen))
    ok("T9-fail-closed", "T5/T6 raise `fault` at local 4/5, hold it to the end of "
                         "the section, freeze the pointer, and never lower `ready`")

    # (i) T4b isolates the `free` conjunct of `fault`: the head is a duplicate of a
    #     stored key while the store is FULL, and the frozen block short-circuits
    #     before comparing keys, so `fault` must be 0 there.
    t4b = sections["T4b_full_store_duplicate_head"]
    assert t4b[17]["driven_sequence"] == 20 and t4b[17]["fault"] == 0, t4b[17]
    assert t4b[17]["ready"] is True, t4b[17]
    assert t4b[17]["valid"] is True, t4b[17]
    ok("T10-fault-free-conjunct", "T4b keeps `fault` low from local 18 while the head "
                                  "is a duplicate and the store is full")

    # (j) T7: the reset edge strikes a genuinely occupied chain, and the same
    #     stimulus without that one edge releases nothing (checked by the vectors'
    #     reset_ignored_at_T7_only control).
    t7 = sections["T7_reset_clears_full_store"]
    assert t7[16]["rst"] is True, t7[16]["local"]
    assert sinks(t7) == hand["T7_reset_clears_full_store"], sinks(t7)
    assert t7[-1]["next_key"] == 8 and t7[-1]["available"] == 0
    ok("T11-reset", "T7's reset lands on a non-empty chain and is followed by the "
                    "contiguous 0..7 drain; without that single edge nothing leaves")

    # (k) T8: the backpressure chain bottoms out at 4 + 16 + 8 = 28 and the drain
    #     is lossless.  `ready` may fall in exactly the four sections that can fill
    #     the chain, and nowhere else.
    low_sections = sorted(name for name, entries in sections.items()
                          if any(not entry["ready"] for entry in entries))
    assert low_sections == ["T3_far_fill_occupancy", "T4_far_slot_blocks_in_window",
                            "T4b_full_store_duplicate_head",
                            "T4c_high_key_field_full_width",
                            "T8_backpressure_then_drain"], low_sections
    t8 = sections["T8_backpressure_then_drain"]
    assert [entry["local"] for entry in t8 if not entry["ready"]][:1] == [29]
    last_release = max(sinks(t8))
    assert last_release == 68 and len(sinks(t8)) == 28, (last_release, len(sinks(t8)))
    assert t8[-1]["next_key"] == 28, t8[-1]["next_key"]
    ok("T12-backpressure", "`ready` falls in exactly four sections; T8 falls first at "
                           "local 29 and drains 0..27 over locals 41..68 to pointer 28")
    return sections


# ---------------------------------------------------------------------------
# 2. the emitted artifacts
# ---------------------------------------------------------------------------

def balanced(text, start):
    """``text[start]`` is an opening paren; return (body, index_after_close)."""
    assert text[start] == "(", text[start:start + 20]
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
            if depth == 0:
                return text[start + 1:index], index + 1
    raise AssertionError("unbalanced parentheses")


def parse_ports(text):
    """``{port: expression}`` from an instantiation port list.

    A plain regex is not enough: the table register's ``q`` connection contains
    parentheses and bit selects, so each port expression is scanned with balanced
    parentheses rather than matched up to the first ``)``.
    """
    ports = {}
    cursor = 0
    for match in re.finditer(r"\.(\w+)\s*\(", text):
        if match.start() < cursor:
            continue
        body, cursor = balanced(text, match.end() - 1)
        ports[match.group(1)] = body.strip()
    return ports


def instances(text, primitive):
    """Every ``primitive #(params) name (ports);`` occurrence in ``text``."""
    found = []
    for match in re.finditer(r"\b" + primitive + r"\s*#\s*\(", text):
        params, after = balanced(text, match.end() - 1)
        open_paren = text.index("(", after)
        assert open_paren >= 0
        name = text[after:open_paren].strip()
        ports, _ = balanced(text, open_paren)
        found.append((name, params.strip(), parse_ports(ports)))
    return found


FIFO_DEPTH = re.compile(r"\.DEPTH\((\d+)\)")
FIFO_POLICY = re.compile(r"\.READY_POLICY\((\d+)\)")
FIFO_LATENCY = re.compile(r"\.AVAILABILITY_LATENCY\(64'd(\d+)\)")
CPP_KERNEL = re.compile(
    r"fifo_kernel<([^,]+),\s*(\d+),\s*gfsim::QueueReadyPolicy::(\w+),\s*(\d+)ULL>"
)


def struct_definitions(header):
    """``{name: [(field, bits, type_name), ...]}`` from the emitted C++ header.

    ``bits`` is ``None`` and ``type_name`` set for a field whose type is another
    emitted struct, which is how ``ReorderResult.head`` is declared.
    """
    out = {}
    for match in re.finditer(r"struct\s+(\w+)\s*\{(.*?)\n\};", header, re.S):
        name, body = match.groups()
        fields = []
        for line in body.splitlines():
            field = re.search(r"(?:gfsim::Bits<(\d+)>|(\w+))\s+(\w+)\{\};", line)
            if field:
                fields.append((field.group(3),
                               int(field.group(1)) if field.group(1) else None,
                               field.group(2)))
        if fields:
            out[name] = fields
    return out


def payload_bits(structs, name):
    return sum(bits if bits else payload_bits(structs, type_name)
               for _field, bits, type_name in structs[name])


def artifact_structure(design_ac, rtl_dir, cpp_dir):
    text = design_ac.read_text()
    counts = {op: text.count(f'"{op}"') for op in ("ac.queue", "ac.rule", "ac.reorder")}
    assert counts["ac.queue"] == 2, counts
    assert counts["ac.rule"] == 1, counts
    assert counts["ac.reorder"] == 0, counts
    assert text.count('ready_policy = "local_occupancy"') == 2, text.count(
        'ready_policy = "local_occupancy"')
    assert "#ac.math_int<16>" in text and "#ac.math_int<8>" in text and \
        "#ac.math_int<4>" in text, "table extent / queue depths not found"
    assert '#ac.math_int<131>' not in text
    assert "table<" in text, "no ac.table in the design"
    ok("V1-design", "design.ac: 2 ac.queue, 1 ac.rule, 0 ac.reorder, both queues "
                    "local_occupancy, table extent 16, depths 8/4")

    rtl_paths = sorted(rtl_dir.rglob("*.v")) + sorted(rtl_dir.rglob("*.sv"))
    assert rtl_paths, f"no RTL under {rtl_dir}"
    blob = "\n".join(path.read_text() for path in rtl_paths)
    fifos = instances(blob, "fifo")
    assert len(fifos) == 2, [name for name, _, _ in fifos]
    depths = tuple(int(FIFO_DEPTH.search(params).group(1)) for _, params, _ in fifos)
    policies = {int(FIFO_POLICY.search(params).group(1)) for _, params, _ in fifos}
    latencies = {int(FIFO_LATENCY.search(params).group(1)) for _, params, _ in fifos}
    assert depths == (IN_DEPTH, OUT_DEPTH), depths
    assert policies == {0}, policies
    assert latencies == {1}, latencies
    assert all("Token_t" in params for _, params, _ in fifos), [p for _, p, _ in fifos]
    ok("V2-rtl-fifo", f"RTL: two fifo instances in source order, DEPTH {depths}, "
                      f"READY_POLICY {sorted(policies)}, AVAILABILITY_LATENCY "
                      f"{sorted(latencies)}, both on the Token payload")

    dffes = instances(blob, "dffe")
    assert len(dffes) == 2, [name for name, _, _ in dffes]
    entry = next((item for item in dffes if "Entry_t" in item[1]), None)
    pointer = next((item for item in dffes if "(64)-1:0" in item[1]), None)
    assert entry is not None, "no Entry table register"
    assert pointer is not None, "no 64-bit pointer register"
    lane = re.match(r"(\w+)", entry[2]["q"]).group(1)
    extent = re.search(
        r"wire logic \[\(\(\(1 \* \((\d+)\)\) \* \(\$bits\(pycircuit_types::\w+\)\)\)\)-1:0\] "
        + lane + r";", blob)
    assert extent and int(extent.group(1)) == CAPACITY, (lane, extent)
    assert re.search(r"\.clk\(pyc_7079635f636c6b\)", entry[2]["clk"] and "" or "") is None
    init = re.search(r"wire logic \[\(64\)-1:0\] (\w+);", blob)
    assert init, "no 64-bit wire for the pointer init value"
    ok("V3-rtl-state", f"RTL: one Entry table register ({entry[0]}) with a "
                       f"{extent.group(1)}-lane table, one 64-bit pointer register "
                       f"({pointer[0]}); no third state element")

    cpp = sorted(cpp_dir.rglob("*.hpp"))
    assert cpp, f"no emitted C++ under {cpp_dir}"
    header = "\n".join(path.read_text() for path in cpp)
    kernels = []
    for match in CPP_KERNEL.finditer(header):
        row = (match.group(1).split("::")[-1], int(match.group(2)),
               match.group(3), int(match.group(4)))
        if row not in kernels:
            kernels.append(row)
    assert len(kernels) == 2, kernels
    assert tuple(row[1] for row in kernels) == (IN_DEPTH, OUT_DEPTH), kernels
    assert {row[0] for row in kernels} == {"Token"}, kernels
    assert {row[2] for row in kernels} == {"LocalOccupancy"}, kernels
    assert {row[3] for row in kernels} == {1}, kernels
    structs = struct_definitions(header)
    payload = structs["Token"]
    assert payload == [("pyc_73657175656e6365", 32, None), ("value", 32, None)], payload
    assert payload_bits(structs, "Token") == INPUT_BITS
    result = structs["ReorderResult"]
    assert [field for field, _bits, _type in result] == \
        ["ready", "available", "head", "next_key", "fault"], result
    widths = [bits if bits else payload_bits(structs, type_name)
              for _field, bits, type_name in result]
    assert widths == [1, 1, INPUT_BITS, 64, 1], (result, widths)
    assert sum(widths) == OUTPUT_BITS
    ok("V4-cpp-kernel", f"C++: {len(kernels)} fifo kernels, DEPTH "
                        f"{tuple(r[1] for r in kernels)}, payload Token = 32+32 = "
                        f"{payload_bits(structs, 'Token')} bits, all LocalOccupancy, "
                        f"latency 1; ReorderResult packs {sum(widths)} bits in field "
                        f"order {[f for f, _b, _t in result]}")
    return blob


# ---------------------------------------------------------------------------
# 3. C-R1 structurally: a field-precise fan-out from the observation nets
# ---------------------------------------------------------------------------

def typedef_fields(text):
    """``{struct_t: (field, ...)}`` from the emitted ``pycircuit_types`` package.

    A field is either a bit vector (``logic [(64)-1:0] next_key;``) or another
    emitted struct (``..._Token_t token;``); both spellings have to be counted,
    otherwise the field indices of a concat do not line up with the declaration.
    """
    out = {}
    for match in re.finditer(r"typedef\s+struct\s+packed\s*\{(.*?)\}\s*(\w+)\s*;",
                             text, re.S):
        body, name = match.groups()
        fields = re.findall(r"^\s*(?:logic\s*\[[^\]]*\]|[\w:]+)\s+(\w+)\s*;", body,
                            re.M)
        assert fields, (name, body)
        out[name] = tuple(fields)
    return out


def assign_graph(blob):
    """``{lhs: rhs}`` for every single-line ``assign`` in the emitted RTL."""
    graph = {}
    for line in blob.splitlines():
        match = re.match(r"\s*assign\s+([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*=\s*(.+);\s*$",
                         line)
        if match:
            graph.setdefault(match.group(1), []).append(match.group(2))
    return graph


def split_concat(expression):
    """Top-level operands of ``{a, b, c}`` or ``None`` if it is not a concat."""
    text = expression.strip()
    if not (text.startswith("{") and text.endswith("}")):
        return None
    body = text[1:-1]
    operands, depth, current = [], 0, ""
    for character in body:
        if character in "{([":
            depth += 1
        elif character in "})]":
            depth -= 1
        if character == "," and depth == 0:
            operands.append(current.strip())
            current = ""
        else:
            current += character
    operands.append(current.strip())
    return operands


def observation_cone(blob, structs):
    """Field-precise transitive fan-out from the two observation fields."""
    graph = assign_graph(blob)
    net_types = {}
    for match in re.finditer(r"wire\s+pycircuit_types::(\w+)\s+(\w+)\s*;", blob):
        net_types[match.group(2)] = match.group(1)
    field_source = {}
    for net, expressions in graph.items():
        kind = net_types.get(net)
        if kind is None or kind not in structs:
            continue
        operands = split_concat(expressions[-1])
        if operands and len(operands) == len(structs[kind]):
            for index, operand in enumerate(operands):
                field_source[(net, index)] = operand.strip()

    def reads(expression):
        out = []
        for match in re.finditer(r"\b([A-Za-z_]\w*)\s*(?:\.\s*(\w+))?", expression):
            name, field = match.group(1), match.group(2)
            if name in ("logic", "wire", "if", "else"):
                continue
            if field is not None and name in net_types and \
                    net_types[name] in structs:
                fields = structs[net_types[name]]
                out.append((name, fields.index(field) if field in fields else None))
            else:
                out.append((name, None))
        return out

    def resolve(net, field):
        """Resolve a struct-field node to the plain net that drives that field."""
        if field is None:
            return net
        return field_source.get((net, field), net)

    def fan_out(start_net, start_field):
        queue = [(start_net, start_field)]
        seen = set()
        reachable = set()
        while queue:
            node = queue.pop()
            if node in seen:
                continue
            seen.add(node)
            name, field = node
            reachable.add(name)
            for lhs, expressions in graph.items():
                for expression in expressions:
                    for target in reads(expression):
                        read_name, read_field = target
                        if read_name != name:
                            continue
                        if read_field is not None and field is not None \
                                and read_field != field:
                            continue
                        operands = split_concat(expression)
                        kind = net_types.get(lhs)
                        if operands and kind in structs and \
                                len(operands) == len(structs[kind]):
                            for index, operand in enumerate(operands):
                                if resolve(*target) == operand.strip() or \
                                        operand.strip().startswith(read_name):
                                    queue.append((lhs, index))
                        else:
                            queue.append((lhs, None))
        return reachable

    # `result` may be driven through a plain wire alias before the concat.
    target, top = "result", None
    for _ in range(8):
        expressions = graph.get(target)
        assert expressions and len(expressions) == 1, (target, expressions)
        top = split_concat(expressions[0])
        if top is not None:
            break
        target = expressions[0].strip()
    fields = ("ready", "available", "head", "next_key", "fault")
    assert top is not None and len(top) == len(fields), (target, top)
    def observation_source(operand):
        """Resolve a result-concat operand to the plain net that actually drives it.

        The operand may be a struct field (``pyc_net_5.fault``) or a wire that is
        itself assigned from one (``pyc_net_65 = pyc_net_5.fault``); both spellings
        appear in the emitted RTL, so both are chased here.
        """
        seen = set()
        while operand not in seen:
            seen.add(operand)
            match = re.fullmatch(r"(\w+)\.(\w+)", operand)
            if match:
                owner, attribute = match.groups()
                kind = net_types.get(owner)
                assert kind in structs, (operand, kind)
                return field_source.get((owner, structs[kind].index(attribute)),
                                        operand)
            expressions = graph.get(operand)
            if not expressions or len(expressions) != 1:
                return operand
            operand = expressions[0].strip()
        return operand

    fault_source = observation_source(top[fields.index("fault")].strip())
    reachable = fan_out(fault_source, None)
    state_reads = {"valid", "take", "data", "clk", "rst"}
    for primitive in ("fifo", "dffe"):
        for name, _params, ports in instances(blob, primitive):
            for port, expression in ports.items():
                for token in re.findall(r"\b([A-Za-z_]\w*)\b", expression):
                    if token in reachable and token not in state_reads:
                        raise AssertionError(
                            f"`fault` reaches {primitive} {name}.{port}({expression}); "
                            "the observation gates the data path"
                        )
    assert len(reachable) <= 6, reachable
    ok("V5-fault-cone", f"`fault` (net {fault_source}) fans out to "
                        f"{sorted(reachable - {fault_source})} and reaches no fifo/dffe "
                        f"port: the observation is a leaf")

    pointer_net = observation_source(top[fields.index("next_key")].strip())
    pointer_owner = [name for name, _params, ports in instances(blob, "dffe")
                     if ports.get("q", "").strip() == pointer_net]
    assert len(pointer_owner) == 1, (pointer_net, pointer_owner)
    ok("V6-next-key-tap", f"`next_key` resolves to {pointer_net}, which is the `q` port "
                          f"of {pointer_owner[0]} directly; the observation is the state "
                          f"element itself, not a recomputation")
    return fault_source, pointer_net


# ---------------------------------------------------------------------------
# 4. C-R1 causally: two scratch mutants through the frozen toolchain
# ---------------------------------------------------------------------------

PROBE_TB = """`include "probe_widths.svh"
module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic take = 0;
  logic [63:0] data = 0;
  wire [130:0] result;
  pyc_root dut(.*);
  initial begin
    #1;
    pyc_7079635f636c6b = 1;
    #1;
    pyc_7079635f636c6b = 0;
    pyc_7079635f727374 = 0;
    #1;
    `include "probe_rows.svh"
    $finish;
  end
endmodule
"""


def probe_rows(rows, with_checks):
    lines = []
    for index, row in enumerate(rows):
        lines.append(f"pyc_7079635f727374=1'b{row['rst']};valid=1'b{row['valid']};"
                     f"take=1'b{row['take']};data=64'b{row['data']:064b};#1;")
        if with_checks:
            lines.append(f'if(result!==131\'b{row["expected"]:0131b})'
                         f'$fatal(1,"A25-T probe row{index} failed");')
        lines.append(f'$display("R%0d %b", {index}, result);'
                     f'$display("WORK {index}");'
                     f'pyc_7079635f636c6b=1\'b{row["clk"]};#1;')
    return "\n".join(lines) + "\n"


def run(command, code=0, env=None, timeout=900):
    result = subprocess.run(list(map(str, command)), text=True, capture_output=True,
                            timeout=timeout, env=env)
    if result.returncode != code:
        print(result.stdout[-3000:])
        print(result.stderr[-3000:], file=sys.stderr)
        raise AssertionError(f"{command[0]}: exit {result.returncode} != {code}")
    return result


def build_mutant(pycircuit, source, source_root, scratch, mutation, label):
    """Compile a scratch copy of the source with one literal substitution."""
    text = Path(source).read_text()
    assert text.count(mutation[0]) == 1, (label, text.count(mutation[0]))
    root = scratch / label
    root.mkdir(parents=True, exist_ok=True)
    variant = root / Path(source).name
    variant.write_text(text.replace(mutation[0], mutation[1]))
    unit = root / "unit"
    run([*pycircuit, "compile", "-c", variant, "--source-root", root,
         "--package-prefix", "q4_queue", "-o", unit])
    design = root / "design.ac"
    run([*pycircuit, "link", unit, "--top",
         "q4_queue.pyc_reorder_pipeline.ReorderPipeline", "-o", design])
    run([*pycircuit, "emit", design, "--target", "verilog", "-o", root / "verilog"])
    return root / "verilog"


def icarus_words(iverilog, vvp, primitives, rtl, workdir, rows):
    workdir.mkdir(parents=True, exist_ok=True)
    (workdir / "probe_widths.svh").write_text(
        f"`define Q4_INPUT_BITS {INPUT_BITS}\n`define Q4_OUTPUT_BITS {OUTPUT_BITS}\n")
    (workdir / "probe_rows.svh").write_text(probe_rows(rows, with_checks=False))
    (workdir / "probe_tb.sv").write_text(PROBE_TB)
    binary = workdir / "probe"
    run([iverilog, "-g2012", "-s", "tb", "-I" + str(workdir), "-o", binary,
         *primitives, *rtl, workdir / "probe_tb.sv"])
    observed = [line for line in run([vvp, binary]).stdout.splitlines()
                if line.startswith("R")]
    assert len(observed) == len(rows), (len(observed), len(rows))
    return [int(line.split()[1], 2) for line in observed]


def mutant_check(pycircuit, source, source_root, scratch, iverilog, vvp, primitives,
                 rtl_paths, rows):
    reference = icarus_words(iverilog, vvp, primitives, rtl_paths,
                             scratch / "reference", rows)
    assert reference == [row["expected"] for row in rows], \
        "the reference build does not reproduce the vector expectations"
    mask_ready_available_head_next = (((1 << (READY_BIT - NEXT_KEY_LOW + 1)) - 1)
                                      << NEXT_KEY_LOW)
    mask_ready_available_head_fault = (
        ((1 << (READY_BIT - HEAD_LOW + 1)) - 1) << HEAD_LOW) | 1
    for label, mutation, keep in (
        ("fault_forced", ("fault=move.fault,", "fault=0,"),
         mask_ready_available_head_next),
        ("next_key_forced", ("next_key=move.next_key,", "next_key=0,"),
         mask_ready_available_head_fault),
    ):
        rtl_dir = build_mutant(pycircuit, source, source_root, scratch, mutation, label)
        receipt = json.loads((rtl_dir / "generated.json").read_text())
        mutant_rtl = sorted(
            (rtl_dir / item["path"] for item in receipt["files"] if item["role"] == "rtl"),
            key=lambda path: (path.name != "design_top.sv", str(path)),
        )
        words = icarus_words(iverilog, vvp, primitives, mutant_rtl,
                             scratch / label / "run", rows)
        differing = [index for index, (a, b) in enumerate(zip(words, reference))  # noqa: B905 - preserve the extracted reference algorithm
                     if (a ^ b) & keep]
        assert not differing, (label, differing[:8])
        changed = sum(1 for a, b in zip(words, reference) if a != b)  # noqa: B905 - preserve the extracted reference algorithm
        assert changed, f"{label}: the mutation changed nothing at all"
        ok(f"V7-{label}", f"forcing {label.replace('_forced', '')} to a constant on the "
                          f"output changes {changed} words and ZERO data-path bits "
                          f"(ready/available/head/next_key/fault as applicable)")
    shutil.rmtree(scratch, ignore_errors=True)


def start_probe(pycircuit, source, source_root, scratch, iverilog, vvp, primitives,
                rtl_paths, start=5):
    """``RT-02``/``BD-06``: the historical ``start`` constant IS the reset value.

    The migrated surface exposes no ``start`` parameter at all, so the only way to
    exercise the claim is a scratch mutant whose pointer initialiser is a non-zero
    ``start``; the frozen source is not touched.  The mutant must sample
    ``next_key == 5`` on the reset epoch and then release exactly 5, 6, 7 from an
    in-order 5..7 stream -- i.e. the first token out is the one whose key equals
    ``start``, and the constant reaches the comparator as well as the register.
    """
    mutation = ("    next_key: ac.u64 = 0\n",
                f"    next_key: ac.u64 = {start}\n")
    rtl_dir = build_mutant(pycircuit, source, source_root, scratch, mutation,
                           "start_five")
    receipt = json.loads((rtl_dir / "generated.json").read_text())
    mutant_rtl = sorted(
        (rtl_dir / item["path"] for item in receipt["files"] if item["role"] == "rtl"),
        key=lambda path: (path.name != "design_top.sv", str(path)),
    )
    rows = []

    def edge(rst=0, valid=0, take=0, data=0):
        for clk in (0, 1):
            rows.append({"clk": clk, "rst": rst, "valid": valid, "take": take,
                         "data": data, "expected": 0})

    edge(rst=1)
    for key in (start, start + 1, start + 2):
        edge(valid=1, take=1, data=(key << 32) | (0xD0000000 | key))
    for _ in range(8):
        edge(take=1)
    words = icarus_words(iverilog, vvp, primitives, mutant_rtl,
                         scratch / "start_five" / "run", rows)
    sampled = [decode(word) for word in words]
    assert sampled[0]["next_key"] == start, sampled[0]
    assert sampled[2]["next_key"] == start, sampled[2]
    released = [sampled[index]["sequence"] for index in range(0, len(rows), 2)
                if sampled[index]["available"] and rows[index]["take"]]
    assert released == [start, start + 1, start + 2], released
    assert sampled[-1]["next_key"] == start + 3, sampled[-1]
    ok("V8-start-constant", f"a scratch `next_key = {start}` mutant (frozen source "
                            f"untouched) samples {start} on the reset epoch and "
                            f"releases exactly {released}: `start` is a compile-time "
                            f"constant that is both the reset value and the first key")


# ---------------------------------------------------------------------------
# 5. four-state (V07): Icarus + vvp only
# ---------------------------------------------------------------------------

FOUR_STATE_TB = """`include "probe_widths.svh"
module tb;
  logic pyc_7079635f636c6b = 0;
  logic pyc_7079635f727374 = 1;
  logic valid = 0;
  logic take = 0;
  logic [63:0] data = 0;
  wire [130:0] result;
  pyc_root dut(.*);
  integer unknowns = 0;
  task automatic tick(input logic rst_, valid_, take_, input logic [63:0] data_);
    pyc_7079635f727374 = rst_;
    valid = valid_;
    take = take_;
    data = data_;
    #1;
    if ($isunknown(result) !== 1'b0) begin
      unknowns = unknowns + 1;
      $display("UNKNOWN epoch result=%b", result);
    end
    pyc_7079635f636c6b = 1; #1; pyc_7079635f636c6b = 0; #1;
  endtask
  initial begin
    #1;
    pyc_7079635f636c6b = 1; #1; pyc_7079635f636c6b = 0;
    pyc_7079635f727374 = 0; #1;
`ifdef PROBE_A
    // A: unknown payload on epochs where `valid` is low -> no effective transfer,
    // so nothing may become unknown anywhere in the result.
    tick(1'b1, 1'b0, 1'b0, 64'h0);
    tick(1'b0, 1'b1, 1'b1, {32'd0, 32'hA0000000});
    tick(1'b0, 1'b0, 1'b1, 64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx);
    tick(1'b0, 1'b1, 1'b1, {32'd1, 32'hA0000001});
    tick(1'b0, 1'b0, 1'b1, 64'bzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz);
    tick(1'b0, 1'b1, 1'b1, {32'd2, 32'hA0000002});
    tick(1'b0, 1'b0, 1'b0, 64'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx);
    tick(1'b0, 1'b0, 1'b1, 64'bzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz);
    tick(1'b0, 1'b0, 1'b1, 64'h0);
    tick(1'b0, 1'b0, 1'b1, 64'h0);
    if (unknowns != 0) $fatal(1, "PROBE_A unknown result bits");
    $display("PROBE_A known_outputs only");
`endif
`ifdef PROBE_B1
    // B1: `valid` itself unknown while the input queue has room -> the push is an
    // EFFECTIVE unknown control and must take the framework failure channel.
    tick(1'b1, 1'b0, 1'b0, 64'h0);
    tick(1'b0, 1'b1, 1'b1, {32'd0, 32'hB0000000});
    tick(1'b0, 1'bx, 1'b1, {32'd1, 32'hB0000001});
    $fatal(1, "PROBE_B1 accepted an unknown valid");
`endif
`ifdef PROBE_B2
    // B2: `take` unknown while the output queue actually holds a token.
    tick(1'b1, 1'b0, 1'b0, 64'h0);
    tick(1'b0, 1'b1, 1'b1, {32'd0, 32'hB0000000});
    tick(1'b0, 1'b1, 1'b1, {32'd1, 32'hB0000001});
    tick(1'b0, 1'b1, 1'b1, {32'd2, 32'hB0000002});
    tick(1'b0, 1'b1, 1'b1, {32'd3, 32'hB0000003});
    tick(1'b0, 1'b0, 1'b1, 64'h0);
    tick(1'b0, 1'b0, 1'bx, 64'h0);
    $fatal(1, "PROBE_B2 accepted an unknown take");
`endif
`ifdef PROBE_C
    // C: a KNOWN duplicate key is a different expectation class -- fail-closed
    // `fault`, no run termination, and the pointer stops advancing.
    tick(1'b1, 1'b0, 1'b0, 64'h0);
    tick(1'b0, 1'b1, 1'b1, {32'd0, 32'hC0000000});
    tick(1'b0, 1'b1, 1'b1, {32'd1, 32'hC0000001});
    tick(1'b0, 1'b1, 1'b1, {32'd1, 32'hC0000001});
    for (integer index = 0; index < 12; index = index + 1)
      tick(1'b0, 1'b0, 1'b1, 64'h0);
    if ($isunknown(result) !== 1'b0) $fatal(1, "PROBE_C unknown result bits");
    if (result[0] !== 1'b1) $fatal(1, "PROBE_C fault was not raised");
    if (result[129] !== 1'b0) $fatal(1, "PROBE_C kept a token available");
    if (result[130] !== 1'b1) $fatal(1, "PROBE_C lowered ready");
    $display("PROBE_C known fault, no run termination, ready high");
`endif
    $finish;
  end
endmodule
"""


def four_state_check(iverilog, vvp, primitives, rtl_paths, scratch):
    workdir = scratch / "four_state"
    workdir.mkdir(parents=True, exist_ok=True)
    (workdir / "probe_widths.svh").write_text(
        f"`define Q4_INPUT_BITS {INPUT_BITS}\n`define Q4_OUTPUT_BITS {OUTPUT_BITS}\n")
    (workdir / "four_state_tb.sv").write_text(FOUR_STATE_TB)
    for probe, expect_fatal in (("PROBE_A", None), ("PROBE_B1", "effective transfers"),
                                ("PROBE_B2", "effective transfers"),
                                ("PROBE_C", None)):
        binary = workdir / probe
        run([iverilog, "-g2012", "-s", "tb", "-D" + probe, "-I" + str(workdir),
             "-o", binary, *primitives, *rtl_paths, workdir / "four_state_tb.sv"])
        result = subprocess.run([str(vvp), str(binary)], text=True,
                                capture_output=True, timeout=300)
        output = result.stdout + result.stderr
        if expect_fatal is None:
            assert result.returncode == 0, (probe, result.returncode, output[-600:])
            assert f"{probe} known" in output, (probe, output[-600:])
            ok(f"V9-{probe}", output.strip().splitlines()[-1]
               if output.strip() else probe)
        else:
            assert result.returncode != 0, (probe, output[-600:])
            assert expect_fatal in output, (probe, output[-600:])
            ok(f"V9-{probe}", f"Icarus run aborted with the framework unknown-control "
                              f"failure: {expect_fatal!r}")
    ok("V9-classes", "PROBE_B (unknown that is an EFFECTIVE control) terminates the run, "
                     "PROBE_C (known duplicate key) does not: the two expectation "
                     "classes are distinct under four-state simulation")
    shutil.rmtree(workdir, ignore_errors=True)


# ---------------------------------------------------------------------------
# 6. the negative controls
# ---------------------------------------------------------------------------

def rival_check(record):
    rivals = record["rival_differing_rows"]
    required = {
        "arrival_order": "RT-04/BD-02: release by arrival, not by key",
        "skip_holes": "RT-06: a missing key is skipped instead of waited for",
        "passthrough_fifo": "RT-04: no reordering at all",
        "key_window": "appendix correction 2: `capacity` read as a key window",
        "out_of_window_dropped": "appendix correction 2: a far key consumed and dropped",
        "duplicate_overwrite": "RT-07/NG-01: a duplicate silently overwrites",
        "drop_on_blocked_output": "BD-04: discard when the output queue is full",
        "ready_lowered_on_fault": "C-R1: lower `in_ready` on an illegal head",
        "stale_accepted": "RT-08/NG-03: accept an already-retired key",
        "combinational_latency": "TM-01: `latency=1` as a pure combinational path",
        "fault_without_free": "C-R1: `fault` without the store-full conjunct",
        "key_truncated_8": "RT-03/BD-05: the store compares a truncated key field",
        "reset_ignored": "TM-14/UT-11: a DUT with no reset at all",
        "reset_ignored_at_T7_only": "C2: T7's own reset edge specifically",
    }
    missing = sorted(set(required) - set(rivals))
    assert not missing, missing
    for label, rows in sorted(rivals.items()):
        assert rows > 0, (label, rows)
    total = len(record["rows"])
    ok("V10-rivals", f"{len(rivals)} negative controls are all separated by the delivered "
                     f"rows: " + ", ".join(f"{k}={v}" for k, v in sorted(rivals.items())))
    print(f"  rival -> rows of the {total} that differ from the frozen contract:")
    for label, rows in sorted(rivals.items()):
        print(f"    {label:26} {rows:4d}   [{required[label]}]")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--design-ac", required=True)
    parser.add_argument("--rtl-dir", required=True)
    parser.add_argument("--cpp-dir", required=True)
    parser.add_argument("--iverilog", required=True)
    parser.add_argument("--vvp", required=True)
    parser.add_argument("--vectors",
                        default=str(Path(__file__).with_name("models.py")))
    parser.add_argument("--primitives", default=None,
                        help="directory of the frozen include/verilog primitives")
    parser.add_argument("--pycircuit", default=None,
                        help="frozen public CLI wrapper; default: this Python -m pycircuit.cli")
    parser.add_argument("--source", default=str(Path(__file__).resolve().parents[2]
                                    / "lit/Source/Inputs/queue-source/pyc_reorder_pipeline.py"))
    parser.add_argument("--scratch", default=None)
    arguments = parser.parse_args()

    repo = next(
        parent for parent in Path(__file__).resolve().parents
        if (parent / "include/verilog").is_dir()
    )
    primitives = sorted(Path(arguments.primitives or (repo / "include/verilog")).glob("*.v"))
    assert primitives, "no Verilog primitives found"
    rtl_dir = Path(arguments.rtl_dir)
    rtl_paths = sorted(rtl_dir.rglob("*.v")) + sorted(rtl_dir.rglob("*.sv"))
    rtl_paths.sort(key=lambda path: (path.name != "design_top.sv", str(path)))

    module = load_vectors(arguments.vectors)
    record = module.oracle("reorder_independent")
    raw = module.oracle("reorder_independent_raw")
    assert len(raw["rows"]) == len(record["rows"])
    assert raw["masked_unknown_rows"] == 12 and record["masked_unknown_rows"] == 0
    # Only the explicit input known/Z planes differ between these two modes.
    # Compare the complete remaining stimulus/expectation rows, not a vacuous
    # equality or just the expected payload word.
    for raw_row, plain_row in zip(raw["rows"], record["rows"], strict=True):
        assert {
            key: value for key, value in raw_row.items()
            if key not in ("data_known", "data_z")
        } == plain_row
    ok("V0-modes", "the plain and `_raw` modes share one stimulus and one expectation "
                   "stream; only the `_raw` mode carries 12 unknown-payload rows")

    stream_contract(record)
    blob = artifact_structure(Path(arguments.design_ac), rtl_dir, Path(arguments.cpp_dir))
    observation_cone(blob, typedef_fields(
        (rtl_dir / "design_top.sv").read_text()
        if (rtl_dir / "design_top.sv").is_file()
        else "\n".join(path.read_text() for path in rtl_paths)))
    rival_check(record)

    # `resolve()` matters on macOS: /tmp is a symlink to /private/tmp and the
    # frozen compiler rejects a publication path that traverses a symlink.
    scratch = Path(arguments.scratch or tempfile.mkdtemp(prefix="a25t-check-")).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    try:
        four_state_check(arguments.iverilog, arguments.vvp, primitives, rtl_paths,
                         scratch)
        command = ([arguments.pycircuit] if arguments.pycircuit else
                   [sys.executable, "-m", "pycircuit.cli"])
        mutant_check(command, Path(arguments.source).resolve(),
                     Path(arguments.source).resolve().parent, scratch / "mutants",
                     arguments.iverilog, arguments.vvp, primitives, rtl_paths,
                     record["rows"])
        start_probe(command, Path(arguments.source).resolve(),
                    Path(arguments.source).resolve().parent, scratch / "mutants",
                    arguments.iverilog, arguments.vvp, primitives, rtl_paths)
    finally:
        if arguments.scratch is None:
            shutil.rmtree(scratch, ignore_errors=True)
    print("\nA25-T independent checker: all checks ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
