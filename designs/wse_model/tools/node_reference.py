"""Independent core trace for installation, epochs, routing and receiving."""

from wse_model.calendar.entry import CalendarRouteEntry, CalendarRouteRegs
from wse_model.calendar.epoch import EpochTracker
from wse_model.calendar.receive import DeliveryOutcome, ReceiveAccount, Segment, SegmentId
from wse_model.calendar.validate import validate_route_bits
from wse_model.errors import WseModelError
from wse_model.noc.calreg import CalRegBank, CalRegSlot, StopTheWorldWindow
from wse_model.noc.forward import simulate_multicast
from wse_model.topology import CALENDAR_BASELINE


def expectations() -> list[list[int]]:
    registers = CalendarRouteRegs(0xC0008, 0x0000000801010000)
    entry = CalendarRouteEntry.from_regs(registers, 40)
    validation = validate_route_bits(
        CALENDAR_BASELINE.topology,
        entry.route_bits,
        source=1,
        expected_land_nodes=frozenset({9}),
        self_delivery=False,
    )
    assert not validation.errors, validation.errors
    hops = {
        hop.node: hop
        for hop in simulate_multicast(CALENDAR_BASELINE.topology, entry.route_bits, source=1).hops
    }
    epochs, bank, window = EpochTracker(width_bits=8), CalRegBank(9), StopTheWorldWindow(9)
    account = None
    active = False
    history = []
    for cycle in range(13):
        post, retire = cycle in (2, 7), cycle in (6, 11)
        send = cycle in (0, 3, 4, 5, 8, 9, 10)
        if post:
            epoch = epochs.next_epoch(1, drained=False)
        else:
            epoch = epochs.counter(1).current
        admitted, config_fault = False, False
        try:
            if cycle == 1:
                window.enter()
                window.drain()
                window.require_drained(action="install")
                bank.install(CalRegSlot(opcode=1, content=b"node test"))
                admitted = True
            elif send or cycle == 12:
                bank.lookup(1)
                admitted = True
        except WseModelError:
            config_fault = True
        reached = send and admitted and 9 in hops
        sequence = 1 if cycle in (5, 10) else 0
        address = 104 if sequence else 100
        accepted, status = False, 0
        if post:
            account = ReceiveAccount(
                9, 1, epoch, 100, 16, entry.expected_rx_bytes, entry.expected_rx_bytes
            )
            active, accepted, status = True, True, 1
        elif retire:
            assert account is not None and account.complete
            active, accepted, status = False, True, 7
        elif reached:
            assert active and account is not None
            outcome = account.deliver(Segment(SegmentId(1, sequence), 1, epoch, address, 4))
            status = {DeliveryOutcome.COUNTED: 2, DeliveryOutcome.DUPLICATE: 3}[outcome]
            accepted = True
        history.append(
            [
                cycle,
                epoch,
                0,
                sum(1 << code for code in bank.installed),
                int(admitted),
                int(config_fault),
                int(entry.route_bits.pair(9)),
                entry.land_count,
                int(entry.flags),
                entry.expected_rx_bytes,
                0,
                int(send),
                int(reached),
                sum(1 << peer for peer in hops[1].egress),
                sum(1 << peer for peer in hops[9].egress),
                int(hops[9].lands),
                0,
                0,
                int(accepted),
                status,
                0,
                int(active),
                account.counted_bytes if account else 0,
                int(active and account is not None and account.complete),
                0,
                epoch,
                1,
                sequence,
                address,
                4,
            ]
        )
    return history
