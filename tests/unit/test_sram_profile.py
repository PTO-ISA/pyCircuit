from __future__ import annotations

import pytest
from pycircuit.hw import Circuit, ClockDomain
from pycircuit.lib.sram import SRAM

pytestmark = pytest.mark.unit


def test_sram_uses_n1_profile_and_nba_safe_always_capture() -> None:
    circuit = Circuit("sram_profile")
    domain = ClockDomain(clk=circuit.clock("clk"), rst=circuit.reset("rst"))
    result = SRAM(
        circuit,
        domain,
        circuit.input("ren", width=1),
        circuit.input("raddr", width=2),
        circuit.input("wvalid", width=1),
        circuit.input("waddr", width=2),
        circuit.input("wdata", width=8),
        circuit.input("wstrb", width=1),
        depth=4,
    )
    circuit.output("rdata", result["rdata"])

    mlir = circuit.emit_mlir()
    assert "pyc.sync_mem" in mlir
    assert "live_window = 1" in mlir
    assert mlir.count("pyc.reg") == 1
    assert "pyc.select %ren" in mlir
    assert result["rdata_live"].width == 1
    assert result["rdata_raw"].width == 8
    assert result["rdata_captured"].width == 8
