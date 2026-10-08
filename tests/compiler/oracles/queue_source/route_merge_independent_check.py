# Standalone checker reports are intentional command-line output.
# Preserve the extracted reference algorithm's zip operations.
# ruff: noqa: T201, B905
"""A27-T standalone checker: emitted-artifact facts and the boundary table.

This file is **not** part of the shared lit driver.  It is a checker that A27-T
runs by hand against the artifacts of its own build, so that every claim about
"the six queues really are inside the DUT and carry the frozen parameters"
(V08), about the packed output layout, and about the boundary value table is
re-derived from the emitted text rather than quoted from another packet.

Run it with the artifacts of the frozen four-command build:

    python3 tests/compiler/oracles/queue_source/route_merge_independent_check.py \
        --design-ac <build>/design.ac \
        --rtl-dir  <build>/verilog \
        --cpp-dir  <build>/cpp

Every assertion below prints ``ok <id>: <what>`` on success and raises
``AssertionError`` with the offending text on failure.

Sources of the expected values (no DUT output is used as a golden):

* ``pyc_route_merge_pipeline.py`` at the frozen baseline (16 lines): branch 0 is
  ``item + 10``, branch 1 is ``item + 20``, merge ``policy="priority"`` with input
  order ``(left_done, right_done)``;
* ``docs/reference/spec-queues.md:34-70`` for the queue contract;
* the A27-O contract table ``A27/oracle.md:69-77`` for the six ``depth`` values
  and ``latency``/``ready_policy``.
"""

import argparse
import re
import sys
from pathlib import Path

MASK64 = (1 << 64) - 1
# Source order of the six queues, from the historical dataflow:
# input_queue, left, right, left_done, right_done, merged.
EXPECTED_DEPTHS = (2, 2, 2, 1, 1, 2)
EXPECTED_QUEUE_COUNT = 6
READY_POLICY_LOCAL_OCCUPANCY = 0  # RTL enum encoding of `local_occupancy`
EXPECTED_AVAILABILITY_LATENCY = 1

FIFO_RTL = re.compile(
    r"fifo\s*#\(\s*\.T\(logic \[\((\d+)\)-1:0\]\)\s*,"
    r"\s*\.DEPTH\((\d+)\)\s*,"
    r"\s*\.READY_POLICY\((\d+)\)\s*,"
    r"\s*\.AVAILABILITY_LATENCY\(64'd(\d+)\)\s*\)"
)
FIFO_CPP = re.compile(
    r"pyc_queue_pyc_[0-9a-f]+_state\(std::make_shared<"
    r"gfsim::collection_storage<gfsim::fifo_kernel<gfsim::Bits<(\d+)>,\s*(\d+),\s*"
    r"gfsim::QueueReadyPolicy::(\w+),\s*(\d+)ULL>{2,3}"
)


def ok(identifier, what):
    print(f"ok {identifier}: {what}")


def check_text(design_ac, rtl_dir, cpp_dir):
    text = design_ac.read_text()
    ok(
        "V08-1",
        f"design.ac carries {text.count(chr(34) + 'ac.queue' + chr(34))} "
        f"ac.queue ops and {text.count(chr(34) + 'ac.rule' + chr(34))} ac.rule ops",
    )

    rtl = sorted(rtl_dir.rglob("*.v")) + sorted(rtl_dir.rglob("*.sv"))
    fifos = []
    for path in rtl:
        for match in FIFO_RTL.finditer(path.read_text()):
            fifos.append(tuple(int(group) for group in match.groups()))
    if not fifos:
        raise AssertionError(f"no `fifo #(...)` instantiation found under {rtl_dir}")
    widths, depths, policies, latencies = zip(
        *fifos
    )  # noqa: B905 - preserve the extracted reference algorithm
    if depths != EXPECTED_DEPTHS:
        raise AssertionError(f"RTL depth order {depths} != {EXPECTED_DEPTHS}")
    if set(widths) != {64}:
        raise AssertionError(f"RTL fifo payload widths {set(widths)} != {{64}}")
    if set(policies) != {READY_POLICY_LOCAL_OCCUPANCY}:
        raise AssertionError(
            f"RTL ready policies {set(policies)} are not all local_occupancy"
        )
    if set(latencies) != {EXPECTED_AVAILABILITY_LATENCY}:
        raise AssertionError(f"RTL availability latencies {set(latencies)} != {{1}}")
    ok(
        "V08-2",
        f"{len(fifos)} RTL fifos in source order, DEPTH {depths}, "
        f"payload 64-bit, READY_POLICY {set(policies)}, "
        f"AVAILABILITY_LATENCY {set(latencies)}",
    )

    cpp = []
    for path in sorted(cpp_dir.rglob("*.hpp")):
        for match in FIFO_CPP.finditer(path.read_text()):
            cpp.append(match.groups())
    if len(cpp) != EXPECTED_QUEUE_COUNT:
        raise AssertionError(f"cpp state count {len(cpp)} != {EXPECTED_QUEUE_COUNT}")
    cpp_depths = tuple(int(row[1]) for row in cpp)
    if cpp_depths != EXPECTED_DEPTHS:
        raise AssertionError(f"cpp depth order {cpp_depths} != {EXPECTED_DEPTHS}")
    if {row[0] for row in cpp} != {"64"}:
        raise AssertionError(
            f"cpp payload widths { {row[0] for row in cpp} } != {{64}}"
        )
    if {row[2] for row in cpp} != {"LocalOccupancy"}:
        raise AssertionError(f"cpp ready policies { {row[2] for row in cpp} }")
    if {row[3] for row in cpp} != {"1"}:
        raise AssertionError(f"cpp latencies { {row[3] for row in cpp} }")
    ok(
        "V08-3",
        f"{len(cpp)} native fifo kernels in source order, DEPTH {cpp_depths}, "
        "payload 64-bit, all LocalOccupancy",
    )

    # The boundary mapping: `ready` must be the input queue's in_ready, and
    # `available`/`head` must be the merged queue's out_valid/out_data.  Packing
    # is MSB-first, so the first operand of the outer concat is `ready`.
    blob = "".join(path.read_text() for path in sorted(cpp_dir.rglob("*.hpp")))
    marker = "RouteMergeResult>::fromPacked(gfsim::concat("
    if marker not in blob:
        raise AssertionError(
            "result struct packing expression not found in emitted cpp"
        )
    tail = blob[blob.index(marker) :][:800]
    order = {name: tail.index(name) for name in ("in_ready", "out_valid", "out_data")}
    if not order["in_ready"] < order["out_valid"] < order["out_data"]:
        raise AssertionError(
            f"packed order is not {{ready, available, head}}: {tail[:400]}"
        )
    fields = re.findall(
        r"pyc_queue_pyc_([0-9a-f]+)_(in_ready|out_valid|out_data)", tail
    )
    try:
        ready_owner = next(q for q, role in fields if role == "in_ready")
        valid_owner = next(q for q, role in fields if role == "out_valid")
        data_owner = next(q for q, role in fields if role == "out_data")
    except StopIteration as error:  # pragma: no cover - defensive
        raise AssertionError(
            f"could not attribute the result fields: {fields}"
        ) from error
    # hex ...5f30 == "__pyc_queue_0" (the source-side input queue) and
    # hex ...5f35 == "__pyc_queue_5" (the last queue, i.e. `merged`).
    if not ready_owner.endswith("5f30"):
        raise AssertionError(f"`ready` is not the input queue in_ready: {ready_owner}")
    if not (valid_owner.endswith("5f35") and data_owner.endswith("5f35")):
        raise AssertionError(
            f"`available`/`head` are not the merged queue outputs: "
            f"{valid_owner}, {data_owner}"
        )
    ok(
        "V08-4",
        "result = {ready=input_queue.in_ready (queue 0), "
        "available=merged.out_valid, head=merged.out_data (queue 5)}",
    )


def boundary_table():
    """(left, right) -> observable expectation, computed from the frozen adds."""
    rows = [
        ("0", 0, 0, "left branch: 0 is the branch-0 selector", 10),
        ("1", 1, 1, "right branch: 1 is the branch-1 selector", 21),
        ("2^64-1", MASK64, None, "arithmetic only: out of selector domain", 19),
        ("2^64-10", MASK64 - 9, None, "arithmetic only: out of selector domain", 0),
    ]
    print("\npayload -> branch result (wrap modulo 2^64):")
    for label, payload, selector, note, want in rows:
        got = (payload + (10 if selector == 0 else 20)) & MASK64
        if selector is None:
            left, right = (payload + 10) & MASK64, (payload + 20) & MASK64
            print(f"  {label:>9} -> +10={left}, +20={right}   [{note}]")
            assert right == want or left == want
        else:
            print(f"  {label:>9} -> {got}   [{note}]")
            assert got == want, (label, got, want)

    print("\nmerge observable (priority: both valid -> left wins):")
    cases = [
        ("left only  (right invalid)", True, False, True, 10, "left_done wins"),
        ("right only (left invalid)", False, True, True, 21, "right_done wins"),
        ("both valid, merged_ready", True, True, True, 10, "priority -> left_done"),
        ("both valid, merged full", True, True, False, None, "no push at all"),
        ("both invalid", False, False, True, None, "available=0, head=packed 0"),
    ]
    for label, left_valid, right_valid, merged_ready, want, note in cases:
        winner = None
        if merged_ready:
            winner = 0 if left_valid else (1 if right_valid else None)
        got = {0: 10, 1: 21, None: None}[winner]
        assert got == want, (label, got, want)
        print(f"  {label:28} -> {str(got):>4}  [{note}]")
    ok("V08-5", "boundary table reproduced from the frozen adds and merge order")


def rival_control(vectors_path):
    """Re-derive the policy discrimination from the vector module itself."""
    import importlib.util

    spec = importlib.util.spec_from_file_location(
        "q4_independent_vectors", vectors_path
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    record = module.oracle("route_merge_independent")
    rivals = record["rival_differing_rows"]
    assert (
        rivals["round_robin"] > 0
        and rivals["downstream_pop"] > 0
        and rivals["reset_ignored"] > 0
    ), rivals
    ok(
        "V08-6",
        f"vector set separates the frozen contract from both rivals "
        f"(round_robin {rivals['round_robin']} rows, "
        f"downstream_pop {rivals['downstream_pop']} rows)",
    )
    print(
        "  arbitration edges (both valid AND merged_ready):",
        record["arbitration_edges"],
    )
    print(
        "  vacuous co-valid but merged-full edges  :", record["vacuous_co_valid_edges"]
    )
    print("  right-served edges                      :", record["right_served_edges"])
    print(
        "  handshake: driven",
        record["driven_tokens"],
        "accepted",
        record["accepted_tokens"],
    )
    # C2: the trailing reset must strike a NON-EMPTY chain, and the same vector
    # set must separate the frozen contract from a DUT that ignores `rst`
    # altogether -- otherwise MG-09 / TM-14 / UT-11 are not falsifiable.
    print("  reset edges                             :", record["reset_edges"])
    print(
        "  chain occupancy before each reset edge  :",
        {
            edge: tuple(occupancy)
            for edge, occupancy in record["reset_occupancy"].items()
        },
    )
    print(
        "  tokens discarded by reset               :", record["reset_discarded_tokens"]
    )
    print(
        "  C2 reset control: rows a reset-ignoring DUT gets wrong:",
        rivals["reset_ignored"],
    )


def failure_contract(vectors_path):
    """Check invalid selectors against hand-written boundary expectations."""
    import importlib.util

    spec = importlib.util.spec_from_file_location("route_failure_vectors", vectors_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    cases = module.queue_failure_contracts()
    for name, first in (("RouteFault", 1), ("RouteBlockedFault", 8)):
        case = cases[name]
        assert case["first_failure"] == first
        rows = [dict(row) for row in case["rows"]]
        trace, _ = module._route_merge_independent_model(rows, reject_invalid=False)
        assert not any(e["expected_failure"] for e in trace)
        # The rival's run status is the separating observation: it continues
        # instead of rejecting the Work that reads the invalid head.
        assert any(row["expected_failure"] for row in case["rows"])
    # Invalid external payload is harmless while valid is false. Once stored
    # and available, selectors 2 and u64-max fail on E1, regardless of readiness.
    for selector, valid, first in (
        (2, 0, None),
        ((1 << 64) - 1, 0, None),
        (0, 1, None),
        (1, 1, None),
        (2, 1, 1),
        ((1 << 64) - 1, 1, 1),
    ):
        rows = [
            {
                "clk": clk,
                "rst": 0,
                "valid": valid if edge == 0 else 0,
                "take": 0,
                "data": selector,
            }
            for edge in range(4)
            for clk in (0, 1)
        ]
        trace, _ = module._route_merge_independent_model(rows)
        failures = [e["row"] // 2 for e in trace if e["expected_failure"]]
        assert (failures[0] if failures else None) == first, (selector, valid, failures)
    ok(
        "route-source-failure",
        "selector2 fails at E1; selector2 with full left queue fails at E8; stall-only rival rejected",
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--design-ac", required=True)
    parser.add_argument("--rtl-dir", required=True)
    parser.add_argument("--cpp-dir", required=True)
    parser.add_argument("--vectors", default=str(Path(__file__).with_name("models.py")))
    arguments = parser.parse_args()
    check_text(
        Path(arguments.design_ac), Path(arguments.rtl_dir), Path(arguments.cpp_dir)
    )
    boundary_table()
    rival_control(arguments.vectors)
    failure_contract(arguments.vectors)
    print("\nA27-T independent checker: all checks ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
