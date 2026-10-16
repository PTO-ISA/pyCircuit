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
                os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL", "2"),
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


def _assert_closed_observation_identities(rows):
    stable = {}
    for row in rows:
        if row["kind"] == "result":
            continue
        registration = json.loads(row["registration"])
        assert set(registration) == {"site", "expansion"}, registration
        identity = json.loads(row["site"])
        assert set(identity) == {"site", "invocation", "ordinal"}, identity
        assert identity["invocation"] == [], identity
        assert isinstance(identity["ordinal"], int) and identity["ordinal"] >= 0
        key = (row["instance"], row["kind"], json.dumps(row["spec"], sort_keys=True))
        value = (row["registration"], row["site"])
        if key in stable:
            assert stable[key] == value, (key, stable[key], value)
        else:
            stable[key] = value


def _method_program(tmp_path):
    source = tmp_path / "method system sources"
    source.mkdir()
    for filename in ("method_effects_provider.py", "method_effects_bench.py"):
        shutil.copy2(FIXTURE / filename, source)
    provider = tmp_path / "method provider unit"
    consumer = tmp_path / "method consumer unit"
    _ok(_compile(source, "method_effects_provider.py", provider))
    (source / "method_effects_provider.py").unlink()
    receipt = json.loads((provider / "unit.json").read_text())
    provider_body = provider / receipt["files"]["body"]
    hidden_body = tmp_path / "unavailable provider body.ac"
    provider_body.rename(hidden_body)
    try:
        assert not provider_body.exists()
        _ok(_compile(source, "method_effects_bench.py", consumer, [provider]))
    finally:
        hidden_body.rename(provider_body)
    assert not (source / "method_effects_provider.py").exists()

    transaction_input = tmp_path / "method transaction input.ac"
    transaction_output = tmp_path / "method transaction output.ac"
    final = tmp_path / "method system.ac"
    linker = os.environ.get("PYCIRCUIT_LINKER")
    assert linker, "set PYCIRCUIT_LINKER to this candidate's native linker"
    arguments = [linker]
    for unit in (provider, consumer):
        receipt = json.loads((unit / "unit.json").read_text())
        arguments += [
            "--body",
            str(unit / receipt["files"]["body"]),
            "--header",
            str(unit / receipt["files"]["interface"]),
        ]
    arguments += [
        "--top",
        "checks.method_effects_bench.ExerciseMethodEffects",
        "--target",
        "final",
        "--output",
        str(final),
        "--transaction-input-out",
        str(transaction_input),
        "--transaction-output-out",
        str(transaction_output),
    ]
    _ok(_run(arguments))
    assert transaction_input.is_file() and transaction_output.is_file()
    input_text = transaction_input.read_text()
    output_text = transaction_output.read_text()
    final_text = final.read_text()
    assert '"ac.method.call"' in input_text
    assert "ac.sampled_admission" in output_text
    assert '"ac.method.call"' not in output_text
    assert output_text != input_text
    assert final_text.count('"ac.observe"(') == 6
    assert final_text.count("ac.observation_id") == 6
    assert final_text.count('kind = "assert"') >= 2
    assert "invocation" in final_text and "owner" in final_text
    return final, transaction_input, transaction_output


def _branch_method_units(tmp_path):
    source = tmp_path / "branch method system sources"
    source.mkdir()
    for filename in ("method_branch_provider.py", "method_branch_bench.py"):
        shutil.copy2(FIXTURE / filename, source)
    provider = tmp_path / "branch method provider unit"
    consumer = tmp_path / "branch method consumer unit"
    _ok(_compile(source, "method_branch_provider.py", provider))
    provider_source = source / "method_branch_provider.py"
    provider_source.unlink()
    receipt = json.loads((provider / "unit.json").read_text())
    provider_body = provider / receipt["files"]["body"]
    hidden_body = tmp_path / "unavailable branch provider body.ac"
    provider_body.rename(hidden_body)
    try:
        assert not provider_body.exists()
        _ok(_compile(source, "method_branch_bench.py", consumer, [provider]))
    finally:
        hidden_body.rename(provider_body)
    assert not provider_source.exists()
    return source, provider, consumer


def _conditional_method_program(tmp_path, *, probe=False):
    source = tmp_path / "conditional method sources"
    source.mkdir()
    provider_name = "method_conditional_provider.py"
    consumer_name = (
        "method_conditional_probe.py" if probe else "method_conditional_bench.py"
    )
    for filename in (provider_name, consumer_name):
        shutil.copy2(FIXTURE / filename, source)
    provider = tmp_path / "conditional provider unit"
    consumer = tmp_path / "conditional consumer unit"
    _ok(_compile(source, provider_name, provider))
    (source / provider_name).unlink()
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "unavailable conditional provider body.ac"
    body.rename(hidden)
    try:
        assert not body.exists() and not (source / provider_name).exists()
        _ok(_compile(source, consumer_name, consumer, [provider]))
    finally:
        hidden.rename(body)

    before = tmp_path / "conditional transaction input.ac"
    after = tmp_path / "conditional transaction output.ac"
    final = tmp_path / "conditional system.ac"
    linker = os.environ.get("PYCIRCUIT_LINKER")
    assert linker, "set PYCIRCUIT_LINKER to this candidate's native linker"
    arguments = [linker]
    for unit in (provider, consumer):
        receipt = json.loads((unit / "unit.json").read_text())
        arguments += [
            "--body",
            str(unit / receipt["files"]["body"]),
            "--header",
            str(unit / receipt["files"]["interface"]),
        ]
    name = (
        "method_conditional_probe.ConditionalProbe"
        if probe
        else "method_conditional_bench.ExerciseConditionalMethods"
    )
    arguments += [
        "--top",
        f"checks.{name}",
        "--target",
        "final",
        "--output",
        str(final),
        "--transaction-input-out",
        str(before),
        "--transaction-output-out",
        str(after),
    ]
    _ok(_run(arguments))
    assert '"ac.method.call"' in before.read_text()
    assert "ac.branch" in before.read_text()
    assert '"ac.method.call"' not in after.read_text()
    assert "ac.branch" not in after.read_text()
    assert "ac.sampled_admission" in after.read_text()
    return final, before, after


def _assert_conditional_method_rows(rows):
    observations = [row for row in rows if row["kind"] != "result"]
    assert len(observations) == 52
    stable = None
    for epoch in range(4):
        current = [row for row in observations if int(row["evaluation_epoch"]) == epoch]
        left, right = (5, 5) if epoch < 2 else (11, 13)
        expected = [
            ("log", "conditional old before", [left, right], "root"),
            ("log", "conditional add", [1, left + 1], "root/left"),
            ("log", "conditional add", [2, left + 3], "root/left"),
            ("log", "conditional add", [3, left + 6], "root/left"),
            (
                "log",
                "conditional snapshots",
                [left + 1, left + 3, left + 6],
                "root/left",
            ),
            ("report", "conditional count", [left + 6], "root/left"),
            ("log", "conditional add", [1, right + 1], "root/right"),
            ("log", "conditional add", [4, right + 5], "root/right"),
            ("log", "conditional add", [3, right + 8], "root/right"),
            (
                "log",
                "conditional snapshots",
                [right + 1, right + 5, right + 8],
                "root/right",
            ),
            ("report", "conditional count", [right + 8], "root/right"),
            ("log", "conditional results", [left + 6, right + 8], "root"),
            ("log", "conditional old after", [left, right], "root"),
        ]
        actual = [
            (
                row["kind"],
                row["spec"].get("event", row["spec"].get("name")),
                [int(value["value"]) for value in row["values"]],
                row["instance"],
            )
            for row in current
        ]
        assert actual == expected
        assert all(int(row["commit_epoch"]) == epoch + 1 for row in current)
        identities = [(row["registration"], row["site"]) for row in current]
        if stable is None:
            stable = identities
        else:
            assert identities == stable
        additions = [
            json.loads(row["site"])
            for row in current
            if row["spec"].get("event") == "conditional add"
        ]
        assert len({json.dumps(row["site"], sort_keys=True) for row in additions}) == 1
        assert (
            len({json.dumps(row["invocation"], sort_keys=True) for row in additions})
            == 6
        )
        assert all(len(row["invocation"]) >= 2 for row in additions)
        assert all(
            json.loads(row["site"])["invocation"] == []
            for row in (current[0], current[-2], current[-1])
        )
    terminal = [row for row in rows if row["kind"] == "result"]
    assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"
    assert int(terminal[0]["epoch_time"]) == 4
    gauges = [
        row for row in terminal[0]["statistics"] if row["name"] == "conditional count"
    ]
    assert {row["object_path"]: int(row["value"]) for row in gauges} == {
        "root/left": 17,
        "root/right": 21,
    }
    assert all(int(row["last_update"]["time"]) == 4 for row in gauges)


def _conditional_probe_rtl_observation_oracle(generated):
    harness = (generated / "simulation_top.sv").read_text()
    support = harness[
        harness.index("  bit pyc_observation_groups_ok;") : harness.index(
            "  task automatic pyc_step("
        )
    ]
    publish = support[support.index("  task automatic pyc_publish_observations;") :]
    groups = re.findall(
        r"if \(pyc_observation_active_(\d+)\) begin\n(.*?)      end", publish, re.S
    )
    # Match semantic groups before borrowing their emitted slot locations. The
    # expected source values below never come from generated payload contents.
    expected = [
        ("root/owner", "log", "conditional add", ["1", "6"], "fire"),
        ("root/owner", "log", "conditional add", ["2", "8"], "fire && chosen"),
        ("root", "log", "conditional true", ["8"], "fire && chosen"),
        ("root/owner", "log", "conditional add", ["4", "10"], "fire && !chosen"),
        ("root", "log", "conditional false", ["10"], "fire && !chosen"),
        ("root/owner", "log", "conditional add", ["3", "chosen ? 11 : 13"], "fire"),
        ("root/sibling", "log", "conditional add", ["1", "6"], "fire"),
        ("root/owner", "report", "conditional count", ["chosen ? 11 : 13"], "fire"),
        (
            "root",
            "log",
            "probe snapshots",
            ["6", "chosen ? 8 : 10", "chosen ? 11 : 13"],
            "fire",
        ),
    ]
    assert len(groups) == len(expected)
    checks = []
    report_slot = None
    for (first, body), (instance, kind, name, values, path) in zip(
        groups, expected, strict=True
    ):
        display = re.search(r'\$display\(("(?:\\.|[^"\\])*")', body)
        assert display is not None
        descriptor = json.loads(json.loads(display[1]))
        assert descriptor["instance"] == instance and descriptor["kind"] == kind
        assert descriptor["spec"].get("event", descriptor["spec"].get("name")) == name
        slots = re.findall(
            r"\$unsigned\(dut\.dut\.pyc_observation_value_(\d+)\)",
            body[display.end() :],
        )
        assert len(slots) == len(values) and slots[0] == first
        if kind == "report":
            report_slot = first
        for slot, value in zip(slots, values, strict=True):
            checks += [
                f"    if (dut.dut.pyc_observation_pending_{slot} !== 1'b1 ||",
                f"        dut.dut.pyc_observation_path_{slot} !== ({path}))",
                f'      $fatal(1,"conditional observation pending/path slot {slot}");',
                f"    if (({path}) && (dut.dut.pyc_observation_valid_{slot} !== 1'b1 ||",
                f"        dut.dut.pyc_observation_value_{slot} !== 8'({value})))",
                f'      $fatal(1,"conditional observation value/valid slot {slot}");',
            ]
        checks += [
            f"    if (pyc_observation_active_{first} !== ({path}))",
            f'      $fatal(1,"conditional observation group activation {first}");',
        ]
    assert report_slot is not None
    declarations = "\n".join(
        re.findall(
            r"^  longint unsigned pyc_report_(?:value|update)_\d+ = 0;$", harness, re.M
        )
    )
    oracle = "\n".join(checks)
    return (
        declarations
        + "\n  longint unsigned pyc_epoch=0, expected_report_value=0, expected_report_update=0;\n"
        + "  logic expected_fire=0, expected_chosen=0;\n"
        + support
        + """
  task automatic check_observation_snapshots;
    logic fire, chosen;
    chosen=(`SELECTOR === 1'b1);
    fire=(`PRIOR === 1'b1) && (`LATER === 1'b1) &&
         ((`SELECTOR === 1'b0) || (chosen && (`ALLOW === 1'b1)));
    expected_fire=fire; expected_chosen=chosen;
    pyc_validate_observations(pyc_observation_groups_ok);
    if (pyc_observation_groups_ok !== 1'b1)
      $fatal(1,"conditional observation groups inconsistent");
"""
        + oracle
        + "\n  endtask\n",
        report_slot,
    )


def _conditional_probe_four_state_rtl(final, tmp_path):
    generated, rtl = _emit_branch_probe_rtl(final, tmp_path)
    observation_support, report_slot = _conditional_probe_rtl_observation_oracle(
        generated
    )
    testbench = tmp_path / "conditional_four_state.sv"
    testbench.write_text(
        r"""
module conditional_four_state;
  logic clk=0, rst=0, permit=0;
  logic [2:0] phase=0;
  wire complete, sampling_error, error;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .pyc_phase(phase), .pyc_root_commit_ok(permit),
    .pyc_check_complete(complete), .pyc_sampling_error(sampling_error),
    .pyc_local_error(error));
  `define R dut.dut
  `define OWNER `R.pyc_instance_owner.pyc_instance_count
  `define SIBLING `R.pyc_instance_sibling.pyc_instance_count
  `define SELECTOR `R.pyc_instance_selector.q_current
  `define ALLOW `R.pyc_instance_allow.q_current
  `define PRIOR `R.pyc_instance_prior.q_current
  `define LATER `R.pyc_instance_later.q_current
  `define EARLY `R.pyc_instance_early_check.q_current
  `define ASSERTION `R.pyc_instance_branch_check.q_current

__OBSERVATION_SUPPORT__

  function automatic logic four(input integer n);
    case(n) 0: four=0; 1: four=1; 2: four=1'bx; 3: four=1'bz; endcase
  endfunction
  task automatic prepare;
    #1; phase=1; #1; check_observation_snapshots();
  endtask
  task automatic commit;
    permit=1; phase=2; #1;
    $display("CONDITIONAL_COMMIT %0d %0d %0d",pyc_epoch,expected_fire,expected_chosen);
    pyc_publish_observations();
    if (expected_fire) begin
      expected_report_value=expected_chosen ? 11 : 13;
      expected_report_update=pyc_epoch+1;
    end
    if (pyc_report_value___REPORT_SLOT__ !== expected_report_value ||
        pyc_report_update___REPORT_SLOT__ !== expected_report_update)
      $fatal(1,"conditional prior report value/update was not preserved");
    pyc_epoch=pyc_epoch+1;
    phase=0; permit=0; #1;
  endtask
  task automatic discard;
    permit=0; phase=2; #1; phase=3; #1; phase=0; #1;
  endtask
  task automatic check_current(input logic [7:0] owner, sibling, input logic history);
    if (`OWNER.q_current !== owner || `SIBLING.q_current !== sibling ||
        `OWNER.managed.clock_current !== history ||
        `SIBLING.managed.clock_current !== history ||
        `R.pyc_instance_selector.managed.clock_current !== history ||
        `R.pyc_instance_allow.managed.clock_current !== history ||
        `R.pyc_instance_prior.managed.clock_current !== history ||
        `R.pyc_instance_later.managed.clock_current !== history ||
        `R.pyc_instance_early_check.managed.clock_current !== history ||
        `R.pyc_instance_branch_check.managed.clock_current !== history)
      $fatal(1,"conditional state/history changed unexpectedly");
  endtask
  task automatic start(input integer timing);
    clk=0; rst=0; permit=1; phase=4; #1; phase=2; #1;
    phase=0; permit=0; #1; check_current(5,5,0);
    pyc_epoch=0; pyc_report_value___REPORT_SLOT__=0; pyc_report_update___REPORT_SLOT__=0;
    expected_report_value=0; expected_report_update=0;
    // Publish on a low Work first, so each later inactive/failing matrix case
    // has a real prior report value/update and previously emitted events.
    prepare(); commit(); check_current(5,5,0);
    if (timing==2 || timing==3) begin
      clk=1; rst=1; prepare(); commit(); rst=0;
      check_current(5,5,1);
    end
    clk=(timing==1 || timing==2 || timing==4);
    rst=(timing==4);
    `SELECTOR=0; `ALLOW=1; `PRIOR=1; `LATER=1; `EARLY=1; `ASSERTION=1;
  endtask
  task automatic expect_error(input logic expected);
    if (error !== expected || sampling_error !== 0 || complete !== 1)
      $fatal(1,"conditional check mismatch: expected %b, got %b/%b/%b", expected,error,sampling_error,complete);
  endtask
  initial begin
    #1;
    // All selector states at low/rising/held/falling/reset. Unselected arms
    // never supply values or checks; unknown selection faults only at rising.
    for (integer timing=0; timing<5; timing=timing+1) begin
      for (integer s=0; s<4; s=s+1) begin
        start(timing); `SELECTOR=four(s); prepare();
        expect_error(timing==1 && s>=2);
        if (timing==1 && s>=2) begin
          discard(); check_current(5,5,0);
        end else begin
          commit();
          check_current((timing==1 && s<2) ? (s==1 ? 11 : 13) : 5,
                        timing==1 && s<2 ? 6 : 5, clk);
        end
      end
      // A selected ordinary assertion retains its every-Work reset mask.
      for (integer a=0; a<4; a=a+1) begin
        start(timing); `SELECTOR=1; `ASSERTION=four(a); prepare();
        expect_error(timing!=4 && a!=1);
        discard(); check_current(5,5,timing==2 || timing==3);
        start(timing); `SELECTOR=0; `ASSERTION=four(a); prepare();
        expect_error(0); discard(); check_current(5,5,timing==2 || timing==3);
      end
      // Every false/X/Z callee guard bypasses when its arm is unselected.
      for (integer g=0; g<4; g=g+1) begin
        start(timing); `SELECTOR=0; `ALLOW=four(g); prepare();
        expect_error(0); discard(); check_current(5,5,timing==2 || timing==3);
        start(timing); `SELECTOR=1; `ALLOW=four(g); prepare();
        expect_error(timing==1 && g>=2);
        if (timing==1 && g==0) begin
          commit(); check_current(5,5,1);
        end else begin
          discard(); check_current(5,5,timing==2 || timing==3);
        end
      end
      // Final transaction cancellation masks assertions preceding the branch.
      start(timing); `SELECTOR=1'bx; `EARLY=0; prepare();
      expect_error(timing==1); discard(); check_current(5,5,timing==2 || timing==3);
      start(timing); `SELECTOR=1; `LATER=0; `EARLY=0; `ASSERTION=1'bz; prepare();
      expect_error(0); commit(); check_current(5,5,clk);
    end
    // A prior false guard masks unknown selection, unlike a later false guard.
    for (integer u=2; u<4; u=u+1) begin
      start(1); `PRIOR=0; `SELECTOR=four(u); `ALLOW=four(u); `EARLY=0;
      prepare(); expect_error(0); commit(); check_current(5,5,1);
      start(1); `SELECTOR=four(u); `LATER=0;
      prepare(); expect_error(1); discard(); check_current(5,5,0);
      start(1); `SELECTOR=1; `ALLOW=four(u); `LATER=0;
      prepare(); expect_error(1); discard(); check_current(5,5,0);
    end
    // Discard a fully successful proposal and retry the same rising edge.
    start(1); `SELECTOR=1; prepare(); expect_error(0); discard(); check_current(5,5,0);
    prepare(); expect_error(0); commit(); check_current(11,6,1);
    $display("CONDITIONAL_FOUR_STATE_OK"); $finish;
  end
endmodule
""".replace(
            "__OBSERVATION_SUPPORT__", observation_support
        ).replace(
            "__REPORT_SLOT__", report_slot
        )
    )
    binary = tmp_path / "conditional_four_state"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "conditional_four_state",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dffe.v"),
                *[str(path) for path in rtl if path.name != "simulation_top.sv"],
                str(testbench),
            ]
        )
    )
    stdout = _ok(_run(["vvp", str(binary)], timeout=30))
    assert "CONDITIONAL_FOUR_STATE_OK" in stdout
    commits = re.split(r"^CONDITIONAL_COMMIT (\d+) ([01]) ([01])\n", stdout, flags=re.M)
    assert _records(commits[0]) == []
    assert len(commits) > 4
    for index in range(1, len(commits), 4):
        epoch, active, chosen = map(int, commits[index : index + 3])
        rows = _records(commits[index + 3])
        expected = (
            [
                ("conditional add", [1, 6]),
                ("conditional add", [2, 8] if chosen else [4, 10]),
                (
                    "conditional true" if chosen else "conditional false",
                    [8 if chosen else 10],
                ),
                ("conditional add", [3, 11 if chosen else 13]),
                ("conditional add", [1, 6]),
                ("conditional count", [11 if chosen else 13]),
                ("probe snapshots", [6, 8 if chosen else 10, 11 if chosen else 13]),
            ]
            if active
            else []
        )
        assert [
            (
                row["spec"].get("event", row["spec"].get("name")),
                [int(value["value"]) for value in row["values"]],
            )
            for row in rows
        ] == expected
        assert all(
            int(row["evaluation_epoch"]) == epoch
            and int(row["commit_epoch"]) == epoch + 1
            for row in rows
        )
    return generated, rtl


def _conditional_probe_cpp(final, tmp_path):
    generated = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(generated)))
    # Expose emitted members only, as in _atomic_cpp. The probe supplies fault
    # stimuli to real storage and observes the ordinary SimSystem coordinator.
    for header in generated.rglob("*.hpp"):
        header.write_text(header.read_text().replace("private:", "public:"))
    driver = generated / "conditional_probe.cpp"
    driver.write_text(
        r"""
#include "pycircuit_system.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>
using Bit = gfsim::wire<gfsim::Bits<1>>;
static Bit four(unsigned s) {
  if (s < 2) return Bit::known(gfsim::Bits<1>{s});
  return Bit::fromPacked(s == 2 ? gfsim::FourState<1>::unknown()
                               : gfsim::FourState<1>::highImpedance());
}
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
template<class Storage, class Output>
void inject(Storage &storage, Output &output, unsigned s) {
  using Current = std::remove_cvref_t<decltype(storage->current(0))>;
  auto &state = const_cast<Current &>(storage->current(0));
  state.q = four(s);
  output.element(0) = state.q;
}
int main() {
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    auto drive = [&](bool clock, bool reset) { dut.drive({four(clock), four(reset)}); };
    drive(false,false); dut.system().Build();
    auto root = dut.root_->pyc_implementation;
    auto owner = root->pyc_instance_owner;
    auto sibling = root->pyc_instance_sibling;
    auto snapshot = [&]() {
      std::vector<std::uint64_t> result;
      auto add = [&](const auto &storage) {
        const auto &q = storage->current(0);
        result.insert(result.end(), {q.q.packed().value().value(),
          q.q.packed().knownMask().value(), q.q.packed().zMask().value(), q.clock});
      };
      add(owner->pyc_instance_count_state); add(sibling->pyc_instance_count_state);
      add(root->pyc_instance_selector_state); add(root->pyc_instance_allow_state);
      add(root->pyc_instance_prior_state); add(root->pyc_instance_later_state);
      add(root->pyc_instance_early_check_state); add(root->pyc_instance_branch_check_state);
      return result;
    };
    auto current = [&](unsigned q, unsigned other, bool history) {
      require(owner->pyc_instance_count_state->current(0).q.value().value() == q, "owner Q");
      require(sibling->pyc_instance_count_state->current(0).q.value().value() == other, "sibling Q");
      const auto state = snapshot();
      for (unsigned i=3; i<state.size(); i+=4) require(state[i] == history, "clock history");
    };
    auto start = [&](unsigned timing) {
      drive(false,false); dut.system().Reset(); current(5,5,false);
      require(dut.system().cycle()==0 && dut.observations().Events().empty(), "reset epoch/events");
      for (const auto &g : dut.observations().Gauges())
        require(g.value.bits==0 && g.lastUpdate==0, "reset gauge");
      if (timing==2 || timing==3) {
        drive(true,true);
        require(dut.system().Step()==gfsim::SimStepResult::Running, "history setup");
        current(5,5,true);
      }
      drive(timing==1 || timing==2 || timing==4, timing==4);
    };
    auto selector = [&](unsigned s) { inject(root->pyc_instance_selector_state,root->pyc_instance_selector_q,s); };
    auto allow = [&](unsigned s) { inject(root->pyc_instance_allow_state,root->pyc_instance_allow_q,s); };
    auto prior = [&](unsigned s) { inject(root->pyc_instance_prior_state,root->pyc_instance_prior_q,s); };
    auto later = [&](unsigned s) { inject(root->pyc_instance_later_state,root->pyc_instance_later_q,s); };
    auto early = [&](unsigned s) { inject(root->pyc_instance_early_check_state,root->pyc_instance_early_check_q,s); };
    auto assertion = [&](unsigned s) { inject(root->pyc_instance_branch_check_state,root->pyc_instance_branch_check_q,s); };
    auto step = [&](bool fails, bool inactive) {
      const auto state = snapshot();
      const auto epoch = dut.system().cycle();
      const auto events = std::vector(dut.observations().Events().begin(),dut.observations().Events().end());
      const auto gauges = std::vector(dut.observations().Gauges().begin(),dut.observations().Gauges().end());
      const auto result = dut.system().Step();
      if (fails) {
        require(result==gfsim::SimStepResult::Failed, "expected check failure");
        require(dut.system().failureInfo().phase==gfsim::SimFailurePhase::Check, "failure phase");
        require(snapshot()==state && dut.system().cycle()==epoch, "zero state/history/epoch commit");
        require(std::vector(dut.observations().Events().begin(),dut.observations().Events().end())==events, "prior events preserved");
        require(std::vector(dut.observations().Gauges().begin(),dut.observations().Gauges().end())==gauges, "prior gauges preserved");
        require(dut.system().Step()==gfsim::SimStepResult::Failed && snapshot()==state && dut.system().cycle()==epoch, "sticky failure");
      } else {
        require(result==gfsim::SimStepResult::Running && dut.system().cycle()==epoch+1, "successful epoch");
        require(dut.observations().Events().empty()==inactive, "effect activation");
        if (!inactive) {
          require(dut.observations().Gauges().size()==1, "gauge inventory");
          require(dut.observations().Gauges()[0].lastUpdate==epoch+1, "gauge epoch");
        } else {
          require(std::vector(dut.observations().Gauges().begin(),dut.observations().Gauges().end())==gauges, "inactive gauge hold");
        }
      }
    };
    for (unsigned timing=0; timing<5; ++timing) {
      for (unsigned s=0; s<4; ++s) {
        start(timing); selector(s); step(timing==1 && s>=2,s>=2);
        if (!(timing==1 && s>=2))
          current(timing==1 && s<2 ? (s==1?11:13) : 5,
                  timing==1 && s<2 ? 6 : 5,timing==1 || timing==2 || timing==4);
      }
      for (unsigned a : {0u,2u,3u}) {
        start(timing); selector(1); assertion(a); step(timing!=4,false);
        start(timing); selector(0); assertion(a); step(false,false);
      }
      for (unsigned g : {0u,2u,3u}) {
        start(timing); selector(0); allow(g); step(false,false);
        start(timing); selector(1); allow(g); step(timing==1 && g>=2,true);
        if (!(timing==1 && g>=2)) current(5,5,timing==1 || timing==2 || timing==4);
      }
      start(timing); selector(2); early(0); step(timing==1,true);
      start(timing); selector(1); later(0); early(0); assertion(3); step(false,true);
    }
    for (unsigned u : {2u,3u}) {
      start(1); prior(0); selector(u); allow(u); early(0); step(false,true); current(5,5,true);
      start(1); selector(u); later(0); step(true,true);
      start(1); selector(1); allow(u); later(0); step(true,true);
    }
    // Preserve a real prior publication on a later failed rising edge, then
    // demonstrate that only explicit Reset clears the sticky failure.
    start(0); step(false,false);
    selector(2); drive(true,false); step(true,true);
    selector(0); require(dut.system().Step()==gfsim::SimStepResult::Failed,"failure recovered without Reset");
    start(1); step(false,false); current(13,6,true);
    start(1); selector(1);
    const auto state = snapshot();
    dut.root_->Work(); dut.root_->DiscardNext(); dut.observations().DiscardNext();
    require(snapshot()==state && dut.system().cycle()==0 && dut.observations().Events().empty(),"Discard committed work");
    step(false,false); current(11,6,true);
  }
  std::cout << "CONDITIONAL_CPP_FOUR_STATE_OK\n";
}
"""
    )
    with (generated / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(conditional_probe conditional_probe.cpp)\n"
            "target_compile_features(conditional_probe PRIVATE cxx_std_20)\n"
            "target_link_libraries(conditional_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "conditional-cpp-build"
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
                "conditional_probe",
                "--parallel",
                os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL", "2"),
            ]
        )
    )
    assert (
        _ok(_run([str(build / "conditional_probe")], timeout=30)).strip()
        == "CONDITIONAL_CPP_FOUR_STATE_OK"
    )


def _conditional_probe_system_precommit(generated, rtl, tmp_path):
    text = (generated / "simulation_top.sv").read_text()
    reports = re.findall(r"longint unsigned pyc_report_value_(\d+) =", text)
    assert len(reports) == 1
    report = reports[0]
    monitor = tmp_path / "conditional_system_precommit.sv"
    monitor.write_text(
        f"""
module conditional_system_precommit;
  logic captured=0;
  logic [7:0] owner_q, sibling_q;
  logic [7:0] histories;
  longint unsigned report_value, report_update;
  `define R pycircuit_sim.dut.dut
  function automatic logic [7:0] history;
    history={{`R.pyc_instance_owner.pyc_instance_count.managed.clock_current,
      `R.pyc_instance_sibling.pyc_instance_count.managed.clock_current,
      `R.pyc_instance_selector.managed.clock_current,
      `R.pyc_instance_allow.managed.clock_current,
      `R.pyc_instance_prior.managed.clock_current,
      `R.pyc_instance_later.managed.clock_current,
      `R.pyc_instance_early_check.managed.clock_current,
      `R.pyc_instance_branch_check.managed.clock_current}};
  endfunction
  initial begin
    wait(pycircuit_sim.pyc_epoch==1 && pycircuit_sim.pyc_phase==0);
    `R.pyc_instance_selector.q_current=1'bx;
    #0;
    owner_q=`R.pyc_instance_owner.pyc_instance_count.q_current;
    sibling_q=`R.pyc_instance_sibling.pyc_instance_count.q_current;
    histories=history();
    report_value=pycircuit_sim.pyc_report_value_{report};
    report_update=pycircuit_sim.pyc_report_update_{report};
    captured=1;
  end
  final begin
    if (captured!==1 || pycircuit_sim.pyc_epoch!==1 ||
        `R.pyc_instance_owner.pyc_instance_count.q_current!==owner_q ||
        `R.pyc_instance_sibling.pyc_instance_count.q_current!==sibling_q ||
        history()!==histories || `R.pyc_instance_selector.q_current!==1'bx ||
        pycircuit_sim.pyc_report_value_{report}!==report_value ||
        pycircuit_sim.pyc_report_update_{report}!==report_update)
      $display("CONDITIONAL-SYSTEM-PRECOMMIT-FAIL");
    else $display("CONDITIONAL-SYSTEM-PRECOMMIT-PASS");
  end
endmodule
"""
    )
    binary = tmp_path / "conditional_system_precommit"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "pycircuit_sim",
                "-s",
                "conditional_system_precommit",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dffe.v"),
                *map(str, rtl),
                str(monitor),
            ]
        )
    )
    failed = _run(["vvp", str(binary), "+cycles=3"], timeout=30)
    assert failed.returncode != 0, failed.stdout + failed.stderr
    assert "CONDITIONAL-SYSTEM-PRECOMMIT-PASS" in failed.stdout
    rows = _records(failed.stdout)
    assert [row["kind"] for row in rows] == ["log"] * 5 + ["report", "log"]
    assert all(
        int(row["evaluation_epoch"]) == 0 and int(row["commit_epoch"]) == 1
        for row in rows
    )
    assert [[int(value["value"]) for value in row["values"]] for row in rows] == [
        [1, 6],
        [4, 10],
        [10],
        [3, 13],
        [1, 6],
        [13],
        [6, 10, 13],
    ]


def _branch_method_program(tmp_path):
    source, provider, consumer = _branch_method_units(tmp_path)
    transaction_input = tmp_path / "branch method transaction input.ac"
    transaction_output = tmp_path / "branch method transaction output.ac"
    final = tmp_path / "branch method system.ac"
    linker = os.environ.get("PYCIRCUIT_LINKER")
    assert linker, "set PYCIRCUIT_LINKER to this candidate's native linker"
    arguments = [linker]
    for unit in (provider, consumer):
        receipt = json.loads((unit / "unit.json").read_text())
        arguments += [
            "--body",
            str(unit / receipt["files"]["body"]),
            "--header",
            str(unit / receipt["files"]["interface"]),
        ]
    arguments += [
        "--top",
        "checks.method_branch_bench.ExerciseMethodBranches",
        "--target",
        "final",
        "--output",
        str(final),
        "--transaction-input-out",
        str(transaction_input),
        "--transaction-output-out",
        str(transaction_output),
    ]
    _ok(_run(arguments))
    assert transaction_input.is_file() and transaction_output.is_file()
    before = transaction_input.read_text()
    after = transaction_output.read_text()
    final_text = final.read_text()
    assert '"ac.method.call"' in before
    assert "ac.branch" in before
    assert "ac.sampled_admission" in after
    assert '"ac.method.call"' not in after
    assert "ac.branch" not in after
    assert final_text.count('"ac.observe"(') == 6
    assert final_text.count("ac.observation_id") == 6
    assert final_text.count('kind = "assert"') >= 2
    assert "invocation" in final_text and "owner" in final_text
    return source, provider, consumer, final, transaction_input, transaction_output


def _branch_probe_program(tmp_path, mode):
    source, provider, _ = _branch_method_units(tmp_path)
    calls = {
        "one": "owner.one_arm(selector, data)\n        owner.snapshot()",
        "both": "owner.both_arms(selector, data)",
        "prior": "owner.prior_write(selector, data)",
        "identity": "owner.identity(selector)",
        "masked": "owner.guarded_one_arm(0, selector, data)",
        "ordered": "owner.guarded_identity(selector)\n        owner.guarded_identity(0)",
    }
    assert mode in calls
    probe = source / "branch_probe.py"
    probe.write_text(
        "import pycircuit as ac\n"
        "from checks.method_branch_provider import BranchProbeOwner\n\n"
        "@ac.system\n"
        "def BranchProbe():\n"
        "    selector: ac.u1 = 0\n"
        "    data: ac.u8 = 9\n"
        "    owner = BranchProbeOwner()\n\n"
        "    @ac.rule\n"
        "    def drive() -> None:\n"
        f"        {calls[mode]}\n\n"
        "    drive()\n"
    )
    consumer = tmp_path / f"branch {mode} consumer unit"
    _ok(_compile(source, probe.name, consumer, [provider]))
    final = tmp_path / f"branch {mode} probe.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.branch_probe.BranchProbe",
            "-o",
            str(final),
        )
    )
    return final


def _emit_branch_probe_rtl(final, tmp_path):
    generated = tmp_path / "verilog"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(generated)))
    receipt = json.loads((generated / "generated.json").read_text())
    rtl = [
        generated / row["path"]
        for row in receipt["files"]
        if row["path"].endswith((".v", ".sv"))
    ]
    assert generated / "simulation_top.sv" in rtl
    return generated, rtl


def _branch_probe_manual_rtl(final, tmp_path, mode):
    generated, rtl = _emit_branch_probe_rtl(final, tmp_path)
    body = {
        "one": r"""
    reset_model();
    `selector_q = 1'bx; clk=1; rst=0; prepare();
    if (error !== 1'b1 || sampling_error !== 1'b0) $fatal(1, "one-arm X enable did not fail");
    deny(8'd5, 1'b0);

    reset_model();
    `selector_q = 1'b1; clk=1; rst=0; prepare(); commit(); check_owner(8'd9, 1'b1);
    `selector_q = 1'bx; clk=1; prepare();
    if (error !== 1'b0 || sampling_error !== 1'b0) $fatal(1, "held clock changed intent semantics");
    commit(); check_owner(8'd9, 1'b1);
    clk=0; prepare();
    if (error !== 1'b0 || sampling_error !== 1'b0) $fatal(1, "falling edge changed intent semantics");
    commit(); check_owner(8'd9, 1'b0);
    clk=1; prepare();
    if (error !== 1'b1 || sampling_error !== 1'b0) $fatal(1, "later active X enable did not fail");
    deny(8'd9, 1'b0);

    reset_model();
    `selector_q = 1'bz; clk=1; rst=1; prepare();
    if (error !== 1'b0 || sampling_error !== 1'b0) $fatal(1, "reset did not mask Z enable");
    commit(); check_owner(8'd5, 1'b1);
""",
        "both": r"""
    reset_model();
    `selector_q = 1'bx; `data_q = 8'bz0x10101; clk=1; rst=0; prepare();
    if (error !== 1'b0 || sampling_error !== 1'b0) $fatal(1, "both-arm E1 rejected unknown data");
    commit(); check_owner(8'bz0x10101, 1'b1);
""",
        "prior": r"""
    reset_model();
    `selector_q = 1'bz; `data_q = 8'bz0x10101; clk=1; rst=0; prepare();
    if (error !== 1'b0 || sampling_error !== 1'b0) $fatal(1, "prior E1 rejected unknown data");
    commit();
    if (`owner_q !== 8'bx0xx01xx || `owner_history !== 1'b1)
      $fatal(1, "prior-write branch did not preserve shared known bits");
""",
        "identity": r"""
    reset_model();
    `selector_q = 1'bx; clk=1; rst=0; prepare();
    if (error !== 1'b1 || sampling_error !== 1'b0) $fatal(1, "identity write lost intent");
    deny(8'd5, 1'b0);
""",
        "masked": r"""
    reset_model();
    `selector_q = 1'bx; `data_q = 8'hxx; clk=1; rst=0; prepare();
    if (error !== 1'b0 || sampling_error !== 1'b0) $fatal(1, "false fire did not mask unknown E");
    commit(); check_owner(8'd5, 1'b1);
""",
        "ordered": r"""
    reset_model();
    `selector_q = 1'bx; clk=1; rst=0; prepare();
    if (error !== 1'b1 || sampling_error !== 1'b0) $fatal(1, "reached unknown guard was masked by later false guard");
    deny(8'd5, 1'b0);
""",
    }[mode]
    testbench = tmp_path / f"branch_{mode}_four_state.sv"
    testbench.write_text(
        f"""
module branch_{mode}_four_state;
  logic clk=0, rst=0;
  logic [2:0] phase=0;
  logic permit=0;
  wire complete, sampling_error, error;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .pyc_phase(phase), .pyc_root_commit_ok(permit),
    .pyc_check_complete(complete), .pyc_sampling_error(sampling_error),
    .pyc_local_error(error));

  `define ROOT dut.dut
  `define OWNER `ROOT.pyc_instance_owner.pyc_instance_value
  `define SELECTOR `ROOT.pyc_instance_selector
  `define DATA `ROOT.pyc_instance_data
  `define owner_q `OWNER.q_current
  `define owner_history `OWNER.managed.clock_current
  `define selector_q `SELECTOR.q_current
  `define data_q `DATA.q_current

  task automatic check_owner(input logic [7:0] q, input logic history);
    if (`owner_q !== q || `owner_history !== history)
      $fatal(1, "branch owner state/history changed");
  endtask
  task automatic prepare; #1; phase=1; #1; endtask
  task automatic commit;
    permit=1; phase=2; #1; phase=0; permit=0; #1;
  endtask
  task automatic deny(input logic [7:0] q, input logic history);
    permit=0; phase=2; #1; phase=3; #1; phase=0; #1; check_owner(q, history);
  endtask
  task automatic reset_model;
    clk=0; rst=0; permit=1; phase=4; #1; phase=2; #1;
    phase=0; permit=0; #1; check_owner(8'd5, 1'b0);
  endtask

  initial begin
    #1;
{body}
    $display("BRANCH_{mode.upper()}_FOUR_STATE_OK");
    $finish;
  end
endmodule
"""
    )
    binary = tmp_path / f"branch_{mode}_four_state"
    design_rtl = [path for path in rtl if path.name != "simulation_top.sv"]
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                f"branch_{mode}_four_state",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dffe.v"),
                *map(str, design_rtl),
                str(testbench),
            ]
        )
    )
    stdout = _ok(_run(["vvp", str(binary)], timeout=20))
    assert f"BRANCH_{mode.upper()}_FOUR_STATE_OK" in stdout
    assert _records(stdout) == []
    return generated


def _branch_probe_system_precommit(final, tmp_path):
    generated, rtl = _emit_branch_probe_rtl(final, tmp_path)
    simulation = generated / "simulation_top.sv"
    text = simulation.read_text()
    reports = re.findall(r"longint unsigned pyc_report_value_(\d+) =", text)
    assert len(reports) == 1
    report = reports[0]
    monitor = tmp_path / "branch_system_precommit.sv"
    monitor.write_text(
        f"""
module branch_system_precommit;
  logic captured = 1'b0;
  logic [7:0] owner_q, data_q;
  logic owner_history, selector_history, data_history;
  longint unsigned report_value, report_update;
  initial begin
    wait (pycircuit_sim.pyc_epoch == 1 && pycircuit_sim.pyc_phase == 0);
    pycircuit_sim.dut.dut.pyc_instance_selector.q_current = 1'bx;
    #0;
    owner_q = pycircuit_sim.dut.dut.pyc_instance_owner.pyc_instance_value.q_current;
    owner_history = pycircuit_sim.dut.dut.pyc_instance_owner.pyc_instance_value.managed.clock_current;
    selector_history = pycircuit_sim.dut.dut.pyc_instance_selector.managed.clock_current;
    data_q = pycircuit_sim.dut.dut.pyc_instance_data.q_current;
    data_history = pycircuit_sim.dut.dut.pyc_instance_data.managed.clock_current;
    report_value = pycircuit_sim.pyc_report_value_{report};
    report_update = pycircuit_sim.pyc_report_update_{report};
    captured = 1'b1;
  end
  final begin
    if (captured !== 1'b1 || pycircuit_sim.pyc_epoch !== 1 ||
        pycircuit_sim.dut.dut.pyc_instance_owner.pyc_instance_value.q_current !== owner_q ||
        pycircuit_sim.dut.dut.pyc_instance_owner.pyc_instance_value.managed.clock_current !== owner_history ||
        pycircuit_sim.dut.dut.pyc_instance_selector.q_current !== 1'bx ||
        pycircuit_sim.dut.dut.pyc_instance_selector.managed.clock_current !== selector_history ||
        pycircuit_sim.dut.dut.pyc_instance_data.q_current !== data_q ||
        pycircuit_sim.dut.dut.pyc_instance_data.managed.clock_current !== data_history ||
        pycircuit_sim.pyc_report_value_{report} !== report_value ||
        pycircuit_sim.pyc_report_update_{report} !== report_update)
      $display("BRANCH-SYSTEM-PRECOMMIT-FAIL");
    else
      $display("BRANCH-SYSTEM-PRECOMMIT-PASS");
  end
endmodule
"""
    )
    binary = tmp_path / "branch_system_precommit"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "pycircuit_sim",
                "-s",
                "branch_system_precommit",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dffe.v"),
                *map(str, rtl),
                str(monitor),
            ]
        )
    )
    failed = _run(["vvp", str(binary), "+cycles=3"], timeout=20)
    assert failed.returncode != 0, failed.stdout + failed.stderr
    assert "BRANCH-SYSTEM-PRECOMMIT-PASS" in failed.stdout
    rows = _records(failed.stdout)
    assert [row["kind"] for row in rows] == ["log", "report"]
    assert [int(row["evaluation_epoch"]) for row in rows] == [0, 0]
    assert [int(row["commit_epoch"]) for row in rows] == [1, 1]
    assert [[int(value["value"]) for value in row["values"]] for row in rows] == [
        [5],
        [5],
    ]


def _assert_branch_method_rows(rows):
    observations = [row for row in rows if row["kind"] != "result"]
    assert len(observations) == 24
    assert all(
        int(row["commit_epoch"]) == int(row["evaluation_epoch"]) + 1
        for row in observations
    )
    stable = None
    for epoch, old in enumerate((5, 5, 17, 17)):
        current = [row for row in observations if int(row["evaluation_epoch"]) == epoch]
        assert [
            (row["kind"], row["spec"].get("event", row["spec"].get("name")))
            for row in current
        ] == [
            ("log", "reader before"),
            ("log", "branch advance"),
            ("log", "branch advance"),
            ("report", "branch count"),
            ("log", "branch saved"),
            ("log", "reader after"),
        ]
        identities = [row["site"] for row in current]
        if stable is None:
            stable = identities
        else:
            assert identities == stable

        assert [int(value["value"]) for value in current[0]["values"]] == [old]
        assert [int(value["value"]) for value in current[1]["values"]] == [
            old,
            old + 11,
            old + 1,
            old + 5,
        ]
        assert [int(value["value"]) for value in current[2]["values"]] == [
            old + 5,
            old + 6,
            old + 8,
            old + 12,
        ]
        assert [int(value["value"]) for value in current[3]["values"]] == [old + 12]
        assert [int(value["value"]) for value in current[4]["values"]] == [
            old + 11,
            old + 6,
        ]
        assert [int(value["value"]) for value in current[5]["values"]] == [old]

        called = [json.loads(row["site"]) for row in current[1:4]]
        assert all(identity["invocation"] for identity in called)
        assert called[0]["site"] == called[1]["site"]
        assert called[0]["invocation"] != called[1]["invocation"]
        assert current[1]["instance"] == current[2]["instance"] == "root/owner"
        assert current[3]["instance"] == "root/owner"
        for row in (current[0], current[4], current[5]):
            assert json.loads(row["site"])["invocation"] == []
            assert row["instance"] == "root"

    terminal = [row for row in rows if row["kind"] == "result"]
    assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"
    assert int(terminal[0]["epoch_time"]) == 4
    gauges = [row for row in terminal[0]["statistics"] if row["name"] == "branch count"]
    assert len(gauges) == 1
    assert gauges[0]["object_path"] == "root/owner"
    assert int(gauges[0]["last_update"]["time"]) == 4
    assert int(gauges[0]["value"]) == 29


def _source_occurrence(definition, *path):
    ast_path = []
    for kind, value in path:
        ast_path.append({"kind": kind, "value" if kind == "index" else "name": value})
    return {
        "expansion": [],
        "site": {"ast_path": ast_path, "definition": definition},
    }


def _method_expected_identities():
    bench = "checks.method_effects_bench.ExerciseMethodEffects"
    provider = "checks.method_effects_provider.MethodCounter"

    def field(name):
        return ("field", name)

    def index(value):
        return ("index", value)

    owner = _source_occurrence(bench, field("body"), index(2), field("body"), index(0))
    registration = _source_occurrence(
        bench,
        field("body"),
        index(2),
        field("body"),
        index(4),
        field("value"),
    )

    def called(call_index, ordinal, site):
        return {
            "invocation": [
                {
                    "call": _source_occurrence(
                        bench,
                        field("body"),
                        index(2),
                        field("body"),
                        index(2),
                        field("body"),
                        index(call_index),
                        field("value"),
                    ),
                    "receiver": {"instance_path": [owner]},
                }
            ],
            "ordinal": ordinal,
            "site": site,
        }

    bump_site = _source_occurrence(
        provider,
        field("body"),
        index(1),
        field("body"),
        index(1),
        field("body"),
        index(2),
    )
    report_site = _source_occurrence(
        provider,
        field("body"),
        index(1),
        field("body"),
        index(3),
        field("body"),
        index(0),
    )
    return registration, [
        called(0, 0, bump_site),
        called(1, 1, bump_site),
        called(3, 2, report_site),
        {
            "invocation": [],
            "ordinal": 3,
            "site": _source_occurrence(
                bench,
                field("body"),
                index(2),
                field("body"),
                index(2),
                field("body"),
                index(4),
            ),
        },
        {
            "invocation": [],
            "ordinal": 4,
            "site": _source_occurrence(
                bench,
                field("body"),
                index(2),
                field("body"),
                index(2),
                field("body"),
                index(5),
            ),
        },
    ]


def _method_identity(row, expected, registration):
    assert json.loads(row["registration"]) == registration
    identity = json.loads(row["site"])
    assert identity == expected
    assert row["instance"] == ("root/owner" if expected["invocation"] else "root")
    return identity


def _assert_method_effect_rows(rows):
    observations = [row for row in rows if row["kind"] != "result"]
    assert len(observations) == 30
    assert all(
        int(row["commit_epoch"]) == int(row["evaluation_epoch"]) + 1
        for row in observations
    )
    assert not any(row["spec"].get("event") == "suppressed" for row in observations)

    registration, expected_identities = _method_expected_identities()
    stable_identities = None
    for epoch in range(6):
        old = 3 * (epoch // 2)
        current = [row for row in observations if int(row["evaluation_epoch"]) == epoch]
        assert len(current) == 5
        assert [
            (row["kind"], row["spec"].get("event", row["spec"].get("name")))
            for row in current
        ] == [
            ("log", "bump"),
            ("log", "bump"),
            ("report", "method count"),
            ("log", "saved candidates"),
            ("log", "literal only"),
        ]
        identities = [row["site"] for row in current]
        if stable_identities is None:
            stable_identities = identities
        else:
            assert identities == stable_identities
        bumps = [
            row
            for row in current
            if row["kind"] == "log" and row["spec"].get("event") == "bump"
        ]
        assert len(bumps) == 2
        assert [[int(value["value"]) for value in row["values"]] for row in bumps] == [
            [old, 1, old + 1],
            [old + 1, 2, old + 3],
        ]
        bump_ids = [
            _method_identity(row, expected, registration)
            for row, expected in zip(bumps, expected_identities[:2], strict=True)
        ]
        assert bump_ids[0]["site"] == bump_ids[1]["site"]
        assert bump_ids[0]["invocation"] != bump_ids[1]["invocation"]

        reports = [row for row in current if row["kind"] == "report"]
        assert len(reports) == 1
        assert reports[0]["spec"] == {"name": "method count"}
        assert [int(value["value"]) for value in reports[0]["values"]] == [old + 3]
        _method_identity(reports[0], expected_identities[2], registration)

        saved = [
            row for row in current if row["spec"].get("event") == "saved candidates"
        ]
        assert len(saved) == 1 and saved[0]["instance"] == "root"
        assert [int(value["value"]) for value in saved[0]["values"]] == [old, old + 1]
        _method_identity(saved[0], expected_identities[3], registration)

        literal = [row for row in current if row["spec"].get("event") == "literal only"]
        assert len(literal) == 1 and literal[0]["values"] == []
        assert literal[0]["spec"]["items"] == [{"kind": "literal", "text": "fixed"}]
        _method_identity(literal[0], expected_identities[4], registration)

    terminal = [row for row in rows if row["kind"] == "result"]
    assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"
    assert int(terminal[0]["epoch_time"]) == 6
    gauges = [row for row in terminal[0]["statistics"] if row["name"] == "method count"]
    assert len(gauges) == 1
    assert gauges[0]["object_path"] == "root/owner"
    assert int(gauges[0]["last_update"]["time"]) == 6
    assert int(gauges[0]["value"]) == 9


def _method_four_state_rtl(generated, tmp_path):
    testbench = tmp_path / "method_effects_four_state.sv"
    testbench.write_text(
        """
module method_effects_four_state;
  logic clk=0, rst=0;
  logic [2:0] phase=0;
  logic permit=0;
  wire complete, sampling_error, error;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .pyc_phase(phase), .pyc_root_commit_ok(permit),
    .pyc_check_complete(complete), .pyc_sampling_error(sampling_error),
    .pyc_local_error(error));

  task automatic check_owner(input logic [7:0] q, input logic history);
    if (dut.dut.pyc_instance_owner.pyc_instance_count.q_current !== q ||
        dut.dut.pyc_instance_owner.pyc_instance_count.managed.clock_current !== history)
      $fatal(1, "committed Method owner state/history changed");
  endtask

  task automatic idle;
    phase=3; #1; phase=0; permit=0; #1;
  endtask

  task automatic reset_model;
    clk=0; rst=0; permit=1; phase=4; #1;
    phase=2; #1; phase=0; permit=0; #1;
    check_owner(8'd0, 1'b0);
  endtask

  task automatic deny(input logic [7:0] q, input logic history);
    permit=0; phase=2; #1; check_owner(q, history); idle();
  endtask

  initial begin
    #1;
    // A false body assertion is checked on low, held and falling epochs. A
    // denied commit preserves both Q and the prior sampled-clock history.
    reset_model();
    dut.dut.pyc_instance_owner.pyc_instance_count.q_current = 8'd9;
    clk=0; rst=0; phase=1; #1;
    if (error !== 1'b1 || sampling_error !== 1'b0) $fatal(1, "low body assertion did not fail");
    deny(8'd9, 1'b0);

    reset_model();
    clk=1; rst=1; phase=1; #1;
    if (error !== 1'b0 || sampling_error !== 1'b0) $fatal(1, "reset did not mask body assertion");
    permit=1; phase=2; #1; phase=0; permit=0; rst=0; #1;
    check_owner(8'd0, 1'b1);
    dut.dut.pyc_instance_owner.pyc_instance_count.q_current = 8'd9;
    clk=1; phase=1; #1;
    if (error !== 1'b1 || sampling_error !== 1'b0) $fatal(1, "held body assertion did not fail");
    deny(8'd9, 1'b1);

    reset_model();
    clk=1; rst=1; phase=1; #1; permit=1; phase=2; #1;
    phase=0; permit=0; rst=0; #1; check_owner(8'd0, 1'b1);
    dut.dut.pyc_instance_owner.pyc_instance_count.q_current = 8'd9;
    clk=0; phase=1; #1;
    if (error !== 1'b1 || sampling_error !== 1'b0) $fatal(1, "falling body assertion did not fail");
    deny(8'd9, 1'b1);

    // X and Z in the statement-time assertion condition both fail closed at
    // an active edge, while denial retains the exact four-state Q and history.
    reset_model();
    dut.dut.pyc_instance_owner.pyc_instance_count.q_current = 8'hxx;
    clk=1; rst=0; phase=1; #1;
    if (error !== 1'b1 || sampling_error !== 1'b0) $fatal(1, "X body assertion did not fail");
    deny(8'hxx, 1'b0);

    reset_model();
    dut.dut.pyc_instance_owner.pyc_instance_count.q_current = 8'hzz;
    clk=1; rst=0; phase=1; #1;
    if (error !== 1'b1 || sampling_error !== 1'b0) $fatal(1, "Z body assertion did not fail");
    deny(8'hzz, 1'b0);

    // Reset masks even an unknown body condition and commits the initializer.
    reset_model();
    dut.dut.pyc_instance_owner.pyc_instance_count.q_current = 8'hxx;
    clk=1; rst=1; phase=1; #1;
    if (error !== 1'b0 || sampling_error !== 1'b0) $fatal(1, "reset did not mask unknown assertion");
    permit=1; phase=2; #1; check_owner(8'd0, 1'b1);
    $display("METHOD_EFFECTS_FOUR_STATE_OK");
    $finish;
  end
endmodule
"""
    )
    binary = tmp_path / "method_effects_four_state"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "method_effects_four_state",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dffe.v"),
                str(generated / "design_top.sv"),
                str(generated / "sources/checks/method_effects_provider.v"),
                str(generated / "sources/checks/method_effects_bench.v"),
                str(testbench),
            ]
        )
    )
    stdout = _ok(_run(["vvp", str(binary)], timeout=20))
    assert "METHOD_EFFECTS_FOUR_STATE_OK" in stdout
    assert _records(stdout) == []


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
    _assert_closed_observation_identities(rows)


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


def test_method_system_effects_execute_atomically_from_one_final_artifact(tmp_path):
    final, transaction_input, transaction_output = _method_program(tmp_path)
    final_digest = hashlib.sha256(final.read_bytes()).hexdigest()
    stage_digests = {
        "input": hashlib.sha256(transaction_input.read_bytes()).hexdigest(),
        "output": hashlib.sha256(transaction_output.read_bytes()).hexdigest(),
    }
    transcripts = []
    failures = []
    manifests = {}
    for target in ("cpp", "verilog"):
        target_root = tmp_path / ("method-" + target)
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        manifest = json.loads((target_root / target / "generated.json").read_text())
        manifests[target] = manifest
        workers = (1, 2) if target == "cpp" else (1,)
        for count in workers:
            arguments = (
                ["--cycles", "3", "--workers", str(count)]
                if target == "cpp"
                else ["+cycles=3"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=20)))
            _assert_method_effect_rows(rows)
            transcripts.append([row for row in rows if row["kind"] != "result"])

        failure_arguments = (
            ["--cycles", "4", "--workers", "2"] if target == "cpp" else ["+cycles=4"]
        )
        failed = _run([str(binary), *failure_arguments], timeout=20)
        assert failed.returncode != 0, failed.stdout + failed.stderr
        failed_rows = _records(failed.stdout)
        # The candidate value 12 fails before publication at evaluation epoch
        # six. Earlier events and the report gauge remain exactly committed.
        failures.append([row for row in failed_rows if row["kind"] != "result"])
        assert failures[-1] == transcripts[-1]
        failed_terminal = [row for row in failed_rows if row["kind"] == "result"]
        if failed_terminal:
            assert len(failed_terminal) == 1
            assert failed_terminal[0]["status"] == "FAILED"
            assert int(failed_terminal[0]["epoch_time"]) == 6
            gauges = [
                row
                for row in failed_terminal[0]["statistics"]
                if row["name"] == "method count"
            ]
            assert len(gauges) == 1
            assert int(gauges[0]["last_update"]["time"]) == 6
            assert int(gauges[0]["value"]) == 9
        else:
            assert "check failed at epoch 6" in failed.stdout + failed.stderr
        if target == "verilog":
            _method_four_state_rtl(target_root / target, target_root)

    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert failures[0] == failures[1] == transcripts[0]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == final_digest
    assert {
        "input": hashlib.sha256(transaction_input.read_bytes()).hexdigest(),
        "output": hashlib.sha256(transaction_output.read_bytes()).hexdigest(),
    } == stage_digests
    assert manifests["cpp"]["entry"] == manifests["verilog"]["entry"]


def test_method_branch_system_executes_shared_candidates_from_one_final_artifact(
    tmp_path,
):
    (
        _,
        _,
        _,
        final,
        transaction_input,
        transaction_output,
    ) = _branch_method_program(tmp_path)
    final_digest = hashlib.sha256(final.read_bytes()).hexdigest()
    stage_digests = {
        "input": hashlib.sha256(transaction_input.read_bytes()).hexdigest(),
        "output": hashlib.sha256(transaction_output.read_bytes()).hexdigest(),
    }
    transcripts = []
    failures = []
    manifests = {}
    for target in ("cpp", "verilog"):
        target_root = tmp_path / ("branch-method-" + target)
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        manifests[target] = json.loads(
            (target_root / target / "generated.json").read_text()
        )
        workers = (1, 2) if target == "cpp" else (1,)
        for count in workers:
            arguments = (
                ["--cycles", "2", "--workers", str(count)]
                if target == "cpp"
                else ["+cycles=2"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=20)))
            _assert_branch_method_rows(rows)
            transcripts.append([row for row in rows if row["kind"] != "result"])

        failure_arguments = (
            ["--cycles", "3", "--workers", "2"] if target == "cpp" else ["+cycles=3"]
        )
        failed = _run([str(binary), *failure_arguments], timeout=20)
        assert failed.returncode != 0, failed.stdout + failed.stderr
        failed_rows = _records(failed.stdout)
        failures.append([row for row in failed_rows if row["kind"] != "result"])
        assert failures[-1] == transcripts[-1]
        terminal = [row for row in failed_rows if row["kind"] == "result"]
        if terminal:
            assert len(terminal) == 1 and terminal[0]["status"] == "FAILED"
            assert int(terminal[0]["epoch_time"]) == 4
            gauges = [
                row
                for row in terminal[0]["statistics"]
                if row["name"] == "branch count"
            ]
            assert len(gauges) == 1
            assert int(gauges[0]["last_update"]["time"]) == 4
            assert int(gauges[0]["value"]) == 29
        else:
            assert "check failed at epoch 4" in failed.stdout + failed.stderr

    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert failures[0] == failures[1] == transcripts[0]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == final_digest
    assert {
        "input": hashlib.sha256(transaction_input.read_bytes()).hexdigest(),
        "output": hashlib.sha256(transaction_output.read_bytes()).hexdigest(),
    } == stage_digests
    assert manifests["cpp"]["entry"] == manifests["verilog"]["entry"]


def test_conditional_method_system_candidates_and_effects_from_one_final_artifact(
    tmp_path,
):
    final, before, after = _conditional_method_program(tmp_path)
    artifacts = (final, before, after)
    digests = [hashlib.sha256(path.read_bytes()).hexdigest() for path in artifacts]
    transcripts = []
    entries = []
    for target in ("cpp", "verilog"):
        target_root = tmp_path / ("conditional-" + target)
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        entries.append(
            json.loads((target_root / target / "generated.json").read_text())["entry"]
        )
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "2", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=2"]
            )
            rows = _records(_ok(_run([str(binary), *arguments], timeout=20)))
            _assert_conditional_method_rows(rows)
            transcripts.append([row for row in rows if row["kind"] != "result"])
        failed = _run(
            [
                str(binary),
                *(
                    ["--cycles", "3", "--workers", "2"]
                    if target == "cpp"
                    else ["+cycles=3"]
                ),
            ],
            timeout=20,
        )
        assert failed.returncode != 0, failed.stdout + failed.stderr
        failed_rows = _records(failed.stdout)
        assert [row for row in failed_rows if row["kind"] != "result"] == transcripts[
            -1
        ]
        terminal = [row for row in failed_rows if row["kind"] == "result"]
        if terminal:
            assert len(terminal) == 1 and terminal[0]["status"] == "FAILED"
            assert int(terminal[0]["epoch_time"]) == 4
            gauges = [
                row
                for row in terminal[0]["statistics"]
                if row["name"] == "conditional count"
            ]
            assert {row["object_path"]: int(row["value"]) for row in gauges} == {
                "root/left": 17,
                "root/right": 21,
            }
            assert all(int(row["last_update"]["time"]) == 4 for row in gauges)
        else:
            assert "check failed at epoch 4" in failed.stdout + failed.stderr
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert entries[0] == entries[1]
    assert [
        hashlib.sha256(path.read_bytes()).hexdigest() for path in artifacts
    ] == digests


def test_conditional_method_four_state_effects_and_system_rollback(tmp_path):
    final, before, after = _conditional_method_program(tmp_path, probe=True)
    artifacts = (final, before, after)
    digests = [hashlib.sha256(path.read_bytes()).hexdigest() for path in artifacts]
    cpp = tmp_path / "conditional-probe-cpp"
    cpp.mkdir()
    _conditional_probe_cpp(final, cpp)
    rtl = tmp_path / "conditional-probe-rtl"
    rtl.mkdir()
    generated, paths = _conditional_probe_four_state_rtl(final, rtl)
    _conditional_probe_system_precommit(generated, paths, rtl)
    assert [
        hashlib.sha256(path.read_bytes()).hexdigest() for path in artifacts
    ] == digests


def test_conditional_report_conflict_rejects_and_preserves_final_artifact(tmp_path):
    source = tmp_path / "conditional reports"
    source.mkdir()
    design = source / "reports.py"
    valid = (
        "import pycircuit as ac\n"
        "@ac.system\n"
        "def ConditionalReports():\n"
        "    selector: ac.u1 = 0\n"
        "    @ac.rule\n"
        "    def observe() -> None:\n"
        "        if selector:\n"
        "            ac.report('left', selector)\n"
        "        else:\n"
        "            ac.report('right', selector)\n"
        "    observe()\n"
    )
    design.write_text(valid)
    valid_unit = tmp_path / "conditional reports unit"
    _ok(_compile(source, design.name, valid_unit))
    output = tmp_path / "published"
    output.mkdir()
    final = output / "conditional reports.ac"
    _ok(
        _cli(
            "link",
            str(valid_unit),
            "--top",
            "checks.reports.ConditionalReports",
            "-o",
            str(final),
        )
    )
    before = {
        str(path.relative_to(output)): path.read_bytes()
        for path in output.rglob("*")
        if path.is_file()
    }
    design.write_text(valid.replace("'right'", "'left'"))
    conflicting_unit = tmp_path / "conflicting reports unit"
    _ok(_compile(source, design.name, conflicting_unit))
    rejected = _cli(
        "link",
        str(conflicting_unit),
        "--top",
        "checks.reports.ConditionalReports",
        "-o",
        str(final),
        "--replace",
    )
    assert rejected.returncode != 0
    assert "report name must be unique within logical owner: left" in rejected.stderr
    assert {
        str(path.relative_to(output)): path.read_bytes()
        for path in output.rglob("*")
        if path.is_file()
    } == before


def test_method_branch_four_state_intent_and_system_precommit(tmp_path):
    for mode in ("one", "both", "prior", "identity", "masked", "ordered"):
        case = tmp_path / mode
        case.mkdir()
        final = _branch_probe_program(case, mode)
        manual = case / "manual"
        manual.mkdir()
        _branch_probe_manual_rtl(final, manual, mode)
        if mode == "one":
            system = case / "system"
            system.mkdir()
            _branch_probe_system_precommit(final, system)


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
                os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL", "2"),
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
