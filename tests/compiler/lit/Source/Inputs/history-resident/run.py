"""Compile resident algorithms and check complete independent host histories."""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import drivers

FIELDS = {
    "rob": (
        ("index", 2),
        ("generation", 16),
        ("epoch", 16),
        ("value", 16),
        ("done", 1),
    ),
    "isq": (
        ("index", 2),
        ("age", 8),
        ("src0_tag", 6),
        ("src1_tag", 6),
        ("value", 16),
        ("valid", 1),
    ),
    "readiness": (("tag", 6), ("ready", 1)),
}
SNAPSHOT_WIDTH = {"rob": 974, "isq": 616}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def packed(kind, name, record):
    value = 0
    for field, width in FIELDS["readiness" if name.endswith("readiness") else kind]:
        value = (value << width) | int(record[field])
    return value


def unpack(kind, name, value):
    result = {}
    for field, width in reversed(
        FIELDS["readiness" if name.endswith("readiness") else kind]
    ):
        result[field] = value & ((1 << width) - 1)
        value >>= width
    assert value == 0
    return result


def queues(kind, snapshot):
    offset = SNAPSHOT_WIDTH[kind]
    result = {}
    for name, width in zip(drivers.QUEUES[kind], drivers.WIDTHS[kind], strict=True):
        offset -= width + 1
        value = (int(snapshot) >> offset) & ((1 << (width + 1)) - 1)
        available = value >> width
        result[name] = (available, unpack(kind, name, value & ((1 << width) - 1)))
    return result


def expected_flags(kind, row):
    result = {}
    for side in ("left", "right"):
        grants = row["grants"][side]
        mapping = (
            {
                "flush_take": "recover",
                "allocate_take": "allocate",
                "completion_take": "complete",
                "allocated_valid": "allocate",
                "retired_valid": "retire",
            }
            if kind == "rob"
            else {
                "request_take": "dispatch",
                "readiness_take": "update_ready",
                "issued_valid": "issue",
            }
        )
        result.update(
            {
                side + "_" + field: int(grant in grants)
                for field, grant in mapping.items()
            }
        )
    return result


def check_result(kind, row, bitstring):
    value = int(bitstring, 2)
    per = 107 if kind == "rob" else 42
    flags = expected_flags(kind, row)
    after = queues(kind, row["packed_after"])
    for side in ("left", "right"):
        off = per if side == "left" else 0
        positions = (
            {
                "flush_take": 106,
                "allocate_take": 105,
                "completion_take": 104,
                "allocated_valid": 103,
                "retired_valid": 51,
            }
            if kind == "rob"
            else {"request_take": 41, "readiness_take": 40, "issued_valid": 39}
        )
        for field, bit in positions.items():
            assert ((value >> (off + bit)) & 1) == flags[side + "_" + field], (
                kind,
                row["epoch"],
                field,
            )
        for port, low, width in (
            (("allocated", 52, 51), ("retired", 0, 51))
            if kind == "rob"
            else (("issued", 0, 39),)
        ):
            if flags[side + "_" + port + "_valid"]:
                observed = (value >> (off + low)) & ((1 << width) - 1)
                assert (
                    unpack(kind, side + "_" + port, observed)
                    == after[side + "_" + port][1]
                )


def stimulus(kind, rows):
    lines = [str(len(rows))]
    for row in rows:
        actions = row["actions"]
        offers, injects = actions.get("offer", {}), actions.get("inject", {})
        for name in drivers.QUEUES[kind]:
            lines.append(
                f"{int(name in offers)} {packed(kind, name, offers[name]) if name in offers else 0} {int(name in injects)} {packed(kind, name, injects[name]) if name in injects else 0} {int(name in actions.get('take', []))}"
            )
    return "\n".join(lines) + "\n"


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
    scratch = Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    (scratch / "receipt.json").unlink(missing_ok=True)
    oracle = repo / "tests/compiler/oracles/history_resident/vectors.json"
    vectors = json.loads(oracle.read_text())
    inputs = [
        *fixtures.glob("*.py"),
        *fixtures.glob("baseline/*"),
        *oracle.parent.glob("*.py"),
        oracle,
        repo / "include/verilog/dffe.v",
        repo / "cmake/verify_example.py",
    ]
    input_hashes = {str(p.relative_to(repo)): digest(p) for p in inputs}
    helpers = [Path(args.source_compiler), Path(args.linker), Path(args.emitter)]
    helper_hashes = {str(p): digest(p) for p in helpers}
    prefix = helpers[0].resolve().parent.parent
    runtime = next(
        p
        for p in (
            prefix / "simulator/gfsim/libpyc6_runtime.a",
            prefix / "lib/libpyc6_runtime.a",
        )
        if p.is_file()
    )
    runtime_hash = digest(runtime)
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python"),
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
        MAKEFLAGS="CFG_CXXFLAGS_PCH_I=-include",
    )
    commands, receipts, visibility = [], [], []

    def run(label, command, text_input=None):
        command = list(map(str, command))
        result = subprocess.run(
            command,
            input=text_input,
            cwd=repo,
            env=env,
            text=True,
            capture_output=True,
            timeout=300,
        )
        (scratch / (label + ".stdout")).write_text(result.stdout)
        (scratch / (label + ".stderr")).write_text(result.stderr)
        commands.append(
            {"label": label, "command": command, "exit_status": result.returncode}
        )
        (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert result.returncode == 0, (
            label,
            result.stdout[-2000:],
            result.stderr[-2000:],
        )
        return result.stdout

    def cli(label, *arguments):
        return run(label, [sys.executable, "-m", "pycircuit.cli", *arguments])

    units = []
    for name in (
        "reusable_circular_rob",
        "reusable_oldest_ready_isq",
        "resident_pairs",
        "resident_systems",
    ):
        unit = scratch / (name + "-unit")
        imports = (
            [arg for published in units[:2] for arg in ("-I", published)]
            if name.startswith("resident_")
            else []
        )
        cli(
            "compile-" + name,
            "compile",
            "-c",
            fixtures / (name + ".py"),
            "--source-root",
            fixtures,
            "--package-prefix",
            "history_resident",
            *imports,
            "-o",
            unit,
            "--replace",
        )
        units.append(unit)

    for kind, symbol in (
        ("rob", "reusable_circular_rob"),
        ("isq", "reusable_oldest_ready_isq"),
    ):
        for profile in ("module", "system"):
            top = (
                f"history_resident.resident_pairs.dual_{kind}"
                if profile == "module"
                else "history_resident.resident_systems." + symbol
            )
            final = scratch / f"{kind}-{profile}.ac"
            closure = [*units[:2], units[2 if profile == "module" else 3]]
            cli(
                f"{kind}-{profile}-link",
                "link",
                *closure,
                "--top",
                top,
                "-o",
                final,
                "--replace",
            )
            traces = {}
            for backend in ("cpp", "verilog"):
                label = f"{kind}-{profile}-{backend}"
                generated = scratch / label
                cli(
                    label + "-emit",
                    "emit",
                    final,
                    "--target",
                    backend,
                    "-o",
                    generated,
                    "--replace",
                )
                executable = scratch / (label + "-driver")
                if backend == "cpp":
                    sources = [
                        generated / item["path"]
                        for item in json.loads(
                            (generated / "generated.json").read_text()
                        )["files"]
                        if item["path"].endswith(".cpp")
                    ]
                    if profile == "module":
                        probe = scratch / (label + "-visibility")
                        shutil.copytree(generated, probe, dirs_exist_ok=True)
                        for header in probe.rglob("*.hpp"):
                            original = generated / header.relative_to(probe)
                            content = original.read_text()
                            amended = content.replace("private:", "public:")
                            header.write_text(amended)
                            assert header.read_text().replace(
                                "public:", "private:"
                            ) == content.replace("public:", "private:")
                            visibility.append(
                                {
                                    "original": str(original),
                                    "sha256": digest(original),
                                    "copy": str(header),
                                    "copy_sha256": digest(header),
                                    "change": "private: to public: only",
                                }
                            )
                        names = drivers.instances(
                            (
                                probe / "sources/history_resident/resident_pairs.hpp"
                            ).read_text(),
                            kind,
                        )
                        driver = scratch / (label + "-driver.cpp")
                        driver.write_text(drivers.cpp(kind, names))
                        sources = [
                            probe / p.relative_to(generated) for p in sources
                        ] + [driver]
                        includes = probe
                    else:
                        includes = generated
                    run(
                        label + "-build",
                        [
                            args.cxx,
                            "-O2",
                            "-std=c++20",
                            "-pthread",
                            "-I" + str(repo / "include"),
                            "-I" + str(includes),
                            *sources,
                            runtime,
                            "-o",
                            executable,
                        ],
                    )
                elif profile == "module":
                    native_header = (
                        scratch
                        / f"{kind}-module-cpp/sources/history_resident/resident_pairs.hpp"
                    )
                    names = drivers.instances(native_header.read_text(), kind)
                    driver = scratch / (label + "-driver.sv")
                    driver.write_text(drivers.verilog(kind, names))
                    run(
                        label + "-build",
                        [
                            args.verilator,
                            "--binary",
                            "--timing",
                            "-CFLAGS",
                            "-std=c++20",
                            "--top-module",
                            "tb",
                            "-j",
                            "2",
                            "-Wno-fatal",
                            "--Mdir",
                            scratch / (label + "-build"),
                            "-o",
                            executable,
                            generated / "design_top.sv",
                            *generated.rglob("*.v"),
                            repo / "include/verilog/dffe.v",
                            driver,
                        ],
                    )
                else:
                    run(
                        label + "-build",
                        [
                            sys.executable,
                            repo / "cmake/verify_example.py",
                            "--compile-system",
                            generated,
                            "--output",
                            executable,
                            "--include",
                            repo / "include",
                            "--verilator",
                            args.verilator,
                        ],
                    )
                groups = ("cases", "witnesses") if profile == "module" else ("systems",)
                for group in groups:
                    rows = vectors[group][kind]["rows"]
                    assert len(rows) <= 256
                    for workers in (1, 2) if backend == "cpp" else (1,):
                        case_label = f"{label}-{group}-workers{workers}"
                        if profile == "module":
                            input_text = stimulus(kind, rows)
                            input_file = scratch / f"{kind}-{group}.stimulus.txt"
                            input_file.write_text(input_text)
                            output = run(
                                case_label,
                                (
                                    [executable, workers]
                                    if backend == "cpp"
                                    else [executable, "+stimulus=" + str(input_file)]
                                ),
                                input_text if backend == "cpp" else None,
                            )
                            observed = [
                                line.split()
                                for line in output.splitlines()
                                if line and line[0].isdigit()
                            ]
                            assert len(observed) == len(rows), case_label
                            for actual, row in zip(observed, rows, strict=True):
                                assert int(actual[0]) == row["epoch"]
                                for index, phase in enumerate(
                                    ("before", "work", "after"), 1
                                ):
                                    assert len(actual[index]) == SNAPSHOT_WIDTH[kind]
                                    assert int(actual[index], 2) == int(
                                        row["packed_" + phase]
                                    ), (case_label, row["epoch"], row["label"], phase)
                                check_result(kind, row, actual[4])
                        else:
                            options = (
                                ["--cycles", len(rows), "--workers", workers]
                                if backend == "cpp"
                                else ["+cycles=" + str(len(rows))]
                            )
                            output = run(case_label, [executable, *options])
                            records = [
                                json.loads(line)
                                for line in output.splitlines()
                                if line.startswith("{")
                            ]
                            observations = {}
                            for record in records:
                                if record["kind"] == "log":
                                    observations.setdefault(
                                        int(record["evaluation_epoch"]), {}
                                    )[record["spec"]["event"]] = int(
                                        record["values"][0]["value"]
                                    )
                            assert list(observations) == list(
                                range(len(rows) * 2)
                            ), case_label
                            observed = list(observations.values())
                            for epoch, row in enumerate(rows):
                                expected = {"epoch": epoch, **expected_flags(kind, row)}
                                before = queues(kind, row["packed_before"])
                                after = queues(kind, row["packed_after"])
                                for name, (available, payload) in before.items():
                                    expected[name + "_available"] = available
                                    expected[name + "_ready"] = 1 - available
                                    expected.update(
                                        {
                                            name
                                            + "_head_"
                                            + field: value if available else 0
                                            for field, value in payload.items()
                                        }
                                    )
                                for side in ("left", "right"):
                                    for port in (
                                        ("allocated", "retired")
                                        if kind == "rob"
                                        else ("issued",)
                                    ):
                                        valid = expected[side + "_" + port + "_valid"]
                                        payload = after[side + "_" + port][1]
                                        expected.update(
                                            {
                                                side
                                                + "_"
                                                + port
                                                + "_publish_"
                                                + field: value if valid else 0
                                                for field, value in payload.items()
                                            }
                                        )
                                assert observed[epoch * 2] == expected, (
                                    case_label,
                                    epoch,
                                    row["label"],
                                    {
                                        key: (observed[epoch * 2].get(key), value)
                                        for key, value in expected.items()
                                        if observed[epoch * 2].get(key) != value
                                    },
                                )
                                assert observed[epoch * 2 + 1] == expected, (
                                    case_label,
                                    epoch,
                                    "old Q high",
                                )
                            assert (
                                records[-1]["status"] == "TERMINATED"
                                and int(records[-1]["epoch_time"]) == len(rows) * 2
                            )
                        key = profile + ":" + group
                        if key in traces:
                            assert traces[key] == observed, (
                                case_label,
                                "backend/worker trace mismatch",
                            )
                        traces[key] = observed
                        receipts.append(
                            {
                                "top": top,
                                "profile": profile,
                                "scenario": group,
                                "backend": backend,
                                "workers": workers,
                                "epochs": len(rows),
                                "finite_bound": 256,
                                "final_sha256": digest(final),
                                "log_sha256": digest(
                                    scratch / (case_label + ".stdout")
                                ),
                            }
                        )
                        print(case_label + " passed", flush=True)
    assert all(digest(repo / name) == value for name, value in input_hashes.items())
    assert all(digest(Path(name)) == value for name, value in helper_hashes.items())
    assert digest(runtime) == runtime_hash
    receipt = {
        "cases": receipts,
        "candidate_inputs": input_hashes,
        "native_helpers": helper_hashes,
        "runtime_archive": {"path": str(runtime), "sha256": runtime_hash},
        "read_only_visibility": visibility,
        "limits": [
            "Original exceptional committed-host readiness actions are exact module coverage, not closed-system equivalence.",
            "ISQ ordinary closed scheduling is a separate independently derived scenario.",
            "Three Bool fields and the Boolean readiness table use approved physical u1 carriers; general Bool nominal/Table authoring remains unsupported.",
            "Complete known-state native masks and RTL values are checked; this gate does not claim original X/Z matrices.",
            "Shared old-Q completion index source form avoids an unsupported aggregate projection identity proof; the generic proof path is not repaired.",
        ],
    }
    (scratch / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


if __name__ == "__main__":
    main()
