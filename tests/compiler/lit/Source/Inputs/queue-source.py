"""Independent Q4/Q6 Python queue source/publication/native/RTL gate."""

import argparse
import atexit
import hashlib
import importlib.util
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
    "iverilog",
    "vvp",
    "scratch",
):
    parser.add_argument("--" + name, required=True)
parser.add_argument(
    "--oracle-checks",
    action="store_true",
    help="run the separated independent checkers on this flow's artifacts (nightly)",
)
parser.add_argument("--fault-checks-only", action="store_true")
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
designs = fixtures / "queue-source"
oracle_dir = repo / "tests/compiler/oracles/queue_source"
evidence = Path(args.scratch).resolve()
evidence.mkdir(parents=True, exist_ok=True)
build = Path(tempfile.mkdtemp(prefix="queue-source-", dir=evidence))
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


candidate_paths = [
    Path(args.source_compiler),
    Path(args.linker),
    Path(args.emitter),
    Path(args.source_compiler).resolve().parent / "pycircuit-opt",
    Path(__file__).resolve(),
    fixtures / "queue-source.cpp",
    fixtures / "queue-source.sv",
    fixtures / "queue-source-vectors.py",
    *sorted(designs.glob("*.py")),
    *sorted(oracle_dir.glob("*.py")),
    *sorted((repo / "python/pycircuit").rglob("*.py")),
    *sorted((repo / "include/gfsim").rglob("*.h")),
    *sorted((repo / "include/verilog").glob("*.v")),
]
candidate_paths.extend(
    path
    for path in (
        Path(args.source_compiler).resolve().parent.parent / "lib/libpyc6_runtime.a",
        Path(args.source_compiler).resolve().parent.parent
        / "simulator/gfsim/libpyc6_runtime.a",
    )
    if path.is_file()
)
candidate_before = {str(path): digest(path) for path in candidate_paths}


def record_candidate():
    after = {str(path): digest(path) for path in candidate_paths}
    (evidence / "candidate-stability.json").write_text(
        json.dumps(
            {
                "before": candidate_before,
                "after": after,
                "unchanged": candidate_before == after,
            },
            indent=2,
        )
        + "\n"
    )


atexit.register(record_candidate)


def run(command, code=0, diagnostic=None, *, timeout=240):
    command = list(map(str, command))
    result = subprocess.run(
        command, cwd=repo, env=env, text=True, capture_output=True, timeout=timeout
    )
    row = {
        "command": command,
        "exit_status": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }
    commands.append(row)
    (evidence / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == code, row
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), row
    if diagnostic:
        assert diagnostic.lower() in result.stderr.lower(), row
    return result


def cli(*arguments, code=0, diagnostic=None):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], code, diagnostic)


def snapshot(path):
    if path.is_dir():
        return {
            item.relative_to(path).as_posix(): item.read_bytes()
            for item in path.rglob("*")
            if item.is_file()
        }
    return path.read_bytes()


def publication_control(path):
    return path.parent / ("." + path.name + ".pycircuit-publication")


def protected_publications(paths):
    payloads = []
    controls = []
    seen_payloads = set()
    seen_controls = set()
    for raw in paths:
        path = Path(raw)
        key = str(path.resolve())
        if key not in seen_payloads:
            assert path.exists(), path
            seen_payloads.add(key)
            payloads.append(path)
        control = publication_control(path)
        control_key = str(control.resolve())
        if control_key not in seen_controls:
            assert control.exists(), (path, control)
            seen_controls.add(control_key)
            controls.append(control)
    return payloads, controls, payloads + controls


def snapshot_digests(path):
    return {
        name: hashlib.sha256(data).hexdigest() for name, data in snapshot(path).items()
    }


def compile_source(
    path, root, output, imports=(), replace=False, code=0, diagnostic=None
):
    command = [
        "compile",
        "-c",
        path,
        "--source-root",
        root,
        "--package-prefix",
        "q4_queue",
        "-o",
        output,
    ]
    for interface in imports:
        command.extend(("-I", interface))
    if replace:
        command.append("--replace")
    return cli(*command, code=code, diagnostic=diagnostic)


def unit_payload(unit, kind):
    receipt = json.loads((unit / "unit.json").read_text())
    return unit / receipt["files"][kind]


def queue_owners(unit):
    text = unit_payload(unit, "body").read_text()
    owners = []
    for line in text.splitlines():
        if '"ac.queue"(' not in line:
            continue
        match = re.search(r"definition = @([^,}]+)", line)
        assert match, line
        owners.append(match.group(1))
    return owners


vector_spec = importlib.util.spec_from_file_location(
    "queue_source_vectors", fixtures / "queue-source-vectors.py"
)
vectors = importlib.util.module_from_spec(vector_spec)
vector_spec.loader.exec_module(vectors)


def fault_checks():
    """Use genuine system finals and inspect emitted committed leaf state."""
    source_root = build / "fault-input"
    source_root.mkdir()
    providers = []
    for name in (
        "pyc_route_merge_pipeline",
        "pyc_credit_pipeline",
        "pyc_reorder_pipeline",
    ):
        source = source_root / (name + ".py")
        shutil.copyfile(designs / source.name, source)
        provider = build / (name + "-fault-unit")
        compile_source(source, source_root, provider)
        providers.append(provider)
    source = source_root / "fault_systems.py"
    shutil.copyfile(designs / source.name, source)
    unit = build / "fault-unit"
    compile_source(source, source_root, unit, providers)
    source_stages = {
        compiled.name: {
            kind: digest(unit_payload(compiled, kind)) for kind in ("body", "interface")
        }
        for compiled in (*providers, unit)
    }
    input_digests = {
        str(path.relative_to(repo)): digest(path)
        for path in (
            Path(__file__).resolve(),
            fixtures / "queue-source-vectors.py",
            designs / "fault_systems.py",
            designs / "pyc_credit_pipeline.py",
            designs / "pyc_route_merge_pipeline.py",
            designs / "pyc_reorder_pipeline.py",
            fixtures / "queue-source-faults.cpp",
            fixtures / "queue-source-faults.sv",
            oracle_dir / "models.py",
            oracle_dir / "credit_independent_check.py",
            repo / "include/verilog/dffe.v",
            repo / "include/verilog/fifo.v",
            repo / "include/gfsim/collection.h",
        )
    }
    runtime_root = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        path
        for path in (
            runtime_root / "simulator/gfsim/libpyc6_runtime.a",
            runtime_root / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    )
    cases = (
        ("RouteFault", 1, "route_selector_out_of_range"),
        ("RouteBlockedFault", 8, "route_selector_out_of_range"),
        ("CreditFault", 1, "credit_nonpositive_cost"),
        ("CreditDeferredFault", 18, "credit_nonpositive_cost"),
        ("ReorderStaleFault", 5, "reorder_stale_key"),
        ("ReorderDuplicateFault", 2, "reorder_duplicate_key"),
        ("ReorderFullDefersFault", 32, ""),
        ("AtomicSiblingFault", 0, "queue_atomic_rejection"),
    )
    contracts = vectors.failure_contracts()
    receipts = []
    for name, edge, message in cases:
        contract = contracts[name]
        assert contract["first_failure"] == (edge if message else None)
        output = build / name
        output.mkdir()
        final = output / "design.ac"
        cli(
            "link",
            *providers,
            unit,
            "--top",
            f"q4_queue.fault_systems.{name}",
            "-o",
            final,
        )
        for target in ("cpp", "verilog"):
            cli("emit", final, "--target", target, "-o", output / target)
        cpp = output / "cpp"
        # Test-owned instrumentation grants read access only on a copy. It does
        # not change the emitted calculation, source check, or storage kernel.
        probe = output / "cpp-probe"
        shutil.copytree(cpp, probe)
        visibility = {}

        def grant_visibility(
            original, destination, *, visibility=visibility, probe=probe
        ):
            before = original.read_text()
            after = before.replace("private:", "public:")
            for left, right in zip(
                before.splitlines(), after.splitlines(), strict=True
            ):
                assert left == right or (
                    left.strip() == "private:" and right.strip() == "public:"
                )
            destination.write_text(after)
            visibility[str(destination.relative_to(probe))] = {
                "original_sha256": digest(original),
                "instrumented_sha256": digest(destination),
            }

        for header in probe.rglob("*.hpp"):
            grant_visibility(cpp / header.relative_to(probe), header)
        # Read-only pending-state visibility is test instrumentation on a
        # disposable header copy; the collection/kernel calculations stay exact.
        collection = probe / "gfsim/collection.h"
        collection.parent.mkdir()
        grant_visibility(repo / "include/gfsim/collection.h", collection)
        header = "\n".join(
            path.read_text() for path in (probe / "sources").rglob("*.hpp")
        )
        families = dict(
            re.findall(r"class pyc_family_(\w+) final \{(.*?)\n\};", header, re.S)
        )
        owners = []

        def visit(family, path=(), families=families, owners=owners):
            body = families[family]
            for line in body.splitlines():
                child = re.search(
                    r"std::shared_ptr<[^;]*::pyc_family_(\w+)<pyc_count>> (\w+);",
                    line,
                )
                if child:
                    visit(child[1], path + (child[2],))
                state = re.search(
                    r"std::shared_ptr<gfsim::collection_storage<(.*?)>> (\w+);",
                    line,
                )
                if state:
                    owners.append((path, state[2], state[1]))

        visit(name)
        assert len(owners) >= 5, (name, owners)
        pipeline = next(
            member
            for family, member in re.findall(
                r"std::shared_ptr<[^;]*::pyc_family_(\w+)<pyc_count>> (\w+);",
                families[name],
            )
            if family.endswith("Pipeline")
        )
        sibling = next(
            member
            for family, member in re.findall(
                r"std::shared_ptr<[^;]*::pyc_family_(\w+)<pyc_count>> (\w+);",
                families[name],
            )
            if family == "Sibling"
        )
        sibling_queue = re.search(
            r"std::shared_ptr<gfsim::collection_storage<gfsim::fifo_kernel<[^;]+>> (\w+);",
            families["Sibling"],
        )[1]
        output_bits = (
            66 if name.startswith("Route") else 26 if name.startswith("Credit") else 131
        )
        expected = [
            format(row["expected"], f"0{output_bits}b")
            for row in contract["rows"][: edge * 2]
        ]
        positive_cpp = (
            "constexpr std::string_view expected[]={"
            + ",".join(json.dumps(word + "|") for word in expected or [""])
            + "};"
        )
        result_cpp = f"dut.root_->pyc_implementation->{pipeline}->result.element(0)"
        positive_rtl = "\n".join(
            f'{index}: if (dut.dut.{pipeline}.result !== {output_bits}\'b{word}) $fatal(1,"positive prefix row {index}");'
            for index, word in enumerate(expected)
        )
        cpp_snapshot = []
        rtl_snapshot = []
        for path, member, kernel in owners:
            expression = "dut.root_->pyc_implementation->" + "->".join((*path, member))
            cpp_snapshot.append(f"save(out, *({expression}));")
            rtl_path = "dut.dut." + ".".join(
                (
                    *path,
                    member.removesuffix("_state").replace(
                        "pyc_queue_", "pyc_instance_", 1
                    ),
                )
            )
            if "fifo_kernel" in kernel:
                params = re.search(
                    r", (\d+), gfsim::QueueReadyPolicy::\w+, (\d+)ULL", kernel
                )
                assert params, kernel
                depth, latency = map(int, params.groups())
                rtl_snapshot.extend(
                    rtl_path + "." + field
                    for field in ("rd", "wr", "count", "initialized")
                )
                rtl_snapshot.extend(
                    f"{rtl_path}.storage[{lane}]" for lane in range(depth)
                )
                timing = "latency_one" if latency == 1 else "latency_delayed"
                rtl_snapshot.append(f"{rtl_path}.{timing}.managed.clock_current")
                if latency > 1:
                    rtl_snapshot.extend(
                        f"{rtl_path}.{timing}.{field}"
                        for field in ("tick", "mature_ptr", "eligible_count")
                    )
                    rtl_snapshot.extend(
                        f"{rtl_path}.{timing}.deadline[{lane}]" for lane in range(depth)
                    )
            else:
                count = 16 if "Entry" in kernel else 1
                for lane in range(count):
                    leaf = rtl_path
                    if count > 1:
                        parent, leaf_name = rtl_path.rsplit(".", 1)
                        raw = leaf_name.removeprefix("pyc_instance_")
                        leaf = f"{parent}.pyc_instances_{raw}[{lane}].{leaf_name}"
                    rtl_snapshot.extend(
                        (leaf + ".q_current", leaf + ".managed.clock_current")
                    )
        driver = probe / "fault_probe.cpp"
        driver.write_text(
            (fixtures / "queue-source-faults.cpp")
            .read_text()
            .replace("@SNAPSHOT@", "\n".join(cpp_snapshot))
            .replace("@EDGE@", str(edge))
            .replace("@MESSAGE@", message)
            .replace("@EXPECTED@", positive_cpp)
            .replace("@RESULT@", result_cpp)
            .replace("@SIBLING@", "dut.root_->pyc_implementation->" + sibling)
            .replace("@SIBLING_QUEUE@", sibling_queue)
        )
        receipt = json.loads((probe / "generated.json").read_text())
        sources = [
            probe / row["path"]
            for row in receipt["files"]
            if row["path"].endswith(".cpp") and row["role"] == "source"
        ]
        native = output / "native-probe"
        run(
            [
                args.cxx,
                "-O0",
                "-std=c++20",
                "-pthread",
                "-I" + str(probe),
                "-I" + str(repo / "include"),
                driver,
                *sources,
                runtime,
                "-o",
                native,
            ]
        )
        native_trace = run([native]).stdout
        assert native_trace.splitlines() == [
            "QUEUE_FAULT_ATOMIC_OK workers=1",
            "QUEUE_FAULT_ATOMIC_OK workers=2",
        ], native_trace
        rtl = output / "verilog"
        bench = output / "fault_probe.sv"
        bench.write_text(
            (fixtures / "queue-source-faults.sv")
            .read_text()
            .replace(
                "@SNAPSHOT@",
                '$sformatf("'
                + "|".join(["%h"] * len(rtl_snapshot))
                + '", '
                + ",".join(rtl_snapshot)
                + ")",
            )
            .replace("@EDGE@", str(edge))
            .replace("@FAIL@", "1" if message else "0")
            .replace("@EXPECTED@", positive_rtl)
            .replace("@SIBLING@", "dut.dut." + sibling)
            .replace(
                "@SIBLING_QUEUE@",
                sibling_queue.removesuffix("_state").replace(
                    "pyc_queue_", "pyc_instance_", 1
                ),
            )
        )
        receipt = json.loads((rtl / "generated.json").read_text())
        sources = [
            rtl / row["path"] for row in receipt["files"] if row["role"] == "rtl"
        ]
        sources.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
        rtl_build = output / "verilated"
        run(
            [
                args.verilator,
                "--binary",
                "--timing",
                "--top-module",
                "fault_probe",
                "--Mdir",
                rtl_build,
                "-j",
                "2",
                "-Wno-fatal",
                "-CFLAGS",
                "-std=c++20",
                "-MAKEFLAGS",
                "CFG_CXXFLAGS_PCH_I=-include",
                repo / "include/verilog/dffe.v",
                repo / "include/verilog/fifo.v",
                *sources,
                bench,
            ]
        )
        rtl_trace = run([rtl_build / "Vfault_probe"]).stdout
        assert "QUEUE_FAULT_ATOMIC_OK" in rtl_trace, rtl_trace
        receipts.append(
            {
                "root": name,
                "first_effective_view": edge if message else None,
                "failure_clock": contract.get("failure_clock", 0) if message else None,
                "bounded_edges": edge,
                "message": message,
                "native_code": "source_check_failed" if message else None,
                "native_workers": [1, 2],
                "verilator": True,
                "observations": 0,
                "candidate_inputs": input_digests,
                "source_stages": source_stages,
                "internal_visibility_instrumentation": visibility,
                "compiler_sha256": digest(Path(args.source_compiler)),
                "emitter_sha256": digest(Path(args.emitter)),
                "native_probe_sha256": digest(native),
                "rtl_probe_sha256": digest(rtl_build / "Vfault_probe"),
                "final_sha256": digest(final),
                "cpp": snapshot_digests(cpp),
                "verilog": snapshot_digests(rtl),
            }
        )
        (evidence / "queue-faults.json").write_text(
            json.dumps(receipts, indent=2) + "\n"
        )


if args.fault_checks_only:
    fault_checks()
    raise SystemExit(0)


provider_root = build / "provider-input"
consumer_root = build / "consumer-input"
provider_root.mkdir()
consumer_root.mkdir()
units = build / "units"
units.mkdir()
provider_path = provider_root / "provider.py"
consumer_path = consumer_root / "consumer.py"
shutil.copyfile(designs / "provider.py", provider_path)
shutil.copyfile(designs / "consumer.py", consumer_path)
provider = units / "provider"
consumer = units / "consumer"
compile_source(provider_path, provider_root, provider)
provider_path.unlink()
assert not provider_path.exists() and list(provider_root.iterdir()) == []
compile_source(consumer_path, consumer_root, consumer, (provider,))
assert not provider_path.exists()

facade_root = build / "facade-input"
facade_consumer_root = build / "facade-consumer-input"
facade_root.mkdir()
facade_consumer_root.mkdir()
facade_path = facade_root / "facade.py"
facade_path.write_text(
    "from q4_queue.provider import LocalTwin as ExportedPair, Pair13\n"
)
facade = units / "facade"
compile_source(facade_path, facade_root, facade, (provider,))
facade_path.unlink()
facade_consumer_path = facade_consumer_root / "facade_consumer.py"
facade_consumer_path.write_text(
    "import pycircuit as ac\n"
    "from q4_queue.facade import ExportedPair, Pair13\n"
    "@ac.module\n"
    "def Parent(valid: ac.u1, data: ac.u13, take: ac.u1) -> Pair13:\n"
    "    return ExportedPair(valid, data, take)\n"
)
facade_consumer = units / "facade-consumer"
compile_source(
    facade_consumer_path, facade_consumer_root, facade_consumer, (provider, facade)
)
assert not provider_path.exists() and not facade_path.exists()

standalone = {}
for name in (
    "forward_local",
    "forward_mixed",
    "snapshots",
    "latency",
    "pyc_route_merge_pipeline",
    "pyc_credit_pipeline",
    "pyc_reorder_pipeline",
):
    root = build / (name + "-input")
    root.mkdir()
    path = root / (name + ".py")
    shutil.copyfile(designs / path.name, path)
    standalone[name] = units / name
    compile_source(path, root, standalone[name])

expected_owners = {
    "provider": {
        "q4_queue.provider.ScalarDefault",
        "q4_queue.provider.ScalarBypass",
        "q4_queue.provider.NestedDefault",
        "q4_queue.provider.NestedBypass",
        "q4_queue.provider.Wide65",
        "q4_queue.provider.Wide130",
        "q4_queue.provider.TablePayload",
    },
    "consumer": set(),
}
provider_owners = queue_owners(provider)
assert (
    len(provider_owners) == 7 and set(provider_owners) == expected_owners["provider"]
), provider_owners
assert queue_owners(consumer) == []
assert queue_owners(facade) == [] and queue_owners(facade_consumer) == []
for name in standalone:
    owners = queue_owners(standalone[name])
    if name == "latency":
        assert (
            set(owners)
            == {
                f"q4_queue.latency.{root}"
                for root in (
                    "LatencyTwo",
                    "LatencyLocal",
                    "LatencyBypass",
                    "NestedLatency",
                    "HugeLatency",
                    "DeadLatency",
                )
            }
            and len(owners) == 6
        ), owners
    elif name == "pyc_route_merge_pipeline":
        # Route + priority merge: six queues, all owned by the one root.
        assert (
            set(owners) == {f"q4_queue.{name}.RouteMergePipeline"} and len(owners) == 6
        ), owners
    elif name == "pyc_credit_pipeline":
        # Credit window: the two queues (`issued`, `completed`),
        # both owned by the one root; the credit slots are module state, not
        # queues.
        assert (
            set(owners) == {f"q4_queue.{name}.CreditPipeline"} and len(owners) == 2
        ), owners
    elif name == "pyc_reorder_pipeline":
        # Reorder: the source queue plus the reorder's own output
        # queue, both owned by the one root; the 16-entry key-addressed table is
        # module state (`ac.table`), not a queue.
        assert (
            set(owners) == {f"q4_queue.{name}.ReorderPipeline"} and len(owners) == 2
        ), owners
    else:
        assert owners == [f"q4_queue.{name}.Top", f"q4_queue.{name}.Top"], owners


runtime_root = Path(args.source_compiler).resolve().parent.parent
runtime = next(
    path
    for path in (
        runtime_root / "simulator/gfsim/libpyc6_runtime.a",
        runtime_root / "lib/libpyc6_runtime.a",
    )
    if path.is_file()
)
primitives = sorted((repo / "include/verilog").glob("*.v"))


positives = [
    ("provider", "LocalTwin", "scalar", (provider,)),
    ("consumer", "ScalarParent", "scalar", (provider, consumer)),
    ("facade_consumer", "Parent", "scalar", (provider, facade, facade_consumer)),
    ("provider", "LocalNested", "nested", (provider,)),
    ("consumer", "NestedParent", "nested", (provider, consumer)),
    ("forward_local", "Top", "forward_local", (standalone["forward_local"],)),
    ("forward_mixed", "Top", "forward_mixed", (standalone["forward_mixed"],)),
    ("snapshots", "Top", "snapshots", (standalone["snapshots"],)),
    (
        "pyc_route_merge_pipeline",
        "RouteMergePipeline",
        "route_merge",
        (standalone["pyc_route_merge_pipeline"],),
    ),
    # Same root, second oracle mode derived independently of `route_merge`
    # (own stimulus, own host reference model, own rival-policy controls).
    (
        "pyc_route_merge_pipeline",
        "RouteMergePipeline",
        "route_merge_independent",
        (standalone["pyc_route_merge_pipeline"],),
    ),
    # Two-slot credit window over two depth-4 local-occupancy queues, one
    # `@ac.rule`, one 58-bit state register. The complete historical oracle
    # uses the checked-module execution boundary.
    (
        "pyc_credit_pipeline",
        "CreditPipeline",
        "credit",
        (standalone["pyc_credit_pipeline"],),
    ),
    (
        "pyc_credit_pipeline",
        "CreditPipeline",
        "credit_independent",
        (standalone["pyc_credit_pipeline"],),
    ),
    # Monotone key release over `ac.table[16]` + one `@ac.rule` + the two
    # historical queues (depth 8 in, depth 4 out, both local_occupancy).  The
    # key width ruling lives in the source docstring; `capacity` counts occupied
    # entries, so `key >= capacity` is legal and is exercised by `S4`.
    (
        "pyc_reorder_pipeline",
        "ReorderPipeline",
        "reorder",
        (standalone["pyc_reorder_pipeline"],),
    ),
    # Same root, second oracle mode derived independently of `reorder`
    # (map-addressed store, explicit per-epoch stimulus, hand-computed absolute
    # epoch tables, masked x/z rows, thirteen rival controls).
    (
        "pyc_reorder_pipeline",
        "ReorderPipeline",
        "reorder_independent",
        (standalone["pyc_reorder_pipeline"],),
    ),
    # The four-state (x/z) variant of the same mode. `_raw` keeps
    # Verilator out of it, which is the only honest reading: Verilator folds x to
    # two states, and on Verilator 5.044 a `z` literal in the generated rows file
    # additionally miscompiles the testbench.
    (
        "pyc_reorder_pipeline",
        "ReorderPipeline",
        "reorder_independent_raw",
        (standalone["pyc_reorder_pipeline"],),
    ),
    ("provider", "Wide65", "wide65", (provider,)),
    ("provider", "Wide130", "wide130", (provider,)),
    ("provider", "TablePayload", "table", (provider,)),
    ("latency", "LatencyTwo", "latency_two", (standalone["latency"],)),
    ("latency", "LatencyLocal", "latency_local", (standalone["latency"],)),
    ("latency", "LatencyBypass", "latency_bypass", (standalone["latency"],)),
    ("latency", "NestedLatency", "latency_nested", (standalone["latency"],)),
    ("latency", "NestedLatency", "latency_nested_raw", (standalone["latency"],)),
    ("latency", "HugeLatency", "latency_huge", (standalone["latency"],)),
    ("latency", "DeadLatency", "latency_dead", (standalone["latency"],)),
]
executions = []
oracle_checks = []
protected_products = [provider, consumer, facade, facade_consumer, *standalone.values()]
for source_name, root_name, oracle_name, closure in positives:
    # A root may carry more than one oracle mode. The `_raw` suffix keeps the
    # four-state variant separate; a second *known*-frame mode needs its own tree
    # as well. Every pre-existing case keeps its exact directory name.
    if oracle_name.endswith("_raw"):
        mode_suffix = "-raw"
    elif oracle_name.endswith("_independent"):
        mode_suffix = "-independent"
    else:
        mode_suffix = ""
    output = build / (source_name + "-" + root_name + mode_suffix)
    output.mkdir()
    vector_dir = output / "vectors"
    vectors.materialize(oracle_name, vector_dir)
    final = output / "design.ac"
    cli("link", *closure, "--top", f"q4_queue.{source_name}.{root_name}", "-o", final)
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", output / target)
    cpp_receipt = json.loads((output / "cpp/generated.json").read_text())
    cpp = [
        output / "cpp" / row["path"]
        for row in cpp_receipt["files"]
        if row["path"].endswith(".cpp")
    ]
    assert cpp
    runner = output / "runner"
    defines = (
        ["-DQ4_MAPPING"]
        if oracle_name in ("wide65", "wide130", "table")
        or oracle_name.startswith("latency_")
        else []
    )
    if oracle_name == "latency_dead":
        defines.append("-DQ6_DEAD")
    if oracle_name.startswith(("route_merge", "credit", "reorder")):
        defines.append("-DQ4_CHECKS")
    run(
        [
            args.cxx,
            "-O2",
            "-std=c++20",
            "-pthread",
            *defines,
            "-I" + str(repo / "include"),
            "-I" + str(output / "cpp"),
            "-I" + str(vector_dir),
            fixtures / "queue-source.cpp",
            *cpp,
            runtime,
            "-o",
            runner,
        ]
    )
    expected = (vector_dir / "expected.stdout").read_text().splitlines()
    native = []
    for workers in (1, 2):
        trace = run(
            [runner, "--workers", workers, "--config", vector_dir / "config.json"]
        ).stdout
        observed = [
            line
            for line in trace.splitlines()
            if line.startswith(("WORK ", "FAILED ", "HOST_RESET "))
        ]
        assert observed == expected
        native.append(observed)
    assert native[0] == native[1]
    rtl_receipt = json.loads((output / "verilog/generated.json").read_text())
    rtl = [
        output / "verilog" / row["path"]
        for row in rtl_receipt["files"]
        if row["role"] == "rtl"
    ]
    rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
    icarus = output / "icarus"
    run(
        [
            args.iverilog,
            "-g2012",
            *defines,
            "-s",
            "tb",
            "-I" + str(vector_dir),
            "-o",
            icarus,
            *primitives,
            *rtl,
            fixtures / "queue-source.sv",
        ]
    )
    observed = [
        line
        for line in run([args.vvp, icarus]).stdout.splitlines()
        if line.startswith(("WORK ", "FAILED ", "HOST_RESET "))
    ]
    assert observed == expected
    if oracle_name == "latency_dead":
        dead_icarus = output / "dead-icarus"
        run(
            [
                args.iverilog,
                "-g2012",
                *defines,
                "-DQ6_DEAD_FAILURE",
                "-s",
                "tb",
                "-I" + str(vector_dir),
                "-o",
                dead_icarus,
                *primitives,
                *rtl,
                fixtures / "queue-source.sv",
            ]
        )
        rejected = run([args.vvp, dead_icarus], code=1)
        assert "DEAD queue maturity completed" in rejected.stdout
        assert "fifo: effective transfers must be known" in rejected.stdout
        assert "dead queue failed to age" not in rejected.stdout
    four_state = oracle_name.endswith("_raw")
    verilator_build = output / "verilator"
    if not four_state:
        run(
            [
                args.verilator,
                "--binary",
                "--timing",
                "--top-module",
                "tb",
                "--prefix",
                "Vqueue_source",
                "--Mdir",
                verilator_build,
                "-j",
                "2",
                "-Wno-fatal",
                "-CFLAGS",
                "-std=c++20",
                "-MAKEFLAGS",
                "CFG_CXXFLAGS_PCH_I=-include",
                *defines,
                "-I" + str(vector_dir),
                *primitives,
                *rtl,
                fixtures / "queue-source.sv",
            ]
        )
        observed = [
            line
            for line in run([verilator_build / "Vqueue_source"]).stdout.splitlines()
            if line.startswith(("WORK ", "FAILED ", "HOST_RESET "))
        ]
        assert observed == expected
    if args.oracle_checks and oracle_name in (
        "route_merge_independent",
        "reorder_independent",
        "credit_independent",
    ):
        checker = {
            "route_merge_independent": "route_merge_independent_check.py",
            "reorder_independent": "reorder_independent_check.py",
            "credit_independent": "credit_independent_check.py",
        }[oracle_name]
        command = [sys.executable, oracle_dir / checker]
        if oracle_name != "credit_independent":
            command += [
                "--design-ac",
                final,
                "--cpp-dir",
                output / "cpp",
                "--rtl-dir",
                output / "verilog",
                "--vectors",
                oracle_dir / "models.py",
            ]
        if oracle_name == "reorder_independent":
            command += [
                "--iverilog",
                args.iverilog,
                "--vvp",
                args.vvp,
                "--source",
                designs / "pyc_reorder_pipeline.py",
                "--primitives",
                repo / "include/verilog",
                "--scratch",
                output / "oracle-checks",
            ]
        # This checker contains several individually bounded compile/run probes.
        # Use the existing nightly owner ceiling for the aggregate invocation;
        # retain the short per-command limit for ordinary driver steps.
        checked = run(command, timeout=3600)
        (output / "oracle-checks.stdout").write_text(checked.stdout)
        oracle_checks.append(
            {
                "oracle": oracle_name,
                "checker": str(oracle_dir / checker),
                "exit_status": checked.returncode,
                "scope": (
                    "reference-model self-check"
                    if oracle_name == "credit_independent"
                    else "independent model and emitted-artifact checks"
                ),
                "log": str(output / "oracle-checks.stdout"),
            }
        )
    protected_products.extend((final, output / "cpp", output / "verilog"))
    serialized = json.loads((vector_dir / "oracle.json").read_text())
    executions.append(
        {
            "root": f"q4_queue.{source_name}.{root_name}",
            "oracle": oracle_name,
            "rows": len(serialized["rows"]),
            "samples": sum(not row["execution_failure"] for row in serialized["rows"]),
            "rejected_rows": sum(
                row["execution_failure"] for row in serialized["rows"]
            ),
            "host_reset_segments": serialized["execution_segments"],
            "native_workers": [1, 2],
            "icarus": True,
            "verilator": not four_state,
            "four_state_native_and_icarus": four_state,
            "final_sha256": digest(final),
            "runner_sha256": digest(runner),
            "oracle_record_sha256": digest(vector_dir / "oracle.json"),
            "native_driver_sha256": digest(fixtures / "queue-source.cpp"),
            "rtl_driver_sha256": digest(fixtures / "queue-source.sv"),
            "serialization_sha256": digest(fixtures / "queue-source-vectors.py"),
            "generated": {
                "cpp": snapshot_digests(output / "cpp"),
                "verilog": snapshot_digests(output / "verilog"),
            },
        }
    )
    (evidence / "executions.json").write_text(json.dumps(executions, indent=2) + "\n")


# Both observed and dead bypass/bypass cycles reject replacement of a genuine
# source unit. The control has the same basename and owner as the bad source.
cycle_rejections = []
for name in ("bypass_cycle", "dead_bypass_cycle"):
    root = build / (name + "-input")
    root.mkdir()
    source = root / (name + ".py")
    source.write_text((designs / "forward_local.py").read_text())
    unit = units / (name + "-control")
    compile_source(source, root, unit)
    control = publication_control(unit)
    assert control.exists(), control
    before = {str(path): snapshot(path) for path in (unit, control)}
    source.write_text((designs / (name + ".py")).read_text())
    rejected = compile_source(source, root, unit, replace=True, code=1)
    assert "cycle" in rejected.stderr.lower(), rejected.stderr
    assert {str(path): snapshot(path) for path in (unit, control)} == before
    cycle_rejections.append({"case": name, "protected_control": str(control)})


def design(
    body,
    inputs="valid: ac.u1, data: ac.u13, take: ac.u1",
    imports="import pycircuit as ac\n",
):
    return (
        imports + "\n@ac.struct\nclass Result:\n"
        "    ready: ac.u1\n    available: ac.u1\n    data: ac.u13\n\n"
        f"@ac.module\ndef Top({inputs}) -> Result:\n" + body
    )


good_body = (
    "    ready, available, value = ac.queue[ac.u13](valid, data, take)\n"
    "    return Result(ready=ready, available=available, data=value)\n"
)


invalid = {
    "expression-site": design(
        "    ready = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=valid, available=valid, data=data)\n"
    ),
    "return-site": design("    return ac.queue[ac.u13](valid, data, take)\n"),
    "nested-call": design(
        "    ready, available, value = Result(ac.queue[ac.u13](valid, data, take))\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "conditional-placement": design(
        "    if valid:\n"
        "        ready, available, value = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "rule-placement": (
        "import pycircuit as ac\n@ac.struct\nclass Result:\n"
        "    ready: ac.u1\n    available: ac.u1\n    data: ac.u13\n"
        "@ac.rule\ndef evaluate(valid, data, take) -> Result:\n"
        "    ready, available, value = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
        "@ac.module\ndef Top(valid: ac.u1, data: ac.u13, take: ac.u1) -> Result:\n"
        "    return evaluate(valid, data, take)\n"
    ),
    "chained-target": design(
        "    ready, available, value = other = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "list-target": design(
        "    [ready, available, value] = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "starred-target": design(
        "    ready, *rest = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=valid, data=data)\n"
    ),
    "nested-target": design(
        "    ready, (available, value) = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "field-target": design(
        "    holder = Result()\n"
        "    holder.ready, available, value = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=holder.ready, available=available, data=value)\n"
    ),
    "target-arity-two": design(
        "    ready, value = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=valid, data=value)\n"
    ),
    "target-arity-four": design(
        "    ready, available, value, extra = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "duplicate-target": design(
        "    ready, ready, value = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=ready, data=value)\n"
    ),
    "rebind-ready": design(
        good_body.replace("    return", "    ready = valid\n    return")
    ),
    "rebind-available": design(
        good_body.replace("    return", "    available = valid\n    return")
    ),
    "rebind-value": design(
        good_body.replace("    return", "    value = data\n    return")
    ),
    "annotated-rebind": design(
        good_body.replace("    return", "    ready: ac.u1 = valid\n    return")
    ),
    "augmented-rebind": design(
        good_body.replace("    return", "    value += 1\n    return")
    ),
    "field-mutation-root": design(
        good_body.replace("    return", "    value.part = data\n    return")
    ),
    "index-mutation-root": design(
        good_body.replace("    return", "    value[0] = valid\n    return")
    ),
    "missing-valid": design(
        "    ready, available, value = ac.queue[ac.u13](in_data=data, out_ready=take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "missing-data": design(
        "    ready, available, value = ac.queue[ac.u13](in_valid=valid, out_ready=take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "missing-ready": design(
        "    ready, available, value = ac.queue[ac.u13](valid, data)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "extra-positional": design(
        "    ready, available, value = ac.queue[ac.u13](valid, data, take, 2)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "unknown-keyword": design(
        "    ready, available, value = ac.queue[ac.u13](valid, data, take, width=2)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "duplicate-argument": design(
        "    ready, available, value = ac.queue[ac.u13](valid, data, take, in_valid=valid)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "star-arguments": design(
        "    ready, available, value = ac.queue[ac.u13](*(valid, data, take))\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "star-keywords": design(
        "    ready, available, value = ac.queue[ac.u13](valid, data, **opts)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "depth-zero": design(
        good_body.replace("(valid, data, take)", "(valid, data, take, depth=0)")
    ),
    "depth-negative": design(
        good_body.replace("(valid, data, take)", "(valid, data, take, depth=(0 - 1))")
    ),
    "depth-boolean": design(
        good_body.replace("(valid, data, take)", "(valid, data, take, depth=True)")
    ),
    "latency-zero": design(
        good_body.replace("(valid, data, take)", "(valid, data, take, latency=0)")
    ),
    "latency-over-u64": design(
        good_body.replace(
            "(valid, data, take)", "(valid, data, take, latency=18446744073709551616)"
        )
    ),
    "latency-negative": design(
        good_body.replace("(valid, data, take)", "(valid, data, take, latency=(0 - 1))")
    ),
    "latency-boolean": design(
        good_body.replace("(valid, data, take)", "(valid, data, take, latency=True)")
    ),
    "unknown-policy": design(
        good_body.replace(
            "(valid, data, take)", '(valid, data, take, ready_policy="other")'
        )
    ),
    "wide-valid": design(good_body, "valid: ac.u2, data: ac.u13, take: ac.u1"),
    "wide-ready": design(good_body, "valid: ac.u1, data: ac.u13, take: ac.u2"),
    "integer-valid": design(
        good_body,
        "valid: Annotated[int, range(1 << 1)], data: ac.u13, take: ac.u1",
        "from typing import Annotated\nimport pycircuit as ac\n",
    ),
    "integer-ready": design(
        good_body,
        "valid: ac.u1, data: ac.u13, take: Annotated[int, range(1 << 1)]",
        "from typing import Annotated\nimport pycircuit as ac\n",
    ),
    "payload-type": design(good_body, "valid: ac.u1, data: ac.u14, take: ac.u1"),
    "queue-shadow": design(
        "    queue = data\n"
        "    ready, available, value = queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "namespace-shadow": design(
        "    ac = data\n"
        "    ready, available, value = ac.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
    "foreign-lookalike": design(
        "    ready, available, value = foreign.queue[ac.u13](valid, data, take)\n"
        "    return Result(ready=ready, available=available, data=value)\n"
    ),
}


def local_boundary(actual, formal):
    return (
        "from typing import Annotated\nimport pycircuit as ac\n"
        "@ac.struct\nclass One:\n    value: ac.u1\n"
        f"@ac.module\ndef Child(value: {formal}) -> One:\n"
        "    return One(value=value)\n"
        f"@ac.module\ndef Top(value: {actual}) -> One:\n"
        "    return Child(value)\n"
    )


invalid.update(
    {
        "local-fixed-to-boolean": local_boundary("ac.u1", "bool"),
        "local-fixed-to-integer": local_boundary(
            "ac.u1", "Annotated[int, range(1 << 1)]"
        ),
        "local-integer-to-boolean": local_boundary(
            "Annotated[int, range(1 << 1)]", "bool"
        ),
        "nominal-payload-mismatch": (
            "import pycircuit as ac\n"
            "@ac.struct\nclass Left:\n    value: ac.u13\n"
            "@ac.struct\nclass Right:\n    value: ac.u13\n"
            "@ac.struct\nclass Result:\n"
            "    ready: ac.u1\n    available: ac.u1\n    data: Left\n"
            "@ac.module\n"
            "def Top(valid: ac.u1, data: Right, take: ac.u1) -> Result:\n"
            "    ready, available, value = ac.queue[Left](valid, data, take)\n"
            "    return Result(ready=ready, available=available, data=value)\n"
        ),
    }
)
assert len(invalid) >= 40


negative_root = build / "negative-input"
negative_root.mkdir()
negative_path = negative_root / "negative.py"
negative_path.write_text(design(good_body))
negative_unit = units / "negative"
compile_source(negative_path, negative_root, negative_unit)
negative_final = build / "negative.ac"
cli("link", negative_unit, "--top", "q4_queue.negative.Top", "-o", negative_final)
for target in ("cpp", "verilog"):
    cli(
        "emit", negative_final, "--target", target, "-o", build / ("negative-" + target)
    )
protected_products.extend(
    (negative_unit, negative_final, build / "negative-cpp", build / "negative-verilog")
)


def boundary_source(annotation, callee, result="One"):
    return (
        "from typing import Annotated\nimport pycircuit as ac\n"
        "from q4_queue.provider import BooleanEcho, IntegerEcho, FixedEcho, Fixed2Echo, One, Two\n"
        "@ac.module\n"
        f"def Top(value: {annotation}) -> {result}:\n"
        f"    return {callee}(value)\n"
    )


boundary_root = build / "boundary-input"
boundary_root.mkdir()
boundary_path = boundary_root / "boundary.py"
boundary_path.write_text(boundary_source("ac.u1", "FixedEcho"))
boundary_unit = units / "boundary"
compile_source(boundary_path, boundary_root, boundary_unit, (provider,))
matching_path = boundary_root / "matching.py"
matching_path.write_text(
    "from typing import Annotated\nimport pycircuit as ac\n"
    "from q4_queue.provider import BooleanEcho, IntegerEcho, FixedEcho, Fixed2Echo, One\n"
    "@ac.module\ndef BooleanTop(value: bool) -> One:\n"
    "    return BooleanEcho(value)\n"
    "@ac.module\ndef IntegerTop(value: Annotated[int, range(1 << 1)]) -> One:\n"
    "    return IntegerEcho(value)\n"
    "@ac.module\ndef FixedTop(value: ac.u1) -> One:\n"
    "    return FixedEcho(value)\n"
    "@ac.module\ndef BooleanToFixed(value: bool) -> One:\n"
    "    converted = FixedEcho(value)\n"
    "    return BooleanEcho(value)\n"
    "@ac.module\ndef IntegerToFixed(value: Annotated[int, range(1 << 1)]) -> One:\n"
    "    converted = FixedEcho(value)\n"
    "    return IntegerEcho(value)\n"
    "@ac.module\ndef FixedWiden(value: ac.u1) -> One:\n"
    "    widened = Fixed2Echo(value)\n"
    "    return FixedEcho(value)\n"
)
matching_unit = units / "matching"
compile_source(matching_path, boundary_root, matching_unit, (provider,))
for root_name in (
    "BooleanTop",
    "IntegerTop",
    "FixedTop",
    "BooleanToFixed",
    "IntegerToFixed",
    "FixedWiden",
):
    cli(
        "link",
        provider,
        matching_unit,
        "--top",
        f"q4_queue.matching.{root_name}",
        "-o",
        build / ("matching-" + root_name + ".ac"),
    )
boundary_final = build / "boundary.ac"
cli(
    "link",
    provider,
    boundary_unit,
    "--top",
    "q4_queue.boundary.Top",
    "-o",
    boundary_final,
)
for target in ("cpp", "verilog"):
    cli(
        "emit", boundary_final, "--target", target, "-o", build / ("boundary-" + target)
    )
legacy_final = build / "legacy-structural.ac"
cli(
    "link",
    provider,
    "--top",
    "q4_queue.provider.LegacyEcho",
    "-o",
    legacy_final,
)
for target in ("cpp", "verilog"):
    cli(
        "emit",
        legacy_final,
        "--target",
        target,
        "-o",
        build / ("legacy-structural-" + target),
    )
for kind in ("body", "interface"):
    lines = [
        line
        for line in unit_payload(provider, kind).read_text().splitlines()
        if 'sym_name = "q4_queue.provider.LegacyEcho"' in line
    ]
    assert len(lines) == 1, (kind, lines)
    assert all(
        attribute not in lines[0]
        for attribute in (
            "ac.return_form",
            "ac.parameters",
            "ac.result_constraints",
            "ac.domain_inputs",
        )
    ), lines[0]
protected_products.extend(
    (
        boundary_unit,
        matching_unit,
        boundary_final,
        *(
            build / ("matching-" + root + ".ac")
            for root in (
                "BooleanTop",
                "IntegerTop",
                "FixedTop",
                "BooleanToFixed",
                "IntegerToFixed",
                "FixedWiden",
            )
        ),
        build / "boundary-cpp",
        build / "boundary-verilog",
        legacy_final,
        build / "legacy-structural-cpp",
        build / "legacy-structural-verilog",
    )
)
protected_payloads, protected_controls, protected_products = protected_publications(
    protected_products
)
protected = {str(path): snapshot(path) for path in protected_products}
rejections = []


# A forged provider header and matching consumer snapshots cannot override the
# unchanged provider body. Source-unit receipts are copied intact; only the
# equal-width fixed-u1 authority is relabeled as a one-bit Integer.
forged_units = build / "forged-units"
forged_units.mkdir()


def copy_managed_unit(source, name):
    destination = forged_units / name
    shutil.copytree(source, destination)
    source_control = source.parent / ("." + source.name + ".pycircuit-publication")
    destination_control = forged_units / ("." + name + ".pycircuit-publication")
    shutil.copytree(source_control, destination_control)
    (destination_control / "owner.json").write_text(
        json.dumps(
            {"destination": name, "kind": "pycircuit-publication-control"},
            separators=(",", ":"),
        )
    )
    return destination


forged_provider = copy_managed_unit(provider, "forged-provider")
forged_consumer = copy_managed_unit(consumer, "forged-consumer")
needle = 'source_kind = "fixed_bits"'
replacement = (
    'source_kind = "integer", domain = '
    '#ac.source_domain<{kind = "integer", lower = #ac.math_int<0>, '
    "upper = #ac.math_int<2>}>"
)
provider_header = unit_payload(forged_provider, "interface")


def forge_local_twin(path):
    lines = path.read_text().splitlines(keepends=True)
    matches = [
        index
        for index, line in enumerate(lines)
        if 'sym_name = "q4_queue.provider.LocalTwin"' in line and needle in line
    ]
    assert len(matches) == 1, (path, matches)
    lines[matches[0]] = lines[matches[0]].replace(needle, replacement, 1)
    path.write_text("".join(lines))


forge_local_twin(provider_header)
snapshot_mutations = 0
for kind in ("body", "interface"):
    path = unit_payload(forged_consumer, kind)
    text = path.read_text()
    if 'sym_name = "q4_queue.provider.LocalTwin"' in text and needle in text:
        forge_local_twin(path)
        snapshot_mutations += 1
assert snapshot_mutations, "consumer must retain the provider declaration snapshot"
forged_absent = build / "forged-absent.ac"
forged = cli(
    "link",
    forged_provider,
    forged_consumer,
    "--top",
    "q4_queue.consumer.ScalarParent",
    "-o",
    forged_absent,
    code=1,
)
assert (
    "published interface" in forged.stderr.lower()
    or "contract" in forged.stderr.lower()
)
assert not forged_absent.exists()
protected_final = build / "consumer-ScalarParent/design.ac"
forged = cli(
    "link",
    forged_provider,
    forged_consumer,
    "--top",
    "q4_queue.consumer.ScalarParent",
    "-o",
    protected_final,
    "--replace",
    code=1,
)
assert (
    "published interface" in forged.stderr.lower()
    or "contract" in forged.stderr.lower()
)
assert {str(path): snapshot(path) for path in protected_products} == protected
rejections.append(
    {
        "case": "forged-provider-header-and-consumer-snapshot",
        "fresh_exit_status": 1,
        "replacement_exit_status": 1,
    }
)

for name, annotation, callee, result in (
    ("fixed-to-boolean", "ac.u1", "BooleanEcho", "One"),
    ("fixed-to-integer", "ac.u1", "IntegerEcho", "One"),
    ("integer-to-boolean", "Annotated[int, range(1 << 1)]", "BooleanEcho", "One"),
    ("boolean-to-wide-fixed", "bool", "Fixed2Echo", "Two"),
    ("integer-narrowing", "Annotated[int, range(1 << 2)]", "FixedEcho", "One"),
):
    text = boundary_source(annotation, callee, result)
    boundary_path.write_text(text)
    archive = evidence / ("invalid-" + name + ".py")
    archive.write_text(text)
    absent = build / ("absent-" + name)
    fresh = compile_source(boundary_path, boundary_root, absent, (provider,), code=1)
    assert fresh.stderr.strip() and not absent.exists(), name
    replaced = compile_source(
        boundary_path, boundary_root, boundary_unit, (provider,), replace=True, code=1
    )
    assert replaced.stderr.strip(), name
    assert {str(path): snapshot(path) for path in protected_products} == protected, name
    rejections.append(
        {
            "case": name,
            "source_sha256": digest(archive),
            "fresh_exit_status": 1,
            "replacement_exit_status": 1,
        }
    )

# Published nominal fields admit explicit constructors without consulting the
# provider's Python AST or inventing authority for its unpublished defaults.
boundary_path.write_text(
    "import pycircuit as ac\n"
    "from q4_queue.provider import One\n"
    "@ac.module\n"
    "def Top(value: ac.u1) -> One:\n"
    "    return One(value=value)\n"
)
explicit_constructor = build / "imported-struct-constructor-explicit"
compile_source(boundary_path, boundary_root, explicit_constructor, (provider,))
cli(
    "link",
    provider,
    explicit_constructor,
    "--top",
    "q4_queue.boundary.Top",
    "-o",
    build / "imported-struct-constructor-explicit.ac",
)
for target in ("cpp", "verilog"):
    cli(
        "emit",
        build / "imported-struct-constructor-explicit.ac",
        "--target",
        target,
        "-o",
        build / ("imported-struct-constructor-explicit-" + target),
    )

for name, text in {
    "imported-struct-constructor-omitted": (
        "import pycircuit as ac\n"
        "from q4_queue.provider import One\n"
        "@ac.module\n"
        "def Top(value: ac.u1) -> One:\n"
        "    return One()\n"
    ),
    "direct-call-target-without-metadata": (
        "import pycircuit as ac\n"
        "from q4_queue.provider import LegacyEcho, One\n"
        "@ac.module\n"
        "def Top(value: ac.u1) -> One:\n"
        "    return LegacyEcho(value)\n"
    ),
}.items():
    boundary_path.write_text(text)
    archive = evidence / ("invalid-" + name + ".py")
    archive.write_text(text)
    absent = build / ("absent-" + name)
    fresh = compile_source(boundary_path, boundary_root, absent, (provider,), code=1)
    assert fresh.stderr.strip() and not absent.exists(), name
    if name == "direct-call-target-without-metadata":
        assert (
            "direct module call requires complete validated source-call metadata"
            in fresh.stderr
        ), fresh.stderr
    replaced = compile_source(
        boundary_path, boundary_root, boundary_unit, (provider,), replace=True, code=1
    )
    assert replaced.stderr.strip(), name
    if name == "direct-call-target-without-metadata":
        assert (
            "direct module call requires complete validated source-call metadata"
            in replaced.stderr
        ), replaced.stderr
    assert {str(path): snapshot(path) for path in protected_products} == protected, name
    rejections.append(
        {
            "case": name,
            "source_sha256": digest(archive),
            "fresh_exit_status": 1,
            "replacement_exit_status": 1,
        }
    )

name = "runtime-controlled-namespace-call"
text = (
    "import pycircuit as ac\nimport q4_queue.provider as provider\n"
    "@ac.module\n"
    "def Top(valid: ac.u1, data: ac.u13, take: ac.u1) -> provider.Pair13:\n"
    "    if valid:\n"
    "        child = provider.LocalTwin(valid, data, take)\n"
    "    else:\n"
    "        child = provider.LocalTwin(valid, data, take)\n"
    "    return child\n"
)
boundary_path.write_text(text)
archive = evidence / ("invalid-" + name + ".py")
archive.write_text(text)
absent = build / ("absent-" + name)
fresh = compile_source(boundary_path, boundary_root, absent, (provider,), code=1)
assert fresh.stderr.strip() and not absent.exists()
replaced = compile_source(
    boundary_path, boundary_root, boundary_unit, (provider,), replace=True, code=1
)
assert replaced.stderr.strip()
assert {str(path): snapshot(path) for path in protected_products} == protected
rejections.append(
    {
        "case": name,
        "source_sha256": digest(archive),
        "fresh_exit_status": 1,
        "replacement_exit_status": 1,
    }
)

for name, text in invalid.items():
    negative_path.write_text(text)
    archive = evidence / ("invalid-" + name + ".py")
    archive.write_text(text)
    absent = build / ("absent-" + name)
    fresh = compile_source(negative_path, negative_root, absent, code=1)
    assert fresh.stderr.strip() and not absent.exists(), name
    replaced = compile_source(
        negative_path, negative_root, negative_unit, replace=True, code=1
    )
    assert replaced.stderr.strip(), name
    assert {str(path): snapshot(path) for path in protected_products} == protected, name
    rejections.append(
        {
            "case": name,
            "source_sha256": digest(archive),
            "fresh_exit_status": 1,
            "replacement_exit_status": 1,
        }
    )


fixture_paths = [
    fixtures / "queue-source.py",
    fixtures / "queue-source.cpp",
    fixtures / "queue-source.sv",
    fixtures / "queue-source-vectors.py",
    fixtures.parent / "queue-source.test",
    *sorted(designs.glob("*.py")),
    *sorted(oracle_dir.glob("*.py")),
    oracle_dir / "MIGRATION-NOTES.md",
]
(evidence / "candidate.json").write_text(
    json.dumps(
        {
            "scope": "Q4/Q6 independent Python source/cross-source/native/RTL fixtures",
            "role": "executor-independent-tests",
            "model": "gpt-6.1-sol",
            "served_model": "unavailable",
            "effort": "high",
            "artifact_directory": str(build),
            "fixtures": {
                str(path.relative_to(repo)): digest(path) for path in fixture_paths
            },
            "tools": {
                str(Path(path).resolve()): digest(Path(path).resolve())
                for path in (
                    args.source_compiler,
                    args.linker,
                    args.emitter,
                    args.cxx,
                    args.verilator,
                    args.iverilog,
                    args.vvp,
                    runtime,
                )
            },
            "runtime_header_and_helper_sha256": {
                str(path.relative_to(repo)): digest(path)
                for path in [*sorted((repo / "include/gfsim").glob("*.h")), *primitives]
            },
            "provider_body_sha256": digest(unit_payload(provider, "body")),
            "provider_interface_sha256": digest(unit_payload(provider, "interface")),
            "executions": executions,
            "oracle_checks_requested": args.oracle_checks,
            "oracle_checks": oracle_checks,
            "wide_table_runtime_roots": ["Wide65", "Wide130", "TablePayload"],
            "metadata_free_structural_root": {
                "root": "q4_queue.provider.LegacyEcho",
                "final_sha256": digest(legacy_final),
                "cpp": snapshot_digests(build / "legacy-structural-cpp"),
                "verilog": snapshot_digests(build / "legacy-structural-verilog"),
                "source_call_attributes": "all_absent",
            },
            "cycle_rejections": cycle_rejections,
            "rejections": rejections,
            "protected_payload_count": len(protected_payloads),
            "protected_control_count": len(protected_controls),
            "protected_path_count": len(protected_products),
            "provider_source_absent_for_consumer": True,
        },
        indent=2,
    )
    + "\n"
)
print(  # noqa: T201 - public gate summary
    f"queue source gate passed: {len(executions)} native workers1/2 + Icarus runs; "
    f"{sum(row['verilator'] for row in executions)} known Verilator runs; "
    f"{len(rejections)} protected source rejections"
)
