"""Independent four-state external-pop credit and atomic Discard oracle."""

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


def test_other_rule_pop_credit_is_four_state_and_discard_atomic(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    shutil.copy2(FIXTURE / "queue_external_pop_probe.py", source)
    unit = tmp_path / "unit"
    _ok(_compile(source, "queue_external_pop_probe.py", unit))
    final = tmp_path / "queue-external-pop.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.queue_external_pop_probe.ExternalPopProbe",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    cpp = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(cpp)))
    for header in cpp.rglob("*.hpp"):
        header.write_text(header.read_text().replace("private:", "public:"))
    shutil.copy2(FIXTURE / "queue_external_pop_probe.cpp", cpp)
    with (cpp / "CMakeLists.txt").open("a") as handle:
        handle.write(
            "\nadd_executable(queue_external_pop_probe queue_external_pop_probe.cpp)\n"
            "target_compile_features(queue_external_pop_probe PRIVATE cxx_std_20)\n"
            "target_link_libraries(queue_external_pop_probe PRIVATE pycircuit_modules)\n"
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
                "queue_external_pop_probe",
                "--parallel",
                os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL", "2"),
            ]
        )
    )
    assert _ok(_run([str(build / "queue_external_pop_probe")], timeout=30)).strip() == (
        "EXTERNAL_POP_CPP_OK 322"
    )
    rtl_root = tmp_path / "rtl"
    rtl_root.mkdir()
    _generated, rtl = _emit_branch_probe_rtl(final, rtl_root)
    binary = rtl_root / "queue_external_pop_probe.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "queue_external_pop_probe",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dff.v"),
                str(_install() / "include/verilog/dffe.v"),
                str(_install() / "include/verilog/fifo.v"),
                *[str(path) for path in rtl if path.name != "simulation_top.sv"],
                str(FIXTURE / "queue_external_pop_probe.sv"),
            ]
        )
    )
    assert "EXTERNAL_POP_RTL_OK 161" in _ok(_run(["vvp", str(binary)], timeout=30))
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
