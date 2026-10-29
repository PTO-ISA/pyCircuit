"""Pure void Modules export no payload ports.

Stateful instance identity, autonomous work and saved Linked execution are
covered by CodeGen/transactional-void-modules.test with independent histories.
"""

from __future__ import annotations

import json

import pytest
from test_scalar_generic_modules import _link
from test_source_system_execution import _cli, _compile, _ok

pytestmark = pytest.mark.system


PURE = """import pycircuit as ac
@ac.module
def Empty() -> None:
    pass
@ac.module
def Explicit() -> None:
    return None
@ac.module
def Bare() -> None:
    return
@ac.module
def Parent() -> None:
    named = Empty()
    alias = named
    Explicit()
    Bare()
"""


@pytest.mark.parametrize("top", ["Empty", "Explicit", "Bare", "Parent"])
def test_pure_void_roots_export_no_payload_ports(tmp_path, top):
    source = tmp_path / "void module sources"
    source.mkdir()
    (source / "provider.py").write_text(PURE)
    provider = tmp_path / "provider"
    _ok(_compile(source, "provider.py", provider))
    final = tmp_path / "final.ac"
    _ok(_link([provider], "checks.provider." + top, final))
    for target in ("cpp", "verilog"):
        output = tmp_path / target
        _ok(_cli("emit", str(final), "--target", target, "-o", str(output)))
        ports = json.loads((output / "ports.json").read_text())
        assert ports["outputs"] == []
        domain_inputs = {
            ordinal for ordinal in ports["domains"].values() if ordinal is not None
        }
        assert {port["ordinal"] for port in ports["inputs"]} == domain_inputs
