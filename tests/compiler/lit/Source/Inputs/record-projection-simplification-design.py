"""Immutable record snapshots, nested writes and explicit source-kind boundaries."""
import pycircuit as ac


@ac.struct
class Inner:
    payload: ac.u7
    tag: ac.u3


@ac.struct
class Envelope:
    prefix: ac.u5
    inner: Inner
    suffix: ac.u11


@ac.struct
class Flag:
    bit: ac.u1


@ac.struct
class Result:
    initial: Envelope
    first: Envelope
    second: Envelope
    final: Envelope
    fixed: ac.u1
    arithmetic: ac.u1
    helper: ac.u1


@ac.rule
def snapshots(a, b, c, d, e, f, g, h, flag) -> Result:
    original_alias = flag  # noqa: F841  # Retained for original-Boolean negative controls.
    converted: ac.u1 = flag
    box = Flag(bit=converted)
    local = Envelope(prefix=a, inner=Inner(payload=b, tag=c), suffix=d)
    initial = local
    local.inner.payload = e
    first = local
    local.inner.payload = b
    local.inner.tag = f
    second = local
    local.prefix = g
    local.suffix = h
    local.inner.payload = e
    return Result(initial=initial, first=first, second=second, final=local,
                  fixed=box.bit, arithmetic=box.bit + 1,
                  helper=ac.popcount(box.bit))


@ac.module
def Top(a: ac.u5, b: ac.u7, c: ac.u3, d: ac.u11,  # noqa: N802  # Hardware module naming.
        e: ac.u7, f: ac.u3, g: ac.u5, h: ac.u11, flag: bool) -> Result:
    return snapshots(a, b, c, d, e, f, g, h, flag)
