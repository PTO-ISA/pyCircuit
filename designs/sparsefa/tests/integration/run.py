"""Rebuild and run the independent sparse lifecycle System from this checkout."""

import argparse
import hashlib
import json
import os
import subprocess
from datetime import datetime, timezone
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--compile-only", action="store_true")
options = parser.parse_args()

ROOT = Path(__file__).resolve().parents[4]
INSTALL = ROOT / ".pycircuit_out/toolchain/install"
RUN_ID = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
OUT = ROOT / ".pycircuit_out" / ("fa2-integration-" + RUN_ID)
LOG = ROOT / "docs/gates/logs/fa2-p0-completion"
ENV = dict(
    os.environ,
    LD_LIBRARY_PATH=f"/home/xiekunpeng/micromamba/envs/acir/lib:{INSTALL}/lib",
    PYTHONPATH=str(INSTALL / "share/pycircuit/python"),
)
DRIVER = INSTALL / "bin/pycircuit"
OUT.mkdir(parents=True, exist_ok=True)
LOG.mkdir(parents=True, exist_ok=True)

RUN_LOG = LOG / ("integration-" + RUN_ID)
RUN_LOG.mkdir()


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


sources = [
    ROOT / f"designs/sparsefa/src/fa2_{name}.py"
    for name in ("interfaces", "numeric", "engine", "scheduler")
]
sources.extend(
    Path(__file__).parent / (name + ".py")
    for name in ("bench", "vectors", "faults", "run")
)
report = {
    "run_id": RUN_ID,
    "status": "RUNNING",
    "output_directory": str(OUT.relative_to(ROOT)),
    "head": subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
    ).strip(),
    "source_sha256": {str(path.relative_to(ROOT)): digest(path) for path in sources},
    "input_definition": "Stimuli and independent expected values are literal/source-defined in bench.py, vectors.py, faults.py; their hashes identify exact inputs.",
    "toolchain_sha256": {
        name: digest(INSTALL / "bin" / name)
        for name in (
            "pycircuit",
            "pycircuit-source-unit",
            "pycircuit-link",
            "pycircuit-emit",
        )
    },
    "rtl_simulation": "NOT RUN: explicitly excluded by user",
    "requested_cycles": {"bench": 2000, "vectors": 5000, "faults": 3000},
    "commands": [],
    "systems": {},
}


def candidate_unchanged():
    report["sources_unchanged"] = all(
        digest(ROOT / name) == value for name, value in report["source_sha256"].items()
    )
    report["toolchain_unchanged"] = all(
        digest(INSTALL / "bin" / name) == value
        for name, value in report["toolchain_sha256"].items()
    )
    return report["sources_unchanged"] and report["toolchain_unchanged"]


def publish():
    content = json.dumps(report, indent=2) + "\n"
    (RUN_LOG / "manifest.json").write_text(content)
    (LOG / "integration-latest.json").write_text(content)


def require(condition, message):
    if not condition:
        report["status"] = "FAIL"
        report["failure"] = message
        candidate_unchanged()
        publish()
        raise SystemExit(message)


publish()


def run(name, args):
    args = [str(v) for v in args]
    log = RUN_LOG / (name + ".log")
    with log.open("w") as stream:
        result = subprocess.run(args, env=ENV, stdout=stream, stderr=subprocess.STDOUT)
    record = {
        "run_id": RUN_ID,
        "stage": name,
        "command": args,
        "exit": result.returncode,
        "log": str(log.relative_to(ROOT)),
        "log_sha256": digest(log),
    }
    report["commands"].append(record)
    with (LOG / "integration-commands.jsonl").open("a") as stream:
        stream.write(json.dumps(record) + "\n")
    if result.returncode:
        report["status"] = "FAIL"
    publish()
    if result.returncode:
        raise SystemExit(f"{name}: exit{result.returncode}; see {RUN_LOG}")
    return log


units = []
imports = []
for source in ("interfaces", "numeric", "engine", "scheduler"):
    unit = OUT / (source + "-unit")
    run(
        source + "-compile",
        [
            DRIVER,
            "compile",
            "-c",
            ROOT / f"designs/sparsefa/src/fa2_{source}.py",
            "--source-root",
            ROOT / "designs/sparsefa/src",
            "--package-prefix",
            "fa2",
            *imports,
            "-o",
            unit,
            "--replace",
        ],
    )
    units.append(unit)
    imports.extend(["-I", unit])
scenarios = [
    ("bench", "SparseLifecycleSystem", 2000),
    ("vectors", "VectorSystem", 5000),
    ("faults", "FaultSystem", 3000),
]
for name, _top, _cycles in scenarios:
    unit = OUT / (name + "-unit")
    run(
        name + "-compile",
        [
            DRIVER,
            "compile",
            "-c",
            Path(__file__).with_name(name + ".py"),
            "--source-root",
            Path(__file__).parent,
            "--package-prefix",
            "integration",
            *imports,
            "-o",
            unit,
            "--replace",
        ],
    )
if options.compile_only:
    unchanged = candidate_unchanged()
    report["status"] = "COMPILE_ONLY" if unchanged else "CANDIDATE_CHANGED"
    report["not_run"] = "Link, emission and C++ execution deferred pending compiler GO"
    publish()
    print(
        f"All providers and three independent Systems compile; manifest: {RUN_LOG}"
    )  # noqa: T201
    raise SystemExit(0 if unchanged else 1)

for name, top, cycles in scenarios:
    unit = OUT / (name + "-unit")
    final = OUT / (name + ".ac")
    run(
        name + "-link",
        [
            DRIVER,
            "link",
            *units,
            unit,
            "--top",
            "integration." + name + "." + top,
            "-o",
            final,
            "--replace",
        ],
    )
    report["systems"][name] = {
        "top": "integration." + name + "." + top,
        "final_sha256": digest(final),
        "runs": [],
    }
    publish()
    for target in ("cpp", "verilog"):
        run(
            name + "-" + target + "-emit",
            [
                DRIVER,
                "emit",
                final,
                "--target",
                target,
                "-o",
                OUT / (name + "-" + target),
                "--replace",
            ],
        )
        report["systems"][name][target + "_manifest_sha256"] = digest(
            OUT / (name + "-" + target) / "generated.json"
        )
        publish()
    compiler = "/home/xiekunpeng/micromamba/envs/acir/bin/x86_64-conda-linux-gnu-"
    run(
        name + "-configure",
        [
            "cmake",
            "-S",
            OUT / (name + "-cpp"),
            "-B",
            OUT / (name + "-build"),
            f"-DCMAKE_PREFIX_PATH={INSTALL}",
            "-DCMAKE_CXX_COMPILER=" + compiler + "g++",
            "-DCMAKE_C_COMPILER=" + compiler + "gcc",
        ],
    )
    run(name + "-build", ["cmake", "--build", OUT / (name + "-build"), "-j2"])
    for workers in (1, 2):
        runner = OUT / (name + "-build/bin/pycircuit_sim")
        log = run(
            name + f"-workers{workers}",
            [runner, "--cycles", cycles, "--workers", workers],
        )
        results = [
            json.loads(line)
            for line in log.read_text().splitlines()
            if line.startswith("{")
        ]
        terminal = [record for record in results if record.get("kind") == "result"]
        require(
            len(terminal) == 1
            and terminal[0]["status"] == "TERMINATED"
            and terminal[0]["error"] is None,
            f"{name} workers{workers}: missing or unsuccessful terminal result",
        )
        require(
            str(terminal[0]["epoch_time"]) == str(cycles * 2),
            f"{name} workers{workers}: incomplete generated run",
        )
        expected_event = {
            "bench": "sparse_lifecycle_pass",
            "vectors": "vector_pass",
            "faults": "fault_pass",
        }[name]
        require(
            any(
                record.get("kind") == "log"
                and record.get("spec", {}).get("event") == expected_event
                for record in results
            ),
            f"{name} workers{workers}: missing bounded-progress oracle event",
        )
        report["systems"][name]["runs"].append(
            {
                "workers": workers,
                "cycles": cycles,
                "runner_sha256": digest(runner),
                "epoch_time": terminal[0]["epoch_time"],
                "status": terminal[0]["status"],
                "exit": 0,
            }
        )
        publish()
require(
    candidate_unchanged(),
    "sources/toolchain changed during execution; rerun exact candidate",
)
report["status"] = "PASS"
publish()
print(
    "Sparse lifecycle, vector and fault generated C++ workers1/2 PASS; Verilog emitted."
)  # noqa: T201 -- command-line validation summary.
