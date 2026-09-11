"""Agentic Circuit's portable Python construction surface."""

from __future__ import annotations

from pkgutil import extend_path

__path__ = extend_path(__path__, __name__)

from . import _types as _types_module
from . import markers as markers
from ._definitions import (
    extern_module,
    interface,
    inline,
    invariant,
    module,
    packet,
    process,
    protocol,
    rule,
    struct,
    system,
    transaction,
    writer_priority,
)
from ._jit import config, jit
from ._resources import address_map, address_space, queue
from ._types import (
    BitfieldSpec,
    Endpoint,
    Flow,
    ResourceRef,
    Static,
    array,
    bits,
    const,
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
    find,
    fork,
    insert,
    instances,
    map,
    matches,
    memory,
    merge,
    observe,
    pipeline,
    popcount,
    priority_encode,
    reorder,
    route,
    schedule,
    scope,
    set,
    sink,
    slot,
    source,
    table,
    view,
)

_UNSIGNED_NAMES = tuple(f"u{width}" for width in _types_module.UNSIGNED_WIDTHS)
for _name in _UNSIGNED_NAMES:
    globals()[_name] = getattr(_types_module, _name)
del _name


CAPTURE_ONLY_API = markers.CAPTURE_ONLY_API

RUNTIME_API = (
    "system",
    "module",
    "extern_module",
    "struct",
    "packet",
    "transaction",
    "protocol",
    "interface",
    "process",
    "rule",
    "invariant",
    "inline",
    "writer_priority",
    "array",
    "bits",
    "BitfieldSpec",
    "queue",
    "ResourceRef",
    "address_space",
    "address_map",
    "Static",
    "Flow",
    "Endpoint",
    "config",
    "const",
    "jit",
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

__all__ = RUNTIME_API
