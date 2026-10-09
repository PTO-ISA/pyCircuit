"""Closed source systems execute using only compiler-generated harnesses.

The numerical oracle below follows declared register updates and old-Q reads.
It is independent of generated signal names, graph shape, and process status.
"""

from __future__ import annotations

import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import pytest

pytestmark = pytest.mark.system
ROOT = Path(__file__).resolve().parents[2]
FIXTURE = Path(__file__).parent / "fixtures/source_system"
MAX_CYCLES = (2**63 - 1) // 2


def _environment() -> dict[str, str]:
    env = dict(os.environ)
    env["PYTHONPATH"] = os.pathsep.join(
        filter(
            None,
            [
                str(ROOT / "python"),
                env.get("PYTHONPATH"),
            ],
        )
    )
    return env


def _run(arguments: list[str], *, timeout: int = 120, cwd: Path | None = None):
    return subprocess.run(
        arguments,
        env=_environment(),
        text=True,
        capture_output=True,
        check=False,
        timeout=timeout,
        cwd=cwd,
    )


def _cli(*arguments: str):
    return _run([sys.executable, "-m", "pycircuit.cli", *arguments])


def _ok(result):
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def _install() -> Path:
    configured = os.environ.get("PYC_TOOLCHAIN_ROOT")
    assert configured, "set PYC_TOOLCHAIN_ROOT to this candidate's installed toolchain"
    prefix = Path(configured).resolve()
    assert (prefix / "share/pycircuit/cmake/pycircuitConfig.cmake").is_file()
    return prefix


def _source(
    tmp_path: Path,
    variant: bool = False,
    spelling: str = "bits",
    assertions: bool = True,
) -> Path:
    source = tmp_path / "system sources with spaces"
    shutil.copytree(FIXTURE, source)
    if variant:
        for filename in ("dut.py", "bench.py", "CMakeLists.txt"):
            path = source / filename
            path.write_text(
                path.read_text()
                .replace("Accumulator", "WindowCounter")
                .replace("bits[8]", "bits[5]")
                .replace("log(", "ac.log(")
                .replace("report(", "ac.report(")
            )
        bench = source / "bench.py"
        bench.write_text("import pycircuit as ac\n" + bench.read_text())
    if spelling != "bits":
        for filename in ("dut.py", "bench.py"):
            path = source / filename
            text = path.read_text()
            if spelling == "direct":
                imports, byte, bit = "from pycircuit import u1, u8\n", "u8", "u1"
            elif spelling == "alias":
                imports, byte, bit = (
                    "from pycircuit import u1 as Flag, u8 as Byte\n",
                    "Byte",
                    "Flag",
                )
            else:
                assert spelling == "namespace"
                imports, byte, bit = "import pycircuit as hw\n", "hw.u8", "hw.u1"
            path.write_text(
                imports + text.replace("bits[8]", byte).replace("bits[1]", bit)
            )
    if not assertions:
        bench = source / "bench.py"
        original = bench.read_text()
        assertion = '        assert right.value < 4, "deliberate limit"\n'
        assert assertion in original
        bench.write_text(original.replace(assertion, ""))
    return source


def _compile(source: Path, filename: str, output: Path, providers=()):
    arguments = [
        "compile",
        "-c",
        str(source / filename),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
        "-o",
        str(output),
    ]
    for provider in providers:
        arguments += ["-I", str(provider)]
    return _cli(*arguments)


def _program(tmp_path: Path, variant=False, spelling="bits", assertions=True):
    source = _source(tmp_path, variant, spelling, assertions)
    dut, bench = tmp_path / "dut unit", tmp_path / "bench unit"
    _ok(_compile(source, "dut.py", dut))
    _ok(_compile(source, "bench.py", bench, [dut]))
    final = tmp_path / "system.ac"
    name = "ExerciseWindowCounter" if variant else "ExerciseAccumulator"
    _ok(
        _cli(
            "link",
            str(dut),
            str(bench),
            "--top",
            f"checks.bench.{name}",
            "-o",
            str(final),
        )
    )
    return source, dut, bench, final


def _bundle(final: Path, tmp_path: Path, target: str) -> Path:
    generated = tmp_path / target
    _ok(_cli("emit", str(final), "--target", target, "-o", str(generated)))
    receipt = json.loads((generated / "generated.json").read_text())
    names = {item["path"] for item in receipt["files"]}
    harness = "simulation_main.cpp" if target == "cpp" else "simulation_top.sv"
    assert harness in names
    build = tmp_path / f"build-{target}"
    package = _install() / "share/pycircuit/cmake"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(generated),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={package}",
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
                "pycircuit_sim",
                "--parallel",
                "2",
            ]
        )
    )
    binary = build / "bin/pycircuit_sim"
    assert binary.is_file()
    return binary


def _records(stdout: str) -> list[dict]:
    return [json.loads(line) for line in stdout.splitlines() if line.startswith("{")]


def _observations(rows, kind, name):
    field = "event" if kind == "log" else "name"
    events = [row for row in rows if row["kind"] == kind and row["spec"][field] == name]
    return [
        (
            int(row["evaluation_epoch"]),
            int(row["commit_epoch"]),
            int(row["values"][0]["value"]),
        )
        for row in events
    ]


def _assert_run_receipt(build, target, status="success"):
    receipt = json.loads((build / "run-execution.json").read_text())
    assert receipt["status"] == status
    executable = build / "simulation" / target / "bin/pycircuit_sim"
    assert receipt["command"][0] == str(executable)
    generated = build / target
    manifest = json.loads((generated / "generated.json").read_text())
    expected = {str(executable), str(generated / "generated.json")}
    expected.update(str(generated / row["path"]) for row in manifest["files"])
    runtimes = {str(path) for path in _install().glob("lib*/*pyc6_runtime*")}
    assert runtimes
    expected.update(runtimes)
    assert expected <= set(receipt["inputs"])
    for path, digest in receipt["inputs"].items():
        assert hashlib.sha256(Path(path).read_bytes()).hexdigest() == digest
    if status == "success":
        assert receipt["inputs_after"] == receipt["inputs"]


def _assert_success(rows):
    # phase increments on the rising half of each cycle. Left accepts every
    # other phase; right accepts all phases. Both observations read old Q.
    for kind, name, values in [
        ("log", "left", [0, 0, 1, 1, 1, 1, 2, 2]),
        ("log", "right", [0, 0, 1, 1, 2, 2, 3, 3]),
        ("report", "progress", [0, 0, 1, 1, 2, 2, 3, 3]),
    ]:
        assert _observations(rows, kind, name) == [
            (epoch, epoch + 1, value) for epoch, value in enumerate(values)
        ]
    results = [row for row in rows if row["kind"] == "result"]
    assert len(results) == 1
    assert results[0]["status"] == "TERMINATED"
    assert int(results[0]["epoch_time"]) == 8
    progress = [row for row in results[0]["statistics"] if row["name"] == "progress"]
    assert len(progress) == 1 and int(progress[0]["value"]) == 3


def test_hello_counter_preserves_original_five_cycle_logs(tmp_path):
    source = tmp_path / "hello source"
    source.mkdir()
    shutil.copy2(ROOT / "examples/hello_counter/hello_counter.py", source)
    unit = tmp_path / "hello unit"
    _ok(_compile(source, "hello_counter.py", unit))
    final = tmp_path / "hello.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.hello_counter.HelloCounter",
            "-o",
            str(final),
        )
    )
    expected = [(epoch, epoch + 1, epoch // 2) for epoch in range(10)]
    for target in ("cpp", "verilog"):
        binary = _bundle(final, tmp_path, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "5", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=5"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=20)))
            assert _observations(rows, "log", "count") == expected
            logs = [row for row in rows if row["kind"] == "log"]
            assert len(logs) == 10
            terminal = [row for row in rows if row["kind"] == "result"]
            assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"
            assert int(terminal[0]["epoch_time"]) == 10


def test_observation_marker_text_is_literal_in_cpp_and_verilator(tmp_path):
    event = 'event @pyc_decimal@ @pyc_string@ 50% "quoted"'
    literal = 'literal @pyc_decimal@ @pyc_string@ 75% "quoted"'
    report = 'report @pyc_decimal@ @pyc_string@ 100% "quoted"'
    source = tmp_path / "marker source"
    source.mkdir()
    (source / "markers.py").write_text(
        "from pycircuit import log, report, rule, system, u8\n"
        "\n"
        "@rule\n"
        "def observe(count):\n"
        f"    log('info', {event!r}, {literal!r}, count)\n"
        f"    report({report!r}, count)\n"
        "    count = count + 1\n"
        "\n"
        "@system\n"
        "def MarkerText():\n"
        "    count: u8 = 0\n"
        "    observe(count)\n"
    )
    unit = tmp_path / "marker unit"
    _ok(_compile(source, "markers.py", unit))
    final = tmp_path / "markers.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.markers.MarkerText",
            "-o",
            str(final),
        )
    )

    transcripts = []
    report_statistics = []
    for target in ("cpp", "verilog"):
        binary = _bundle(final, tmp_path, target)
        arguments = ["--cycles", "2"] if target == "cpp" else ["+cycles=2"]
        rows = _records(_ok(_run([str(binary), *arguments], timeout=20)))
        observations = [row for row in rows if row["kind"] != "result"]
        logs = [row for row in observations if row["kind"] == "log"]
        reports = [row for row in observations if row["kind"] == "report"]
        assert len(logs) == len(reports) == 4
        assert all(
            row["spec"]
            == {
                "event": event,
                "items": [
                    {"kind": "literal", "text": literal},
                    {"kind": "value", "ordinal": 0},
                ],
                "level": "info",
            }
            for row in logs
        )
        assert all(row["spec"] == {"name": report} for row in reports)
        assert [int(row["values"][0]["value"]) for row in logs] == [0, 0, 1, 1]
        assert [int(row["values"][0]["value"]) for row in reports] == [0, 0, 1, 1]
        results = [row for row in rows if row["kind"] == "result"]
        assert len(results) == 1 and results[0]["status"] == "TERMINATED"
        statistics = [row for row in results[0]["statistics"] if row["name"] == report]
        assert len(statistics) == 1
        assert int(statistics[0]["last_update"]["time"]) == 4
        assert int(statistics[0]["value"]) == 1
        transcripts.append(observations)
        report_statistics.append(statistics)
    assert transcripts[0] == transcripts[1]
    assert report_statistics[0] == report_statistics[1]


def test_duplicate_report_names_in_one_module_reject_and_preserve_unit(tmp_path):
    source = tmp_path / "duplicate report source"
    source.mkdir()
    design = source / "duplicate_reports.py"
    valid = (
        "from pycircuit import report, rule, system, u8\n"
        "\n"
        "@rule\n"
        "def observe_left(value):\n"
        "    report('left', value)\n"
        "\n"
        "@rule\n"
        "def observe_right(value):\n"
        "    report('right', value)\n"
        "\n"
        "@system\n"
        "def DistinctReports():\n"
        "    value: u8 = 0\n"
        "    observe_left(value)\n"
        "    observe_right(value)\n"
    )
    design.write_text(valid)
    output = tmp_path / "duplicate report unit"
    _ok(_compile(source, design.name, output))
    before = {
        path.name: path.read_bytes() for path in output.iterdir() if path.is_file()
    }
    design.write_text(valid.replace("report('right', value)", "report('left', value)"))
    rejected = _cli(
        "compile",
        "-c",
        str(design),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
        "-o",
        str(output),
        "--replace",
    )
    assert rejected.returncode != 0
    assert "report name must be unique within module: left" in rejected.stderr
    assert {
        path.name: path.read_bytes() for path in output.iterdir() if path.is_file()
    } == before


def test_duplicate_report_names_across_instances_execute_in_both_backends(tmp_path):
    source = tmp_path / "instance report source"
    source.mkdir()
    (source / "probe.py").write_text(
        "from pycircuit import module, report, rule, u8\n"
        "\n"
        "@rule\n"
        "def observe(value):\n"
        "    report('shared', value)\n"
        "\n"
        "@module\n"
        "def Probe(value: u8) -> {}:\n"
        "    observe(value)\n"
        "    return {}\n"
    )
    (source / "bench.py").write_text(
        "from checks.probe import Probe\n"
        "from pycircuit import rule, system, u8\n"
        "\n"
        "@rule\n"
        "def advance(value):\n"
        "    value = value + 1\n"
        "\n"
        "@system\n"
        "def TwoProbes():\n"
        "    value: u8 = 0\n"
        "    left = Probe()\n"
        "    right = Probe()\n"
        "\n"
        "    @rule\n"
        "    def drive():\n"
        "        left(value=value)\n"
        "        right(value=value)\n"
        "\n"
        "    advance(value)\n"
        "    drive()\n"
    )
    probe = tmp_path / "probe unit"
    bench = tmp_path / "probe bench unit"
    _ok(_compile(source, "probe.py", probe))
    _ok(_compile(source, "bench.py", bench, [probe]))
    final = tmp_path / "two-probes.ac"
    _ok(
        _cli(
            "link",
            str(probe),
            str(bench),
            "--top",
            "checks.bench.TwoProbes",
            "-o",
            str(final),
        )
    )

    transcripts = []
    statistics = []
    for target in ("cpp", "verilog"):
        binary = _bundle(final, tmp_path, target)
        arguments = ["--cycles", "1"] if target == "cpp" else ["+cycles=1"]
        rows = _records(_ok(_run([str(binary), *arguments], timeout=20)))
        reports = [row for row in rows if row["kind"] == "report"]
        assert len(reports) == 4
        assert {row["instance"] for row in reports} == {"root/left", "root/right"}
        assert all(row["spec"] == {"name": "shared"} for row in reports)
        results = [row for row in rows if row["kind"] == "result"]
        assert len(results) == 1 and results[0]["status"] == "TERMINATED"
        gauges = [row for row in results[0]["statistics"] if row["name"] == "shared"]
        assert len(gauges) == 2
        assert {row["object_path"] for row in gauges} == {"root/left", "root/right"}
        assert all(int(row["last_update"]["time"]) == 2 for row in gauges)
        assert all(int(row["value"]) == 0 for row in gauges)
        transcripts.append(reports)
        statistics.append(gauges)
    assert transcripts[0] == transcripts[1]
    assert statistics[0] == statistics[1]


def test_system_without_assertions_still_commits_child_storage(tmp_path):
    _source_root, _dut, _bench, final = _program(tmp_path, assertions=False)
    for target in ("cpp", "verilog"):
        binary = _bundle(final, tmp_path, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "4", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            _assert_success(_records(_ok(_run([str(binary), *arguments], timeout=20))))


def test_assertion_before_conditional_owner_write_preserves_provenance_and_commit(
    tmp_path,
):
    source = tmp_path / "assertion owner source"
    source.mkdir()
    design = source / "owner_after_assert.py"
    valid = """from pycircuit import log, rule, struct, system, table, u1, u2, u4, u7

@struct
class Pair:
    west: u7
    east: u7

@struct
class Event:
    pair: Pair
    fire: u1

@struct
class Fixtures:
    passing: table[4, Event] = (
        Event(pair=Pair(west=9, east=9), fire=1),
        Event(pair=Pair(west=14, east=3), fire=0),
        Event(pair=Pair(west=65, east=65), fire=1),
        Event(pair=Pair(west=2, east=2), fire=1),
    )
    failing: table[4, Event] = (
        Event(pair=Pair(west=9, east=9), fire=1),
        Event(pair=Pair(west=14, east=3), fire=0),
        Event(pair=Pair(west=65, east=64), fire=1),
        Event(pair=Pair(west=2, east=2), fire=1),
    )

@rule
def verify_and_advance(cursor: u4, sibling: u7, phase: u4, pair: Pair, fire: u1):
    log("info", "cursor", cursor)
    log("info", "sibling", sibling)
    log("info", "phase", phase)
    if fire:
        sibling = sibling + 1
        if pair.west == 127:
            assert pair.east == 127, "maximum pair fields must agree"
        else:
            assert pair.west == pair.east, "pair fields must agree"
        cursor = cursor + 1
    phase = phase + 1

@system
def PassingOwner():
    cursor: u4 = 0
    sibling: u7 = 0
    phase: u4 = 0
    fixtures = Fixtures()
    event = fixtures.passing[phase % 4]
    verify_and_advance(cursor, sibling, phase, event.pair, event.fire)

@system
def FailingOwner():
    cursor: u4 = 0
    sibling: u7 = 0
    phase: u4 = 0
    fixtures = Fixtures()
    event = fixtures.failing[phase % 4]
    verify_and_advance(cursor, sibling, phase, event.pair, event.fire)
"""
    design.write_text(valid)
    unit = tmp_path / "assertion owner unit"
    _ok(_compile(source, design.name, unit))

    before = {path.name: path.read_bytes() for path in unit.iterdir() if path.is_file()}
    invalid = valid.replace("cursor = cursor + 1", "cursor = pair.west")
    assert invalid != valid
    design.write_text(invalid)
    fresh = tmp_path / "invalid assertion owner unit"
    rejected = _compile(source, design.name, fresh)
    assert rejected.returncode != 0
    assert not fresh.exists()
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
    assert rejected.returncode != 0
    assert {
        path.name: path.read_bytes() for path in unit.iterdir() if path.is_file()
    } == before
    design.write_text(valid)

    passing = tmp_path / "passing-owner.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.owner_after_assert.PassingOwner",
            "-o",
            str(passing),
        )
    )
    failing = tmp_path / "failing-owner.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.owner_after_assert.FailingOwner",
            "-o",
            str(failing),
        )
    )

    passing_transcripts = []
    failing_transcripts = []
    for target in ("cpp", "verilog"):
        passing_build = tmp_path / ("passing-" + target)
        passing_build.mkdir()
        passing_binary = _bundle(passing, passing_build, target)
        workers = (1, 2) if target == "cpp" else (1,)
        for count in workers:
            arguments = (
                ["--cycles", "4", "--workers", str(count)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            rows = _records(_ok(_run([str(passing_binary), *arguments], timeout=20)))
            assert _observations(rows, "log", "cursor") == [
                (epoch, epoch + 1, value)
                for epoch, value in enumerate([0, 0, 1, 1, 1, 1, 2, 2])
            ]
            assert _observations(rows, "log", "sibling") == [
                (epoch, epoch + 1, value)
                for epoch, value in enumerate([0, 0, 1, 1, 1, 1, 2, 2])
            ]
            assert _observations(rows, "log", "phase") == [
                (epoch, epoch + 1, epoch // 2) for epoch in range(8)
            ]
            passing_transcripts.append([row for row in rows if row["kind"] != "result"])

        failing_build = tmp_path / ("failing-" + target)
        failing_build.mkdir()
        failing_binary = _bundle(failing, failing_build, target)
        arguments = ["--cycles", "4"] if target == "cpp" else ["+cycles=4"]
        result = _run([str(failing_binary), *arguments], timeout=20)
        assert result.returncode != 0, result.stdout + result.stderr
        rows = _records(result.stdout)
        terminal = [row for row in rows if row["kind"] == "result"]
        if terminal:
            assert len(terminal) == 1 and terminal[0]["status"] == "FAILED"
            assert terminal[0]["error"]["code"] == "source_check_failed"
        assert _observations(rows, "log", "cursor") == [
            (epoch, epoch + 1, value) for epoch, value in enumerate([0, 0, 1, 1])
        ]
        assert _observations(rows, "log", "sibling") == [
            (epoch, epoch + 1, value) for epoch, value in enumerate([0, 0, 1, 1])
        ]
        assert _observations(rows, "log", "phase") == [
            (epoch, epoch + 1, epoch // 2) for epoch in range(4)
        ]
        failing_transcripts.append([row for row in rows if row["kind"] != "result"])

    assert passing_transcripts[0] == passing_transcripts[1] == passing_transcripts[2]
    assert failing_transcripts[0] == failing_transcripts[1]


@pytest.mark.parametrize(
    "selected,checks", [("UncheckedRoot", 0), ("TwoCheckedChildren", 4)]
)
def test_emitted_verification_metadata_counts_selected_reachable_instances(
    tmp_path, selected, checks
):
    source = tmp_path / "verification source"
    source.mkdir()
    shutil.copy2(FIXTURE / "verification_bench.py", source)
    shutil.copy2(FIXTURE / "verification_dut.py", source)
    provider = tmp_path / "verification provider"
    _ok(_compile(source, "verification_dut.py", provider))
    unit = tmp_path / "verification unit"
    _ok(_compile(source, "verification_bench.py", unit, [provider]))
    # The link closure carries checks in an unrelated root and a reusable child.
    # A text count would wrongly credit both when selecting the unchecked root,
    # and would miss the repeated child instance when selecting the checked one.
    assert (unit / "verification_bench.ac").read_text().count('"ac.expect"(') == 1
    assert (provider / "verification_dut.ac").read_text().count('"ac.expect"(') == 2
    entry = f"checks.verification_bench.{selected}"
    final = tmp_path / "verification.ac"
    _ok(_cli("link", str(provider), str(unit), "--top", entry, "-o", str(final)))
    assert final.read_text().count('"ac.expect"(') == 3
    for target in ("cpp", "verilog"):
        generated = tmp_path / target
        _ok(_cli("emit", str(final), "--target", target, "-o", str(generated)))
        receipt = json.loads((generated / "generated.json").read_text())
        assert "simulation_verification.json" in {
            row["path"] for row in receipt["files"]
        }
        metadata = json.loads((generated / "simulation_verification.json").read_text())
        assert metadata == {
            "entry": {"definition": f'@"{entry}"', "arguments": []},
            "entry_source": {"package": "checks", "path": "verification_bench.py"},
            "source_checks": checks,
        }


@pytest.mark.parametrize(
    "binding,annotation,shadow",
    [
        ("from pycircuit import u8", "u8", "    u8 = 8\n"),
        ("from pycircuit import u8 as Byte", "Byte", "    Byte = 8\n"),
        ("import pycircuit as hw", "hw.u8", "    hw = 8\n"),
    ],
    ids=["direct", "alias", "namespace"],
)
def test_source_builtin_type_shadow_rejects_without_replacing_unit(
    tmp_path, binding, annotation, shadow
):
    source = tmp_path / "source"
    source.mkdir()
    design = source / "typed.py"
    prefix = (
        binding + "\nfrom pycircuit import system, rule, log\n"
        "@rule\ndef increment(count):\n    log('info', 'count', count)\n"
        "    count = count + 1\n"
    )
    body = (
        "@system\ndef Typed():\n"
        + f"    count: {annotation} = 0\n"
        + "    increment(count)\n"
    )
    design.write_text(prefix + body)
    output = tmp_path / "unit"
    _ok(_compile(source, "typed.py", output))
    before = {
        path.name: path.read_bytes() for path in output.iterdir() if path.is_file()
    }
    design.write_text(
        prefix + body.replace("def Typed():\n", "def Typed():\n" + shadow)
    )
    rejected = _cli(
        "compile",
        "-c",
        str(design),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
        "-o",
        str(output),
        "--replace",
    )
    assert rejected.returncode != 0
    assert "shadow" in rejected.stderr.lower(), rejected.stderr
    assert {
        path.name: path.read_bytes() for path in output.iterdir() if path.is_file()
    } == before


@pytest.mark.parametrize(
    "binding,annotation",
    [
        ("from checks.impostor import u8", "u8"),
        ("from checks.impostor import u8 as Byte", "Byte"),
        ("import checks.impostor as hw", "hw.u8"),
    ],
    ids=["direct", "alias", "namespace"],
)
def test_source_builtin_type_requires_pycircuit_binding(tmp_path, binding, annotation):
    source = tmp_path / "source"
    source.mkdir()
    # Resolve a real published foreign symbol with the builtin's spelling;
    # missing-provider rejection would not test type-binding identity.
    (source / "impostor.py").write_text(
        "from pycircuit import module\n@module\ndef u8() -> {}:\n    return {}\n"
    )
    provider = tmp_path / "provider"
    _ok(_compile(source, "impostor.py", provider))
    (source / "typed.py").write_text(
        binding
        + "\nfrom pycircuit import system\n@system\ndef Typed():\n"
        + f"    count: {annotation} = 0\n"
    )
    output = tmp_path / "unit"
    rejected = _compile(source, "typed.py", output, [provider])
    assert rejected.returncode != 0
    assert (
        "impostor" in rejected.stderr or "type" in rejected.stderr.lower()
    ), rejected.stderr
    assert not output.exists()


def _record_source(tmp_path):
    source = _source(tmp_path)
    provider = tmp_path / "record provider"
    _ok(_compile(source, "record_types.py", provider))
    return source, provider


@pytest.mark.parametrize("spelling", ["direct", "alias", "reexport"])
def test_imported_nested_struct_full_constructors_execute_with_nominal_rule_result(
    tmp_path, spelling
):
    source, provider = _record_source(tmp_path)
    providers = [provider]
    bench = source / "record_bench.py"
    if spelling == "alias":
        bench.write_text(
            bench.read_text()
            .replace(
                "import Inner, Packet", "import Inner as Payload, Packet as Envelope"
            )
            .replace("-> Packet", "-> Envelope")
            .replace("result: Packet = Packet(", "result: Envelope = Envelope(")
            .replace("inner=Inner(", "inner=Payload(")
        )
    elif spelling == "reexport":
        (source / "record_facade.py").write_text(
            "from checks.record_types import Inner, Packet\n"
        )
        facade = tmp_path / "record facade"
        _ok(_compile(source, "record_facade.py", facade, [provider]))
        providers.append(facade)
        bench.write_text(
            bench.read_text().replace("checks.record_types", "checks.record_facade")
        )
    unit = tmp_path / "record bench"
    _ok(_compile(source, "record_bench.py", unit, providers))
    final = tmp_path / "records.ac"
    _ok(
        _cli(
            "link",
            *map(str, [*providers, unit]),
            "--top",
            "checks.record_bench.ConstructPacket",
            "-o",
            str(final),
        )
    )
    transcripts = []
    for target in ("cpp", "verilog"):
        binary = _bundle(final, tmp_path, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "3", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=3"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=20)))
            for name, values in (
                ("payload", [0, 0, 1, 1, 2, 2]),
                ("flag", [1] * 6),
                ("sequence", [3] * 6),
            ):
                assert _observations(rows, "log", name) == [
                    (epoch, epoch + 1, value) for epoch, value in enumerate(values)
                ]
            transcripts.append([row for row in rows if row["kind"] == "log"])
    assert transcripts[0] == transcripts[1] == transcripts[2]


@pytest.mark.parametrize(
    "mutation,diagnostic",
    [
        ("outer-omission", "field"),
        ("nested-omission", "field"),
        ("default-omission", "field"),
        ("unknown-field", "field"),
        ("duplicate-field", "duplicate"),
        ("foreign-nominal", "type"),
        ("shadowed-constructor", "shadow"),
        ("persistent-initializer", "initializer"),
    ],
)
def test_imported_struct_constructor_refusals_preserve_published_unit(
    tmp_path, mutation, diagnostic
):
    source, provider = _record_source(tmp_path)
    bench = source / "record_bench.py"
    output = tmp_path / "record bench"
    _ok(_compile(source, "record_bench.py", output, [provider]))
    before = {
        path.name: path.read_bytes() for path in output.iterdir() if path.is_file()
    }
    original = bench.read_text()
    constructor = "Packet(inner=Inner(flag=1, data=count), sequence=3)"
    replacements = {
        "outer-omission": "Packet(sequence=3)",
        "nested-omission": "Packet(inner=Inner(flag=1), sequence=3)",
        "default-omission": "Packet(inner=Inner(flag=1, data=count))",
        "unknown-field": "Packet(inner=Inner(flag=1, data=count), sequence=3, extra=0)",
        "duplicate-field": "Packet(inner=Inner(flag=1, data=count), sequence=3, sequence=4)",
        "foreign-nominal": "ForeignPacket(inner=Inner(flag=1, data=count), sequence=3)",
    }
    if mutation == "shadowed-constructor":
        changed = original.replace(
            "    result: Packet", "    Packet = 8\n    result: Packet"
        )
    elif mutation == "persistent-initializer":
        changed = original.replace(
            "    count: u8 = 0",
            "    saved: Packet = Packet(inner=Inner(flag=1, data=0), sequence=3)\n"
            "    count: u8 = 0",
        )
    else:
        assert constructor in original
        changed = original.replace(constructor, replacements[mutation])
    if mutation == "foreign-nominal":
        changed = changed.replace(
            "import Inner, Packet", "import ForeignPacket, Inner, Packet"
        )
    bench.write_text(changed)
    rejected = _cli(
        "compile",
        "-c",
        str(bench),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
        "-I",
        str(provider),
        "-o",
        str(output),
        "--replace",
    )
    assert rejected.returncode != 0
    assert diagnostic in rejected.stderr.lower(), rejected.stderr
    assert {
        path.name: path.read_bytes() for path in output.iterdir() if path.is_file()
    } == before


@pytest.mark.parametrize(
    "variant,spelling",
    [
        (False, "bits"),
        (True, "bits"),
        (False, "direct"),
        (False, "alias"),
        (False, "namespace"),
    ],
    ids=[
        "eight-bit",
        "renamed-five-bit",
        "direct-builtins",
        "aliased-builtins",
        "namespace-builtins",
    ],
)
def test_closed_system_generated_cpp_and_verilator_observations(
    tmp_path, variant, spelling
):
    _source_root, _dut, _bench, final = _program(tmp_path, variant, spelling)
    transcripts = []
    for target in ("cpp", "verilog"):
        binary = _bundle(final, tmp_path, target)
        workers = (1, 2) if target == "cpp" else (1,)
        for count in workers:
            arguments = (
                ["--cycles", "4", "--workers", str(count)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=20)))
            _assert_success(rows)
            transcripts.append([row for row in rows if row["kind"] != "result"])
        failed_arguments = ["--cycles", "5"] if target == "cpp" else ["+cycles=5"]
        failure = _run([str(binary), *failed_arguments], timeout=20)
        assert failure.returncode != 0, failure.stdout + failure.stderr
        rows = _records(failure.stdout)
        for kind, name in (("log", "left"), ("log", "right"), ("report", "progress")):
            assert len(_observations(rows, kind, name)) == 8
            assert all(row[0] < 8 for row in _observations(rows, kind, name))
        if target == "cpp":
            results = [row for row in rows if row["kind"] == "result"]
            assert len(results) == 1 and results[0]["status"] == "FAILED"
            assert int(results[0]["epoch_time"]) == 8
        else:
            assert "check failed at epoch 8" in failure.stdout + failure.stderr
        if not variant:
            # Invalid limits must refuse before producing observations. This
            # shares the already generated binary and exercises no long run.
            for token in ("0", "-1", "", "1x", str(MAX_CYCLES + 1), str(2**64)):
                arguments = (
                    ["--cycles", token] if target == "cpp" else [f"+cycles={token}"]
                )
                rejected = _run([str(binary), *arguments], timeout=20)
                assert rejected.returncode != 0, (target, token, rejected.stdout)
                assert _records(rejected.stdout) == [], (target, token, rejected.stdout)
                assert "cycles" in rejected.stdout + rejected.stderr
            # A value beyond 32 bits must remain intact, while the largest
            # admitted limit must remain valid. The source's deliberate
            # assertion keeps both executions bounded at eight epochs.
            for token in (str(2**32 + 1), str(MAX_CYCLES)):
                arguments = (
                    ["--cycles", token] if target == "cpp" else [f"+cycles={token}"]
                )
                wide = _run([str(binary), *arguments], timeout=20)
                assert wide.returncode != 0, (target, token, wide.stdout)
                wide_rows = _records(wide.stdout)
                for kind, name in (
                    ("log", "left"),
                    ("log", "right"),
                    ("report", "progress"),
                ):
                    assert _observations(wide_rows, kind, name) == _observations(
                        rows, kind, name
                    )
                if target == "cpp":
                    assert wide.returncode == 1
                    result = [row for row in wide_rows if row["kind"] == "result"]
                    assert len(result) == 1 and result[0]["status"] == "FAILED"
                    assert int(result[0]["epoch_time"]) == 8
                else:
                    assert "check failed at epoch 8" in wide.stdout + wide.stderr
    assert transcripts[0] == transcripts[1] == transcripts[2]


@pytest.mark.parametrize(
    "imported", [False, True], ids=["local-child", "imported-child"]
)
def test_system_cannot_be_a_structural_child_and_output_is_preserved(
    tmp_path, imported
):
    source = tmp_path / "source"
    source.mkdir()
    output = tmp_path / "output"
    filename = "parent.py"
    parent = source / filename
    parent.write_text(
        "from pycircuit import module\n@module\ndef Parent() -> {}:\n    return {}\n"
    )
    _ok(_compile(source, filename, output))
    before = {
        path.name: path.read_bytes() for path in output.iterdir() if path.is_file()
    }
    declaration = 'from pycircuit import system\n@system\ndef Provider():\n    """Closed test system."""\n'
    providers = []
    if imported:
        (source / "provider.py").write_text(declaration)
        provider = tmp_path / "provider"
        _ok(_compile(source, "provider.py", provider))
        providers = [provider]
        declaration = "from checks.provider import Provider\n"
    parent.write_text(
        declaration
        + "from pycircuit import module\n@module\ndef Parent() -> {}:\n    child = Provider()\n    return {}\n"
    )
    arguments = [
        "compile",
        "-c",
        str(parent),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
        "-o",
        str(output),
        "--replace",
    ]
    for provider in providers:
        arguments += ["-I", str(provider)]
    rejected = _cli(*arguments)
    assert rejected.returncode != 0
    assert "root-only" in rejected.stderr
    assert {
        path.name: path.read_bytes() for path in output.iterdir() if path.is_file()
    } == before


@pytest.mark.parametrize("artifact", ["body", "header"])
def test_root_kind_pair_tampering_is_rejected_without_replacing_program(
    tmp_path, artifact
):
    _source_root, dut, bench, final = _program(tmp_path)
    suffix = ".ac" if artifact == "body" else ".interface.ac"
    tampered = bench / ("bench" + suffix)
    original = tampered.read_text()
    assert 'ac.root_kind = "system"' in original
    tampered.write_text(original.replace(', ac.root_kind = "system"', ""))
    assert tampered.read_text() != original
    before = final.read_bytes()
    rejected = _cli(
        "link",
        str(dut),
        str(bench),
        "--top",
        "checks.bench.ExerciseAccumulator",
        "-o",
        str(final),
        "--replace",
    )
    assert rejected.returncode != 0
    assert final.read_bytes() == before
    # Also ask the native pair verifier directly, bypassing publication hashes.
    linker = Path(
        _environment().get("PYCIRCUIT_LINKER", str(_install() / "bin/pycircuit-link"))
    )
    report = tmp_path / "owner.json"
    pair = _run(
        [
            str(linker),
            "--body",
            str(bench / "bench.ac"),
            "--header",
            str(bench / "bench.interface.ac"),
            "--verify-only",
            "--unit-owner-out",
            str(report),
        ]
    )
    assert pair.returncode != 0
    assert not report.exists()


@pytest.mark.parametrize("marker", ["unit", "7 : i64", '"module"'])
def test_matching_invalid_root_kind_rejects_and_preserves_published_output(
    tmp_path, marker
):
    _source_root, dut, bench, final = _program(tmp_path)
    for filename in ("bench.ac", "bench.interface.ac"):
        artifact = bench / filename
        original = artifact.read_text()
        assert original.count('ac.root_kind = "system"') == 1
        artifact.write_text(
            original.replace('ac.root_kind = "system"', f"ac.root_kind = {marker}")
        )
    before = final.read_bytes()
    linker = Path(
        _environment().get("PYCIRCUIT_LINKER", str(_install() / "bin/pycircuit-link"))
    )
    report = tmp_path / "existing-owner.json"
    report.write_bytes(b"preserved owner output\n")
    pair = _run(
        [
            str(linker),
            "--body",
            str(bench / "bench.ac"),
            "--header",
            str(bench / "bench.interface.ac"),
            "--verify-only",
            "--unit-owner-out",
            str(report),
        ]
    )
    assert pair.returncode != 0
    assert "ac.root_kind must be 'system' when present" in pair.stderr
    assert report.read_bytes() == b"preserved owner output\n"
    rejected = _cli(
        "link",
        str(dut),
        str(bench),
        "--top",
        "checks.bench.ExerciseAccumulator",
        "-o",
        str(final),
        "--replace",
    )
    assert rejected.returncode != 0
    assert final.read_bytes() == before


@pytest.mark.parametrize("target", ["cpp", "verilog"])
def test_public_run_builds_and_executes_closed_system(tmp_path, target):
    source = _source(tmp_path)
    result = _cli(
        "run",
        str(source),
        "--target",
        target,
        "--toolchain",
        str(_install()),
        "--build-dir",
        str(tmp_path / "run build with spaces"),
        "--cycles",
        "4",
    )
    successful_rows = _records(_ok(result))
    _assert_success(successful_rows)
    _assert_run_receipt(tmp_path / "run build with spaces", target)
    if target == "verilog":
        failed = _cli(
            "run",
            str(source),
            "--target",
            target,
            "--toolchain",
            str(_install()),
            "--build-dir",
            str(tmp_path / "run build with spaces"),
            "--cycles",
            str(2**32 + 1),
        )
        assert failed.returncode == 1, failed.stdout + failed.stderr
        failed_rows = _records(failed.stdout)
        for kind, name in (("log", "left"), ("log", "right"), ("report", "progress")):
            assert _observations(failed_rows, kind, name) == _observations(
                successful_rows, kind, name
            )
        assert "check failed at epoch 8" in failed.stdout + failed.stderr
        _assert_run_receipt(tmp_path / "run build with spaces", target, "failed")


def test_parallel_public_run_backends_keep_independent_default_builds(tmp_path):
    source = _source(tmp_path)
    prefix = _install()

    def execute(target):
        return _run(
            [
                sys.executable,
                "-m",
                "pycircuit.cli",
                "run",
                str(source),
                "--target",
                target,
                "--toolchain",
                str(prefix),
                "--cycles",
                "4",
            ],
            cwd=tmp_path,
        )

    with ThreadPoolExecutor(max_workers=2) as pool:
        cpp, rtl = list(pool.map(execute, ("cpp", "verilog")))
    cpp_rows, rtl_rows = _records(_ok(cpp)), _records(_ok(rtl))
    _assert_success(cpp_rows)
    _assert_success(rtl_rows)
    assert [row for row in cpp_rows if row["kind"] != "result"] == [
        row for row in rtl_rows if row["kind"] != "result"
    ]
    caches = list((tmp_path / ".pycircuit_out/run").rglob("CMakeCache.txt"))
    configured = {}
    for cache in caches:
        for line in cache.read_text().splitlines():
            if line.startswith("PYC_EXAMPLE_RUN_TARGET:"):
                configured[line.partition("=")[2]] = cache.parent
    assert set(configured) == {"cpp", "verilog"}
    assert configured["cpp"] != configured["verilog"]
    for target, build in configured.items():
        _assert_run_receipt(build, target)


def test_public_run_rejects_a_reusable_module_bundle(tmp_path):
    source = _source(tmp_path)
    cmake = source / "CMakeLists.txt"
    cmake.write_text(
        cmake.read_text()
        .replace("checks.bench.ExerciseAccumulator", "checks.dut.Accumulator")
        .replace(
            "ENTRY_SOURCE bench SOURCES dut.py bench.py",
            "ENTRY_SOURCE dut SOURCES dut.py",
        )
    )
    build = tmp_path / "module build"
    result = _cli(
        "run",
        str(source),
        "--toolchain",
        str(_install()),
        "--build-dir",
        str(build),
        "--cycles",
        "1",
    )
    assert result.returncode != 0
    assert "run requires a source @system root" in result.stderr
    receipt = json.loads((build / "cpp/generated.json").read_text())
    assert "simulation_main.cpp" not in {item["path"] for item in receipt["files"]}
    assert not (build / "simulation").exists()


def _atomic_program(tmp_path):
    source = _source(tmp_path)
    (source / "bench.py").write_bytes((source / "atomic_bench.py").read_bytes())
    dut, bench = tmp_path / "dut", tmp_path / "bench"
    _ok(_compile(source, "dut.py", dut))
    _ok(_compile(source, "bench.py", bench, [dut]))
    final = tmp_path / "atomic.ac"
    _ok(
        _cli(
            "link",
            str(dut),
            str(bench),
            "--top",
            "checks.bench.AtomicFailure",
            "-o",
            str(final),
        )
    )
    return final


def _atomic_cpp(generated, tmp_path):
    # Test-owned access instrumentation of emitted headers only. The test reads
    # actual leaf Q and clock history; it never supplies a DUT result.
    probe = tmp_path / "cpp-probe"
    shutil.copytree(generated, probe)
    generated = probe
    for header in generated.rglob("*.hpp"):
        header.write_text(header.read_text().replace("private:", "public:"))
    dut_header = (generated / "sources/checks/dut.hpp").read_text()
    registers = re.findall(r"\b(pyc_instance_\w+_state);", dut_header)
    assert len(registers) == 1
    storage = registers[0]
    system_header = (generated / "pycircuit_system.hpp").read_text()
    inputs = re.search(r"struct Inputs \{(.*?)\};", system_header, re.S)
    assert inputs is not None
    assert len(re.findall(r"decltype\(", inputs[1])) == 2
    driver = generated / "atomic_probe.cpp"
    driver.write_text(
        """
#include "pycircuit_system.hpp"
#include <iostream>
int main() {
  const auto bit = [](bool v) { return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{v ? 1u : 0u}); };
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    dut.drive({bit(false), bit(false)});
    dut.system().Build();
    auto root = dut.root_->pyc_implementation;
    auto left = root->pyc_instance_left;
    auto right = root->pyc_instance_right;
    auto check = [&](unsigned q, bool clock) {
      for (const auto &leaf : {left, right}) {
        const auto &current = leaf->STORAGE->current(0);
        if (current.q.value().value() != q || current.clock != clock) return false;
      }
      return true;
    };
    for (unsigned retry = 0; retry < 2; ++retry) {
      dut.drive({bit(false), bit(false)});
      dut.system().Reset();
      if (!check(0, false) || dut.system().cycle() != 0) return 1;
      dut.drive({bit(true), bit(false)});
      if (dut.system().Step() != gfsim::SimStepResult::Failed) return 2;
      if (dut.system().failureInfo().phase != gfsim::SimFailurePhase::Check) return 3;
      if (!check(0, false) || dut.system().cycle() != 0) return 4;
      for (const auto &leaf : {left, right}) {
        if (leaf->pyc_instance_value_d.element(0).value().value() != 1) return 5;
        if (!leaf->pyc_instance_value_en.element(0).value().toBool()) return 6;
        if (!leaf->pyc_instance_value_clk.element(0).value().toBool()) return 7;
      }
      if (!dut.observations().Events().empty()) return 8;
      if (dut.system().Step() != gfsim::SimStepResult::Failed || !check(0, false)) return 9;
    }
    // Reset assertion suppresses ordinary source checks at a successful edge.
    dut.drive({bit(false), bit(false)}); dut.system().Reset();
    dut.drive({bit(true), bit(true)});
    if (dut.system().Step() != gfsim::SimStepResult::Running || !check(0, true)) return 10;
    const auto count = dut.observations().Events().size();
    if (count != 1) return 12;
    const auto epoch = dut.observations().Events().front().epoch;
    dut.drive({bit(false), bit(false)});
    if (dut.system().Step() != gfsim::SimStepResult::Failed || !check(0, true)) return 11;
    if (dut.system().cycle() != 1 || dut.observations().Events().size() != count ||
        dut.observations().Events().front().epoch != epoch) return 12;
  }
  std::cout << "ATOMIC_ZERO_COMMIT_OK\\n";
}
""".replace(
            "STORAGE", storage
        )
    )
    with (generated / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(atomic_probe atomic_probe.cpp)\n"
            "target_compile_features(atomic_probe PRIVATE cxx_std_20)\n"
            "target_link_libraries(atomic_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "atomic-cpp-build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(generated),
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
                "atomic_probe",
                "--parallel",
                "2",
            ]
        )
    )
    assert (
        _ok(_run([str(build / "atomic_probe")], timeout=20)).strip()
        == "ATOMIC_ZERO_COMMIT_OK"
    )


def _atomic_rtl(generated, tmp_path):
    # These private control pins belong to the generated simulation adapter.
    # This API test drives phases to inspect denied pending writes before fatal.
    testbench = tmp_path / "atomic_probe.sv"
    testbench.write_text(
        """
module atomic_probe;
  logic clk=0, rst=0;
  logic [2:0] phase=0;
  logic permit=0;
  wire error;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .pyc_phase(phase), .pyc_root_commit_ok(permit), .pyc_local_error(error));
  task automatic check_current(input integer q, input logic clock);
    if (dut.dut.pyc_instance_left.pyc_instance_value.q_current !== q ||
        dut.dut.pyc_instance_right.pyc_instance_value.q_current !== q ||
        dut.dut.pyc_instance_left.pyc_instance_value.managed.clock_current !== clock ||
        dut.dut.pyc_instance_right.pyc_instance_value.managed.clock_current !== clock)
      $fatal(1, "committed state or clock changed");
  endtask
  task automatic reset_model;
    clk=0; rst=0; permit=1; phase=4; #1;
    phase=2; #1; phase=0; permit=0; #1;
    check_current(0,0);
  endtask
  initial begin
    #1;
    for (integer retry=0; retry<2; retry=retry+1) begin
      reset_model();
      clk=1; phase=1; #1;
      if (!error) $fatal(1,"source assertion was not checked");
      if (dut.dut.pyc_instance_left.pyc_instance_value.managed.q_pending !== 1 ||
          dut.dut.pyc_instance_right.pyc_instance_value.managed.q_pending !== 1 ||
          dut.dut.pyc_instance_left.pyc_instance_value.managed.clock_pending !== 1 ||
          dut.dut.pyc_instance_right.pyc_instance_value.managed.clock_pending !== 1)
        $fatal(1,"both DUTs must propose a state and clock update");
      phase=2; #1; check_current(0,0);
      phase=3; #1; phase=0; #1;
      phase=1; #1; phase=3; #1; check_current(0,0);
    end
    reset_model();
    clk=1; rst=1; phase=1; #1;
    if (error) $fatal(1,"reset did not suppress the ordinary assertion");
    permit=1; phase=2; #1; phase=0; permit=0; #1; check_current(0,1);
    clk=0; rst=0; phase=1; #1;
    if (!error) $fatal(1,"ordinary assertion should fail after reset release");
    phase=3; #1; check_current(0,1);
    $display("ATOMIC_ZERO_COMMIT_OK"); $finish;
  end
endmodule
"""
    )
    binary = tmp_path / "atomic_rtl"
    runtime = _install() / "include/verilog/dffe.v"
    _ok(
        _run(
            [
                "verilator",
                "--binary",
                "--timing",
                "-CFLAGS",
                "-std=c++20",
                "--Wno-fatal",
                "--top-module",
                "atomic_probe",
                "--Mdir",
                str(tmp_path / "atomic-verilated"),
                "-o",
                str(binary),
                str(runtime),
                str(generated / "design_top.sv"),
                str(generated / "sources/checks/dut.v"),
                str(generated / "sources/checks/bench.v"),
                str(testbench),
            ]
        )
    )
    stdout = _ok(_run([str(binary)], timeout=20))
    assert "ATOMIC_ZERO_COMMIT_OK" in stdout
    assert _records(stdout) == []


@pytest.mark.parametrize("target", ["cpp", "verilog"])
def test_failed_assertion_discards_two_dut_registers_and_clock_history(
    tmp_path, target
):
    final = _atomic_program(tmp_path)
    generated = tmp_path / target
    _ok(_cli("emit", str(final), "--target", target, "-o", str(generated)))
    if target == "cpp":
        _atomic_cpp(generated, tmp_path)
    else:
        _atomic_rtl(generated, tmp_path)
