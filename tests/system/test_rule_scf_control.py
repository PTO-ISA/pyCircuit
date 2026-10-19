"""Finite loop control preserves reached effects, induction values and else."""

from __future__ import annotations

import hashlib
import os
import shutil

import pytest
from test_source_system_execution import (
    FIXTURE,
    _assert_closed_observation_identities,
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


def test_break_continue_and_loop_else_preserve_same_cycle_state(tmp_path):
    source = tmp_path / "scf sources"
    source.mkdir()
    shutil.copy2(FIXTURE / "scf_control_bench.py", source)
    unit = tmp_path / "unit"
    _ok(_compile(source, "scf_control_bench.py", unit))
    final = tmp_path / "scf-control.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.scf_control_bench.ExerciseScfControl",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    # For contributes 0,0,0,2,5,9,54,54; while contributes
    # 80,81,81,84,88,8,8,8. A break suppresses loop-else, whereas a
    # continue at the last iteration still reaches natural loop exit.
    expected = [
        (3, 80, 0, 0, 83),
        (83, 81, 1, 1, 164),
        (164, 81, 2, 2, 245),
        (245, 86, 3, 3, 75),
        (75, 93, 4, 4, 168),
        (168, 17, 5, 5, 185),
        (185, 62, 5, 5, 247),
        (247, 62, 5, 5, 53),
    ]
    transcripts = []
    for target in ("cpp", "verilog"):
        target_root = tmp_path / target
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            args = (
                ["--cycles", "8", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=8"]
            )
            records = _records(_ok(_run([str(binary), *args], timeout=30)))
            events = [
                row
                for row in records
                if row["kind"] == "log" and row["spec"]["event"] == "scf control"
            ]
            assert [
                tuple(int(v["value"]) for v in row["values"]) for row in events
            ] == [values for values in expected for _ in range(2)]
            assert [int(row["evaluation_epoch"]) for row in events] == list(range(16))
            _assert_closed_observation_identities(records)
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def test_loop_else_boundary_values_and_snapshots_from_one_final(tmp_path):
    source = tmp_path / "else source"
    source.mkdir()
    filename = "scf_else_boundaries.py"
    shutil.copy2(FIXTURE / filename, source)
    unit = tmp_path / "unit"
    _ok(_compile(source, filename, unit))
    final = tmp_path / "else-boundaries.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.scf_else_boundaries.ExerciseLoopElseBoundaries",
            "-o",
            str(final),
        )
    )
    # Link reloads the published unit, and each emitter reloads this same final.
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    expected = [
        (5, 9, 14, 13, 7, 30, 19, 14),
        (14, 9, 23, 13, 7, 31, 28, 23),
        (23, 9, 32, 13, 7, 32, 37, 32),
        (32, 9, 41, 13, 27, 33, 46, 41),
    ]
    transcripts = []
    for target in ("cpp", "verilog"):
        root = tmp_path / target
        root.mkdir()
        binary = _bundle(final, root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "4", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            records = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            events = [
                row
                for row in records
                if row["kind"] == "log" and row["spec"]["event"] == "else boundaries"
            ]
            assert [
                tuple(int(v["value"]) for v in row["values"]) for row in events
            ] == [v for v in expected for _ in range(2)]
            assert [int(row["evaluation_epoch"]) for row in events] == list(range(8))
            _assert_closed_observation_identities(records)
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


@pytest.mark.parametrize(
    "body,diagnostic",
    [
        (
            "    for fresh in range(0):\n        pass\n"
            "    else:\n        value = fresh\n    return value\n",
            "fresh",
        ),
        (
            "    for index in range(2):\n        if value == 0:\n            break\n"
            "    else:\n        fresh = value + 1\n    return fresh\n",
            "fresh",
        ),
        (
            "    for index in range(2):\n        if value == 0:\n            break\n"
            "    else:\n        return value\n    return value\n",
            "return",
        ),
        (
            "    cursor: ac.u2 = 0\n    while cursor < 3:\n"
            "        if value == 0:\n            continue\n"
            "        cursor = cursor + 1\n"
            "    else:\n        value = value + 1\n    return value\n",
            "progress",
        ),
        (
            "    for index in range(0):\n        for nested in range(1):\n"
            "            pass\n    else:\n        value = value + 1\n"
            "    return value\n",
            "nested",
        ),
    ],
    ids=[
        "zero-new-binder",
        "conditional-else-local",
        "conditional-else-return",
        "while-continue-no-progress",
        "zero-nested-loop",
    ],
)
def test_loop_else_rejections_preserve_published_unit(tmp_path, body, diagnostic):
    source = tmp_path / "source"
    source.mkdir()
    design = source / "rejected_else.py"
    prefix = "import pycircuit as ac\n@ac.rule\ndef evaluate(value: ac.u8) -> ac.u8:\n"
    suffix = "@ac.module\ndef Top(value: ac.u8) -> ac.u8:\n    return evaluate(value)\n"
    design.write_text(prefix + "    return value\n" + suffix)
    unit = tmp_path / "unit"
    _ok(_compile(source, design.name, unit))
    before = {
        p.relative_to(unit): p.read_bytes() for p in unit.rglob("*") if p.is_file()
    }
    final = tmp_path / "protected.ac"
    _ok(_cli("link", str(unit), "--top", "checks.rejected_else.Top", "-o", str(final)))
    final_before = final.read_bytes()
    design.write_text(prefix + body + suffix)
    rejected = _cli(
        "compile",
        "-c",
        str(design),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
        "-o",
        str(unit),
        "--replace",
    )
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert diagnostic in rejected.stderr, rejected.stderr
    assert 'loc("rejected_else.py":' in rejected.stderr, rejected.stderr
    assert (
        "Traceback" not in rejected.stderr and "Assertion failed" not in rejected.stderr
    )
    assert {
        p.relative_to(unit): p.read_bytes() for p in unit.rglob("*") if p.is_file()
    } == before
    assert final.read_bytes() == final_before


def test_loop_else_failure_discards_state_clocks_and_observations(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    shutil.copy2(FIXTURE / "scf_else_failure.py", source)
    unit = tmp_path / "unit"
    _ok(_compile(source, "scf_else_failure.py", unit))
    final = tmp_path / "loop-else-failure.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.scf_else_failure.LoopElseFailure",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    cpp = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(cpp)))
    for header in cpp.rglob("*.hpp"):
        header.write_text(header.read_text().replace("private:", "public:"))
    shutil.copy2(FIXTURE / "scf_else_failure.cpp", cpp)
    with (cpp / "CMakeLists.txt").open("a") as handle:
        handle.write(
            "\nadd_executable(scf_else_failure scf_else_failure.cpp)\n"
            "target_compile_features(scf_else_failure PRIVATE cxx_std_20)\n"
            "target_link_libraries(scf_else_failure PRIVATE pycircuit_modules)\n"
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
                "scf_else_failure",
                "--parallel",
                os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL", "2"),
            ]
        )
    )
    assert _ok(_run([str(build / "scf_else_failure")], timeout=30)).strip() == (
        "LOOP_ELSE_CPP_OK 16"
    )
    rtl_root = tmp_path / "rtl"
    rtl_root.mkdir()
    _generated, rtl = _emit_branch_probe_rtl(final, rtl_root)
    binary = rtl_root / "scf_else_failure.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "scf_else_failure",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dff.v"),
                str(_install() / "include/verilog/dffe.v"),
                str(_install() / "include/verilog/fifo.v"),
                *[str(path) for path in rtl if path.name != "simulation_top.sv"],
                str(FIXTURE / "scf_else_failure.sv"),
            ]
        )
    )
    assert "LOOP_ELSE_RTL_OK 8" in _ok(_run(["vvp", str(binary)], timeout=30))
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
