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
    "find",
    "concat",
    "insert",
    "matches",
    "source",
    "popcount",
    "count_leading_zeros",
    "count_trailing_zeros",
    "priority_encode",
    "onehot_encode",
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


def find(values: object, *, where: object, key: object | None = None) -> Never:
    _ = (values, where, key)
    return _capture_time_only("find")


def concat(*values: object) -> Never:
    return _capture_time_only("concat")


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
