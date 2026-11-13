"""Compiler-side model: what the toolchain must emit and self-check.

Whitepaper §10 and Calendar §3.10 define three hard prohibitions on the compiled
product. They are the only consistency anchor left once the design deliberately
removed the "one instruction atomically selects route and timeslot" mechanism, so
the model checks them explicitly rather than trusting the toolchain:

* **F1** — no Calendar static state may land in ``.data`` or ``.bss``. Under SPMD
  a writable global is shared by every core, so 40 cores would race on it.
* **F2** — ``CalendarKeyRef`` constants must not be materialized (never have
  their address taken). Materializing one demotes ``opcode`` from an I-cache
  immediate to a D-cache load.
* **F3** — the ``kCalendarRoute`` table must be in ``.rodata``, 64 B aligned, and
  exactly ``keyCount x nodeCount x 16`` bytes, with no store ever targeting it.
"""

from wse_model.compiler.package import (
    CallSiteImmediate,
    DeploymentPackage,
    build_package,
    check_opcode_domains,
    deployment_artifacts,
)
from wse_model.compiler.selfcheck import (
    CALENDAR_NAMESPACE,
    KEY_REF_NAMES,
    ROUTE_TABLE_SYMBOL,
    ObjectFile,
    Section,
    SectionInfo,
    Symbol,
    SymbolKind,
    check_compiled_product,
    load_object_file,
)

__all__ = [
    "CALENDAR_NAMESPACE",
    "CallSiteImmediate",
    "DeploymentPackage",
    "build_package",
    "check_opcode_domains",
    "deployment_artifacts",
    "KEY_REF_NAMES",
    "ROUTE_TABLE_SYMBOL",
    "ObjectFile",
    "Section",
    "SectionInfo",
    "Symbol",
    "SymbolKind",
    "check_compiled_product",
    "load_object_file",
]
