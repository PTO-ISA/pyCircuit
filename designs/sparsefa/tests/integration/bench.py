# ruff: noqa: F821, F823, F841, N802 -- compiler-owned hardware state and System names.
"""Independent sparse lifecycle scenarios over the full tensor scheduler.

Q=1, K=[0,1,7], V=[2,6,8], list=[2,0,1], causal position1.
The original token2 is masked. Binary64 oracle: (2+6*exp(1))/(1+exp(1)).
The FP16 output must be within one representable FP16 step of that oracle.
"""
import pycircuit as ac
from fa2.fa2_interfaces import MemoryReply
from fa2.fa2_scheduler import FA2TensorScheduler, TensorCommand


@ac.module
def SparseResponseWord(address: ac.u64, scenario: ac.u4) -> ac.u32:
    data = 0
    if address == 24576:
        data = 2049 if scenario == 3 else 0 if scenario == 6 else 2048 if scenario == 7 else 3
    elif address == 32768:
        data = 2048 if scenario == 7 else 3 if scenario == 2 else 2
    elif address == 32776:
        data = 0 if scenario == 1 else 1
    elif address == 4096 or address == 8200:
        data = 15360
    elif address == 12288:
        data = 16384
    elif address == 12302:
        data = 17920
    return data

@ac.system
def SparseLifecycleSystem():
    cycle: ac.u16 = 0
    scenario: ac.u4 = 0
    started: ac.u1 = 0
    pending: ac.u1 = 0
    delay: ac.u3 = 0
    reply_data: ac.u32 = 0
    writes: ac.u4 = 0
    row_writes: ac.u2 = 0
    completions: ac.u4 = 0
    terminal_age: ac.u3 = 0
    stalled: ac.u1 = 0
    held_address: ac.u64 = 0
    held_data: ac.u32 = 0
    held_size: ac.u3 = 0
    held_write: ac.u1 = 0
    terminal_code: ac.u4 = 0
    terminal_tag: ac.u32 = 0
    command = TensorCommand(valid=~started & (scenario < 8), tag=73, batches=1, heads=1, queries=1, keys=2048 if scenario == 7 else 3, dimensions=1, scale=1065353216, indexed=1, causal=1, bf16=0, causal_origin=1, causal_origin_negative=0, q_base=4096, k_base=8192, v_base=12288, o_base=16384, count_base=24576, index_base=32768, q_batch_stride=0, q_head_stride=0, q_row_stride=2, k_batch_stride=0, k_head_stride=0, k_row_stride=8, v_batch_stride=0, v_head_stride=0, v_row_stride=14, o_batch_stride=0, o_head_stride=0, o_row_stride=2, count_batch_stride=0, count_head_stride=0, count_row_stride=4, index_batch_stride=0, index_head_stride=0, index_row_stride=8192)
    ready = ~pending & (cycle[:2] == 0)
    response_valid = pending & (delay == 0)
    cancelling = (scenario == 4) & pending & (delay == 2)
    completion_ready = terminal_age == 3
    memory = MemoryReply(request_ready=ready, valid=response_valid, data=reply_data, error=0)
    request, status = FA2TensorScheduler(command, memory, completion_ready, cancelling)
    response_word = SparseResponseWord(request.address, scenario)

    @ac.rule
    def protocol_and_oracle() -> None:
        cycle = cycle + 1
        assert ~request.response_ready | pending, 'engine waits only for an accepted outstanding request'
        if status.command_ready & command.valid:
            started = 1
            row_writes = 0
        if pending:
            if delay != 0:
                delay = delay - 1
            if response_valid & request.response_ready:
                pending = 0
        if stalled & ~cancelling:
            assert request.valid, 'stalled request retained'
            assert request.address == held_address, 'stable stalled address'
            assert request.write == held_write, 'stable stalled direction'
            assert request.size == held_size, 'stable stalled size'
            if request.write:
                assert request.data == held_data, 'stable stalled write payload'
        stalled = request.valid & ~ready
        held_address = request.address
        held_data = request.data
        held_size = request.size
        held_write = request.write
        if request.valid & ready:
            reply_data = response_word
            assert ~pending, 'at most one outstanding request'
            pending = 1
            delay = 3
            assert (request.address != 8208) & (request.address != 12316), 'masked original token never gathered'
            if request.address == 24576:
                assert request.size == 4, 'external count width'
            elif request.address == 32768:
                assert request.size == 4, 'external index width'
            elif request.address == 32772:
                pass
            elif request.address == 32776:
                pass
            elif request.address == 4096:
                assert (scenario == 0) | (scenario == 5) | (scenario == 6), 'validation precedes Q access'
            elif request.address == 8192:
                pass
            elif request.address == 8200:
                pass
            elif request.address == 12288:
                pass
            elif request.address == 12302:
                pass
            else:
                assert request.write & (request.address == 16384), 'only declared tensor addresses'
            if request.write:
                assert (scenario == 0) | (scenario == 5) | (scenario == 6), 'no failed-operation publication'
                assert request.address == 16384, 'output address'
                if scenario == 6:
                    assert request.data == 0, 'empty indexed row is zero'
                else:
                    assert (request.data >= 17644) & (request.data <= 17646), 'independent nontrivial attention oracle'
                assert row_writes == 0, 'no duplicate output per command'
                row_writes = row_writes + 1
                writes = writes + 1
        if status.completion_valid:
            assert pending == 0, 'terminal waits for response drain'
            assert ~request.valid, 'terminal issues no memory requests'
            if terminal_age == 0:
                terminal_code = status.code
                terminal_tag = status.tag
            else:
                assert status.code == terminal_code, 'held terminal code stable'
                assert status.tag == terminal_tag, 'held terminal identity stable'
            terminal_age = terminal_age + 1
            expected_code: ac.u4 = 0
            if scenario == 1:
                expected_code = 4
            elif (scenario == 2) | (scenario == 7):
                expected_code = 3
            elif scenario == 3:
                expected_code = 2
            elif scenario == 4:
                expected_code = 6
            assert status.code == expected_code, 'scenario terminal status'
            if expected_code == 0:
                assert row_writes == 1, 'successful completion has one output'
            else:
                assert row_writes == 0, 'failed command has no output'
            assert status.tag == 73, 'command identity'
            if completion_ready:
                completions = completions + 1
                scenario = scenario + 1
                started = 0
                terminal_age = 0
        if cycle == 2000:
            assert completions == 8, 'all sparse/error/cancel/restart commands completed'
            assert writes == 3, 'exactly one output per successful command'
            assert scenario == 8, 'completion not duplicated'
            ac.log('info', 'sparse_lifecycle_pass', completions, writes)
    protocol_and_oracle()
