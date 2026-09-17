"""Capture-time-only syntax markers for the Agentic Circuit frontend.

These objects exist so authored source can be imported and inspected by the
ACPy frontend.  They intentionally do not manufacture runtime stand-ins.
"""

from __future__ import annotations

from typing import Never

CAPTURE_ONLY_API = (
    "scope",
    "map",
    "set",
    "instances",
    "view",
    "concat",
    "literal",
    "zero",
    "zext",
    "sext",
    "truncate",
    "wrap",
    "saturate",
    "checked",
    "refine",
    "static_assert",
    "insert",
    "matches",
    "source",
    "popcount",
    "udiv",
    "sdiv",
    "urem",
    "srem",
    "divrem",
    "addw",
    "subw",
    "andw",
    "orw",
    "xorw",
    "sll",
    "srl",
    "sra",
    "sllw",
    "srlw",
    "sraw",
    "smin",
    "umin",
    "smax",
    "umax",
    "mulw",
    "madd",
    "maddw",
    "msub",
    "bitfield_extract",
    "bitfield_popcount",
    "bitfield_clz",
    "bitfield_ctz",
    "bitfield_clear",
    "bitfield_set",
    "bitfield_reverse_bytes",
    "bitfield_insert",
    "sext_low",
    "zext_low",
    "csel",
    "count_leading_zeros",
    "count_trailing_zeros",
    "priority_encode",
    "onehot_encode",
    "onehot_enum",
    "match_enum",
    "memory",
    "sink",
    "observe",
    "expect",
    "compute",
    "pipeline",
    "route",
    "merge",
    "schedule",
    "engine",
    "reorder",
    "fork",
    "barrier",
    "table",
    "slot",
)

__all__ = CAPTURE_ONLY_API


def _capture_time_only(marker: str) -> Never:
    raise NotImplementedError(
        f"{marker} is capture-time only: the ACPy frontend interprets it "
        "from source; ordinary Python execution cannot produce a runtime value"
    )


def scope(name: str) -> Never:
    return _capture_time_only("scope")


def map(*values: object) -> Never:
    return _capture_time_only("map")


def set(*values: object) -> Never:
    return _capture_time_only("set")


def instances(*values: object) -> Never:
    return _capture_time_only("instances")


def view(value: object, *selectors: object) -> Never:
    return _capture_time_only("view")


def concat(*values: object) -> Never:
    return _capture_time_only("concat")


def literal(value: int, value_type: object) -> Never:
    _ = (value, value_type)
    return _capture_time_only("literal")


def zero(value_type: object) -> Never:
    _ = value_type
    return _capture_time_only("zero")


def zext(value: object, value_type: object) -> Never:
    _ = (value, value_type)
    return _capture_time_only("zext")


def sext(value: object, value_type: object) -> Never:
    _ = (value, value_type)
    return _capture_time_only("sext")


def truncate(value: object, value_type: object) -> Never:
    _ = (value, value_type)
    return _capture_time_only("truncate")


def wrap(value: object, value_type: object) -> Never:
    _ = (value, value_type)
    return _capture_time_only("wrap")


def saturate(value: object, value_type: object) -> Never:
    _ = (value, value_type)
    return _capture_time_only("saturate")


def checked(
    value: object, value_type: object, *, fallback: object | None = None
) -> Never:
    _ = (value, value_type, fallback)
    return _capture_time_only("checked")


def refine(value: object, value_type: object) -> Never:
    _ = (value, value_type)
    return _capture_time_only("refine")


def static_assert(condition: bool, message: str = "static assertion failed") -> None:
    _ = (condition, message)
    _capture_time_only("static_assert")


def insert(value: object, field: object, *, lsb: int) -> Never:
    _ = (value, field, lsb)
    return _capture_time_only("insert")


def matches(value: object, pattern: str) -> Never:
    _ = (value, pattern)
    return _capture_time_only("matches")


def source(
    payload: object,
    *,
    depth: int = 1,
    latency: int = 1,
    rate: int = 1,
    lanes: int = 1,
) -> Never:
    _ = (payload, depth, latency, rate, lanes)
    return _capture_time_only("source")


def popcount(value: object) -> Never:
    return _capture_time_only("popcount")


def _binary_alu(marker: str, left: object, right: object) -> Never:
    _ = (left, right)
    return _capture_time_only(marker)


def udiv(left: object, right: object) -> Never:
    return _binary_alu("udiv", left, right)


def sdiv(left: object, right: object) -> Never:
    return _binary_alu("sdiv", left, right)


def urem(left: object, right: object) -> Never:
    return _binary_alu("urem", left, right)


def srem(left: object, right: object) -> Never:
    return _binary_alu("srem", left, right)


def divrem(left: object, right: object, *, signed: object, word: object) -> Never:
    _ = (left, right, signed, word)
    return _capture_time_only("divrem")


def addw(left: object, right: object) -> Never:
    return _binary_alu("addw", left, right)


def subw(left: object, right: object) -> Never:
    return _binary_alu("subw", left, right)


def andw(left: object, right: object) -> Never:
    return _binary_alu("andw", left, right)


def orw(left: object, right: object) -> Never:
    return _binary_alu("orw", left, right)


def xorw(left: object, right: object) -> Never:
    return _binary_alu("xorw", left, right)


def sll(left: object, right: object) -> Never:
    return _binary_alu("sll", left, right)


def srl(left: object, right: object) -> Never:
    return _binary_alu("srl", left, right)


def sra(left: object, right: object) -> Never:
    return _binary_alu("sra", left, right)


def sllw(left: object, right: object) -> Never:
    return _binary_alu("sllw", left, right)


def srlw(left: object, right: object) -> Never:
    return _binary_alu("srlw", left, right)


def sraw(left: object, right: object) -> Never:
    return _binary_alu("sraw", left, right)


def smin(left: object, right: object) -> Never:
    return _binary_alu("smin", left, right)


def umin(left: object, right: object) -> Never:
    return _binary_alu("umin", left, right)


def smax(left: object, right: object) -> Never:
    return _binary_alu("smax", left, right)


def umax(left: object, right: object) -> Never:
    return _binary_alu("umax", left, right)


def mulw(left: object, right: object) -> Never:
    return _binary_alu("mulw", left, right)


def _ternary_alu(marker: str, left: object, right: object, auxiliary: object) -> Never:
    _ = (left, right, auxiliary)
    return _capture_time_only(marker)


def madd(left: object, right: object, auxiliary: object) -> Never:
    return _ternary_alu("madd", left, right, auxiliary)


def maddw(left: object, right: object, auxiliary: object) -> Never:
    return _ternary_alu("maddw", left, right, auxiliary)


def msub(left: object, right: object, auxiliary: object) -> Never:
    return _ternary_alu("msub", left, right, auxiliary)


def bitfield_extract(
    value: object, width: object, offset: object, *, signed: bool = False
) -> Never:
    _ = (value, width, offset, signed)
    return _capture_time_only("bitfield_extract")


def _bitfield_alu(marker: str, value: object, width: object, offset: object) -> Never:
    _ = (value, width, offset)
    return _capture_time_only(marker)


def bitfield_popcount(value: object, width: object, offset: object) -> Never:
    return _bitfield_alu("bitfield_popcount", value, width, offset)


def bitfield_clz(value: object, width: object, offset: object) -> Never:
    return _bitfield_alu("bitfield_clz", value, width, offset)


def bitfield_ctz(value: object, width: object, offset: object) -> Never:
    return _bitfield_alu("bitfield_ctz", value, width, offset)


def bitfield_clear(value: object, width: object, offset: object) -> Never:
    return _bitfield_alu("bitfield_clear", value, width, offset)


def bitfield_set(value: object, width: object, offset: object) -> Never:
    return _bitfield_alu("bitfield_set", value, width, offset)


def bitfield_reverse_bytes(value: object, width: object, offset: object) -> Never:
    return _bitfield_alu("bitfield_reverse_bytes", value, width, offset)


def bitfield_insert(
    value: object, source: object, width: object, offset: object
) -> Never:
    _ = (value, source, width, offset)
    return _capture_time_only("bitfield_insert")


def sext_low(value: object, width: object) -> Never:
    _ = (value, width)
    return _capture_time_only("sext_low")


def zext_low(value: object, width: object) -> Never:
    _ = (value, width)
    return _capture_time_only("zext_low")


def csel(
    predicate: object,
    lhs: object,
    rhs: object,
    *,
    negate_false: bool = False,
) -> Never:
    _ = (predicate, lhs, rhs, negate_false)
    return _capture_time_only("csel")


def count_leading_zeros(value: object) -> Never:
    return _capture_time_only("count_leading_zeros")


def count_trailing_zeros(value: object) -> Never:
    return _capture_time_only("count_trailing_zeros")


def priority_encode(value: object, *, order: str = "low") -> Never:
    _ = (value, order)
    return _capture_time_only("priority_encode")


def onehot_encode(value: object, *, order: str = "low") -> Never:
    _ = (value, order)
    return _capture_time_only("onehot_encode")


def onehot_enum(
    value: object,
    *,
    members: object,
    empty: object,
    conflict: object,
) -> Never:
    _ = (value, members, empty, conflict)
    return _capture_time_only("onehot_enum")


def match_enum(value: object, cases: object, *, invalid: object) -> Never:
    _ = (value, cases, invalid)
    return _capture_time_only("match_enum")


def memory(
    data_type: object, *, entries: int = 16, init: int = 0, latency: int = 1
) -> Never:
    _ = (data_type, entries, init, latency)
    return _capture_time_only("memory")


def sink(value: object) -> Never:
    return _capture_time_only("sink")


def observe(value: object) -> Never:
    return _capture_time_only("observe")


def expect(value: object, *, predicate: object, message: str) -> Never:
    _ = (value, predicate, message)
    return _capture_time_only("expect")


def compute(
    value: object,
    function: object,
    *,
    depth: int = 1,
    latency: int = 1,
    rate: int = 1,
) -> Never:
    _ = (value, function, depth, latency, rate)
    return _capture_time_only("compute")


def pipeline(
    value: object,
    *,
    stages: int = 1,
    depth: int = 1,
    rate: int = 1,
) -> Never:
    _ = (value, stages, depth, rate)
    return _capture_time_only("pipeline")


priority = "priority"


def route(
    value: object,
    *,
    by: object,
    outputs: int,
    depth: int = 1,
    latency: int = 1,
) -> Never:
    _ = (value, by, outputs, depth, latency)
    return _capture_time_only("route")


def merge(
    *values: object,
    policy: object = priority,
    depth: int = 1,
    latency: int = 1,
) -> Never:
    _ = (values, policy, depth, latency)
    return _capture_time_only("merge")


def schedule(
    value: object,
    *,
    by: object,
    waits_for: object,
    resource: object,
    cost: object,
    no_dependency: int,
    entries: int = 16,
    resources: int = 1,
    depth: int = 1,
    latency: int = 1,
) -> Never:
    _ = (
        value,
        by,
        waits_for,
        resource,
        cost,
        no_dependency,
        entries,
        resources,
        depth,
        latency,
    )
    return _capture_time_only("schedule")


def engine(
    value: object,
    *,
    cost: object,
    lanes: int = 1,
    depth: int = 1,
    latency: int = 1,
) -> Never:
    _ = (value, cost, lanes, depth, latency)
    return _capture_time_only("engine")


def reorder(
    value: object,
    *,
    by: object,
    entries: int = 16,
    start: int = 0,
    depth: int = 1,
    latency: int = 1,
) -> Never:
    _ = (value, by, entries, start, depth, latency)
    return _capture_time_only("reorder")


def fork(
    value: object,
    *,
    outputs: int,
    depth: int = 1,
    latency: int = 1,
) -> Never:
    _ = (value, outputs, depth, latency)
    return _capture_time_only("fork")


def barrier(
    *values: object,
    depth: int = 1,
    latency: int = 1,
) -> Never:
    _ = (values, depth, latency)
    return _capture_time_only("barrier")


class _TableDeclaration:
    def __init__(self, entries: object, entry_type: object) -> None:
        self.entries = entries
        self.entry_type = entry_type

    def __call__(self, *, init: object = 0) -> Never:
        _ = init
        return _capture_time_only("table")


class _TableFactory:
    def __getitem__(self, parameters: object) -> _TableDeclaration:
        if not isinstance(parameters, tuple) or len(parameters) != 2:
            raise TypeError("ac.table requires ac.table[entries, Entry]")
        return _TableDeclaration(parameters[0], parameters[1])

    def __call__(self, *args: object, **kwargs: object) -> Never:
        del args, kwargs
        raise TypeError(
            "ac.table(value, ...) was removed; use ac.memory for "
            "request/response memory or ac.table[entries, Entry](init=0) "
            "for state Table"
        )


table = _TableFactory()


def slot(value: object) -> Never:
    return _capture_time_only("slot")
