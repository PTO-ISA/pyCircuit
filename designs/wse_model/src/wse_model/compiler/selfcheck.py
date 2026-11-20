"""The F1/F2/F3 self-checks over a declared compiled product.

The model does not parse ELF. It checks a **declared** symbol and section
manifest, which is what the toolchain already has and what the design's
``llvm-nm`` / ``llvm-readelf`` probes read. Making the manifest explicit has two
benefits: the invariant can be tested without a compiler, and a compiler that
cannot produce the manifest fails the check rather than passing by omission.
"""

from __future__ import annotations

import json
from collections.abc import Iterable
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Any

from wse_model.calendar.entry import ENTRY_SIZE_BYTES
from wse_model.calendar.validate import D_CACHE_LINE_BYTES, ValidationReport
from wse_model.errors import CalendarError

__all__ = [
    "CALENDAR_NAMESPACE",
    "KEY_REF_NAMES",
    "ROUTE_TABLE_ALIGNMENT_BYTES",
    "ROUTE_TABLE_SYMBOL",
    "ObjectFile",
    "Section",
    "SectionInfo",
    "Symbol",
    "SymbolKind",
    "check_compiled_product",
    "load_object_file",
]

#: Names the compiled product must keep in ``.text`` / ``.rodata`` only (F1).
CALENDAR_NAMESPACE = "Calendar"

#: The ``CalendarKeyRef`` constants that must never be materialized (F2).
KEY_REF_NAMES = ("kRowAllGather", "kColAllGather")

#: The emitted route table's symbol name (Calendar §3.10).
ROUTE_TABLE_SYMBOL = "kCalendarRoute"

#: ``.rodata`` must be at least this aligned for the table (prohibition F3).
ROUTE_TABLE_ALIGNMENT_BYTES = 64


class Section(str, Enum):
    """The sections the self-checks care about."""

    TEXT = ".text"
    RODATA = ".rodata"
    DATA = ".data"
    BSS = ".bss"
    OTHER = "other"


class SymbolKind(str, Enum):
    """Symbol kinds, spelled the way ``llvm-nm`` reports them."""

    FUNCTION = "function"  # nm T / t
    READ_ONLY_OBJECT = "read-only object"  # nm R / r
    WRITABLE_OBJECT = "writable object"  # nm D / d
    BSS_OBJECT = "bss object"  # nm B / b
    OTHER = "other"

    @property
    def is_writable(self) -> bool:
        return self in (SymbolKind.WRITABLE_OBJECT, SymbolKind.BSS_OBJECT)

    @property
    def is_read_only(self) -> bool:
        return self is SymbolKind.READ_ONLY_OBJECT

    @property
    def in_rodata_family(self) -> bool:
        return self in (SymbolKind.READ_ONLY_OBJECT, SymbolKind.FUNCTION)


@dataclass(frozen=True)
class Symbol:
    """One declared symbol of the compiled kernel object."""

    name: str
    kind: SymbolKind
    section: Section
    size: int = 0
    addr_align: int = 1
    is_global: bool = True

    def __post_init__(self) -> None:
        if self.size < 0:
            raise CalendarError(f"symbol {self.name}: negative size")
        if self.addr_align < 1:
            raise CalendarError(f"symbol {self.name}: alignment must be positive")

    @property
    def is_calendar(self) -> bool:
        """Whether this symbol belongs to the Calendar namespace.

        Matched case-insensitively: the design spells the core type
        ``CalendarChannel`` and the tables ``kCalendarRoute`` / ``kRowAllGather``,
        but a symbol like ``calendar_epochCtr`` is equally in scope.
        """
        lowered = self.name.lower()
        return "calendar" in lowered or self.name.startswith("kCal")

    def describe(self) -> dict[str, object]:
        return {
            "name": self.name,
            "kind": self.kind.value,
            "section": self.section.value,
            "size": self.size,
            "addr_align": self.addr_align,
            "is_global": self.is_global,
        }


@dataclass(frozen=True)
class SectionInfo:
    """A declared section and its alignment."""

    name: Section
    addr_align: int
    size: int = 0

    def __post_init__(self) -> None:
        if self.addr_align < 1:
            raise CalendarError(f"section {self.name.value}: alignment must be positive")


@dataclass(frozen=True)
class ObjectFile:
    """The declared symbol and section view of a compiled kernel object."""

    symbols: tuple[Symbol, ...] = ()
    sections: tuple[SectionInfo, ...] = ()
    path: str = ""

    def with_symbols(self, *symbols: Symbol) -> ObjectFile:
        return ObjectFile(
            symbols=self.symbols + tuple(symbols), sections=self.sections, path=self.path
        )

    def section(self, name: Section) -> SectionInfo | None:
        for info in self.sections:
            if info.name is name:
                return info
        return None

    def find(self, name: str) -> Symbol | None:
        for symbol in self.symbols:
            if symbol.name == name:
                return symbol
        return None

    def replacing(self, symbol: Symbol) -> ObjectFile:
        """Return a copy with every symbol of that name replaced by ``symbol``."""
        kept = tuple(item for item in self.symbols if item.name != symbol.name)
        return ObjectFile(symbols=kept + (symbol,), sections=self.sections, path=self.path)

    def named(self, names: Iterable[str]) -> tuple[Symbol, ...]:
        wanted = set(names)
        return tuple(symbol for symbol in self.symbols if symbol.name in wanted)

    def describe(self) -> dict[str, object]:
        return {
            "path": self.path,
            "sections": [
                {"name": s.name.value, "addr_align": s.addr_align, "size": s.size}
                for s in self.sections
            ],
            "symbols": [symbol.describe() for symbol in self.symbols],
        }


def _check_f1(object_file: ObjectFile, report: ValidationReport) -> None:
    """No Calendar static state in ``.data`` / ``.bss``."""
    report.ran("F1-no-calendar-static-state")
    offenders = [
        symbol
        for symbol in object_file.symbols
        if symbol.is_calendar
        and (symbol.section in (Section.DATA, Section.BSS) or symbol.kind.is_writable)
    ]
    for symbol in offenders:
        report.error(
            "F1",
            f"{symbol.name} is a writable Calendar symbol in {symbol.section.value} "
            f"({symbol.kind.value}); under SPMD every core shares one global, so "
            "this is a data race that makes epochs and route bits non-deterministic "
            "(Calendar §3.10). CalendarChannel and epochCtr must be kernel-local.",
        )
    if not offenders:
        report.ran("F1-clean")


def _check_f2(object_file: ObjectFile, report: ValidationReport) -> None:
    """``CalendarKeyRef`` constants must not be materialized."""
    report.ran("F2-no-materialized-key-ref")
    offenders = object_file.named(KEY_REF_NAMES)
    for symbol in offenders:
        report.error(
            "F2",
            f"{symbol.name} is materialized as a {symbol.kind.value} in "
            f"{symbol.section.value}; a CalendarKeyRef must be passed by value or "
            "as a non-type template parameter, because taking its address demotes "
            "opcode from an I-cache immediate to a D-cache load (Calendar §3.10)",
        )
    if not offenders:
        report.ran("F2-clean")


def _check_f3(
    object_file: ObjectFile, *, key_count: int, node_count: int, report: ValidationReport
) -> None:
    """The route table's section, alignment, size, and read-only-ness."""
    report.ran("F3-route-table-emission")
    rodata = object_file.section(Section.RODATA)
    if rodata is None:
        report.error("F3", "the object declares no .rodata section")
    elif rodata.addr_align < ROUTE_TABLE_ALIGNMENT_BYTES:
        report.error(
            "F3",
            f".rodata has AddrAlign={rodata.addr_align} but the Calendar table "
            f"requires at least {ROUTE_TABLE_ALIGNMENT_BYTES} B; a 16 B entry "
            "straddling two lines doubles the cold-miss cost (Calendar §3.10)",
        )

    table = object_file.find(ROUTE_TABLE_SYMBOL)
    if table is None:
        report.error("F3", f"the object declares no {ROUTE_TABLE_SYMBOL} symbol")
        return

    if not table.kind.is_read_only or table.section is not Section.RODATA:
        report.error(
            "F3",
            f"{ROUTE_TABLE_SYMBOL} is a {table.kind.value} in "
            f"{table.section.value}; it must be a read-only object in .rodata so "
            "that it is never dirtied and the tail DCCI needs no clean half "
            "(Calendar §3.10)",
        )

    if table.addr_align < ROUTE_TABLE_ALIGNMENT_BYTES:
        report.error(
            "F3",
            f"{ROUTE_TABLE_SYMBOL} has alignment {table.addr_align}, below the "
            f"required {ROUTE_TABLE_ALIGNMENT_BYTES} B",
        )

    expected = key_count * node_count * ENTRY_SIZE_BYTES
    if table.size != expected:
        report.error(
            "F3",
            f"{ROUTE_TABLE_SYMBOL} is {table.size} B but keyCount={key_count} x "
            f"nodeCount={node_count} x {ENTRY_SIZE_BYTES} B = {expected} B",
        )
    if expected % D_CACHE_LINE_BYTES != 0:
        report.warn(
            "F3-PADDING",
            f"the table is {expected} B before padding, so the emitter must pad "
            f"the segment to {D_CACHE_LINE_BYTES} B (Calendar §2.7.3 step 3)",
        )


def check_compiled_product(
    object_file: ObjectFile, *, key_count: int, node_count: int
) -> ValidationReport:
    """Run F1, F2, and F3 over a declared compiled product."""
    report = ValidationReport()
    if key_count < 0 or node_count < 0:
        raise CalendarError("keyCount and nodeCount must be non-negative")
    _check_f1(object_file, report)
    _check_f2(object_file, report)
    _check_f3(
        object_file,
        key_count=key_count,
        node_count=node_count,
        report=report,
    )
    return report


_SCHEMA = "wse-model/kernel-object/1"


def load_object_file(source: str | Path | dict[str, Any]) -> ObjectFile:
    """Load a declared object manifest from JSON text, a path, or a mapping."""
    if isinstance(source, dict):
        payload = source
        label = "<mapping>"
    else:
        path = Path(source)
        if path.is_file():
            payload = json.loads(path.read_text(encoding="utf-8"))
            label = str(path)
        else:
            payload = json.loads(str(source))
            label = "<inline>"

    schema = payload.get("schema")
    if schema != _SCHEMA:
        raise CalendarError(f"unsupported kernel-object schema {schema!r}; expected {_SCHEMA!r}")

    sections = tuple(
        SectionInfo(
            name=Section(entry.get("name", Section.OTHER.value)),
            addr_align=int(entry.get("addr_align", 1)),
            size=int(entry.get("size", 0)),
        )
        for entry in payload.get("sections", ())
    )
    symbols = tuple(
        Symbol(
            name=str(entry["name"]),
            kind=SymbolKind(entry.get("kind", SymbolKind.OTHER.value)),
            section=Section(entry.get("section", Section.OTHER.value)),
            size=int(entry.get("size", 0)),
            addr_align=int(entry.get("addr_align", 1)),
            is_global=bool(entry.get("is_global", True)),
        )
        for entry in payload.get("symbols", ())
    )
    return ObjectFile(symbols=symbols, sections=sections, path=label)
