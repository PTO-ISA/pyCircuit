"""Independent resource/selector four-state truth tables on generated Systems."""

from __future__ import annotations

import hashlib
import os
import shutil

import pytest
from test_source_system_execution import (
    FIXTURE,
    _cli,
    _compile,
    _emit_branch_probe_rtl,
    _install,
    _ok,
    _run,
)

pytestmark = pytest.mark.system


def test_rule_resources_gate_only_reached_diagnostics(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    shutil.copy2(FIXTURE / "resource_demand_probe.py", source)
    unit = tmp_path / "unit"
    _ok(_compile(source, "resource_demand_probe.py", unit))
    final = tmp_path / "resource-demand.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.resource_demand_probe.ResourceDemandProbe",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    cpp = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(cpp)))
    for header in cpp.rglob("*.hpp"):
        header.write_text(header.read_text().replace("private:", "public:"))
    shutil.copy2(FIXTURE / "resource_demand_probe.cpp", cpp)
    with (cpp / "CMakeLists.txt").open("a") as handle:
        handle.write(
            "\nadd_executable(resource_demand_probe resource_demand_probe.cpp)\n"
            "target_compile_features(resource_demand_probe PRIVATE cxx_std_20)\n"
            "target_link_libraries(resource_demand_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(cpp),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(
        _run(
            [
                "cmake",
                "--build",
                str(build),
                "--target",
                "resource_demand_probe",
                "--parallel",
                os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL", "2"),
            ]
        )
    )
    assert _ok(_run([str(build / "resource_demand_probe")], timeout=30)).strip() == (
        "RESOURCE_DEMAND_CPP_OK 338"
    )
    rtl_root = tmp_path / "rtl"
    rtl_root.mkdir()
    _generated, rtl = _emit_branch_probe_rtl(final, rtl_root)
    testbench = FIXTURE / "resource_demand_probe.sv"
    binary = rtl_root / "resource_demand_probe.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "resource_demand_probe",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dff.v"),
                str(_install() / "include/verilog/dffe.v"),
                str(_install() / "include/verilog/fifo.v"),
                *[str(path) for path in rtl if path.name != "simulation_top.sv"],
                str(testbench),
            ]
        )
    )
    assert "RESOURCE_DEMAND_RTL_OK 169" in _ok(_run(["vvp", str(binary)], timeout=30))
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
