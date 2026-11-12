# ruff: noqa: F823, N802 -- pyCircuit captures state proposals and module names.
"""Generated Shared-KV resource schedule, not an attention datapath.

Inputs are fixed for one reset-to-done run. Count is 64..2048 in multiples
of 64; heads/dim are positive multiples of 16 up to 64; KV slots are 1 or 2.
Index values are externally precomputed: validation consumes 64 cycles but
neither fetches actual indices nor verifies their values. No numeric results
are produced. All timing assumptions below are explicit scheduling budgets.

Two Index slots and one/two KV slots retain ordered batches. The memory model
issues eight rows, waits 16 cycles, then drains one shared K=V FP16/BF16
row per request (2*D bytes) on a
32-byte/cycle shared response bus. Credits return together after that group.
This conservative grouped policy is intentionally distinct from rolling DMA
credits in the independent handwritten model. Index response beats have bus
priority. Q is loaded before streaming; O is written after all batches.

This bounded shared-list, unmasked, error-free subset omits tail batches and
head/dimension tails, actual duplicate checking, cancellation/error draining,
SRAM bank/feeder stalls and final numerical normalization. It is not NDF
conformance evidence or a complete hardware latency prediction.
"""

import pycircuit as ac


@ac.struct
class TimingStatus:
    cycle: ac.u32
    done: ac.u32
    stage: ac.u32
    batch: ac.u32
    slot: ac.u32
    index_allocated: ac.u32
    index_released: ac.u32
    index_loaded: ac.u32
    dispatched: ac.u32
    filled: ac.u32
    retired: ac.u32
    index_stage: ac.u32
    dma_stage: ac.u32
    requests: ac.u32
    outstanding: ac.u32


@ac.module
def SharedKVTiming(
    count: ac.u32, kv_slots: ac.u32, heads: ac.u32, dim: ac.u32
) -> TimingStatus:
    cycle: ac.u32 = 0
    setup: ac.u32 = 0
    setup_left: ac.u32 = 0
    index_allocated: ac.u32 = 0
    index_released: ac.u32 = 0
    index_loaded: ac.u32 = 0
    index_phase: ac.u32 = 0
    index_left: ac.u32 = 0
    dispatched: ac.u32 = 0
    filled: ac.u32 = 0
    dma_phase: ac.u32 = 0
    dma_left: ac.u32 = 0
    groups: ac.u32 = 0
    requests: ac.u32 = 0
    outstanding: ac.u32 = 0
    retired: ac.u32 = 0
    stage: ac.u32 = 0
    stage_left: ac.u32 = 0
    wave: ac.u32 = 0
    done: ac.u32 = 0

    # Independent clock accounting and initial Count/Q transfer budget.
    @ac.rule
    def startup():
        if done == 0:
            cycle = cycle + 1
        if setup == 0:
            assert count >= 64
            assert count <= 2048
            assert count % 64 == 0
            assert kv_slots >= 1
            assert kv_slots <= 2
            assert heads >= 16
            assert heads <= 64
            assert heads % 16 == 0
            assert dim >= 16
            assert dim <= 64
            assert dim % 16 == 0
            # Count latency+beat, Q latency+FP16 beats.
            setup_left = heads * dim // 16 + 29
            setup = 1
        elif setup == 1:
            if setup_left == 1:
                setup = 2
            else:
                setup_left = setup_left - 1

    # Reservations include DMA, validation and KV request dispatch.
    # Metadata is considered copied when the last row request is issued.
    @ac.rule
    def index_stream():
        if setup == 2:
            if index_phase == 0:
                if index_loaded < count // 64:
                    if index_allocated - index_released < 2:
                        index_allocated = index_allocated + 1
                        index_phase = 1
                        index_left = 12
            elif index_phase == 1:
                if index_left == 1:
                    index_phase = 2
                    index_left = 8
                else:
                    index_left = index_left - 1
            elif index_phase == 2:
                if index_left == 1:
                    index_phase = 3
                    index_left = 64
                else:
                    index_left = index_left - 1
            elif index_phase == 3:
                if index_left == 1:
                    index_loaded = index_loaded + 1
                    index_phase = 0
                else:
                    index_left = index_left - 1

    # Atomic reservation/credit updates share one Rule; no separate handshakes.
    @ac.rule
    def kv_dma():
        if dma_phase == 0:
            if dispatched < index_loaded:
                if dispatched - retired < kv_slots:
                    dispatched = dispatched + 1
                    dma_phase = 1
                    dma_left = 8
                    groups = 0
        elif dma_phase == 1:
            requests = requests + 1
            outstanding = outstanding + 1
            if dma_left == 1:
                if groups == 7:
                    index_released = index_released + 1
                dma_phase = 2
                dma_left = 16
            else:
                dma_left = dma_left - 1
        elif dma_phase == 2:
            if dma_left == 1:
                dma_phase = 3
                # Eight shared K=V rows * dim * two bytes / 32.
                dma_left = dim // 2
            else:
                dma_left = dma_left - 1
        elif dma_phase == 3:
            if index_phase != 2:
                if dma_left == 1:
                    outstanding = 0
                    if groups == 7:
                        filled = filled + 1
                        dma_phase = 0
                    else:
                        groups = groups + 1
                        dma_phase = 1
                        dma_left = 8
                else:
                    dma_left = dma_left - 1

    # One compute consumer, four-stage ordered batch commit; slot stays owned
    # through every 16-head wave. Array tiles use begin/groups/end/4 bubbles/take.
    @ac.rule
    def compute():
        if stage == 0:
            if retired < filled:
                wave = 0
                stage = 1
                stage_left = (dim // 16 + 7) * 4
        elif stage == 1:
            if stage_left == 1:
                stage = 2
                # Four FP32 passes at 64/cycle, 1040 exponentials at 32/cycle.
                stage_left = 97
            else:
                stage_left = stage_left - 1
        elif stage == 2:
            if stage_left == 1:
                stage = 3
                stage_left = dim // 16 * 11
            else:
                stage_left = stage_left - 1
        elif stage == 3:
            if stage_left == 1:
                if wave + 1 < heads // 16:
                    wave = wave + 1
                    stage = 1
                    stage_left = (dim // 16 + 7) * 4
                else:
                    stage = 4
            else:
                stage_left = stage_left - 1
        elif stage == 4:
            retired = retired + 1
            if retired == count // 64:
                stage = 5
                # FP16 O bytes plus acknowledgement latency.
                stage_left = heads * dim // 16 + 12
            else:
                stage = 0
        elif stage == 5:
            if stage_left == 1:
                stage = 6
                done = 1
            else:
                stage_left = stage_left - 1

    return TimingStatus(
        cycle=cycle,
        done=done,
        stage=stage,
        batch=retired,
        slot=retired % kv_slots,
        index_allocated=index_allocated,
        index_released=index_released,
        index_loaded=index_loaded,
        dispatched=dispatched,
        filled=filled,
        retired=retired,
        index_stage=index_phase,
        dma_stage=dma_phase,
        requests=requests,
        outstanding=outstanding,
    )
