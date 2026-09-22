"""Agentic Circuit's portable Python construction surface."""

from __future__ import annotations

from pkgutil import extend_path

__path__ = extend_path(__path__, __name__)

from . import _api_inventory
from . import _types as _types_module
from . import markers as markers
from ._definitions import (
    extern_module,
    interface,
    inline,
    invariant,
    module,
    module_decl,
    packet,
    process,
    protocol,
    rule,
    struct,
    system,
    transaction,
    writer_priority,
)
from ._families import (
    case,
    config,
    integer_range,
    one_of,
    static_bool,
    static_config,
    static_enum,
    static_int,
    static_parameter,
)
from ._resources import address_map, address_space, queue
from ._types import (
    BitfieldSpec,
    count_width,
    Endpoint,
    Flow,
    index_width,
    index,
    range,
    ResourceRef,
    Queue,
    Static,
    array,
    bits,
    const,
    encoding,
    param,
    s8,
    s16,
    s32,
    s64,
)
from .markers import (
    barrier,
    compute,
    concat,
    count_leading_zeros,
    count_trailing_zeros,
    engine,
    expect,
    fork,
    insert,
    map,
    match_enum,
    matches,
    memory,
    merge,
    observe,
    onehot_encode,
    onehot_enum,
    literal,
    pipeline,
    popcount,
    priority_encode,
    reorder,
    route,
    schedule,
    scope,
    sext,
    set,
    sink,
    slot,
    source,
    table,
    static_assert,
    truncate,
    wrap,
    saturate,
    checked,
    refine,
    view,
    zero,
    zext,
)

_UNSIGNED_NAMES = tuple(f"u{width}" for width in _types_module.UNSIGNED_WIDTHS)
for _name in _UNSIGNED_NAMES:
    globals()[_name] = getattr(_types_module, _name)
del _name


CAPTURE_ONLY_API = markers.CAPTURE_ONLY_API

# Reserved declaration names: the package accepts the spelling so authored
# source can import it, but the ACPy queue frontend has no implementation for
# it.  These names stay reachable as explicit attributes (``ac.packet``) and
# are deliberately absent from both ``RUNTIME_API`` and ``__all__``.
RESERVED_API = _api_inventory.RESERVED_API

RUNTIME_API = (
    "system",
    "module",
    "module_decl",
    "struct",
    "rule",
    "invariant",
    "inline",
    "writer_priority",
    "array",
    "bits",
    "BitfieldSpec",
    "queue",
    "ResourceRef",
    "Queue",
    "address_space",
    "address_map",
    "Static",
    "Flow",
    "Endpoint",
    "config",
    "const",
    "encoding",
    "param",
    "index_width",
    "index",
    "range",
    "count_width",
    "static_bool",
    "static_int",
    "static_enum",
    "static_config",
    "static_parameter",
    "one_of",
    "integer_range",
    "case",
    "round_robin",
    "priority",
    *_UNSIGNED_NAMES,
    "s8",
    "s16",
    "s32",
    "s64",
)

round_robin = "round_robin"
priority = "priority"

# ``__all__`` is the wildcard-import surface.  A wildcard import MUST NOT bind
# a Python builtin, so a name that would shadow one stays reachable only as an
# explicit attribute (``ac.range``) and never appears here.
_BUILTIN_SHADOWING_API = ("range",)

__all__ = tuple(
    name for name in RUNTIME_API if name not in _BUILTIN_SHADOWING_API
)
