"""Independent typed payload and instance oracles for generated observations."""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def value(item):
    return (
        {"kind": "bool", "value": item}
        if isinstance(item, bool)
        else {"kind": "integer", "value": str(item)}
    )


def items(*sequence):
    result, ordinal = [], 0
    for item in sequence:
        if isinstance(item, str):
            result.append({"kind": "literal", "text": item})
        else:
            result.append({"kind": "value", "ordinal": ordinal})
            ordinal += 1
    return result


def log_spec(event, *sequence):
    return {"event": event, "items": items(*sequence), "level": "info"}


def observation_identity(record, definition_prefix):
    registration = json.loads(record["registration"])
    assert registration["site"]["definition"].startswith(definition_prefix), registration
    assert registration["site"]["ast_path"], registration
    assert "expansion" in registration, registration
    identity = json.loads(record["site"])
    assert set(identity) == {"site", "invocation", "ordinal"}, identity
    assert identity["invocation"] == [], identity
    assert isinstance(identity["ordinal"], int) and identity["ordinal"] >= 0, identity
    assert identity["site"]["site"]["definition"].startswith(definition_prefix), identity
    assert identity["site"]["site"]["ast_path"], identity
    assert "expansion" in identity["site"], identity
    return registration, identity


# Literal arrays are independent of compiler results and hardware source state.
ROWS = (
    (0, (False, 17, 51, 93, 119), 119, (0, 17, True), (0, 51, False)),
    (1, (True, 18, 52, 93, 119), 119, (17, 18, False), (51, 52, False)),
    (2, (False, 19, 53, 93, 119), 119, (18, 19, False), (52, 53, False)),
)
ARITHMETIC_ROWS = (
    (17, 17, 99, 0, False, 93, 119),
    (18, 18, 98, 2, False, 93, 119),
    (116, 116, 0, 198, True, 93, 119),
    (117, 117, 255, 200, False, 93, 119),
    (0, 0, 116, 222, False, 93, 119),
    (1, 1, 115, 224, False, 93, 119),
    (16, 16, 100, 254, False, 93, 119),
)
ONLY_OBSERVATION_ROWS = (
    (17, 7, False, 34),
    (20, 8, True, 37),
    (23, 9, False, 40),
)


def expected_epoch(epoch):
    phase, mixed, after, left, right = ROWS[epoch // 2]
    return [
        ("root", "log", log_spec("before", 0), [value(phase)]),
        (
            "root",
            "log",
            log_spec("mixed", "begin", False, 0, "middle", 0, 0, 0, "end"),
            [value(item) for item in mixed],
        ),
        ("root", "log", log_spec("literal", "words"), []),
        ("root", "log", log_spec("after", 0), [value(after)]),
        ("root", "report", {"name": "ticks"}, [value(phase)]),
        (
            "root/left",
            "log",
            log_spec("leaf", "old", 0, "next", 0, False),
            [value(item) for item in left],
        ),
        (
            "root/right",
            "log",
            log_spec("leaf", "old", 0, "next", 0, False),
            [value(item) for item in right],
        ),
    ]


def observations(stdout, epochs):
    records = [json.loads(line) for line in stdout.splitlines() if line.startswith("{")]
    events = [record for record in records if record["kind"] != "result"]
    assert len(events) == epochs * 7, events
    for epoch in range(epochs):
        actual = events[epoch * 7 : (epoch + 1) * 7]
        assert [
            (record["instance"], record["kind"], record["spec"], record["values"])
            for record in actual
        ] == expected_epoch(epoch), (epoch, actual)
        for record in actual:
            assert int(record["evaluation_epoch"]) == epoch
            assert int(record["commit_epoch"]) == epoch + 1
            observation_identity(
                record, "observation_groups.source_observation_groups."
            )
    return records, events


def arithmetic_observations(stdout, instance):
    records = [json.loads(line) for line in stdout.splitlines() if line.startswith("{")]
    events = [record for record in records if record["kind"] != "result"]
    assert len(events) == 14 * 3, events
    spec = log_spec(
        "arith",
        "sum_left",
        0,
        "sum_right",
        0,
        "subtract",
        0,
        "product",
        0,
        "equal",
        False,
        "constants",
        0,
        0,
    )
    for epoch in range(14):
        row = ARITHMETIC_ROWS[epoch // 2]
        expected = [
            ("log", spec, [value(item) for item in row]),
            ("log", log_spec("arithmetic_scalar", 0), [value(row[0])]),
            ("report", {"name": "arithmetic_progress"}, [value(row[0])]),
        ]
        actual = events[epoch * 3 : (epoch + 1) * 3]
        assert [
            (item["kind"], item["spec"], item["values"]) for item in actual
        ] == expected, (epoch, actual)
        for item in actual:
            assert item["instance"] == instance
            assert int(item["evaluation_epoch"]) == epoch
            assert int(item["commit_epoch"]) == epoch + 1
            observation_identity(
                item, "observation_groups.source_observation_groups."
            )
    return records, events


def only_observations(stdout, epochs, symbol):
    records = [json.loads(line) for line in stdout.splitlines() if line.startswith("{")]
    events = [record for record in records if record["kind"] != "result"]
    assert len(events) == epochs * 3, events
    identities = None
    for epoch in range(epochs):
        row = ONLY_OBSERVATION_ROWS[epoch // 2]
        expected = [
            (
                "root",
                "log",
                log_spec("only_log", "old", 0, 0, False, 0),
                [value(item) for item in row],
            ),
            ("root", "log", log_spec("only_literal", "unchanged"), []),
            ("root", "report", {"name": "observation_sample"}, [value(row[0])]),
        ]
        actual = events[epoch * 3 : (epoch + 1) * 3]
        assert [
            (item["instance"], item["kind"], item["spec"], item["values"])
            for item in actual
        ] == expected, (epoch, actual)
        current = [(item["registration"], item["site"]) for item in actual]
        assert len({site for _, site in current}) == 3, current
        assert current[0][0] == current[1][0] != current[2][0], current
        if identities is None:
            identities = current
        assert current == identities, (epoch, current, identities)
        for item in actual:
            assert int(item["evaluation_epoch"]) == epoch
            assert int(item["commit_epoch"]) == epoch + 1
            observation_identity(
                item, "observation_groups.source_observation_groups." + symbol
            )
    return records, events


def four_state_precommit_monitor(simulation_top):
    text = simulation_top.read_text()
    reports = re.findall(r"longint unsigned pyc_report_value_(\d+) =", text)
    assert len(reports) == 1, reports
    report = reports[0]
    return f"""
module observation_precommit_probe;
  reg [7:0] left_q, right_q;
  logic left_clock, right_clock;
  logic captured = 1'b0;
  longint unsigned report_value, report_update;
  initial begin
    wait (pycircuit_sim.pyc_epoch == 1 && pycircuit_sim.pyc_phase == 0);
    left_q = pycircuit_sim.dut.dut.pyc_instance_left.pyc_instance_value.q_current;
    right_q = pycircuit_sim.dut.dut.pyc_instance_right.pyc_instance_value.q_current;
    left_clock = pycircuit_sim.dut.dut.pyc_instance_left.pyc_instance_value.managed.clock_current;
    right_clock = pycircuit_sim.dut.dut.pyc_instance_right.pyc_instance_value.managed.clock_current;
    report_value = pycircuit_sim.pyc_report_value_{report};
    report_update = pycircuit_sim.pyc_report_update_{report};
    captured = 1'b1;
    force pycircuit_sim.dut.dut.pyc_instance_input_left.q_current = 8'b000x0001;
  end
  final begin
    if (captured !== 1'b1 || pycircuit_sim.pyc_epoch !== 1 ||
        pycircuit_sim.dut.dut.pyc_instance_left.pyc_instance_value.q_current !== left_q ||
        pycircuit_sim.dut.dut.pyc_instance_right.pyc_instance_value.q_current !== right_q ||
        pycircuit_sim.dut.dut.pyc_instance_left.pyc_instance_value.managed.clock_current !== left_clock ||
        pycircuit_sim.dut.dut.pyc_instance_right.pyc_instance_value.managed.clock_current !== right_clock ||
        pycircuit_sim.pyc_report_value_{report} !== report_value ||
        pycircuit_sim.pyc_report_update_{report} !== report_update)
      $display("FOURSTATE-PRECOMMIT-FAIL");
    else
      $display("FOURSTATE-PRECOMMIT-PASS");
  end
endmodule
"""


def native_precommit_driver():
    return r'''#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

void requireAt(bool condition, const char *expression, int line) {
  if (!condition) {
    std::cerr << "native precommit assertion line " << line << ": "
              << expression << '\n';
    std::abort();
  }
}
#define require(condition) requireAt((condition), #condition, __LINE__)
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
auto mixed() {
  gfsim::Bits<8> value{0x55}, knownMask{0xef}, zMask{0};
  return gfsim::wire<gfsim::Bits<8>>::fromPacked(
      gfsim::FourState<8>::fromMasks(value, knownMask, zMask));
}
template <class Storage, class Pin>
void inject(Storage &storage, Pin &pin) {
  using Current = std::remove_cvref_t<decltype(storage.current(0))>;
  auto &current = const_cast<Current &>(storage.current(0));
  current.q = mixed();
  pin.element(0) = current.q;
}
template <class Wire>
bool equivalent(const Wire &left, const Wire &right) {
  return left.packed().parityEquivalent(right.packed());
}
int main(int argc, char **argv) {
  require(argc == 2);
  const unsigned workers = std::strtoul(argv[1], nullptr, 10);
  pyc_dut dut(workers);
  std::vector<gfsim::ReportGaugeDescriptor> reports;
  for (const auto &entry : pyc_observation_metadata())
    if (entry.kind == "report")
      reports.push_back({entry.stableOrdinal, entry.instance, entry.reportName});
  require(reports.size() == 1);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), reports);
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":8,"schema":"pycircuit-model-config","version":"1"})";
  require(executor.ConfigureJson(
      reinterpret_cast<const std::uint8_t *>(config.data()), config.size()) ==
      PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  pyc_dut::Inputs inputs;
  inputs.pyc_7079635f636c6b = known<1>(0);
  inputs.pyc_7079635f727374 = known<1>(0);
  dut.drive(inputs);
  PycircuitModelStepResultV1 result{sizeof(result)};
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.cycles() == 1);
  auto family = dut.root_->pyc_implementation;
  auto left = family->pyc_instance_left->pyc_instance_value_state->current(0);
  auto right = family->pyc_instance_right->pyc_instance_value_state->current(0);
  auto previousEvents = std::vector<gfsim::CommittedEventSlot>(
      dut.observations().Events().begin(), dut.observations().Events().end());
  auto previousGauges = std::vector<gfsim::GaugeSnapshot>(
      dut.observations().Gauges().begin(), dut.observations().Gauges().end());
  require(!previousEvents.empty() && previousGauges.size() == 1);
  require(previousGauges[0].lastUpdate == 1);
  inject(*family->pyc_instance_input_left_state,
         family->pyc_instance_input_left_q);
  auto injected = family->pyc_instance_input_left_state->current(0);
  inputs.pyc_7079635f636c6b = known<1>(1);
  dut.drive(inputs);
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(executor.cycles() == 1 && result.epoch_time == 1);
  require(dut.system().failureInfo().phase == gfsim::SimFailurePhase::Check);
  require(dut.system().failureInfo().code == "runtime_failure");
  require(dut.system().failureInfo().message == "model check failed");
  const auto leftAfter = family->pyc_instance_left->pyc_instance_value_state->current(0);
  const auto rightAfter = family->pyc_instance_right->pyc_instance_value_state->current(0);
  const auto injectedAfter = family->pyc_instance_input_left_state->current(0);
  require(equivalent(leftAfter.q, left.q) && leftAfter.clock == left.clock);
  require(equivalent(rightAfter.q, right.q) && rightAfter.clock == right.clock);
  require(equivalent(injectedAfter.q, injected.q) &&
          injectedAfter.clock == injected.clock);
  require(std::vector<gfsim::CommittedEventSlot>(
              dut.observations().Events().begin(),
              dut.observations().Events().end()) == previousEvents);
  require(std::vector<gfsim::GaugeSnapshot>(
              dut.observations().Gauges().begin(),
              dut.observations().Gauges().end()) == previousGauges);
}
'''


def expose_access_only_headers(root):
    changed = {}
    for header in root.rglob("*.hpp"):
        original = header.read_text()
        lines = original.splitlines(keepends=True)
        exposed = "".join(
            line.replace("private:", "public:")
            if line.strip() == "private:"
            else line
            for line in lines
        )
        if exposed != original:
            for before, after in zip(lines, exposed.splitlines(keepends=True), strict=True):
                assert before == after or (
                    before.strip() == "private:" and after.strip() == "public:"
                )
            header.write_text(exposed)
            changed[header.relative_to(root).as_posix()] = {
                "original": hashlib.sha256(original.encode()).hexdigest(),
                "exposed": hashlib.sha256(exposed.encode()).hexdigest(),
            }
    return changed


def main():
    parser = argparse.ArgumentParser()
    for name in (
        "repo",
        "source-compiler",
        "linker",
        "emitter",
        "cxx",
        "verilator",
        "iverilog",
        "vvp",
        "scratch",
    ):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    repo = Path(args.repo).resolve()
    scratch = Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="observation-groups-", dir=scratch))
    source = work / "source"
    source.mkdir()
    fixture = Path(__file__).with_name("source_observation_groups.py")
    shutil.copy2(fixture, source)
    prefix = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        path
        for path in (
            prefix / "runtime/libpyc6_runtime.a",
            prefix / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    )
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python"),
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
    )
    inputs = [
        Path(__file__),
        fixture,
        Path(args.source_compiler),
        Path(args.linker),
        Path(args.emitter),
        runtime,
        repo / "cmake/verify_example.py",
    ]

    def digest(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    input_hashes = {str(path): digest(path) for path in inputs}
    (work / "input-hashes.json").write_text(json.dumps(input_hashes, indent=2) + "\n")
    commands = []

    def run(label, command, failure=False):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, capture_output=True, text=True, timeout=120
        )
        (work / (label + ".stdout")).write_text(result.stdout)
        (work / (label + ".stderr")).write_text(result.stderr)
        commands.append(
            {"command": command, "exit_status": result.returncode, "label": label}
        )
        (work / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert (result.returncode != 0) if failure else (result.returncode == 0), (
            command,
            result.stdout,
            result.stderr,
        )
        return result

    def cli(label, *arguments):
        return run(label, [sys.executable, "-m", "pycircuit.cli", *arguments])

    unit = work / "unit"
    cli(
        "compile",
        "compile",
        "-c",
        source / fixture.name,
        "--source-root",
        source,
        "--package-prefix",
        "observation_groups",
        "-o",
        unit,
    )
    cases = []
    successful_prefix = None
    only_successful_prefix = None
    for symbol, failing, check_count, cycles, arithmetic in (
        ("ObservationGroups", False, 2, 3, False),
        ("ObservationGroupsFailure", True, 3, 3, False),
        ("ArithmeticObservationGroups", False, 1, 7, True),
        ("CapturedArithmeticObservationGroups", False, 1, 7, True),
        ("OnlyObservationGroups", False, 0, 3, False),
        ("OnlyObservationGroupsFailure", True, 1, 3, False),
    ):
        observation_only = symbol.startswith("OnlyObservationGroups")
        output = work / symbol
        output.mkdir()
        entry = "observation_groups.source_observation_groups." + symbol
        final = output / "system.ac"
        cli(symbol + "-link", "link", unit, "--top", entry, "-o", final)
        transcripts = []
        for target in ("cpp", "verilog"):
            generated = output / target
            cli(
                symbol + "-emit-" + target,
                "emit",
                final,
                "--target",
                target,
                "-o",
                generated,
            )
            assert json.loads(
                (generated / "simulation_verification.json").read_text()
            ) == {
                "entry": {"definition": f'@"{entry}"', "arguments": []},
                "entry_source": {
                    "package": "observation_groups",
                    "path": fixture.name,
                },
                "source_checks": check_count,
            }
            binary = output / ("simulation-" + target)
            if target == "cpp":
                manifest = json.loads((generated / "generated.json").read_text())
                run(
                    symbol + "-build-cpp",
                    [
                        args.cxx,
                        "-std=c++20",
                        "-pthread",
                        "-I" + str(repo / "include"),
                        "-I" + str(generated),
                        *[
                            generated / item["path"]
                            for item in manifest["files"]
                            if item["path"].endswith(".cpp")
                        ],
                        runtime,
                        "-o",
                        binary,
                    ],
                )
            else:
                run(
                    symbol + "-build-rtl",
                    [
                        sys.executable,
                        repo / "cmake/verify_example.py",
                        "--compile-system",
                        generated,
                        "--output",
                        binary,
                        "--include",
                        repo / "include",
                        "--verilator",
                        args.verilator,
                    ],
                )
            for workers in (1, 2) if target == "cpp" else (1,):
                result = run(
                    f"{symbol}-{target}-{workers}",
                    (
                        [binary, "--cycles", str(cycles), "--workers", str(workers)]
                        if target == "cpp"
                        else [binary, f"+cycles={cycles}"]
                    ),
                    failure=failing,
                )
                records, events = (
                    only_observations(result.stdout, 4 if failing else cycles * 2, symbol)
                    if observation_only
                    else arithmetic_observations(
                        result.stdout,
                        "root" if symbol.startswith("Captured") else "root/dut",
                    )
                    if arithmetic
                    else observations(result.stdout, 4 if failing else cycles * 2)
                )
                terminal = [record for record in records if record["kind"] == "result"]
                if target == "cpp" or not failing:
                    assert len(terminal) == 1
                    assert terminal[0]["status"] == (
                        "FAILED" if failing else "TERMINATED"
                    )
                    assert int(terminal[0]["epoch_time"]) == (
                        4 if failing else cycles * 2
                    )
                    if failing:
                        assert terminal[0]["error"]["phase"] == "check"
                        assert terminal[0]["error"]["code"] == "source_check_failed"
                        assert terminal[0]["error"]["message"] == "grouped failure"
                        if observation_only:
                            gauge = [
                                item
                                for item in terminal[0]["statistics"]
                                if item["name"] == "observation_sample"
                            ]
                            assert len(gauge) == 1 and int(gauge[0]["value"]) == 20
                    else:
                        gauge_name = (
                            "observation_sample"
                            if observation_only
                            else "arithmetic_progress" if arithmetic else "ticks"
                        )
                        gauge = [
                            item
                            for item in terminal[0]["statistics"]
                            if item["name"] == gauge_name
                        ]
                        assert len(gauge) == 1 and int(gauge[0]["value"]) == (
                            23 if observation_only else 16 if arithmetic else 2
                        )
                else:
                    assert "check failed at epoch 4" in result.stdout + result.stderr
                transcripts.append(events)
        if symbol == "ObservationGroupsFailure":
            native_probe = output / "native-precommit-probe"
            shutil.copytree(output / "cpp", native_probe)
            native_visibility = expose_access_only_headers(native_probe)
            assert native_visibility
            native_driver = output / "native-precommit-driver.cpp"
            native_driver.write_text(native_precommit_driver())
            native_manifest = json.loads(
                (native_probe / "generated.json").read_text()
            )
            native_sources = [
                native_probe / item["path"]
                for item in native_manifest["files"]
                if item["path"].endswith(".cpp")
                and not item["path"].endswith("simulation_main.cpp")
            ]
            native_binary = output / "native-precommit"
            run(
                symbol + "-build-native-precommit",
                [
                    args.cxx,
                    "-std=c++20",
                    "-pthread",
                    "-I" + str(repo / "include"),
                    "-I" + str(native_probe),
                    native_driver,
                    *native_sources,
                    runtime,
                    "-o",
                    native_binary,
                ],
            )
            for workers in (1, 2):
                run(
                    f"{symbol}-native-precommit-{workers}",
                    [native_binary, workers],
                )
            generated = output / "verilog"
            monitor = output / "four-state-precommit-probe.sv"
            monitor.write_text(
                four_state_precommit_monitor(generated / "simulation_top.sv")
            )
            manifest = json.loads((generated / "generated.json").read_text())
            rtl_sources = [
                generated / item["path"]
                for item in manifest["files"]
                if item["role"] in ("rtl", "runtime-glue")
                and item["path"].endswith((".v", ".sv"))
            ]
            four_state_binary = output / "simulation-four-state"
            run(
                symbol + "-build-four-state",
                [
                    args.iverilog,
                    "-g2012",
                    "-s",
                    "pycircuit_sim",
                    "-s",
                    "observation_precommit_probe",
                    "-o",
                    four_state_binary,
                    repo / "include/verilog/dffe.v",
                    *rtl_sources,
                    monitor,
                ],
            )
            for cycles_text in (
                "1",
                "0001",
                "4294967297",
                "4611686018427387903",
            ):
                four_state = run(
                    symbol + "-four-state-precommit-" + cycles_text,
                    [args.vvp, four_state_binary, "+cycles=" + cycles_text],
                    failure=True,
                )
                output_text = four_state.stdout + four_state.stderr
                assert "inconsistent observation group at epoch 1" in output_text, output_text
                assert "FOURSTATE-PRECOMMIT-PASS" in output_text, output_text
                assert "FOURSTATE-PRECOMMIT-FAIL" not in output_text, output_text
                records = [
                    json.loads(line)
                    for line in four_state.stdout.splitlines()
                    if line.startswith("{")
                ]
                events = [
                    record for record in records if record["kind"] != "result"
                ]
                assert len(events) == 7, events
                assert [
                    (
                        record["instance"],
                        record["kind"],
                        record["spec"],
                        record["values"],
                    )
                    for record in events
                ] == expected_epoch(0), events
                assert all(
                    int(record["evaluation_epoch"]) == 0
                    and int(record["commit_epoch"]) == 1
                    for record in events
                ), events
            for index, cycles_text in enumerate(
                (
                    "",
                    "0",
                    "-1",
                    "+1",
                    " 1",
                    "1x",
                    "4611686018427387904",
                    "18446744073709551616",
                )
            ):
                rejected = run(
                    f"{symbol}-four-state-invalid-cycles-{index}",
                    [args.vvp, four_state_binary, "+cycles=" + cycles_text],
                    failure=True,
                )
                rejected_text = rejected.stdout + rejected.stderr
                assert "+cycles requires a positive decimal value" in rejected_text
                assert not any(
                    line.startswith("{") for line in rejected.stdout.splitlines()
                ), rejected.stdout
        assert transcripts[0] == transcripts[1] == transcripts[2]
        semantic_prefix = [
            {
                key: record[key]
                for key in (
                    "kind",
                    "instance",
                    "evaluation_epoch",
                    "commit_epoch",
                    "spec",
                    "values",
                )
            }
            for record in transcripts[0][:28]
        ]
        if failing:
            if observation_only:
                assert semantic_prefix == only_successful_prefix
            else:
                assert semantic_prefix == successful_prefix
        elif observation_only:
            only_successful_prefix = semantic_prefix[:12]
        elif not arithmetic:
            successful_prefix = semantic_prefix
        cases.append(
            {
                "entry": entry,
                "source_checks": check_count,
                "failed_work": failing,
                "epochs": 4 if failing else cycles * 2,
                "native_workers": [1, 2],
                "rtl": "pass",
                "complete_observations_equal": True,
                "observation_only_rules": observation_only,
                "native_precommit_rollback": symbol == "ObservationGroupsFailure",
                "actual_four_state_precommit": symbol
                == "ObservationGroupsFailure",
            }
        )
    protected_paths = [
        path
        for folder in (
            unit,
            *[work / case["entry"].rsplit(".", 1)[-1] for case in cases],
        )
        for path in folder.rglob("*")
        if path.is_file()
    ]
    published = {str(path): digest(path) for path in protected_paths}
    source_path = source / fixture.name
    original_source = source_path.read_text()
    rejected = []
    for label, expression, diagnostic in (
        (
            "boolean-plus-byte",
            "True + x",
            "Boolean literal cannot be implicitly converted",
        ),
        (
            "unequal-fixed-widths",
            "wide + x",
            "unsigned arithmetic operands require equal widths",
        ),
        ("oversize-fixed-peer", "256 + x", "width"),
    ):
        # Mutate the captured-register expression, retaining the separately
        # checked module-input arithmetic expression and all published products.
        prefix, suffix = original_source.rsplit("17 + x", 1)
        source_path.write_text(prefix + expression + suffix)
        try:
            result = run(
                "reject-" + label,
                [
                    sys.executable,
                    "-m",
                    "pycircuit.cli",
                    "compile",
                    "-c",
                    source_path,
                    "--source-root",
                    source,
                    "--package-prefix",
                    "observation_groups",
                    "-o",
                    unit,
                    "--replace",
                ],
                failure=True,
            )
            assert diagnostic in result.stderr, (label, result.stderr)
            assert published == {str(path): digest(path) for path in protected_paths}
            rejected.append(
                {
                    "case": label,
                    "diagnostic": result.stderr,
                    "publication_preserved": True,
                }
            )
        finally:
            source_path.write_text(original_source)
    log_statement = (
        '        log("info", "only_log", "old", sample, phase + 7, '
        'sample == 20, 17 + sample)'
    )
    report_statement = '        report("observation_sample", sample)'
    for label, original, replacement, diagnostic in (
        (
            "nested-malformed-report",
            report_statement,
            '        report("", sample)',
            "report name must be a nonempty static string",
        ),
        (
            "nested-captured-report",
            report_statement,
            '        saved = report("observation_sample", sample)\n'
            + report_statement,
            "unsupported hardware expression 'Call'",
        ),
        (
            "nested-empty-rule",
            log_statement + '\n        log("info", "only_literal", "unchanged")',
            "        pass",
            "registered structural rule has no hardware behavior",
        ),
        (
            "nested-conditional-log",
            log_statement,
            "        if phase == 0:\n    " + log_statement,
            "unsupported expression statement in pure rule",
        ),
        (
            "nested-unknown-call",
            log_statement,
            "        missing_observation(sample)",
            "unsupported behavioral rule statement 'Expr'",
        ),
    ):
        assert original_source.count(original) == 2, (label, original)
        source_path.write_text(original_source.replace(original, replacement, 1))
        try:
            result = run(
                "reject-" + label,
                [
                    sys.executable,
                    "-m",
                    "pycircuit.cli",
                    "compile",
                    "-c",
                    source_path,
                    "--source-root",
                    source,
                    "--package-prefix",
                    "observation_groups",
                    "-o",
                    unit,
                    "--replace",
                ],
                failure=True,
            )
            assert diagnostic in result.stderr, (label, result.stderr)
            assert "Traceback" not in result.stderr, (label, result.stderr)
            if label != "nested-empty-rule":
                assert "has no hardware behavior" not in result.stderr, result.stderr
            assert published == {str(path): digest(path) for path in protected_paths}
            rejected.append(
                {
                    "case": label,
                    "diagnostic": result.stderr,
                    "publication_preserved": True,
                }
            )
        finally:
            source_path.write_text(original_source)
    assert input_hashes == {str(path): digest(path) for path in inputs}
    (work / "evidence.json").write_text(
        json.dumps(
            {
                "role": "independent generated observation tests",
                "model": "gpt-6.1-sol",
                "effort": "high",
                "cases": cases,
                "negative_cases": rejected,
                "input_sha256": input_hashes,
                "artifact_sha256": {
                    str(path): digest(path)
                    for path in work.rglob("*")
                    if path.is_file()
                },
                "scope": "Supported unconditional observations only. Partial/inactive publication groups are checked through actual Runtime ObservationSlots, without inventing source conditional-observation admission.",
            },
            indent=2,
        )
        + "\n"
    )
    print(
        "PASS: typed distinct grouped values, separate child occurrences, full failed-Work prefix, arithmetic boundaries, observation-only nested rules and protected refusals; native workers1/2 and RTL"
    )  # noqa: T201 - standalone gate receipt


if __name__ == "__main__":
    main()
