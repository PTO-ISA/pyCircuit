# ruff: noqa: F821, F823, F841, N802 -- compiler-owned hardware state and System names.
"""Independent delayed memory failures, operand rejection and clean restart.

D2 singleton Q=[1,1], K=[0,0], V=[2,-3], so normal O=V exactly.
Scenarios0..5 error COUNT/INDEX/Q/K/Vsecondlane/Ofirstlane responses;
6..8 reject Q infinity, K=16 and V NaN. Scenario9 restarts successfully.
All responses retain pending until DUT response_ready accepts them.
"""
import pycircuit as ac
from fa2.fa2_interfaces import MemoryReply
from fa2.fa2_scheduler import FA2TensorScheduler, TensorCommand


@ac.system
def FaultSystem():
    cycle: ac.u16 = 0
    scenario: ac.u4 = 0
    started: ac.u1 = 0
    pending: ac.u1 = 0
    delay: ac.u2 = 0
    reply_data: ac.u32 = 0
    reply_error: ac.u1 = 0
    injected: ac.u1 = 0
    writes: ac.u2 = 0
    completions: ac.u4 = 0
    command = TensorCommand(valid=(scenario < 10) & ~started, tag=117,
        batches=1, heads=1, queries=1, keys=1, dimensions=2,
        scale=1065353216, indexed=1, causal=0, bf16=0,
        causal_origin=0, causal_origin_negative=0,
        q_base=4096, k_base=8192, v_base=12288, o_base=16384,
        count_base=24576, index_base=32768,
        q_batch_stride=0, q_head_stride=0, q_row_stride=4,
        k_batch_stride=0, k_head_stride=0, k_row_stride=8,
        v_batch_stride=0, v_head_stride=0, v_row_stride=12,
        o_batch_stride=0, o_head_stride=0, o_row_stride=4,
        count_batch_stride=0, count_head_stride=0, count_row_stride=4,
        index_batch_stride=0, index_head_stride=0, index_row_stride=8192)
    ready = ~pending & (cycle[:2] != 0)
    response_valid = pending & (delay == 0)
    memory = MemoryReply(request_ready=ready, valid=response_valid,
                         data=reply_data, error=reply_error)
    request, status = FA2TensorScheduler(command, memory, 1, 0)

    @ac.rule
    def memory_and_oracle() -> None:
        cycle = cycle + 1
        assert ~request.response_ready | pending, "fault DUT waits only for accepted requests"
        if status.command_ready & command.valid:
            started = 1
            writes = 0
            injected = 0
        if pending:
            if delay != 0:
                delay = delay - 1
            if response_valid & request.response_ready:
                pending = 0
        if request.valid & ready:
            assert ~pending, "fault test one outstanding request"
            pending = 1
            delay = 2
            reply_data = 0
            reply_error = 0
            if request.address == 24576:
                assert request.size == 4, "fault count size"
                reply_data = 1
                if scenario == 0:
                    reply_error = 1
                    injected = 1
            elif request.address == 32768:
                assert request.size == 4, "fault index size"
                reply_data = 0
                if scenario == 1:
                    reply_error = 1
                    injected = 1
            elif (request.address == 4096) | (request.address == 4098):
                reply_data = 15360
                if (scenario == 2) & (request.address == 4098):
                    reply_error = 1
                    injected = 1
                if (scenario == 6) & (request.address == 4098):
                    reply_data = 31744
                    injected = 1
            elif (request.address == 8192) | (request.address == 8194):
                reply_data = 0
                if (scenario == 3) & (request.address == 8194):
                    reply_error = 1
                    injected = 1
                if (scenario == 7) & (request.address == 8194):
                    reply_data = 19456
                    injected = 1
            elif (request.address == 12288) | (request.address == 12290):
                reply_data = 16384 if request.address == 12288 else 49664
                if (scenario == 4) & (request.address == 12290):
                    reply_error = 1
                    injected = 1
                if (scenario == 8) & (request.address == 12290):
                    reply_data = 32256
                    injected = 1
            elif (request.address == 16384) | (request.address == 16386):
                assert request.write, "O accesses are writes"
                assert (scenario == 5) | (scenario == 9), "failed inputs never publish O"
                assert request.size == 2, "fault output element size"
                if request.address == 16384:
                    assert writes == 0, "output lane0 exactly once"
                    assert request.data == 16384, "singleton lane0 equals original V"
                    if scenario == 5:
                        reply_error = 1
                        injected = 1
                else:
                    assert writes == 1, "output lane1 exactly once"
                    assert request.data == 49664, "singleton lane1 equals original V"
                writes = writes + 1
            else:
                assert False, "fault test unexpected address"  # noqa: B011 -- hardware assertion, not host Python.
            if request.address < 16384:
                assert ~request.write, "QKV reads never write"
        if status.completion_valid:
            assert pending == 0, "fault completion drains final response"
            assert ~request.valid, "fault completion issues no request"
            assert status.tag == 117, "fault command identity"
            if scenario < 6:
                assert injected, "memory fault actually reached"
                assert status.code == 5, "all memory response failures map to code5"
            elif scenario < 9:
                assert injected, "operand fault actually reached"
                assert status.code == 7, "invalid QKV operands map to code7"
            else:
                assert status.code == 0, "clean restart after all fault classes"
            if scenario == 5:
                assert writes == 1, "failed O response leaves result invalid and stops further writes"
            elif scenario == 9:
                assert writes == 2, "clean restart publishes complete vector"
            else:
                assert writes == 0, "failed input/shadow transaction publishes no output"
            completions = completions + 1
            scenario = scenario + 1
            started = 0
        if cycle == 3000:
            assert completions == 10, "all faults and restart complete once"
            assert scenario == 10, "no duplicate fault completion"
            ac.log("info", "fault_pass", completions, writes)
    memory_and_oracle()
