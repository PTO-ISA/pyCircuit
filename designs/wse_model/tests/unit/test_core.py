"""The AICORE model: pipes, local DRAM, buffers, and Cube/Vector throughput."""

from __future__ import annotations

import pytest

from wse_model.analysis.platform import LOCAL_DRAM_PAGE_BYTES
from wse_model.core import (
    BUFFER_CAPACITY_BYTES,
    INDEPENDENT_FROM_MTE4,
    BufferKind,
    LocalDram,
    LocalDramLayout,
    MatmulShape,
    Pipe,
    PipeBarrier,
    PipeTracker,
    StepRequirement,
    TileRequirement,
    calendar_step_pipes,
    fit_check,
    matmul_timing,
    vector_timing,
)
from wse_model.errors import OpenItemError, WseModelError

pytestmark = pytest.mark.unit


# -- pipelines -------------------------------------------------------------


def test_the_pipe_set_matches_the_design() -> None:
    assert {pipe.value for pipe in Pipe} == {
        "PIPE_S",
        "PIPE_MTE1",
        "PIPE_MTE2",
        "PIPE_MTE3",
        "PIPE_MTE4",
        "PIPE_M",
        "PIPE_V",
        "PIPE_FIX",
    }
    assert Pipe.CUBE.is_compute and Pipe.VECTOR.is_compute and Pipe.FIXPIPE.is_compute
    assert Pipe.MTE3.is_memory and Pipe.MTE4.is_memory
    assert not Pipe.SCALAR.is_memory


def test_mte3_and_scalar_stay_dispatchable_while_mte4_waits() -> None:
    """Calendar §4.7, invariant O3: otherwise every member waits on nobody."""
    tracker = PipeTracker()
    tracker.begin_mte4_wait()
    assert tracker.dispatchable() == INDEPENDENT_FROM_MTE4
    tracker.check_no_deadlock()
    tracker.issue(Pipe.MTE3)
    tracker.issue(Pipe.SCALAR)
    with pytest.raises(WseModelError) as excinfo:
        tracker.issue(Pipe.VECTOR)
    assert "O3" in str(excinfo.value)


def test_a_tracker_without_a_wait_admits_every_pipe() -> None:
    tracker = PipeTracker()
    assert set(tracker.dispatchable()) == set(Pipe)
    tracker.issue(Pipe.VECTOR, 2)
    tracker.check_no_deadlock()
    assert not tracker.drained
    tracker.retire(Pipe.VECTOR, 2)
    assert tracker.drained


def test_retiring_more_than_outstanding_is_rejected() -> None:
    tracker = PipeTracker()
    tracker.issue(Pipe.CUBE)
    with pytest.raises(WseModelError):
        tracker.retire(Pipe.CUBE, 2)


def test_the_mte3_barrier_must_wait_for_the_actual_send() -> None:
    """Calendar §4.7 item 4: not for the NoC's CalReg release."""
    barrier = PipeBarrier(Pipe.MTE3)
    assert barrier.waits_for_actual_send
    assert barrier.admits_release(sent=4, queued=4)
    assert not barrier.admits_release(sent=3, queued=4)
    with pytest.raises(WseModelError):
        PipeBarrier(Pipe.VECTOR)


def test_calendar_steps_map_onto_the_documented_pipes() -> None:
    steps = calendar_step_pipes()
    assert [step.step for step in steps] == [
        "Step0 MatMul",
        "Step1 Barrier",
        "Step2 Align",
        "Step3 MTE4 RECV",
        "Step4 MTE3 SEND",
        "Step5 Join",
        "Step6 next",
    ]
    by_name = {step.step: step for step in steps}
    # Step3 must not block anything: that is invariant O3.
    assert by_name["Step3 MTE4 RECV"].blocks == ()
    assert Pipe.MTE3 in by_name["Step4 MTE3 SEND"].requires
    assert set(by_name["Step5 Join"].requires) == {Pipe.MTE3, Pipe.MTE4}
    assert isinstance(steps[0], StepRequirement)


# -- local DRAM ------------------------------------------------------------


def test_local_dram_geometry_matches_the_whitepaper() -> None:
    dram = LocalDram()
    assert dram.capacity_bytes == 384 * 1024 * 1024
    assert round(dram.gib, 0) == 0.0 or dram.gib == 0.375
    assert dram.bandwidth_bytes_per_sec == 1.0e12
    assert dram.page_bytes == LOCAL_DRAM_PAGE_BYTES == 2048
    assert dram.read_seconds(1_000_000_000_000) == 1.0


def test_page_granularity_is_the_real_access_unit() -> None:
    dram = LocalDram()
    assert dram.pages_touched(1) == 1
    assert dram.pages_touched(2048) == 1
    assert dram.pages_touched(2049) == 2
    # An unaligned access can straddle a boundary, which is why the design makes
    # 2 KB blocking a layout rule.
    assert dram.pages_touched(1, aligned=False) == 2


def test_row_padding_costs_bandwidth() -> None:
    dram = LocalDram()
    assert dram.bandwidth_efficiency(rows=4, row_bytes=2048, row_stride_bytes=2048) == 1.0
    assert dram.bandwidth_efficiency(rows=4, row_bytes=1536, row_stride_bytes=2048) == 0.75
    assert dram.pages_for_rows(rows=4, row_bytes=1536, row_stride_bytes=2048) == 4


def test_layout_requires_two_kilobyte_blocking() -> None:
    LocalDramLayout(row_stride_bytes=4096).check()
    with pytest.raises(WseModelError) as excinfo:
        LocalDramLayout(row_stride_bytes=1000).check()
    assert "2 KB" in str(excinfo.value) or "2048" in str(excinfo.value)


def test_the_fill_path_into_l0b_is_open_item_q8() -> None:
    with pytest.raises(OpenItemError) as excinfo:
        LocalDramLayout(row_stride_bytes=2048).require_fill_path()
    assert "Q8" in str(excinfo.value)


def test_expert_capacity_is_the_residency_limit() -> None:
    """Whitepaper §17.3: 384 MB per core is the hard limit on专家 residency."""
    dram = LocalDram()
    assert dram.expert_capacity(expert_bytes=192 * 1024 * 1024) == 2
    assert dram.expert_capacity(expert_bytes=384 * 1024 * 1024) == 1
    with pytest.raises(WseModelError):
        dram.expert_capacity(expert_bytes=0)


# -- buffers ---------------------------------------------------------------


def test_the_buffer_hierarchy_matches_the_whitepaper() -> None:
    assert BUFFER_CAPACITY_BYTES == {
        "L0A": 128 * 1024,
        "L0B": 512 * 1024,
        "L0C": 256 * 1024,
        "L1": 1024 * 1024,
        "UB": 384 * 1024,
    }
    # The design's one asymmetry: L0B is four times L0A so more weights reside.
    assert BufferKind.L0B.capacity_bytes == 4 * BufferKind.L0A.capacity_bytes


def test_double_buffering_doubles_the_footprint() -> None:
    single = TileRequirement("arena", BufferKind.UB, 192 * 1024)
    assert single.fits and single.required_bytes == 192 * 1024
    doubled = TileRequirement("arena", BufferKind.UB, 192 * 1024, double_buffered=True)
    assert doubled.required_bytes == 384 * 1024
    assert doubled.fits
    over = TileRequirement("arena", BufferKind.UB, 384 * 1024, double_buffered=True)
    assert not over.fits
    assert round(over.occupancy, 3) == 2.0


def test_fit_check_reports_every_overflow() -> None:
    residency = fit_check(
        TileRequirement("weights", BufferKind.L0B, 512 * 1024),
        TileRequirement("acc", BufferKind.L0C, 300 * 1024),
    )
    assert not residency.fits
    assert [item.name for item in residency.overflows] == ["acc"]
    assert residency.total_for(BufferKind.L0B) == 512 * 1024
    assert residency.describe()["levels"]["L0C"]["capacity_bytes"] == 256 * 1024


def test_arena_placement_is_open_item_q7() -> None:
    residency = fit_check(TileRequirement("arena", BufferKind.UB, 1024))
    with pytest.raises(OpenItemError) as excinfo:
        residency.arena_placement()
    assert "Q7" in str(excinfo.value)


# -- cube ------------------------------------------------------------------


def test_cube_cycles_for_one_full_fp16_tile() -> None:
    timing = matmul_timing(MatmulShape(16, 16, 16), precision_name="fp16")
    assert timing.cube_cycles == 1
    assert timing.compute_seconds == pytest.approx(1 / 1.4e9)
    assert timing.ideal_weight_bytes == 16 * 16 * 2


def test_cube_cycles_scale_with_each_operand_dimension() -> None:
    assert matmul_timing(MatmulShape(32, 16, 16)).cube_cycles == 2
    assert matmul_timing(MatmulShape(16, 32, 16)).cube_cycles == 2
    assert matmul_timing(MatmulShape(16, 16, 32)).cube_cycles == 2
    # FP8 has a 16x32x32 block: four times the MACs per cycle.
    assert matmul_timing(MatmulShape(16, 16, 16), precision_name="fp8").cube_cycles == 1
    assert matmul_timing(MatmulShape(16, 32, 32), precision_name="fp8").cube_cycles == 1


def test_a_decode_gemv_matches_the_published_first_order_verdict() -> None:
    """Whitepaper §1, §19.2: at batch 1 the balance point says memory-bound."""
    timing = matmul_timing(MatmulShape(1, 4096, 4096), precision_name="fp16")
    assert timing.shape.is_gemv
    assert timing.first_order_intensity == pytest.approx(2.0)
    assert timing.balance_point == pytest.approx(11.469, rel=1e-3)
    assert timing.first_order_is_memory_bound
    assert timing.describe()["first_order_bound_by"] == "memory"


def test_the_tile_accurate_verdict_charges_for_the_m_dimension() -> None:
    """A 16-row Cube block runs 1/16 utilised at batch 1.

    This is where the tile-accurate model and the whitepaper's balance-point
    analysis part company: the balance point assumes the Cube fills its M
    dimension, so at FP16 batch 1 it predicts memory-bound, while charging for the
    idle rows makes compute the longer leg.
    """
    timing = matmul_timing(MatmulShape(1, 4096, 4096), precision_name="fp16")
    assert timing.m_steps == 1
    assert timing.n_steps == 256
    assert timing.k_steps == 256
    assert timing.cube_cycles == 65536
    assert timing.m_utilization == pytest.approx(1 / 16)
    assert timing.effective_macs_per_cycle == pytest.approx(256.0)
    assert not timing.is_memory_bound
    assert timing.roofline_seconds == timing.compute_seconds
    described = timing.describe()
    assert described["verdicts_agree"] is False
    assert described["bound_by"] == "compute"
    assert described["first_order_bound_by"] == "memory"


def test_the_two_verdicts_agree_at_fp8_and_fp4_batch_one() -> None:
    """Lowering precision moves the memory side ahead of the idle-row penalty."""
    for name in ("fp8", "fp4"):
        timing = matmul_timing(MatmulShape(1, 4096, 4096), precision_name=name)
        assert timing.is_memory_bound
        assert timing.first_order_is_memory_bound
        assert timing.describe()["verdicts_agree"] is True


def test_a_filled_m_dimension_reaches_the_published_peak() -> None:
    """At M = 16 the Cube runs full: 4096 MACs/cycle at FP16."""
    timing = matmul_timing(MatmulShape(16, 16, 16), precision_name="fp16")
    assert timing.m_utilization == 1.0
    assert timing.effective_macs_per_cycle == pytest.approx(4096.0)


def test_a_saturated_shape_agrees_with_the_balance_point() -> None:
    """With M at or above the block height both verdicts are compute-bound."""
    timing = matmul_timing(MatmulShape(64, 1024, 1024), precision_name="fp16")
    assert timing.m_utilization == 1.0
    assert not timing.is_memory_bound
    assert not timing.first_order_is_memory_bound
    assert timing.describe()["verdicts_agree"] is True


def test_batching_a_gemv_moves_it_toward_compute_bound() -> None:
    tiny = matmul_timing(MatmulShape(1, 256, 256))
    bigger = matmul_timing(MatmulShape(8, 256, 256))
    assert tiny.first_order_intensity == pytest.approx(2.0)
    assert bigger.first_order_intensity == pytest.approx(16.0)
    assert bigger.first_order_is_memory_bound is False
    assert not bigger.is_memory_bound


def test_fp8_halves_the_weight_bytes_at_the_same_shape() -> None:
    """Whitepaper §19.2: the gain for a memory-bound operator comes from bytes."""
    fp16 = matmul_timing(MatmulShape(1, 4096, 4096), precision_name="fp16")
    fp8 = matmul_timing(MatmulShape(1, 4096, 4096), precision_name="fp8")
    assert fp8.ideal_weight_bytes == fp16.ideal_weight_bytes // 2
    assert fp8.fetch_seconds == pytest.approx(fp16.fetch_seconds / 2)


def test_padded_fetch_bytes_are_accounted() -> None:
    unpadded = matmul_timing(MatmulShape(1, 1024, 1024))
    padded = matmul_timing(MatmulShape(1, 1024, 1024), fetched_weight_bytes=3_000_000)
    assert padded.weight_bytes == 3_000_000
    assert padded.fetch_seconds > unpadded.fetch_seconds
    assert padded.arithmetic_intensity < unpadded.arithmetic_intensity


def test_vector_throughput_is_256_bytes_per_cycle() -> None:
    vector = vector_timing()
    assert vector.bytes_per_cycle == 256
    assert vector.cycles(256) == 1
    assert vector.cycles(257) == 2
    assert vector.elements_per_second(bytes_per_element=2) == pytest.approx(179.2e9)
    with pytest.raises(WseModelError):
        vector.cycles(-1)


def test_invalid_shapes_are_rejected() -> None:
    with pytest.raises(WseModelError):
        MatmulShape(-1, 8, 8)
