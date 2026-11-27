"""Independent core expectations for the connected five-node functional test."""

from wse_model.calendar.receive import DeliveryOutcome, ReceiveAccount, Segment, SegmentId
from wse_model.calendar.route_bits import RouteBits
from wse_model.calendar.validate import validate_route_bits
from wse_model.noc.forward import simulate_multicast
from wse_model.topology import CALENDAR_BASELINE


def expectations() -> list[list[int]]:
    nodes = (1, 9, 8, 10, 17)
    history = []
    for epoch, route_value in ((1, 0xC003F0008), (2, 0xC003B0008)):
        route = RouteBits.from_int(route_value, 40)
        lands = frozenset((9, 8, 10, 17) if epoch == 1 else (8, 10, 17))
        validation = validate_route_bits(
            CALENDAR_BASELINE.topology,
            route,
            source=1,
            expected_land_nodes=lands,
            self_delivery=False,
        )
        assert not validation.errors, validation.errors
        trace = simulate_multicast(CALENDAR_BASELINE.topology, route, source=1)
        assert trace.is_clean_tree_walk
        assert frozenset(trace.land_nodes) == lands
        hops = {hop.node: hop for hop in trace.hops}
        accounts = {
            node: ReceiveAccount(
                node, 1, epoch, 100, 16, 8 if node in lands else 0, 8 if node in lands else 0
            )
            for node in nodes
        }
        for offset in range(6):
            send = offset in (1, 2, 3)
            for node in nodes:
                account, hop = accounts[node], hops[node]
                status, accepted, active = 0, False, True
                if offset == 0:
                    status, accepted = 1, True
                elif offset == 5:
                    assert account.complete
                    status, accepted, active = 7, True, False
                elif send and hop.lands:
                    segment = Segment(
                        SegmentId(1, 1 if offset == 3 else 0),
                        1,
                        epoch,
                        104 if offset == 3 else 100,
                        4,
                    )
                    outcome = account.deliver(segment)
                    status = {DeliveryOutcome.COUNTED: 2, DeliveryOutcome.DUPLICATE: 3}[outcome]
                    accepted = True
                history.append(
                    [
                        (epoch - 1) * 6 + offset,
                        node,
                        int(send),
                        sum(1 << peer for peer in hop.egress),
                        int(hop.lands),
                        0,
                        int(accepted),
                        status,
                        0,
                        int(active),
                        account.counted_bytes,
                        int(active and account.complete),
                        0,
                    ]
                )
    return history
