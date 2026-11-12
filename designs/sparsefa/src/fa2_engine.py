# ruff: noqa: F821, F823, F841, N802
"""Serial single-query sparse/dense attention hardware, bounded D <= 64.

Independent guarded transactions implement external memory phases. The active
index prefix is fully fetched, range checked and duplicate checked before Q/K/V
work. Indexed capacity is 2048; dense traversal uses the command key length.
FP32-format arithmetic follows fa2_numeric's explicit finite RTZ/FTZ profile.
"""
import pycircuit as ac
from fa2.fa2_interfaces import EngineCommand, EngineStatus, MemoryReply, MemoryRequest
from fa2.fa2_numeric import decode16, encode16, fp_add, fp_div, fp_exp_neg, fp_mul


@ac.module
def Extend32(value: ac.u32) -> ac.u64:
    return value


@ac.module
def Extend6(value: ac.u6) -> ac.u64:
    return value


@ac.module
def Extend7(value: ac.u7) -> ac.u64:
    return value


@ac.module
def Slot(value: ac.u6) -> ac.u7:
    return value


@ac.module
def Extend64(value: ac.u64) -> ac.bits[128]:
    return value


@ac.struct
class EngineState:
    count: ac.u32
    position: ac.u32
    incoming_index: ac.u32
    dimension: ac.u6
    token: ac.u32
    dot: ac.u32
    score: ac.u32
    maximum: ac.u32
    denominator: ac.u32
    next_maximum: ac.u32
    next_denominator: ac.u32
    alpha: ac.u32
    beta: ac.u32
    query_lane: ac.u32
    accumulator_lane: ac.u32
    value_lane: ac.u32
    scaled_accumulation: ac.u32
    bank: ac.u7


@ac.struct
class DatapathResult:
    dimension_wide: ac.u7
    query_address: ac.u8
    current_address: ac.u9
    next_address: ac.u9
    decoded: ac.u32
    added: ac.u32
    multiplied: ac.u32
    exponential: ac.u32
    new_maximum: ac.u32
    operand_bad: ac.u1
    address_bad: ac.u1
    alignment_bad: ac.u1
    metadata_alignment_bad: ac.u1
    scale_bad: ac.u1
    waiting: ac.u1
    request_phase: ac.u1
    encoded: ac.u16
    lane_offset: ac.u64
    token_offset: ac.u64
    index_offset: ac.u64


@ac.module
def Datapath(
    command: EngineCommand,
    saved: EngineCommand,
    state: EngineState,
    memory: MemoryReply,
    phase: ac.u6,
) -> DatapathResult:
    dimension_wide = Slot(state.dimension)
    current_slot = state.bank | dimension_wide
    next_slot = (state.bank ^ 64) | dimension_wide
    query_address = (Extend6(state.dimension) << 2)[:8]
    current_address = (Extend7(current_slot) << 2)[:9]
    next_address = (Extend7(next_slot) << 2)[:9]
    lane_offset = Extend6(state.dimension) * 2
    token_offset = Extend32(state.token)
    index_offset = Extend32(state.position) * 4
    request_phase = (
        (phase == 1)
        | (phase == 3)
        | (phase == 6)
        | (phase == 9)
        | (phase == 12)
        | (phase == 14)
    )
    waiting = (
        (phase == 2)
        | (phase == 4)
        | (phase == 7)
        | (phase == 10)
        | (phase == 13)
        | (phase == 15)
    )
    decoded = decode16(memory.data[:16], saved.bf16)
    score_larger = (
        (
            (state.score[:31] > state.maximum[:31])
            & (state.score[31:32] == 0)
            & (state.maximum[31:32] == 0)
        )
        | (
            (state.score[:31] < state.maximum[:31])
            & (state.score[31:32] == 1)
            & (state.maximum[31:32] == 1)
        )
        | ((state.score[31:32] == 0) & (state.maximum[31:32] == 1))
    )
    new_maximum = (
        state.score if (state.denominator == 0) | score_larger else state.maximum
    )
    # One shared multiply, add and exponential datapath. State phases serialize
    # dependent operations and keep arithmetic hardware replication bounded.
    multiply_a = (
        state.query_lane
        if phase == 10
        else (
            state.denominator
            if phase == 20
            else (
                state.accumulator_lane
                if phase == 21
                else state.beta if phase == 22 else state.dot
            )
        )
    )
    multiply_b = (
        decoded
        if phase == 10
        else (
            state.alpha
            if (phase == 20) | (phase == 21)
            else state.value_lane if phase == 22 else saved.scale
        )
    )
    multiplied = fp_mul(multiply_a, multiply_b)
    add_a = (
        state.dot
        if phase == 10
        else (
            state.maximum
            if phase == 18
            else (
                state.score
                if phase == 19
                else multiplied if phase == 20 else state.scaled_accumulation
            )
        )
    )
    add_b = (
        multiplied
        if (phase == 10) | (phase == 22)
        else (
            (state.next_maximum ^ 2147483648)
            if (phase == 18) | (phase == 19)
            else state.beta
        )
    )
    added = fp_add(add_a, add_b)
    exponential = fp_exp_neg(added)
    normalized = fp_div(state.accumulator_lane, state.denominator)
    encoded = encode16(normalized, saved.bf16)

    # Reject wraparound before accepting any memory operation. All relevant
    # address expressions are computed with 128-bit intermediate precision.
    command_last = command.key_length - 1 if command.key_length != 0 else 0
    command_width = Extend7(command.dimensions) * 2
    q_end = Extend64(command.q_base) + Extend64(command_width)
    o_end = Extend64(command.o_base) + Extend64(command_width)
    k_end = (
        Extend64(command.k_base)
        + Extend64(Extend32(command_last)) * Extend64(command.k_stride)
        + Extend64(command_width)
    )
    v_end = (
        Extend64(command.v_base)
        + Extend64(Extend32(command_last)) * Extend64(command.v_stride)
        + Extend64(command_width)
    )
    index_end = Extend64(command.index_base) + 8192
    count_end = Extend64(command.count_base) + 4
    address_bad = (
        (q_end > 18446744073709551616)
        | (o_end > 18446744073709551616)
        | (k_end > 18446744073709551616)
        | (v_end > 18446744073709551616)
        | (
            command.indexed
            & ((index_end > 18446744073709551616) | (count_end > 18446744073709551616))
        )
    )
    alignment_bad = (
        (
            command.q_base
            | command.k_base
            | command.v_base
            | command.o_base
            | command.k_stride
            | command.v_stride
        )
        & 1
    ) != 0
    metadata_alignment_bad = command.indexed & (
        ((command.count_base | command.index_base) & 3) != 0
    )
    scale_bad = (
        (command.scale[31:32] != 0)
        | (command.scale > 1065353216)
        | (command.scale < 8388608)
    )
    operand_bad = (decoded & 2147483647) > 1090519040
    return DatapathResult(
        dimension_wide=dimension_wide,
        query_address=query_address,
        current_address=current_address,
        next_address=next_address,
        decoded=decoded,
        added=added,
        multiplied=multiplied,
        exponential=exponential,
        new_maximum=new_maximum,
        operand_bad=operand_bad,
        address_bad=address_bad,
        alignment_bad=alignment_bad,
        metadata_alignment_bad=metadata_alignment_bad,
        scale_bad=scale_bad,
        waiting=waiting,
        request_phase=request_phase,
        encoded=encoded,
        lane_offset=lane_offset,
        token_offset=token_offset,
        index_offset=index_offset,
    )


def trie_address(slot: ac.u13) -> ac.u15:
    wide: ac.u16 = slot
    return (wide << 3)[:15]


def trie_leaf(key: ac.u32) -> ac.u64:
    wide: ac.u64 = key
    return wide | 9223372036854775808


def trie_branch(bit: ac.u5, left: ac.u13, right: ac.u13) -> ac.u64:
    bit_wide: ac.u64 = bit
    left_wide: ac.u64 = left
    right_wide: ac.u64 = right
    return (bit_wide << 32) | (left_wide << 37) | (right_wide << 49)


def trie_patch(node: ac.u64, right: ac.u1, child: ac.u13) -> ac.u64:
    old_left: ac.u13 = node[37:49]
    old_right: ac.u13 = node[49:61]
    left_child = old_left if right else child
    right_child = child if right else old_right
    return trie_branch(node[32:37], left_child, right_child)


@ac.struct
class TrieStatus:
    is_leaf: ac.u1
    matches: ac.u1
    insert_here: ac.u1
    has_parent: ac.u1


@ac.module
def IndexTrie(
    incoming_index: ac.u32,
    position: ac.u32,
    phase: ac.u6,
    memory_valid: ac.u1,
    cancel: ac.u1,
) -> TrieStatus:
    """Exact uint32 membership, at most 32 branch decisions per traversal.

    Node publication precedes ordered-index publication in phase32. A cancelled
    command abandons the tree; phase0 invalidates it without clearing the RAM.
    """
    nodes = ac.byte_mem[ac.u64](depth=32768)
    root: ac.u13 = 0
    next_free: ac.u13 = 0
    cursor: ac.u13 = 0
    node: ac.u64 = 0
    split_bit: ac.u5 = 0
    parent: ac.u13 = 0
    parent_node: ac.u64 = 0
    parent_right: ac.u1 = 0
    parent_valid: ac.u1 = 0

    # One trie transition owns its correlated node, path and allocator updates.
    # The active-phase guard also leaves unknown cancel unreachable in idle
    # attention/done phases where this module has no operation.
    active = (
        (phase == 0)
        | (phase == 4)
        | (phase == 5)
        | (phase == 25)
        | (phase == 27)
        | (phase == 28)
        | (phase == 29)
        | (phase == 30)
        | (phase == 31)
        | (phase == 33)
    )

    # One node write port serves leaf allocation, branch allocation and parent
    # publication. Its mux observes committed state; step owns the write and
    # its correlated pointer updates in one transaction.
    new_branch = next_free + 1
    allocation_right = (incoming_index >> split_bit)[:1]
    left_child = cursor if allocation_right else next_free
    right_child = next_free if allocation_right else cursor
    allocated_branch = trie_branch(split_bit, left_child, right_child)
    patched_parent = trie_patch(parent_node, parent_right, new_branch)
    write_slot = (
        0
        if phase == 33
        else next_free if phase == 29 else new_branch if phase == 30 else parent
    )
    write_data = (
        trie_leaf(incoming_index)
        if (phase == 29) | (phase == 33)
        else allocated_branch if phase == 30 else patched_parent
    )
    write_node = (
        (phase == 29) | (phase == 30) | (phase == 33) | ((phase == 31) & parent_valid)
    )

    @ac.rule(guard=lambda: memory_valid)
    def start_key_ready() -> None:
        cursor = root
        parent_valid = 0

    @ac.rule(guard=lambda: active & ~cancel)
    def step() -> None:
        if write_node:
            nodes.write(trie_address(write_slot), write_data)
        match phase:
            case 0:
                root = 0
                next_free = 0
                parent_valid = 0
            case 4:
                start_key_ready()
            case 25 | 27:
                if position != 0:
                    node = nodes.read(trie_address(cursor))
            case 5:
                if position != 0:
                    if node[63:64]:
                        if node[:32] != incoming_index:
                            split_bit = (
                                31 - ac.count_leading_zeros(incoming_index ^ node[:32])
                            )[:5]
                            cursor = root
                            parent_valid = 0
                    else:
                        right = (incoming_index >> node[32:37])[:1]
                        cursor = node[49:61] if right else node[37:49]
            case 28:
                if ~node[63:64] & (node[32:37] >= split_bit):
                    right = (incoming_index >> node[32:37])[:1]
                    parent = cursor
                    parent_node = node
                    parent_right = right
                    parent_valid = 1
                    cursor = node[49:61] if right else node[37:49]
            case 29:
                assert next_free <= 4093, "index trie allocation capacity"
            case 31:
                if ~parent_valid:
                    root = new_branch
                next_free = next_free + 2
            case 33:
                root = 0
                next_free = 1

    step()
    return TrieStatus(
        is_leaf=node[63:64],
        matches=node[:32] == incoming_index,
        insert_here=node[63:64] | (node[32:37] < split_bit),
        has_parent=parent_valid,
    )


@ac.struct
class ControllerState:
    phase: ac.u6
    code: ac.u4


@ac.module
def ControllerTransition(
    command: EngineCommand,
    saved: EngineCommand,
    memory: MemoryReply,
    completion_ready: ac.u1,
    cancel: ac.u1,
    progress: EngineState,
    datapath: DatapathResult,
    trie: TrieStatus,
    phase: ac.u6,
    code: ac.u4,
) -> ControllerState:
    next_phase = phase
    next_code = code
    if cancel & (phase != 0) & (phase != 16) & (phase != 17):
        next_code = 6
        next_phase = 16
        if datapath.waiting & ~memory.valid:
            next_phase = 17
    else:
        match phase:
            case 0:
                if command.valid & ~cancel:
                    next_code = 0
                    next_phase = 1 if command.indexed else 6
                    if (
                        (command.dimensions == 0)
                        | (command.dimensions > 64)
                        | ~command.indexed & (command.key_length > 2048)
                        | datapath.address_bad
                        | datapath.alignment_bad
                        | datapath.metadata_alignment_bad
                        | datapath.scale_bad
                    ):
                        next_code = 1
                        next_phase = 16
            case 1 | 3 | 6 | 9 | 12 | 14:
                if memory.request_ready:
                    next_phase = phase + 1
            case 2:
                if memory.valid:
                    next_phase = 6 if memory.data == 0 else 3
                    if (memory.data > 2048) | (memory.data > saved.key_length):
                        next_code = 2
                        next_phase = 16
                    if memory.error:
                        next_code = 5
                        next_phase = 16
            case 4:
                if memory.valid:
                    next_phase = 25
                    if memory.data >= saved.key_length:
                        next_code = 3
                        next_phase = 16
                    if memory.error:
                        next_code = 5
                        next_phase = 16
            case 25:
                next_phase = 5
            case 5:
                next_phase = 25
                if progress.position == 0:
                    next_phase = 33
                elif trie.is_leaf:
                    next_phase = 27
                    if trie.matches:
                        next_code = 4
                        next_phase = 16
            case 27:
                next_phase = 28
            case 28:
                next_phase = 29 if trie.insert_here else 27
            case 29 | 30 | 31:
                next_phase = phase + 1
            case 33:
                next_phase = 32
            case 32:
                next_phase = 6 if progress.position + 1 == progress.count else 3
            case 7:
                if memory.valid:
                    next_phase = (
                        8 if datapath.dimension_wide == saved.dimensions - 1 else 6
                    )
                    if memory.error | datapath.operand_bad:
                        next_code = 5 if memory.error else 7
                        next_phase = 16
            case 8:
                next_phase = 24 if progress.position >= progress.count else 26
            case 26:
                next_phase = 9
                if saved.causal & (
                    saved.causal_empty | (progress.token > saved.query_position)
                ):
                    next_phase = 8
            case 10:
                if memory.valid:
                    next_phase = (
                        23 if datapath.dimension_wide == saved.dimensions - 1 else 9
                    )
                    if memory.error | datapath.operand_bad:
                        next_code = 5 if memory.error else 7
                        next_phase = 16
            case 11:
                next_phase = 18
            case 13:
                if memory.valid:
                    next_phase = 21
                    if memory.error | datapath.operand_bad:
                        next_code = 5 if memory.error else 7
                        next_phase = 16
            case 15:
                if memory.valid:
                    next_phase = (
                        16 if datapath.dimension_wide == saved.dimensions - 1 else 24
                    )
                    if memory.error:
                        next_code = 5
                        next_phase = 16
            case 16:
                if completion_ready:
                    next_phase = 0
            case 17:
                if memory.valid:
                    next_phase = 16
            case 18:
                next_phase = 19
            case 19:
                next_phase = 20
            case 20:
                next_phase = 12
            case 21:
                next_phase = 22
            case 22:
                next_phase = (
                    8 if datapath.dimension_wide == saved.dimensions - 1 else 12
                )
            case 23:
                next_phase = 11
            case 24:
                next_phase = 14
    return ControllerState(phase=next_phase, code=next_code)


@ac.module
def PhaseController(
    command: EngineCommand,
    saved: EngineCommand,
    memory: MemoryReply,
    completion_ready: ac.u1,
    cancel: ac.u1,
    progress: EngineState,
    datapath: DatapathResult,
    trie: TrieStatus,
) -> ControllerState:
    phase: ac.u6 = 0
    code: ac.u4 = 0

    transition = ControllerTransition(
        command,
        saved,
        memory,
        completion_ready,
        cancel,
        progress,
        datapath,
        trie,
        phase,
        code,
    )

    @ac.rule
    def advance() -> None:
        phase = transition.phase
        code = transition.code

    advance()
    return ControllerState(phase=phase, code=code)


@ac.module
def FA2Engine(
    command: EngineCommand, memory: MemoryReply, completion_ready: ac.u1, cancel: ac.u1
) -> tuple[MemoryRequest, EngineStatus]:
    # 0 idle; 1/2 count; 3/4 index; 5/25/27..33 trie; 6/7 Q;
    # 8 token; 9/10 K; 11 softmax; 12/13 V; 14/15 O; 16 done; 17 drain.
    count: ac.u32 = 0
    position: ac.u32 = 0
    incoming_index: ac.u32 = 0
    dimension: ac.u6 = 0
    token: ac.u32 = 0
    dot: ac.u32 = 0
    score: ac.u32 = 0
    maximum: ac.u32 = 0
    denominator: ac.u32 = 0
    next_maximum: ac.u32 = 0
    next_denominator: ac.u32 = 0
    alpha: ac.u32 = 0
    beta: ac.u32 = 0
    query_lane: ac.u32 = 0
    accumulator_lane: ac.u32 = 0
    value_lane: ac.u32 = 0
    scaled_accumulation: ac.u32 = 0
    bank: ac.u7 = 0
    saved: EngineCommand = EngineCommand(
        valid=0,
        tag=0,
        q_base=0,
        k_base=0,
        v_base=0,
        o_base=0,
        count_base=0,
        index_base=0,
        k_stride=0,
        v_stride=0,
        key_length=0,
        query_position=0,
        dimensions=0,
        scale=0,
        indexed=0,
        causal=0,
        causal_empty=0,
        bf16=0,
    )
    queries = ac.byte_mem[ac.u32](depth=256)
    accumulators = ac.byte_mem[ac.u32](depth=512)
    indices = ac.byte_mem[ac.u32](depth=8192)

    progress = EngineState(
        count=count,
        position=position,
        incoming_index=incoming_index,
        dimension=dimension,
        token=token,
        dot=dot,
        score=score,
        maximum=maximum,
        denominator=denominator,
        next_maximum=next_maximum,
        next_denominator=next_denominator,
        alpha=alpha,
        beta=beta,
        query_lane=query_lane,
        accumulator_lane=accumulator_lane,
        value_lane=value_lane,
        scaled_accumulation=scaled_accumulation,
        bank=bank,
    )
    controller = PhaseController(
        command, saved, memory, completion_ready, cancel, progress, datapath, trie
    )
    phase = controller.phase
    code = controller.code
    datapath = Datapath(command, saved, progress, memory, phase)
    trie = IndexTrie(incoming_index, position, phase, memory.valid, cancel)

    # Pure module wires let each Rule capture only its consumed fields.
    datapath_added = datapath.added
    datapath_current_address = datapath.current_address
    datapath_decoded = datapath.decoded
    datapath_dimension_wide = datapath.dimension_wide
    datapath_exponential = datapath.exponential
    datapath_multiplied = datapath.multiplied
    datapath_new_maximum = datapath.new_maximum
    datapath_next_address = datapath.next_address
    datapath_query_address = datapath.query_address
    memory_data = memory.data
    memory_request_ready = memory.request_ready
    memory_valid = memory.valid
    saved_causal = saved.causal
    saved_causal_empty = saved.causal_empty
    saved_dimensions = saved.dimensions
    saved_indexed = saved.indexed
    saved_query_position = saved.query_position

    @ac.rule(guard=lambda: command.valid)
    def capture_command_ready() -> None:
        saved = command
        count = command.key_length
        position = 0
        dimension = 0
        maximum = 0
        denominator = 0
        bank = 0

    @ac.rule(guard=lambda: (phase == 0) & ~cancel)
    def capture_command() -> None:
        capture_command_ready()

    @ac.rule(guard=lambda: memory_request_ready)
    def load_query_lane_ready() -> None:
        query_lane = queries.read(datapath_query_address)

    @ac.rule(guard=lambda: (phase == 9) & ~cancel)
    def load_query_lane() -> None:
        load_query_lane_ready()

    @ac.rule(guard=lambda: memory_valid)
    def capture_count_ready() -> None:
        count = memory_data

    @ac.rule(guard=lambda: (phase == 2) & ~cancel)
    def capture_count() -> None:
        capture_count_ready()

    @ac.rule(guard=lambda: memory_valid)
    def capture_index_ready() -> None:
        incoming_index = memory_data

    @ac.rule(guard=lambda: (phase == 4) & ~cancel)
    def capture_index() -> None:
        capture_index_ready()

    @ac.rule(guard=lambda: (phase == 32) & ~cancel)
    def validate_prefix() -> None:
        indices.write((position << 2)[:13], incoming_index)
        position = position + 1
        if position == count:
            position = 0

    @ac.rule(guard=lambda: memory_valid)
    def load_query_ready() -> None:
        queries.write(datapath_query_address, datapath_decoded)
        accumulators.write(datapath_current_address, 0)
        if datapath_dimension_wide == saved_dimensions - 1:
            dimension = 0
        else:
            dimension = dimension + 1

    @ac.rule(guard=lambda: (phase == 7) & ~cancel)
    def load_query() -> None:
        load_query_ready()

    @ac.rule(guard=lambda: (phase == 8) & ~cancel)
    def load_token() -> None:
        if position < count:
            selected = position
            if saved_indexed:
                selected = indices.read((position << 2)[:13])
            token = selected
        dot = 0
        dimension = 0

    @ac.rule(guard=lambda: (phase == 26) & ~cancel)
    def mask_token() -> None:
        if saved_causal & (saved_causal_empty | (token > saved_query_position)):
            position = position + 1

    @ac.rule(guard=lambda: memory_valid)
    def accumulate_dot_ready() -> None:
        dot = datapath_added
        if datapath_dimension_wide == saved_dimensions - 1:
            dimension = 0
        else:
            dimension = dimension + 1

    @ac.rule(guard=lambda: (phase == 10) & ~cancel)
    def accumulate_dot() -> None:
        accumulate_dot_ready()

    @ac.rule(guard=lambda: (phase == 11) & ~cancel)
    def select_max() -> None:
        next_maximum = datapath_new_maximum

    @ac.rule(guard=lambda: memory_valid)
    def load_value_ready() -> None:
        value_lane = datapath_decoded
        accumulator_lane = accumulators.read(datapath_current_address)

    @ac.rule(guard=lambda: (phase == 13) & ~cancel)
    def load_value() -> None:
        load_value_ready()

    @ac.rule(guard=lambda: memory_valid)
    def advance_output_ready() -> None:
        if datapath_dimension_wide == saved_dimensions - 1:
            pass
        else:
            dimension = dimension + 1

    @ac.rule(guard=lambda: (phase == 15) & ~cancel)
    def advance_output() -> None:
        advance_output_ready()

    @ac.rule(guard=lambda: (phase == 18) & ~cancel)
    def compute_alpha() -> None:
        alpha = 0 if denominator == 0 else datapath_exponential

    @ac.rule(guard=lambda: (phase == 19) & ~cancel)
    def compute_beta() -> None:
        beta = datapath_exponential

    @ac.rule(guard=lambda: (phase == 20) & ~cancel)
    def update_denominator() -> None:
        next_denominator = datapath_added

    @ac.rule(guard=lambda: (phase == 21) & ~cancel)
    def rescale_vector() -> None:
        scaled_accumulation = datapath_multiplied

    @ac.rule(guard=lambda: (phase == 22) & ~cancel)
    def commit_vector() -> None:
        accumulators.write(datapath_next_address, datapath_added)
        if datapath_dimension_wide == saved_dimensions - 1:
            maximum = next_maximum
            denominator = next_denominator
            bank = bank ^ 64
            dimension = 0
            position = position + 1
        else:
            dimension = dimension + 1

    @ac.rule(guard=lambda: (phase == 23) & ~cancel)
    def scale_score() -> None:
        score = datapath_multiplied

    @ac.rule(guard=lambda: (phase == 24) & ~cancel)
    def load_output() -> None:
        accumulator_lane = accumulators.read(datapath_current_address)

    capture_command()
    load_query_lane()
    capture_count()
    capture_index()
    validate_prefix()
    load_query()
    load_token()
    mask_token()
    accumulate_dot()
    select_max()
    load_value()
    advance_output()
    compute_alpha()
    compute_beta()
    update_denominator()
    rescale_vector()
    commit_vector()
    scale_score()
    load_output()
    address = (
        saved.count_base
        if phase == 1
        else (
            saved.index_base + datapath.index_offset
            if phase == 3
            else (
                saved.q_base + datapath.lane_offset
                if phase == 6
                else (
                    saved.k_base
                    + datapath.token_offset * saved.k_stride
                    + datapath.lane_offset
                    if phase == 9
                    else (
                        saved.v_base
                        + datapath.token_offset * saved.v_stride
                        + datapath.lane_offset
                        if phase == 12
                        else saved.o_base + datapath.lane_offset
                    )
                )
            )
        )
    )
    return MemoryRequest(
        valid=datapath.request_phase & ~cancel,
        write=phase == 14,
        address=address,
        data=datapath.encoded if denominator != 0 else 0,
        size=4 if (phase == 1) | (phase == 3) else 2,
        response_ready=datapath.waiting | (phase == 17),
    ), EngineStatus(
        command_ready=(phase == 0) & ~cancel,
        completion_valid=phase == 16,
        tag=saved.tag,
        code=code,
        phase=phase,
    )
