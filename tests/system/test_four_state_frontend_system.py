"""Four-state authoring, verification-only uses, reset policy, and System rows."""

from __future__ import annotations

import json
from pathlib import Path

import pytest
from test_physical_rtl_export import SOURCE as PHYSICAL_SOURCE
from test_physical_rtl_export import simulate
from test_source_system_execution import _cli, _compile, _install, _ok, _records, _run

pytestmark = pytest.mark.system


PATTERNS = {
    1: "x",
    5: "10XZ1",
    65: "1" + "0" * 30 + "x" + "z" + "1" * 32,
    130: "10xz" + "01" * 63,
}


def _planes(pattern: str) -> tuple[int, int, int]:
    value = known = z = 0
    for symbol in pattern.lower():
        value <<= 1
        known <<= 1
        z <<= 1
        if symbol == "1":
            value |= 1
            known |= 1
        elif symbol == "0":
            known |= 1
        elif symbol == "z":
            z |= 1
        else:
            assert symbol == "x"
    return value, known, z


def _source(tmp_path: Path, name: str, text: str) -> tuple[Path, Path]:
    root = tmp_path / (name + "-source")
    root.mkdir()
    path = root / (name + ".py")
    path.write_text(text, encoding="utf-8")
    return root, path


def _published(tmp_path: Path, name: str, text: str, providers=()) -> Path:
    root, path = _source(tmp_path, name, text)
    unit = tmp_path / (name + "-unit")
    _ok(_compile(root, path.name, unit, providers))
    return unit


def _link(
    units,
    top: str,
    output: Path,
    *,
    reset: str | None = None,
    replace: bool = False,
):
    arguments = ["link", *(str(unit) for unit in units), "--top", top]
    if reset is not None:
        arguments += ["--reset-semantics", reset]
    arguments += ["-o", str(output)]
    if replace:
        arguments.append("--replace")
    return _cli(*arguments)


@pytest.mark.parametrize("width", sorted(PATTERNS))
def test_four_state_literal_exact_width_and_source_kind(
    tmp_path: Path, width: int
) -> None:
    pattern = PATTERNS[width]
    unit = _published(
        tmp_path,
        "literal",
        f"""import pycircuit as ac
@ac.module
def Top() -> ac.bits[{width}]:
    return ac.four_state({pattern!r})
""",
    )
    final = tmp_path / "literal.ac"
    _ok(_link((unit,), "checks.literal.Top", final))
    text = final.read_text()
    assert pattern.lower() in text
    # An explicit pattern remains fixed Bits authority. It must not create an
    # Integer closed-source constant merely because every symbol is known.
    assert "closedSourceConstant" not in text


MALFORMED = {
    "empty": "''",
    "space": "'10 x'",
    "prefix": "'0b10'",
    "question": "'10?'",
    "separator": "'10_01'",
    "integer": "3",
    "runtime": "text",
}


@pytest.mark.parametrize("case", sorted(MALFORMED))
def test_four_state_literal_malformed_and_replacement_protected(
    tmp_path: Path, case: str
) -> None:
    root, path = _source(
        tmp_path,
        "literal",
        """import pycircuit as ac
@ac.module
def Top() -> ac.bits[4]:
    return ac.four_state('10xz')
""",
    )
    unit = tmp_path / "unit"
    _ok(_compile(root, path.name, unit))
    before = {item.name: item.read_bytes() for item in unit.iterdir() if item.is_file()}
    argument = MALFORMED[case]
    prefix = "text = '10xz'\n" if case == "runtime" else ""
    path.write_text(
        prefix
        + f"""import pycircuit as ac
@ac.module
def Top() -> ac.bits[4]:
    return ac.four_state({argument})
""",
        encoding="utf-8",
    )
    result = _cli(
        "compile",
        "-c",
        str(path),
        "--source-root",
        str(root),
        "--package-prefix",
        "checks",
        "-o",
        str(unit),
        "--replace",
    )
    assert result.returncode != 0
    assert before == {
        item.name: item.read_bytes() for item in unit.iterdir() if item.is_file()
    }


def test_four_state_literal_struct_table_and_closed_initializers(
    tmp_path: Path,
) -> None:
    unit = _published(
        tmp_path,
        "aggregate",
        """import pycircuit as ac
@ac.struct
class Payload:
    small: ac.bits[5]
    wide: ac.bits[65]
@ac.module
def Top() -> tuple[Payload, ac.table[2, ac.bits[5]]]:
    saved: Payload = Payload(small=ac.four_state('10xz1'),
                             wide=ac.four_state('1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'))
    entries: ac.table[2, ac.bits[5]] = (ac.four_state('10xz1'), ac.four_state('z01x0'))
    @ac.rule
    def hold():
        saved = saved
        entries = entries
    return saved, entries
""",
    )
    final = tmp_path / "aggregate.ac"
    _ok(_link((unit,), "checks.aggregate.Top", final))
    for target in ("cpp", "verilog"):
        _ok(_cli("emit", str(final), "--target", target, "-o", str(tmp_path / target)))


def test_four_state_literal_zero_extension_and_explicit_narrowing(
    tmp_path: Path,
) -> None:
    unit = _published(
        tmp_path,
        "widths",
        """import pycircuit as ac
@ac.module
def Extend() -> ac.bits[5]: return ac.four_state('xz')
@ac.module
def Narrow() -> ac.u1: return ac.four_state('xz')[:1]
""",
    )
    for name in ("Extend", "Narrow"):
        _ok(_link((unit,), "checks.widths." + name, tmp_path / (name + ".ac")))

    root, path = _source(
        tmp_path,
        "bad_width",
        """import pycircuit as ac
@ac.module
def Top() -> ac.u1: return ac.four_state('xz')
""",
    )
    result = _compile(root, path.name, tmp_path / "bad-unit")
    assert result.returncode != 0
    assert "narrow" in result.stderr or "width" in result.stderr


DIAGNOSTIC = """import pycircuit as ac
def exact(value: ac.bits[4]) -> bool:
    return ac.case_eq(value, ac.four_state('10xz'))
@ac.module
def Top(value: ac.bits[4]) -> ac.bits[4]:
    same = exact(value)
    known = ac.is_known(value)
    @ac.rule
    def check():
        if same:
            assert same
            ac.log('info', 'exact', same)
        assert known or not known
    return value
"""


def test_case_eq_and_is_known_diagnostic_alias_helper_and_control_allowed(
    tmp_path: Path,
) -> None:
    unit = _published(tmp_path, "diagnostic", DIAGNOSTIC)
    final = tmp_path / "diagnostic.ac"
    _ok(_link((unit,), "checks.diagnostic.Top", final))


def test_verification_helper_struct_is_field_sensitive_and_assert_allows_write(
    tmp_path: Path,
) -> None:
    unit = _published(
        tmp_path,
        "field_sensitive",
        """import pycircuit as ac
@ac.struct
class Inspection:
    observed: ac.u1
    payload: ac.bits[4]
def inspect(value: ac.bits[4]) -> Inspection:
    return Inspection(observed=ac.is_known(value), payload=value)
@ac.module
def Top(value: ac.bits[4]) -> ac.bits[4]:
    result = inspect(value)
    saved: ac.bits[4] = 0
    @ac.rule
    def update():
        assert result.observed
        ac.log('info', 'known', result.observed)
        saved = result.payload
    return result.payload
""",
    )
    final = tmp_path / "field-sensitive.ac"
    _ok(_link((unit,), "checks.field_sensitive.Top", final))


def test_verification_helper_observer_field_cannot_escape_as_output(
    tmp_path: Path,
) -> None:
    root, path = _source(
        tmp_path,
        "field_escape",
        """import pycircuit as ac
@ac.struct
class Inspection:
    observed: ac.u1
    payload: ac.bits[4]
def inspect(value: ac.bits[4]) -> Inspection:
    return Inspection(observed=ac.is_known(value), payload=value)
@ac.module
def Top(value: ac.bits[4]) -> ac.u1:
    return inspect(value).observed
""",
    )
    result = _compile(root, path.name, tmp_path / "unit")
    assert result.returncode != 0
    assert "verification" in result.stderr or "diagnostic" in result.stderr


FUNCTIONAL_ESCAPES = {
    "output": "return ac.case_eq(value, ac.four_state('10xz'))",
    "state": "saved = ac.is_known(value)\n    return value",
    "index": "return entries[ac.is_known(value)]",
    "guard": "return value",
    "call-argument": "return Sink(ac.case_eq(value, ac.four_state('10xz')))",
}


@pytest.mark.parametrize("case", sorted(FUNCTIONAL_ESCAPES))
def test_verification_predicate_functional_escape_rejected(
    tmp_path: Path, case: str
) -> None:
    body = FUNCTIONAL_ESCAPES[case]
    setup = ""
    result = "ac.bits[4]"
    if case == "state":
        setup = """    saved: bool = False
    @ac.rule
    def write():
        saved = ac.is_known(value)
"""
        body = "return value"
    elif case == "index":
        setup = "    entries: ac.table[2, ac.bits[4]] = (ac.four_state('0000'), ac.four_state('1111'))\n"
    elif case == "guard":
        setup = """    @ac.rule(guard=lambda: ac.is_known(value))
    def guarded():
        pass
"""
    elif case == "call-argument":
        setup = ""
    if case == "output":
        result = "bool"
    prefix = (
        "@ac.module\ndef Sink(flag: bool) -> ac.bits[4]:\n    return ac.four_state('0000')\n"
        if case == "call-argument"
        else ""
    )
    source = f"""import pycircuit as ac
{prefix}@ac.module
def Top(value: ac.bits[4]) -> {result}:
{setup}    {body}
"""
    root, path = _source(tmp_path, "escape", source)
    outcome = _compile(root, path.name, tmp_path / "unit")
    assert outcome.returncode != 0
    assert "verification" in outcome.stderr or "diagnostic" in outcome.stderr


def test_knownness_does_not_become_integer_static_proof(tmp_path: Path) -> None:
    root, path = _source(
        tmp_path,
        "static",
        """import pycircuit as ac
@ac.module
def Generic(value: ac.bits[W], *, W: int = 4) -> ac.bits[W]: return value
@ac.module
def Top(value: ac.bits[4]) -> ac.bits[4]:
    return Generic(value, W=ac.is_known(value))
""",
    )
    result = _compile(root, path.name, tmp_path / "unit")
    assert result.returncode != 0


SYSTEM_SOURCE = """import pycircuit as ac
@ac.module
def Register(incoming: ac.u8) -> ac.u8:
    saved: ac.u8 = 0
    @ac.rule
    def update(): saved = incoming
    return saved
@ac.system(samples=((0, 0), (1, ac.four_state('x'))))
def Scenario():
    observed = Register(ac.four_state('10xz0011'))
    @ac.rule
    def check():
        assert ac.case_eq(observed, ac.four_state('00000000'))
"""

GUARD_SYSTEM_SOURCE = """import pycircuit as ac
@ac.system(samples=((0, 0), (1, ac.four_state('x'))))
def Scenario():
    marker: ac.u1 = 0
    @ac.rule(guard=lambda: ac.four_state('x'))
    def effect():
        marker = 1
"""


def _generated_system(final: Path, tmp_path: Path, target: str) -> Path:
    tmp_path.mkdir(parents=True, exist_ok=True)
    generated = tmp_path / target
    _ok(_cli("emit", str(final), "--target", target, "-o", str(generated)))
    build = tmp_path / (target + "-build")
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
    _ok(_run(["cmake", "--build", str(build), "--target", "pycircuit_sim"]))
    return next(build.rglob("pycircuit_sim"))


def test_system_samples_and_reset_policy_serialize_per_final(tmp_path: Path) -> None:
    unit = _published(tmp_path, "scenario", SYSTEM_SOURCE)
    finals = {}
    for policy in ("strict", "if_one"):
        final = tmp_path / (policy + ".ac")
        _ok(_link((unit,), "checks.scenario.Scenario", final, reset=policy))
        text = final.read_text()
        assert f'reset = "{policy}"' in text
        assert "samples" in text
        finals[policy] = final.read_bytes()
        cpp = _generated_system(final, tmp_path / policy, "cpp")
        for workers in (1, 2):
            run = _run([str(cpp), "--workers", str(workers)])
            assert (run.returncode == 0) == (policy == "if_one"), (
                run.stdout + run.stderr
            )
        rtl = _generated_system(final, tmp_path / policy, "verilog")
        run = _run([str(rtl)])
        assert (run.returncode == 0) == (policy == "if_one"), run.stdout + run.stderr
    assert finals["strict"] != finals["if_one"]


def test_system_samples_reject_explicit_cycles_and_guard_x_survives_if_one_reset_x(
    tmp_path: Path,
) -> None:
    unit = _published(tmp_path, "guard", GUARD_SYSTEM_SOURCE)
    final = tmp_path / "if-one.ac"
    _ok(_link((unit,), "checks.guard.Scenario", final, reset="if_one"))
    runner = _generated_system(final, tmp_path, "cpp")
    explicit = _run([str(runner), "--cycles", "2"])
    assert explicit.returncode != 0
    automatic = _run([str(runner)])
    assert automatic.returncode != 0


def test_physical_reset_unknown_obeys_final_policy(tmp_path: Path) -> None:
    source, path = _source(tmp_path, "physical", PHYSICAL_SOURCE)
    unit = tmp_path / "physical-unit"
    _ok(_compile(source, path.name, unit))
    for policy in ("strict", "if_one"):
        final = tmp_path / (policy + ".ac")
        _ok(_link((unit,), "checks.physical.Stateless", final, reset=policy))
        output = tmp_path / (policy + "-physical")
        _ok(
            _cli(
                "emit",
                str(final),
                "--target",
                "verilog",
                "--rtl-mode",
                "physical",
                "-o",
                str(output),
            )
        )
        simulation = tmp_path / (policy + "-simulation")
        simulation.mkdir()
        transcript = simulate(
            output,
            simulation,
            "rst=1'bx;edge_up();",
            stateless=True,
        )
        assert ("PYCIRCUIT_PHYSICAL_FAULT" in transcript) == (policy == "strict")


def test_four_state_raw_masks_native_and_rtl_oracle(tmp_path: Path) -> None:
    pattern = "10xz"
    value, known, z = _planes(pattern)
    unit = _published(
        tmp_path,
        "mask",
        f"""import pycircuit as ac
@ac.module
def Top() -> ac.bits[4]: return ac.four_state({pattern!r})
""",
    )
    final = tmp_path / "mask.ac"
    _ok(_link((unit,), "checks.mask.Top", final))
    cpp = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(cpp)))
    (cpp / "mask_probe.cpp").write_text(
        f"""#include "pycircuit_system.hpp"
int main(){{pyc_dut d(1);d.system().Build();d.system().Reset();if(d.system().Step()!=gfsim::SimStepResult::Running)return 1;auto p=d.sample().result.packed();return p.value().value()!={value}||p.knownMask().value()!={known}||p.zMask().value()!={z};}}
""",
        encoding="utf-8",
    )
    with (cpp / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(mask_probe mask_probe.cpp)\n"
            "target_link_libraries(mask_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "cpp-build"
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
    _ok(_run(["cmake", "--build", str(build), "--target", "mask_probe"]))
    _ok(_run([str(next(build.rglob("mask_probe")))]))

    rtl = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(rtl)))
    bench = tmp_path / "tb.sv"
    bench.write_text(
        "module tb;wire[3:0] result;pyc_root dut(.*);initial begin #1;"
        f"if(result!==4'b{pattern})$fatal; $finish;end endmodule\n",
        encoding="utf-8",
    )
    manifest = json.loads((rtl / "generated.json").read_text())
    sources = [rtl / row["path"] for row in manifest["files"] if row["role"] == "rtl"]
    binary = tmp_path / "mask.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "tb",
                "-o",
                str(binary),
                *map(str, sources),
                str(bench),
            ]
        )
    )
    _ok(_run(["vvp", str(binary)]))


def _bits4(pattern: str) -> dict[str, object]:
    value, known, z = _planes(pattern)
    digits = (len(pattern) + 3) // 4
    return {
        "kind": "bits4",
        "width": len(pattern),
        "value": f"{value:0{digits}x}",
        "known": f"{known:0{digits}x}",
        "z": f"{z:0{digits}x}",
    }


def test_four_state_logs_preserve_bits4_planes_and_known_legacy_values(
    tmp_path: Path,
) -> None:
    wide65 = PATTERNS[65].lower()
    wide130 = PATTERNS[130].lower()
    unit = _published(
        tmp_path,
        "observations",
        f"""import pycircuit as ac
@ac.system(samples=((1, 0),))
def Observe():
    @ac.rule
    def publish():
        ac.log('info', 'payloads', ac.four_state('10xz'),
               ac.four_state({wide65!r}), ac.four_state({wide130!r}),
               ac.four_state('101'), 7, True)
""",
    )
    final = tmp_path / "observations.ac"
    _ok(_link((unit,), "checks.observations.Observe", final, reset="if_one"))
    expected = [
        _bits4("10xz"),
        _bits4(wide65),
        _bits4(wide130),
        {"kind": "integer", "value": "5"},
        {"kind": "integer", "value": "7"},
        {"kind": "bool", "value": True},
    ]
    transcripts = []
    for target in ("cpp", "verilog"):
        runner = _generated_system(final, tmp_path / target, target)
        workers = (1, 2) if target == "cpp" else (1,)
        for worker in workers:
            arguments = [str(runner)] + (
                ["--workers", str(worker)] if target == "cpp" else []
            )
            result = _run(arguments)
            assert result.returncode == 0, result.stdout + result.stderr
            logs = [row for row in _records(result.stdout) if row["kind"] == "log"]
            assert len(logs) == 1
            assert logs[0]["spec"]["event"] == "payloads"
            assert logs[0]["values"] == expected
            transcripts.append(logs)
    assert transcripts[0] == transcripts[1] == transcripts[2]


@pytest.mark.parametrize(
    "expression",
    ["ac.four_state('x')", "ac.four_state('" + "1" * 65 + "')"],
    ids=["unknown", "wide"],
)
def test_numeric_reports_reject_unknown_or_wide_bits(
    tmp_path: Path, expression: str
) -> None:
    source = f"""import pycircuit as ac
@ac.system(samples=((0, 0),))
def Observe():
    @ac.rule
    def publish():
        ac.report('numeric', {expression})
"""
    root, path = _source(tmp_path, "bad_report", source)
    unit = tmp_path / "unit"
    compiled = _compile(root, path.name, unit)
    if compiled.returncode != 0:
        assert "report" in compiled.stderr or "numeric" in compiled.stderr
        return
    final = tmp_path / "bad-report.ac"
    linked = _link((unit,), "checks.bad_report.Observe", final, reset="if_one")
    if linked.returncode != 0:
        assert "report" in linked.stderr or "numeric" in linked.stderr
        return
    for target in ("cpp", "verilog"):
        output = tmp_path / target
        emitted = _cli("emit", str(final), "--target", target, "-o", str(output))
        if emitted.returncode != 0:
            assert "report" in emitted.stderr or "numeric" in emitted.stderr
            assert not output.exists()
            continue
        runner = _generated_system(final, tmp_path / (target + "-run"), target)
        result = _run([str(runner)])
        assert result.returncode != 0
        assert not any(row["kind"] == "report" for row in _records(result.stdout))


def test_failed_epoch_publishes_no_four_state_observation(tmp_path: Path) -> None:
    unit = _published(
        tmp_path,
        "failed_log",
        """import pycircuit as ac
@ac.system(samples=((1, 0),))
def Observe():
    marker: ac.u1 = 0
    @ac.rule
    def publish():
        ac.log('info', 'discarded', ac.four_state('10xz'))
    @ac.rule(guard=lambda: ac.four_state('x'))
    def fail():
        marker = 1
""",
    )
    final = tmp_path / "failed-log.ac"
    _ok(_link((unit,), "checks.failed_log.Observe", final, reset="if_one"))
    for target in ("cpp", "verilog"):
        runner = _generated_system(final, tmp_path / target, target)
        result = _run([str(runner)])
        assert result.returncode != 0
        assert not any(row["kind"] == "log" for row in _records(result.stdout))


def test_if_one_reset_x_does_not_mask_unknown_enable_fault(tmp_path: Path) -> None:
    source, path = _source(tmp_path, "enable", PHYSICAL_SOURCE)
    unit = tmp_path / "unit"
    _ok(_compile(source, path.name, unit))
    final = tmp_path / "if-one.ac"
    _ok(_link((unit,), "checks.enable.Pair", final, reset="if_one"))
    output = tmp_path / "physical"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(output),
        )
    )
    simulation = tmp_path / "simulation"
    simulation.mkdir()
    transcript = simulate(
        output,
        simulation,
        "rst=1'bx;enable_second=1'bx;edge_up();",
    )
    assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 1


def test_public_run_uses_finite_samples_and_iverilog_without_cycles(
    tmp_path: Path,
) -> None:
    example = tmp_path / "example"
    example.mkdir()
    (example / "scenario.py").write_text(
        """import pycircuit as ac
@ac.system(samples=((0, 0),))
def Scenario():
    @ac.rule
    def publish():
        ac.log('info', 'payload', ac.four_state('10xz'))
""",
        encoding="utf-8",
    )
    (example / "CMakeLists.txt").write_text(
        """cmake_minimum_required(VERSION 3.25)
project(FourStatePublicRun LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(four_state_public PACKAGE public_four_state
  TOP public_four_state.scenario.Scenario ENTRY_SOURCE scenario SOURCES scenario.py)
""",
        encoding="utf-8",
    )
    for target in ("cpp", "verilog"):
        build = tmp_path / (target + "-run")
        result = _cli(
            "run",
            str(example),
            "--target",
            target,
            "--toolchain",
            str(_install()),
            "--build-dir",
            str(build),
        )
        assert result.returncode == 0, result.stdout + result.stderr
        execution = json.loads((build / "run-execution.json").read_text())
        assert execution["status"] == "success"
        assert execution["sampling_epochs"] == 1
        assert execution["reset_semantics"] == "strict"
        assert execution["backend"] == ("cpp" if target == "cpp" else "iverilog")
        receipt = json.loads((build / target / "generated.json").read_text())
        assert len(receipt["system_samples"]) == 1
        assert receipt["reset_semantics"] == "strict"
        assert receipt["needs_four_state"] is True

        rejected = _cli(
            "run",
            str(example),
            "--target",
            target,
            "--toolchain",
            str(_install()),
            "--build-dir",
            str(tmp_path / (target + "-cycles")),
            "--cycles",
            "1",
        )
        assert rejected.returncode != 0
        assert "--cycles is not allowed with finite System samples" in rejected.stderr


def test_unreachable_four_state_sibling_does_not_select_four_state_backend(
    tmp_path: Path,
) -> None:
    unit = _published(
        tmp_path,
        "reachability",
        """import pycircuit as ac
@ac.module
def Top() -> ac.u1: return 1
@ac.module
def Unused() -> ac.u1: return ac.four_state('x')
""",
    )
    final = tmp_path / "reachability.ac"
    _ok(_link((unit,), "checks.reachability.Top", final))
    output = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(output)))
    receipt = json.loads((output / "generated.json").read_text())
    assert receipt["needs_four_state"] is False
