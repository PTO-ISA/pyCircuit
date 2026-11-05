# ruff: noqa: F821, F823, F841, N802 -- compiler-owned hardware state and System names.
"""Independent BF16 unequal-score D2 and FP16 D64 singleton acceptance.

BF16: Q=[1,-1], K=[[1,0],[0,1]], V=[[2,-2],[6,4]], indices=[1,0].
Binary64 expected [2.4768116880884703,-1.2847824678672946]; acceptance
uses BF16 atol=.07, rtol=.02, converted to conservative representable bounds.
FP16 singleton: original token2, 64 lanes V[d]=((d%8)-4)/2 exactly.
"""
import pycircuit as ac
from fa2.fa2_interfaces import MemoryReply
from fa2.fa2_scheduler import FA2TensorScheduler, TensorCommand


@ac.module
def VectorOffset(value: ac.u7) -> ac.u64:
    return value


@ac.system
def VectorSystem():
    cycle: ac.u16 = 0
    scenario: ac.u2 = 0
    started: ac.u1 = 0
    pending: ac.u1 = 0
    reply_data: ac.u32 = 0
    writes: ac.u7 = 0
    completions: ac.u2 = 0
    command = TensorCommand(
        valid=(scenario < 2) & ~started,
        tag=91,
        batches=1,
        heads=1,
        queries=1,
        keys=3,
        dimensions=2 if scenario == 0 else 64,
        scale=1065353216,
        indexed=1,
        causal=0,
        bf16=scenario == 0,
        causal_origin=0,
        causal_origin_negative=0,
        q_base=4096,
        k_base=8192,
        v_base=12288,
        o_base=16384,
        count_base=24576,
        index_base=32768,
        q_batch_stride=0,
        q_head_stride=0,
        q_row_stride=128,
        k_batch_stride=0,
        k_head_stride=0,
        k_row_stride=160,
        v_batch_stride=0,
        v_head_stride=0,
        v_row_stride=192,
        o_batch_stride=0,
        o_head_stride=0,
        o_row_stride=128,
        count_batch_stride=0,
        count_head_stride=0,
        count_row_stride=4,
        index_batch_stride=0,
        index_head_stride=0,
        index_row_stride=8192,
    )
    output_offset = VectorOffset(writes) << 1
    ready = ~pending & (cycle[:2] != 0)
    memory = MemoryReply(request_ready=ready, valid=pending, data=reply_data, error=0)
    request, status = FA2TensorScheduler(command, memory, 1, 0)

    @ac.rule
    def memory_and_oracle() -> None:
        cycle = cycle + 1
        assert (
            ~request.response_ready | pending
        ), "engine waits only for an accepted outstanding request"
        pending = 0
        if status.command_ready & command.valid:
            started = 1
            writes = 0
        if request.valid & ready:
            pending = 1
            reply_data = 0
            if request.address == 24576:
                reply_data = 2 if scenario == 0 else 1
            elif request.address == 32768:
                reply_data = 1 if scenario == 0 else 2
            elif request.address == 32772:
                assert scenario == 0, "singleton reads one index only"
                reply_data = 0
            elif (request.address >= 4096) & (request.address < 4224):
                if scenario == 0:
                    assert (request.address == 4096) | (
                        request.address == 4098
                    ), "BF16 Q shape"
                    reply_data = 16256 if request.address == 4096 else 49024
                else:
                    reply_data = 15360
            elif (request.address >= 8192) & (request.address < 8672):
                if scenario == 0:
                    assert (
                        (request.address == 8192)
                        | (request.address == 8194)
                        | (request.address == 8352)
                        | (request.address == 8354)
                    ), "BF16 K original token addresses"
                    reply_data = (
                        16256
                        if (request.address == 8192) | (request.address == 8354)
                        else 0
                    )
                else:
                    assert (request.address >= 8512) & (
                        request.address < 8640
                    ), "D64 singleton K original token2"
                    reply_data = 0
            elif (request.address >= 12288) & (request.address < 12864):
                if scenario == 0:
                    assert (
                        (request.address == 12288)
                        | (request.address == 12290)
                        | (request.address == 12480)
                        | (request.address == 12482)
                    ), "BF16 V distinct stride"
                    if request.address == 12288:
                        reply_data = 16384
                    elif request.address == 12290:
                        reply_data = 49152
                    elif request.address == 12480:
                        reply_data = 16576
                    else:
                        reply_data = 16512
                else:
                    assert (request.address >= 12672) & (
                        request.address < 12800
                    ), "D64 singleton V original token2"
                    memory_lane = ((request.address - 12672) >> 1) & 7
                    if memory_lane == 0:
                        reply_data = 49152
                    elif memory_lane == 1:
                        reply_data = 48640
                    elif memory_lane == 2:
                        reply_data = 48128
                    elif memory_lane == 3:
                        reply_data = 47104
                    elif memory_lane == 4:
                        reply_data = 0
                    elif memory_lane == 5:
                        reply_data = 14336
                    elif memory_lane == 6:
                        reply_data = 15360
                    else:
                        reply_data = 15872
            else:
                assert request.write, "only known read regions"
            if request.write:
                assert request.size == 2, "output element size"
                assert (
                    request.address == 16384 + output_offset
                ), "ordered distinct output lanes"
                if scenario == 0:
                    assert writes < 2, "BF16 output width"
                    if writes == 0:
                        assert (request.data >= 16407) & (
                            request.data <= 16422
                        ), "BF16 lane0 binary64 tolerance"
                    else:
                        assert (request.data & 32768) != 0, "BF16 negative lane sign"
                        assert ((request.data & 32767) >= 16281) & (
                            (request.data & 32767) <= 16304
                        ), "BF16 lane1 binary64 tolerance"
                else:
                    assert writes < 64, "D64 output width"
                    lane = writes & 7
                    expected: ac.u32 = 0
                    if lane == 0:
                        expected = 49152
                    elif lane == 1:
                        expected = 48640
                    elif lane == 2:
                        expected = 48128
                    elif lane == 3:
                        expected = 47104
                    elif lane == 5:
                        expected = 14336
                    elif lane == 6:
                        expected = 15360
                    elif lane == 7:
                        expected = 15872
                    assert (
                        request.data == expected
                    ), "FP16 singleton equals independent V lane"
                writes = writes + 1
        if status.completion_valid:
            assert status.code == 0, "vector success"
            assert status.tag == 91, "vector command tag"
            assert pending == 0, "vector writes acknowledged before terminal"
            if scenario == 0:
                assert writes == 2, "all BF16 lanes published"
            else:
                assert writes == 64, "all D64 lanes published"
            completions = completions + 1
            scenario = scenario + 1
            started = 0
        if cycle == 5000:
            assert (
                completions == 2
            ), "vector bounded progress and no duplicate completion"
            assert scenario == 2, "both vector scenarios"
            ac.log("info", "vector_pass", completions, writes)

    memory_and_oracle()
