"""The public model surface is pinned.

Like the sibling PTO-ISA repositories, this test freezes each public namespace's
``__all__`` so that a removal or a rename is a deliberate, visible change rather
than an accident. Adding a name requires updating this file, which is the point.

The sets below are the model's contract with its consumers: the CLI, the
examples, the contract tests, and anything downstream that imports
``wse_model``.
"""

from __future__ import annotations

import importlib

import pytest

import wse_model

pytestmark = pytest.mark.contract

#: Every public namespace and the exact set of names it exports.
EXPECTED_EXPORTS: dict[str, frozenset[str]] = {
    "wse_model": frozenset(
        {
            "CALENDAR_BASELINE",
            "WHITEPAPER_HARDWARE",
            "CalendarError",
            "ConservationError",
            "IllegalBitPairError",
            "LandSetError",
            "MeshTopology",
            "OpenItemError",
            "RouteTreeError",
            "TopologyError",
            "TopologyProfile",
            "VersionMismatchError",
            "WseModelError",
            "__version__",
            "topology_profile",
        }
    ),
    "wse_model.calendar": frozenset(
        {
            "ALIGNMENT_SEMAPHORE_WIDTH_BITS",
            "ARENA_SEGMENT_ALIGNMENT_BYTES",
            "AlignmentDomain",
            "ArenaSegment",
            "BitPair",
            "CalendarKeyRef",
            "CalendarKeyRegistry",
            "CalendarRouteEntry",
            "CalendarRouteTable",
            "Collective",
            "CoreIngress",
            "D_CACHE_LINE_BYTES",
            "DeliveryOutcome",
            "Diagnostic",
            "ENTRY_LAYOUT_OVERHEAD_BYTES",
            "ENTRY_SIZE_BYTES",
            "EPOCH_TAG_MAX_BITS",
            "EPOCH_TAG_MIN_BITS",
            "EpochCounter",
            "EpochTracker",
            "GroupRole",
            "KeyDefinition",
            "MAX_KEY_COUNT",
            "OPCODE_RESERVED",
            "OPCODE_WIDTH_BITS",
            "PayloadGeometry",
            "RED_OP_WIDTH_BITS",
            "ROUTE_BITS_NODE_CAPACITY",
            "ReceiveAccount",
            "RecvContext",
            "RedOp",
            "ReductionElementType",
            "RouteBits",
            "RouteEntryFlags",
            "RouteKey",
            "CalendarRouteRegs",
            "Segment",
            "SegmentId",
            "Severity",
            "SymmetricArena",
            "TableLayout",
            "TreeAnalysis",
            "ValidationReport",
            "build_table",
            "check_induced_tree",
            "entry_layout_size_bytes",
            "rows_from_bitmaps",
            "validate_entry_layout",
            "validate_route_bits",
            "validate_table_budget",
        }
    ),
    "wse_model.noc": frozenset(
        {
            "CalRegBank",
            "CalRegImage",
            "CalRegSlot",
            "DEFAULT_EPOCH_TAG_BITS",
            "DEFAULT_LINK_WIDTH_BYTES",
            "Flit",
            "FlitHeader",
            "Hop",
            "Landing",
            "MulticastTrace",
            "Noc",
            "SendOutcome",
            "SendPlan",
            "StopTheWorldWindow",
            "delivery_order",
            "flit_count",
            "link_load",
            "simulate_multicast",
            "split_payload",
        }
    ),
    "wse_model.core": frozenset(
        {
            "BUFFER_CAPACITY_BYTES",
            "BufferKind",
            "BufferResidency",
            "INDEPENDENT_FROM_MTE4",
            "LocalDram",
            "LocalDramLayout",
            "MatmulShape",
            "MatmulTiming",
            "PIPE_ORDER_NOTE",
            "Pipe",
            "PipeBarrier",
            "PipeTracker",
            "StepRequirement",
            "TileRequirement",
            "VectorTiming",
            "calendar_step_pipes",
            "fit_check",
            "matmul_timing",
            "vector_timing",
        }
    ),
    "wse_model.host": frozenset(
        {
            "FABRIC_LANES",
            "HOST_LANES",
            "TOTAL_LANES",
            "BatcherMem",
            "BatcherResponsibility",
            "BatcherSpec",
            "DispatchChain",
            "DispatchPath",
            "InstallState",
            "LaunchConstraints",
            "Loader",
            "RefillAccount",
            "RuntimeTier",
            "Scheduler",
            "UbBus",
            "UbLane",
            "VersionSet",
            "dispatch_chains",
            "dispatch_paths",
        }
    ),
    "wse_model.analysis": frozenset(
        {
            "AICORE_CLOCK_HZ",
            "BANDWIDTHS",
            "LOCAL_DRAM_BYTES",
            "LOCAL_DRAM_BYTES_PER_SEC",
            "NOC_LINK_BYTES_PER_SEC",
            "NOC_LINK_WIDTH_BYTES",
            "ON_CHIP_BUFFERS",
            "PRECISIONS",
            "AicoreSpec",
            "Bandwidth",
            "DCacheSpec",
            "LatencyBlock",
            "LatencyBudget",
            "PrecisionSpec",
            "RooflinePoint",
            "RouteTableResidency",
            "arithmetic_intensity_decode",
            "balance_point",
            "collective_transfer_seconds",
            "dcache_budget",
            "flit_header_cost",
            "link_time_seconds",
            "local_dram_vs_noc_ratio",
            "min_batch_to_escape_memory_bound",
            "precision",
            "roofline",
            "roofline_table",
            "two_phase_allgather_bytes",
        }
    ),
    "wse_model.compiler": frozenset(
        {
            "CALENDAR_NAMESPACE",
            "KEY_REF_NAMES",
            "ROUTE_TABLE_SYMBOL",
            "CallSiteImmediate",
            "DeploymentPackage",
            "ObjectFile",
            "Section",
            "SectionInfo",
            "Symbol",
            "SymbolKind",
            "build_package",
            "check_compiled_product",
            "check_opcode_domains",
            "deployment_artifacts",
            "load_object_file",
        }
    ),
}


@pytest.mark.parametrize("module_name", sorted(EXPECTED_EXPORTS))
def test_public_all_matches_the_pinned_set(module_name: str) -> None:
    module = importlib.import_module(module_name)
    exported = frozenset(module.__all__)
    expected = EXPECTED_EXPORTS[module_name]
    assert exported == expected, (
        f"{module_name}.__all__ drifted:\n"
        f"  missing: {sorted(expected - exported)}\n"
        f"  added:   {sorted(exported - expected)}"
    )


@pytest.mark.parametrize("module_name", sorted(EXPECTED_EXPORTS))
def test_every_exported_name_exists(module_name: str) -> None:
    module = importlib.import_module(module_name)
    missing = [name for name in module.__all__ if not hasattr(module, name)]
    assert not missing, f"{module_name} exports names it does not define: {missing}"


def test_the_version_is_a_single_source_of_truth() -> None:
    from wse_model.version import __version__

    assert wse_model.__version__ == __version__


def test_the_top_level_namespace_stays_small() -> None:
    """Subpackages are imported explicitly, not flattened into the root."""
    assert len(wse_model.__all__) <= 20
    for name in ("RouteBits", "MatmulShape", "UbBus", "Noc"):
        assert name not in wse_model.__all__


def test_subpackages_are_importable_without_the_acir_toolchain() -> None:
    """The semantic core must never depend on the agentic_circuit frontend."""
    for module_name in EXPECTED_EXPORTS:
        importlib.import_module(module_name)
