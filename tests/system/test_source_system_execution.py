"""Closed source systems execute using only compiler-generated harnesses.

The numerical oracle below follows declared register updates and old-Q reads.
It is independent of generated signal names, graph shape, and process status.
"""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
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
                str(ROOT / "python/pycircuit/src"),
                env.get("PYTHONPATH"),
            ],
        )
    )
    return env


def _run(arguments: list[str], *, timeout: int = 120):
    return subprocess.run(
        arguments,
        env=_environment(),
        text=True,
        capture_output=True,
        check=False,
        timeout=timeout,
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


def _source(tmp_path: Path, variant: bool = False) -> Path:
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


def _program(tmp_path: Path, variant=False):
    source = _source(tmp_path, variant)
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


@pytest.mark.parametrize(
    "variant", [False, True], ids=["eight-bit", "renamed-five-bit"]
)
def test_closed_system_generated_cpp_and_verilator_observations(tmp_path, variant):
    _source_root, _dut, _bench, final = _program(tmp_path, variant)
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
""".replace("STORAGE", storage)
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
    testbench.write_text("""
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
""")
    binary = tmp_path / "atomic_rtl"
    runtime = _install() / "include/verilog/dffe.v"
    _ok(
        _run(
            [
                "verilator",
                "--binary",
                "--timing",
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
