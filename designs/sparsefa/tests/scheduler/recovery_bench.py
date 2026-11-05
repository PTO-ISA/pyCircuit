# ruff: noqa: F821, F823, F841, N802 -- compiler-owned captured state and published System names.
"""Cancel an outstanding read, reuse the scheduler, then reject bad layout."""
import pycircuit as ac
from fa2.fa2_interfaces import MemoryReply
from fa2.fa2_scheduler import FA2TensorScheduler, TensorCommand


@ac.system
def TensorRecoverySystem():
    cycle: ac.u16 = 0
    scenario: ac.u2 = 0
    started: ac.u1 = 0
    pending: ac.u1 = 0
    delay: ac.u2 = 0
    writes: ac.u8 = 0
    requests: ac.u8 = 0
    completions: ac.u8 = 0
    command = TensorCommand(valid=~started & (scenario < 3), tag=100,
        batches=1, heads=1, queries=1, keys=0, dimensions=1,
        scale=1065353216, indexed=0, causal=0, bf16=0,
        causal_origin=0, causal_origin_negative=0,
        q_base=4096, k_base=8192, v_base=12288,
        o_base=16385 if scenario == 2 else 16384,
        count_base=0, index_base=0,
        q_batch_stride=0, q_head_stride=0, q_row_stride=0,
        k_batch_stride=0, k_head_stride=0, k_row_stride=0,
        v_batch_stride=0, v_head_stride=0, v_row_stride=0,
        o_batch_stride=0, o_head_stride=0, o_row_stride=0,
        count_batch_stride=0, count_head_stride=0, count_row_stride=0,
        index_batch_stride=0, index_head_stride=0, index_row_stride=0)
    response_valid = pending & (delay == 0)
    memory = MemoryReply(request_ready=~pending, valid=response_valid, data=0, error=0)
    cancel = (scenario == 0) & pending & (delay == 1)
    request, status = FA2TensorScheduler(command, memory, 1, cancel)

    @ac.rule
    def memory_and_check() -> None:
        was_pending = pending
        cycle = cycle + 1
        if pending:
            if delay == 0:
                pending = 0
            else:
                delay = delay - 1
        if status.command_ready & ~started & (scenario < 3):
            started = 1
        if request.valid & ~was_pending:
            assert scenario != 2, "invalid command has no memory traffic"
            pending = 1
            delay = 3
            requests = requests + 1
            if request.write:
                assert scenario == 1, "cancelled command must not write output"
                assert request.address == 16384, "reused output address"
                assert request.data == 0, "empty row zero output"
                writes = writes + 1
        if status.completion_valid:
            assert was_pending == 0, "terminal waits for drain"
            if scenario == 0:
                assert status.code == 6, "cancel status"
                assert requests == 1, "cancel stopped new requests"
                assert writes == 0, "cancel leaves no output"
            elif scenario == 1:
                assert status.code == 0, "successful reuse"
                assert requests == 3, "empty row Q and O only"
                assert writes == 1, "single empty-row write"
            else:
                assert status.code == 1, "layout rejection"
                assert requests == 3, "no rejected-command requests"
            completions = completions + 1
            scenario = scenario + 1
            started = 0
        if cycle == 400:
            assert completions == 3, "all recovery cases completed"
            assert writes == 1, "exactly one write across recovery"
            ac.log("info", "tensor_recovery_pass", completions, writes)
    memory_and_check()
