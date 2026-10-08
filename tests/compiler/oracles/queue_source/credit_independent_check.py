#!/usr/bin/env python3
# Standalone checker reports are intentional command-line output.
# Preserve the extracted reference algorithm's intentional unused event index.
# ruff: noqa: T201, B007
"""A06-T independent oracle + checker for the ``pyc_credit_pipeline`` credit window.

Owner: **A06-T** (independent test instance).  This file is a *new* artifact: it is
not derived from ``pyc_credit_pipeline.py`` (the frozen subject), not from the
existing ``credit`` oracle mode in ``queue-source-vectors.py``, and **never** from
DUT or generated-RTL output.  It re-derives the frozen contract from

* ``docs/gates/logs/20261007-remaining-migration-orchestration/A06/oracle.md``
  (``IF-01``..``IF-07``, ``ST-01``..``ST-09``, ``OUT-01``..``OUT-12``,
  ``BP-01``..``BP-05``, ``BD-01``..``BD-16``, ``NEG-01``..``NEG-08``, the sect. 3.1
  phase model and the sect. 6 tables),
* ``A06/mapping.json`` (machine-readable copy of the same 49 assertions),
* ``A06/CD-ENTRY-APPENDIX.md`` (migrated port / Struct / queue tables and the named
  divergences ``CD-ENTRY``, ``IF-1``, ``CD-2``, ``C-2``, ``Q-D1(b)``, ``G-1``).

Derivation discipline
---------------------
The model below is hand-written from the oracle's *rules* (sect. 2.3 Work/Xfer and
the sect. 3.1 phase order), not transcribed from any existing host model.  Three
independent confirmations are built in:

1. ``_documented_tables()`` re-derives the five **documented** absolute-epoch tables
   of ``oracle.md`` sect. 6 (``OUT-07``..``OUT-11``) from the oracle's own stimulus
   using *this* model.  A wrong model would not reproduce them.
2. ``ARITHMETIC`` holds absolute epochs computed **by hand** from the single formula
   of ``oracle.md`` sect. 3.3 (``release = admission + cost + 1``,
   ``sink = admission + cost + 2``) for this file's own stimulus.
3. The negative controls are evaluated on the *fixed* stimulus columns, so a rival
   can never separate itself merely by being driven differently.

``self_test()`` never reads a DUT: rows and expected values are produced here.  The
harness (``queue-source.py``) later feeds these rows to the frozen native model and
to the emitted RTL, which is the actual falsification attempt.

Usage::

    python3 credit_independent_check.py
"""

from __future__ import annotations

import json
import sys

RUN_ID = "20261007-remaining-migration-orchestration"

# ---------------------------------------------------------------------------
# 1. Frozen constants (A06-O IF-01/IF-03/IF-04/IF-05 + CD-ENTRY appendix sect. 1.2)
# ---------------------------------------------------------------------------

SLOTS = 2  # IF-05: the window is 2 credit registers, not a queue depth
DEPTH = 4  # IF-03/IF-04: `issued` and `completed` are both depth 4
LATENCY = 1  # IF-03/IF-04: latency 1 == no extra pipeline delay
SEQ_BITS = 4  # IF-01: CreditToken.sequence : u4
CYC_BITS = 4  # IF-01: CreditToken.cycles   : u4  (the sole cost source)
VAL_BITS = 16  # IF-01: CreditToken.value    : u16 (opaque)
TOKEN_BITS = SEQ_BITS + CYC_BITS + VAL_BITS  # 24
RESULT_BITS = 2 + TOKEN_BITS  # ready + available + head = 26
TOKEN_MASK = (1 << TOKEN_BITS) - 1


def pack_token(sequence: int, cycles: int, value: int) -> int:
    """Declaration order == packed order, MSB first (IF-01 / CD-ENTRY sect. 1.2)."""
    return (
        ((sequence & 0xF) << (CYC_BITS + VAL_BITS))
        | ((cycles & 0xF) << VAL_BITS)
        | (value & 0xFFFF)
    )


def cycles_of(packed: int) -> int:
    return (packed >> VAL_BITS) & 0xF


def sequence_of(packed: int) -> int:
    return (packed >> (CYC_BITS + VAL_BITS)) & 0xF


def value_of(packed: int) -> int:
    return packed & 0xFFFF


def pack_result(ready: int, available: int, head: int) -> int:
    return (
        ((ready & 1) << (RESULT_BITS - 1))
        | ((available & 1) << TOKEN_BITS)
        | (head & TOKEN_MASK)
    )


# ---------------------------------------------------------------------------
# 2. Queue primitive (IF-03/IF-04, ST-06, oracle.md sect. 3.3)
# ---------------------------------------------------------------------------


class Queue:
    """A bounded queue with ``latency`` and the frozen ``local_occupancy`` rule.

    * ``ready`` is pure *pre-edge* occupancy (``count < depth``), so a slot freed on
      edge E is **not** reusable on E (ST-06, PM-ratified fail-safe reading of the
      order-dependent historical rule; LEDGER sect. 55 / E-49 / E-50).
    * a token pushed on edge ``k`` becomes visible at edge ``k + latency`` -- the
      next epoch's Work for ``latency = 1`` (sect. 3.3).
    """

    def __init__(self, depth: int, latency: int) -> None:
        self.depth = depth
        self.latency = latency
        self.items: list[tuple[int, int]] = []  # (data, first edge visible)

    def count(self) -> int:
        return len(self.items)

    def ready(self) -> bool:
        return len(self.items) < self.depth

    def visible(self, edge: int) -> bool:
        return bool(self.items) and self.items[0][1] <= edge

    def head(self, edge: int) -> int:
        return self.items[0][0] if self.visible(edge) else 0

    def commit(self, edge: int, push: bool, data: int, pop: bool) -> None:
        if pop:
            assert self.items and self.items[0][1] <= edge, "pop of an invisible head"
            self.items.pop(0)
        if push:
            assert len(self.items) < self.depth, "push into a full queue"
            self.items.append((data, edge + self.latency))


class Slot:
    __slots__ = ("remaining", "data")

    def __init__(self, remaining: int, data: int) -> None:
        self.remaining = remaining
        self.data = data


# ---------------------------------------------------------------------------
# 3. Parametric credit-window model
# ---------------------------------------------------------------------------

KNOBS = {
    "slots": SLOTS,
    "in_depth": DEPTH,
    "out_depth": DEPTH,
    "in_latency": LATENCY,
    "out_latency": LATENCY,
    "admit_index": "lowest",  # ST-02
    "retire_index": "lowest",  # OUT-04
    "cost_field": "cycles",  # IF-06
    "countdown": 1,  # ST-04
    "countdown_on_admit": False,  # ST-03
    "reuse_released": False,  # ST-06
    "release_on_done": False,  # ST-05 / BP-01
    "drop_output": False,  # OUT-05
    "accept_zero_cost": False,  # BD-01 / NEG-05
    "skip_zero_cost_head": False,  # C-2 (explicitly forbidden)
    "rollback_on_fail": True,  # whole-system checking precedes every commit
    "ready_credit_predicate": False,  # T-03 trap
    "sink_per_epoch": 1,  # IF-07
    "reset_clears": True,  # ST-08
    "unk01": "gfsim",  # effective cost check requires an old-Q free slot
}


def knobs(**overrides) -> dict:
    return dict(KNOBS, **overrides)


def cost_of(packed: int, field: str) -> int:
    if field == "cycles":
        return cycles_of(packed)
    if field == "sequence":
        return sequence_of(packed)
    if field == "value_low4":
        return value_of(packed) & 0xF
    if field == "one":
        return 1
    raise ValueError(field)


class State:
    def __init__(self, k: dict) -> None:
        self.issued = Queue(k["in_depth"], k["in_latency"])
        self.completed = Queue(k["out_depth"], k["out_latency"])
        self.slots: list[Slot | None] = [None] * k["slots"]
        self.pending: list[int] = []  # retired-but-not-yet-pushed output data
        self.dropped = 0


def read(state: State, epoch: int, valid: int, take: int, data: int, k: dict) -> dict:
    """One epoch's combinational truth, read entirely from the pre-edge state."""
    issued, completed, slots = state.issued, state.completed, state.slots
    n = k["slots"]

    out_valid = issued.visible(epoch)
    head = issued.head(epoch)
    cost = cost_of(head, k["cost_field"])
    capacity_ready = issued.ready()

    free = [slot is None for slot in slots]
    done = [slot is not None and slot.remaining == 0 for slot in slots]
    active = [slot is not None and slot.remaining > 0 for slot in slots]

    # An available zero-cost head is checked only when an old-Q slot is free.
    # Failed whole-system checking discards every proposed queue/slot update.
    # ``safe`` is the "every accepted runtime cost must be positive" obligation
    # (BD-01/BD-04); full slots defer inspection, including on a retirement edge.
    failure = bool(out_valid) and cost == 0 and not k["accept_zero_cost"]
    failure_raised = failure and (any(free) if k["unk01"] == "gfsim" else True)
    safe = not failure
    if k["rollback_on_fail"] and failure_raised:
        active = [False] * n

    # --- retirement (Work step 1) -----------------------------------------
    space = completed.ready()
    retire = [False] * n
    order = (
        list(range(n - 1, -1, -1)) if k["retire_index"] == "highest" else list(range(n))
    )
    if k["retire_index"] == "lowest_cost":
        order = sorted(
            range(n), key=lambda i: (slots[i].remaining if slots[i] else 0, i)
        )
    gate = True if k["release_on_done"] else space
    if gate and not (k["rollback_on_fail"] and failure_raised):
        for index in order:
            if done[index]:
                retire[index] = True
                break

    if k["reuse_released"]:  # rival: a slot freed on this edge is reusable on it
        free = [free[i] or retire[i] for i in range(n)]

    # --- admission (Work step 3) ------------------------------------------
    any_free = any(free)
    admit = bool(out_valid) and any_free and safe
    skip = bool(k["skip_zero_cost_head"]) and bool(out_valid) and cost == 0 and any_free
    admit_at = [False] * n
    if admit:
        admit_order = range(n) if k["admit_index"] == "lowest" else range(n - 1, -1, -1)
        for index in admit_order:
            if free[index]:
                admit_at[index] = True
                break

    # --- outputs (OUT-01..OUT-06) -----------------------------------------
    pending_push = bool(state.pending) or any(retire)
    push = bool(pending_push) and space and not k["drop_output"]
    available = completed.visible(epoch)
    sink_pops = min(
        k["sink_per_epoch"] if (take and available) else 0, completed.count()
    )

    ready_out = (any_free and safe) if k["ready_credit_predicate"] else capacity_ready
    head_out = completed.head(epoch)
    return {
        "out_valid": bool(out_valid),
        "head": head,
        "cost": cost,
        "capacity_ready": capacity_ready,
        "ready": bool(ready_out),
        "available": bool(available),
        "head_out": head_out,
        "free": free,
        "done": done,
        "active": active,
        "space": space,
        "retire": retire,
        "admit_at": admit_at,
        "admit": admit,
        "skip": skip,
        "failure": failure,
        "failure_raised": failure_raised,
        "pop_issued": admit or skip,
        "push": push,
        "sink_pops": sink_pops,
        "expected": pack_result(int(bool(ready_out)), int(bool(available)), head_out),
    }


def commit(
    state: State,
    epoch: int,
    valid: int,
    take: int,
    data: int,
    r: dict,
    k: dict,
    trace: dict | None,
    section: str,
) -> None:
    if r.get("reset") and k["reset_clears"]:
        state.issued = Queue(k["in_depth"], k["in_latency"])
        state.completed = Queue(k["out_depth"], k["out_latency"])
        state.slots = [None] * k["slots"]
        state.pending = []
        if trace is not None:
            trace["resets"].append(epoch)
        return

    if r["failure_raised"] and k["rollback_on_fail"]:
        return

    state.issued.commit(
        epoch, bool(valid) and r["capacity_ready"], data, r["pop_issued"]
    )
    for index in range(k["slots"]):
        if r["retire"][index]:
            slot = state.slots[index]
            assert slot is not None
            state.pending.append(slot.data)
    pushed = state.pending.pop(0) if r["push"] else None
    if not r["push"] and k["drop_output"] and state.pending:
        state.dropped += len(state.pending)
        state.pending = []

    for _ in range(r["sink_pops"]):
        state.completed.commit(epoch, False, 0, True)
    if r["push"]:
        state.completed.commit(epoch, True, pushed, False)

    for index in range(k["slots"]):
        if r["admit_at"][index]:
            remaining = r["cost"]
            if k["countdown_on_admit"] and remaining > 0:
                remaining -= 1
            state.slots[index] = Slot(remaining, r["head"])
        elif r["retire"][index]:
            state.slots[index] = None
        elif r["active"][index]:
            slot = state.slots[index]
            assert slot is not None
            slot.remaining = max(0, slot.remaining - k["countdown"])


# ---------------------------------------------------------------------------
# 4. Stimulus (A06-T's own; independent of the O and S stimulus sets)
# ---------------------------------------------------------------------------
#
# ``drive``: "handshake" offers a token only while the issued queue reports
# capacity (the historical testbench's own rule); "overdrive" offers it
# unconditionally so that the capacity contract is witnessed directly.
# ``reset_at`` inserts one synchronous reset epoch (rst=1, valid=0, take=0) whose
# sampled value is still the pre-reset state.


def program() -> list[dict]:
    return [
        {
            # C1: unequal costs -> completion order != arrival order, and epochs 11
            # and 15 have BOTH slots done at once, which is the only shape in which
            # ST-02 (admission index) and OUT-04 (retire index) become observable.
            "name": "C1",
            "tokens": [
                (0, 5, 0x0A01),
                (1, 1, 0x0A02),
                (2, 4, 0x0A03),
                (3, 1, 0x0A04),
                (4, 2, 0x0A05),
                (5, 1, 0x0A06),
            ],
            "low": (),
            "drive": "handshake",
            "epochs": 20,
        },
        {
            # C2: available zero-cost head with a free slot fails before
            # the sibling slot countdown, input push, or any other commit.
            "name": "C2",
            "tokens": [(0, 3, 0x0B01), (1, 0, 0x0B02), (2, 1, 0x0B03), (3, 2, 0x0B04)],
            "low": (),
            "drive": "handshake",
            "epochs": 16,
        },
        {
            # C3: the UNK-01 discriminating shape -- a cost == 0 head while BOTH
            # slots are occupied (epochs 4..8).
            "name": "C3",
            "tokens": [(0, 4, 0x0C01), (1, 4, 0x0C02), (2, 0, 0x0C03)],
            "low": (),
            "drive": "handshake",
            "epochs": 14,
        },
        {
            # C4: long output backpressure: `completed` fills to depth 4 while both
            # slots hold a done token, then drains 1/epoch.
            "name": "C4",
            "tokens": [(i, 1, 0x0E00 + i) for i in range(6)],
            "low": tuple(range(5, 17)),
            "drive": "handshake",
            "epochs": 26,
        },
        {
            # C5: input saturation at depth 4 plus the width / cost upper bounds:
            # cost 15, sequence 15, value 0xFFFF.
            "name": "C5",
            "tokens": [
                (0, 2, 0x0D01),
                (15, 1, 0xFFFF),
                (2, 15, 0x0D03),
                (3, 1, 0x0D04),
                (4, 1, 0x0D05),
                (5, 1, 0x0D06),
                (6, 1, 0x0D07),
                (7, 1, 0x0D08),
            ],
            "low": (),
            "drive": "handshake",
            "epochs": 32,
        },
        {
            # C6: one synchronous reset in mid-flight (ST-08 / OUT-12): both slots
            # and both queues must be empty afterwards and the design must restart.
            "name": "C6",
            "tokens": [(i, 2, 0x0F00 + i) for i in range(6)],
            "low": (),
            "drive": "handshake",
            "epochs": 20,
            "reset_at": 8,
        },
        {
            # C7: overdrive -- `valid` held high while `ready` is low, so the
            # capacity contract (IF-03, ready == count < 4) is witnessed directly.
            "name": "C7",
            "tokens": [(i, 3, 0x1000 + i) for i in range(6)],
            "low": (),
            "drive": "overdrive",
            "epochs": 20,
        },
        {
            # Additional admission demand while completed is full distinguishes
            # early credit return from holding credit until the output push.
            "name": "C8",
            "tokens": [(i, 1, 0x1100 + i) for i in range(12)],
            "low": tuple(range(5, 25)),
            "drive": "overdrive",
            "epochs": 44,
        },
    ]


WEAK = {
    # P2 probe stimulus: equal costs only, so no two slots can ever be done in the
    # same epoch and the admission index is unobservable.
    "name": "W",
    "tokens": [(i, 2, 0x2000 + i) for i in range(6)],
    "low": (),
    "drive": "handshake",
    "epochs": 18,
}


def state_snapshot(state: State) -> tuple:
    """All owned mutable state, including token values and latency deadlines."""
    return (
        tuple(state.issued.items),
        tuple(state.completed.items),
        tuple(
            None if slot is None else (slot.remaining, slot.data)
            for slot in state.slots
        ),
        tuple(state.pending),
        state.dropped,
    )


def run_section(section: dict, k: dict, driven_program=None, offset: int = 0):
    """Run one section; return (rows, trace, driven-entries-of-this-section)."""
    state = State(k)
    trace = {"events": [], "resets": []}
    rows: list[dict] = []
    driven: list[dict] = []
    cursor = 0
    tokens = section["tokens"]
    for epoch in range(1, section["epochs"] + 1):
        reset = 1 if section.get("reset_at") == epoch else 0
        if driven_program is not None:
            fixed = driven_program[offset + epoch - 1]
            valid, take, data = fixed["valid"], fixed["take"], fixed["data"]
            reset = fixed["rst"]
        else:
            take = 0 if epoch in section["low"] else 1
            held = cursor < len(tokens)
            data = pack_token(*tokens[cursor]) if held else 0
            if reset or not held:
                valid = 0
            elif section["drive"] == "overdrive":
                valid = 1
            else:
                valid = 1 if state.issued.ready() else 0
            driven.append(
                {
                    "section": section["name"],
                    "epoch": epoch,
                    "rst": reset,
                    "valid": valid,
                    "take": take,
                    "data": data,
                }
            )

        r = read(state, epoch, valid, take, data, k)
        r["reset"] = bool(reset)
        for clk in (0, 1):
            rows.append(
                {
                    "clk": clk,
                    "rst": reset,
                    "valid": valid,
                    "take": take,
                    "data": data,
                    "expected": r["expected"],
                    "expected_failure": bool(r["failure_raised"] and not reset),
                }
            )

        before_state = state_snapshot(state)
        before_remaining = [None if s is None else s.remaining for s in state.slots]
        before_data = [None if s is None else s.data for s in state.slots]
        before_issued, before_completed = state.issued.count(), state.completed.count()
        commit(state, epoch, valid, take, data, r, k, trace, section["name"])
        failed = r["failure_raised"] and k["rollback_on_fail"] and not reset
        accepted = bool(valid) and r["capacity_ready"] and not reset and not failed
        if accepted:
            cursor += 1
        trace["events"].append(
            {
                "section": section["name"],
                "epoch": epoch,
                "valid": valid,
                "take": take,
                "accepted": accepted,
                "cursor": cursor,
                "reset": bool(reset),
                "reset_committed": bool(r.get("reset")) and k["reset_clears"],
                "admit_at": [
                    i for i in range(k["slots"]) if r["admit_at"][i] and not failed
                ],
                "retire_at": [
                    i for i in range(k["slots"]) if r["retire"][i] and not failed
                ],
                "push": r["push"] and not failed,
                "space": r["space"],
                "cost": r["cost"],
                "head": r["head"],
                "out_valid": r["out_valid"],
                "ready": r["ready"],
                "available": r["available"],
                "head_out": r["head_out"],
                "done": r["done"],
                "active": r["active"],
                "free": r["free"],
                "failure": r["failure"],
                "failure_raised": r["failure_raised"],
                "committed": not failed,
                "state_before": before_state,
                "state_after": state_snapshot(state),
                "sink": r["head_out"] if r["sink_pops"] and not failed else None,
                "sink_pops": r["sink_pops"],
                "issued_before": before_issued,
                "completed_before": before_completed,
                "issued_count": state.issued.count(),
                "completed_count": state.completed.count(),
                "remaining_before": before_remaining,
                "remaining_after": [
                    None if s is None else s.remaining for s in state.slots
                ],
                "data_before": before_data,
                "data_after": [None if s is None else s.data for s in state.slots],
            }
        )
    return rows, trace, driven


def build(k: dict | None = None):
    """Run the whole delivered program; return (rows, traces, driven timeline)."""
    k = k or knobs()
    rows: list[dict] = []
    traces: dict[str, dict] = {}
    driven: list[dict] = []
    for section in program():
        section_rows, trace, section_driven = run_section(section, k)
        rows.extend(section_rows)
        traces[section["name"]] = trace
        driven.extend(section_driven)
    return rows, traces, driven


def replay(driven: list[dict], k: dict):
    """Re-run the model on already-fixed stimulus columns.

    A rival can therefore only differ in *behaviour*: the columns the DUT is driven
    with are held constant.
    """
    rows, traces, fixed = [], {}, []
    offset = 0
    while offset < len(driven):
        name = driven[offset]["section"]
        end = offset + 1
        while end < len(driven) and driven[end]["section"] == name:
            end += 1
        section = {
            "name": name,
            "tokens": [],
            "low": (),
            "drive": "handshake",
            "epochs": end - offset,
        }
        section_rows, trace, _ = run_section(
            section, k, driven_program=driven, offset=offset
        )
        rows.extend(section_rows)
        traces[name] = trace
        offset = end
    return rows, traces, fixed


def events_of(traces: dict) -> list[dict]:
    return [event for trace in traces.values() for event in trace["events"]]


def rival_rows(driven: list[dict], k: dict) -> list[int]:
    rows, _, _ = replay(driven, k)
    return [row["expected"] for row in rows]


# ---------------------------------------------------------------------------
# 5. Independent hand arithmetic (oracle.md sect. 3.3, computed by hand)
# ---------------------------------------------------------------------------
#
# release(admission, cost) = admission + cost + 1   (ST-03: the admission edge does
#                                                    not count down, then `cost`
#                                                    further Xfers reach 0)
# sink(admission, cost)    = admission + cost + 2   (the push commits on the release
#                                                    edge; latency 1 makes it visible
#                                                    one epoch later)
ARITHMETIC = [
    # section, sequence, admission epoch, cost, release epoch, sink epoch
    ("C1", 1, 3, 1, 5, 6),
    ("C1", 0, 2, 5, 8, 9),
    ("C1", 3, 9, 1, 11, 12),
    ("C1", 2, 6, 4, 12, 13),
    ("C1", 4, 12, 2, 15, 16),
    ("C1", 5, 13, 1, 16, 17),
    ("C4", 0, 2, 1, 4, 17),
]


# ---------------------------------------------------------------------------
# 6. Documented-table replay: oracle.md sect. 6 (OUT-07..OUT-11)
# ---------------------------------------------------------------------------

O_S1 = [
    (0, 8, 0x0111),
    (1, 1, 0x0222),
    (2, 2, 0x0333),
    (3, 1, 0x0444),
    (4, 3, 0x0555),
    (5, 1, 0x0666),
]
O_S4 = [(i, 1, 0x0100 + i) for i in range(6)]
O_S5 = [(0, 6, 0x0111), (1, 0, 0x0222), (2, 1, 0x0333)]
O_S6 = [(15 if i == 1 else i, 8, 0xFFFF if i == 1 else 0x0100 + i) for i in range(12)]

# Transcribed by hand from A06/oracle.md sect. 6 and A06/mapping.json.
O_TABLES = {
    "S1": {
        "tokens": O_S1,
        "low": (),
        "epochs": 20,
        "admit": {2: (0, 0), 3: (1, 1), 6: (1, 2), 10: (1, 3), 12: (0, 4), 13: (1, 5)},
        "release": {0: 11, 1: 5, 2: 9, 3: 12, 4: 16, 5: 15},
        "sink": {6: 1, 10: 2, 12: 0, 13: 3, 16: 5, 17: 4},
    },
    "S2": {
        "tokens": O_S1,
        "low": tuple(range(9, 15)),
        "epochs": 22,
        "admit": {2: (0, 0), 3: (1, 1), 6: (1, 2), 10: (1, 3), 12: (0, 4), 13: (1, 5)},
        "release": {0: 11, 1: 5, 2: 9, 3: 12, 4: 16, 5: 15},
        "sink": {6: 1, 15: 2, 16: 0, 17: 3, 18: 5, 19: 4},
    },
    "S4": {
        "tokens": O_S4,
        "low": tuple(range(5, 21)),
        "epochs": 32,
        "admit": {2: (0, 0), 3: (1, 1), 5: (0, 2), 6: (1, 3), 8: (0, 4), 9: (1, 5)},
        "release": {0: 4, 1: 5, 2: 7, 3: 8, 4: 22, 5: 23},
        "sink": {21: 0, 22: 1, 23: 2, 24: 3, 25: 4, 26: 5},
    },
    "S5": {
        "tokens": O_S5,
        "low": (),
        "epochs": 16,
        "admit": {2: (0, 0)},
        "release": {},
        "sink": {},
        "historical_partial_commit_release": {0: 9},
        "historical_partial_commit_sink": {10: 0},
        "first_failure": 3,
    },
    "S6": {
        "tokens": O_S6,
        "low": (),
        "epochs": 72,
        "admit": {
            2: (0, 0),
            3: (1, 1),
            12: (0, 2),
            13: (1, 3),
            22: (0, 4),
            23: (1, 5),
            32: (0, 6),
            33: (1, 7),
            42: (0, 8),
            43: (1, 9),
            52: (0, 10),
            53: (1, 11),
        },
        "release": {
            0: 11,
            1: 12,
            2: 21,
            3: 22,
            4: 31,
            5: 32,
            6: 41,
            7: 42,
            8: 51,
            9: 52,
            10: 61,
            11: 62,
        },
        "sink": {
            12: 0,
            13: 1,
            22: 2,
            23: 3,
            32: 4,
            33: 5,
            42: 6,
            43: 7,
            52: 8,
            53: 9,
            62: 10,
            63: 11,
        },
    },
}


def witnesses_of(trace: dict, tokens=None) -> dict:
    token_ids = (
        {pack_token(*token): index for index, token in enumerate(tokens)}
        if tokens is not None
        else {}
    )

    def token_id(token):
        return token_ids[token] if tokens is not None else sequence_of(token)

    admit, release, sink = {}, {}, {}
    for event in trace["events"]:
        for index in event["admit_at"]:
            admit[event["epoch"]] = (index, token_id(event["head"]))
        for index in event["retire_at"]:
            release[token_id(event["data_before"][index])] = event["epoch"]
        if event["sink"] is not None:
            sink[event["epoch"]] = token_id(event["sink"])
    return {"admit": admit, "release": release, "sink": sink}


def _documented_tables() -> dict:
    out = {}
    for name, table in O_TABLES.items():
        section = {
            "name": name,
            "tokens": table["tokens"],
            "low": table["low"],
            "drive": "handshake",
            "epochs": table["epochs"],
        }
        _, trace, _ = run_section(section, knobs())
        got = witnesses_of(trace, table["tokens"])
        for key in ("admit", "release", "sink"):
            assert got[key] == table[key], (name, key, got[key], table[key])
        if "first_failure" in table:
            failed = [e for e in trace["events"] if e["failure_raised"]]
            assert failed[0]["epoch"] == table["first_failure"]
            assert all(e["state_before"] == e["state_after"] for e in failed)
        out[name] = {key: len(got[key]) for key in ("admit", "release", "sink")}
    return out


# ---------------------------------------------------------------------------
# 7. Rival / negative-control battery
# ---------------------------------------------------------------------------


def depth2_fifo_rows(driven: list[dict]) -> list[int]:
    """NEG-01: a plain depth-2 FIFO that mistakes `credits` for a queue depth."""
    fifo = Queue(2, 1)
    out: list[int] = []
    for entry in driven:
        epoch = entry["epoch"]
        if entry["rst"]:
            fifo = Queue(2, 1)
            out.extend([pack_result(1, 0, 0)] * 2)
            continue
        ready = fifo.ready()
        available = fifo.visible(epoch)
        expected = pack_result(int(ready), int(available), fifo.head(epoch))
        fifo.commit(
            epoch,
            bool(entry["valid"]) and ready,
            entry["data"],
            bool(entry["take"]) and available,
        )
        out.extend([expected] * 2)
    return out


def rival_table(
    driven: list[dict], expected: list[int], weak: list[dict], weak_expected: list[int]
) -> dict:
    rivals: dict[str, dict] = {}

    def add(label, rows, note, must_differ=True, reference=None):
        reference = expected if reference is None else reference
        differing = sum(1 for a, b in zip(reference, rows, strict=True) if a != b)
        rivals[label] = {
            "differing_rows": differing,
            "must_differ": must_differ,
            "note": note,
        }

    add(
        "NEG-01 depth2_fifo",
        depth2_fifo_rows(driven),
        "credits mistaken for a queue depth (plain depth-2 FIFO)",
    )
    add(
        "NEG-02 cost_sorted",
        rival_rows(driven, knobs(slots=8, retire_index="lowest_cost")),
        "unbounded window, output sorted by cost",
    )
    add(
        "NEG-03 no_window",
        rival_rows(driven, knobs(slots=8)),
        "the 2-credit window ignored (admit on arrival)",
    )
    add(
        "NEG-04 fixed_cost_one",
        rival_rows(driven, knobs(cost_field="one")),
        "no per-slot cost countdown (constant 1-epoch cost)",
    )
    add(
        "NEG-05 accept_cost_zero",
        rival_rows(driven, knobs(accept_zero_cost=True)),
        "cycles == 0 accepted and emitted",
    )
    add(
        "NEG-06 same_edge_reuse",
        rival_rows(driven, knobs(reuse_released=True)),
        "a slot released on edge E is refilled on E",
    )
    add(
        "NEG-07 early_release",
        rival_rows(driven, knobs(release_on_done=True)),
        "credit returned when done, before the output push commits",
    )
    add(
        "NEG-08 high_done_index",
        rival_rows(driven, knobs(retire_index="highest")),
        "completion tie-break uses the highest slot index",
    )
    add(
        "R09 admit_high_index",
        rival_rows(driven, knobs(admit_index="highest")),
        "admission targets the highest free index (ST-02 isolating)",
    )
    add(
        "R10 countdown_on_admit",
        rival_rows(driven, knobs(countdown_on_admit=True)),
        "the admission edge also counts down (ST-03)",
    )
    add(
        "R11 no_countdown",
        rival_rows(driven, knobs(countdown=0)),
        "remaining never decrements",
    )
    add(
        "R12 double_countdown",
        rival_rows(driven, knobs(countdown=2)),
        "two decrements per edge (ST-04)",
    )
    add(
        "R13 cost_from_sequence",
        rival_rows(driven, knobs(cost_field="sequence")),
        "cost read from the wrong payload field (IF-06)",
    )
    add(
        "R14 cost_from_value",
        rival_rows(driven, knobs(cost_field="value_low4")),
        "cost read from `value` (IF-06)",
    )
    add(
        "R15 issued_depth_3",
        rival_rows(driven, knobs(in_depth=3)),
        "issued depth 3 (IF-03)",
    )
    add(
        "R16 completed_depth_3",
        rival_rows(driven, knobs(out_depth=3)),
        "completed depth 3 (IF-04)",
    )
    add("R17 window_3", rival_rows(driven, knobs(slots=3)), "credits == 3 (IF-05)")
    add("R18 window_1", rival_rows(driven, knobs(slots=1)), "credits == 1 (IF-05)")
    add(
        "R19 ready_is_credit",
        rival_rows(driven, knobs(ready_credit_predicate=True)),
        "the module `ready` collapsed onto the credit predicate (T-03)",
    )
    add(
        "R20 pop_two_per_epoch",
        rival_rows(driven, knobs(sink_per_epoch=2)),
        "two consumer pops per epoch (IF-07/OUT-06)",
    )
    add(
        "R21 skip_zero_cost_head",
        rival_rows(driven, knobs(skip_zero_cost_head=True, rollback_on_fail=False)),
        "the cost == 0 head is skipped instead of stalling (C-2 violation)",
    )
    add(
        "R22 commit_on_failure",
        rival_rows(driven, knobs(rollback_on_fail=False)),
        "the failing epoch commits sibling countdown/queue updates",
    )
    add(
        "R23 ignore_reset",
        rival_rows(driven, knobs(reset_clears=False)),
        "reset ignored (ST-08)",
    )
    add(
        "R24 drop_when_blocked",
        rival_rows(driven, knobs(release_on_done=True, drop_output=True)),
        "a completed token is dropped when the output is blocked (OUT-05/BP-01)",
    )
    add(
        "R25 completed_latency_2",
        rival_rows(driven, knobs(out_latency=2)),
        "completed visibility delayed by one epoch (IF-04)",
    )
    add(
        "R26 issued_latency_2",
        rival_rows(driven, knobs(in_latency=2)),
        "issued visibility delayed by one epoch (IF-03)",
    )

    # Unfalsifiability probes: must NOT separate.
    add(
        "R27 failure_without_free",
        rival_rows(driven, knobs(unk01="pyc")),
        "cost-zero inspection performed while all old-Q slots are occupied",
    )
    add(
        "P2 admit_high_index_on_equal_costs",
        rival_rows(weak, knobs(admit_index="highest")),
        "R09 on an all-equal-cost stimulus: no two slots can complete together, so "
        "the admission index is invisible -- ST-02 is falsifiable *only* through a "
        "simultaneous-completion shape such as C1 epochs 11/15",
        False,
        reference=weak_expected,
    )
    assert weak_expected == rival_rows(weak, knobs()), "weak stimulus replay mismatch"
    return rivals


# ---------------------------------------------------------------------------
# 8. Contract checks over the frozen trace
# ---------------------------------------------------------------------------


def contract_checks(rows, traces, driven) -> dict:
    w: dict[str, list[str]] = {}

    def note(key, text):
        w.setdefault(key, []).append(text)

    events = events_of(traces)
    by_section = {name: trace["events"] for name, trace in traces.items()}
    assert len(rows) == 2 * len(events)

    # --- ST-01 ------------------------------------------------
    for event in events:
        assert len(event["admit_at"]) <= 1 and len(event["retire_at"]) <= 1, event
    note("ST-01", f"{len(events)} epochs; no epoch has 2 admissions or 2 retirements")
    note(
        "OUT-02",
        f"{sum(1 for e in events if e['sink'] is not None)} sink observations, "
        f"never 2 in one epoch",
    )
    note("IF-07", "the consumer pops at most once per epoch")

    # --- ST-02 ------------------------------------------------
    simultaneous = [e for e in events if sum(1 for d in e["done"] if d) >= 2]
    assert simultaneous, "no epoch with two simultaneously done slots"
    for event in events:
        for index in event["admit_at"]:
            free_indices = [i for i, flag in enumerate(event["free"]) if flag]
            assert index == min(free_indices), event
    note(
        "ST-02",
        "admission always targets the lowest free index; observable only at "
        f"simultaneous-done epochs {[e['epoch'] for e in simultaneous]} "
        f"(section {simultaneous[0]['section']})",
    )

    # --- ST-03 ------------------------------------------------
    for event in events:
        for index in event["admit_at"]:
            assert event["remaining_after"][index] == event["cost"], event
    note("ST-03", "after every admission edge remaining == cost (no countdown)")

    # --- ST-04 ------------------------------------------------
    for event in events:
        for index in range(SLOTS):
            before, after = (
                event["remaining_before"][index],
                event["remaining_after"][index],
            )
            if index in event["admit_at"]:
                continue
            if index in event["retire_at"]:
                assert after is None, event
            elif after is not None and before is not None:
                assert after == (
                    before - 1 if event["committed"] and before > 0 else before
                ), event
    note("ST-04", "every occupied, not-done slot decrements exactly once per edge")

    # --- ST-05 / BP-01 ---------------------------------------
    for event in events:
        if event["retire_at"]:
            assert event["push"], event
    note("ST-05", "no retirement without a committing push (completed gating)")

    # --- ST-06 ------------------------------------------------
    for event in events:
        assert not (set(event["admit_at"]) & set(event["retire_at"])), event
    note("ST-06", "no epoch admits into a slot it retires on the same edge")

    # --- ST-07 ------------------------------------------------
    both = [e for e in events if e["admit_at"] and e["retire_at"]]
    assert both, "no concurrent admit+retire epoch"
    note(
        "ST-07",
        f"concurrent admission+retirement at epochs "
        f"{[(e['section'], e['epoch']) for e in both]}",
    )

    # --- ST-08 / OUT-12 --------------------------------------
    for name, section_events in by_section.items():
        for event in section_events:
            if not event["reset_committed"]:
                continue
            assert event["remaining_after"] == [None] * SLOTS, event
            assert event["issued_count"] == 0 and event["completed_count"] == 0, event
            note(
                "ST-08",
                f"{name}: reset edge at epoch {event['epoch']} clears both "
                f"slots, `issued` and `completed`",
            )
    note(
        "ST-08",
        "reset clears queue/slot state; runtime diagnostics retain their generic source-check identity",
    )

    # --- IF-03/IF-04/BD-08/BD-09 ------------------------------
    for event in events:
        assert event["issued_before"] <= DEPTH and event["completed_before"] <= DEPTH
        assert event["issued_count"] <= DEPTH and event["completed_count"] <= DEPTH
    low_ready = [e for e in events if not e["ready"]]
    assert low_ready, "the stimulus never witnesses ready == 0"
    note(
        "IF-03",
        f"ready == 0 first at {low_ready[0]['section']} epoch "
        f"{low_ready[0]['epoch']} with issued count {low_ready[0]['issued_before']}",
    )
    note("IF-04", "completed never buffers more than 4 tokens")
    note("BD-08", f"{len(low_ready)} epochs refuse a push while issued is at depth 4")
    note("BD-09", "completed saturation blocks retirement (C4/C5)")

    # --- IF-05 ------------------------------------------------
    for event in events:
        occupied = sum(1 for r in event["remaining_before"] if r is not None)
        assert occupied <= SLOTS, event
    note("IF-05", "never more than 2 occupied credit slots")

    # --- T-03 / Q-D1(a) --------------------------------------
    stalled = [
        e
        for e in events
        if e["ready"] and e["out_valid"] and not e["admit_at"] and not e["failure"]
    ]
    assert stalled, "no epoch witnesses ready=1 with a blocked admission"
    note(
        "T-03",
        f"{len(stalled)} epochs have ready=1 with a valid head and no "
        f"admission: the module ready is queue capacity, not the credit predicate",
    )

    # --- BD-01 / BD-04 / C-2 ---------------------------------
    zero = [e for e in events if e["out_valid"] and e["cost"] == 0]
    assert zero, "the stimulus never presents a cost == 0 head"
    for event in zero:
        assert not event["admit_at"] and event["sink"] is None, event
    note(
        "BD-01",
        f"cost == 0 head on {len(zero)} epochs ({zero[0]['section']} from "
        f"epoch {zero[0]['epoch']}): never admitted, never consumed",
    )
    note("C-2", "effective zero-cost checks terminate execution; no post-failure Xfer")
    note("BD-04", "cycles == 0 is the only non-positive u4 value and never admits")

    # First failure is derived before any DUT execution; every failed edge has
    # identical complete pre/post state. Full section durations remain intact.
    for name, first_epoch in (("C2", 3), ("C3", 8)):
        failed = [e for e in by_section[name] if e["failure_raised"]]
        assert failed[0]["epoch"] == first_epoch, (name, failed[0])
        for event in failed:
            assert event["state_before"] == event["state_after"], event
            assert (
                not event["accepted"]
                and not event["admit_at"]
                and not event["retire_at"]
            )
            assert event["sink"] is None and not event["push"]
    deferred = [e for e in by_section["C3"] if e["failure"] and not e["failure_raised"]]
    assert [e["epoch"] for e in deferred] == [4, 5, 6, 7]
    assert all(not any(e["free"]) and e["committed"] for e in deferred)
    note(
        "BD-02",
        "C2 fails at epoch 3, C3 defers 4..7 and fails at 8; all failed state is unchanged",
    )

    # --- BD-06 ------------------------------------------------
    both_busy = [
        e for e in events if all(r is not None and r > 0 for r in e["remaining_before"])
    ]
    assert both_busy and not any(e["admit_at"] for e in both_busy)
    note(
        "BD-06",
        f"{len(both_busy)} epochs with both slots counting down: no admission, "
        f"both countdowns continue",
    )

    # --- BD-07 / BP-01 / BD-09 --------------------------------
    frozen_window = [
        e
        for e in by_section["C4"]
        if e["remaining_before"] == [0, 0] and not e["space"]
    ]
    assert frozen_window, "no frozen backpressure window"
    epochs = [e["epoch"] for e in frozen_window]
    assert epochs == list(range(min(epochs), max(epochs) + 1)), epochs
    for event in frozen_window:
        assert event["completed_before"] == DEPTH, event
        assert not event["retire_at"] and not event["admit_at"], event
    note(
        "BD-07",
        f"C4 frozen epochs {min(epochs)}..{max(epochs)}: both slots done, "
        f"completed at depth 4, zero releases and zero admissions",
    )
    note("BP-01", "the completed token keeps its slot and its credit while blocked")

    # --- BP-02 / BP-03 / BP-04 --------------------------------
    note("BP-02", "at most 4 completed tokens are buffered (C4/C5)")
    note(
        "BP-03",
        "both slots occupied -> inputReady 0 and admission stops while "
        "`issued` fills to 4 (C4/C5/C7)",
    )
    after = [e for e in by_section["C4"] if e["epoch"] >= max(epochs)]
    drained = [e["epoch"] for e in after if e["sink"] is not None]
    assert drained == list(range(min(drained), min(drained) + len(drained))), drained
    assert len(drained) == 6, drained
    note("BP-04", f"C4 drains one token per epoch at epochs {drained}")

    # --- OUT-01 / OUT-03 / OUT-04 -----------------------------
    emitted = {}
    for event in events:
        for index in event[
            "admit_at"
        ]:  # noqa: B007 - preserve the extracted reference algorithm
            emitted[event["head"]] = event["epoch"]
        if event["sink"] is not None:
            assert event["sink"] in emitted, event
            assert event["sink"] == event["head_out"], event
    note(
        "OUT-01",
        f"all {len(emitted)} admitted tokens are emitted bit-for-bit "
        f"(including sequence 15 / value 0xFFFF)",
    )
    c1_sink = [
        sequence_of(e["sink"]) for e in by_section["C1"] if e["sink"] is not None
    ]
    arrival = [t[0] for t in program()[0]["tokens"]]
    assert c1_sink != arrival
    note(
        "OUT-03",
        f"C1 sink order {c1_sink} != arrival order {arrival} "
        f"(the cost-5 head is overtaken)",
    )
    note(
        "OUT-04",
        f"simultaneous-done epochs {[e['epoch'] for e in simultaneous]} "
        f"emit the lowest done index",
    )
    rate = max(
        sum(1 for e in events if e["section"] == s and e["sink"] is not None)
        for s in by_section
    )
    note(
        "OUT-06", f"at most one output per epoch (max sink count per section = {rate})"
    )

    # --- BD-05 ------------------------------------------------
    boundary = pack_token(15, 1, 0xFFFF)
    admitted = [e["epoch"] for e in events if e["accepted"] and e["head"] == boundary]
    sunk = [e["epoch"] for e in events if e["sink"] == boundary]
    assert admitted and sunk, (admitted, sunk)
    note(
        "BD-05",
        f"sequence 15 / value 0xFFFF admitted at epoch {admitted[0]} and "
        f"emitted at epoch {sunk[0]}",
    )
    assert pack_token(15, 15, 0xFFFF) != boundary

    # --- BP-05 ------------------------------------------------
    note(
        "BP-05",
        "S2 replay keeps S1's receipt order; C4's stall never reorders the "
        "completed FIFO",
    )

    # --- OUT-12 -----------------------------------------------
    for name, section_events in by_section.items():
        if name in ("C2", "C3"):
            continue  # execution terminates at the independently predicted failure
        assert section_events[-1]["remaining_after"] == [None] * SLOTS, name
        assert section_events[-1]["issued_count"] == 0, name
        assert section_events[-1]["completed_count"] == 0, name
    note("OUT-12", "C1/C4/C5/C6/C7 end fully drained (both slots, both queues empty)")

    # --- hand arithmetic (oracle.md sect. 3.3) ----------------
    for section, sequence, admit_epoch, cost, release_epoch, sink_epoch in ARITHMETIC:
        section_events = by_section[section]
        event = section_events[admit_epoch - 1]
        assert event["admit_at"] and event["cost"] == cost, (section, sequence, event)
        token = event["head"]
        assert sequence_of(token) == sequence, (section, sequence, hex(token))
        # Lowest-index ties defer C1 keys 2/5 by one edge; C4 takes low
        # through 16, so its already-buffered key 0 is consumed on 17.
        release_delay = 1 if (section, sequence) in (("C1", 2), ("C1", 5)) else 0
        sink_delay = 12 if (section, sequence) == ("C4", 0) else 0
        assert release_epoch == admit_epoch + cost + 1 + release_delay
        assert sink_epoch == release_epoch + 1 + sink_delay
        released = [
            e["epoch"]
            for e in section_events
            if e["retire_at"] and e["data_before"][e["retire_at"][0]] == token
        ]
        sunk = [e["epoch"] for e in section_events if e["sink"] == token]
        assert released == [release_epoch], (section, sequence, released)
        assert sunk == [sink_epoch], (section, sequence, sunk)
        note(
            "ARITH",
            f"{section} seq {sequence}: admit {admit_epoch} + cost {cost} -> "
            f"release {release_epoch} = admit+cost+1, sink {sink_epoch} = "
            f"admit+cost+2 (hand-computed absolute epochs)",
        )
    return w


# ---------------------------------------------------------------------------
# 9. The 49 frozen assertions: three-state list
# ---------------------------------------------------------------------------
#
#   covered   -- this oracle has a deterministic check that would fail if the
#                assertion's testable content (under the frozen migration) were
#                violated; the witness names it.
#   uncovered -- testable in principle but this oracle has no separating check:
#                a reported gap.
#   na        -- the assertion is about an artifact the migrated root does not have
#                (the retired frontend/verifier rejection list, the gfsim
#                run-terminating failure channel, the historical register
#                realisation), or is superseded by a registered divergence
#                (CD-ENTRY / IF-1 / C-2).

ASSERTIONS = {
    "IF-01": (
        "covered",
        "24-bit payload in declaration order; all sections carry "
        "distinct sequence/cycles/value bits and every emitted head is "
        "compared bit-for-bit; R13/R14 (cost from the wrong field) are "
        "separated",
    ),
    "IF-02": (
        "covered",
        "build-level witness: the harness copies the root, links "
        "`q4_queue.pyc_credit_pipeline.CreditPipeline`, emits both "
        "targets and binds 3 inputs to one 26-bit `result` in the "
        "shared driver (no Q4_MAPPING define).  The historical "
        "`@ac.system` spelling is NA by CD-ENTRY/IF-1",
    ),
    "IF-03": (
        "covered",
        "issued capacity is the module `ready` (T-03 witness); C5/C7 "
        "witness ready == 0 at 4 pending; R15 (depth 3) and R26 "
        "(latency 2) are separated",
    ),
    "IF-04": (
        "covered",
        "C4 holds `completed` at exactly 4 before refusing the next "
        "push; R16 (depth 3) and R25 (latency 2) are separated",
    ),
    "IF-05": (
        "covered",
        "never more than 2 occupied slots and R17/R18 (credits 3/1) "
        "are separated: `credits` is a window, not a queue depth",
    ),
    "IF-06": (
        "covered",
        "cost comes from `cycles` only: cost 15 (C5 token 2), cost 1 "
        "and cost 8 tokens follow the sect. 3.3 arithmetic; R13/R14 "
        "are separated",
    ),
    "IF-07": (
        "covered",
        "never 2 sink observations in one epoch; R20 (two pops per epoch) is separated",
    ),
    "ST-01": (
        "covered",
        "no epoch has 2 admissions or 2 retirements (checked over all "
        "epochs of all 7 sections)",
    ),
    "ST-02": (
        "covered",
        "admission always targets the lowest free index; it is visible "
        "only where two slots complete together (C1 epochs 11/15), "
        "where R09 is separated.  Probe P2 shows it is *invisible* on "
        "an all-equal-cost stimulus: the required shape is supplied",
    ),
    "ST-03": (
        "covered",
        "after every admission edge remaining == cost; R10 "
        "(countdown on admission) is separated",
    ),
    "ST-04": (
        "covered",
        "every occupied, not-done slot decrements exactly once per "
        "edge; R11/R12 are separated",
    ),
    "ST-05": (
        "covered",
        "no retirement without a committing push; C4 holds the credit "
        "across the whole stall; R07/R24 are separated",
    ),
    "ST-06": (
        "covered",
        "no epoch admits into a slot it retires; R06 (same-edge reuse) is separated",
    ),
    "ST-07": (
        "covered",
        "concurrent admission and retirement on different slots at the "
        "epochs listed in the ST-07 witness (e.g. C1 epoch 12)",
    ),
    "ST-08": (
        "covered",
        "the C6 mid-stream reset edge empties both slots and both "
        "queues and the design restarts; R23 (reset ignored) is "
        "separated. Diagnostic identity is the current generic source-check identity",
    ),
    "ST-09": (
        "na",
        "the historical i29 concat(valid,remaining,data) register layout is a "
        "realisation detail, not an observable; the frozen root declares "
        "CreditSlot{valid:u1, remaining:u4, data:CreditToken} (read-only "
        "structural match, no row-level witness is possible)",
    ),
    "OUT-01": (
        "covered",
        "every emitted head equals the packed admitted token "
        "bit-for-bit, for every admitted token",
    ),
    "OUT-02": ("covered", "never two sink observations in one epoch"),
    "OUT-03": (
        "covered",
        "C1's sink order differs from its arrival order: a later, "
        "cheaper token overtakes the cost-5 head",
    ),
    "OUT-04": (
        "covered",
        "the simultaneous-done epochs emit the lowest done index; R08 "
        "(highest index) is separated",
    ),
    "OUT-05": (
        "covered",
        "no push into `completed` without `space`; R24 (drop when "
        "blocked) is separated",
    ),
    "OUT-06": (
        "covered",
        "at most one output per epoch, and C5 spends 14+ epochs "
        "delivering 2 tokens at cost 8; R20 is separated",
    ),
    "OUT-07": (
        "covered",
        "the documented S1 table of oracle.md sect. 6 is re-derived "
        "exactly by this model (admit/release/sink)",
    ),
    "OUT-08": (
        "covered",
        "the documented S2 table (take low 9..14) is re-derived exactly",
    ),
    "OUT-09": (
        "covered",
        "the documented S4 table (take low 5..20; both slots held done "
        "during 11..20; drain 21..26) is re-derived exactly",
    ),
    "OUT-10": (
        "covered",
        "available zero-cost heads fail before Xfer when an old-Q slot is free; "
        "S5 has no committed release or sink after first failure at epoch 3",
    ),
    "OUT-11": (
        "covered",
        "the documented S6 table (2 tokens / 10 epochs, saturation, "
        "sequence 15 / value 0xFFFF intact) is re-derived exactly",
    ),
    "OUT-12": (
        "covered",
        "C6's reset edge and the end state of C1/C4/C5/C6/C7 are both "
        "slots and both queues empty; R23 is separated",
    ),
    "BP-01": (
        "covered",
        "C4 frozen window: slot contents, remaining and completed "
        "occupancy are constant while blocked; R07/R24 are separated",
    ),
    "BP-02": (
        "covered",
        "`completed` never buffers more than 4 tokens; R16 is separated",
    ),
    "BP-03": (
        "covered",
        "with both slots occupied admission stops and `issued` fills to "
        "depth 4 (C4/C5/C7)",
    ),
    "BP-04": ("covered", "C4 drains exactly one token per epoch after the stall"),
    "BP-05": (
        "covered",
        "the documented S2 replay keeps S1's receipt order (backpressure "
        "does not reorder), and C4's stall never reorders the FIFO",
    ),
    "BD-01": (
        "covered",
        "effective cost-zero heads consume nothing; R05 is separated",
    ),
    "BD-02": (
        "covered",
        "whole-system checking discards every queue/slot update on failure; "
        "R22 commit_on_failure is separated",
    ),
    "BD-03": (
        "covered",
        "independent first-failure and zero-commit expectations; actual "
        "hardware execution is established only by the separate system fault gate",
    ),
    "BD-04": (
        "covered",
        "cycles is u4: 0 never admits (C2/C3) and 15 is honoured (C5 "
        "token 2, occupied for 16 epochs)",
    ),
    "BD-05": (
        "covered",
        "the sequence 15 / value 0xFFFF token is admitted and emitted bit-for-bit (C5)",
    ),
    "BD-06": (
        "covered",
        "epochs with both slots still counting down admit nothing and keep counting",
    ),
    "BD-07": (
        "covered",
        "C4 frozen window: both slots done, `completed` full, nothing "
        "released; R07 is separated",
    ),
    "BD-08": (
        "covered",
        "`issued` saturates at 4 and refuses further pushes (C5/C7); the "
        "C7 overdrive section drives `valid` into a full queue and the "
        "refusal is a row-level expectation",
    ),
    "BD-09": ("covered", "`completed` saturates at 4 and stops retirement (C4)"),
    "BD-10": (
        "na",
        "a compile-time rejection of non-positive `credits` belonged to the "
        "retired `ac.credit` verifier; the current frontend has no credit op "
        "(IF-1/UNK-02) and the frozen source spells the constant 2",
    ),
    "BD-11": (
        "na",
        "same: `depth`/`latency` positivity was an `ac.credit` verifier rule; "
        "the frozen source spells the constants 4/1",
    ),
    "BD-12": (
        "na",
        "`cost must yield an integer Var of width <= 64` was an `ac.credit` "
        "cost-region verifier rule; the migrated cost is `head.cycles` (u4)",
    ),
    "BD-13": (
        "na",
        "cost-region purity was an `ac.credit` verifier rule; the migrated "
        "root has no cost region",
    ),
    "BD-14": (
        "na",
        "`ac.credit.yield` termination was an `ac.credit` verifier rule; the "
        "migrated root has no cost region",
    ),
    "BD-15": (
        "na",
        "`ACPY-QUEUE-016 unsupported keyword` was an old-frontend diagnostic; "
        "the migrated root has no `credit()` call",
    ),
    "BD-16": (
        "na",
        "`output queue must match input queue type` was an `ac.credit` "
        "verifier rule; the migrated root names CreditToken on both queues "
        "by construction",
    ),
}


def classify() -> dict:
    counts = {"covered": 0, "uncovered": 0, "na": 0}
    for state, _ in ASSERTIONS.values():
        counts[state] += 1
    counts["total"] = len(ASSERTIONS)
    return counts


# ---------------------------------------------------------------------------
# 10. Driver
# ---------------------------------------------------------------------------


def vectors() -> dict:
    """Rows for the harness (``oracle("credit_independent")``)."""
    rows, traces, _ = build()
    return {
        "input_bits": TOKEN_BITS,
        "output_bits": RESULT_BITS,
        "rows": rows,
        "empty_expected": pack_result(1, 0, 0),
        "host_reset_rows": [
            2 * sum(section["epochs"] for section in program()[:index])
            for index in range(1, len(program()))
        ],
        "first_failures": {
            name: next(
                (e["epoch"] for e in trace["events"] if e["failure_raised"]), None
            )
            for name, trace in traces.items()
        },
    }


def self_test(verbose: bool = True) -> dict:
    rows, traces, driven = build()
    witnesses = contract_checks(rows, traces, driven)
    tables = _documented_tables()

    expected = [row["expected"] for row in rows]
    weak_rows, _, weak_driven = run_section(dict(WEAK), knobs())
    rivals = rival_table(
        driven, expected, weak_driven, [row["expected"] for row in weak_rows]
    )
    for label, data in rivals.items():
        if data["must_differ"]:
            assert data["differing_rows"] > 0, f"vector set cannot separate {label}"
        else:
            assert (
                data["differing_rows"] == 0
            ), f"probe {label} was expected to be indistinguishable"
    assert rival_rows(driven, knobs()) == expected, "frozen model replay mismatch"

    counts = classify()
    report = {
        "rows": len(rows),
        "first_failures": {
            name: next(
                (e["epoch"] for e in trace["events"] if e["failure_raised"]), None
            )
            for name, trace in traces.items()
        },
        "sections": {name: len(trace["events"]) for name, trace in traces.items()},
        "documented_tables": tables,
        "rivals": rivals,
        "rivals_separated": sum(
            1 for v in rivals.values() if v["must_differ"] and v["differing_rows"] > 0
        ),
        "probes_indistinguishable": [
            k for k, v in rivals.items() if not v["must_differ"]
        ],
        "assertions": counts,
        "uncovered": [k for k, v in ASSERTIONS.items() if v[0] == "uncovered"],
        "witnesses": witnesses,
    }
    if verbose:
        print(
            f"A06-T independent credit oracle: {len(rows)} rows over "
            f"{len(traces)} sections"
        )
        print("documented oracle.md sect. 6 tables re-derived by this model:")
        for name in O_TABLES:
            print(
                f"  {name}: admit={tables[name]['admit']} "
                f"release={tables[name]['release']} sink={tables[name]['sink']} MATCH"
            )
        print("rivals on the delivered stimulus (differing rows):")
        for label, data in sorted(
            rivals.items(), key=lambda kv: -kv[1]["differing_rows"]
        ):
            tag = "separated" if data["differing_rows"] else "INDISTINGUISHABLE"
            print(f"  {label:<36} {data['differing_rows']:>4}  {tag}")
        print("assertions:", json.dumps(counts))
        print("uncovered:", report["uncovered"])
    return report


def main() -> int:
    report = self_test()
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
