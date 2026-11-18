"""WSE semantic core; current pyCircuit hardware models live in models/."""

from wse_model.errors import (
    CalendarError,
    ConservationError,
    IllegalBitPairError,
    LandSetError,
    OpenItemError,
    RouteTreeError,
    TopologyError,
    VersionMismatchError,
    WseModelError,
)
from wse_model.topology import (
    CALENDAR_BASELINE,
    WHITEPAPER_HARDWARE,
    MeshTopology,
    TopologyProfile,
    topology_profile,
)
from wse_model.version import __version__

__all__ = [
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
]
