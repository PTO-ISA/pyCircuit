"""Test-only queue reference models; never imported by the compiler or DUT."""

from collections import deque


def _stimuli():
    rows = []

    def row(clk, valid=0, take=0, reset=0, data=0):
        rows.append(
            {"clk": clk, "valid": valid, "take": take, "rst": reset, "data": data}
        )

    row(0)
    for data in (8191, 1, 4096, 7):
        row(0, 1, data=data)
        row(1, 1, data=data)
    row(1, take=1, data=123)
    for data in (23, 42):
        row(0, 1, 1, data=data)
        row(1, 1, 1, data=data)
    for data in (65, 66, 67, 68):
        row(0, take=1, data=data)
        row(1, take=1, data=data)
    row(0, 1, data=8191)
    row(1, 1, data=8191)
    row(0, 1, 1, 1, 71)
    row(1, 1, 1, 1, 71)
    row(0)
    assert len(rows) == 27
    return rows


def _pack(fields):
    result = 0
    for width, value in fields:
        result = (result << width) | value
    return result


def _oracle(mode, width, depths, bypass):
    pending = [deque(), deque()]
    previous_clock = 0
    result = []
    saw_full_policy_difference = False
    for index, original in enumerate(_stimuli()):
        row = dict(original)
        if width == 19:
            row["data"] = _pack(
                ((5, (index * 3 + 1) % 32), (1, index % 2), (13, row["data"]))
            )
        available = [bool(queue) for queue in pending]
        capacity = [len(queue) < depths[i] for i, queue in enumerate(pending)]
        if mode in ("pair", "nested"):
            takes = [bool(row["take"]), bool(row["take"])]
            ready = [
                capacity[i] or (bypass[i] and available[i] and takes[i])
                for i in range(2)
            ]
        elif mode in ("forward_local", "forward_mixed"):
            ready_a = capacity[0]
            ready_b = capacity[1] or (
                bypass[1] and available[1] and ready_a and bool(row["take"])
            )
            ready = [ready_a, ready_b]
            takes = [ready_b and bool(row["take"]), ready_a and bool(row["take"])]
        elif mode == "snapshots":
            ready = capacity
            takes = [ready[1], bool(row["take"])]
        else:
            raise AssertionError(mode)
        values = [queue[0] if queue else 0 for queue in pending]
        fields = []
        for i in range(2):
            fields.extend(
                ((1, int(ready[i])), (1, int(available[i])), (width, values[i]))
            )
        if mode == "snapshots":
            fields.append((13, (row["data"] + 1) & 8191))
        row["expected"] = _pack(fields)
        result.append(row)
        if (
            mode in ("pair", "nested")
            and all(len(queue) == depths[i] for i, queue in enumerate(pending))
            and row["take"]
        ):
            assert ready == [False, True]
            saw_full_policy_difference = True
        if row["clk"] and not previous_clock:
            if row["rst"]:
                pending = [deque(), deque()]
            else:
                for i, queue in enumerate(pending):
                    pop = available[i] and takes[i]
                    push = bool(row["valid"]) and ready[i]
                    if pop:
                        queue.popleft()
                    if push:
                        increment = int(mode == "snapshots" and i == 1)
                        queue.append((row["data"] + increment) & ((1 << width) - 1))
                    assert len(queue) <= depths[i]
        previous_clock = row["clk"]
    if mode in ("pair", "nested"):
        assert saw_full_policy_difference
    return {
        "input_bits": width,
        "output_bits": 2 * (width + 2) + (13 if mode == "snapshots" else 0),
        "rows": result,
    }


def _single(width, depth, values):
    rows = [
        {"clk": 0, "valid": 0, "take": 0, "rst": 1, "data": 0},
        {"clk": 1, "valid": 0, "take": 0, "rst": 1, "data": 0},
        {"clk": 0, "valid": 1, "take": 0, "rst": 0, "data": values[0]},
        {"clk": 1, "valid": 1, "take": 0, "rst": 0, "data": values[0]},
        {"clk": 0, "valid": 1, "take": 0, "rst": 0, "data": values[1]},
        {"clk": 1, "valid": 1, "take": 0, "rst": 0, "data": values[1]},
        {"clk": 0, "valid": 0, "take": 1, "rst": 0, "data": values[2]},
        {"clk": 1, "valid": 0, "take": 1, "rst": 0, "data": values[2]},
        {"clk": 0, "valid": 0, "take": 1, "rst": 0, "data": 0},
        {"clk": 1, "valid": 0, "take": 1, "rst": 0, "data": 0},
        {"clk": 0, "valid": 0, "take": 0, "rst": 0, "data": 0},
    ]
    pending = deque()
    previous_clock = 0
    mask = (1 << width) - 1
    for row in rows:
        available = bool(pending)
        ready = len(pending) < depth
        head = pending[0] if pending else 0
        row["expected"] = _pack(((1, int(ready)), (1, int(available)), (width, head)))
        if row["clk"] and not previous_clock:
            if row["rst"]:
                pending.clear()
            else:
                if available and row["take"]:
                    pending.popleft()
                if row["valid"] and ready:
                    pending.append(row["data"] & mask)
        previous_clock = row["clk"]
    return {"input_bits": width, "output_bits": width + 2, "rows": rows}


def _delayed(width, depth, latency, bypass=False, raw=False, dead=False):
    # Mathematical Python integers and absolute birth/maturity edge numbers:
    # this oracle shares no ring counters, wrapped deadlines or cursor logic.
    rows = []
    mask = (1 << width) - 1

    def row(clk, valid=0, take=0, reset=0, serial=0):
        value = ((serial * 1031 + 17) ^ (serial << (width - 3))) & mask
        known, z = mask, 0
        if raw and serial % 4:
            known = mask // 3 if serial % 4 == 1 else 0
            z = (
                (mask ^ known) & (mask // 5)
                if serial % 4 == 1
                else (mask if serial % 4 == 2 else 0)
            )
        rows.append(
            {
                "clk": clk,
                "valid": int(valid),
                "take": int(take),
                "rst": int(reset),
                "data": value,
                "data_known": known,
                "data_z": z,
            }
        )

    def edge(valid=0, take=0, reset=0, serial=0):
        row(0, valid, take, reset, serial)
        row(1, valid, take, reset, serial)

    row(0)
    edge(1, 1, serial=1)  # E0 birth; old state cannot flow through.
    for _ in range(min(latency - 1, 4)):
        edge(0, 1)
    edge(0, 1)  # Earliest EL pop for small L, early ineligible for huge L.
    for serial in range(2, depth + 2):
        edge(1, 0, serial=serial)
    row(1, 0, 1, serial=17)  # Held high with changed request pins.
    for _ in range(48):
        edge(0, 0)  # Backpressure over many timestamp wraps.
    edge(1, 1, serial=9)  # Full eligible replacement only for downstream_pop.
    for serial in range(12, 52):
        edge(serial % 5 != 2, serial % 7 != 3, serial=serial)
        row(1, 1, 1, serial=serial + 100)  # Held high never ages.
        row(0, 0, 0)  # Falling and repeated-low samples.
        row(0, 1, 1, serial=serial + 200)
    edge(1, 1, reset=1, serial=71)
    for _ in range(8):
        edge(0, 1)  # Stale occupied slots must not revive after reset.
    edge(1, 0, serial=73)
    for _ in range(min(latency + 2, 8)):
        edge(0, 1)

    births = deque()
    previous_clock = False
    committed = 0
    accepted = popped = 0
    saw_waiting_zero = saw_full = saw_replacement = False
    for row in rows:
        eligible = bool(births) and births[0][1] <= committed
        pop = eligible and row["take"]
        ready = len(births) < depth or (bypass and pop)
        value, known, z = births[0][0] if eligible else (0, mask, 0)
        saw_waiting_zero |= (
            bool(births) and not eligible and value == 0 and known == mask and z == 0
        )
        saw_full |= len(births) == depth
        if dead:
            row["expected"] = _pack(
                ((1, row["valid"]), (1, row["take"]), (width, row["data"]))
            )
            row["expected_known"] = (3 << width) | row["data_known"]
            row["expected_z"] = row["data_z"]
        else:
            row["expected"] = _pack(
                ((1, int(ready)), (1, int(eligible)), (width, value))
            )
            row["expected_known"] = (3 << width) | known
            row["expected_z"] = z
        if row["clk"] and not previous_clock:
            if row["rst"]:
                births.clear()
                committed = 0
            else:
                committed += 1
                saw_replacement |= (
                    len(births) == depth and pop and row["valid"] and ready
                )
                if pop:
                    births.popleft()
                    popped += 1
                if row["valid"] and ready:
                    births.append(
                        (
                            (row["data"], row["data_known"], row["data_z"]),
                            committed + latency - 1,
                        )
                    )
                    accepted += 1
                assert len(births) <= depth
        previous_clock = row["clk"]
    assert committed < 1024 and saw_waiting_zero and saw_full
    if latency < 16:
        assert popped > 0
    else:
        assert popped == 0  # Bounded early-state proof only.
    if bypass and latency < 16:
        assert saw_replacement
    return {"input_bits": width, "output_bits": width + 2, "rows": rows}


class _RouteMergeFifo:
    """Host FIFO for the A27 route/priority-merge oracle.

    Mathematical host deques and absolute edge numbers only: this shares no ring
    counters, wrapped deadlines or cursor logic with the DUT.  ``ready`` is the
    ``local_occupancy`` contract taken before the edge, so a slot released on edge
    E is never reused on E.
    """

    def __init__(self, depth, latency):
        self.depth = depth
        self.latency = latency
        self.tokens = deque()

    def ready(self):
        return len(self.tokens) < self.depth

    def eligible(self, edge):
        return bool(self.tokens) and self.tokens[0][1] <= edge

    def value(self, edge):
        return self.tokens[0][0] if self.eligible(edge) else 0

    def transfer(self, edge, push_ok, data, pop):
        if pop:
            self.tokens.popleft()
        if push_ok:
            self.tokens.append((data, edge + self.latency - 1))
        assert len(self.tokens) <= self.depth


def _route_merge_stimuli():
    """Saturating, draining and arbitration-bearing stimulus for the A27 root.

    Deterministic and generator-free: 16 or more stalled drives are needed before
    the depth 2/2/2/1/1/2 chain (in-flight bound 10) can pull ``in_ready`` low, and
    a release is needed before ``merged`` can have space while both transform
    results hold a token.
    """
    rows = []

    def row(clk, valid=0, take=0, reset=0, data=0):
        rows.append(
            {
                "clk": int(clk),
                "valid": int(valid),
                "take": int(take),
                "rst": int(reset),
                "data": int(data),
            }
        )

    def edge(valid=0, take=0, reset=0, data=0):
        row(0, valid, take, reset, data)
        row(1, valid, take, reset, data)

    row(0)
    edge(reset=1)
    for serial in range(1, 19):
        edge(1, 0, data=serial % 2)
    for _ in range(8):
        edge(0, 1)
    for serial in range(27, 47):
        edge(serial % 4 != 3, serial % 3 == 0, data=serial % 2)
    for serial in range(47, 59):
        edge(1, 1, data=(serial // 2) % 2)
    row(0, 1, 0, data=1)
    row(1, 1, 0, data=1)
    row(1, 0, 0, data=0)
    for _ in range(6):
        edge(0, 1)
    edge(reset=1)
    for serial in range(3):
        edge(1, 1, data=serial % 2)
    row(0)
    return rows


def _route_merge_trace(payload_bits, depths, adds, arbiter):
    """Model the six-queue root from contract parameters, not from the fixture.

    ``depths`` is ``(input_queue, left, right, left_done, right_done, merged)`` and
    ``adds`` is ``(left_add, right_add)``.  Every queue is latency one with an
    explicit local-occupancy ready.  ``arbiter`` selects the merge stage only:
    ``priority`` is ``inputs[0]`` first, ``round_robin`` is the host reference model
    kept purely as a negative control (no hardware path is delivered for it).
    """
    assert arbiter in ("priority", "round_robin")
    mask = (1 << payload_bits) - 1
    left_add, right_add = adds
    queues = [_RouteMergeFifo(depth, 1) for depth in depths]
    iq, left, right, left_done, right_done, merged = queues
    flags = {"input_stall": False, "route_stall": False, "arbitrated": False}
    witnesses = []
    rows = []
    cursor = 0
    edge = 0
    previous_clock = False
    for original in _route_merge_stimuli():
        row = dict(original)
        # Every control and payload below is sampled before this row's clock edge;
        # all six queues transfer together, so none may read a post-edge head.
        iq_eligible = iq.eligible(edge)
        iq_ready = iq.ready()
        iq_head = iq.value(edge)
        is_left = iq_head == 0
        is_right = iq_head == 1
        left_ready = left.ready()
        right_ready = right.ready()
        left_valid = left.eligible(edge)
        right_valid = right.eligible(edge)
        left_head = left.value(edge)
        right_head = right.value(edge)
        left_done_ready = left_done.ready()
        right_done_ready = right_done.ready()
        left_done_valid = left_done.eligible(edge)
        right_done_valid = right_done.eligible(edge)
        left_done_head = left_done.value(edge)
        right_done_head = right_done.value(edge)
        merged_valid = merged.eligible(edge)
        merged_ready = merged.ready()
        route_take = iq_eligible and (
            (is_left and left_ready) or (is_right and right_ready)
        )
        flags["input_stall"] |= not iq_ready
        flags["route_stall"] |= iq_eligible and not route_take
        row["expected"] = _pack(
            (
                (1, int(iq_ready)),
                (1, int(merged_valid)),
                (payload_bits, merged.value(edge)),
            )
        )
        rows.append(row)
        if row["clk"] and not previous_clock:
            if row["rst"]:
                for queue in queues:
                    queue.tokens.clear()
                cursor = 0
            else:
                edge += 1
                both = left_done_valid and right_done_valid
                # The merge stage spends at most one input per edge, and only when
                # the output queue has capacity (never on a pop-released slot).
                winner = None
                if merged_ready and (left_done_valid or right_done_valid):
                    if arbiter == "priority":
                        winner = 0 if left_done_valid else 1
                    else:
                        winner = next(
                            index
                            for index in (cursor, 1 - cursor)
                            if (left_done_valid if index == 0 else right_done_valid)
                        )
                        cursor = (winner + 1) % 2
                    if both:
                        flags["arbitrated"] = True
                        witnesses.append((edge, winner))
                iq.transfer(
                    edge,
                    bool(row["valid"]) and iq_ready,
                    row["data"] & mask,
                    route_take,
                )
                left.transfer(
                    edge,
                    iq_eligible and is_left and left_ready,
                    iq_head,
                    left_valid and left_done_ready,
                )
                right.transfer(
                    edge,
                    iq_eligible and is_right and right_ready,
                    iq_head,
                    right_valid and right_done_ready,
                )
                left_done.transfer(
                    edge,
                    left_valid and left_done_ready,
                    (left_head + left_add) & mask,
                    winner == 0,
                )
                right_done.transfer(
                    edge,
                    right_valid and right_done_ready,
                    (right_head + right_add) & mask,
                    winner == 1,
                )
                merged.transfer(
                    edge,
                    winner is not None,
                    left_done_head if winner == 0 else right_done_head,
                    merged_valid and bool(row["take"]),
                )
        previous_clock = row["clk"]
    return rows, flags, witnesses


def _route_merge(payload_bits, depths, adds):
    rows, flags, witnesses = _route_merge_trace(payload_bits, depths, adds, "priority")
    # (i) the stimulus must actually reach the arbitration and the backpressure
    #     boundaries, otherwise the corresponding assertions would be vacuous.
    assert flags["input_stall"], "stimulus never pulled in_ready low"
    assert flags["route_stall"], "stimulus never exercised head-of-line blocking"
    assert flags["arbitrated"] and witnesses, "stimulus has no non-vacuous merge edge"
    # (ii) the vector set must be able to tell strict priority from round robin;
    #      the same custom as saw_full_policy_difference above.
    rival, _, _ = _route_merge_trace(payload_bits, depths, adds, "round_robin")
    assert [row["expected"] for row in rival] != [row["expected"] for row in rows], (
        "vector set cannot distinguish priority from round_robin"
    )
    return {
        "input_bits": payload_bits,
        "output_bits": payload_bits + 2,
        "rows": rows,
        "arbitration_edges": witnesses,
    }


"""A27-T independent oracle mode for the route + priority-merge root.

Derivation discipline
---------------------
Every expected value below comes from exactly one of:

* ``docs/reference/spec-queues.md:34-70`` -- ``out_valid = eligible_count != 0``,
  ``out_data = out_valid ? storage[rd] : packed_zero<T>``,
  ``do_pop = out_valid & out_ready``, ``do_push = in_valid & in_ready``,
  ``local_occupancy: in_ready = count < depth``, "a token captured on E0 ...
  can first be consumed at EL" and "a full but unavailable queue cannot replace
  a token under either ready policy";
* the A27-O contract (``A27/oracle.md`` ``A27-O-RT-*``/``MG-*``/``TM-*``) plus the
  frozen historical source ``pyc_route_merge_pipeline.py`` (16 lines): two-output
  route on the identity selector, ``+10`` on branch 0, ``+20`` on branch 1, then
  ``policy="priority"`` merge whose input order is ``(left_done, right_done)``;
* the harness sampling convention, which this module pins with an independent DUT
  probe (``/private/tmp/a27t-probe/probe.cpp``, scratch): ``sample(row i)`` is the
  committed state *before* row ``i``'s rising edge, so a token pushed at edge ``k``
  is first observable on the row after edge ``k``.

It is deliberately **not** derived from the ``route_merge`` mode that A27-S wrote
in this same file: that mode's stimulus, host FIFO class and witness bookkeeping
are untouched here, and this module shares no code with them.
"""

_IND_MASK = (1 << 64) - 1
_IND_DEPTHS = (2, 2, 2, 1, 1, 2)  # input, left, right, left_done, right_done, merged
_IND_ADDS = (10, 20)


class _IndQueue:
    """A single latency-1 queue whose ready formula is an explicit parameter.

    ``tokens`` holds ``(value, maturity_edge)`` pairs with *absolute* edge numbers
    and the payload as a mathematical Python integer, so no ring counter, wrapped
    deadline or cursor is shared with the DUT or with any other oracle here.
    """

    def __init__(self, depth):
        self.depth = depth
        self.tokens = []

    def available(self, edges):
        return bool(self.tokens) and self.tokens[0][1] <= edges

    def head(self, edges):
        return self.tokens[0][0] if self.available(edges) else 0

    def ready(self, policy, do_pop):
        full = len(self.tokens) < self.depth
        if policy == "local_occupancy":
            return full
        assert policy == "downstream_pop", policy
        return full or do_pop

    def transfer(self, edges, push_ok, value, pop):
        if pop:
            assert self.available(edges), "pop of an unavailable token"
            self.tokens.pop(0)
        if push_ok:
            self.tokens.append((value, edges + 1))
        assert len(self.tokens) <= self.depth, (len(self.tokens), self.depth)


def _route_merge_independent_model(rows, arbiter="priority",
                                   policy="local_occupancy"):
    """Committed-state model of the six-queue chain.

    Every decision for one rising edge reads a single pre-edge snapshot and all
    six transfers commit together; the ready formulas are resolved in dependency
    order so that ``downstream_pop`` stays well defined (it makes ``in_ready``
    combinational in downstream ready).
    """
    assert arbiter in ("priority", "round_robin")
    iq, left, right, ld, rd, mg = (_IndQueue(d) for d in _IND_DEPTHS)
    edges = 0
    cursor = 0
    previous_clock = 0
    trace = []
    for row in rows:
        rising = bool(row["clk"]) and not previous_clock
        previous_clock = row["clk"]

        # -- observable ports, from committed state only --
        mg_available = mg.available(edges)
        mg_head = mg.head(edges)

        # -- one pre-edge snapshot --
        iq_head = iq.head(edges)
        iq_available = iq.available(edges)
        is_left, is_right = iq_head == 0, iq_head == 1
        left_available, right_available = left.available(edges), right.available(edges)
        left_head, right_head = left.head(edges), right.head(edges)
        ld_available, rd_available = ld.available(edges), rd.available(edges)
        ld_head, rd_head = ld.head(edges), rd.head(edges)

        # -- resolve pop/ready in dependency order (downstream -> upstream) --
        mg_pop = mg_available and bool(row["take"])
        mg_ready = mg.ready(policy, mg_pop)
        winner = None
        if mg_ready:
            if arbiter == "priority":
                winner = 0 if ld_available else (1 if rd_available else None)
            elif ld_available or rd_available:
                order = (cursor, 1 - cursor)
                winner = next(i for i in order if (ld_available, rd_available)[i])
                cursor = (winner + 1) % 2
        ld_pop, rd_pop = winner == 0, winner == 1
        ld_ready = ld.ready(policy, ld_pop)
        rd_ready = rd.ready(policy, rd_pop)
        ld_push = left_available and ld_ready
        rd_push = right_available and rd_ready
        left_pop, right_pop = ld_push, rd_push
        left_ready = left.ready(policy, left_pop)
        right_ready = right.ready(policy, right_pop)
        route_pop = iq_available and (
            (is_left and left_ready) or (is_right and right_ready)
        )
        iq_ready = iq.ready(policy, route_pop)
        iq_push = bool(row["valid"]) and iq_ready
        left_push = iq_available and is_left and left_ready
        right_push = iq_available and is_right and right_ready

        row["expected"] = (
            (int(iq_ready) << 65) | (int(mg_available) << 64) | mg_head
        )
        trace.append(
            {
                "row": len(trace),
                "edge": edges + 1 if rising else 0,
                "iq_ready": iq_ready,
                "route_pop": route_pop,
                "iq_push": iq_push,
                "ld_available": ld_available,
                "rd_available": rd_available,
                "mg_ready": mg_ready,
                "mg_available": mg_available,
                "winner": winner,
                "mg_pop": mg_pop,
                "mg_push": winner is not None,
                "mg_head": mg_head,
                "occupancy": (len(iq.tokens), len(left.tokens), len(right.tokens),
                              len(ld.tokens), len(rd.tokens), len(mg.tokens)),
            }
        )

        if not rising:
            continue
        if row["rst"]:
            for queue in (iq, left, right, ld, rd, mg):
                queue.tokens.clear()
            cursor = 0
            edges += 1
            continue
        iq.transfer(edges, iq_push, row["data"] & _IND_MASK, route_pop)
        left.transfer(edges, left_push, iq_head, ld_push)
        right.transfer(edges, right_push, iq_head, rd_push)
        ld.transfer(edges, ld_push, (left_head + _IND_ADDS[0]) & _IND_MASK, ld_pop)
        rd.transfer(edges, rd_push, (right_head + _IND_ADDS[1]) & _IND_MASK, rd_pop)
        mg.transfer(edges, winner is not None,
                    ld_head if winner == 0 else (rd_head if winner == 1 else 0),
                    mg_pop)
        edges += 1
    return trace, (iq, left, right, ld, rd, mg)


def _route_merge_independent_stimulus():
    """Hand-designed stimulus: saturate, stall, release in bursts, drain, reset.

    * 16 dense injections hold ``take`` low long enough that the in-flight bound
      of ten tokens is exceeded and ``in_ready`` really falls (T4);
    * the bursty ``take`` pattern (1,1,0,1,0) keeps re-opening ``merged`` while
      both depth-one transform result queues hold a token, which is the only
      situation in which the priority decision is observable at all (T6);
    * gating ``valid`` in blocks of five makes one branch starve and re-aligns the
      two branch phases, so more than one such co-valid edge occurs (T7/F4);
    * the final eight ``take`` edges drain the chain to empty, and the
      conservation identity is asserted on that pre-reset window (T/RT-09);
    * the chain is then refilled so the trailing reset edge strikes a NON-EMPTY
      chain: the tokens it discards are exactly what a reset-ignoring DUT would
      instead expose after the reset (C2 -> MG-09 / TM-14 / UT-11).

    Every driven payload is 0 or 1.  Out-of-range payloads would wedge the DUT
    permanently (C-1 / T8) and are therefore excluded from the pass set.
    """
    rows = []

    def row(clk, rst=0, valid=0, take=0, data=0):
        # `int(...)` matters here: this generator writes `valid`/`take` straight
        # into a C++ brace initialiser, where Python's True/False would not compile.
        rows.append({"clk": int(clk), "rst": int(rst), "valid": int(valid),
                     "take": int(take), "data": int(data)})

    def edge(rst=0, valid=0, take=0, data=0):
        row(0, rst, valid, take, data)
        row(1, rst, valid, take, data)

    row(0)
    edge(rst=1)
    for serial in range(16):
        edge(valid=1, data=serial % 2)
    for _ in range(3):
        edge()
    for serial in range(40):
        edge(valid=(serial // 5) % 2 == 0,
             take=(1, 1, 0, 1, 0)[serial % 5],
             data=serial % 2)
    for _ in range(8):
        edge(take=1)
    # C2: the drain above is proven to empty the chain (assertion (v-a)); the
    # stimulus then REFILLS it so that the trailing reset edge strikes a chain
    # that still holds in-flight tokens.  A DUT with no reset behaviour would
    # keep those tokens and expose them on `available`/`head` after the reset,
    # which is what makes MG-09/TM-14/UT-11 falsifiable (assertion (ix)).
    for serial in range(4):
        edge(valid=1, take=0, data=serial % 2)
    edge(rst=1)
    for _ in range(6):
        edge(take=1)
    row(0)
    return rows


def _route_merge_independent():
    rows = _route_merge_independent_stimulus()
    # (i) the pass set must stay inside the selector domain: only 0 and 1.
    assert {row["data"] for row in rows} == {0, 1}, "out-of-range stimulus in pass set"
    trace, queues = _route_merge_independent_model(rows)

    # (ii) backpressure must really be reached, and acceptance is a handshake.
    stalled = [entry for entry in trace if entry["edge"] and not entry["iq_ready"]]
    driven = sum(1 for entry in trace if entry["edge"] and rows[entry["row"]]["valid"])
    accepted = sum(1 for entry in trace if entry["edge"] and entry["iq_push"])
    assert stalled, "T4: in_ready never fell, the backpressure claim is vacuous"
    assert accepted < driven, (
        f"T3: every one of the {driven} driven tokens was accepted; the stimulus "
        "never exercised a silent rejection"
    )

    # (iii) arbitration is only meaningful where BOTH inputs are valid AND the
    #       merge output can actually take a token (T6).
    arbitrated = [entry for entry in trace
                  if entry["edge"] and entry["ld_available"]
                  and entry["rd_available"] and entry["mg_ready"]]
    vacuous = [entry for entry in trace
               if entry["edge"] and entry["ld_available"] and entry["rd_available"]
               and not entry["mg_ready"]]
    assert arbitrated, "T6: no edge has both inputs valid with merge capacity"
    assert all(entry["winner"] == 0 for entry in arbitrated), (
        "priority must always serve inputs[0] = left_done on an arbitrated edge"
    )
    assert vacuous, "stimulus should also show the vacuous co-valid-but-blocked edges"

    # (iv) a single branch is half rate: depth-one local-occupancy result queues
    #      cannot be refilled on the edge that consumed them (T5 / F4 / Q-D7).
    served = [(entry["edge"], entry["winner"]) for entry in trace
              if entry["edge"] and entry["winner"] is not None]
    left_served = [edge for edge, side in served if side == 0]
    right_served = [edge for edge, side in served if side == 1]
    assert right_served, "T7: the right branch was never served"
    for name, series in (("left", left_served), ("right", right_served)):
        assert all(b - a != 1 for a, b in zip(series, series[1:])), (  # noqa: B905 - preserve the extracted reference algorithm
            f"T5: the {name} branch was served on consecutive edges, which a "
            "depth-one local-occupancy queue cannot do"
        )

    # (v) conservation, split over the two windows that C2 distinguishes.
    #
    # Before C2 the whole statement was `pushes == pops` plus "every reset edge
    # struck an empty chain", which is exactly what made reset unfalsifiable: a
    # DUT with no reset behaviour reproduced the identical expected stream.  The
    # drain identity is therefore evaluated on the PRE-RESET window, and the
    # trailing reset is required to strike a NON-EMPTY chain, so the end-to-end
    # identity has to carry the discarded tokens explicitly.
    reset_rows = [entry for entry in trace
                  if entry["edge"] and rows[entry["row"]]["rst"]]
    assert reset_rows, "stimulus has no reset edge"
    last_reset = max(entry["edge"] for entry in reset_rows)
    reset_occupancy = {entry["edge"]: entry["occupancy"] for entry in reset_rows}

    # (v-a) PRE-RESET window: the chain provably drains to empty, and up to that
    #       drain every token that entered `merged` was consumed -- the original
    #       conservation statement, bound to the window before the reset.
    drained = [entry["edge"] for entry in trace
               if entry["edge"] and entry["edge"] < last_reset
               and not any(entry["occupancy"])]
    assert drained, "the chain never drained to empty before the reset edge"
    drain_edge = max(drained)
    drain_pushes = sum(1 for entry in trace
                       if entry["edge"] and entry["edge"] <= drain_edge
                       and entry["mg_push"])
    drain_pops = sum(1 for entry in trace
                     if entry["edge"] and entry["edge"] <= drain_edge
                     and entry["mg_pop"])
    assert drain_pushes == drain_pops, (drain_edge, drain_pushes, drain_pops)

    # (v-b) the trailing reset must be NON-VACUOUS: it strikes a chain that still
    #       holds in-flight tokens.  This clause is what makes MG-09 / TM-14 /
    #       UT-11 falsifiable; assertion (ix) evaluates the counterfactual.
    assert any(reset_occupancy[last_reset]), (
        "the trailing reset struck an empty chain, which makes reset "
        "unfalsifiable (MG-09 / TM-14 / UT-11)",
        reset_occupancy,
    )
    discarded = sum(sum(occupancy) for occupancy in reset_occupancy.values())

    # (v-c) end-to-end conservation with the reset discard made explicit: every
    #       accepted token is popped out of `merged`, still in the chain on the
    #       final row, or discarded by a reset edge.
    accepted_total = sum(1 for entry in trace if entry["edge"] and entry["iq_push"])
    popped_total = sum(1 for entry in trace if entry["edge"] and entry["mg_pop"])
    final_occupancy = tuple(len(queue.tokens) for queue in queues)
    assert accepted_total == popped_total + discarded + sum(final_occupancy), (
        accepted_total, popped_total, discarded, final_occupancy,
    )
    del queues
    popped = {entry["mg_head"] for entry in trace
              if entry["edge"] and entry["mg_pop"]}
    assert popped <= {10, 21}, popped
    assert {10, 21} <= {entry["mg_head"] for entry in trace if entry["mg_available"]}

    # (vi) boundary coverage: an empty merge reads packed zero, and both branches
    #      are individually observable.
    assert any(not entry["mg_available"] and entry["mg_head"] == 0
               for entry in trace), "no both-inputs-invalid row"
    assert any(entry["mg_available"] and entry["mg_head"] == 10 for entry in trace)
    assert any(entry["mg_available"] and entry["mg_head"] == 21 for entry in trace)

    # (vii) arithmetic boundary cases that the selector domain makes unreachable
    #       through the DUT; asserted here as host-level facts only (RN-08).
    assert (_IND_MASK - 9) & _IND_MASK == _IND_MASK - 9
    assert ((_IND_MASK - 9) + _IND_ADDS[0]) & _IND_MASK == 0
    assert (_IND_MASK + _IND_ADDS[1]) & _IND_MASK == 19

    # (viii) negative controls: the same stimulus must be able to tell the frozen
    #        contract from each rival it is meant to exclude.  Falsifiable at the
    #        vector level, evaluated before any DUT runs.
    #        The third control is the C2 reset control: clearing every `rst` in
    #        the stimulus models a DUT that implements no reset at all, and the
    #        vector set must then produce a DIFFERENT expected stream, otherwise
    #        MG-09 / TM-14 / UT-11 would be unfalsifiable.
    rivals = {}
    for label, kwargs, clear_rst in (
        ("round_robin", {"arbiter": "round_robin"}, False),
        ("downstream_pop", {"policy": "downstream_pop"}, False),
        ("reset_ignored", {}, True),
    ):
        rival_rows = _route_merge_independent_stimulus()
        if clear_rst:
            for rival_row in rival_rows:
                rival_row["rst"] = 0
        _route_merge_independent_model(rival_rows, **kwargs)
        differing = sum(
            1 for ours, theirs in zip(rows, rival_rows)  # noqa: B905 - preserve the extracted reference algorithm
            if ours["expected"] != theirs["expected"]
        )
        assert differing, (
            f"negative control failed: the vector set cannot distinguish the "
            f"frozen contract from {label}"
        )
        rivals[label] = differing
    assert rivals["round_robin"] >= 1

    return {
        "input_bits": 64,
        "output_bits": 66,
        "rows": rows,
        "arbitration_edges": [entry["edge"] for entry in arbitrated],
        "vacuous_co_valid_edges": [entry["edge"] for entry in vacuous],
        "left_served_edges": left_served,
        "right_served_edges": right_served,
        "stalled_edges": [entry["edge"] for entry in stalled],
        "driven_tokens": driven,
        "accepted_tokens": accepted,
        "rival_differing_rows": rivals,
        "per_branch_fifo_observable": False,
        "reset_edges": [entry["edge"] for entry in reset_rows],
        "reset_occupancy": {edge: list(occupancy)
                            for edge, occupancy in reset_occupancy.items()},
        "reset_discarded_tokens": discarded,
        "pre_reset_drained_edge": drain_edge,
        "final_occupancy": list(final_occupancy),
    }


# ---------------------------------------------------------------------------
# A25 reorder (``pyc_reorder_pipeline``) oracle mode
#
# Derivation discipline
# ---------------------
# Every expected value below comes from exactly one of:
#
# * the frozen A25-O oracle (``docs/gates/logs/${RUN_ID}/A25/oracle.md``,
#   203 lines) as corrected by the PM dispatch appendix ``A25/PRE-S-APPENDIX.md``:
#   ``RT-01``..``RT-10`` (payload passthrough, ``capacity 16`` as an OCCUPANCY
#   bound and NOT a key window, key = ``item.sequence``, monotone release,
#   out-of-order buffering, release only on ``key == next_key``, duplicate and
#   stale keys invalid, ``start`` non-negative) and ``TM-01``..``TM-03``
#   (``latency=1``, reverse backpressure, waiting tokens hold capacity);
# * the frozen block ``simulator/gfsim/include/gfsim/queue_blocks.h:400-503`` at
#   baseline ``8887e6dec7b4cc530a9967c860dc6a224d79a4ab`` (sha256
#   ``e7db1c3357442e14…``): ``:414`` ``entries_.size() < capacity_`` reads the OLD
#   occupancy, ``:426`` ``key < nextKey_`` is stale, ``:430``
#   ``entries_.contains(key)`` is duplicate, ``:439`` retirement requires
#   ``entries_.find(nextKey_)`` plus ``output_.canProposePush()``, ``:448-449``
#   erase and ``++nextKey_`` happen in Xfer so an entry admitted on edge E can
#   never retire on E, ``:469`` ``nextKey()`` is the observable pointer and
#   ``:474`` resets it to ``start_``;
# * the ratified design ``docs/work-items/p-reorder-sequence-reorder-design.md``
#   (sect. 4.3 full/admit/retire equations, sect. 4.4 timing, sect. 6.3
#   backpressure chain, sect. 9.3 ``A-FC-1``..``A-FC-10``);
# * the key width ruling recorded in the source docstring: the key VALUE is
#   ``sequence`` (u32), the stored/compared domain and ``next_key`` are u64, and
#   the result packs to ``1 + 1 + 64 + 64 + 1 = 131`` bits;
# * the harness sampling convention shared with the ``route_merge_independent``
#   and ``credit`` modes: two rows per epoch, and both rows carry the committed
#   pre-edge state read with that epoch's inputs.
#
# Only ``local_occupancy`` is delivered (``CD-DEPEND-3``: the historical
# order-dependent same-edge reuse is inexpressible and is deliberately not
# invented).  Arrival-order release, hole skipping, "capacity is a key window",
# duplicate overwrite, drop-on-blocked-output, lowering ``ready`` on an illegal
# head, accepting a stale key and an ignored reset exist here only as rival host
# models used as negative controls, evaluated on the already-fixed vector rows
# before any DUT runs.
# ---------------------------------------------------------------------------

_A25_CAPACITY = 16
_A25_IN_DEPTH = 8
_A25_OUT_DEPTH = 4
_A25_LATENCY = 1
_A25_SEQ_BITS = 32
_A25_VAL_BITS = 32
_A25_TOKEN_BITS = _A25_SEQ_BITS + _A25_VAL_BITS
_A25_KEY_BITS = 64
_A25_RESULT_BITS = 2 + _A25_TOKEN_BITS + _A25_KEY_BITS + 1
_A25_SEQ_MASK = (1 << _A25_SEQ_BITS) - 1
_A25_VAL_MASK = (1 << _A25_VAL_BITS) - 1
_A25_FAR_KEY = 1000


def _a25_token(sequence, value):
    """Pack a ``Token`` the way its declaration order implies (MSB first)."""
    return ((sequence & _A25_SEQ_MASK) << _A25_VAL_BITS) | (value & _A25_VAL_MASK)


def _a25_sequence(packed):
    return (packed >> _A25_VAL_BITS) & _A25_SEQ_MASK


def _a25_value(packed):
    return packed & _A25_VAL_MASK


class _A25Fifo:
    """Host FIFO for one bounded queue of the A25 root.

    Mathematical deque plus absolute edge numbers, sharing no ring counters,
    wrapped deadlines or cursor logic with the DUT.  A token pushed on edge ``k``
    carries availability deadline ``k + latency - 1``, so with ``latency = 1`` it
    is first visible to the consumer on edge ``k + 1`` (``TM-01``).  ``ready`` is
    ``local_occupancy``: ``count < depth`` on the pre-edge occupancy, so a slot
    released on edge E is never refilled on E (``CD-DEPEND-3``).
    """

    def __init__(self, depth, latency=_A25_LATENCY):
        self.depth = depth
        self.latency = latency
        self.tokens = deque()

    def count(self):
        return len(self.tokens)

    def ready(self):
        return len(self.tokens) < self.depth

    def eligible(self, edge):
        return bool(self.tokens) and self.tokens[0][0] <= edge

    def value(self, edge):
        return self.tokens[0][1] if self.eligible(edge) else 0

    def transfer(self, edge, push, data, pop):
        if pop:
            self.tokens.popleft()
        if push:
            self.tokens.append((edge + self.latency - 1, data))
        assert len(self.tokens) <= self.depth, (len(self.tokens), self.depth)


def _a25_section(name, tokens, takes, drive="handshake", sink=None):
    """One reset-delimited section: offered tokens, per-epoch ``take``, witness.

    ``drive="handshake"`` offers the next token only while the input queue
    reports capacity, so no offered token is ever silently rejected.
    ``drive="always"`` offers it on every epoch and re-offers it after a
    rejection, which is what exercises real backpressure (``RT-10``/``TM-02``).
    """
    return {
        "name": name,
        "drive": drive,
        "tokens": list(tokens),
        "takes": list(takes),
        "sink": dict(sink or {}),
    }


def _a25_program():
    """The hand-designed sections; each one is restarted by a reset edge.

    ``S1``/``S2`` carry hand-computed ABSOLUTE sink tables (epoch -> sequence)
    that pin the frozen cadence accept@E -> admit@E+1 -> retire@E+2 -> sink@E+3
    independently of the host model; the other sections state their contract as
    invariants over the witness.  Values are 32-bit patterns with high bits set,
    so truncating ``sequence`` or ``value`` is visible in the result bits.
    """
    s1_values = (0xDEAD0000, 0xBEEF0001, 0x0F0F0002, 0xFFFF0003)
    s2_keys = (3, 1, 0, 2)
    s2_values = {3: 0x33330003, 1: 0x11110001, 0: 0x00000000, 2: 0x22220002}
    s4_order = [_A25_FAR_KEY, 0, 1] + list(range(15, 1, -1))
    s7_keys = list(range(32))
    s8_keys = list(range(_A25_FAR_KEY, _A25_FAR_KEY + _A25_CAPACITY))
    return [
        # S1: in-order stream; the absolute table pins the cadence.
        _a25_section(
            "S1_in_order",
            [(sequence, value) for sequence, value in enumerate(s1_values)],
            [1] * 9,
            sink={4: 0, 5: 1, 6: 2, 7: 3},
        ),
        # S2: arrival order 3,1,0,2 with `take` low while they land.
        _a25_section(
            "S2_out_of_order",
            [(key, s2_values[key]) for key in s2_keys],
            [0] * 4 + [1] * 7,
            sink={6: 0, 7: 1, 8: 2, 9: 3},
        ),
        # S3: keys 2 and 3 with no 0/1 -- a hole must wait, not skip.
        _a25_section(
            "S3_hole_waits",
            [(2, 0x22220002), (3, 0x33330003)],
            [1] * 8,
        ),
        # S4: key 1000 first (>= capacity), then 0 and 1, then 15..2: all sixteen
        # in-window keys must still leave in order while 1000 keeps its slot.
        _a25_section(
            "S4_capacity_and_far_key",
            [(key, 0x40000000 | key) for key in s4_order],
            [1] * (len(s4_order) + 30),
        ),
        # S5: key 5 stored, then a second key 5 -- duplicate, fail-closed.
        _a25_section(
            "S5_duplicate_stalls",
            [(5, 0xAAAA0005), (5, 0xBBBB0005)],
            [1] * 10,
        ),
        # S6: 0 and 1 retire, then a second key 0 -- stale, fail-closed.
        _a25_section(
            "S6_stale_stalls",
            [(0, 0x60000000), (1, 0x60000001), (0, 0x60000000)],
            [1] * 13,
        ),
        # S7: 32 ordered tokens offered with `take` low for 40 epochs: the chain
        # fills to 4 + 16 + 8 = 28 in flight, offers are refused, `ready` falls;
        # the second half releases `take` and the whole stream drains in order.
        _a25_section(
            "S7_backpressure_drain",
            [(key, 0x70000000 | key) for key in s7_keys],
            [0] * 40 + [1] * 40,
            drive="always",
        ),
        # S8: fill the table with sixteen far keys and hold `take` low, so the
        # section ends with a FULL table (the reset in S9 must strike it).
        _a25_section(
            "S8_fill_far_keys",
            [(key, 0x80000000 | (key - _A25_FAR_KEY)) for key in s8_keys],
            [0] * 24,
        ),
        # S9: the reset pair strikes that full table; only a real reset (table
        # cleared AND next_key back to 0) lets an in-order stream leave at all.
        _a25_section(
            "S9_reset_then_in_order",
            [(key, 0x90000000 | key) for key in range(8)],
            [1] * 24,
        ),
    ]


def _a25_knobs(**overrides):
    base = {
        "capacity": _A25_CAPACITY,
        "release": "by_key",
        "key_window": False,
        "dup_overwrites": False,
        "drop_on_blocked": False,
        "ready_on_fault": False,
        "stale_ok": False,
    }
    return dict(base, **overrides)


def _a25_read(in_q, out_q, table, next_key, edge, take, knobs):
    """One epoch's combinational truth, every term read from the pre-edge state.

    The order of the terms mirrors the source rule (and, through it, the frozen
    ``doWork``): all searches first, on the committed table, and the two writes
    happen later on disjoint slots.  ``retire`` needs output-queue capacity but
    never a slot released on this same edge, and ``admit`` needs a slot that was
    already free before this edge (appendix correction 2, design sect. 6.2).
    """
    in_valid = in_q.eligible(edge)
    in_token = in_q.value(edge)
    in_ready = in_q.ready()
    out_ready = out_q.ready()
    out_valid = out_q.eligible(edge)
    out_token = out_q.value(edge)
    incoming_key = _a25_sequence(in_token)
    occupied = [(index, slot) for index, slot in enumerate(table) if slot]
    free_index = next((index for index, slot in enumerate(table) if not slot), None)
    free_valid = free_index is not None
    if knobs["release"] == "by_key":
        match = next((item for item in occupied if item[1][0] == next_key), None)
    elif knobs["release"] == "arrival":
        match = min(occupied, key=lambda item: item[1][2], default=None)
    else:
        match = min(occupied, key=lambda item: item[1][0], default=None)
    match_index = match[0] if match else 0
    match_valid = match is not None
    duplicate_valid = any(item[1][0] == incoming_key for item in occupied)
    stale = incoming_key < next_key
    illegal = duplicate_valid if knobs["stale_ok"] else (stale or duplicate_valid)
    if knobs["key_window"] and incoming_key >= knobs["capacity"]:
        illegal = True
    invalid = bool(in_valid) and free_valid and illegal
    if knobs["dup_overwrites"]:
        admit = bool(in_valid) and free_valid and not stale
    else:
        admit = bool(in_valid) and free_valid and not illegal
    if knobs["ready_on_fault"]:
        # Rival only: the forbidden "pull in_ready low on an illegal head".
        in_ready = in_ready and not invalid
    retire = match_valid and (out_ready or knobs["drop_on_blocked"])
    push = match_valid and out_ready
    return {
        "ready": in_ready,
        "available": out_valid,
        "head": out_token,
        "next_key": next_key,
        "fault": invalid,
        "in_valid": bool(in_valid),
        "in_token": in_token,
        "incoming_key": incoming_key,
        "out_valid": bool(out_valid),
        "out_token": out_token,
        "out_ready": out_ready,
        "free_index": free_index,
        "free_valid": free_valid,
        "match_index": match_index,
        "match_valid": match_valid,
        "duplicate_valid": duplicate_valid,
        "stale": stale,
        "admit": admit,
        "retire": retire,
        "push": push,
        "retired_token": table[match_index][1] if match_valid else 0,
        "expected": _pack(
            (
                (1, int(in_ready)),
                (1, int(out_valid)),
                (_A25_TOKEN_BITS, out_token),
                (_A25_KEY_BITS, next_key),
                (1, int(invalid)),
            )
        ),
    }


def _a25_commit(state, edge, read, knobs, valid, data, take, arrival):
    """Commit one rising edge from the combinational result ``read``."""
    in_q, out_q, table = state["in_q"], state["out_q"], state["table"]
    in_q.transfer(edge + 1, bool(valid) and read["ready"], data, read["admit"])
    out_q.transfer(
        edge + 1, read["push"], read["retired_token"], read["out_valid"] and take
    )
    if read["retire"]:
        if knobs["release"] == "by_key":
            state["next_key"] += 1
        else:
            state["next_key"] = table[read["match_index"]][0] + 1
        table[read["match_index"]] = None
    if read["admit"]:
        if knobs["dup_overwrites"] and read["duplicate_valid"]:
            write = next(
                index
                for index, slot in enumerate(table)
                if slot and slot[0] == read["incoming_key"]
            )
        else:
            write = read["free_index"]
        table[write] = (read["incoming_key"], read["in_token"], arrival)
    return arrival + 1


def _a25_state():
    return {
        "in_q": _A25Fifo(_A25_IN_DEPTH),
        "out_q": _A25Fifo(_A25_OUT_DEPTH),
        "table": [None] * _A25_CAPACITY,
        "next_key": 0,
    }


def _a25_occupancy(state):
    return (
        sum(1 for slot in state["table"] if slot),
        state["in_q"].count(),
        state["out_q"].count(),
    )


def _a25_run(knobs):
    """Generate the vector rows and the per-section witness for the program."""
    state = _a25_state()
    rows = []
    witness = {}
    edge = 0
    for section in _a25_program():
        name = section["name"]
        seen = {
            "sink": {}, "sink_order": [], "fault": [], "ready_low": [],
            "rejected": [], "offered": 0, "accepted": 0, "accepted_trace": [],
            "next_key": [], "max_occupancy": 0, "pre_reset_occupancy": None,
            "occupancy": [],
        }
        witness[name] = seen
        # Every section opens with a synchronous reset pair: the sampled result is
        # still the pre-reset state, which is what the harness samples on the edge
        # that clears the chain.
        read = _a25_read(state["in_q"], state["out_q"], state["table"],
                         state["next_key"], edge, 0, knobs)
        seen["pre_reset_occupancy"] = _a25_occupancy(state)
        for clk in (0, 1):
            rows.append({"clk": clk, "rst": 1, "valid": 0, "take": 0, "data": 0,
                         "expected": read["expected"]})
        state = _a25_state()
        cursor = 0
        arrival = 0
        for epoch, take in enumerate(section["takes"], start=1):
            held = cursor < len(section["tokens"])
            ready_pre = state["in_q"].ready()
            offered = bool(held and (section["drive"] == "always" or ready_pre))
            sequence, value = section["tokens"][cursor] if held else (0, 0)
            data = _a25_token(sequence, value) if offered else 0
            read = _a25_read(state["in_q"], state["out_q"], state["table"],
                             state["next_key"], edge, take, knobs)
            for clk in (0, 1):
                rows.append({"clk": clk, "rst": 0, "valid": int(offered),
                             "take": int(bool(take)), "data": data,
                             "expected": read["expected"]})
            if read["out_valid"] and take:
                seen["sink"][epoch] = _a25_sequence(read["out_token"])
                seen["sink_order"].append(
                    (_a25_sequence(read["out_token"]), _a25_value(read["out_token"]))
                )
            if read["fault"]:
                seen["fault"].append(epoch)
            if not read["ready"]:
                seen["ready_low"].append(epoch)
            seen["next_key"].append(read["next_key"])
            occupancy = _a25_occupancy(state)
            seen["occupancy"].append(occupancy)
            seen["max_occupancy"] = max(seen["max_occupancy"], sum(occupancy))
            if offered:
                seen["offered"] += 1
                if read["ready"]:
                    seen["accepted"] += 1
                    cursor += 1
                else:
                    seen["rejected"].append(epoch)
            seen["accepted_trace"].append(seen["accepted"])
            arrival = _a25_commit(state, edge, read, knobs, offered, data, take,
                                  arrival)
            edge += 1
        seen["final"] = (state["next_key"], _a25_occupancy(state))
    return rows, witness


def _a25_replay(rows, knobs):
    """Recompute ``expected`` for already-fixed input rows (rival controls)."""
    state = _a25_state()
    expected = []
    edge = 0
    arrival = 0
    previous = 0
    for row in rows:
        rising = bool(row["clk"]) and not previous
        read = _a25_read(state["in_q"], state["out_q"], state["table"],
                         state["next_key"], edge, row["take"], knobs)
        expected.append(read["expected"])
        if rising and row["rst"]:
            state = _a25_state()
        elif rising:
            arrival = _a25_commit(state, edge, read, knobs, row["valid"],
                                  row["data"], row["take"], arrival)
            edge += 1
        previous = row["clk"]
    return expected


def _a25_check(label, actual, expected):
    assert actual == expected, f"{label}: actual={actual!r} expected={expected!r}"


def _a25():
    """Frozen reorder contract -> native/RTL vectors (two rows per epoch)."""
    knobs = _a25_knobs()
    rows, witness = _a25_run(knobs)
    program = _a25_program()

    for section in program:
        name = section["name"]
        seen = witness[name]
        # (i) the hand-computed absolute cadence tables, where the program states
        #     one: accept@E -> admit@E+1 -> retire@E+2 -> sink@E+3.
        if section["sink"]:
            _a25_check(f"{name} sink table (epoch -> sequence)",
                       seen["sink"], section["sink"])
        # (ii) every section but S7 offers tokens by handshake, so no offered
        #      token is silently rejected and exactly the offered ones are taken.
        if section["drive"] == "handshake":
            _a25_check(f"{name} silent rejections", seen["rejected"], [])
            _a25_check(f"{name} offered / accepted tokens",
                       (seen["offered"], seen["accepted"]),
                       (len(section["tokens"]), len(section["tokens"])))

    # S1: RT-04/TM-01 -- in-order release, both payload fields untouched.
    _a25_check("S1 release sequence", witness["S1_in_order"]["sink_order"],
               [(0, 0xDEAD0000), (1, 0xBEEF0001), (2, 0x0F0F0002),
                (3, 0xFFFF0003)])

    # S2: A-FC-1/A-FC-2 -- arrival order is 3,1,0,2 and release order is 0,1,2,3,
    # each sequence keeping its own value (BD-07).  A passthrough FIFO, an
    # arrival-order release and a "keep only the last value" model all differ.
    s2 = witness["S2_out_of_order"]
    _a25_check("S2 release sequence", s2["sink_order"],
               [(0, 0x00000000), (1, 0x11110001), (2, 0x22220002),
                (3, 0x33330003)])
    _a25_check("S2 release epochs", sorted(s2["sink"]), [6, 7, 8, 9])

    # S3: A-FC-5/RT-06 -- keys 2 and 3 arrive with no 0/1: zero output, and the
    # pointer never moves.  A "hole means skip" model releases both.
    s3 = witness["S3_hole_waits"]
    _a25_check("S3 zero output and frozen pointer",
               (s3["sink"], set(s3["next_key"])), ({}, {0}))

    # S4: A-FC-3/A-FC-4 and appendix correction 2 -- key 1000 is accepted and
    # occupies one of the 16 entries, and the whole 0..15 stream still leaves in
    # order, so "capacity is a key window" is refuted by the delivered stream.
    s4 = witness["S4_capacity_and_far_key"]
    _a25_check("S4 release sequence",
               [sequence for sequence, _ in s4["sink_order"]],
               list(range(_A25_CAPACITY)))
    _a25_check("S4 far key admitted but never released",
               (_A25_FAR_KEY in s4["sink"],
                s4["fault"], _A25_FAR_KEY >= _A25_CAPACITY),
               (False, [], True))
    _a25_check("S4 pointer after the in-window stream",
               (s4["next_key"][-1], s4["final"]),
               (_A25_CAPACITY, (_A25_CAPACITY, (1, 0, 0))))

    # S5: NG-01/A-FC-6/A-FC-10/T13 -- the second key 5 is a duplicate.  From the
    # epoch it reaches the head the chain is fail-closed: no output, no pointer
    # movement, no overwrite, and `ready` KEEPS reporting pure queue capacity.
    s5 = witness["S5_duplicate_stalls"]
    _a25_check("S5 first fault epoch", s5["fault"][:1], [3])
    _a25_check("S5 fault held to the end of the section",
               len(s5["fault"]), len(program[4]["takes"]) - 2)
    _a25_check("S5 stalled window (pointer, output, ready)",
               (set(s5["next_key"][2:]), s5["sink"], set(s5["ready_low"])),
               ({0}, {}, set()))
    _a25_check("S5 first key never released and never overwritten",
               s5["sink_order"], [])

    # S6: NG-03 -- 0 and 1 retire and leave, then a second key 0 is stale:
    # fail-closed from that epoch on, pointer frozen at 2, no new output.
    s6 = witness["S6_stale_stalls"]
    _a25_check("S6 release before the stall", s6["sink_order"],
               [(0, 0x60000000), (1, 0x60000001)])
    _a25_check("S6 stale fault epochs", s6["fault"][:1], [4])
    _a25_check("S6 pointer frozen / ready never lowered",
               (set(s6["next_key"][4:]), set(s6["ready_low"])), ({2}, set()))

    # S7: RT-10/TM-02/TM-03/BD-04/A-FC-8 -- 32 tokens offered with `take` low:
    # every storage fills (4 output + 16 table + 8 input = 28 in flight), which is
    # the only way `ready` can fall (T4), later offers are refused by the
    # handshake, and once `take` rises the whole stream leaves in order.
    s7 = witness["S7_backpressure_drain"]
    _a25_check("S7 accepted at the end of the blocked window",
               s7["accepted_trace"][39], 28)
    _a25_check("S7 peak in-flight occupancy", s7["max_occupancy"], 28)
    _a25_check("S7 handshake refusals",
               (len(s7["rejected"]) > 0, set(s7["rejected"]) <= set(s7["ready_low"])),
               (True, True))
    _a25_check("S7 in-order lossless drain",
               [sequence for sequence, _ in s7["sink_order"]],
               list(range(32)))
    _a25_check("S7 drained to empty", s7["final"], (32, (0, 0, 0)))
    assert s7["ready_low"], "T4: ready never fell, the backpressure claim is vacuous"

    # S8/S9: reset must be falsifiable (the A27 C2 lesson).  S8 ends with a FULL
    # table; S9's reset pair strikes it, and only a real reset -- table cleared
    # AND next_key back to start -- lets an in-order stream leave at all.
    s8 = witness["S8_fill_far_keys"]
    s9 = witness["S9_reset_then_in_order"]
    _a25_check("S8 full table at the reset",
               (s8["final"], s8["fault"], s8["ready_low"]),
               ((0, (16, 0, 0)), [], []))
    _a25_check("S9 reset strikes a non-empty chain",
               (sum(s9["pre_reset_occupancy"]) > 0, s9["pre_reset_occupancy"]),
               (True, (16, 0, 0)))
    _a25_check("S9 post-reset release sequence",
               [sequence for sequence, _ in s9["sink_order"]], list(range(8)))
    _a25_check("S9 post-reset fault free", s9["fault"], [])

    # (iii) global invariants: release is strictly increasing inside every
    #       section (RT-04), and only the two illegal-key sections ever fault.
    for section in program:
        sequences = [sequence for sequence, _ in
                     witness[section["name"]]["sink_order"]]
        assert all(a < b for a, b in zip(sequences, sequences[1:])), (  # noqa: B905 - preserve the extracted reference algorithm
            f"{section['name']}: release is not strictly increasing"
        )
    faults = [name for name in witness if witness[name]["fault"]]
    _a25_check("sections that raise fault", sorted(faults),
               ["S5_duplicate_stalls", "S6_stale_stalls"])

    # (iv) negative controls: the same stimulus must tell the frozen contract from
    #      each rival it exists to exclude, evaluated before any DUT runs.
    rivals = {}
    for label, overrides, clear_rst in (
        ("arrival_order", {"release": "arrival"}, False),
        ("skip_holes", {"release": "lowest"}, False),
        ("key_window", {"key_window": True}, False),
        ("duplicate_overwrite", {"dup_overwrites": True}, False),
        ("drop_on_blocked_output", {"drop_on_blocked": True}, False),
        ("ready_lowered_on_fault", {"ready_on_fault": True}, False),
        ("stale_accepted", {"stale_ok": True}, False),
        ("reset_ignored", {}, True),
    ):
        rival_rows = [dict(row) for row in rows]
        if clear_rst:
            for rival_row in rival_rows:
                rival_row["rst"] = 0
        rival = _a25_replay(rival_rows, _a25_knobs(**overrides))
        differing = sum(1 for ours, theirs in zip(rows, rival, strict=True)
                        if ours["expected"] != theirs)
        assert differing, f"vector set cannot distinguish {label}"
        rivals[label] = differing
    _a25_check("frozen model replay reproduces its own vectors",
               _a25_replay([dict(row) for row in rows], knobs),
               [row["expected"] for row in rows])

    return {
        "input_bits": _A25_TOKEN_BITS,
        "output_bits": _A25_RESULT_BITS,
        "rows": rows,
        "sink_order": {
            section["name"]: [sequence for sequence, _ in
                              witness[section["name"]]["sink_order"]]
            for section in program
        },
        "fault_epochs": {name: witness[name]["fault"] for name in witness},
        "ready_low_epochs": {name: witness[name]["ready_low"]
                             for name in witness},
        "accepted": {name: witness[name]["accepted"] for name in witness},
        "rival_differing_rows": rivals,
    }


# ---------------------------------------------------------------------------
# A25 reorder INDEPENDENT mode (``reorder_independent``) -- A25-T
#
# Why a second mode exists at all
# -------------------------------
# The ``reorder`` mode above shares the source's own vocabulary: an ARRAY of
# sixteen slots, a first-match scan for the next key, a first-free scan for the
# admission slot, and a per-slot occupation flag.  A test built on that shape can
# only show that the DUT agrees with a second array.  This mode is built on a
# different KIND of object, so that it can falsify the frozen contract rather
# than restate it:
#
# * the store is a MAP ``key -> (token, arrival)``.  That is the frozen block's
#   own shape (``queue_blocks.h:485`` ``std::map<uint64_t, T> entries_;``) and it
#   has no slot index, no free-slot policy and no scan order anywhere, so a DUT
#   whose behaviour depended on WHICH slot holds an entry would disagree with it.
# * "duplicate" is a map-membership test, occupancy is ``len(map)``, and the
#   released token is ``map[next_key]``.  ``capacity`` therefore can only ever be
#   read as an OCCUPANCY bound; a key window is not expressible in this model's
#   vocabulary at all.
# * the two queues are mathematical deques with absolute edge deadlines, so the
#   cadence claim (accept@E -> admit@E+1 -> retire@E+2 -> sink@E+3) is recomputed
#   from ``latency = 1`` instead of being copied from the delivered source.
#
# Every expected value is derived from exactly one of:
#
# * the contract clauses of the frozen ``A25/oracle.md`` sect. 3: (1) out-of-order
#   arrival is accepted and buffered, (5) a full store backpressures, (6) only a
#   token whose key equals the committed next key is released, (7) negative,
#   duplicate and already-retired keys are invalid;
# * the PM dispatch appendix ``A25/PRE-S-APPENDIX.md`` sect. 5, which fixes the
#   SHAPE of "invalid": missing key WAITS, duplicate / stale is a fail-closed
#   permanent stall that never lowers ``in_ready``, ``key >= start + capacity`` is
#   LEGAL (correction 2: ``capacity`` counts occupied entries), and ``next_key`` /
#   ``fault`` are pure observations (``C-R1``);
# * the two queue contracts (input depth 8 latency 1, output depth 4 latency 1,
#   both ``local_occupancy``);
# * ``A25-T``'s own hand-computed absolute epoch tables for T1, T2, T6 and T9,
#   written from the four clauses above and NOT from any model or DUT output.
#
# No DUT, RTL, emitted artifact or ``reorder`` mode value is read here.  The
# companion checker ``reorder_independent_check.py`` re-derives the contract
# facts from the emitted artifacts, runs the four-state (Icarus) probes and
# falsifies the rivals below.
# ---------------------------------------------------------------------------

_A25I_CAPACITY = 16
_A25I_IN_DEPTH = 8
_A25I_OUT_DEPTH = 4
_A25I_LATENCY = 1
_A25I_SEQ_BITS = 32
_A25I_VAL_BITS = 32
_A25I_TOKEN_BITS = _A25I_SEQ_BITS + _A25I_VAL_BITS
_A25I_KEY_BITS = 64
_A25I_RESULT_BITS = 1 + 1 + _A25I_TOKEN_BITS + _A25I_KEY_BITS + 1
_A25I_SEQ_MASK = (1 << _A25I_SEQ_BITS) - 1
_A25I_VAL_MASK = (1 << _A25I_VAL_BITS) - 1
_A25I_FAR = 1000


def _a25i_token(sequence, value):
    """Pack ``Token`` the way its declaration order implies (MSB first)."""
    return ((sequence & _A25I_SEQ_MASK) << _A25I_VAL_BITS) | (value & _A25I_VAL_MASK)


def _a25i_sequence(packed):
    return (packed >> _A25I_VAL_BITS) & _A25I_SEQ_MASK


def _a25i_value(packed):
    return packed & _A25I_VAL_MASK


class _A25iQueue:
    """One queue as mathematics: a deque plus absolute availability deadlines.

    A token handed to ``push`` on edge ``E`` carries deadline ``E + latency`` and
    is first visible to its consumer on that deadline edge (``latency = 1`` means
    "visible on the next edge").  ``ready`` is ``local_occupancy``: the pre-edge
    occupancy, so a slot released on edge ``E`` is never refilled on ``E``.
    """

    def __init__(self, depth, latency=_A25I_LATENCY):
        self.depth = depth
        self.latency = latency
        self.tokens = deque()

    def count(self):
        return len(self.tokens)

    def ready(self):
        return len(self.tokens) < self.depth

    def visible(self, edge):
        return bool(self.tokens) and self.tokens[0][0] <= edge

    def value(self, edge):
        return self.tokens[0][1] if self.visible(edge) else 0

    def transfer(self, edge, push, token, pop):
        if pop:
            self.tokens.popleft()
        if push:
            self.tokens.append((edge + self.latency, token))
        assert len(self.tokens) <= self.depth, (len(self.tokens), self.depth)


def _a25i_stimulus(four_state=False):
    """Explicit per-epoch stimulus: every offer, hold, gap and reset is by hand.

    ``drive`` is not a policy here.  Each epoch states its own ``valid`` /
    ``take`` / payload, which is what makes a held offer (the same token offered
    on several consecutive epochs, exactly as the shared harness would) an
    explicit authoring decision instead of a side effect of a cursor rule.

    ``four_state`` drives the T9 idle epochs with x / z instead of a known zero.
    It is off for ``reorder_independent`` because that mode also runs Verilator,
    which must never be cited for an x/z claim and which -- measurably, on
    Verilator 5.044 -- miscompiles a large generated row file that contains a
    ``z`` literal at all.  The ``reorder_independent_raw`` variant turns it on:
    that mode is ``_raw``-suffixed, so the harness runs the native model and
    Icarus (both four-state capable) and skips Verilator.
    """
    rows = []
    spans = {}

    def offer(valid=0, take=0, token=None, unknown=None, rst=0):
        data = 0 if token is None else token
        row = {
            "clk": 0,
            "rst": int(rst),
            "valid": int(valid),
            "take": int(take),
            "data": int(data),
        }
        if unknown is not None:
            # Four-state stimulus: the payload bus is entirely x or entirely z.
            # The hpp still carries `data`, which is the value a two-state
            # simulator folds the literal to.
            row["data_known"] = 0
            row["data_z"] = ((1 << _A25I_TOKEN_BITS) - 1
                             if unknown == "z" else 0)
        rows.append(row)
        rows.append(dict(row, clk=1))

    def begin(name):
        spans[name] = [len(rows) // 2 + 1]

    def end(name):
        spans[name].append(len(rows) // 2)

    def open_section(name):
        """Start a section at `start`: pointer 0, empty store, empty queues.

        The reset edge is emitted BEFORE the section's span opens, so local epoch
        1 is always the section's first data epoch and the hand-computed absolute
        tables below are stated in the same numbering the sections use.
        """
        offer(rst=1)
        begin(name)

    # ---- T1: a full permutation of 0..9, with a hand-computed sink table.
    open_section("T1_permutation")
    for key in (9, 4, 7, 0, 2, 8, 1, 6, 3, 5):
        offer(valid=1, take=1, token=_a25i_token(key, 0xA1000000 | key))
    for _ in range(10):
        offer(take=1)
    end("T1_permutation")

    # ---- T2: keys 5,3,4 arrive with 0,1,2 still missing: the hole WAITS.
    open_section("T2_hole_then_fill")
    for key in (5, 3, 4, 0, 1, 2):
        offer(valid=1, take=1, token=_a25i_token(key, 0xA2000000 | key))
    for _ in range(7):
        offer(take=1)
    end("T2_hole_then_fill")

    # ---- T3: sixteen far keys (all >= capacity).  They are ACCEPTED, and the
    #      only reason `ready` finally falls is that the store holds all sixteen
    #      of them: a "capacity is a key window" DUT refuses every one and fills
    #      its input queue sixteen edges earlier.
    open_section("T3_far_fill_occupancy")
    for index in range(_A25I_CAPACITY):
        offer(valid=1, token=_a25i_token(_A25I_FAR + index, 0xA3000000 | index))
    for _ in range(11):
        offer(valid=1, token=_a25i_token(_A25I_FAR + _A25I_CAPACITY, 0xA3001000))
    for _ in range(6):
        offer(take=1)
    end("T3_far_fill_occupancy")

    # ---- T4: one far key plus fifteen in-window keys, every one of them ABOVE
    #      the pointer and key 0 absent, so nothing ever retires and the store
    #      occupancy is exactly the number of accepted tokens.  The far key is one
    #      of the sixteen, so the store fills one edge earlier than it would if
    #      the far key had been dropped -- which is the only way the acceptance of
    #      a far key becomes observable at all (see the checker's silent-drop
    #      control).
    open_section("T4_far_slot_blocks_in_window")
    offer(valid=1, token=_a25i_token(_A25I_FAR, 0xA4000000))
    for key in range(1, _A25I_CAPACITY):
        offer(valid=1, token=_a25i_token(key, 0xA4000000 | key))
    for _ in range(11):
        offer(valid=1, token=_a25i_token(_A25I_CAPACITY, 0xA4001000))
    for _ in range(4):
        offer()
    end("T4_far_slot_blocks_in_window")

    # ---- T4b: the store is FULL and the head is a DUPLICATE.  `fault` must be
    #      ZERO here: the frozen block short-circuits on `entries_.size() <
    #      capacity_` (``queue_blocks.h:414``) and never looks at the key when the
    #      store is full, so the `free` conjunct is part of the observation.  This
    #      is the only shape in which the two readings of `fault` differ.
    open_section("T4b_full_store_duplicate_head")
    for index in range(_A25I_CAPACITY):
        offer(valid=1, token=_a25i_token(20 + index, 0xA4B00000 | index))
    for _ in range(14):
        offer(valid=1, token=_a25i_token(20, 0xA4B00000))
    for _ in range(4):
        offer()
    end("T4b_full_store_duplicate_head")

    # ---- T4c: the key field is carried at FULL WIDTH into the compare domain.
    #      Two groups of eight keys whose low eight bits are identical: a DUT that
    #      compared a truncated key would see the second group as DUPLICATES and
    #      raise `fault` from local 10 on, while a full-width DUT sees sixteen
    #      distinct keys, stores all of them, raises no fault, and only then runs
    #      out of store.  This is the reachable part of ``BD-05``'s intent: the
    #      2**32 crossing itself needs 2**32 retirements and stays unreachable.
    open_section("T4c_high_key_field_full_width")
    for index in range(8):
        offer(valid=1, token=_a25i_token(0x00000100 + index, 0xA4C00000 | index))
    for index in range(8):
        offer(valid=1, token=_a25i_token(0xFFFFFF00 + index, 0xA4C10000 | index))
    for _ in range(11):
        offer(valid=1, token=_a25i_token(0xFFFFFFFF, 0xA4C1FFFF))
    for _ in range(4):
        offer()
    end("T4c_high_key_field_full_width")

    # ---- T5: key 1 is stored and then offered again.  For one edge it is a
    #      DUPLICATE and then, because it retires on that same edge, a STALE copy
    #      of it is left at the input head for the rest of the section.  Either
    #      way the block is fail-closed, so the second half of the section is a
    #      permanent stall with a frozen pointer, an untouched `ready` and no
    #      bypass for the later key 7.
    open_section("T5_duplicate_then_stale_stall")
    offer(valid=1, take=1, token=_a25i_token(0, 0xA5000000))
    offer(valid=1, take=1, token=_a25i_token(1, 0xA5000001))
    offer(valid=1, take=1, token=_a25i_token(1, 0xA5000001))
    for _ in range(6):
        offer(take=1)
    offer(valid=1, take=1, token=_a25i_token(7, 0xA5000007))
    for _ in range(13):
        offer(take=1)
    end("T5_duplicate_then_stale_stall")

    # ---- T6: a stale key on arrival (0 after 0,1,2 retired).
    open_section("T6_stale_on_arrival")
    for key in (0, 1, 2, 0):
        offer(valid=1, take=1, token=_a25i_token(key, 0xA6000000 | key))
    for _ in range(12):
        offer(take=1)
    end("T6_stale_on_arrival")

    # ---- T7: the reset edge strikes a store holding fifteen far keys plus one
    #      token still in the input queue; only a real reset lets 0..7 leave.
    open_section("T7_reset_clears_full_store")
    for index in range(_A25I_CAPACITY):
        offer(valid=1, token=_a25i_token(_A25I_FAR + index, 0xA7000000 | index))
    offer(rst=1)
    for key in range(8):
        offer(valid=1, take=1, token=_a25i_token(key, 0xA7000000 | key))
    for _ in range(4):
        offer(take=1)
    end("T7_reset_clears_full_store")

    # ---- T8: the backpressure chain.  `take` is held low for the whole fill, so
    #      the output queue (4) fills first, then the store (16), then the input
    #      queue (8): 28 tokens in flight, and `ready` falls only at the 29th
    #      edge.  `take` then rises and the stream drains in order and losslessly.
    open_section("T8_backpressure_then_drain")
    for key in range(32):
        offer(valid=1, token=_a25i_token(key, 0xA8000000 | key))
    for _ in range(8):
        offer()
    for _ in range(40):
        offer(take=1)
    end("T8_backpressure_then_drain")

    # ---- T9: alternating offer / idle, with a hand-computed sink table.  In the
    #      plain mode the idle epochs drive a known zero; in the `_raw` variant
    #      they drive x and z, because `valid` is low there so no transfer is
    #      effective and every output bit must still be known (the frozen contract
    #      has no x/z result, and the unknown-control channel is reserved for an
    #      unknown that is actually EFFECTIVE).
    open_section("T9_masked_unknown")
    for index, key in enumerate((0, 1, 2, 3, 4, 5)):
        offer(valid=1, take=1, token=_a25i_token(key, 0xA9000000 | key))
        offer(take=1, unknown=(("x" if index % 2 == 0 else "z")
                               if four_state else None))
    for _ in range(4):
        offer(take=1)
    end("T9_masked_unknown")

    return rows, {name: tuple(span) for name, span in spans.items()}


def _a25i_knobs(**overrides):
    """The frozen contract, as a set of switchable decisions.

    Every switch defaults to the frozen reading; the rivals below flip exactly
    one of them, which is what makes the negative controls honest.
    """
    base = {
        "capacity": _A25I_CAPACITY,
        "key_bits": _A25I_KEY_BITS,  # store/compare width of the key field
        "release": "by_key",       # by_key | arrival | lowest | passthrough
        "key_window": False,       # capacity as a key window instead of occupancy
        "out_of_window_dropped": False,  # key >= capacity consumed but not stored
        "dup_overwrites": False,   # a duplicate silently replaces the stored one
        "drop_on_blocked": False,  # release even when the output queue is full
        "ready_on_fault": False,   # lower in_ready on an illegal head
        "stale_ok": False,         # already-retired keys are accepted
        "latency_zero": False,     # latency=1 implemented as pure combinational
        "fault_ignores_free": False,  # fault without the `free` conjunct
    }
    return dict(base, **overrides)


def _a25i_read(state, edge, take, knobs):
    """One epoch of the reorder contract, read from the committed pre-edge state.

    Only the four contract clauses are used; the store is addressed by the full
    key, so no term below can depend on a physical slot.
    """
    in_q, out_q, store = state["in_q"], state["out_q"], state["store"]
    in_valid = in_q.visible(edge)
    in_token = in_q.value(edge)
    in_ready = in_q.ready()
    out_ready = out_q.ready()
    out_valid = out_q.visible(edge)
    out_token = out_q.value(edge)
    next_key = state["next_key"]
    incoming_key = _a25i_sequence(in_token) & ((1 << knobs["key_bits"]) - 1)
    occupied = len(store)

    free = occupied < knobs["capacity"]
    duplicate = bool(in_valid) and incoming_key in store
    stale = bool(in_valid) and incoming_key < next_key
    illegal = stale if knobs["stale_ok"] else (stale or duplicate)
    if knobs["key_window"] and in_valid and incoming_key >= knobs["capacity"]:
        illegal = True

    fault = bool(in_valid) and (illegal if knobs["fault_ignores_free"]
                                else (free and illegal))
    if knobs["dup_overwrites"]:
        admit = bool(in_valid) and free and not stale
    else:
        admit = bool(in_valid) and free and not illegal

    # Clause 6: the only releasable entry is the one whose key IS `next_key`.
    # `admit` is decided above and does not depend on `match_valid`, which is what
    # lets the `latency_zero` rival below retire the token it is admitting on the
    # same edge (``TM-01``: the reorder's own ``latency=1`` stage, and the output
    # queue's, implemented as a pure combinational path).
    if knobs["release"] == "passthrough":
        match_valid = bool(in_valid)
        release_token = in_token
        same_edge = False
    elif knobs["release"] == "arrival":
        oldest = min(store.items(), key=lambda item: item[1][1], default=None)
        match_valid = oldest is not None
        release_token = oldest[1][0] if oldest else 0
        same_edge = False
    elif knobs["release"] == "lowest":
        lowest = min(store, default=None)
        match_valid = lowest is not None
        release_token = store[lowest][0] if lowest is not None else 0
        same_edge = False
    else:
        held = next_key in store
        same_edge = bool(admit) and knobs["latency_zero"] and not held \
            and incoming_key == next_key
        match_valid = held or same_edge
        release_token = store[next_key][0] if held else (in_token if same_edge else 0)

    if knobs["ready_on_fault"]:
        # Rival only: the explicitly forbidden "pull in_ready low on an illegal
        # head" reading.  `ready` is otherwise pure input-queue capacity.
        in_ready = in_ready and not fault
    retire = match_valid and (out_ready or knobs["drop_on_blocked"])
    push = match_valid and out_ready
    # A zero-latency output stage exposes the retirement on the very edge it is
    # pushed; if the consumer takes it on that edge the token never enters the
    # queue at all.
    exposed = bool(retire) and knobs["latency_zero"] and not out_valid
    available = bool(out_valid) or exposed
    head = out_token if out_valid else (release_token if exposed else 0)
    return {
        "ready": in_ready,
        "available": available,
        "head": head,
        "next_key": next_key,
        "fault": fault,
        "in_valid": bool(in_valid),
        "in_token": in_token,
        "incoming_key": incoming_key,
        "out_valid": bool(out_valid),
        "out_token": out_token,
        "out_ready": out_ready,
        "store_keys": frozenset(store),
        "occupied": occupied,
        "free": free,
        "duplicate": duplicate,
        "stale": stale,
        "admit": admit,
        "retire": retire,
        "push": push,
        "same_edge": same_edge,
        "exposed": exposed,
        "release_token": release_token,
        "expected": _pack(
            (
                (1, int(in_ready)),
                (1, int(available)),
                (_A25I_TOKEN_BITS, head),
                (_A25I_KEY_BITS, next_key),
                (1, int(fault)),
            )
        ),
    }


def _a25i_commit(state, edge, read, knobs, valid, data, take, arrival):
    """Commit one rising edge from the combinational result ``read``."""
    in_q, out_q, store = state["in_q"], state["out_q"], state["store"]
    in_q.transfer(edge, bool(valid) and read["ready"], data, read["admit"])
    # `local_occupancy` output queue: a slot freed on this edge is never refilled
    # on this edge, and a token the zero-latency rival exposed (and the consumer
    # took) on this edge never enters the queue at all.
    out_q.transfer(edge,
                   read["push"] and not (read["exposed"] and take),
                   read["release_token"],
                   read["out_valid"] and take)
    if read["retire"]:
        if knobs["release"] == "passthrough":
            pass
        elif knobs["release"] == "by_key":
            if state["next_key"] in store:
                del store[state["next_key"]]
            state["next_key"] += 1
        elif knobs["release"] == "arrival":
            key = min(store, key=lambda item: store[item][1])
            del store[key]
            state["next_key"] = key + 1
        else:
            key = min(store)
            del store[key]
            state["next_key"] = key + 1
    if read["admit"] and not read["same_edge"]:
        # A same-edge retirement consumed the incoming token before it could be
        # stored, which is exactly the register the frozen `latency=1` buys.
        key = read["incoming_key"]
        if knobs["out_of_window_dropped"] and key >= knobs["capacity"]:
            # Rival only: "key >= capacity" read as an illegal stimulus that is
            # consumed and thrown away.  The token leaves the input queue exactly
            # as in the frozen contract; only the store write is missing.
            pass
        elif knobs["dup_overwrites"] and read["duplicate"] and key in store:
            # Rival only: a duplicate silently replaces the stored token.  If the
            # same commit retired that key the entry is already gone and this
            # degenerates to an ordinary admission, which is the rival's own
            # semantics, not a special case for this stimulus.
            store[key] = (read["in_token"], store[key][1])
        else:
            store[key] = (read["in_token"], arrival)
    return arrival + 1


def _a25i_state(knobs):
    """Fresh committed state: both queues empty, store empty, pointer at `start`.

    ``latency_zero`` does NOT shorten the queue deadlines here.  A one-edge queue
    deadline is unobservable under the shared pre-edge sampling convention (the
    consumer's next read is the next epoch either way), so the rival is modelled
    where it IS observable: as a combinational path through the reorder's own
    stage and the output queue (see ``_a25i_read``).
    """
    del knobs
    return {
        "in_q": _A25iQueue(_A25I_IN_DEPTH, _A25I_LATENCY),
        "out_q": _A25iQueue(_A25I_OUT_DEPTH, _A25I_LATENCY),
        "store": {},
        "next_key": 0,
    }


def _a25i_occupancy(state):
    return (len(state["store"]), state["in_q"].count(), state["out_q"].count())


def _a25i_run(rows, knobs):
    """Recompute ``expected`` for fixed stimulus rows; return it with the trace.

    The sampling convention is the shared one (``credit`` /
    ``route_merge_independent`` / ``reorder``): both rows of an epoch carry the
    committed pre-edge state read together with that epoch's inputs, and the
    ``clk = 1`` row is the edge that commits them.
    """
    state = _a25i_state(knobs)
    expected = []
    trace = []
    edge = 0
    arrival = 0
    previous = 0
    for index, row in enumerate(rows):
        rising = bool(row["clk"]) and not previous
        read = _a25i_read(state, edge, row["take"], knobs)
        expected.append(read["expected"])
        trace.append(
            {
                "row": index,
                "epoch": index // 2 + 1,
                "edge": edge,
                "rising": rising,
                "rst": bool(row["rst"]),
                "take": bool(row["take"]),
                "driven": bool(row["valid"]),
                "driven_token": row["data"],
                "take_rule": read["admit"],
                "push": read["push"],
                "retire": read["retire"],
                "sink": bool(read["out_valid"] and row["take"]),
                "sink_sequence": _a25i_sequence(read["out_token"]),
                "sink_value": _a25i_value(read["out_token"]),
                "sink_key_expected": read["next_key"],
                "available": bool(read["out_valid"]),
                "ready": bool(read["ready"]),
                "fault": bool(read["fault"]),
                "next_key": read["next_key"],
                "occupied": read["occupied"],
                "free": read["free"],
                "duplicate": read["duplicate"],
                "stale": read["stale"],
                "in_flight": sum(_a25i_occupancy(state)),
                "occupancy": _a25i_occupancy(state),
            }
        )
        if rising and row["rst"]:
            state = _a25i_state(knobs)
            edge = 0
            arrival = 0
        elif rising:
            arrival = _a25i_commit(state, edge, read, knobs, row["valid"],
                                   row["data"], row["take"], arrival)
            edge += 1
        previous = row["clk"]
    return expected, trace


def _a25i_epochs(trace, span):
    """The trace entries of one section, renumbered from 1 inside the section.

    Both rows of an epoch carry the same committed pre-edge read, so exactly one
    entry per epoch is kept (the ``clk = 0`` row) to make the per-section tables
    comparable with a hand-computed epoch table.
    """
    first, last = span
    kept = []
    for entry in trace:
        if first <= entry["epoch"] <= last and not entry["rising"]:
            kept.append(dict(entry, local=entry["epoch"] - first + 1))
    return kept


def _a25i_sinks(entries):
    return {entry["local"]: entry["sink_sequence"]
            for entry in entries if entry["sink"]}


def _a25i_peaks(entries):
    return max(entry["in_flight"] for entry in entries)


def _a25i_independent(four_state=False):
    """Frozen reorder contract -> an independently derived vector set."""
    stimulus, spans = _a25i_stimulus(four_state)
    knobs = _a25i_knobs()
    expected, trace = _a25i_run(stimulus, knobs)
    rows = [dict(row, expected=value)
            for row, value in zip(stimulus, expected, strict=True)]
    sections = {name: _a25i_epochs(trace, span) for name, span in spans.items()}
    sink_tables = {name: _a25i_sinks(entries)
                   for name, entries in sections.items()}

    # (i) HAND-COMPUTED ABSOLUTE EPOCH TABLES.  Written from the four contract
    #     clauses plus "a token handed over on edge E is visible on E+1" and
    #     "a retirement on edge E is pushed with deadline E+1", before any model
    #     or DUT ran.  They pin the release order AND the cadence, including the
    #     non-uniform gap in T1 where key 1 has not reached the store yet.
    _a25_check(
        "T1 sink table (epoch -> sequence), hand-computed",
        sink_tables["T1_permutation"],
        {7: 0, 10: 1, 11: 2, 12: 3, 13: 4, 14: 5, 15: 6, 16: 7, 17: 8, 18: 9},
    )
    _a25_check(
        "T2 sink table (epoch -> sequence), hand-computed",
        sink_tables["T2_hole_then_fill"],
        {7: 0, 8: 1, 9: 2, 10: 3, 11: 4, 12: 5},
    )
    _a25_check(
        "T6 sink table (epoch -> sequence), hand-computed",
        sink_tables["T6_stale_on_arrival"],
        {4: 0, 5: 1, 6: 2},
    )
    _a25_check(
        "T9 masked-unknown sink table (epoch -> sequence), hand-computed",
        sink_tables["T9_masked_unknown"],
        {4: 0, 6: 1, 8: 2, 10: 3, 12: 4, 14: 5},
    )

    # (ii) T1/T2: the payload pair travels untouched and the release order is the
    #      key order, not the arrival order (RT-01/RT-04/RT-05/BD-07/TM-01).
    for name in ("T1_permutation", "T2_hole_then_fill"):
        pairs = [(entry["sink_sequence"], entry["sink_value"])
                 for entry in sections[name] if entry["sink"]]
        assert all(sequence < following for (sequence, _), (following, _)
                   in zip(pairs, pairs[1:])), (name, pairs)  # noqa: B905 - preserve the extracted reference algorithm
        assert all((0xA0000000 | sequence) == value
                   or (value >> 24) in (0xA1, 0xA2) for sequence, value in pairs)
        assert [sequence for sequence, _ in pairs] == list(
            range(len(pairs))), (name, pairs)

    # (iii) T3: every far key is ACCEPTED.  The proof is not a value but an
    #       epoch: the store holds all sixteen by `local` 18, so the input queue
    #       is the only thing left to fill and `ready` falls at `local` 25.  A
    #       DUT that treated `capacity` as a key window would refuse all sixteen,
    #       never pop its input queue and fall already at `local` 9.
    t3 = sections["T3_far_fill_occupancy"]
    _a25_check("T3 the store reaches all sixteen far keys at local 18",
               [(e["local"], e["occupied"]) for e in t3 if e["occupied"] == 16][:1],
               [(18, 16)])
    _a25_check("T3 first `ready`-low epoch", [e["local"] for e in t3
                                              if not e["ready"]][:1], [25])
    _a25_check("T3 ready is high at local 9 (a key-window DUT is already low)",
               t3[8]["ready"], True)
    _a25_check("T3 zero output, zero fault, pointer frozen at start",
               (sink_tables["T3_far_fill_occupancy"],
                [e["local"] for e in t3 if e["fault"]],
                {e["next_key"] for e in t3}), ({}, [], {0}))
    _a25_check("T3 final occupancy (store, input, output)",
               t3[-1]["occupancy"], (16, 8, 0))

    # (iv) T4: the far key occupies ONE of the sixteen slots, so the sixteenth
    #      in-window key is blocked until a slot is freed.  `ready` falls at the
    #      same relative epoch as in T3 because the store -- not the key domain --
    #      is what ran out.
    t4 = sections["T4_far_slot_blocks_in_window"]
    _a25_check("T4 the store reaches all sixteen entries at local 18",
               [(e["local"], e["occupied"]) for e in t4 if e["occupied"] == 16][:1],
               [(18, 16)])
    _a25_check("T4 first `ready`-low epoch",
               [e["local"] for e in t4 if not e["ready"]][:1], [25])
    _a25_check("T4 zero output and zero fault", (sink_tables[
        "T4_far_slot_blocks_in_window"], [e["local"] for e in t4 if e["fault"]]),
        ({}, []))

    # (iv-b) T4b: a duplicate at the head of a FULL store is NOT a fault.  The
    #        store short-circuits before the key comparison, so `fault` carries
    #        the `free` conjunct; a DUT that dropped it would raise `fault` on
    #        every epoch from local 18 to the end of the section.
    t4b = sections["T4b_full_store_duplicate_head"]
    _a25_check("T4b the head is a duplicate of a stored key while full",
               (t4b[17]["occupied"], t4b[17]["duplicate"], t4b[17]["free"]),
               (16, True, False))
    _a25_check("T4b `fault` stays low on the full-store duplicate",
               [e["local"] for e in t4b if e["fault"]], [])
    _a25_check("T4b first `ready`-low epoch and zero output",
               ([e["local"] for e in t4b if not e["ready"]][:1],
                sink_tables["T4b_full_store_duplicate_head"]), ([25], {}))
    _a25_check("T4b final occupancy (store, input, output)",
               t4b[-1]["occupancy"], (16, 8, 0))

    # (iv-c) T4c: the key is compared at FULL WIDTH.  Eight keys 0x100..0x107 and
    #        eight keys 0xFFFFFF00..0xFFFFFF07 have identical low bytes; a DUT that
    #        compared a truncated key would call the second group duplicates and
    #        raise `fault` from local 10 on.  The delivered bits instead fill the
    #        store with sixteen DISTINCT entries and never fault.
    t4c = sections["T4c_high_key_field_full_width"]
    _a25_check("T4c the store reaches all sixteen distinct keys at local 18",
               [(e["local"], e["occupied"]) for e in t4c if e["occupied"] == 16][:1],
               [(18, 16)])
    _a25_check("T4c first `ready`-low epoch, zero fault and zero output",
               ([e["local"] for e in t4c if not e["ready"]][:1],
                [e["local"] for e in t4c if e["fault"]],
                sink_tables["T4c_high_key_field_full_width"]), ([25], [], {}))
    _a25_check("T4c the pointer never leaves start",
               {e["next_key"] for e in t4c}, {0})

    # (v) T5: duplicate -> stale is fail-closed and PERMANENT.  `fault` is high on
    #     every epoch from the first duplicate to the end of the section, the
    #     pointer freezes, nothing is overwritten, nothing is dropped, the later
    #     key 7 never bypasses the stuck head, and `ready` -- pure input-queue
    #     capacity -- is never lowered (C-R1).
    t5 = sections["T5_duplicate_then_stale_stall"]
    _a25_check("T5 fault starts at local 4 and never clears",
               [e["local"] for e in t5 if e["fault"]],
               list(range(4, len(t5) + 1)))
    _a25_check("T5 duplicate on the first fault edge, stale afterwards",
               (t5[3]["duplicate"], t5[4]["duplicate"], t5[4]["stale"]),
               (True, False, True))
    _a25_check("T5 release before the stall and nothing after",
               sink_tables["T5_duplicate_then_stale_stall"], {4: 0, 5: 1})
    _a25_check("T5 pointer frozen at 2 and never lowered `ready`",
               ({e["next_key"] for e in t5[4:]}, {e["ready"] for e in t5}),
               ({2}, {True}))
    _a25_check("T5 the later key 7 never bypasses the stuck head",
               (t5[-1]["occupancy"][1] >= 1,
                all(entry["sink_sequence"] < 2 for entry in t5 if entry["sink"])),
               (True, True))

    # (vi) T6: an already-retired key on arrival is stale, not a duplicate, and is
    #      fail-closed by the same rule.
    t6 = sections["T6_stale_on_arrival"]
    _a25_check("T6 fault starts at local 5 and never clears",
               [e["local"] for e in t6 if e["fault"]],
               list(range(5, len(t6) + 1)))
    _a25_check("T6 stale but never duplicate, pointer frozen at 3",
               ({e["stale"] for e in t6[4:]}, {e["duplicate"] for e in t6},
                {e["next_key"] for e in t6[5:]}),
               ({True}, {False}, {3}))
    _a25_check("T6 `ready` is never lowered", {e["ready"] for e in t6}, {True})

    # (vii) T7: the reset edge strikes a NON-EMPTY store and input queue, and only
    #       a real reset lets 0..7 leave at all (the A27 C2 lesson: a reset that
    #       struck an empty chain would be unfalsifiable).
    t7 = sections["T7_reset_clears_full_store"]
    _a25_check("T7 reset strikes a non-empty chain",
               (t7[16]["rst"], t7[16]["occupancy"], sum(t7[16]["occupancy"]) > 0),
               (True, (15, 1, 0), True))
    _a25_check("T7 post-reset release table (epoch -> sequence)",
               sink_tables["T7_reset_clears_full_store"],
               {21: 0, 22: 1, 23: 2, 24: 3, 25: 4, 26: 5, 27: 6, 28: 7})
    _a25_check("T7 post-reset pointer and fault-free drain",
               (t7[-1]["next_key"], [e["local"] for e in t7[17:] if e["fault"]]),
               (8, []))

    # (viii) T8: the backpressure chain bottoms out at exactly 4 + 16 + 8 = 28
    #        tokens in flight, `ready` falls at the 29th edge, only 28 are ever
    #        accepted, and the drain is in order and lossless.
    t8 = sections["T8_backpressure_then_drain"]
    _a25_check("T8 first `ready`-low epoch",
               [e["local"] for e in t8 if not e["ready"]][:1], [29])
    _a25_check("T8 peak in-flight occupancy (4 out + 16 store + 8 in)",
               _a25i_peaks(t8), 28)
    _a25_check("T8 accepted tokens under backpressure",
               sum(1 for e in t8 if e["take_rule"]), 28)
    _a25_check("T8 lossless in-order drain",
               sink_tables["T8_backpressure_then_drain"],
               {41 + index: index for index in range(28)})
    _a25_check("T8 drained to empty at pointer 28",
               (t8[-1]["next_key"], t8[-1]["occupancy"]), (28, (0, 0, 0)))
    _a25_check("T8 fault free throughout", [e["local"] for e in t8 if e["fault"]],
               [])

    # (ix) T9 four-state bookkeeping.  In the `_raw` variant the unknown rows
    #      really carry an unknown payload bus and are never effective transfers,
    #      so every expected bit of BOTH modes is still fully two-state; in the
    #      plain mode there is no unknown row at all (it also runs Verilator).
    unknown_rows = [row for row in rows
                    if row.get("data_known", (1 << _A25I_TOKEN_BITS) - 1) == 0]
    _a25_check("T9 unknown-payload rows", len(unknown_rows),
               12 if four_state else 0)
    if four_state:
        _a25_check("T9 no unknown payload is ever offered with `valid` high",
                   {row["valid"] for row in unknown_rows}, {0})
        _a25_check("T9 unknown rows carry both x and z",
                   sorted({row["data_z"] for row in unknown_rows}),
                   [0, (1 << _A25I_TOKEN_BITS) - 1])
    _a25_check("T9 every expected value is fully two-state",
               all(0 <= row["expected"] < (1 << _A25I_RESULT_BITS) for row in rows),
               True)

    # (x) global invariants over every section at once.
    releases = [(entry["epoch"], entry["sink_sequence"])
                for entry in trace if entry["sink"]]
    by_section = {}
    for name, (first, last) in spans.items():  # noqa: B007 - preserve the extracted reference algorithm
        sequences = [entry["sink_sequence"] for entry in sections[name]
                     if entry["sink"]]
        assert all(a < b for a, b in zip(sequences, sequences[1:])), (name, sequences)  # noqa: B905 - preserve the extracted reference algorithm
        by_section[name] = sequences
    _a25_check("T1/T2/T6/T8/T9 release the exact contiguous run from start",
               {name: by_section[name] for name in
                ("T1_permutation", "T2_hole_then_fill", "T6_stale_on_arrival",
                 "T7_reset_clears_full_store", "T8_backpressure_then_drain",
                 "T9_masked_unknown")},
               {"T1_permutation": list(range(10)),
                "T2_hole_then_fill": list(range(6)),
                "T6_stale_on_arrival": list(range(3)),
                "T7_reset_clears_full_store": list(range(8)),
                "T8_backpressure_then_drain": list(range(28)),
                "T9_masked_unknown": list(range(6))})
    _a25_check("far-key sections release nothing at all",
               (by_section["T3_far_fill_occupancy"],
                by_section["T4_far_slot_blocks_in_window"],
                by_section["T4b_full_store_duplicate_head"],
                by_section["T4c_high_key_field_full_width"]), ([], [], [], []))
    _a25_check("sections that ever fault",
               sorted({name for name, entries in sections.items()
                       if any(entry["fault"] for entry in entries)}),
               ["T5_duplicate_then_stale_stall", "T6_stale_on_arrival"])
    del releases

    # (xi) NEGATIVE CONTROLS.  Each rival is the same stimulus replayed through
    #      exactly one flipped decision, and the vector set must be able to tell
    #      it from the frozen contract -- otherwise the corresponding assertion
    #      above would be unfalsifiable.  Evaluated before any DUT runs.
    rivals = {}
    for label, overrides, clear_rst in (
        ("arrival_order", {"release": "arrival"}, False),
        ("skip_holes", {"release": "lowest"}, False),
        ("passthrough_fifo", {"release": "passthrough"}, False),
        ("key_window", {"key_window": True}, False),
        ("out_of_window_dropped", {"out_of_window_dropped": True}, False),
        ("duplicate_overwrite", {"dup_overwrites": True}, False),
        ("drop_on_blocked_output", {"drop_on_blocked": True}, False),
        ("ready_lowered_on_fault", {"ready_on_fault": True}, False),
        ("stale_accepted", {"stale_ok": True}, False),
        ("key_truncated_8", {"key_bits": 8}, False),
        ("combinational_latency", {"latency_zero": True}, False),
        ("fault_without_free", {"fault_ignores_free": True}, False),
        ("reset_ignored", {}, True),
    ):
        rival_rows = [dict(row) for row in stimulus]
        if clear_rst:
            for rival_row in rival_rows:
                rival_row["rst"] = 0
        rival_expected, _ = _a25i_run(rival_rows, _a25i_knobs(**overrides))
        differing = sum(1 for ours, theirs in zip(expected, rival_expected,
                                                  strict=True)
                        if ours != theirs)
        assert differing, f"vector set cannot distinguish {label}"
        rivals[label] = differing

    # (xi-b) TARGETED reset control (the A27 ``C2`` lesson, sharpened): clearing
    #        every ``rst`` in the whole program is a coarse control because a
    #        reset-ignoring DUT then never restarts any section.  The interesting
    #        claim is narrower -- the store is full of far keys and the pointer is
    #        still at 0 when T7's OWN reset edge arrives, so if exactly that one
    #        edge is dropped the section must produce no output at all.  Both
    #        controls are reported; the second is the one that isolates the edge.
    t7_reset_epoch = spans["T7_reset_clears_full_store"][0] + 16
    targeted_rows = [dict(row) for row in stimulus]
    for index in (2 * (t7_reset_epoch - 1), 2 * (t7_reset_epoch - 1) + 1):
        assert targeted_rows[index]["rst"] == 1, index
        targeted_rows[index]["rst"] = 0
    targeted_expected, _ = _a25i_run(targeted_rows, knobs)
    targeted = sum(1 for ours, theirs in zip(expected, targeted_expected,
                                             strict=True) if ours != theirs)
    assert targeted, "T7's own reset edge is unfalsifiable"
    rivals["reset_ignored_at_T7_only"] = targeted
    _a25_check("T7 without its own reset edge releases nothing",
               _a25i_sinks(_a25i_epochs(_a25i_run(targeted_rows, knobs)[1],
                                        spans["T7_reset_clears_full_store"])), {})
    _a25_check("frozen model replay reproduces its own vectors",
               _a25i_run([dict(row) for row in stimulus], knobs)[0], expected)

    return {
        "input_bits": _A25I_TOKEN_BITS,
        "output_bits": _A25I_RESULT_BITS,
        "four_state": four_state,
        "masked_unknown_rows": len(unknown_rows),
        "rows": rows,
        "sink_tables": sink_tables,
        "sink_order": {
            name: [entry["sink_sequence"] for entry in entries if entry["sink"]]
            for name, entries in sections.items()
        },
        "fault_epochs": {
            name: [entry["local"] for entry in entries if entry["fault"]]
            for name, entries in sections.items()
        },
        "ready_low_epochs": {
            name: [entry["local"] for entry in entries if not entry["ready"]]
            for name, entries in sections.items()
        },
        "accepted": {
            name: sum(1 for entry in entries if entry["take_rule"])
            for name, entries in sections.items()
        },
        "peak_in_flight": {name: _a25i_peaks(entries)
                           for name, entries in sections.items()},
        "section_final": {
            name: (entries[-1]["next_key"], entries[-1]["occupancy"])
            for name, entries in sections.items()
        },
        "rival_differing_rows": rivals,
    }


# ---------------------------------------------------------------------------
# A06 credit window (``pyc_credit_pipeline``) oracle mode
#
# Derivation discipline
# ---------------------
# Every expected value below comes from exactly one of:
#
# * the frozen A06-O oracle (``docs/gates/logs/${RUN_ID}/A06/oracle.md``):
#   ``IF-01``/``IF-03``/``IF-04`` (24-bit ``sequence``/``cycles``/``value``
#   payload on two depth-4 latency-1 queues), ``ST-01``..``ST-09`` (at most one
#   admission and one retirement per epoch, lowest free index, admission does
#   not count down, a slot is released only on the edge where the push into
#   ``completed`` commits, ``free = !valid`` read from the old state),
#   ``OUT-01``..``OUT-06`` (payload passthrough, one token per epoch, completion
#   order, lowest done index), ``BP-01``/``BP-04`` (a blocked output keeps the
#   token *and* its credit occupied, then drains one token per epoch) and the
#   absolute epoch tables of ``OUT-07``..``OUT-11``;
# * the ratified design ``docs/work-items/p-credit-queue-credit-design.md``
#   (sect. 3.5 update equations, sect. 4.4 Q-D1(a), sect. 6.5 ``C-2``);
# * the harness sampling convention, pinned here by construction (and shared
#   with the newer ``route_merge_independent`` mode): ``sample(row i)`` is the
#   committed state *before* row ``i``'s rising edge, so A06-O epoch ``t`` is
#   the ``clk=0`` row of the ``t``-th pair and the edge that commits epoch ``t``
#   is the ``clk=1`` row of the same pair.
#
# Only ``local_occupancy`` is delivered (frozen contract).  ``downstream_pop``,
# same-edge slot reuse, highest-index tie-breaks and "accept cost = 0" exist
# here only as rival host models used as negative controls, and they are
# evaluated on the already-fixed vector rows before any DUT runs.  The
# "credits as a depth-2 FIFO" degeneration is excluded structurally rather than
# behaviourally (V1/V2/V6 of the design's sect. 7.1: two fifos of DEPTH 4 plus a
# 58-bit state register cannot be a depth-2 FIFO).
# ---------------------------------------------------------------------------

_CR_SLOTS = 2
_CR_DEPTH = 4
_CR_LATENCY = 1
_CR_SEQUENCE_BITS = 4
_CR_CYCLES_BITS = 4
_CR_VALUE_BITS = 16
_CR_TOKEN_BITS = _CR_SEQUENCE_BITS + _CR_CYCLES_BITS + _CR_VALUE_BITS
_CR_RESULT_BITS = 2 + _CR_TOKEN_BITS
_CR_TOKEN_MASK = (1 << _CR_TOKEN_BITS) - 1


def _credit_token(sequence, cycles, value):
    """Pack a ``CreditToken`` the way its declaration order implies (MSB first)."""
    return (
        ((sequence & 0xF) << (_CR_CYCLES_BITS + _CR_VALUE_BITS))
        | ((cycles & 0xF) << _CR_VALUE_BITS)
        | (value & 0xFFFF)
    )


class _CreditFifo:
    """Host FIFO for one bounded queue of the A06 root.

    Mathematical deque plus absolute edge numbers, sharing no ring counters or
    wrapped deadlines with the DUT.  ``ready`` is the capacity contract sampled
    *before* the edge (``count < depth`` for ``local_occupancy``), so a slot
    released on edge E is never refilled on E.  A token pushed on edge ``k``
    carries availability deadline ``k + latency - 1``.
    """

    def __init__(self, depth, latency, policy="local_occupancy"):
        self.depth = depth
        self.latency = latency
        self.policy = policy
        self.tokens = deque()

    def count(self):
        return len(self.tokens)

    def ready(self, out_valid):
        capacity = len(self.tokens) < self.depth
        if self.policy == "local_occupancy":
            return capacity
        # Rival-only reading of `downstream_pop`; the frozen contract never uses
        # it and its self-resolving form `capacity | do_pop` is not modelled.
        return capacity or out_valid

    def eligible(self, edge):
        return bool(self.tokens) and self.tokens[0][2] <= edge

    def tag(self, edge):
        return self.tokens[0][0] if self.eligible(edge) else None

    def value(self, edge):
        return self.tokens[0][1] if self.eligible(edge) else 0

    def transfer(self, edge, push, tag, data, pop):
        if pop:
            self.tokens.popleft()
        if push:
            self.tokens.append((tag, data, edge + self.latency - 1))
        assert len(self.tokens) <= self.depth, (len(self.tokens), self.depth)


def _credit_program():
    """The five frozen A06-O stimuli, each restarted by a reset edge.

    ``S1``/``S2`` are OUT-07/OUT-08 (six tokens, ``take`` always high / low on
    epochs 9-14); ``S4`` is OUT-09 (six tokens of cost 1, ``take`` low on epochs
    5-20); ``S5`` is OUT-10 (costs 6, 0, 1); ``S6`` is OUT-11 (twelve tokens of
    cost 8, the second one ``sequence = 15`` / ``value = 0xFFFF``).  The
    ``admit``/``release``/``sink`` entries are the A06-O golden tables, keyed by
    epoch and by push index respectively.
    """
    s1 = [(0, 8, 0x0111), (1, 1, 0x0222), (2, 2, 0x0333),
          (3, 1, 0x0444), (4, 3, 0x0555), (5, 1, 0x0666)]
    s4 = [(i, 1, 0x0100 + i) for i in range(6)]
    s5 = [(i, cycles, 0x0100 + i) for i, cycles in enumerate((6, 0, 1))]
    s6 = [(15 if i == 1 else i, 8, 0xFFFF if i == 1 else 0x0100 + i)
          for i in range(12)]
    return [
        {"name": "S1", "tokens": s1, "low": (), "epochs": 20,
         "admit": {2: (0, 0), 3: (1, 1), 6: (1, 2), 10: (1, 3), 12: (0, 4),
                   13: (1, 5)},
         "release": {0: 11, 1: 5, 2: 9, 3: 12, 4: 16, 5: 15},
         "sink": {6: 1, 10: 2, 12: 0, 13: 3, 16: 5, 17: 4}},
        # S2 backs the consumer off only; A06-O-OUT-08 states that the admission
        # and completion order of the two credits is unchanged (BP-05), and the
        # output queue never fills enough for `space` to block a retirement, so
        # the admission/release tables are S1's.
        {"name": "S2", "tokens": s1, "low": tuple(range(9, 15)), "epochs": 22,
         "admit": {2: (0, 0), 3: (1, 1), 6: (1, 2), 10: (1, 3), 12: (0, 4),
                   13: (1, 5)},
         "release": {0: 11, 1: 5, 2: 9, 3: 12, 4: 16, 5: 15},
         "sink": {6: 1, 15: 2, 16: 0, 17: 3, 18: 5, 19: 4}},
        {"name": "S4", "tokens": s4, "low": tuple(range(5, 21)), "epochs": 32,
         "admit": {2: (0, 0), 3: (1, 1), 5: (0, 2), 6: (1, 3), 8: (0, 4),
                   9: (1, 5)},
         "release": {0: 4, 1: 5, 2: 7, 3: 8, 4: 22, 5: 23},
         "sink": {21: 0, 22: 1, 23: 2, 24: 3, 25: 4, 26: 5}},
        {"name": "S5", "tokens": s5, "low": (), "epochs": 16,
         "admit": {2: (0, 0)},
         "release": {0: 9},
         "sink": {10: 0}},
        {"name": "S6", "tokens": s6, "low": (), "epochs": 72,
         "admit": {2: (0, 0), 3: (1, 1), 12: (0, 2), 13: (1, 3), 22: (0, 4),
                   23: (1, 5), 32: (0, 6), 33: (1, 7), 42: (0, 8), 43: (1, 9),
                   52: (0, 10), 53: (1, 11)},
         "release": {0: 11, 1: 12, 2: 21, 3: 22, 4: 31, 5: 32, 6: 41, 7: 42,
                     8: 51, 9: 52, 10: 61, 11: 62},
         "sink": {12: 0, 13: 1, 22: 2, 23: 3, 32: 4, 33: 5, 42: 6, 43: 7,
                  52: 8, 53: 9, 62: 10, 63: 11},
         "first_ready_low": 7},
    ]


def _credit_read(issue, done, credit, edge, take, knobs):
    """One epoch's combinational truth, all of it read from the pre-edge state."""
    slots = knobs["slots"]
    out_valid = issue.eligible(edge)
    # Q-D1(a): the module output is QUEUE CAPACITY, never the credit predicate.
    ready = issue.ready(out_valid)
    head = issue.value(edge)
    head_tag = issue.tag(edge)
    cost = (head >> _CR_VALUE_BITS) & 0xF
    free = [slot is None for slot in credit]
    at_zero = [slot is not None and slot[2] == 0 for slot in credit]
    index_order = (range(slots - 1, -1, -1) if knobs["high_index"]
                   else range(slots))
    space = done.ready(done.eligible(edge))
    retire_at = [False] * slots
    if space:
        target = next((index for index in index_order if at_zero[index]), None)
        if target is not None:
            retire_at[target] = True
    if knobs["reuse_released"]:  # rival: same-edge reuse of a released slot
        free = [free[index] or retire_at[index] for index in range(slots)]
    # Q-D1(b) (demoted, canonical PYG `inputReady`) is `any_free and safe`.
    safe = knobs["accept_zero_cost"] or not (out_valid and cost == 0)
    take_issue = any(free) and safe
    admit = out_valid and take_issue
    admit_at = [False] * slots
    if admit:
        target = next(index for index in index_order if free[index])
        admit_at[target] = True
    done_valid = any(at_zero)
    done_data = next((credit[index][1] for index in range(slots)
                      if at_zero[index]), 0)
    done_tag = next((credit[index][0] for index in range(slots)
                     if at_zero[index]), None)
    available = done.eligible(edge)
    head_out = done.value(edge)
    return {
        "ready": ready, "available": available, "head_out": head_out,
        "out_valid": out_valid, "head": head, "head_tag": head_tag, "cost": cost,
        "at_zero": at_zero, "take_issue": take_issue, "admit_at": admit_at,
        "space": space, "retire_at": retire_at, "done_valid": done_valid,
        "done_data": done_data, "done_tag": done_tag, "sink_tag": done.tag(edge),
        "active": [credit[index] is not None and not at_zero[index]
                   for index in range(slots)],
        "expected": _pack(((1, int(ready)), (1, int(available)),
                           (_CR_TOKEN_BITS, head_out))),
    }


def _credit_commit(issue, done, credit, edge, valid, data, tag, take, read,
                   knobs, witness, epoch):
    """Commit one rising edge from the combinational result ``read``."""
    issue.transfer(edge + 1, bool(valid) and read["ready"], tag, data,
                   read["out_valid"] and read["take_issue"])
    done.transfer(edge + 1, read["done_valid"] and read["space"],
                  read["done_tag"], read["done_data"],
                  read["available"] and bool(take))
    for index in range(knobs["slots"]):
        if read["admit_at"][index]:
            credit[index] = (read["head_tag"], read["head"], read["cost"])
            if witness is not None:
                witness["admit"][epoch] = (index, read["head_tag"])
        elif read["retire_at"][index]:
            if witness is not None:
                witness["released"][credit[index][0]] = epoch
            credit[index] = None
        elif read["active"][index]:
            credit[index] = (credit[index][0], credit[index][1],
                             credit[index][2] - 1)
    if witness is not None:
        if not read["ready"]:
            witness["ready_low"].append(epoch)
        if valid and not read["ready"]:
            witness["stall"].append(epoch)
        if read["available"] and take:
            witness["sink"][epoch] = read["sink_tag"]
        witness["frozen"].append(
            (epoch, tuple(credit), done.count(), read["head_out"]))


def _credit_knobs(**overrides):
    base = {"slots": _CR_SLOTS, "depth": _CR_DEPTH,
            "policy": "local_occupancy", "reuse_released": False,
            "high_index": False, "accept_zero_cost": False}
    return dict(base, **overrides)


def _credit_run(knobs):
    """Generate the vector rows and the epoch witnesses for the frozen program."""
    issue = _CreditFifo(knobs["depth"], _CR_LATENCY, knobs["policy"])
    done = _CreditFifo(knobs["depth"], _CR_LATENCY)
    credit = [None] * knobs["slots"]
    rows = []
    witness = {}
    for section in _credit_program():
        name = section["name"]
        witness[name] = {"admit": {}, "sink": {}, "released": {}, "ready_low": [],
                         "stall": [], "frozen": [], "accepted": 0}
        # Synchronous reset: the sampled result is still the pre-reset state.
        read = _credit_read(issue, done, credit, 0, 0, knobs)
        rows.append({"clk": 0, "rst": 1, "valid": 0, "take": 0, "data": 0,
                     "expected": read["expected"]})
        rows.append({"clk": 1, "rst": 1, "valid": 0, "take": 0, "data": 0,
                     "expected": read["expected"]})
        issue.tokens.clear()
        done.tokens.clear()
        credit = [None] * knobs["slots"]
        cursor = 0
        for epoch in range(1, section["epochs"] + 1):
            take = 0 if epoch in section["low"] else 1
            out_valid = issue.eligible(epoch - 1)
            ready = issue.ready(out_valid)
            held = cursor < len(section["tokens"])
            # The testbench of A06-O-OUT-11 stops pushing while `issued` is
            # full, so no driven token is ever silently rejected (T-05).
            valid = 1 if (held and ready) else 0
            data = _credit_token(*section["tokens"][cursor]) if held else 0
            read = _credit_read(issue, done, credit, epoch - 1, take, knobs)
            for clk in (0, 1):
                rows.append({"clk": clk, "rst": 0, "valid": valid, "take": take,
                             "data": data, "expected": read["expected"]})
            _credit_commit(issue, done, credit, epoch - 1, valid, data,
                           cursor if held else None, take, read, knobs,
                           witness[name], epoch)
            if valid:
                witness[name]["accepted"] += 1
                cursor += 1
        witness[name]["final"] = (tuple(credit), done.count(), issue.count())
    return rows, witness


def _credit_replay(rows, knobs):
    """Recompute ``expected`` for already-fixed input rows (rival controls).

    The stimulus is held constant, so a rival can only differ in behaviour and
    never in what it was driven with.
    """
    issue = _CreditFifo(knobs["depth"], _CR_LATENCY, knobs["policy"])
    done = _CreditFifo(knobs["depth"], _CR_LATENCY)
    credit = [None] * knobs["slots"]
    expected = []
    edge = 0
    previous = 0
    for row in rows:
        rising = bool(row["clk"]) and not previous
        read = _credit_read(issue, done, credit, edge, row["take"], knobs)
        expected.append(read["expected"])
        if rising and row["rst"]:
            issue.tokens.clear()
            done.tokens.clear()
            credit = [None] * knobs["slots"]
            edge = 0
        elif rising:
            _credit_commit(issue, done, credit, edge, row["valid"], row["data"],
                           None, row["take"], read, knobs, None, None)
            edge += 1
        previous = row["clk"]
    return expected


def _credit_check(label, actual, expected):
    assert actual == expected, f"{label}: actual={actual!r} expected={expected!r}"


def _credit():
    """Frozen credit contract -> native/RTL vectors (two rows per A06-O epoch)."""
    knobs = _credit_knobs()
    rows, witness = _credit_run(knobs)
    program = _credit_program()
    for section in program:
        name = section["name"]
        seen = witness[name]
        # (i) the frozen absolute epoch tables of A06-O OUT-07..OUT-11.
        if "admit" in section:
            _credit_check(f"{name} admission table (epoch -> slot, token)",
                          seen["admit"], section["admit"])
        _credit_check(f"{name} release table (token -> epoch)",
                      seen["released"], section["release"])
        _credit_check(f"{name} sink table (epoch -> token)",
                      seen["sink"], section["sink"])
        # (ii) the stimulus is a handshake: nothing is driven into a full queue,
        #      so every driven token is accepted and none is silently dropped.
        _credit_check(f"{name} silent rejections", seen["stall"], [])
        _credit_check(f"{name} accepted pushes", seen["accepted"],
                      len(section["tokens"]))
        delivered = len(section["sink"])
        _credit_check(f"{name} tokens delivered",
                      delivered, 1 if name == "S5" else len(section["tokens"]))
        # (iii) the credit window really is returned: S1/S2/S4/S6 end drained.
        if name != "S5":
            _credit_check(f"{name} final slots / completed / issued",
                          seen["final"], (tuple([None] * _CR_SLOTS), 0, 0))

    # (iv) S1: completion order is not input order, the deep token is overtaken
    #      by later cheaper ones, and one edge both retires and admits.
    _credit_check("S1 completion sequence",
                  [program[0]["tokens"][index][0]
                   for _, index in sorted(witness["S1"]["sink"].items())],
                  [1, 2, 0, 3, 5, 4])
    _credit_check("S1 same-edge admit + retire on different slots",
                  (12 in witness["S1"]["admit"],
                   witness["S1"]["released"][3], witness["S1"]["released"][4]),
                  (True, 12, 16))
    _credit_check("S1 deep-token cost", program[0]["tokens"][0][1], 8)

    # (v) S6: the input queue saturates (BD-08) and the module `ready` -- queue
    #     capacity, Q-D1(a) -- first drops on epoch 7; admission then runs at
    #     the frozen 2 tokens / 10 epochs, far below a FIFO's 1 token / epoch.
    _credit_check("S6 first ready-low epoch", witness["S6"]["ready_low"][0],
                  program[-1]["first_ready_low"])
    admits = sorted(witness["S6"]["admit"])
    _credit_check("S6 admitted pair period",
                  [admits[i + 2] - admits[i] for i in range(0, 10, 2)],
                  [10, 10, 10, 10, 10])
    _credit_check("S6 width boundary token preserved",
                  (program[-1]["tokens"][1][0], program[-1]["tokens"][1][2]),
                  (15, 0xFFFF))

    # (vi) S4: a completed token keeps its credit while the output is blocked
    #      (BP-01/BD-07): both slots stay (occupied, remaining 0) and the output
    #      queue stays full through the whole window.
    frozen = {entry[0]: entry[1:] for entry in witness["S4"]["frozen"]}
    for epoch in range(11, 21):
        slots, count, _ = frozen[epoch]
        _credit_check(f"S4 epoch {epoch} frozen remaining",
                      [None if slot is None else slot[2] for slot in slots],
                      [0, 0])
        _credit_check(f"S4 epoch {epoch} completed occupancy", count, _CR_DEPTH)
    _credit_check("S4 drain epochs", sorted(witness["S4"]["sink"]),
                  [21, 22, 23, 24, 25, 26])

    # (vii) S5/C-2: the cost-0 head is never admitted and never reported, the
    #       in-flight token still drains, and `ready` stays high throughout the
    #       stall because it is queue capacity and not a credit predicate.
    _credit_check("S5 admitted tokens", sorted(witness["S5"]["admit"]), [2])
    _credit_check("S5 sink", sorted(witness["S5"]["sink"]), [10])
    _credit_check("S5 ready low during the stall",
                  [epoch for epoch in range(5, 17)
                   if epoch in witness["S5"]["ready_low"]], [])
    _credit_check("S5 blocked tokens still queued at the head",
                  witness["S5"]["final"][2], 2)
    _credit_check("S5 zero-cost token never leaves the input queue",
                  witness["S5"]["final"][0], (None, None))

    # (viii) negative controls: the vector set must be able to tell the frozen
    #        contract from each rival it exists to exclude, evaluated on the
    #        frozen stimulus before any DUT runs.
    rivals = {}
    for label, overrides in (
        ("downstream_pop", {"policy": "downstream_pop"}),
        ("same_edge_reuse", {"reuse_released": True}),
        ("highest_index", {"high_index": True}),
        ("accept_cost_zero", {"accept_zero_cost": True}),
    ):
        rival = _credit_replay(rows, _credit_knobs(**overrides))
        differing = sum(1 for ours, theirs in zip(rows, rival, strict=True)
                        if ours["expected"] != theirs)
        assert differing, f"vector set cannot distinguish {label}"
        rivals[label] = differing
    _credit_check("frozen model replay reproduces its own vectors",
                  _credit_replay(rows, knobs),
                  [row["expected"] for row in rows])

    return {
        "input_bits": _CR_TOKEN_BITS,
        "output_bits": _CR_RESULT_BITS,
        "rows": rows,
        "sink_epochs": {name: sorted(witness[name]["sink"])
                        for name in witness},
        "ready_low_epochs": {name: witness[name]["ready_low"]
                             for name in witness},
        "rival_differing_rows": rivals,
        "frozen_window": [10, 20],
    }

def oracle(name):
    delayed = {
        "latency_two": (13, 2, 2),
        "latency_local": (13, 4, 3),
        "latency_bypass": (13, 4, 3, True),
        "latency_nested": (19, 4, 3),
        "latency_nested_raw": (19, 4, 3, False, True),
        "latency_huge": (13, 2, (1 << 64) - 1),
        "latency_dead": (13, 1, 3, False, False, True),
    }
    if name in delayed:
        return _delayed(*delayed[name])
    arguments = {
        "scalar": ("pair", 13, (2, 2), (False, True)),
        "nested": ("nested", 19, (3, 3), (False, True)),
        "forward_local": ("forward_local", 13, (2, 3), (False, False)),
        "forward_mixed": ("forward_mixed", 13, (2, 3), (False, True)),
        "snapshots": ("snapshots", 13, (2, 2), (False, False)),
    }
    if name == "route_merge":
        return _route_merge(
            payload_bits=64,
            depths=(2, 2, 2, 1, 1, 2),
            adds=(10, 20),
        )
    if name == "route_merge_independent":
        return _route_merge_independent()
    if name == "credit":
        return _credit()
    if name == "reorder":
        return _a25()
    # A25-T: same root, second oracle mode derived independently of `reorder`
    # (map-addressed store, explicit per-epoch stimulus, hand-computed absolute
    # epoch tables, masked x/z rows, thirteen rival controls).
    if name == "reorder_independent":
        return _a25i_independent()
    # A25-T: the four-state variant.  `_raw` keeps Verilator out of the x/z run
    # (the harness convention, and the only honest one: Verilator two-state-folds
    # x and miscompiles the generated rows file when it contains a `z` literal).
    if name == "reorder_independent_raw":
        return _a25i_independent(four_state=True)
    if name == "wide65":
        return _single(65, 2, ((1 << 64) | 3, (1 << 63) | 5, 7))
    if name == "wide130":
        return _single(
            130, 5, ((1 << 129) | (1 << 64) | 3, (1 << 128) | (1 << 65) | 5, 7)
        )
    if name == "table":
        first = _pack(((13, 1), (13, 2), (13, 8191)))
        second = _pack(((13, 4096), (13, 7), (13, 31)))
        return _single(39, 3, (first, second, 0))
    result = _oracle(*arguments[name])
    if name == "scalar":
        assert result["rows"][0]["expected"] == 536887296
        assert result["rows"][9]["expected"] == 536870911
    if name == "snapshots":
        packed = result["rows"][3]["expected"]
        assert (packed >> 28) & 8191 == 8191
        assert (packed >> 13) & 8191 == 0
        assert packed & 8191 == 2
    return result


