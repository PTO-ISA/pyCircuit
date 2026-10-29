"""Independent four-state external-pop credit and atomic Discard oracle."""

from __future__ import annotations

import hashlib
import json
import os
import shutil

import pytest
from test_source_system_execution import (
    FIXTURE,
    _bundle,
    _cli,
    _compile,
    _emit_branch_probe_rtl,
    _install,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


def test_published_allocation_pool_preserves_replay_and_instance_isolation(tmp_path):
    """Issue #358: retain the complete one/two-instance source workload."""
    source = tmp_path / "source"
    shutil.copytree(FIXTURE / "allocation_repro", source)
    units = {
        name: tmp_path / f"{name}-unit" for name in ("contracts", "pool", "one", "two")
    }
    hidden_bodies = []
    try:
        for name, dependencies in (
            ("contracts", ()),
            ("pool", ("contracts",)),
            ("one", ("contracts", "pool")),
            ("two", ("contracts", "pool")),
        ):
            arguments = [
                "compile",
                "-c",
                str(source / f"{name}.py"),
                "--source-root",
                str(source),
                "--package-prefix",
                "allocation_repro",
                "-o",
                str(units[name]),
            ]
            for dependency in dependencies:
                arguments += ["-I", str(units[dependency])]
            _ok(_cli(*arguments))
            (source / f"{name}.py").unlink()
            if name in ("contracts", "pool"):
                receipt = json.loads((units[name] / "unit.json").read_text())
                body = units[name] / receipt["files"]["body"]
                hidden = tmp_path / f"{name}-hidden-body.ac"
                body.rename(hidden)
                hidden_bodies.append((body, hidden))
    finally:
        for body, hidden in hidden_bodies:
            hidden.rename(body)

    # Each instance must accept the same allocation identity independently.
    # Different positions expose accidental result/state sharing. The two
    # latency-one queues make a response visible at Work epochs 4 and 5,
    # then consume it at the rising edge: exactly one response per instance.
    for name, expected in (
        ("one", [("result", (1, 10))]),
        ("two", [("other", (1, 3)), ("result", (1, 10))]),
    ):
        output = tmp_path / name
        output.mkdir()
        final = output / "system.ac"
        _ok(
            _cli(
                "link",
                str(units["contracts"]),
                str(units["pool"]),
                str(units[name]),
                "--top",
                f"allocation_repro.{name}.Exercise",
                "-o",
                str(final),
            )
        )
        digest = hashlib.sha256(final.read_bytes()).hexdigest()
        for target in ("cpp", "verilog"):
            binary = _bundle(final, output, target)
            for workers in (1, 2) if target == "cpp" else (1,):
                arguments = (
                    ["--cycles", "8", "--workers", str(workers)]
                    if target == "cpp"
                    else ["+cycles=8"]
                )
                rows = _records(_ok(_run([str(binary), *arguments], timeout=30)))
                logs = [row for row in rows if row["kind"] == "log"]
                assert all(
                    value["kind"] in {"bool", "integer"}
                    for row in logs
                    for value in row["values"]
                ), "known pool responses must not contain X/Z bit planes"
                assert sorted(
                    (
                        row["spec"]["event"],
                        int(row["evaluation_epoch"]),
                        int(row["commit_epoch"]),
                        tuple(int(value["value"]) for value in row["values"]),
                    )
                    for row in logs
                ) == [
                    (event, epoch, epoch + 1, values)
                    for event, values in expected
                    for epoch in (4, 5)
                ]
                terminal = [row for row in rows if row["kind"] == "result"]
                assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"
                assert terminal[0]["error"] is None
                assert int(terminal[0]["epoch_time"]) == 16
        assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


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
