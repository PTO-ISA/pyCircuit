"""Independent source semantics, hierarchy identity and dual-backend oracles."""

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

parser = argparse.ArgumentParser()
for name in (
    "repo",
    "source-compiler",
    "linker",
    "emitter",
    "cxx",
    "verilator",
    "scratch",
):
    parser.add_argument("--" + name, required=True)
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def run(command, accepted=True):
    command = list(map(str, command))
    result = subprocess.run(
        command, env=env, cwd=repo, capture_output=True, text=True, timeout=240
    )
    commands.append(
        {
            "command": command,
            "exit_status": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
    )
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if accepted else 1), commands[-1]
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), commands[-1]
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def snapshot(directory):
    return {
        path.relative_to(directory).as_posix(): path.read_bytes()
        for path in directory.rglob("*")
        if path.is_file()
    }


toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next(
    (
        path
        for path in (
            toolroot / "simulator/gfsim/libpyc6_runtime.a",
            toolroot / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    ),
    None,
)
assert runtime is not None, "Runtime archive missing from this build/install"
with tempfile.TemporaryDirectory(
    prefix="defaults-composition-", dir=scratch
) as temporary:
    build = Path(temporary)
    source = build / "source"
    source.mkdir()
    design = source / "design.py"
    shutil.copyfile(fixtures / "defaults-composition-design.py", design)
    unit = build / "unit"

    def compile_source(path, output, accepted=True, replace=False, interfaces=()):
        arguments = [
            "compile",
            "-c",
            path,
            "--source-root",
            source,
            "--package-prefix",
            "defaults_probe",
            "-o",
            output,
        ]
        if replace:
            arguments.append("--replace")
        for interface in interfaces:
            arguments.extend(["-I", interface])
        return cli(*arguments, accepted=accepted)

    compile_source(design, unit)
    unit_receipt = json.loads((unit / "unit.json").read_text())
    unit_text = (unit / unit_receipt["files"]["body"]).read_text()
    contracts = [
        line for line in unit_text.splitlines() if 'ac.return_form = "single"' in line
    ]
    assert contracts, "typed-result modules must publish source-call contracts"
    for contract in contracts:
        assert all(
            name in contract
            for name in (
                "ac.domain_inputs =",
                "ac.parameters =",
                "ac.result_constraints =",
                'ac.return_form = "single"',
            )
        )
        parameters = contract.split("ac.parameters = [", 1)[1].split(
            "], ac.result_constraints", 1
        )[0]
        assert "present = true" not in parameters
        if parameters:
            assert "default = {present = false}" in parameters
    final = build / "defaults.ac"
    cli("link", unit, "--top", "defaults_probe.design.Top", "-o", final)
    # Each syntactic call remains an instance; the shared local value is fanout.
    # Top owns two Middle calls, four Add occurrences and one TableProbe;
    # Middle owns one Cell.
    final_text = final.read_text()

    def source_calls(text, owner):
        calls = []
        for line in text.splitlines():
            if '"ac.instance"' not in line:
                continue
            match = re.search(r"callee = @defaults_probe\.design\.(\w+)", line)
            occurrence = line.split("parameters =")[0]
            if match and f"definition = @defaults_probe.design.{owner}" in occurrence:
                calls.append(match[1])
        return calls

    calls = source_calls(final_text, "Top")
    assert sorted(calls) == ["Add"] * 4 + ["Middle", "Middle", "TableProbe"], calls
    assert source_calls(final_text, "Middle") == ["Cell"]
    assert '"pyc_clk"' in final_text and '"pyc_rst"' in final_text
    twins = build / "twins.ac"
    cli("link", unit, "--top", "defaults_probe.design.SameInputs", "-o", twins)
    twin_calls = source_calls(twins.read_text(), "SameInputs")
    assert twin_calls == ["Cell", "Cell"], twin_calls
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", build / target)
    receipt = json.loads((build / "cpp/generated.json").read_text())
    cpp = [
        build / "cpp" / row["path"]
        for row in receipt["files"]
        if row["path"].endswith(".cpp")
    ]
    assert cpp, "Generated source-owned C++ translation unit missing"
    runner = build / "runner"
    run(
        [
            args.cxx,
            "-std=c++20",
            "-pthread",
            "-I" + str(repo / "include"),
            "-I" + str(build / "cpp"),
            fixtures / "defaults-composition.cpp",
            *cpp,
            runtime,
            "-o",
            runner,
        ]
    )
    traces = []
    for workers in (1, 2):
        trace = run([runner, str(workers)]).stdout
        (scratch / f"workers-{workers}.stdout").write_text(trace)
        traces.append([row for row in trace.splitlines() if row.startswith("WORK ")])
    assert len(traces[0]) == 72 and traces[0] == traces[1]
    receipt = json.loads((build / "verilog/generated.json").read_text())
    rtl = [
        build / "verilog" / row["path"]
        for row in receipt["files"]
        if row["role"] == "rtl"
    ]
    rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
    with tempfile.TemporaryDirectory(prefix="defaults-rtl-") as rtl_temporary:
        rtl_build = Path(rtl_temporary)
        run(
            [
                args.verilator,
                "--binary",
                "--timing",
                "--top-module",
                "tb",
                "--prefix",
                "Vdefaults",
                "--Mdir",
                rtl_build,
                "-j",
                "2",
                "-Wno-fatal",
                *sorted((repo / "include/verilog").glob("*.v")),
                *rtl,
                fixtures / "defaults-composition.sv",
            ]
        )
        rtl_result = run([rtl_build / "Vdefaults"])
        (scratch / "rtl.stdout").write_text(rtl_result.stdout)
        rtl_trace = [
            row for row in rtl_result.stdout.splitlines() if row.startswith("WORK ")
        ]
        assert rtl_trace == traces[0]

    text = design.read_text()
    alias = """import pycircuit as ac
@ac.struct
class Inner:
    payload: ac.u5 = 7
@ac.struct
class Outer:
    inner: Inner = Inner()
@ac.rule
def modify(a, b):
    a.inner.payload = 11
    return {"out": b.inner.payload}
@ac.module
def Alias() -> {"out": ac.u5}:
    owner: Outer = Outer()
    return modify(owner, owner)
"""
    hidden = """import pycircuit as ac
@ac.struct
class Result:
    value: ac.u1
@ac.module
def Hidden() -> Result:
    owner: ac.u1 = 0
    return Result(value=pyc_clk)
"""
    kind_mapping = """from typing import Annotated
import pycircuit as ac
@ac.struct
class Flag:
    value: ac.u1
@ac.module
def Child(flag: bool) -> Flag:
    return Flag(value=flag)
@ac.module
def Mapping(flag: bool) -> {"out": Annotated[int, range(1 << 1)]}:
    unused = Child(flag)
    return {"out": flag}
"""
    overridden = """import pycircuit as ac
@ac.struct
class Record:
    value: ac.u5 = 32
@ac.module
def Explicit(value: ac.u5) -> Record:
    return Record(value=value)
"""
    readonly = source / "field_readonly.py"
    readonly.write_text(
        alias.replace("    a.inner.payload = 11\n", "").replace(
            'return {"out": b.inner.payload}',
            'return {"out": a.inner.payload ^ b.inner.payload}',
        )
    )
    compile_source(readonly, build / "field-readonly-unit")
    kind_valid = source / "mapping_valid.py"
    kind_valid.write_text(
        kind_mapping.replace('return {"out": flag}', 'return {"out": 0}')
    )
    compile_source(kind_valid, build / "mapping-valid-unit")
    cases = {
        "table-default-short": (
            text.replace("Word(value=5, mark=2),\n                              ", ""),
            ("table", "length", "count", "extent", "element"),
        ),
        "table-default-overridden": (
            text.replace("Word(value=7, mark=4)", "Word(value=32, mark=4)").replace(
                "local = Box()", "local = Box(items=0)"
            ),
            ("default", "range", "represent", "unsigned", "field"),
        ),
        "table-default-runtime": (
            text.replace("Word(value=7, mark=4)", "Word(value=value, mark=4)"),
            ("default", "static", "name", "resolve"),
        ),
        "table-construction-short": (
            text.replace("Word(value=9, mark=1),\n                   ", ""),
            ("table", "length", "count", "extent", "element"),
        ),
        "table-construction-long": (
            text.replace(
                "Word(value=21, mark=3)]",
                "Word(value=21, mark=3), Word(value=1, mark=2)]",
            ),
            ("table", "length", "count", "extent", "element"),
        ),
        "table-construction-wrong-element": (
            text.replace("Word(value=9, mark=1)", "9"),
            (
                "table",
                "struct",
                "type",
                "element",
                "boundary",
                "aggregate literal requires contextual zero",
            ),
        ),
        "table-construction-wrong-nominal": (
            text.replace(
                "@ac.struct\nclass Box:",
                "@ac.struct\nclass OtherWord:\n    value: ac.u5\n    mark: ac.u3\n\n@ac.struct\nclass Box:",
            ).replace("Word(value=9, mark=1)", "OtherWord(value=9, mark=1)"),
            ("nominal", "struct", "type", "element", "boundary"),
        ),
        "table-zero-extent": (
            text.replace("items: ac.table[3, Word]", "items: ac.table[0, Word]"),
            ("table", "extent", "positive", "shape"),
        ),
        "table-negative-extent": (
            text.replace("items: ac.table[3, Word]", "items: ac.table[(0 - 3), Word]"),
            ("table", "extent", "positive", "shape"),
        ),
        "table-recursive-field": (
            text.replace("items: ac.table[3, Word]", "items: ac.table[3, Box]"),
            ("cycle", "recursive", "struct", "nominal"),
        ),
        "table-partial-field-writer": (
            text.replace("    local.flag = 0", "    local.items[0].value = value"),
            ("table", "index", "target", "field", "writ", "assign"),
        ),
        "tuple-without-context": (
            text.replace(
                "    local = Box()", "    untyped = (1, 2, 3)\n    local = Box()"
            ),
            ("tuple", "literal", "context", "expression", "unsupported"),
        ),
        "list-without-context": (
            text.replace(
                "    local = Box()", "    untyped = [1, 2, 3]\n    local = Box()"
            ),
            ("list", "literal", "context", "expression", "unsupported"),
        ),
        "invalid-default-overridden": (
            overridden,
            ("default", "range", "represent", "unsigned", "field"),
        ),
        "invalid-unused-default": (
            text + "\n@ac.struct\nclass Unused:\n    bad: ac.u2 = 4\n",
            ("default", "range", "represent", "unsigned", "field"),
        ),
        "default-runtime-name": (
            text.replace("payload: ac.u5 = (1 + 6)", "payload: ac.u5 = value"),
            ("default", "static", "name", "resolve"),
        ),
        "default-sibling-name": (
            text.replace("payload: ac.u5 = (1 + 6)", "payload: ac.u5 = ready"),
            ("default", "static", "name", "resolve"),
        ),
        "default-host-call": (
            text.replace("payload: ac.u5 = (1 + 6)", "payload: ac.u5 = print(7)"),
            ("default", "static", "call", "constructor"),
        ),
        "default-negative": (
            text.replace("payload: ac.u5 = (1 + 6)", "payload: ac.u5 = (0 - 1)"),
            ("default", "range", "unsigned", "negative", "represent"),
        ),
        "nominal-default": (
            text.replace(
                "@ac.struct\nclass Parcel:",
                "@ac.struct\nclass Other:\n    ready: ac.u1 = 1\n    payload: ac.u5 = 7\n\n@ac.struct\nclass Parcel:",
            ).replace("configured: Inner = Inner()", "configured: Inner = Other()"),
            ("default", "nominal", "type", "struct"),
        ),
        "recursive-struct-default": (
            text.replace("raw: Inner", "raw: Parcel"),
            ("cycle", "recursive", "struct"),
        ),
        "unknown-field": (
            text.replace("local = Parcel()", "local = Parcel(missing=1)"),
            ("field", "keyword", "constructor"),
        ),
        "duplicate-field": (
            text.replace("local = Parcel()", "local = Parcel(mark=1, mark=2)"),
            ("field", "keyword", "duplicate", "syntax"),
        ),
        "field-writable-alias": (alias, ("alias", "overlap", "owner", "writ")),
        "field-two-writers": (
            alias.replace(
                "    a.inner.payload = 11\n",
                "    a.inner.payload = 11\n    b.inner.payload = 13\n",
            ),
            ("alias", "overlap", "owner", "writ"),
        ),
        "recursive-module": (
            text.replace(
                "child = Cell(enable, index, value)",
                "child = Middle(enable, index, value)",
            ),
            ("cycle", "recursive", "module", "call"),
        ),
        "dynamic-module": (
            text.replace(
                "    left = Middle(left_enable, index, value)",
                "    if left_enable:\n        left = Middle(left_enable, index, value)",
            ),
            ("module", "scope", "call", "statement", "persistent"),
        ),
        "rule-module-call": (
            text.replace("local = Parcel()", "local = Cell(enable, index, value)"),
            ("rule", "module", "call", "constructor"),
        ),
        "annotated-module-call": (
            text.replace(
                "left = Middle(left_enable, index, value)",
                "left: ChildResult = Middle(left_enable, index, value)",
            ),
            ("annotation", "annotated", "call", "persistent", "initializer"),
        ),
        "mapping-kind-after-unused-call": (
            kind_mapping,
            ("kind", "bool", "integer", "type"),
        ),
        "hidden-clock-assignment": (
            text.replace(
                "    left = Middle(left_enable, index, value)",
                "    pyc_clk = 0\n    left = Middle(left_enable, index, value)",
            ),
            ("pyc_clk", "reserved", "name", "binding", "shadow"),
        ),
        "hidden-reset-assignment": (
            text.replace(
                "    left = Middle(left_enable, index, value)",
                "    pyc_rst = 0\n    left = Middle(left_enable, index, value)",
            ),
            ("pyc_rst", "reserved", "name", "binding", "shadow"),
        ),
        "hidden-clock-free-name": (hidden, ("pyc_clk", "name", "resolve", "binding")),
        "hidden-clock-call-argument": (
            text.replace(
                "left = Middle(left_enable, index, value)",
                "left = Middle(pyc_clk, index, value)",
            ),
            ("pyc_clk", "name", "resolve", "binding"),
        ),
    }
    unit_before = snapshot(unit)
    for name, (invalid, diagnostic) in cases.items():
        design.write_text(invalid)
        result = compile_source(design, unit, accepted=False, replace=True)
        assert any(word in result.stderr.lower() for word in diagnostic), commands[-1]
        if name == "mapping-kind-after-unused-call":
            assert (
                "integer port range must be a power of two" not in result.stderr
            ), commands[-1]
        assert snapshot(unit) == unit_before, name
        absent = build / ("invalid-" + name)
        compile_source(design, absent, accepted=False)
        assert not absent.exists(), name
    design.write_text(text)

    provider = source / "provider.py"
    provider.write_text(
        """import pycircuit as ac
@ac.struct
class Foreign:
    ready: ac.u1 = 1
    payload: ac.u5 = 7
@ac.module
def Provider(value: ac.u5) -> {"out": ac.u5}:
    return {"out": value}
"""
    )
    provider_unit = build / "provider-unit"
    compile_source(provider, provider_unit)
    caller = source / "caller.py"
    baseline = """import pycircuit as ac
from defaults_probe.provider import Provider
@ac.module
def Caller(value: ac.u5) -> {"out": ac.u5}:
    return {"out": value}
"""
    caller.write_text(baseline)
    caller_unit = build / "caller-unit"
    compile_source(caller, caller_unit, interfaces=(provider_unit,))
    before = snapshot(caller_unit)
    # This slice does not introduce source struct imports or publish defaults.
    # Both conveniences reject against an otherwise valid imported-unit caller.
    imported_cases = {
        "imported-omitted-default": """import pycircuit as ac
from defaults_probe.provider import Foreign
@ac.module
def Caller(value: ac.u5) -> {"out": ac.u5}:
    foreign = Foreign(payload=value)
    return {"out": foreign.payload}
""",
        "cross-source-direct-call": baseline.replace(
            '{"out": value}', '{"out": Provider(value).out}'
        ),
    }
    for name, invalid in imported_cases.items():
        caller.write_text(invalid)
        result = compile_source(
            caller,
            caller_unit,
            accepted=False,
            replace=True,
            interfaces=(provider_unit,),
        )
        assert any(
            word in result.stderr.lower()
            for word in ("import", "field", "cross", "source", "call", "default")
        ), commands[-1]
        assert snapshot(caller_unit) == before, name
        absent = build / ("invalid-" + name)
        compile_source(caller, absent, accepted=False, interfaces=(provider_unit,))
        assert not absent.exists(), name
    # Compile consumer with the provider Python removed: only the header may
    # authorize Frame -> Table[3,Cell] -> Cell's nominal fields.
    table_provider_source = source / "table_provider.py"
    table_caller_source = source / "table_caller.py"
    shutil.copyfile(
        fixtures / "defaults-composition-table-provider.py", table_provider_source
    )
    shutil.copyfile(
        fixtures / "defaults-composition-table-caller.py", table_caller_source
    )
    table_provider = build / "table-provider-unit"
    table_caller = build / "table-caller-unit"
    compile_source(table_provider_source, table_provider)
    table_provider_source.unlink()
    compile_source(table_caller_source, table_caller, interfaces=(table_provider,))
    table_final = build / "table-closure.ac"
    cli(
        "link",
        table_provider,
        table_caller,
        "--top",
        "defaults_probe.table_caller.Caller",
        "-o",
        table_final,
    )
    for target in ("cpp", "verilog"):
        cli("emit", table_final, "--target", target, "-o", build / ("table-" + target))
    protected_final = table_final.read_bytes()
    protected_targets = {
        target: snapshot(build / ("table-" + target)) for target in ("cpp", "verilog")
    }
    protected_caller = snapshot(table_caller)

    def table_protected():
        assert table_final.read_bytes() == protected_final
        assert snapshot(table_caller) == protected_caller
        assert all(
            snapshot(build / ("table-" + target)) == saved
            for target, saved in protected_targets.items()
        )

    caller_text = table_caller_source.read_text()
    wrong_nominal = caller_text.replace(
        "@ac.module",
        "@ac.struct\nclass Shadow:\n    lead: ac.u3\n    cells: ac.table[3, ac.u9]\n    trail: ac.u1\n\n@ac.module",
        1,
    ).replace("-> Frame:", "-> Shadow:")
    table_caller_source.write_text(wrong_nominal)
    compile_source(
        table_caller_source,
        table_caller,
        accepted=False,
        replace=True,
        interfaces=(table_provider,),
    )
    table_protected()
    absent_caller = build / "wrong-table-nominal-unit"
    compile_source(
        table_caller_source, absent_caller, accepted=False, interfaces=(table_provider,)
    )
    assert not absent_caller.exists()
    table_caller_source.write_text(caller_text)

    # Missing provider closure must preserve the final IR and both targets.
    cli(
        "link",
        table_caller,
        "--top",
        "defaults_probe.table_caller.Caller",
        "-o",
        table_final,
        "--replace",
        accepted=False,
    )
    table_protected()
    missing_table_final = build / "missing-table-closure.ac"
    cli(
        "link",
        table_caller,
        "--top",
        "defaults_probe.table_caller.Caller",
        "-o",
        missing_table_final,
        accepted=False,
    )
    assert not missing_table_final.exists()

    for kind in ("interface", "body"):
        substituted = build / ("table-substituted-" + kind)
        shutil.copytree(table_provider, substituted)
        metadata = json.loads((substituted / "unit.json").read_text())
        payload = substituted / metadata["files"][kind]
        original = payload.read_text()
        changed = original.replace('name = "stamp"', 'name = "different_stamp"', 1)
        assert (
            changed != original
        ), "nominal field mutation did not reach provider declaration"
        payload.write_text(changed)
        # Each schema still has the same packed width; name authority differs.
        cli(
            "link",
            substituted,
            table_caller,
            "--top",
            "defaults_probe.table_caller.Caller",
            "-o",
            table_final,
            "--replace",
            accepted=False,
        )
        table_protected()
        absent = build / ("substituted-" + kind + ".ac")
        cli(
            "link",
            substituted,
            table_caller,
            "--top",
            "defaults_probe.table_caller.Caller",
            "-o",
            absent,
            accepted=False,
        )
        assert not absent.exists()

    invalid_table_final = build / "invalid-table-final.ac"
    table_final_text = table_final.read_text()
    invalid_text = table_final_text.replace(
        "defaults_probe.table_provider.Cell",
        "defaults_probe.table_provider.MissingCell",
        1,
    )
    assert invalid_text != table_final_text
    invalid_table_final.write_text(invalid_text)
    for target in ("cpp", "verilog"):
        cli(
            "emit",
            invalid_table_final,
            "--target",
            target,
            "-o",
            build / ("table-" + target),
            "--replace",
            accepted=False,
        )
        table_protected()
        absent = build / ("invalid-table-" + target)
        cli(
            "emit",
            invalid_table_final,
            "--target",
            target,
            "-o",
            absent,
            accepted=False,
        )
        assert not absent.exists()

    inputs = [
        fixtures / ("defaults-composition" + suffix)
        for suffix in (
            "-design.py",
            "-table-provider.py",
            "-table-caller.py",
            ".py",
            ".cpp",
            ".sv",
        )
    ]
    (scratch / "candidate.json").write_text(
        json.dumps(
            {
                "fixture_sha256": {
                    str(path.relative_to(repo)): hashlib.sha256(
                        path.read_bytes()
                    ).hexdigest()
                    for path in inputs
                },
                "verified_final_sha256": hashlib.sha256(final.read_bytes()).hexdigest(),
                "successful_work_samples": 72,
                "workers": [1, 2],
                "rtl": "verilator",
                "rejected_cases": sorted([*cases, *imported_cases]),
                "failed_compile_preserved_unit": True,
                "same_inputs_stateful_occurrences": 2,
                "readonly_field_alias_accepted": True,
                "mapping_kind_positive_control_accepted": True,
                "generated_child_failure_and_discard_retry": True,
                "native_data_value_known_z_planes_preserved": True,
                "scalar_table_literal_and_expression_images": 7,
                "table_field_saved_flagged_replaced_selected_zero_defaults": True,
                "table_field_nominal_header_only_consumer": True,
                "table_field_body_header_substitutions_rejected": True,
                "table_field_failed_link_emit_preserved_products": True,
            },
            indent=2,
        )
        + "\n"
    )
