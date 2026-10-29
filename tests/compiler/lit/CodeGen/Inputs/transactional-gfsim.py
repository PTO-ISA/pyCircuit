"""Run saved Linked designs against example-owned independent histories."""

import ast
import json
import os
import shutil
import sys
from collections import Counter
from pathlib import Path

from transactional_test_support import SourceTest

arguments = sys.argv[1:]
target = arguments[-1]
sys.path.insert(0, str(Path(arguments[0]) / "cmake"))
from verify_example import verify_expected_observations  # noqa: E402


def logs(result):
    rows = [
        row
        for line in result.stdout.splitlines()
        if line.startswith("{")
        for row in [json.loads(line)]
    ]
    if target == "t-gfsim":
        results = [row for row in rows if row.get("kind") == "result"]
        assert len(results) == 1, rows
        assert results[0]["status"] == ("FAILED" if result.returncode else "TERMINATED")
    return [row for row in rows if row.get("kind") == "log"]


for example, units, top in (
    ("hello_counter", ("hello_counter",), "hello_counter.HelloCounter"),
    (
        "module_control_state_machine",
        ("module_control_state_machine",),
        "module_control_state_machine.ModuleControlStateMachine",
    ),
    (
        "local_method_accumulator",
        ("local_method_accumulator",),
        "local_method_accumulator.LocalMethodAccumulator",
    ),
    ("sibling_method_calls", ("Sender", "Receiver", "Top"), "Top.Top"),
):
    case_arguments = list(arguments)
    case_arguments[6] = str(Path(arguments[6]) / example)
    package = f"example_{example}"
    fixture = SourceTest(case_arguments, example, package)
    history = json.loads((fixture.sources / "expected.json").read_text())
    cycles = len(next(iter(history.values())))
    for unit in units:
        fixture.compile_source(unit, *units[:2] if unit == "Top" else ())
    final = fixture.scratch / "final.ac"
    linked = fixture.link_saved(units, f"{package}.{top}", final)
    executable, _ = fixture.build_system(
        linked if target == "t-gfsim" else final, target
    )
    args = (
        ["+cycles=" + str(cycles)]
        if target == "verilog"
        else ["--cycles", cycles, "--workers", 1]
    )
    verify_expected_observations(
        history, logs(fixture.run([executable, *args])), cycles * 2
    )
    if target == "t-gfsim" and example == "hello_counter":
        fixture.run([executable, "--workers", 2], fails=True)
    if example == "hello_counter":
        reset_executable, _ = fixture.build_system(
            linked if target == "t-gfsim" else final,
            target,
            reset_cycle=2,
            name="reset",
        )
        # Work observes old Q even on reset; the rising reset overrides the
        # proposed increment and the next cycle starts at the initial value.
        reset_history = {"count": [[[0]], [[1]], [[2]], [[0]], [[1]]]}
        verify_expected_observations(
            reset_history, logs(fixture.run([reset_executable, *args])), 10
        )
        if target == "t-gfsim":
            # The portable generated consumer requires Runtime alone. LLVM
            # discovery is disabled, and sources/units were removed above.
            portable = fixture.scratch / "portable-generated"
            shutil.copytree(fixture.scratch / target, portable)
            build = fixture.scratch / "runtime-only-build"
            fixture.run(
                [
                    "cmake",
                    "-S",
                    portable,
                    "-B",
                    build,
                    f"-DCMAKE_PREFIX_PATH={os.environ['PYC_TOOLCHAIN_ROOT']}",
                    "-DCMAKE_DISABLE_FIND_PACKAGE_LLVM=TRUE",
                    "-DCMAKE_DISABLE_FIND_PACKAGE_MLIR=TRUE",
                    f"-DCMAKE_CXX_COMPILER={fixture.cxx}",
                ]
            )
            fixture.run(
                [
                    "cmake",
                    "--build",
                    build,
                    "--target",
                    "pycircuit_sim",
                    "--parallel",
                    2,
                ]
            )
            verify_expected_observations(
                history, logs(fixture.run([build / "bin/pycircuit_sim", *args])), 10
            )

case_arguments = list(arguments)
case_arguments[6] = str(Path(arguments[6]) / "conditional")
fixture = SourceTest(case_arguments, None, "conditional_test")
shutil.copyfile(
    Path(__file__).with_name("transactional-gfsim-conditional.py"),
    fixture.sources / "conditional.py",
)
fixture.compile_source("conditional")
final = fixture.scratch / "final.ac"
linked = fixture.link_saved(("conditional",), "conditional_test.conditional.Top", final)
executable, _ = fixture.build_system(linked if target == "t-gfsim" else final, target)
args = ["+cycles=5"] if target == "verilog" else ["--cycles", 5, "--workers", 1]
history = {"total": [], "progress": [], "consume": []}
for cycle, (total, accepted) in enumerate(((0, 0), (3, 1), (8, 2), (8, 2), (13, 3))):
    history["total"].append([[total]])
    history["progress"].append([[cycle, accepted]])
    history["consume"].append([] if cycle == 2 else [[3 if cycle == 0 else 5]])
verify_expected_observations(history, logs(fixture.run([executable, *args])), 10)

# Reuse the existing assertion-failure source and its System adaptation.
case_arguments = list(arguments)
case_arguments[6] = str(Path(arguments[6]) / "failure")
fixture = SourceTest(case_arguments, None, "failure_test")
tree = ast.parse((Path(__file__).with_name("transactional-methods.py")).read_text())
source = next(
    ast.literal_eval(node.value)
    for node in tree.body
    if isinstance(node, ast.Assign)
    and any(
        isinstance(name, ast.Name) and name.id == "failure_source"
        for name in node.targets
    )
)
source = source.replace(
    "@ac.module\ndef Failure() -> tuple[ac.bits[8], ac.bits[8]]:",
    "@ac.system\ndef FailureSystem():",
).replace("    return total, heartbeat\n", "")
(fixture.sources / "failure.py").write_text(source)
fixture.compile_source("failure")
final = fixture.scratch / "final.ac"
linked = fixture.link_saved(("failure",), "failure_test.failure.FailureSystem", final)
executable, _ = fixture.build_system(linked if target == "t-gfsim" else final, target)
args = ["+cycles=8"] if target == "verilog" else ["--cycles", 8, "--workers", 1]
actual = Counter()
for row in logs(fixture.run([executable, *args], fails=True)):
    values = tuple(
        int(value["value"], 16 if value["kind"] == "bits4" else 10)
        for value in row["values"]
    )
    actual[(int(row["evaluation_epoch"]), row["spec"]["event"], values)] += 1
expected = Counter()
for epoch, total in ((0, 0), (1, 0), (2, 1), (3, 1), (4, 2)):
    for event, values in (
        ("heartbeat", (total,)),
        ("attempt", (total, total + 1)),
        ("received", (total + 1,)),
    ):
        expected[(epoch, event, values)] += 1
assert actual == expected, (actual, expected)
print(f"saved Linked transactional {target}: PASS")
