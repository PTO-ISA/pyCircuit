"""Independent default-width identity checks against both generated backends."""

import argparse
import ast
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

VALUES = (0, 1, 65535, 65536, 131071, 87381, 43690, 0)
VALUES32 = (0, 1, 65535, 65536, 4294967295, 2863311530, 1431655765, 0)
PACKAGE = "history_scalar_parameterized"
MODULE = PACKAGE + ".scalar_parameterized_types.scalar_parameterized_types"
SYSTEM = PACKAGE + ".bench.ScalarParameterizedTypesSystem"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identity_inlining(owned):
    """Prove this bounded source rewrite from the retained original syntax."""
    historical = ast.parse(
        (owned / "oracles/scalar_parameterized_types.py.txt").read_text()
    )
    keep = next(
        node
        for node in historical.body
        if isinstance(node, ast.FunctionDef) and node.name == "keep"
    )
    assert len(keep.body) == 1 and isinstance(keep.body[0], ast.Return)
    assert ast.dump(keep.body[0].value) == ast.dump(
        ast.Name(id="value", ctx=ast.Load())
    )
    root = next(
        node
        for node in historical.body
        if isinstance(node, ast.FunctionDef)
        and node.name == "scalar_parameterized_types"
    )
    assignment, returned = root.body
    assert isinstance(assignment, ast.Assign)
    assert isinstance(assignment.value, ast.Call)
    assert assignment.value.func.id == "keep"
    assert len(assignment.value.args) == 1
    assert assignment.value.args[0].id == "value"
    assert isinstance(returned, ast.Return) and returned.value.id == "result"
    active = ast.parse((owned / "scalar_parameterized_types.py").read_text())
    module = next(node for node in active.body if isinstance(node, ast.FunctionDef))
    assert module.args.kwonlyargs[0].arg == "width"
    assert ast.literal_eval(module.args.kw_defaults[0]) == 17
    assert module.args.args[0].annotation.slice.id == "width"
    assert module.returns.values[0].slice.id == "width"
    assert len(module.body) == 1 and isinstance(module.body[0], ast.Return)
    assert module.body[0].value.values[0].id == "value"


def system_events(stdout, entry, values):
    records = [json.loads(line) for line in stdout.splitlines() if line.startswith("{")]
    events = [item for item in records if item["kind"] == "log"]
    assert len(events) == 16, events
    assert len(records) == 17, records
    for epoch, event in enumerate(events):
        assert event["instance"] == "root"
        assert event["spec"] == {
            "event": "identity",
            "items": [{"kind": "value", "ordinal": 0}],
            "level": "info",
        }
        assert event["values"] == [
            {"kind": "integer", "value": str(values[epoch // 2])}
        ]
        assert int(event["evaluation_epoch"]) == epoch
        assert int(event["commit_epoch"]) == epoch + 1
        assert json.loads(event["site"])["site"]["definition"] == entry
    terminal = records[-1]
    assert terminal["kind"] == "result" and terminal["status"] == "TERMINATED"
    assert int(terminal["epoch_time"]) == 16
    return events


def main():
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
    owned = fixtures / "history-scalar-parameterized-types"
    identity_inlining(owned)
    historical_assets = json.loads((owned / "oracles/manifest.json").read_text())
    for asset in historical_assets["assets"]:
        assert digest(owned / "oracles" / asset["asset"]) == asset["sha256"]
    scratch = Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="scalar-parameterized-", dir=scratch))
    source = work / "source"
    source.mkdir()
    for path in owned.glob("*.py"):
        shutil.copy2(path, source)
    native_root = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        path
        for path in (
            native_root / "simulator/gfsim/libpyc6_runtime.a",
            native_root / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    )
    inputs = [
        Path(__file__),
        *[path for path in owned.rglob("*") if path.is_file()],
        fixtures / "source-historical-scalar-parameterized.cpp",
        fixtures / "source-historical-scalar-parameterized.sv",
        Path(args.source_compiler),
        Path(args.linker),
        Path(args.emitter),
        native_root / "bin/pycircuit-opt",
        runtime,
        repo / "cmake/verify_example.py",
    ]
    input_hashes = {str(path): digest(path) for path in inputs}
    (work / "input-hashes.json").write_text(json.dumps(input_hashes, indent=2) + "\n")
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python/pycircuit/src"),
        PYTHONDONTWRITEBYTECODE="1",
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
    )
    commands = []

    def run(label, command, failure=False):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, capture_output=True, text=True, timeout=180
        )
        (work / (label + ".stdout")).write_text(result.stdout)
        (work / (label + ".stderr")).write_text(result.stderr)
        commands.append(
            {"label": label, "command": command, "exit_status": result.returncode}
        )
        (work / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert (result.returncode != 0) if failure else (result.returncode == 0), (
            label,
            result.stdout,
            result.stderr,
        )
        return result.stdout, result.stderr

    def cli(label, *arguments, failure=False):
        return run(
            label,
            [sys.executable, "-m", "pycircuit.cli", *arguments],
            failure=failure,
        )

    units = []

    def compile_unit(name):
        unit = work / (name + "-unit")
        command = [
            "compile",
            "-c",
            source / (name + ".py"),
            "--source-root",
            source,
            "--package-prefix",
            PACKAGE,
            "-o",
            unit,
        ]
        for provider in units:
            command.extend(["-I", provider])
        cli(name + "-compile", *command)
        units.append(unit)

    compile_unit("scalar_parameterized_types")
    cases = []
    for entry, is_system, source_name, width, values in (
        (MODULE, False, "scalar_parameterized_types", 17, VALUES),
        (SYSTEM, True, "bench", 17, VALUES),
        (
            PACKAGE + ".local_default.LocalDefaultSystem",
            True,
            "local_default",
            17,
            VALUES,
        ),
        (
            PACKAGE + ".different_default.DifferentDefaultSystem",
            True,
            "different_default",
            32,
            VALUES32,
        ),
    ):
        if is_system:
            compile_unit(source_name)
        name = source_name if is_system else "module"
        output = work / name
        output.mkdir()
        final = output / "system.ac"
        cli(name + "-link", "link", *units, "--top", entry, "-o", final)
        observations = []
        root_entries = []
        for target in ("cpp", "verilog"):
            generated = output / target
            cli(
                name + "-emit-" + target,
                "emit",
                final,
                "--target",
                target,
                "-o",
                generated,
            )
            manifest = json.loads((generated / "generated.json").read_text())
            assert manifest["entry"]["definition"] == f'@"{entry}"'
            root_entries.append(manifest["entry"])
            if is_system:
                assert json.loads(
                    (generated / "simulation_verification.json").read_text()
                ) == {
                    "entry": {"definition": f'@"{entry}"', "arguments": []},
                    "entry_source": {"package": PACKAGE, "path": source_name + ".py"},
                    "source_checks": 1,
                }
            else:
                assert len(manifest["entry"]["arguments"]) == 1
                assert "#ac.math_int<17>" in manifest["entry"]["arguments"][0]["value"]
                assert 'name = "width"' in final.read_text()
                assert 'input_names = ["value"]' in final.read_text()
                assert 'output_names = ["result"]' in final.read_text()
            binary = output / ("simulation-" + target)
            if target == "cpp":
                sources = [
                    generated / item["path"]
                    for item in manifest["files"]
                    if item["path"].endswith(".cpp")
                ]
                if not is_system:
                    sources.append(
                        fixtures / "source-historical-scalar-parameterized.cpp"
                    )
                run(
                    name + "-build-cpp",
                    [
                        args.cxx,
                        "-std=c++20",
                        "-pthread",
                        "-I" + str(repo / "include"),
                        "-I" + str(generated),
                        *sources,
                        runtime,
                        "-o",
                        binary,
                    ],
                )
            elif is_system:
                run(
                    name + "-build-rtl",
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
            else:
                run(
                    name + "-build-rtl",
                    [
                        args.verilator,
                        "--binary",
                        "--timing",
                        "-Wno-fatal",
                        "--top-module",
                        "tb",
                        "-CFLAGS",
                        "-std=c++20",
                        "--Mdir",
                        output / "verilated",
                        "-o",
                        binary,
                        *[
                            generated / item["path"]
                            for item in manifest["files"]
                            if item["path"].endswith((".v", ".sv"))
                        ],
                        fixtures / "source-historical-scalar-parameterized.sv",
                    ],
                )
            for workers in (1, 2) if target == "cpp" else (1,):
                command = (
                    [binary, "--cycles", "8", "--workers", str(workers)]
                    if is_system and target == "cpp"
                    else (
                        [binary, "+cycles=8"]
                        if is_system
                        else [binary, str(workers)] if target == "cpp" else [binary]
                    )
                )
                stdout, _ = run(f"{name}-{target}-{workers}", command)
                if is_system:
                    observations.append(system_events(stdout, entry, values))
                else:
                    assert [
                        line
                        for line in stdout.splitlines()
                        if line.startswith("KNOWN ")
                    ] == [f"KNOWN {value}" for value in VALUES]
                    planes = [
                        line
                        for line in stdout.splitlines()
                        if line.startswith("PLANES ")
                    ]
                    assert planes == (
                        [
                            "PLANES 0 0 0",
                            "PLANES 0 0 131071",
                            "PLANES 65536 65537 43690",
                        ]
                        if target == "cpp"
                        else []
                    )
        assert root_entries[0] == root_entries[1]
        if is_system:
            assert observations[0] == observations[1] == observations[2]
        cases.append(
            {
                "entry": entry,
                "kind": "system" if is_system else "module",
                "output": str(output),
                "cycles": 8,
                "native_workers": [1, 2],
                "rtl": "pass",
                "formal_and_return_width": width,
                "complete_observations_equal": is_system,
                "native_host_four_state": not is_system,
            }
        )
        assert input_hashes == {str(path): digest(path) for path in inputs}
        (work / (name + "-evidence.json")).write_text(
            json.dumps(
                {
                    "role": "independent bounded scalar identity tests",
                    "model": "gpt-6.1-sol",
                    "effort": "high",
                    "case": cases[-1],
                    "input_sha256": input_hashes,
                    "artifact_sha256": {
                        str(path): digest(path)
                        for path in output.rglob("*")
                        if path.is_file()
                    },
                    "scope": "This individual case receipt does not establish the remaining cases or full owning gate acceptance.",
                },
                indent=2,
            )
            + "\n"
        )
    protected_paths = [
        path
        for directory in (*units, *[Path(case["output"]) for case in cases])
        for path in directory.rglob("*")
        if path.is_file()
    ]
    published = {str(path): digest(path) for path in protected_paths}
    negative_cases = []
    bench_path = source / "bench.py"
    original_bench = bench_path.read_text()
    for label, replacement, diagnostic in (
        (
            "wrong-input-width",
            original_bench.replace("incoming: bits[17]", "incoming: bits[18]"),
            "mathematical boundary requires known Integer source and destination kinds",
        ),
        (
            "nonempty-system-constructor",
            original_bench.replace(
                "dut = scalar_parameterized_types()",
                "dut = scalar_parameterized_types(width=17)",
            ),
            "structural module declaration requires an empty constructor; bind authored inputs in a rule",
        ),
    ):
        assert replacement != original_bench
        bench_path.write_text(replacement)
        try:
            _, stderr = cli(
                "reject-" + label,
                "compile",
                "-c",
                bench_path,
                "--source-root",
                source,
                "--package-prefix",
                PACKAGE,
                "-I",
                units[0],
                "-o",
                units[1],
                "--replace",
                failure=True,
            )
            assert diagnostic in stderr, stderr
            assert published == {path: digest(Path(path)) for path in published}
            negative_cases.append(
                {"case": label, "diagnostic": stderr, "publication_preserved": True}
            )
        finally:
            bench_path.write_text(original_bench)
    provider_path = source / "scalar_parameterized_types.py"
    original_provider = provider_path.read_text()
    provider_path.write_text(original_provider.replace("width: int = 17", "width: int"))
    required_unit = work / "required-provider-unit"
    try:
        cli(
            "required-provider-compile",
            "compile",
            "-c",
            provider_path,
            "--source-root",
            source,
            "--package-prefix",
            PACKAGE,
            "-o",
            required_unit,
        )
        _, stderr = cli(
            "reject-missing-required-default",
            "compile",
            "-c",
            bench_path,
            "--source-root",
            source,
            "--package-prefix",
            PACKAGE,
            "-I",
            required_unit,
            "-o",
            units[1],
            "--replace",
            failure=True,
        )
        assert "missing required instance parameter" in stderr, stderr
        assert published == {path: digest(Path(path)) for path in published}
        negative_cases.append(
            {
                "case": "missing-required-default",
                "diagnostic": stderr,
                "publication_preserved": True,
            }
        )
    finally:
        provider_path.write_text(original_provider)
    nested_keep = original_provider.replace(
        "from pycircuit import bits, module", "from pycircuit import bits, module, rule"
    ).replace(
        '    return {"result": value}',
        "    @rule\n    def keep(value: bits[width]) -> bits[width]:\n"
        '        return value\n\n    result = keep(value)\n    return {"result": result}',
    )
    provider_path.write_text(nested_keep)
    try:
        _, stderr = cli(
            "reject-nested-keep-expression",
            "compile",
            "-c",
            provider_path,
            "--source-root",
            source,
            "--package-prefix",
            PACKAGE,
            "-o",
            units[0],
            "--replace",
            failure=True,
        )
        assert "unsupported hardware expression 'Call'" in stderr, stderr
        assert published == {path: digest(Path(path)) for path in published}
        negative_cases.append(
            {
                "case": "nested-keep-expression",
                "diagnostic": stderr,
                "publication_preserved": True,
            }
        )
    finally:
        provider_path.write_text(original_provider)
    bindings = work / "bindings.json"
    bindings.write_text('[{"name":"width","value":17}]\n')
    _, stderr = cli(
        "reject-nonempty-parameters",
        "link",
        *units,
        "--top",
        MODULE,
        "--parameters",
        bindings,
        "-o",
        work / "module/system.ac",
        "--replace",
        failure=True,
    )
    assert "static parameter bindings are not implemented yet" in stderr
    assert "--parameters must be an empty array or omitted" in stderr
    assert published == {path: digest(Path(path)) for path in published}
    assert input_hashes == {str(path): digest(path) for path in inputs}
    (work / "evidence.json").write_text(
        json.dumps(
            {
                "role": "independent bounded scalar identity tests",
                "model": "gpt-6.1-sol",
                "effort": "high",
                "historical_assets": historical_assets,
                "identity_helper_inlining": "syntax-proven identity",
                "cases": cases,
                "nonempty_parameter_binding_rejected": stderr,
                "negative_cases": negative_cases,
                "publication_preserved": True,
                "input_sha256": input_hashes,
                "artifact_sha256": {
                    str(path): digest(path)
                    for path in work.rglob("*")
                    if path.is_file()
                },
                "remaining_scope": "Only the keyword-default17 identity fixture is restored. Changing fixed-width17 inputs through imported/same-unit children and a distinct default32 owner exercise existing default binding. Retired param[int] identity, nested keep expression calls, nonempty constructors/CLI parameter bindings, general dependent-type specialization and source/RTL four-state constructors are not established. Historical test assets establish width17 generation/type checks, not runtime identity stimuli.",
            },
            indent=2,
        )
        + "\n"
    )
    print(
        "PASS: exact17 identity vectors and native bitplanes, generated module and eight-cycle system; workers1/2 and RTL"
    )  # noqa: T201 - standalone gate receipt


if __name__ == "__main__":
    main()
