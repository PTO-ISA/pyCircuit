from pathlib import Path

import pytest
from pycircuit.api_contract import scan_text

pytestmark = pytest.mark.unit


def _codes(source: str) -> list[str]:
    return [
        diagnostic.code
        for diagnostic in scan_text(path=Path("fixture.py"), text=source)
    ]


def test_cycle_aware_method_surface_passes_api_hygiene() -> None:
    source = """
from pycircuit import CycleAwareSignal, cas

def build(value: CycleAwareSignal, cond: CycleAwareSignal | None):
    value.select(value, value)
    value.trunc(width=4)
    value.zext(width=16)
    value.sext(width=16)
    value.as_unsigned()
    cas(domain, source).trunc(width=4)
"""

    assert _codes(source) == []

    aliased = """
import pycircuit as pc
from pycircuit import CycleAwareSignal as CAS

def build(value: CAS):
    pc.cas(domain, source).trunc(width=4)
    value.as_unsigned()
"""
    assert _codes(aliased) == []

    fake_prefix = """
import pycircuit_fake as pc

def build():
    pc.cas(source).trunc(width=4)
"""
    assert _codes(fake_prefix) == ["PYC415"]

    qualified = """
import pycircuit.v6

def build(value: pycircuit.v6.CycleAwareSignal):
    from pycircuit import cas as local_cas
    selected = value if flag else local_cas(domain, source)
    selected.trunc(width=4)
"""
    assert _codes(qualified) == []


def test_named_comparison_methods_are_rejected_for_every_receiver_kind() -> None:
    source = """
from pycircuit import CycleAwareSignal, Wire

def build(cas_value: CycleAwareSignal, wire_value: Wire, unknown):
    cas_value.eq(cas_value)
    cas_value.lt(cas_value)
    wire_value.eq(wire_value)
    unknown.lt(unknown)
"""

    assert _codes(source) == ["PYC415", "PYC415", "PYC415", "PYC415"]


def test_explicit_wire_method_surface_fails_api_hygiene() -> None:
    source = """
from pycircuit import Wire

def build(value: Wire, cond: Wire):
    cond.select(value, value)
    value.trunc(width=4)
    value.zext(width=16)
    value.sext(width=16)
    value.as_unsigned()
"""

    assert _codes(source) == ["PYC415", "PYC415", "PYC415", "PYC415", "PYC418"]


def test_api_hygiene_does_not_guess_untyped_receiver_kinds() -> None:
    source = """
from pycircuit import Circuit, CycleAwareDomain, cas

def build(m: Circuit, domain: CycleAwareDomain):
    wire_value = cas(domain, m.input("value", width=8))
    cas_value = m.input("raw", width=8)
    wire_value.trunc(width=4)
    cas_value.trunc(width=4)
    unknown.select(lhs, rhs)
"""

    assert _codes(source) == ["PYC415", "PYC415"]


def test_api_hygiene_rejects_alias_and_control_flow_ambiguity() -> None:
    source = """
from other import cas
from pycircuit import Circuit, CycleAwareDomain, cas as pyc_cas

def build(m: Circuit, domain: CycleAwareDomain, flag):
    external = cas(source)
    external.trunc(width=4)
    value = m.input("value", width=8)
    if flag:
        value = pyc_cas(domain, value)
    value.trunc(width=4)
    method = m.input("other", width=8).trunc
    reflected = getattr(value, "sext")
"""

    assert _codes(source) == ["PYC415", "PYC415", "PYC415", "PYC415"]

    shadowed = """
from functools import partial
from pycircuit import Circuit, CycleAwareDomain, cas

def cas(value):
    return value

def build(m: Circuit, domain: CycleAwareDomain):
    value = cas(m.input("value", width=8))
    partial(value.trunc, width=4)
    try:
        selected = m.input("left", width=8)
    except Exception:
        selected = domain.create_signal("right", width=8)
    selected.trunc(width=4)
"""

    assert _codes(shadowed) == ["PYC415", "PYC415"]

    binding_forms = """
from functools import partial
from pycircuit import Circuit, cas

def build(m: Circuit, funcs, context):
    for cas in funcs:
        cas(m.input("a", width=8)).trunc(width=4)
    with context as cas:
        cas(m.input("b", width=8)).trunc(width=4)
    partial(getattr(m.input("c", width=8), "trunc"), width=4)
    [cas(m.input("d", width=8)).trunc(width=4) for cas in funcs]
    try:
        pass
    except Exception as cas:
        cas(m.input("e", width=8)).trunc(width=4)
    match context:
        case {"handler": cas} if m.input("guard", width=8).trunc(width=4):
            cas(m.input("f", width=8)).trunc(width=4)
    selected = m.input("g", width=8)
    for _item in funcs:
        break
    else:
        selected = cas(domain, selected)
    selected.trunc(width=4)
    while funcs:
        break
    else:
        selected = cas(domain, selected)
    selected.trunc(width=4)
"""

    assert _codes(binding_forms) == [
        "PYC415",
        "PYC415",
        "PYC415",
        "PYC415",
        "PYC415",
        "PYC415",
        "PYC415",
        "PYC415",
        "PYC415",
    ]
