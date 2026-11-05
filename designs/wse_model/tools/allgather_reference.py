"""Core-owned expectations for the serialized, 32-member FFN fixture.

Only the test schedule lives here. Route validation, multicast traversal and
receive accounting use the ordinary Python model, outside the compiled DUT.
"""

from wse_model.calendar.receive import DeliveryOutcome, ReceiveAccount, Segment, SegmentId
from wse_model.calendar.validate import validate_route_bits
from wse_model.fixtures import ffn_example
from wse_model.noc.forward import simulate_multicast


def expectations() -> list[list[int]]:
    example = ffn_example()
    nodes = sorted(example.members)
    history = []
    for key_id in (0, 1):
        epoch = key_id + 1
        geometry = example.geometry(key_id)
        groups = example.groups(key_id)
        arena = example.arena(key_id)
        accounts = {
            node: ReceiveAccount(
                node,
                1,
                epoch,
                arena.recv_sym_base,
                geometry.capacity,
                example.table.entry(key_id, node).expected_rx_bytes,
                example.table.entry(key_id, node).expected_rx_bytes,
            )
            for node in nodes
        }
        for offset in range(258):
            sending = 1 <= offset <= 256
            source = (offset - 1) // 8 if sending else 0
            row = (offset - 1) % 8 if sending else 0
            sequence = key_id * 8 + row
            hops = {}
            if sending:
                route = example.table.entry(key_id, source).route_bits
                validation = validate_route_bits(
                    example.table.topology,
                    route,
                    source=source,
                    expected_land_nodes=frozenset(groups[source]) - {source},
                    self_delivery=False,
                )
                assert not validation.errors, validation.errors
                trace = simulate_multicast(example.table.topology, route, source=source)
                assert trace.is_clean_tree_walk
                hops = {hop.node: hop for hop in trace.hops}
                segment = Segment(
                    SegmentId(source, sequence),
                    1,
                    epoch,
                    arena.recv_sym_base
                    + row * geometry.recv_row_stride
                    + groups[source].index(source) * geometry.row_bytes,
                    geometry.row_bytes,
                )
            for node in nodes:
                account = accounts[node]
                hop = hops.get(node)
                status, accepted, active = 0, False, True
                if offset == 0:
                    status, accepted = 1, True
                elif offset == 257:
                    assert account.complete
                    assert len(account.seen) == (56 if key_id == 0 else 24)
                    status, accepted, active = 7, True, False
                elif hop is not None and hop.lands:
                    assert account.deliver(segment) is DeliveryOutcome.COUNTED
                    status, accepted = 2, True
                history.append(
                    [
                        key_id * 258 + offset,
                        key_id,
                        node,
                        source,
                        row,
                        sequence,
                        epoch,
                        int(hop is not None),
                        sum(1 << peer for peer in hop.egress) if hop else 0,
                        int(hop.lands) if hop else 0,
                        0,
                        int(accepted),
                        status,
                        0,
                        int(active),
                        account.counted_bytes,
                        int(active and account.complete),
                        0,
                        segment.address if sending else arena.recv_sym_base,
                        geometry.row_bytes,
                    ]
                )
    return history
