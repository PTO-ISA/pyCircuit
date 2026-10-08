"""Publish example navigation and excerpts from verified generated artifacts."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATALOG = Path("examples/catalog.json")


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def catalog(repo: Path) -> dict:
    data = json.loads((repo / CATALOG).read_text())
    if data.get("schema") != "pycircuit-example-catalog":
        raise ValueError("unsupported example catalog schema")
    entries = data.get("examples", [])
    api_entries = data.get("api_coverage", [])
    names = [row.get("name") for row in (*entries, *api_entries)]
    if any(not name or Path(name).name != name for name in names):
        raise ValueError("example catalog contains an unsafe name")
    if len(set(names)) != len(names):
        raise ValueError("example catalog contains duplicate names")
    for row in entries:
        folder = Path(row.get("folder", ""))
        source = Path(row.get("source", ""))
        if folder != Path("examples") / row["name"]:
            raise ValueError(f"example folder does not match its name: {row['name']}")
        if source.name != row.get("source") or source.suffix != ".py":
            raise ValueError(f"example source is unsafe: {row['name']}")
        required = [repo / folder / "CMakeLists.txt", repo / folder / source]
        if row.get("system"):
            if not isinstance(row["system"], str) or not row["system"].isidentifier():
                raise ValueError(f"example system name is invalid: {row['name']}")
            system_source = Path(row.get("system_source", row["source"]))
            if (
                system_source.name != str(system_source)
                or system_source.suffix != ".py"
            ):
                raise ValueError(f"example system source is unsafe: {row['name']}")
            required.append(repo / folder / system_source)
            cycles = row.get("system_cycles")
            if cycles is not None and (type(cycles) is not int or cycles <= 0):
                raise ValueError(f"example system cycles are invalid: {row['name']}")
        else:
            required.append(repo / folder / "config.json")
        if not all(path.is_file() for path in required):
            raise ValueError(f"example coverage is incomplete: {row['name']}")
        generated = row.get("generated")
        if generated and (
            Path(generated).name != generated
            or not (repo / folder / generated).is_file()
        ):
            raise ValueError(f"example generated receipt is missing: {row['name']}")
    for row in api_entries:
        owner = Path(row.get("owner", ""))
        if (
            owner.is_absolute()
            or ".." in owner.parts
            or not owner.parts
            or owner.parts[0] != "tests"
        ):
            raise ValueError(f"API coverage owner is unsafe: {row['name']}")
        if not (repo / owner).is_file():
            raise ValueError(f"API coverage owner is missing: {row['name']}")
        root = row.get("system", row.get("root"))
        if root:
            if not isinstance(root, str) or not all(
                part.isidentifier() for part in root.split(".")
            ):
                raise ValueError(f"API system root is invalid: {row['name']}")
            source = Path(row.get("system_source", row.get("root_source", "")))
            if (
                source.is_absolute()
                or ".." in source.parts
                or not source.parts
                or source.parts[0] != "tests"
                or source.suffix != ".py"
                or not (repo / source).is_file()
            ):
                raise ValueError(
                    f"API system source is missing or unsafe: {row['name']}"
                )
    return data


def navigation(repo: Path) -> str:
    data = catalog(repo)
    rows = data["examples"]
    api_rows = data["api_coverage"]
    lines = [
        "# Examples navigation",
        "",
        "Examples use the single Python → MLIR → C++/Verilog → runner flow.",
        f"The current catalog contains **{len(rows)}** runnable examples and "
        f"**{len(api_rows)}** API-owned coverage cases.",
        "",
        "- [Writing and verification standard](STANDARD.md)",
        "- [System execution](../docs/architecture/system-execution.md)",
        "- [Machine-readable catalog](catalog.json)",
        "- [Current language](../docs/reference/language.md)",
        "",
        "## Runnable examples",
        "",
        "| Design | Source | System run | Native oracle | RTL oracle | Generated output |",
        "| --- | --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        folder = Path(row["folder"])
        name = row["name"]
        directory = repo / folder
        relative = folder.relative_to("examples").as_posix()
        title = (
            f"[{name}]({relative}/README.md)"
            if (directory / "README.md").exists()
            else name
        )
        source = f"[Python]({relative}/{row['source']})"
        native = (
            f"[driver]({relative}/driver.cpp)"
            if (directory / "driver.cpp").is_file()
            else "—"
        )
        rtl = (
            f"[testbench]({relative}/rtl_tb.sv)"
            if (directory / "rtl_tb.sv").is_file()
            else "—"
        )
        generated = (
            f"[artifacts]({relative}/GENERATED.md)"
            if row.get("generated") and (directory / "GENERATED.md").is_file()
            else "—"
        )
        system = "—"
        if row.get("system"):
            system_source = row.get("system_source", row["source"])
            system = f"[{row['system']}]({relative}/{system_source})"
            if not (directory / "driver.cpp").is_file():
                native = rtl = "compiler-generated"
        lines.append(
            f"| {title} | {source} | {system} | {native} | {rtl} | {generated} |"
        )
    lines += [
        "",
        "## API-owned coverage",
        "",
        "These cases are covered by their current compiler test owners.",
        "",
        "| Case | Test owner | Source root |",
        "| --- | --- | --- |",
    ]
    for row in api_rows:
        system = (
            f"[{row['system']}](../{row['system_source']})"
            if row.get("system")
            else f"[{row['root']}](../{row['root_source']})" if row.get("root") else "—"
        )
        lines.append(f"| {row['name']} | [test](../{row['owner']}) | {system} |")
    lines += [
        "",
        "## Build and run",
        "",
        "```sh",
        "cmake -S examples -B /absolute/build/examples -G Ninja \\",
        "  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install",
        "cmake --build /absolute/build/examples --parallel 4",
        "ctest --test-dir /absolute/build/examples --output-on-failure --no-tests=error",
        "```",
        "",
        "The aggregate build and this catalog contain the same runnable example set.",
        "",
    ]
    return "\n".join(lines)


def excerpt(path: Path, marker: str, length: int = 14) -> tuple[int, str]:
    lines = path.read_text().splitlines()
    start = next((index for index, line in enumerate(lines) if marker in line), 0)
    return start + 1, "\n".join(lines[start : start + length])


def generated(repo: Path, name: str, build: Path) -> tuple[str, dict]:
    if not name or name in {".", ".."} or Path(name).name != name:
        raise ValueError("example name must identify one flat example folder")
    source = repo / "examples" / name
    proof = json.loads((build / "verification.json").read_text())
    if proof.get("schema") != "pycircuit-example-verification-v1":
        raise ValueError("example requires a current successful verification receipt")
    execution = build / "execution.json"
    commands = json.loads(execution.read_text())
    if not commands or any(row["exit_status"] != 0 for row in commands):
        raise ValueError("example execution is missing or failed")
    if digest(execution) != proof["execution_sha256"]:
        raise ValueError("example execution changed after verification")
    # Use the verifier's exact input-membership policy, including primitives.
    # It is also installed standalone; importing it must not execute a runner.
    module_spec = importlib.util.spec_from_file_location(
        "pycircuit_example_verifier", ROOT / "cmake/verify_example.py"
    )
    verifier = importlib.util.module_from_spec(module_spec)
    module_spec.loader.exec_module(verifier)
    inputs = verifier.bound_inputs(
        source, build, Path(commands[0]["command"][0]), Path(proof["runtime_include"])
    )
    if inputs.keys() != proof["inputs"].keys():
        raise ValueError("stale verified example source/build input membership changed")
    for key, expected in proof["inputs"].items():
        if inputs[key] != expected:
            raise ValueError(f"stale verified example input: {key}")
    traces = []
    for filename in ("serial.stdout", "parallel.stdout", "rtl-run.stdout"):
        trace = [
            line
            for line in (build / filename).read_text().splitlines()
            if line.startswith("WORK ")
        ]
        traces.append(trace)
    if not traces[0] or any(trace != traces[0] for trace in traces[1:]):
        raise ValueError("verified Work traces are missing or disagree")
    if (
        len(traces[0]) != proof["work_samples"]
        or hashlib.sha256(("\n".join(traces[0]) + "\n").encode()).hexdigest()
        != proof["trace_sha256"]
    ):
        raise ValueError("Work trace changed after verification")
    cpp = json.loads((build / "cpp/generated.json").read_text())
    rtl = json.loads((build / "verilog/generated.json").read_text())
    owner = Path(cpp["entry_source"]["path"])
    if owner.is_absolute() or ".." in owner.parts:
        raise ValueError("unsafe entry source path")
    final = build / owner.with_suffix(".ac")
    header = next(
        build / "cpp" / row["path"]
        for row in cpp["files"]
        if row["role"] == "header" and Path(row["path"]).stem == owner.stem
    )
    verilog = next(
        build / "verilog" / row["path"]
        for row in rtl["files"]
        if row["role"] == "rtl"
        and Path(row["path"]).suffix == ".v"
        and Path(row["path"]).stem == owner.stem
    )
    lines = [
        f"# {name}: generated output",
        "",
        "These excerpts come from the public compiler's actual output after a successful",
        "native worker-1/worker-2 and RTL oracle run. They are reading samples, not a",
        "second implementation or standalone replacement for the complete generated files.",
        "",
        f"Verified WORK trace rows: **{proof['work_samples']}**. "
        f"Final artifact SHA-256: `{digest(final)}`.",
        "",
        "Trace rows may be sparse checkpoints or summaries; see the example README for the number of checked epochs.",
        "",
        "Source, runner and generated-file digests are recorded in [GENERATED.json](GENERATED.json).",
        "The recipe below regenerates complete artifacts outside the source tree.",
        "",
        "```sh",
        f"cmake -S examples/{name} -B /absolute/build/{name} -G Ninja \\",
        "  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install",
        f"cmake --build /absolute/build/{name} --parallel 4",
        f"ctest --test-dir /absolute/build/{name} --output-on-failure --no-tests=error",
        f"python3 tools/pycircuit/example_catalog.py generated --example {name} \\",
        f"  --build /absolute/build/{name}",
        "```",
        "",
    ]
    samples = []
    for title, path, marker, language in (
        ("Verified MLIR", final, '"ac.module"', "mlir"),
        ("Source-owned C++ Work", header, "void Work()", "cpp"),
        ("Source-owned Verilog", verilog, "module ", "systemverilog"),
    ):
        line, content = excerpt(path, marker)
        relative = path.relative_to(build).as_posix()
        samples.append({"path": relative, "line": line, "sha256": digest(path)})
        lines += [
            f"## {title}",
            "",
            f"`{relative}`, from line {line}:",
            "",
            f"```{language}",
            content,
            "```",
            "",
        ]
    published_proof = {
        key: value for key, value in proof.items() if key != "runtime_include"
    }
    metadata = {
        "schema": "pycircuit-generated-example-v1",
        "example": name,
        "entry": cpp["entry"],
        "verification": published_proof,
        "excerpts": samples,
    }
    return "\n".join(lines), metadata


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    sub.add_parser("navigation")
    create = sub.add_parser("generated")
    create.add_argument("--example", required=True)
    create.add_argument("--build", required=True, type=Path)
    args = parser.parse_args()
    if args.action == "navigation":
        (ROOT / "examples/README.md").write_text(navigation(ROOT))
    else:
        text, metadata = generated(ROOT, args.example, args.build.resolve())
        folder = ROOT / "examples" / args.example
        (folder / "GENERATED.md").write_text(text)
        (folder / "GENERATED.json").write_text(json.dumps(metadata, indent=2) + "\n")


if __name__ == "__main__":
    main()
