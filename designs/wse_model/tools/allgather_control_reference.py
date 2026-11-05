"""Independent control-loop expectations using the ordinary WSE semantic core.

The only injected delays are test adapter inputs: node 31 cannot post before
cycle 5; node 30 cannot accept the last row-phase segment before cycle 264.
No compiled source helper or compiled observation supplies expected results.
"""

from wse_model.calendar.epoch import EpochTracker
from wse_model.calendar.receive import DeliveryOutcome, ReceiveAccount, Segment, SegmentId
from wse_model.calendar.validate import validate_route_bits
from wse_model.fixtures import ffn_example
from wse_model.noc.calreg import CalRegBank, CalRegSlot
from wse_model.noc.forward import simulate_multicast


def expectations(*, delayed: bool = False) -> tuple[list[list[int]], list[list[int]]]:
    example = ffn_example()
    nodes = sorted(example.members)
    banks = {node: CalRegBank(node) for node in nodes}
    epochs = {node: EpochTracker(width_bits=8) for node in nodes}
    accounts = dict.fromkeys(nodes)
    active = dict.fromkeys(nodes, False)
    history, control = [], []
    stage, key, cursor, epoch = 0, 0, 0, 0
    done_samples = 0
    for cycle in range(1024):
        geometry, arena = example.geometry(key), example.arena(key)
        groups = example.groups(key)
        source, row = divmod(cursor, geometry.row_count) if stage == 3 else (0, 0)
        seq = key * geometry.row_count + row
        address = arena.recv_sym_base
        if stage == 3:
            address += (
                row * geometry.recv_row_stride + groups[source].index(source) * geometry.row_bytes
            )
        hops = {}
        if stage == 3:
            route = example.table.entry(key, source).route_bits
            valid = validate_route_bits(
                example.table.topology,
                route,
                source=source,
                expected_land_nodes=frozenset(groups[source]) - {source},
                self_delivery=False,
            )
            assert not valid.errors
            trace = simulate_multicast(example.table.topology, route, source=source)
            assert trace.is_clean_tree_walk
            hops = {hop.node: hop for hop in trace.hops}
        sent_by_node = []
        for node in nodes:
            if stage == 0:
                assert not active[node]
                banks[node].install(CalRegSlot(opcode=1, content=b"opaque-test-image"))
            banks[node].lookup(1)
            if stage == 1:
                assert not active[node]
                epochs[node].next_epoch(1, drained=True)
            local_epoch = epochs[node].counter(1).current
            entry = example.table.entry(key, node)
            account = accounts[node]
            status, accepted = 0, False
            hop = hops.get(node)
            segment_epoch = epoch
            if stage == 2 and not active[node] and (not delayed or node != 31 or cycle >= 5):
                segment_epoch = local_epoch
                account = ReceiveAccount(
                    node,
                    1,
                    local_epoch,
                    arena.recv_sym_base,
                    geometry.capacity,
                    geometry.exp_val(entry.land_count),
                    entry.expected_rx_bytes,
                )
                accounts[node] = account
                active[node] = True
                status, accepted = 1, True
            elif stage == 3 and hop is not None and hop.lands:
                permitted = not (delayed and node == 30 and 261 <= cycle < 264)
                if permitted:
                    assert active[node]
                    result = account.deliver(
                        Segment(SegmentId(source, seq), 1, epoch, address, geometry.row_bytes)
                    )
                    assert result in (DeliveryOutcome.COUNTED, DeliveryOutcome.DUPLICATE)
                    status = 2 if result is DeliveryOutcome.COUNTED else 3
                    accepted = True
            elif stage == 5:
                assert active[node] and account.complete
                active[node] = False
                status, accepted = 7, True
            target = stage == 3 and node in groups[source] and node != source
            sent = not target or (hop is not None and hop.lands and accepted and status in (2, 3))
            sent_by_node.append(sent)
            history.append(
                [
                    cycle,
                    key,
                    node,
                    source,
                    row,
                    seq,
                    epoch,
                    int(hop is not None),
                    sum(1 << peer for peer in hop.egress) if hop else 0,
                    int(hop.lands) if hop else 0,
                    0,
                    int(accepted),
                    status,
                    0,
                    int(active[node]),
                    account.counted_bytes if account else 0,
                    int(active[node] and account.complete),
                    0,
                    address,
                    geometry.row_bytes,
                    stage,
                    local_epoch,
                    2,
                    1,
                    0,
                    0,
                    entry.expected_rx_bytes,
                    0,
                    1,
                    int(active[node]),
                    int(sent),
                    0,
                ]
            )
            # Posts carry the local counter; all sends carry the agreed source epoch.
            assert segment_epoch == local_epoch or stage in (0, 1)
        uniform = len({epochs[node].counter(1).current for node in nodes}) == 1
        posted, sent = all(active.values()), all(sent_by_node)
        complete = all(active[node] and accounts[node].complete for node in nodes)
        retired = not any(active.values())
        control.append(
            [
                cycle,
                stage,
                key,
                cursor,
                epoch,
                1,
                int(posted),
                int(sent),
                int(complete),
                1,
                int(retired),
                0,
                int(uniform),
            ]
        )
        assert uniform
        if stage == 0:
            stage = 1
        elif stage == 1:
            epoch = epochs[nodes[0]].counter(1).current
            stage = 2
        elif stage == 2 and posted:
            stage = 3
        elif stage == 3 and sent:
            if cursor + 1 == len(nodes) * geometry.row_count:
                stage = 4
            else:
                cursor += 1
        elif stage == 4 and complete:
            stage = 5
        elif stage == 5 and retired:
            if key == 0:
                key, cursor, stage = 1, 0, 1
            else:
                stage = 6
        elif stage == 6:
            done_samples += 1
            if done_samples == 2:
                return history, control
    raise AssertionError("core-controlled fixture did not complete")
