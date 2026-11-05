# ruff: noqa: F821, F823, F841, N802 -- compiler-owned captured state and published System names.
"""Generated-DUT tensor scheduling and negative causal-origin scenario.

The test-owned memory model supplies zero Q/K and distinct constant V per head.
With one key, the independent expected output is V for query1 and zero for the
fully masked query0. Irregular strides make batch/head/query swaps observable.
"""
import pycircuit as ac
from fa2.fa2_interfaces import MemoryReply
from fa2.fa2_scheduler import FA2TensorScheduler, TensorCommand


@ac.system
def TensorSchedulerSystem():
    cycle: ac.u16 = 0
    started: ac.u1 = 0
    pending: ac.u1 = 0
    reply_data: ac.u32 = 0
    writes: ac.u8 = 0
    completions: ac.u8 = 0
    command = TensorCommand(
        valid=~started,
        tag=73,
        batches=2,
        heads=2,
        queries=2,
        keys=1,
        dimensions=1,
        scale=1065353216,
        indexed=0,
        causal=1,
        bf16=0,
        causal_origin=1,
        causal_origin_negative=1,
        q_base=4096,
        k_base=8192,
        v_base=12288,
        o_base=16384,
        count_base=0,
        index_base=0,
        q_batch_stride=128,
        q_head_stride=32,
        q_row_stride=6,
        k_batch_stride=144,
        k_head_stride=36,
        k_row_stride=2,
        v_batch_stride=160,
        v_head_stride=40,
        v_row_stride=2,
        o_batch_stride=176,
        o_head_stride=44,
        o_row_stride=10,
        count_batch_stride=0,
        count_head_stride=0,
        count_row_stride=0,
        index_batch_stride=0,
        index_head_stride=0,
        index_row_stride=0,
    )
    ready = (cycle[:2] != 0) & ~pending
    memory = MemoryReply(request_ready=ready, valid=pending, data=reply_data, error=0)
    request, status = FA2TensorScheduler(command, memory, 1, 0)

    @ac.rule
    def memory_and_check() -> None:
        was_pending = pending
        cycle = cycle + 1
        pending = 0
        if status.command_ready & ~started:
            started = 1
        if request.valid & ready:
            pending = 1
            reply_data = 0
            assert request.size == 2, "tensor transfer size"
            if request.address == 12288:
                reply_data = 15360
            elif request.address == 12328:
                reply_data = 16384
            elif request.address == 12448:
                reply_data = 16896
            elif request.address == 12488:
                reply_data = 17408
            if request.write:
                assert writes < 8, "no duplicate tensor outputs"
                expected_address: ac.u64 = 16384
                if (writes & 4) != 0:
                    expected_address = expected_address + 176
                if (writes & 2) != 0:
                    expected_address = expected_address + 44
                if (writes & 1) != 0:
                    expected_address = expected_address + 10
                assert request.address == expected_address, "tensor row address"
                expected_value: ac.u32 = 0
                if writes == 1:
                    expected_value = 15360
                elif writes == 3:
                    expected_value = 16384
                elif writes == 5:
                    expected_value = 16896
                elif writes == 7:
                    expected_value = 17408
                assert request.data == expected_value, "independent masked row output"
                writes = writes + 1
        if status.completion_valid:
            assert status.code == 0, "successful tensor status"
            assert status.tag == 73, "tensor identity"
            assert writes == 8, "completion after every output"
            assert was_pending == 0, "completion after final write acknowledgement"
            completions = completions + 1
        if cycle == 1000:
            assert writes == 8, "bounded tensor progress"
            assert completions == 1, "exactly one tensor completion"
            ac.log("info", "tensor_scheduler_pass", writes, completions)

    memory_and_check()
