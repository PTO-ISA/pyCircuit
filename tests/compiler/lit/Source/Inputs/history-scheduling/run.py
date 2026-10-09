"""Verify finite source schedulers against independent baseline histories."""

import argparse
import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
from pathlib import Path


def load(path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def packed_token(token, persistent):
    widths = (8, 8, 2, 8, 16) if persistent else (4, 4, 1, 4, 16)
    packed = 0
    for value, width in zip(token, widths, strict=True):
        assert 0 <= value < 1 << width
        packed = (packed << width) | value
    return packed


def module_vectors(records, persistent):
    prefix = "schedule_" if persistent else "dependency_"
    selected = {
        name: record
        for name, record in records.items()
        if name.startswith(prefix)
        and name
        not in ("schedule_batch", "dependency_batch", "schedule_persistent_original")
    }
    rows, histories = [], []
    for name, record in selected.items():
        begin = len(rows)
        for edge, frame in enumerate(record["frames"]):
            for clock in (0, 1):
                expected = record["rows"][2 * edge + clock]
                error = record["events"][edge]["failure"]
                result = 0
                if expected is not None:
                    result = (expected["ready"] << 1) | expected["available"]
                    result = (result << (42 if persistent else 29)) | packed_token(
                        expected["head"], persistent
                    )
                rows.append(
                    {
                        "clock": clock,
                        "valid": frame["valid"],
                        "take": frame["take"],
                        "token": packed_token(frame["data"], persistent),
                        "expected": result,
                        "failure": expected is None,
                        "host_reset": frame["host_reset"] and clock == 0,
                        "message": error["historical_code"] if error else "",
                    }
                )
        histories.append({"name": name, "begin": begin, "end": len(rows)})
    return rows, histories


def write_vectors(folder, rows, histories, persistent):
    input_bits = 42 if persistent else 29
    text = f"#pragma once\n#include <cstdint>\nconstexpr unsigned input_bits={input_bits}, output_bits={input_bits + 2};\n"
    text += "struct Row { unsigned clock, valid, take; std::uint64_t token, expected; bool failure, host_reset; const char *message; };\nconstexpr Row rows[]={\n"
    for row in rows:
        text += (
            "{"
            + ",".join(
                str(int(row[key]))
                for key in (
                    "clock",
                    "valid",
                    "take",
                    "token",
                    "expected",
                    "failure",
                    "host_reset",
                )
            )
            + ","
            + json.dumps(row["message"])
            + "},\n"
        )
    text += "};\nstruct History {const char *name; unsigned begin,end;};\nconstexpr History histories[]={\n"
    text += "".join(
        "{" + json.dumps(h["name"]) + f',{h["begin"]},{h["end"]}' + "},\n"
        for h in histories
    )
    (folder / "scheduling_vectors.hpp").write_text(text + "};\n")
    (folder / "scheduling_widths.svh").write_text(
        f"`define INPUT_BITS {input_bits}\n`define OUTPUT_BITS {input_bits+2}\n"
    )
    # One dynamic loop consumes the same complete rows as the native host. Repeated
    # literal task calls make Verilator inline every state snapshot at every row.
    vector_bits = 3 + input_bits + input_bits + 2 + 2
    vectors = []
    for row in rows:
        value = (row["clock"] << 2) | (row["valid"] << 1) | row["take"]
        value = (value << input_bits) | row["token"]
        value = (value << (input_bits + 2)) | row["expected"]
        value = (value << 1) | row["failure"]
        value = (value << 1) | row["host_reset"]
        vectors.append(f"{vector_bits}'d{value}")
        # Independently decode the serialized RTL row and compare all host fields.
        decoded = {}
        for key, width in (
            ("host_reset", 1),
            ("failure", 1),
            ("expected", input_bits + 2),
            ("token", input_bits),
            ("take", 1),
            ("valid", 1),
            ("clock", 1),
        ):
            decoded[key] = value & ((1 << width) - 1)
            value >>= width
        assert value == 0 and all(decoded[key] == row[key] for key in decoded)
    declarations = (
        "typedef struct packed {\n"
        "  logic clock, push, pop;\n"
        "  logic [`INPUT_BITS-1:0] token;\n"
        "  logic [`OUTPUT_BITS-1:0] expected;\n"
        "  logic expected_failure, reset_host;\n"
        "} SchedulingRow;\n"
        f"localparam integer row_count={len(rows)}, history_count={len(histories)};\n"
        "localparam SchedulingRow vectors[0:row_count-1]='{\n"
        + ",\n".join(vectors)
        + "\n};\n"
    )
    for label in ("begin", "end"):
        declarations += (
            f"localparam integer history_{label}[0:history_count-1]='{{"
            + ",".join(str(h[label]) for h in histories)
            + "};\n"
        )
    declarations += (
        "localparam string history_name[0:history_count-1]='{"
        + ",".join(json.dumps(h["name"]) for h in histories)
        + "};\n"
    )
    (folder / "scheduling_rows.svh").write_text(declarations)


def snapshots(cpp, rtl):
    headers = "\n".join(path.read_text() for path in (cpp / "sources").rglob("*.hpp"))
    storage = sorted(
        set(
            re.findall(
                r"std::shared_ptr<gfsim::collection_storage<[^\n]+>> (\w+);", headers
            )
        )
    )
    assert len(storage) >= 4
    native = "\n  ".join(
        f"save(out, *dut.root_->pyc_implementation->{name});" for name in storage
    )
    text = "\n".join(path.read_text() for path in (rtl / "sources").rglob("*.v"))
    state = []
    for match in re.finditer(r"  dffe #[^\n]+ (\w+) \(", text):
        prior = text[: match.start()].splitlines()[-1]
        group = re.search(r"begin : (\w+)", prior)
        if group:
            count = int(re.search(r"\* \((\d+)\)", prior)[1])
            paths = [f"dut.dut.{group[1]}[{i}].{match[1]}" for i in range(count)]
        else:
            paths = ["dut.dut." + match[1]]
        for path in paths:
            state += [path + ".q_current", path + ".managed.clock_current"]
    for match in re.finditer(r"  fifo #[^\n]+ (\w+) \(", text):
        path = "dut.dut." + match[1]
        depth = int(re.search(r"DEPTH\((\d+)\)", match[0])[1])
        assert "AVAILABILITY_LATENCY(64'd1)" in match[0]
        state += [
            path + "." + field
            for field in (
                "rd",
                "wr",
                "count",
                "initialized",
                "latency_one.managed.clock_current",
            )
        ]
        state += [path + f".storage[{i}]" for i in range(depth)]
    assert len(state) >= 28
    return native, ",\n      ".join(state)


def system_observations(stdout, persistent, cycles):
    records = [json.loads(line) for line in stdout.splitlines() if line.startswith("{")]
    widths = {
        "epoch": 8,
        "ready": 1,
        "available": 1,
        "sequence": 8 if persistent else 4,
        "waits_for": 8 if persistent else 4,
        "resource": 2 if persistent else 1,
        "cost": 8 if persistent else 4,
        "value": 16,
    }
    observations = {}
    last_epoch = -1
    for record in records:
        if record.get("kind") == "log":
            assert record["instance"] == "root"
            assert re.fullmatch(r"0|[1-9][0-9]*", record["evaluation_epoch"])
            epoch = int(record["evaluation_epoch"])
            assert 0 <= epoch < 2 * cycles
            assert epoch >= last_epoch, ("interleaved evaluation epochs", epoch)
            last_epoch = epoch
            assert record["commit_epoch"] == str(epoch + 1)
            spec = record["spec"]
            event = spec["event"]
            assert event in widths and spec["level"] == "info"
            assert spec["items"] == [{"kind": "value", "ordinal": 0}]
            assert len(record["values"]) == 1
            scalar = record["values"][0]
            width = widths[event]
            if width == 1:
                assert scalar["kind"] == "bool" and type(scalar["value"]) is bool
            else:
                assert scalar["kind"] == "integer"
                assert isinstance(scalar["value"], str)
                assert re.fullmatch(r"0|[1-9][0-9]*", scalar["value"])
            value = int(scalar["value"])
            assert 0 <= value < 1 << width
            bucket = observations.setdefault(epoch, {})
            assert event not in bucket, ("duplicate event", epoch, event)
            bucket[event] = value
        else:
            assert record.get("kind") == "result"
    assert list(observations) == list(range(2 * cycles))
    assert all(set(bucket) == set(widths) for bucket in observations.values())
    assert sum(record["kind"] == "result" for record in records) == 1
    assert records[-1]["kind"] == "result" and records[-1]["error"] is None
    return list(observations.values()), records


def main():
    parser = argparse.ArgumentParser()
    for name in ("repo", "source-compiler", "linker", "emitter", "scratch"):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    repo = Path(args.repo).resolve()
    fixtures = Path(__file__).resolve().parent
    scratch = Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    prefix = scratch / "runtime"
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python"),
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
        MAKEFLAGS="CFG_CXXFLAGS_PCH_I=-include",
    )
    oracle_path = fixtures.parent / "history-scheduling-oracles.py"
    oracle = load(oracle_path)
    records = oracle.self_test()
    records.setdefault(
        "schedule_persistent_stream",
        oracle.run(
            [(0, 255, 0, 1, 10), (1, 255, 1, 8, 11), (2, 0, 1, 1, 12)],
            no_dependency=255,
            persistent=True,
            epochs=20,
        ),
    )
    paths = [
        *fixtures.rglob("*"),
        oracle_path,
        fixtures.parent.parent / "source-historical-scheduling.test",
        repo / "CMakeLists.txt",
        repo / "simulator/gfsim/CMakeLists.txt",
        *sorted((repo / "simulator/gfsim").glob("*.cpp")),
        *sorted((repo / "include/gfsim").rglob("*")),
        *sorted((repo / "include/verilog").glob("*.v")),
        repo / "cmake/pycircuitConfig.cmake.in",
        repo / "cmake/toolchain-metadata.json.in",
    ]
    hashes = {
        str(path.relative_to(repo)): digest(path) for path in paths if path.is_file()
    }
    helpers = {
        str(Path(tool).resolve()): digest(Path(tool))
        for tool in (args.source_compiler, args.linker, args.emitter)
    }
    (scratch / "bound-inputs.json").write_text(
        json.dumps({"candidate_inputs": hashes, "native_helpers": helpers}, indent=2)
        + "\n"
    )
    commands = []

    def run(label, command):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, capture_output=True, text=True, timeout=480
        )
        (scratch / (label + ".stdout")).write_text(result.stdout)
        (scratch / (label + ".stderr")).write_text(result.stderr)
        commands.append(
            {"label": label, "command": command, "exit_status": result.returncode}
        )
        (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert result.returncode == 0, (
            label,
            result.stdout[-1500:],
            result.stderr[-1500:],
        )
        return result

    def cli(label, *arguments):
        return run(label, [sys.executable, "-m", "pycircuit.cli", *arguments])

    # Lit supplies build-tree tools. Build and install the existing public Runtime
    # package in scratch so generated consumers also work without a prior install.
    runtime_build = scratch / "runtime-build"
    run(
        "runtime-configure",
        [
            "cmake",
            "-S",
            repo,
            "-B",
            runtime_build,
            "-G",
            "Ninja",
            "-DPYC_BUILD_COMPILER_DEV=OFF",
            "-DPYC_BUILD_TESTING=OFF",
            "-DPYC_INSTALL_PYTHON=OFF",
            "-DCMAKE_INSTALL_PREFIX=" + str(prefix),
        ],
    )
    run("runtime-build", ["cmake", "--build", runtime_build, "-j", "2"])
    run("runtime-install", ["cmake", "--install", runtime_build])
    runtime_package = prefix / "share/pycircuit/cmake"
    (scratch / "units").mkdir(exist_ok=True)
    units = []
    for name in (
        "pyc_dependency_pipeline",
        "persistent_schedule",
        "scheduling_systems",
    ):
        unit = scratch / "units" / name
        imports = (
            [item for prior in units for item in ("-I", prior)]
            if name == "scheduling_systems"
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
            "history_scheduling",
            *imports,
            "-o",
            unit,
            "--replace",
        )
        units.append(unit)
    receipts = []
    for persistent, unit, source, symbol in (
        (False, units[0], "pyc_dependency_pipeline", "DependencyPipeline"),
        (True, units[1], "persistent_schedule", "PersistentSchedule"),
    ):
        label = "schedule" if persistent else "dependency"
        folder = scratch / label
        folder.mkdir(exist_ok=True)
        cli(
            label + "-link",
            "link",
            unit,
            "--top",
            f"history_scheduling.{source}.{symbol}",
            "-o",
            folder / "final.ac",
            "--replace",
        )
        for backend in ("cpp", "verilog"):
            cli(
                label + "-emit-" + backend,
                "emit",
                folder / "final.ac",
                "--target",
                backend,
                "-o",
                folder / backend,
                "--replace",
            )
        rows, histories = module_vectors(records, persistent)
        write_vectors(folder, rows, histories, persistent)
        native, rtl = snapshots(folder / "cpp", folder / "verilog")
        (folder / "host.cpp").write_text(
            (fixtures / "host.cpp").read_text().replace("@STATE_SNAPSHOTS@", native)
        )
        (folder / "host.sv").write_text(
            (fixtures / "host.sv").read_text().replace("@STATE_SNAPSHOTS@", rtl)
        )
        cmake = folder / "consumer"
        cmake.mkdir(exist_ok=True)
        (cmake / "CMakeLists.txt").write_text(
            'cmake_minimum_required(VERSION 3.25)\nproject(SchedulingHistory LANGUAGES C CXX)\nadd_subdirectory("'
            + str(folder / "cpp")
            + '" modules)\nadd_executable(history_host "'
            + str(folder / "host.cpp")
            + '")\ntarget_compile_options(history_host PRIVATE -fno-access-control)\ntarget_include_directories(history_host PRIVATE "'
            + str(folder)
            + '")\ntarget_link_libraries(history_host PRIVATE pycircuit_modules)\nset_target_properties(history_host PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")\n'
        )
        run(
            label + "-configure",
            [
                "cmake",
                "-S",
                cmake,
                "-B",
                folder / "host-build",
                "-G",
                "Ninja",
                "-DCMAKE_PREFIX_PATH=" + str(prefix),
                "-Dpycircuit_DIR=" + str(runtime_package),
            ],
        )
        run(label + "-build", ["cmake", "--build", folder / "host-build", "-j", "2"])
        result = run(label + "-cpp-history", [folder / "host-build/bin/history_host"])
        for history in histories:
            for workers in (1, 2):
                assert (
                    f'SCHEDULING_HISTORY_OK {history["name"]} views={history["end"]-history["begin"]} workers={workers}'
                    in result.stdout
                )
        verilog_sources = [
            folder / "verilog/design_top.sv",
            *sorted((folder / "verilog/sources").rglob("*.v")),
        ]
        leaves = [
            repo / "include/verilog" / name
            for name in (
                "byte_mem.v",
                "dff.v",
                "dffe.v",
                "fifo.v",
                "sync_mem.v",
                "sync_mem_dp.v",
            )
        ]
        run(
            label + "-rtl-build",
            [
                "verilator",
                "--binary",
                "--timing",
                "-CFLAGS",
                "-std=c++20",
                "-Wno-fatal",
                "-j",
                "2",
                "--top-module",
                "scheduling_host",
                "--Mdir",
                folder / "rtl-build",
                "-o",
                folder / "rtl-host",
                "-I" + str(folder),
                *leaves,
                *verilog_sources,
                folder / "host.sv",
            ],
        )
        result = run(label + "-rtl-history", [folder / "rtl-host"])
        for history in histories:
            assert (
                f'SCHEDULING_HISTORY_OK {history["name"]} views={history["end"]-history["begin"]}'
                in result.stdout
            )
        receipts.append(
            {
                "module": symbol,
                "histories": histories,
                "views": len(rows),
                "workers": [1, 2],
                "cpp": "PASS",
                "verilator": "PASS",
                "final_sha256": digest(folder / "final.ac"),
            }
        )
    for symbol, case in (
        ("pyc_dependency_pipeline", "dependency_parallel"),
        ("schedule_v2", "schedule_persistent_stream"),
    ):
        folder = scratch / symbol
        folder.mkdir(exist_ok=True)
        cli(
            symbol + "-link",
            "link",
            *units,
            "--top",
            f"history_scheduling.scheduling_systems.{symbol}",
            "-o",
            folder / "final.ac",
            "--replace",
        )
        expected = []
        for row in records[case]["rows"]:
            assert row is not None
            key, waits_for, resource, cost, value = row["head"]
            expected.append(
                {
                    "epoch": row["epoch"],
                    "ready": row["ready"],
                    "available": row["available"],
                    "sequence": key,
                    "waits_for": waits_for,
                    "resource": resource,
                    "cost": cost,
                    "value": value,
                }
            )
        cycles = len(records[case]["frames"])
        traces = []
        for backend in ("cpp", "verilog"):
            generated, build = folder / backend, folder / (backend + "-build")
            cli(
                symbol + "-emit-" + backend,
                "emit",
                folder / "final.ac",
                "--target",
                backend,
                "-o",
                generated,
                "--replace",
            )
            run(
                symbol + "-configure-" + backend,
                [
                    "cmake",
                    "-S",
                    generated,
                    "-B",
                    build,
                    "-G",
                    "Ninja",
                    "-DCMAKE_PREFIX_PATH=" + str(prefix),
                    "-Dpycircuit_DIR=" + str(runtime_package),
                ],
            )
            run(
                symbol + "-build-" + backend,
                ["cmake", "--build", build, "--target", "pycircuit_sim", "-j", "2"],
            )
            for workers in (1, 2) if backend == "cpp" else (1,):
                options = (
                    ["--cycles", cycles, "--workers", workers]
                    if backend == "cpp"
                    else ["+cycles=" + str(cycles)]
                )
                result = run(
                    f"{symbol}-{backend}-{workers}",
                    [build / "bin/pycircuit_sim", *options],
                )
                observations, events = system_observations(
                    result.stdout, symbol == "schedule_v2", cycles
                )
                assert observations == expected, (
                    symbol,
                    backend,
                    observations,
                    expected,
                )
                assert (
                    events[-1]["status"] == "TERMINATED"
                    and int(events[-1]["epoch_time"]) == 2 * cycles
                )
                traces.append(observations)
        assert all(trace == traces[0] for trace in traces)
        receipts.append(
            {
                "system": symbol,
                "cycles": cycles,
                "views": len(expected),
                "cpp_workers": [1, 2],
                "cpp": "PASS",
                "verilator": "PASS",
                "final_sha256": digest(folder / "final.ac"),
            }
        )
    changed_inputs = [
        path for path, sha in hashes.items() if digest(repo / path) != sha
    ]
    changed_helpers = [
        path for path, sha in helpers.items() if digest(Path(path)) != sha
    ]
    assert not changed_inputs and not changed_helpers, (
        "bound inputs changed during gate",
        changed_inputs,
        changed_helpers,
    )
    receipt = {
        "cases": receipts,
        "candidate_inputs": hashes,
        "native_helpers": helpers,
        "independent_rivals": records["rivals"],
        "baseline": "8887e6dec7b4cc530a9967c860dc6a224d79a4ab",
        "limits": [
            "Three historical direct queue batch preloads remain independent model/original-source oracles; current single-push queues start empty.",
            "Custom-domain invalid predecessor is model-only; original schedule_v2 u8 predecessor is bounded by sentinel255.",
            "Module gate preserves all representable failed suffixes and explicit host reset; system roots use regular compiler-owned clock/reset execution.",
            "No four-state or nightly matrix run is claimed.",
        ],
    }
    (scratch / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print("SCHEDULING_COMMON_FLOW_OK", flush=True)


if __name__ == "__main__":
    main()
