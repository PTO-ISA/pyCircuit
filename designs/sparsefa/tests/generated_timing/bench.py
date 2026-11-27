# ruff: noqa: F823, N802 -- pyCircuit captures state proposals and module names.
"""Observe the pyCircuit-authored Shared-KV resource schedule in generated C++."""

import pycircuit as ac
from sparsefa.fa2_shared_kv_timing import SharedKVTiming


@ac.module
def TraceScenario(count: ac.u32, kv_slots: ac.u32, heads: ac.u32, dim: ac.u32):
    status = SharedKVTiming(count, kv_slots, heads, dim)
    last_allocated: ac.u32 = 0
    last_released: ac.u32 = 0
    last_loaded: ac.u32 = 0
    last_dispatched: ac.u32 = 0
    last_filled: ac.u32 = 0
    last_retired: ac.u32 = 0
    last_requests: ac.u32 = 0
    last_stage: ac.u32 = 0
    reported: ac.u1 = 0

    @ac.rule
    def observe() -> None:
        assert status.outstanding <= 8, "at most eight KV row credits"
        assert status.index_allocated >= status.index_released
        assert status.index_allocated - status.index_released <= 2
        assert status.dispatched <= status.index_loaded
        assert status.filled <= status.dispatched
        assert status.retired <= status.filled
        assert status.requests <= count
        assert status.dispatched - status.retired <= kv_slots
        if (
            status.index_allocated != last_allocated
            or status.index_released != last_released
            or status.index_loaded != last_loaded
            or status.dispatched != last_dispatched
            or status.filled != last_filled
            or status.retired != last_retired
            or status.requests != last_requests
            or status.stage != last_stage
        ):
            ac.log(
                "info",
                "skv_generated_progress",
                count,
                kv_slots,
                status.cycle,
                status.index_allocated,
                status.index_released,
                status.index_loaded,
                status.dispatched,
                status.filled,
                status.retired,
                status.requests,
                status.outstanding,
                status.stage,
            )
            last_allocated = status.index_allocated
            last_released = status.index_released
            last_loaded = status.index_loaded
            last_dispatched = status.dispatched
            last_filled = status.filled
            last_retired = status.retired
            last_requests = status.requests
            last_stage = status.stage
        if (status.done == 1) & ~reported:
            ac.log(
                "info",
                "skv_generated_done",
                count,
                kv_slots,
                status.cycle,
                status.index_allocated,
                status.index_released,
                status.index_loaded,
                status.dispatched,
                status.filled,
                status.retired,
                status.requests,
            )
            reported = 1


@ac.system
def SharedKV128One():
    TraceScenario(128, 1, 64, 64)


@ac.system
def SharedKV128Two():
    TraceScenario(128, 2, 64, 64)


@ac.system
def SharedKV2048One():
    TraceScenario(2048, 1, 64, 64)


@ac.system
def SharedKV2048Two():
    TraceScenario(2048, 2, 64, 64)
