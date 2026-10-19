"""Finite loop control preserves reached effects, induction values and else."""

from __future__ import annotations

import hashlib
import json
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


def test_inferred_loop_else_returns_reload_and_execute(tmp_path):
    source = tmp_path / "inferred source"
    source.mkdir()
    filename = "scf_else_returns.py"
    shutil.copy2(FIXTURE / filename, source)
    unit = tmp_path / "unit"
    _ok(_compile(source, filename, unit))
    final = tmp_path / "else-returns.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.scf_else_returns.ExerciseElseReturns",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    # Independent wrapping-u8 results, including while's 0/1/3/6 increments.
    expected = [
        (250, 253, 255, 5, 12, 7, 11, 13, 17, 23),
        (253, 0, 2, 8, 15, 11, 14, 16, 20, 26),
        (0, 3, 5, 11, 18, 16, 17, 19, 23, 29),
        (3, 6, 8, 14, 21, 22, 20, 22, 26, 32),
    ]
    transcripts = []
    # Remove original source/unit paths while retaining their evidence bytes.
    # Both targets reload only the saved final.
    source.rename(tmp_path / "retained-source")
    unit.rename(tmp_path / "retained-unit")
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
                if row["kind"] == "log" and row["spec"]["event"] == "else returns"
            ]
            assert [
                tuple(int(v["value"]) for v in row["values"]) for row in events
            ] == [v for v in expected for _ in range(2)]
            assert [int(row["evaluation_epoch"]) for row in events] == list(range(8))
            for event in ("bare else", "none else"):
                values = [
                    int(row["values"][0]["value"])
                    for row in records
                    if row["kind"] == "log" and row["spec"]["event"] == event
                ]
                assert values == [value for value in (250, 253, 0, 3) for _ in range(2)]
            _assert_closed_observation_identities(events)
            for event in ("bare else", "none else"):
                nested = [
                    row
                    for row in records
                    if row["kind"] == "log" and row["spec"]["event"] == event
                ]
                assert len({row["site"] for row in nested}) == 1
                identity = json.loads(nested[0]["site"])
                assert set(identity) == {"site", "invocation", "ordinal"}
                assert len(identity["invocation"]) == 1
                assert identity["invocation"][0]["receiver"] == {"instance_path": []}
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


@pytest.mark.parametrize(
    "body,diagnostic",
    [
        (
            "    for outer in range(0):\n        pass\n"
            "    else:\n        for inner in range(0):\n            pass\n"
            "    return value\n",
            "nested source for",
        ),
        (
            "    for outer in range(1):\n        pass\n"
            "    else:\n        for inner in range(1):\n            pass\n"
            "    return value\n",
            "nested source for",
        ),
        (
            "    for outer in range(0):\n        pass\n"
            "    else:\n        if value == 0:\n            for inner in range(1):\n"
            "                pass\n    return value\n",
            "nested source for",
        ),
        (
            "    for outer in range(0):\n        pass\n"
            "    else:\n        for middle in range(0):\n            pass\n"
            "        else:\n            for inner in range(0):\n                pass\n"
            "            else:\n                value = value + 1\n"
            "    return value\n",
            "nested source for",
        ),
        (
            "    for index in range(1):\n        if value == 0:\n            break\n"
            "    else:\n        return value\n",
            "return in a Method/Rule branch",
        ),
        (
            "    if value == 0:\n        return value\n    return value\n",
            "return in a Method/Rule branch",
        ),
        (
            "    for index in range(0):\n        pass\n"
            "    else:\n        return value\n    value = value + 1\n",
            "after Method/Rule return",
        ),
        (
            "    for index in range(0):\n        pass\n    else:\n        return\n",
            "return contract",
        ),
        (
            "    for index in range(0):\n        pass\n"
            "    else:\n        return None\n",
            "return contract",
        ),
    ],
    ids=[
        "nested-zero-else",
        "nested-nonzero-else",
        "nested-if-else",
        "nested-else-chain",
        "inferred-conditional-else-return",
        "inferred-ordinary-branch-return",
        "after-else-return",
        "bare-return-as-value",
        "none-return-as-value",
    ],
)
def test_loop_else_return_and_nesting_rejections_preserve_outputs(
    tmp_path, body, diagnostic
):
    source = tmp_path / "source"
    source.mkdir()
    design = source / "else_admission.py"
    prefix = "import pycircuit as ac\n@ac.rule\ndef evaluate(value: ac.u8):\n"
    suffix = "@ac.module\ndef Top(value: ac.u8) -> ac.u8:\n    return evaluate(value)\n"
    design.write_text(prefix + "    return value\n" + suffix)
    unit = tmp_path / "unit"
    _ok(_compile(source, design.name, unit))
    final = tmp_path / "protected.ac"
    _ok(_cli("link", str(unit), "--top", "checks.else_admission.Top", "-o", str(final)))
    for target in ("cpp", "verilog"):
        _ok(_cli("emit", str(final), "--target", target, "-o", str(tmp_path / target)))
    protected = [
        p
        for directory in (unit, tmp_path / "cpp", tmp_path / "verilog")
        for p in directory.rglob("*")
        if p.is_file()
    ] + [final]
    before = {p: p.read_bytes() for p in protected}
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
    if diagnostic in ("after Method/Rule return", "return contract"):
        # These existing contract diagnostics are attached to captured transport.
        assert "else_admission.transport.mlir" in rejected.stderr, rejected.stderr
    else:
        assert 'loc("else_admission.py":' in rejected.stderr, rejected.stderr
    assert (
        "Traceback" not in rejected.stderr and "Assertion failed" not in rejected.stderr
    )
    after = {
        p: p.read_bytes()
        for directory in (unit, tmp_path / "cpp", tmp_path / "verilog")
        for p in directory.rglob("*")
        if p.is_file()
    }
    after[final] = final.read_bytes()
    assert after == before


def test_conditional_loop_else_reserves_its_branch_occurrence(tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    design = source / "budget_else.py"
    # The private TableQueryBudget occurrence cap is also locked by the native
    # TableQuerySourceTest. Empty environments avoid exhausting copy slots first.
    # Every ordinary if reserves one occurrence; the final if-break and its
    # conditional else reserve one each. Prefill only this source ledger; later
    # pipeline budgets need not admit an at-capacity source publication.
    occurrence_limit = 4096
    prefix = "import pycircuit as ac\n@ac.rule\ndef evaluate() -> ac.u1:\n"
    finish = (
        "    for index in range(1):\n        if True:\n            break\n"
        "    else:\n        pass\n    return 0\n"
        "@ac.module\ndef Top() -> ac.u1:\n    return evaluate()\n"
    )
    flat_if = "    if True:\n        pass\n"
    design.write_text(prefix + flat_if + finish)
    unit = tmp_path / "unit"
    _ok(_compile(source, design.name, unit))
    final = tmp_path / "protected.ac"
    _ok(_cli("link", str(unit), "--top", "checks.budget_else.Top", "-o", str(final)))
    for target in ("cpp", "verilog"):
        _ok(_cli("emit", str(final), "--target", target, "-o", str(tmp_path / target)))
    before = {
        p: p.read_bytes()
        for directory in (unit, tmp_path / "cpp", tmp_path / "verilog")
        for p in directory.rglob("*")
        if p.is_file()
    }
    before[final] = final.read_bytes()
    design.write_text(prefix + flat_if * (occurrence_limit - 1) + finish)
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
    assert (
        "source loop-else branch construction: Table query occurrences budget exhausted"
        in rejected.stderr
    )
    loop_line = 4 + 2 * (occurrence_limit - 1)
    assert f'loc("budget_else.py":{loop_line}:5)' in rejected.stderr
    assert (
        "Traceback" not in rejected.stderr and "Assertion failed" not in rejected.stderr
    )
    after = {
        p: p.read_bytes()
        for directory in (unit, tmp_path / "cpp", tmp_path / "verilog")
        for p in directory.rglob("*")
        if p.is_file()
    }
    after[final] = final.read_bytes()
    assert after == before
