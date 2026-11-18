"""End-to-end collective closure: Step0..Step6 driven by the compiled table.

This is the model's integration point. Given a compiled route table and the
payload geometry, it runs one collective phase the way the design describes:

======  ==========================================================================
Step    model action
======  ==========================================================================
Step1   the caller guarantees ``sendTile`` is ready (producer ordering)
Step2   all members share the same ``opcode`` domain and the same epoch
Step3   each member posts ``MTE4_NOC_RECV_WAIT`` with its own ``expVal``
Step4   each member sends from its own row of the table
Step4s  a non-self-delivering fabric's local segment is written by UB-to-UB copy
Step5   drains the sends, then checks that every ``expVal`` was reached
Step6   retires the accounts and reports
======  ==========================================================================

The ordering invariants are enforced, not assumed: Step3 precedes Step4 (O3), the
epoch is uniform across members (E1), and the completion test is byte accounting
rather than a notification.
"""

from __future__ import annotations

from dataclasses import dataclass, field

from wse_model.calendar.collective import Collective
from wse_model.calendar.geometry import PayloadGeometry
from wse_model.calendar.receive import ReceiveAccount
from wse_model.calendar.table import CalendarRouteTable
from wse_model.errors import CalendarError, WseModelError
from wse_model.noc.network import Noc, SendOutcome, SendPlan

__all__ = ["AllGatherReport", "MemberState", "run_allgather"]


@dataclass
class MemberState:
    """One core's role and accounting in a collective phase."""

    node: int
    group: tuple[int, ...]
    rank_in_group: int
    self_off: int
    exp_val: int
    expected_rx_bytes: int
    counted_bytes: int = 0
    complete: bool = False
    land_count: int = 0

    def describe(self) -> dict[str, object]:
        return {
            "node": self.node,
            "group": list(self.group),
            "rank_in_group": self.rank_in_group,
            "self_off": self.self_off,
            "land_count": self.land_count,
            "expVal": self.exp_val,
            "expectedRxBytes": self.expected_rx_bytes,
            "counted_bytes": self.counted_bytes,
            "complete": self.complete,
        }


@dataclass
class AllGatherReport:
    """The outcome of one AllGather phase."""

    phase: str
    key_id: int
    opcode: int
    epoch: int
    members: tuple[int, ...]
    states: tuple[MemberState, ...]
    sends: tuple[SendOutcome, ...] = ()
    arm_lead_cycles: int | None = None
    link_load: dict[tuple[int, int], int] = field(default_factory=dict)

    @property
    def ok(self) -> bool:
        return all(state.complete for state in self.states)

    @property
    def incomplete(self) -> tuple[MemberState, ...]:
        return tuple(state for state in self.states if not state.complete)

    @property
    def total_wire_bytes(self) -> int:
        return sum(outcome.wire_bytes for outcome in self.sends)

    @property
    def total_header_bytes(self) -> int:
        return sum(outcome.header_bytes_moved for outcome in self.sends)

    @property
    def total_payload_bytes(self) -> int:
        return sum(outcome.payload_bytes_delivered for outcome in self.sends)

    @property
    def max_tree_depth(self) -> int:
        return max((outcome.max_depth for outcome in self.sends), default=0)

    @property
    def peak_link_load(self) -> int:
        return max(self.link_load.values(), default=0)

    def check(self) -> None:
        """Raise unless every member's ``expVal`` was reached exactly."""
        if self.ok:
            return
        detail = "; ".join(
            f"node {state.node} expected {state.exp_val} B, counted {state.counted_bytes} B"
            for state in self.incomplete
        )
        raise CalendarError(
            f"phase {self.phase} (keyId {self.key_id}, epoch {self.epoch}) did not "
            f"complete: {detail}"
        )

    def describe(self) -> dict[str, object]:
        return {
            "phase": self.phase,
            "key_id": self.key_id,
            "opcode": self.opcode,
            "epoch": self.epoch,
            "members": list(self.members),
            "member_count": len(self.members),
            "ok": self.ok,
            "arm_lead_cycles": self.arm_lead_cycles,
            "max_tree_depth": self.max_tree_depth,
            "peak_link_load": self.peak_link_load,
            "total_wire_bytes": self.total_wire_bytes,
            "total_header_bytes": self.total_header_bytes,
            "total_payload_bytes": self.total_payload_bytes,
            "useful_payload_fraction": (
                round(self.total_payload_bytes / self.total_wire_bytes, 6)
                if self.total_wire_bytes
                else 0.0
            ),
            "states": [state.describe() for state in self.states],
        }


def run_allgather(
    noc: Noc,
    table: CalendarRouteTable,
    key_id: int,
    *,
    geometry: PayloadGeometry,
    groups: dict[int, tuple[int, ...]],
    epoch: int | None = None,
    recv_sym_base: int = 0x1000,
    self_delivery: bool | None = None,
) -> AllGatherReport:
    """Run one AllGather phase across every member core.

    ``groups`` maps each member core to its collective group, so one phase may
    contain several independent trees that share a single ``opcode`` domain —
    which is exactly the FFN phase B shape (four ROW groups, 32 cores reporting
    on one semaphore).
    """
    definition = table.key(key_id)
    if definition.collective is not Collective.ALL_GATHER:
        raise CalendarError(
            f"keyId {key_id} is {definition.collective.name}; the closure models "
            "AllGather only. Reduce semantics are blocked on open item C-10 "
            "(merge buffering, out-of-order merge timing, overflow behaviour)"
        )
    if self_delivery is None:
        self_delivery = definition.self_delivery
    if self_delivery != definition.self_delivery:
        raise CalendarError(
            f"self_delivery={self_delivery} disagrees with the compiled key "
            f"({definition.self_delivery}); the source's own L bit would be "
            "inconsistent (Calendar §2.7.2)"
        )

    members = tuple(sorted(groups))
    for node in members:
        if node not in definition.member_nodes:
            raise CalendarError(
                f"node {node} participates in phase {definition.route_key.phase_id} "
                "but is not a member of the compiled key"
            )

    # E1: one epoch for the whole opcode domain, driven by each core's own
    # counter rather than supplied by the caller (Calendar §1.5).
    epoch_value = noc.next_epoch(members[0], definition.opcode)
    for node in members[1:]:
        noc.next_epoch(node, definition.opcode)
    noc.epoch_trackers[members[0]].assert_spmd_consistent(
        definition.opcode,
        {node: noc.epoch_trackers[node] for node in members[1:]},
    )
    if epoch is not None and epoch_value != epoch:
        raise CalendarError(
            f"phase {definition.route_key.phase_id}: the core counter yields epoch "
            f"{epoch_value} but epoch {epoch} was requested; a divergent epoch "
            "makes receivers drop packets (invariant E1)"
        )

    land_counts = definition.land_counts()
    states: list[MemberState] = []

    # Step3: post every receive command before any send (invariant O3).
    for node in members:
        group = groups[node]
        rank_in_group = group.index(node)
        land_count = land_counts[node]
        exp_val = geometry.exp_val(land_count)
        entry = definition.entry(node)
        account = ReceiveAccount(
            aicore=node,
            opcode=definition.opcode,
            epoch=epoch_value,
            dst=recv_sym_base,
            capacity=geometry.capacity,
            exp_val=exp_val,
            compiled_exp_val=entry.expected_rx_bytes,
        )
        noc.ingress[node].post(account)
        states.append(
            MemberState(
                node=node,
                group=group,
                rank_in_group=rank_in_group,
                self_off=rank_in_group * geometry.row_bytes,
                exp_val=exp_val,
                expected_rx_bytes=entry.expected_rx_bytes,
                land_count=land_count,
            )
        )

    # Step4: every member sends its own segment along its own row of the table.
    sends: list[SendOutcome] = []
    for state in states:
        entry = definition.entry(state.node)
        plan = SendPlan(
            source=state.node,
            route_bits=entry.route_bits,
            opcode=definition.opcode,
            epoch=epoch_value,
            row_count=geometry.row_count,
            row_bytes=geometry.row_bytes,
            arena_base=recv_sym_base,
            row_stride=geometry.recv_row_stride,
            source_self_off=state.self_off,
            red_op=definition.key_ref(route_version=table.route_version).red_op,
            self_delivery=definition.self_delivery,
            expected_rx_bytes=entry.expected_rx_bytes,
        )
        sends.append(noc.send(plan))

    # Step5: drain the sends, then evaluate completion by byte count.
    aggregate_load: dict[tuple[int, int], int] = {}
    for outcome in sends:
        for link, count in outcome.link_load.items():
            aggregate_load[link] = aggregate_load.get(link, 0) + count
        for window in noc.windows.values():
            window.drain(outcome.flit_hops)

    for state in states:
        outstanding = noc.ingress[state.node].outstanding
        if len(outstanding) != 1:
            raise WseModelError(
                f"node {state.node}: expected exactly one outstanding receive "
                f"command, found {len(outstanding)}"
            )
        account = outstanding[0]
        state.counted_bytes = account.counted_bytes
        state.complete = account.complete

    report = AllGatherReport(
        phase=definition.route_key.phase_id,
        key_id=key_id,
        opcode=definition.opcode,
        epoch=epoch_value,
        members=members,
        states=tuple(states),
        sends=tuple(sends),
        arm_lead_cycles=definition.arm_lead_cycles,
        link_load=aggregate_load,
    )

    # Step6: retire only when complete, so an incomplete phase cannot be papered
    # over by a subsequent round.
    if report.ok:
        for state in states:
            noc.ingress[state.node].retire(noc.ingress[state.node].outstanding[0])
    return report
