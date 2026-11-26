"""Calendar encoding, keys, epochs, geometry, validation, and the route table.

The public surface of this package is the semantic core of the Calendar scheme:
the values it defines, the rules it enforces, and the artifacts it emits. Nothing
here schedules; the NoC algorithm owns timing (Calendar §2.7.1).
"""

from wse_model.calendar.collective import (
    ALIGNMENT_SEMAPHORE_WIDTH_BITS,
    OPCODE_RESERVED,
    OPCODE_WIDTH_BITS,
    RED_OP_WIDTH_BITS,
    AlignmentDomain,
    Collective,
    RedOp,
    ReductionElementType,
)
from wse_model.calendar.entry import (
    ENTRY_LAYOUT_OVERHEAD_BYTES,
    ENTRY_SIZE_BYTES,
    ROUTE_BITS_NODE_CAPACITY,
    CalendarRouteEntry,
    CalendarRouteRegs,
    RouteEntryFlags,
    entry_layout_size_bytes,
)
from wse_model.calendar.epoch import (
    EPOCH_TAG_MAX_BITS,
    EPOCH_TAG_MIN_BITS,
    EpochCounter,
    EpochTracker,
    RecvContext,
)
from wse_model.calendar.geometry import (
    ARENA_SEGMENT_ALIGNMENT_BYTES,
    ArenaSegment,
    PayloadGeometry,
    SymmetricArena,
)
from wse_model.calendar.key import (
    CalendarKeyRef,
    CalendarKeyRegistry,
    GroupRole,
    RouteKey,
)
from wse_model.calendar.receive import (
    CoreIngress,
    DeliveryOutcome,
    ReceiveAccount,
    Segment,
    SegmentId,
)
from wse_model.calendar.route_bits import BitPair, RouteBits
from wse_model.calendar.table import (
    CalendarRouteTable,
    KeyDefinition,
    TableLayout,
    build_table,
    rows_from_bitmaps,
)
from wse_model.calendar.validate import (
    D_CACHE_LINE_BYTES,
    MAX_KEY_COUNT,
    Diagnostic,
    Severity,
    TreeAnalysis,
    ValidationReport,
    check_induced_tree,
    validate_entry_layout,
    validate_route_bits,
    validate_table_budget,
)

__all__ = [
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
]
