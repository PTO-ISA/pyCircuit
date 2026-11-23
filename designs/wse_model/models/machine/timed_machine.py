# ruff: noqa: N802, N803, F823, F841
"""Runtime-programmed WSE discrete-event machine; one NoC tick is 0.5 ns.

Storage microsteps do not advance model_time. Each COPY/SEND is a standalone
packet; per-flit events stream through directed links with Calendar arbitration.
Finite queues and actual packet-buffer memory are model resources.
"""
import pycircuit as ac
from wse_calendar.control_ledger import CalRegLedger, CalRegRequest, EpochLedger, EpochRequest
from wse_calendar.forward import ForwardHop, HopInput
from wse_calendar.ledger import ReceiveLedger, Request
from wse_machine.event_queue import EventQueue
from wse_machine.machine_core import (
    BankedMemory,
    Command,
    HostRequest,
    MachineResult,
    ReadAddress,
    decode,
    first_node,
    segment_of,
    transfer_size,
)


@ac.struct
class TimedResult:
    state: MachineResult
    model_time: ac.u64
    events: ac.u32
    event_kind: ac.u8
    event_source: ac.u6
    event_node: ac.u6
    event_seq: ac.u16
    event_flit: ac.u16


def service_ticks(
    nbytes: ac.u32, first: ac.u64, numerator: ac.u64, denominator: ac.u64, width: ac.u32
) -> ac.u64:
    more_beats: ac.u64 = (nbytes - 1) // width
    return first + (more_beats * numerator + denominator - 1) // denominator


def event_payload(
    kind: ac.u4, source: ac.u6, node: ac.u6, ingress: ac.u6, flit: ac.u16, direction: ac.u3
) -> ac.u64:
    result: ac.u64 = kind
    source_wide: ac.u64 = source
    node_wide: ac.u64 = node
    ingress_wide: ac.u64 = ingress
    flit_wide: ac.u64 = flit
    direction_wide: ac.u64 = direction
    return (
        result
        | (source_wide << 4)
        | (node_wide << 10)
        | (ingress_wide << 16)
        | (flit_wide << 22)
        | (direction_wide << 38)
    )


def row_participants(route_row: ac.u16) -> ac.u8:
    # One grid row has eight two-bit entries. The high bit of each entry is P.
    return ac.concat(
        route_row[15:16],
        route_row[13:14],
        route_row[11:12],
        route_row[9:10],
        route_row[7:8],
        route_row[5:6],
        route_row[3:4],
        route_row[1:2],
    )


def row_address(index: ac.u6) -> ac.u9:
    address: ac.u9 = index
    return address * 8


def set_slot(storage: ac.bits[512], index: ac.u4, value: ac.u64) -> ac.bits[512]:
    shift: ac.u9 = index
    shift = shift * 64
    mask: ac.bits[512] = 18446744073709551615
    wide_value: ac.bits[512] = value
    return (storage & ~(mask << shift)) | (wide_value << shift)


def get_slot(storage: ac.bits[512], index: ac.u4) -> ac.u64:
    shift: ac.u9 = index
    shift = shift * 64
    word = storage >> shift
    return word[0:64]


@ac.module
def TimedWseMachine(host: HostRequest) -> TimedResult:
    program = ac.byte_mem[ac.bits[512]](depth=65536)
    ledger = ReceiveLedger(
        NODES=40, EARLY_SLOTS=2, SEEN_SLOTS=56, TOTAL_EARLY=80, TOTAL_SEEN=2240, ROW_BITS=1232
    )
    epochs = EpochLedger()
    configuration = CalRegLedger()
    current_bits: ac.bits[512] = 0
    pc: ac.u16 = 0
    count: ac.u16 = 0
    stage: ac.u5 = 0
    selected: ac.u6 = 0
    offset: ac.u32 = 0
    current: Command = Command(
        kind=0,
        node=0,
        opcode=0,
        epoch=0,
        seq=0,
        source=0,
        src=0,
        dst=0,
        nbytes=0,
        capacity=0,
        expected=0,
        compiled_expected=0,
        route=0,
        route_high=0,
        flags=0,
    )
    active_nodes: ac.u40 = 0
    frontier: ac.u40 = 0
    visited: ac.u40 = 0
    targets: ac.u40 = 0
    remote_mask: ac.u40 = 0
    beat_remaining: ac.u40 = 0
    parents = ac.table[40, ac.u6](init=0)
    payload: ac.bits[512] = 0
    fault_code: ac.u8 = 0
    read_bytes: ac.u64 = 0
    write_bytes: ac.u64 = 0

    # Discrete-event time is independent of the generated simulator's microsteps.
    model_time: ac.u64 = 0
    events: ac.u32 = 0
    event_kind: ac.u8 = 0
    event_source: ac.u6 = 0
    event_node: ac.u6 = 0
    event_seq: ac.u16 = 0
    event_flit: ac.u16 = 0
    event_ingress: ac.u6 = 0
    event_direction: ac.u3 = 0
    queue = EventQueue()
    calendar = ac.byte_mem[ac.bits[320]](depth=6400)
    queue_next = ac.byte_mem[ac.u16](depth=2048)
    queue_head = ac.byte_mem[ac.u64](depth=320)
    queue_tail = ac.byte_mem[ac.u64](depth=320)
    queue_count = ac.byte_mem[ac.u64](depth=320)
    busy = ac.byte_mem[ac.u64](depth=320)
    packets = ac.byte_mem[ac.bits[512]](depth=2560)
    packet_remote = ac.byte_mem[ac.u40](depth=200)
    packet_left = ac.byte_mem[ac.u40](depth=200)
    received = ac.byte_mem[ac.u16](depth=3200)
    committed_identities = ac.byte_mem[ac.u64](depth=320)
    reserved_identities = ac.byte_mem[ac.u64](depth=320)
    reserved = ac.byte_mem[ac.u64](depth=320)
    receive_capacity = ac.byte_mem[ac.u32](depth=160)
    read_free = ac.byte_mem[ac.u64](depth=320)
    write_free = ac.byte_mem[ac.u64](depth=320)
    root_left = ac.byte_mem[ac.u64](depth=320)
    network_pending = ac.byte_mem[ac.u64](depth=320)
    ejection_free = ac.byte_mem[ac.u64](depth=320)
    link_free = ac.byte_mem[ac.u64](depth=1280)
    inflight: ac.u8 = 0
    queued_total: ac.u16 = 0
    blocked: bool = False
    blocked_source: ac.u6 = 0
    scan: ac.u6 = 0
    first_ub_ticks: ac.u64 = 200
    ub_numerator: ac.u64 = 10
    ub_denominator: ac.u64 = 7
    tx_cdc: ac.u64 = 2
    rx_cdc: ac.u64 = 2
    hop_ticks: ac.u64 = 1
    slot_ticks: ac.u64 = 1024
    align_ticks: ac.u64 = 16
    arm_ticks: ac.u64 = 8
    guard_ticks: ac.u64 = 1
    write_first_ticks: ac.u64 = 200
    read_width: ac.u32 = 128
    write_width: ac.u32 = 128
    flit_payload_bytes: ac.u32 = 52
    calendar_origin: ac.u64 = 0
    pending_when: ac.bits[512] = 0
    pending_payload: ac.bits[512] = 0
    pending_count: ac.u4 = 0
    pending_read: ac.u4 = 0

    beat_bytes = (
        transfer_size(current.src + offset, current.dst + offset, current.nbytes - offset)
        if offset[0:6] == 0
        else 8
    )
    host_node_valid = host.node < 40
    host_address_valid = host.address <= 65528 and host.address[0:3] == 0
    host_address_valid = host_address_valid and (not host.wide or host.address[0:6] == 0)
    host_read_window = stage == 0 or stage == 6 or stage == 7
    host_read = host.kind == 4 and host_node_valid and host_address_valid and host_read_window
    observation_node = host.node if host_node_valid else 0
    memory = BankedMemory(
        ReadAddress(
            node=host.node if host_read else current.source[0:6] if stage == 4 else 0,
            address=(host.address if host_read else current.src + offset if stage == 4 else 0),
        )
    )
    buffer = BankedMemory(
        ReadAddress(node=current.source[0:6], address=offset if stage == 5 else 0)
    )
    hop = ForwardHop(
        HopInput(
            route=current.route,
            node=selected,
            has_ingress=selected != current.source[0:6],
            ingress=event_ingress if stage == 9 else parents[selected % 40],
        )
    )

    @ac.method
    def stage_1() -> None:
        one: ac.u40 = 1
        if blocked:
            memory_value_1_raw = busy.read(row_address(blocked_source % 40))
            memory_value_1 = memory_value_1_raw[0:1]
            memory_value_2_raw = queue_count.read(row_address(blocked_source % 40))
            memory_value_2 = memory_value_2_raw[0:4]
            if not memory_value_1 and memory_value_2 == 0:
                blocked = False
                pc = pc + 1
            else:
                scan = 0
                stage = 8
        elif pc >= count:
            fault_code = 2
            stage = 7
        else:
            address: ac.u16 = pc
            command = decode(program.read(address * 64))
            valid_header = command.kind != 0 and command.kind <= 9 and command.flags <= 1
            node_command = valid_header and command.kind != 7 and command.kind != 9
            valid_node = command.node < 40 and (
                command.kind == 6 or (command.opcode != 0 and command.opcode < 7)
            )
            if not valid_header or (node_command and not valid_node):
                fault_code = 2
                stage = 7
            if valid_header and command.kind == 9:
                if inflight != 0 or queued_total != 0:
                    scan = 0
                    stage = 8
                else:
                    model_time = model_time + align_ticks + arm_ticks
                    calendar_origin = model_time
                    pc = pc + 1
            if valid_header and command.kind == 7:
                if inflight != 0 or queued_total != 0:
                    scan = 0
                    stage = 8
                elif active_nodes != 0:
                    fault_code = 9
                    stage = 7
                else:
                    event_kind = 8
                    events = events + 1
                    stage = 6
            if node_command and valid_node:
                node = command.node[0:6]
                node_row = row_address(node % 40)
                opcode = command.opcode[0:3]
                observed = ledger.observe(node)
                drained = not observed.active and observed.pending == 0
                if command.kind == 1:
                    installed = configuration.process(
                        node, CalRegRequest(kind=2, opcode=opcode, drained=drained)
                    )
                    if installed.fault:
                        fault_code = 3
                        stage = 7
                    else:
                        pc = pc + 1
                elif command.kind == 2:
                    if not drained:
                        fault_code = 4
                        stage = 7
                    else:
                        epoch = epochs.process(
                            node, EpochRequest(advance=True, opcode=opcode, drained=drained)
                        )
                        if epoch.fault:
                            fault_code = 4
                            stage = 7
                        else:
                            pc = pc + 1
                elif command.kind == 3:
                    installed_post = configuration.process(
                        node, CalRegRequest(kind=3, opcode=opcode, drained=drained)
                    )
                    epoch_post = epochs.process(
                        node, EpochRequest(advance=False, opcode=opcode, drained=drained)
                    )
                    end: ac.u33 = command.dst
                    capacity: ac.u33 = command.capacity
                    end = end + capacity
                    if (
                        command.epoch == 0
                        or command.epoch > 255
                        or epoch_post.value != command.epoch[0:8]
                        or installed_post.fault
                        or (command.dst >= 65536)
                        or (command.dst[0:3] != 0)
                        or (end > 65536)
                    ):
                        fault_code = 5
                        stage = 7
                    else:
                        post = ledger.process(
                            node,
                            Request(
                                kind=1,
                                segment=segment_of(command),
                                capacity=command.capacity,
                                expected=command.expected,
                                compiled_expected=command.compiled_expected,
                            ),
                        )
                        if post.fault or not post.accepted:
                            fault_code = 5
                            stage = 7
                        else:
                            memory_write_3: ac.u64 = 0
                            committed_identities.write(node_row, memory_write_3)
                            memory_write_4: ac.u64 = 0
                            reserved_identities.write(node_row, memory_write_4)
                            capacity_address: ac.u8 = node
                            capacity_address = capacity_address * 4
                            receive_capacity.write(capacity_address, command.capacity)
                            active_nodes = active_nodes | one << node
                            pc = pc + 1
                elif command.kind == 4 or command.kind == 8:
                    if command.source >= 40:
                        fault_code = 7
                        stage = 7
                    else:
                        source = command.source[0:6]
                        source_row = row_address(source % 40)
                        count_word = queue_count.read(source_row)
                        queued = count_word[0:4]
                        if queued == 8:
                            scan = 0
                            stage = 8
                        else:
                            if queued == 0:
                                memory_write_7: ac.u64 = pc
                                queue_head.write(source_row, memory_write_7)
                            else:
                                memory_value_8_raw = queue_tail.read(source_row)
                                memory_value_8 = memory_value_8_raw[0:16]
                                previous = memory_value_8
                                next_address_wide = previous % 1024 * 2
                                next_address: ac.u11 = next_address_wide[0:11]
                                queue_next.write(next_address, pc)
                            memory_write_9: ac.u64 = pc
                            queue_tail.write(source_row, memory_write_9)
                            memory_write_11: ac.u64 = queued + 1
                            queue_count.write(source_row, memory_write_11)
                            queued_total = queued_total + 1
                            if command.kind == 4:
                                blocked = True
                                blocked_source = source
                                scan = 0
                                stage = 8
                            else:
                                pc = pc + 1
                elif command.kind == 5:
                    epoch_wait = epochs.process(
                        node, EpochRequest(advance=False, opcode=opcode, drained=drained)
                    )
                    matching = ledger.matches(node, opcode, command.epoch[0:8])
                    if (
                        command.epoch == 0
                        or command.epoch > 255
                        or (not matching)
                        or (epoch_wait.value != command.epoch[0:8])
                    ):
                        fault_code = 5
                        stage = 7
                    elif observed.complete:
                        event_kind = 7
                        event_node = node
                        events = events + 1
                        pc = pc + 1
                    elif inflight == 0 and queued_total == 0:
                        fault_code = 11
                        stage = 7
                    else:
                        scan = 0
                        stage = 8
                elif command.kind == 6:
                    memory_value_12_raw = reserved_identities.read(node_row)
                    memory_value_12 = memory_value_12_raw[0:8]
                    if memory_value_12 != 0:
                        scan = 0
                        stage = 8
                    else:
                        retired = ledger.process(
                            node,
                            Request(
                                kind=3,
                                segment=segment_of(command),
                                capacity=0,
                                expected=0,
                                compiled_expected=0,
                            ),
                        )
                        if retired.fault:
                            fault_code = 5
                            stage = 7
                        else:
                            memory_write_13: ac.u64 = 0
                            committed_identities.write(node_row, memory_write_13)
                            active_nodes = active_nodes & ~(one << node)
                            pc = pc + 1

    @ac.method
    def stage_2() -> None:
        one: ac.u40 = 1
        bit = one << selected
        if hop.fault or (visited & bit) != 0:
            fault_code = 6
            stage = 7
        else:
            visited = visited | bit
            if hop.lands and selected != current.source[0:6]:
                targets = targets | bit
            # ForwardHop already filters egress to physical neighbors of selected.
            discovered = visited | frontier
            egress = hop.egress
            collision = (discovered & egress) != 0
            new_neighbors = egress & ~discovered
            frontier = frontier | new_neighbors
            for direction in range(4):
                peer: ac.u6 = (
                    selected - 8
                    if direction == 0
                    else (
                        selected - 1
                        if direction == 1
                        else selected + 1 if direction == 2 else selected + 8
                    )
                )
                if (new_neighbors & (one << peer)) != 0:
                    parents[peer % 40] = selected
            # Each two-bit route entry stores its participation bit at position 1.
            route = current.route
            passing = ac.concat(
                row_participants(route[64:80]),
                row_participants(route[48:64]),
                row_participants(route[32:48]),
                row_participants(route[16:32]),
                row_participants(route[0:16]),
            )
            if collision:
                fault_code = 6
                stage = 7
            elif frontier == 0:
                if visited != passing:
                    fault_code = 6
                    stage = 7
                else:
                    selected = 0
                    stage = 3
            else:
                selected = first_node(frontier)
                frontier = frontier & ~(one << selected)

    @ac.method
    def stage_3() -> None:
        one: ac.u40 = 1
        capacity_address: ac.u8 = selected
        capacity_address = capacity_address * 4
        capacity_value = receive_capacity.read(capacity_address)
        target_bit = one << selected
        if targets & target_bit != 0:
            receive = ledger.observe(selected)
            admission = configuration.process(
                selected, CalRegRequest(kind=3, opcode=current.opcode[0:3], drained=False)
            )
            receive_epoch = epochs.process(
                selected, EpochRequest(advance=False, opcode=current.opcode[0:3], drained=False)
            )
            decision = ledger.inspect(selected, segment_of(current))
            if (
                admission.fault
                or receive_epoch.value != current.epoch[0:8]
                or receive.pending != 0
                or (decision.status == 9)
            ):
                fault_code = 5
                stage = 7
            elif decision.status == 3:
                targets = targets & ~target_bit
            else:
                reservation_address = row_address(selected % 40)
                reserved_word = reserved.read(reservation_address)
                reserved_bytes = reserved_word[0:32]
                if (
                    decision.status == 2
                    and receive.counted + reserved_bytes + current.nbytes > capacity_value
                ):
                    fault_code = 5
                    stage = 7
                elif decision.status != 2:
                    fault_code = 5
                    stage = 7
                else:
                    committed_word = committed_identities.read(reservation_address)
                    reserved_identity_word = reserved_identities.read(reservation_address)
                    committed_count = committed_word[0:8]
                    reserved_count = reserved_identity_word[0:8]
                    assert (
                        committed_count + reserved_count < 56
                    ), "receive identity reservation capacity"
                    next_reserved_count: ac.u64 = reserved_count + 1
                    reserved_identities.write(reservation_address, next_reserved_count)
                    next_reserved_bytes: ac.u64 = reserved_bytes + current.nbytes
                    reserved.write(reservation_address, next_reserved_bytes)
            receive_index: ac.u16 = current.source[0:6]
            selected_index: ac.u16 = selected
            receive_address_wide = (receive_index * 40 + selected_index) % 1600 * 2
            receive_address: ac.u12 = receive_address_wide[0:12]
            received.write(receive_address, 0)
        if stage == 3:
            if selected == 39:
                remote_mask = targets
                if current.flags == 1:
                    targets = targets | one << current.source[0:6]
                source = current.source[0:6]
                packet_address: ac.u12 = source
                packet_address = packet_address * 64
                packets.write(packet_address, current_bits)
                mask_address: ac.u8 = source
                mask_address = mask_address * 5
                packet_remote.write(mask_address, remote_mask)
                packet_left.write(mask_address, targets)
                if targets == 0:
                    memory_write_21: ac.u64 = 0
                    busy.write(row_address(source % 40), memory_write_21)
                    inflight = inflight - 1
                else:
                    read_address: ac.u9 = source
                    read_address = read_address * 8
                    read_available = read_free.read(read_address)
                    begin = read_available if read_available > model_time else model_time
                    finish = begin + service_ticks(
                        current.nbytes, first_ub_ticks, ub_numerator, ub_denominator, read_width
                    )
                    read_free.write(read_address, finish)
                    pending_when = set_slot(pending_when, pending_count, finish)
                    pending_payload = set_slot(
                        pending_payload, pending_count, event_payload(1, source, source, 0, 0, 0)
                    )
                    pending_count = pending_count + 1
                stage = 1
            else:
                selected = selected + 1

    @ac.method
    def stage_4() -> None:
        # Snapshot source bytes at one modeled read-completion instant.
        buffer.write(current.source[0:6], offset, memory, beat_bytes == 64)
        increment: ac.u64 = beat_bytes
        read_bytes = read_bytes + increment
        if offset + beat_bytes == current.nbytes:
            source = current.source[0:6]
            mask_address: ac.u8 = source
            mask_address = mask_address * 5
            remote_targets = packet_remote.read(mask_address)
            if remote_targets != 0:
                pending_when = set_slot(pending_when, pending_count, model_time + tx_cdc)
                pending_payload = set_slot(
                    pending_payload, pending_count, event_payload(2, source, source, 0, 0, 0)
                )
                initial_outstanding = network_pending.read(row_address(source))
                initial_outstanding = initial_outstanding + 1
                network_pending.write(row_address(source), initial_outstanding)
                pending_count = pending_count + 1
            if current.flags == 1:
                write_address: ac.u9 = source
                write_address = write_address * 8
                write_available = write_free.read(write_address)
                begin = write_available if write_available > model_time else model_time
                finish = begin + service_ticks(
                    current.nbytes, write_first_ticks, ub_numerator, ub_denominator, write_width
                )
                write_free.write(write_address, finish)
                pending_when = set_slot(pending_when, pending_count, finish)
                pending_payload = set_slot(
                    pending_payload, pending_count, event_payload(5, source, source, 0, 0, 0)
                )
                pending_count = pending_count + 1
            stage = 1
        else:
            offset = offset + beat_bytes

    @ac.method
    def stage_5() -> None:
        one: ac.u40 = 1
        if selected == current.source[0:6] or host.landing_permit & one << selected != 0:
            memory.write(selected, current.dst + offset, buffer, beat_bytes == 64)
            increment: ac.u64 = beat_bytes
            write_bytes = write_bytes + increment
            if offset + beat_bytes == current.nbytes:
                source = current.source[0:6]
                event_kind = 5
                events = events + 1
                if selected != source:
                    committed = ledger.commit(selected, segment_of(current))
                    assert committed == 2, "reserved receive commits once"
                    memory_value_22_raw = reserved.read(row_address(selected % 40))
                    memory_value_22 = memory_value_22_raw[0:32]
                    memory_write_23: ac.u64 = memory_value_22 - current.nbytes
                    reserved.write(row_address(selected % 40), memory_write_23)
                    memory_value_24_raw = reserved_identities.read(row_address(selected % 40))
                    memory_value_24 = memory_value_24_raw[0:8]
                    memory_write_25: ac.u64 = memory_value_24 - 1
                    reserved_identities.write(row_address(selected % 40), memory_write_25)
                    memory_value_26_raw = committed_identities.read(row_address(selected % 40))
                    memory_value_26 = memory_value_26_raw[0:8]
                    memory_write_27: ac.u64 = memory_value_26 + 1
                    committed_identities.write(row_address(selected % 40), memory_write_27)
                mask_address: ac.u8 = source
                mask_address = mask_address * 5
                remaining_targets = packet_left.read(mask_address)
                remaining_targets = remaining_targets & ~(one << selected)
                packet_left.write(mask_address, remaining_targets)
                if remaining_targets == 0:
                    pending_when = set_slot(pending_when, pending_count, model_time + guard_ticks)
                    pending_payload = set_slot(
                        pending_payload, pending_count, event_payload(6, source, source, 0, 0, 0)
                    )
                    pending_count = pending_count + 1
                stage = 1
            else:
                offset = offset + beat_bytes

    @ac.method
    def event_2(outstanding: ac.u16) -> ac.u16:
        one: ac.u40 = 1
        source = event_source
        if selected != source and (host.landing_permit & (one << selected)) == 0:
            pending_when = set_slot(pending_when, pending_count, model_time + 1)
            pending_payload = set_slot(
                pending_payload, pending_count, event_payload(5, source, selected, 0, 0, 0)
            )
            pending_count = pending_count + 1
            scan = 0
            stage = 8
        else:
            stage = 5

        return outstanding

    @ac.method
    def event_3(outstanding: ac.u16) -> ac.u16:
        source = event_source
        if outstanding != 0:
            pending_when = set_slot(pending_when, pending_count, model_time + 1)
            pending_payload = set_slot(
                pending_payload, pending_count, event_payload(6, source, source, 0, 0, 0)
            )
            pending_count = pending_count + 1
        else:
            memory_write_28: ac.u64 = 0
            busy.write(row_address(source % 40), memory_write_28)
            inflight = inflight - 1
        stage = 1

        return outstanding

    @ac.method
    def event_4(outstanding: ac.u16) -> ac.u16:
        one: ac.u40 = 1
        source = event_source
        # Each arrival independently forwards this flit, i.e. cut-through.
        root_mask: ac.u4 = 0
        for direction in range(4):
            direction_bits: ac.u3 = direction
            peer: ac.u6 = (
                selected - 8
                if direction == 0
                else (
                    selected - 1
                    if direction == 1
                    else selected + 1 if direction == 2 else selected + 8
                )
            )
            if (hop.egress & (one << peer)) != 0:
                if event_kind == 2:
                    direction_one: ac.u4 = 1
                    root_mask = root_mask | (direction_one << direction_bits)
                pending_when = set_slot(pending_when, pending_count, model_time)
                pending_payload = set_slot(
                    pending_payload,
                    pending_count,
                    event_payload(3, source, selected, peer, event_flit, direction_bits),
                )
                outstanding = outstanding + 1
                pending_count = pending_count + 1
        if event_kind == 2:
            root_word: ac.u64 = root_mask
            root_left.write(row_address(source), root_word)
        mask_address: ac.u8 = source
        mask_address = mask_address * 5
        remote_targets = packet_remote.read(mask_address)
        if event_kind == 4 and (remote_targets & (one << selected)) != 0:
            pending_when = set_slot(pending_when, pending_count, model_time)
            pending_payload = set_slot(
                pending_payload, pending_count, event_payload(9, source, selected, 0, event_flit, 0)
            )
            outstanding = outstanding + 1
            pending_count = pending_count + 1
        stage = 1

        return outstanding

    @ac.method
    def event_5(outstanding: ac.u16) -> ac.u16:
        source = event_source
        flits_wide = (current.nbytes + flit_payload_bytes - 1) // flit_payload_bytes
        flits: ac.u16 = flits_wide[0:16]
        ejection_address: ac.u9 = selected
        ejection_address = ejection_address * 8
        ejection_available = ejection_free.read(ejection_address)
        if ejection_available > model_time:
            pending_when = set_slot(pending_when, pending_count, ejection_available)
            pending_payload = set_slot(
                pending_payload, pending_count, event_payload(9, source, selected, 0, event_flit, 0)
            )
            outstanding = outstanding + 1
            pending_count = pending_count + 1
        else:
            ejection_free.write(ejection_address, model_time + 1)
            index: ac.u16 = source
            selected_index: ac.u16 = selected
            index = index * 40 + selected_index
            receive_address_wide = (index % 1600) * 2
            receive_address: ac.u12 = receive_address_wide[0:12]
            received_count = received.read(receive_address)
            received_count = received_count + 1
            received.write(receive_address, received_count)
            if received_count == flits:
                ready = model_time + rx_cdc
                write_address: ac.u9 = selected
                write_address = write_address * 8
                write_available = write_free.read(write_address)
                begin = write_available if write_available > ready else ready
                finish = begin + service_ticks(
                    current.nbytes,
                    write_first_ticks,
                    ub_numerator,
                    ub_denominator,
                    write_width,
                )
                write_free.write(write_address, finish)
                pending_when = set_slot(pending_when, pending_count, finish)
                pending_payload = set_slot(
                    pending_payload, pending_count, event_payload(5, source, selected, 0, 0, 0)
                )
                pending_count = pending_count + 1
        stage = 1

        return outstanding

    @ac.method
    def event_6(outstanding: ac.u16) -> ac.u16:
        source = event_source
        flits_wide = (current.nbytes + flit_payload_bytes - 1) // flit_payload_bytes
        flits: ac.u16 = flits_wide[0:16]
        link: ac.u16 = selected
        direction_index: ac.u16 = event_direction
        link = link * 4 + direction_index
        link_address: ac.u11 = link[0:11]
        link_address = link_address * 8
        link_available = link_free.read(link_address)
        candidate = link_available if link_available > model_time else model_time
        slot_wide = ((candidate - calendar_origin) // slot_ticks) % 8
        slot: ac.u16 = slot_wide[0:16]
        calendar_address_wide = link * 40
        calendar_address: ac.u13 = calendar_address_wide[0:13]
        calendar_row = calendar.read(calendar_address)
        # Gather this source's permissions, one bit per Calendar window.
        source_row = calendar_row >> source
        permissions: ac.u8 = 0
        for window in range(8):
            window_offset: ac.u9 = window
            window_bit: ac.u3 = window
            entry = source_row >> (window_offset * 40)
            permitted: ac.u8 = entry[0:1]
            permissions = permissions | (permitted << window_bit)
        # Rotate the calendar so bit zero is the current window.
        repeated: ac.u16 = permissions
        upcoming = (repeated | (repeated << 8)) >> slot
        windows = upcoming[0:8]
        distance: ac.u64 = ac.count_trailing_zeros(windows)
        scheduled = (
            candidate
            if distance == 0
            else calendar_origin
            + ((candidate - calendar_origin) // slot_ticks + distance) * slot_ticks
        )
        if windows == 0:
            fault_code = 12
            stage = 7
        elif scheduled != model_time:
            pending_when = set_slot(pending_when, pending_count, scheduled)
            pending_payload = set_slot(
                pending_payload,
                pending_count,
                event_payload(3, source, selected, event_ingress, event_flit, event_direction),
            )
            outstanding = outstanding + 1
            pending_count = pending_count + 1
            stage = 1
        else:
            link_free.write(link_address, model_time + 1)
            # One source flit remains buffered until all root branches accept it.
            if selected == source:
                root_word = root_left.read(row_address(source))
                root_mask: ac.u4 = root_word[0:4]
                direction_one: ac.u4 = 1
                root_mask = root_mask & ~(direction_one << event_direction)
                updated_root: ac.u64 = root_mask
                root_left.write(row_address(source), updated_root)
                if root_mask == 0 and event_flit + 1 < flits:
                    pending_when = set_slot(pending_when, pending_count, model_time + 1)
                    pending_payload = set_slot(
                        pending_payload,
                        pending_count,
                        event_payload(2, source, source, 0, event_flit + 1, 0),
                    )
                    outstanding = outstanding + 1
                    pending_count = pending_count + 1
            pending_when = set_slot(pending_when, pending_count, model_time + hop_ticks)
            pending_payload = set_slot(
                pending_payload,
                pending_count,
                event_payload(4, source, event_ingress, selected, event_flit, 0),
            )
            outstanding = outstanding + 1
            pending_count = pending_count + 1
            stage = 1

        return outstanding

    @ac.method
    def stage_9() -> None:
        queue_size = queue.size()
        if event_kind != 5:
            events = events + 1
        source = event_source
        outstanding_word = network_pending.read(row_address(source))
        outstanding: ac.u16 = outstanding_word[0:16]
        if event_kind == 2 or event_kind == 3 or event_kind == 4 or event_kind == 9:
            outstanding = outstanding - 1
        if queue_size > 1016:
            fault_code = 10
            stage = 7
        elif event_kind == 1:
            stage = 4
        elif event_kind == 5:
            outstanding = event_2(outstanding)
        elif event_kind == 6:
            outstanding = event_3(outstanding)
        elif event_kind == 2 or event_kind == 4:
            outstanding = event_4(outstanding)
        elif event_kind == 9:
            outstanding = event_5(outstanding)
        elif event_kind == 3:
            outstanding = event_6(outstanding)
        else:
            fault_code = 13
            stage = 7
        updated_outstanding: ac.u64 = outstanding
        network_pending.write(row_address(source), updated_outstanding)

    @ac.method
    def stage_8() -> None:
        queue_empty = queue.empty()
        if scan < 40:
            source = scan
            memory_value_29_raw = busy.read(row_address(source % 40))
            memory_value_29 = memory_value_29_raw[0:1]
            memory_value_30_raw = queue_count.read(row_address(source % 40))
            memory_value_30 = memory_value_30_raw[0:4]
            if not memory_value_29 and memory_value_30 != 0:
                memory_value_31_raw = queue_head.read(row_address(source % 40))
                memory_value_31 = memory_value_31_raw[0:16]
                next_pc = memory_value_31
                current_bits = program.read(next_pc * 64)
                command = decode(current_bits)
                current = command
                opcode = command.opcode[0:3]
                next_address_wide = next_pc % 1024 * 2
                next_address: ac.u11 = next_address_wide[0:11]
                next_head = queue_next.read(next_address)
                memory_write_32: ac.u64 = next_head
                queue_head.write(row_address(source % 40), memory_write_32)
                memory_write_34: ac.u64 = memory_value_30 - 1
                queue_count.write(row_address(source % 40), memory_write_34)
                queued_total = queued_total - 1
                memory_write_35: ac.u64 = 1
                busy.write(row_address(source % 40), memory_write_35)
                inflight = inflight + 1
                source_end: ac.u33 = command.src
                destination_end: ac.u33 = command.dst
                source_start: ac.u33 = command.src
                destination_start: ac.u33 = command.dst
                length: ac.u33 = command.nbytes
                source_end = source_end + length
                destination_end = destination_end + length
                overlap = (
                    source_start < destination_end
                    and destination_start < source_end
                    and (command.src != command.dst)
                )
                if (
                    command.source >= 40
                    or command.epoch == 0
                    or command.epoch > 255
                    or (command.seq > 65535)
                    or (command.route_high > 65535)
                    or (command.src[0:3] != 0)
                    or (command.dst[0:3] != 0)
                    or (command.nbytes == 0)
                    or (command.nbytes[0:3] != 0)
                    or (source_end > 65536)
                    or (destination_end > 65536)
                    or (command.flags == 1 and overlap)
                ):
                    fault_code = 7
                    stage = 7
                else:
                    source_admission = configuration.process(
                        command.source[0:6], CalRegRequest(kind=3, opcode=opcode, drained=False)
                    )
                    source_epoch = epochs.process(
                        command.source[0:6],
                        EpochRequest(advance=False, opcode=opcode, drained=False),
                    )
                    if source_admission.fault or source_epoch.value != command.epoch[0:8]:
                        fault_code = 5
                        stage = 7
                    else:
                        selected = command.source[0:6]
                        offset = 0
                        frontier = 0
                        visited = 0
                        targets = 0
                        stage = 2
            else:
                scan = scan + 1
        elif not queue_empty:
            queue.start_pop()
            stage = 10
        elif inflight != 0 or queued_total != 0:
            fault_code = 11
            stage = 7
        else:
            stage = 1

    @ac.method
    def stage_10() -> None:
        incoming = queue.result()
        model_time = incoming.when
        event_kind = incoming.payload[0:4]
        event_source = incoming.payload[4:10]
        event_node = incoming.payload[10:16]
        event_ingress = incoming.payload[16:22]
        event_flit = incoming.payload[22:38]
        event_direction = incoming.payload[38:41]
        packet_address: ac.u12 = event_source
        packet_address = packet_address * 64
        packet_bits = packets.read(packet_address)
        current = decode(packet_bits)
        event_seq = current.seq[0:16]
        selected = event_node
        offset = 0
        stage = 9

    @ac.rule
    def execute() -> TimedResult:
        host_in_range = host_node_valid and host_address_valid
        queue_busy = queue.busy()
        if queue_busy:
            queue.advance()
        elif pending_read < pending_count:
            ready_when = get_slot(pending_when, pending_read)
            ready_payload = get_slot(pending_payload, pending_read)
            queue.push(ready_when, ready_payload)
            pending_read = pending_read + 1
            if pending_read == pending_count:
                pending_read = 0
                pending_count = 0
        elif (
            host.kind > 6
            or not host_node_valid
            or (host.kind == 4 and (not host_address_valid or not host_read_window))
        ):
            fault_code = 1
            stage = 7
        elif host.kind == 6:
            if stage != 0:
                fault_code = 1
                stage = 7
            elif host.address < 1280:
                mask_word: ac.bits[320] = host.data[0:40]
                calendar_address_wide = host.address * 5
                calendar_address: ac.u13 = calendar_address_wide[0:13]
                calendar_mask: ac.u40 = 31
                calendar.write(calendar_address, mask_word, mask=calendar_mask)
            elif (
                host.address < 4096
                or host.address > 4109
                or (
                    (host.address == 4098 or (host.address >= 4101 and host.address <= 4103))
                    and host.data[0:64] == 0
                )
                or (host.address >= 4106 and host.address <= 4108 and host.data[0:32] == 0)
            ):
                fault_code = 1
                stage = 7
            else:
                if host.address == 4096:
                    first_ub_ticks = host.data[0:64]
                if host.address == 4097:
                    ub_numerator = host.data[0:64]
                if host.address == 4098:
                    ub_denominator = host.data[0:64]
                if host.address == 4099:
                    tx_cdc = host.data[0:64]
                if host.address == 4100:
                    rx_cdc = host.data[0:64]
                if host.address == 4101:
                    hop_ticks = host.data[0:64]
                if host.address == 4102:
                    slot_ticks = host.data[0:64]
                if host.address == 4103:
                    align_ticks = host.data[0:64]
                if host.address == 4104:
                    arm_ticks = host.data[0:64]
                if host.address == 4105:
                    guard_ticks = host.data[0:64]
                if host.address == 4106:
                    read_width = host.data[0:32]
                if host.address == 4107:
                    write_width = host.data[0:32]
                if host.address == 4108:
                    flit_payload_bytes = host.data[0:32]
                if host.address == 4109:
                    write_first_ticks = host.data[0:64]
        elif host.kind == 1:
            if stage != 0 or not host_in_range:
                fault_code = 1
                stage = 7
            else:
                memory.write(host.node, host.address, host.data, host.wide)
        elif host.kind == 2:
            if stage != 0 or not host_address_valid:
                fault_code = 1
                stage = 7
            else:
                word: ac.bits[512] = host.data
                program_mask: ac.u64 = 18446744073709551615 if host.wide else 255
                program.write(host.address[0:16], word, mask=program_mask)
        elif host.kind == 3:
            if stage != 0 or host.program_count == 0 or host.program_count > 1024:
                fault_code = 1
                stage = 7
            else:
                count = host.program_count
                stage = 1
        else:
            # Dispatch once, even if the selected method advances stage.
            active_stage = stage
            if active_stage == 1:
                stage_1()
            elif active_stage == 2:
                stage_2()
            elif active_stage == 3:
                stage_3()
            elif active_stage == 4:
                stage_4()
            elif active_stage == 5:
                stage_5()
            elif active_stage == 9:
                stage_9()
            elif active_stage == 8:
                stage_8()
            elif active_stage == 10:
                stage_10()
        data: ac.bits[512] = 0
        if host_read:
            narrow_data: ac.bits[512] = memory[0:64]
            data = memory if host.wide else narrow_data
        account = ledger.observe(observation_node)
        account_epoch = ledger.epoch_of(observation_node)
        state = MachineResult(
            data=data,
            pc=pc,
            stage=stage,
            selected_node=selected,
            offset=offset,
            counted=account.counted if host_node_valid else 0,
            active=account.active and host_node_valid,
            complete=account.complete and host_node_valid,
            pending=account.pending if host_node_valid else 0,
            epoch=account_epoch if host_node_valid else 0,
            fault=stage == 7,
            fault_code=fault_code,
            done=stage == 6,
            read_bytes=read_bytes,
            write_bytes=write_bytes,
        )

        return TimedResult(
            state=state,
            model_time=model_time,
            events=events,
            event_kind=event_kind,
            event_source=event_source,
            event_node=event_node,
            event_seq=event_seq,
            event_flit=event_flit,
        )

    return execute
