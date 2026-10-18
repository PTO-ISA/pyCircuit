"""Public-flow snapshots, source-kind rejection and independent transport planes."""

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import time
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
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
# Retain source units, final IR, receipts, generated files and simulator binaries.
build = Path(tempfile.mkdtemp(prefix="execution-", dir=scratch))
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def run(command, accepted=True, file_size_limit=None, expected_exit=None):
    command = list(map(str, command))
    started = time.monotonic()
    options = {}
    if file_size_limit is not None:
        import resource
        import signal

        def limit_child_file_size():
            signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
            resource.setrlimit(
                resource.RLIMIT_FSIZE, (file_size_limit, file_size_limit)
            )

        options["preexec_fn"] = limit_child_file_size
    try:
        result = subprocess.run(
            command,
            env=env,
            cwd=repo,
            capture_output=True,
            text=True,
            timeout=240,
            **options,
        )
    except subprocess.TimeoutExpired:
        commands.append(
            {"command": command, "exit_status": None, "timeout_seconds": 240}
        )
        (build / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        raise
    commands.append(
        {
            "command": command,
            "exit_status": result.returncode,
            "elapsed_seconds": time.monotonic() - started,
            "stdout": result.stdout,
            "stderr": result.stderr,
            "child_file_size_limit": file_size_limit,
        }
    )
    (build / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    if expected_exit is None:
        expected_exit = 0 if accepted else 1
    assert result.returncode == expected_exit, commands[-1]
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


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


widths = {"a": 5, "b": 7, "c": 3, "d": 11, "e": 7, "f": 3, "g": 5, "h": 11, "flag": 1}
rows = [
    {
        name: format((n * (2 * index + 3) + index * 7) % (1 << width), f"0{width}b")
        for index, (name, width) in enumerate(widths.items())
    }
    for n in range(24)
]
for n in range(4):
    rows.append(
        {
            name: (
                str(n % 2)
                if name == "flag"
                else "".join("01xz"[(bit + n + index) % 4] for bit in range(width))
            )
            for index, (name, width) in enumerate(widths.items())
        }
    )


def expected_fields(row):
    # Independent scalar transport specification, in declaration packing order.
    # No record assignment evaluator or DUT execution supplies these values.
    return [
        row[name]
        for name in (
            "a",
            "b",
            "c",
            "d",  # original snapshot
            "a",
            "e",
            "c",
            "d",  # first payload snapshot
            "a",
            "b",
            "f",
            "d",  # repeated payload and nested tag snapshot
            "g",
            "e",
            "f",
            "h",  # final transport
            "flag",
        )
    ] + [str(1 - int(row["flag"])), row["flag"], row["a"]]


def planes(text):
    # Native inputs deliberately retain latent value bits behind X/Z. Each
    # scalar is packed independently before concatenating the expected fields.
    return (
        "".join(
            (
                "1"
                if char == "1" or (char in "xz" and (len(text) - index - 1) % 3 == 0)
                else "0"
            )
            for index, char in enumerate(text)
        ),
        "".join("1" if char in "01" else "0" for char in text),
        "".join("1" if char == "z" else "0" for char in text),
    )


expected = ["".join(expected_fields(row)) for row in rows]
expected_planes = [
    [
        "".join(planes(field)[plane] for field in expected_fields(row))
        for plane in range(3)
    ]
    for row in rows
]
result_width = 112
assert all(len(value) == result_width for value in expected)
(build / "vectors.json").write_text(
    json.dumps(
        {
            "input_widths": widths,
            "rows": rows,
            "expected": expected,
            "expected_value_known_z": expected_planes,
        },
        indent=2,
    )
    + "\n"
)
header = [f"constexpr unsigned row_count = {len(rows)}, result_width = {result_width};"]
for name, values in (
    ("expected", expected),
    *(
        ("expected_" + name, [row[index] for row in expected_planes])
        for index, name in enumerate(("value", "known", "z"))
    ),
):
    header.append(
        f"constexpr std::string_view {name}[] = {{"
        + ",".join(json.dumps(value) for value in values)
        + "};"
    )
header.append("void drive(pyc_dut::Inputs &ports, unsigned row) { switch(row) {")
for index, row in enumerate(rows):
    header.append(f"case {index}:")
    header.extend(
        f'ports.{name}=input<{widths[name]}>("{value}");' for name, value in row.items()
    )
    header.append("break;")
header.append("default: require(false); } }")
(build / "record-projection-vectors.hpp").write_text("\n".join(header) + "\n")
sv = [
    f"localparam integer row_count={len(rows)};",
    f"wire [{result_width - 1}:0] result;",
]
sv.extend(f"logic [{width - 1}:0] {name}=0;" for name, width in widths.items())
sv.append("task drive(input integer row); case(row)")
for index, row in enumerate(rows):
    sv.append(f"{index}:begin")
    sv.extend(f"{name}={widths[name]}'b{value};" for name, value in row.items())
    sv.append("end")
sv.extend(
    [
        "endcase endtask",
        f"function automatic logic [{result_width - 1}:0] golden(input integer row); case(row)",
    ]
)
sv.extend(
    f"{index}:golden={result_width}'b{value};" for index, value in enumerate(expected)
)
sv.extend(
    [
        "default:golden='x; endcase endfunction",
        "function automatic bit known_row(input integer row); known_row=(row<24); endfunction",
    ]
)
(build / "record-projection-vectors.svh").write_text("\n".join(sv) + "\n")

source = build / "source"
source.mkdir()
design = source / "design.py"
original = (fixtures / "record-projection-simplification-design.py").read_text()
design.write_text(original)
unit = build / "unit"


def compile_source(output, accepted=True, replace=False):
    arguments = [
        "compile",
        "-c",
        design,
        "--source-root",
        source,
        "--package-prefix",
        "record_probe",
        "-o",
        output,
    ]
    if replace:
        arguments.append("--replace")
    return cli(*arguments, accepted=accepted)


compile_source(unit)
# Use the same capture and native source-unit route to retain the real imported
# module. Ordinary published outputs remain the authority for linking below.
sys.path.insert(0, str(repo / "python"))


def capture_source_transport(path):
    from pycircuit._source_capture import _capture_source_file
    from pycircuit._source_transport import _emit_source_transport

    return _emit_source_transport(_capture_source_file(path, source_root=source))


native = build / "native-reproduction"
native.mkdir()


def capture_file(path):
    transport = native / (path.stem + ".transport.mlir")
    transport.write_text(
        capture_source_transport(path),
        encoding="utf-8",
    )
    return transport


transport = capture_file(design)


def native_compile(
    captured,
    owner_path,
    outputs,
    accepted=True,
    file_size_limit=None,
):
    command = [
        args.source_compiler,
        "--capture",
        captured,
        "--package",
        "record_probe",
        "--path",
        owner_path,
        "--body-out",
        outputs[0],
        "--interface-out",
        outputs[1],
        "--deps-out",
        outputs[2],
    ]
    return run(command, accepted, file_size_limit)


ordinary_names = ("design.ac", "design.interface.ac", "consumed.json")
default = native / "default"
default.mkdir()
off_outputs = [default / name for name in ordinary_names]
native_compile(transport, "design.py", off_outputs)
assert sorted(path.name for path in default.iterdir()) == sorted(ordinary_names)
for name in ordinary_names[:2]:
    assert (default / name).read_bytes() == (unit / name).read_bytes(), name
stdout_result = native_compile(transport, "design.py", ["-", "-", "-"])
assert stdout_result.stdout == "".join(
    (default / name).read_text() for name in (ordinary_names[2], *ordinary_names[:2])
)

final = build / "design.ac"
cli("link", unit, "--top", "record_probe.design.Top", "-o", final)
replay_base = [
    args.linker,
    "--body",
    unit / "design.ac",
    "--header",
    unit / "design.interface.ac",
    "--top",
    "record_probe.design.Top",
    "--target",
    "final",
]
replay = native / "replay-final.ac"
transaction_input = native / "before-transactions.ac"
transaction_output = native / "after-transactions.ac"
run(
    [
        *replay_base,
        "--output",
        replay,
        "--transaction-input-out",
        transaction_input,
        "--transaction-output-out",
        transaction_output,
    ]
)
assert replay.read_bytes() == final.read_bytes() == transaction_output.read_bytes()
before_text = transaction_input.read_text()
after_text = transaction_output.read_text()
assert '"ac.transaction"' in before_text and '"ac.transaction"' not in after_text
assert '"ac.rule"' in after_text
assert "ac.struct.create" in before_text and "ac.struct.get" in before_text
# Equivalent Table shapes with different source provenance cannot be replaced
# directly in SSA. The verified final projection stays as the representation bridge.
assert "ac.struct.get" in after_text
optimizer = Path(args.source_compiler).resolve().parent / "pycircuit-opt"
for stage in (transaction_input, transaction_output):
    run([optimizer, stage, "-o", native / ("parsed-" + stage.name)])

protection = native / "stage-output-protection"
protection.mkdir()
sentinel = protection / "sentinel"
sentinel.write_bytes(b"existing stage bytes\n")
protected = {
    path: path.read_bytes() for path in (*unit.rglob("*"), final) if path.is_file()
}
protected[sentinel] = sentinel.read_bytes()
body_alias = protection / "body-alias.ac"
body_alias.symlink_to(unit / "design.ac")
stage_cases = [
    ("missing-pair", ["--transaction-input-out", protection / "before.ac"], 2),
    (
        "verify-mode",
        [
            "--verify-only",
            "--transaction-input-out",
            protection / "before.ac",
            "--transaction-output-out",
            protection / "after.ac",
        ],
        2,
    ),
    (
        "duplicate-stage-flag",
        [
            "--transaction-input-out",
            protection / "before.ac",
            "--transaction-input-out",
            protection / "extra-before.ac",
            "--transaction-output-out",
            protection / "after.ac",
        ],
        2,
    ),
    (
        "same-stage",
        [
            "--transaction-input-out",
            protection / "shared.ac",
            "--transaction-output-out",
            protection / "shared.ac",
        ],
        1,
    ),
    (
        "body-input-alias",
        [
            "--transaction-input-out",
            body_alias,
            "--transaction-output-out",
            protection / "after.ac",
        ],
        1,
    ),
    (
        "header-input-alias",
        [
            "--transaction-input-out",
            unit / "design.interface.ac",
            "--transaction-output-out",
            protection / "after.ac",
        ],
        1,
    ),
    (
        "existing-stage",
        [
            "--transaction-input-out",
            sentinel,
            "--transaction-output-out",
            protection / "after.ac",
        ],
        1,
    ),
    (
        "missing-parent",
        [
            "--transaction-input-out",
            protection / "before.ac",
            "--transaction-output-out",
            protection / "absent" / "after.ac",
        ],
        1,
    ),
    (
        "final-alias",
        [
            "--transaction-input-out",
            protection / "final.ac",
            "--transaction-output-out",
            protection / "after.ac",
        ],
        1,
    ),
    (
        "empty-stage",
        [
            "--transaction-input-out",
            "",
            "--transaction-output-out",
            protection / "after.ac",
        ],
        1,
    ),
    (
        "stdout-stage",
        [
            "--transaction-input-out",
            "-",
            "--transaction-output-out",
            protection / "after.ac",
        ],
        2,
    ),
]
stage_rejections = []
for name, options, expected_exit in stage_cases:
    run(
        [*replay_base, "--output", protection / "final.ac", *options],
        accepted=False,
        expected_exit=expected_exit,
    )
    assert all(path.read_bytes() == value for path, value in protected.items()), name
    assert not any(
        (protection / filename).exists()
        for filename in (
            "final.ac",
            "before.ac",
            "after.ac",
            "shared.ac",
            "extra-before.ac",
        )
    ), name
    stage_rejections.append(name)

hard_alias = protection / "hard-body-alias.ac"
os.link(unit / "design.ac", hard_alias)
run(
    [
        *replay_base,
        "--output",
        protection / "final.ac",
        "--transaction-input-out",
        hard_alias,
        "--transaction-output-out",
        protection / "after.ac",
    ],
    accepted=False,
)
assert all(path.read_bytes() == value for path, value in protected.items())
assert not (protection / "final.ac").exists() and not (protection / "after.ac").exists()
stage_rejections.append("hard-link-input-alias")
run(
    [
        *replay_base,
        "--output",
        sentinel,
        "--transaction-input-out",
        protection / "before.ac",
        "--transaction-output-out",
        protection / "after.ac",
    ],
    accepted=False,
)
assert all(path.read_bytes() == value for path, value in protected.items())
assert (
    not (protection / "before.ac").exists() and not (protection / "after.ac").exists()
)
stage_rejections.append("existing-final")

# A child-only POSIX limit induces a real regular-file stream error. All three
# fresh final-link outputs must be removed when staged publication fails.
try:
    import resource
    import signal
except ImportError:
    stream_failure = "skipped: POSIX resource/signal modules unavailable"
else:
    if not hasattr(resource, "RLIMIT_FSIZE") or not hasattr(signal, "SIGXFSZ"):
        stream_failure = "skipped: POSIX file-size limit or signal unavailable"
    else:
        directory = native / "stage-stream-error"
        directory.mkdir()
        outputs = [directory / name for name in ("final.ac", "before.ac", "after.ac")]
        limit = max(1, len(transaction_input.read_bytes()) // 2)
        run(
            [
                *replay_base,
                "--output",
                outputs[0],
                "--transaction-input-out",
                outputs[1],
                "--transaction-output-out",
                outputs[2],
            ],
            accepted=False,
            file_size_limit=limit,
        )
        assert not any(path.exists() for path in outputs)
        stage_rejections.append("stage-stream-error")
        stream_failure = {
            "child_file_size_limit": limit,
            "fresh_final_and_stages_removed": True,
        }
for target in ("cpp", "verilog"):
    cli("emit", final, "--target", target, "-o", build / target)
toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next(
    (
        path
        for path in (
            toolroot / "runtime/libpyc6_runtime.a",
            toolroot / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    ),
    None,
)
assert runtime is not None, "Runtime archive missing from this build/install"
cpp_receipt = json.loads((build / "cpp/generated.json").read_text())
cpp = [
    build / "cpp" / row["path"]
    for row in cpp_receipt["files"]
    if row["path"].endswith(".cpp")
]
assert cpp, "generated C++ source missing"
runner = build / "runner"
run(
    [
        args.cxx,
        "-std=c++20",
        "-pthread",
        "-I" + str(repo / "include"),
        "-I" + str(build / "cpp"),
        "-I" + str(build),
        fixtures / "record-projection-simplification.cpp",
        *cpp,
        runtime,
        "-o",
        runner,
    ]
)
config = build / "config.json"
config.write_text(
    json.dumps(
        {
            "deadlock_window": None,
            "max_domain_cycles": {},
            "max_ticks": 64,
            "schema": "pycircuit-model-config",
            "version": "1",
        },
        separators=(",", ":"),
    )
    + "\n"
)
expected_trace = [f"WORK {index} {value}" for index, value in enumerate(expected)]
for workers in (1, 2):
    trace = run([runner, "--workers", workers, "--config", config]).stdout
    (build / f"workers-{workers}.stdout").write_text(trace)
    assert [
        line for line in trace.splitlines() if line.startswith("WORK ")
    ] == expected_trace

rtl_receipt = json.loads((build / "verilog/generated.json").read_text())
rtl = [
    build / "verilog" / row["path"]
    for row in rtl_receipt["files"]
    if row["role"] == "rtl"
]
rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
bench = fixtures / "record-projection-simplification.sv"
run(
    [
        args.verilator,
        "--binary",
        "--timing",
        "--top-module",
        "tb",
        "--prefix",
        "Vrecords",
        "--Mdir",
        build / "rtl-build",
        "-j",
        "2",
        "-Wno-fatal",
        "-I" + str(build),
        *rtl,
        bench,
    ]
)
trace = run([build / "rtl-build/Vrecords"]).stdout
(build / "verilator.stdout").write_text(trace)
assert [
    line for line in trace.splitlines() if line.startswith("WORK ")
] == expected_trace[:24]
icarus = build / "icarus"
run(
    [
        args.iverilog,
        "-g2012",
        "-DRECORD_PROJECTION_FOUR_STATE",
        "-I" + str(build),
        "-s",
        "tb",
        "-o",
        icarus,
        *rtl,
        bench,
    ]
)
trace = run([args.vvp, icarus]).stdout
(build / "icarus.stdout").write_text(trace)
assert [
    line for line in trace.splitlines() if line.startswith("WORK ")
] == expected_trace

protected_unit = snapshot(unit)
protected_final = final.read_bytes()
protected_products = {target: snapshot(build / target) for target in ("cpp", "verilog")}
cases = {
    "original-boolean-alias-arithmetic": original.replace(
        "arithmetic=box.bit + 1", "arithmetic=original_alias + 1"
    ),
    "original-boolean-alias-helper": original.replace(
        "helper=ac.popcount(box.bit)", "helper=ac.popcount(original_alias)"
    ),
}
for name, invalid in cases.items():
    design.write_text(invalid)
    (build / (name + ".py")).write_text(invalid)
    for output, replace in ((build / ("invalid-" + name), False), (unit, True)):
        rejected = compile_source(output, accepted=False, replace=replace)
        assert any(
            word in rejected.stderr.lower()
            for word in ("boolean", "integer", "kind", "fixed", "arithmetic")
        ), commands[-1]
        if not replace:
            assert not output.exists(), name
        assert snapshot(unit) == protected_unit, name
        assert final.read_bytes() == protected_final, name
        assert all(
            snapshot(build / target) == before
            for target, before in protected_products.items()
        ), name
design.write_text(original)
paths = [
    fixtures / ("record-projection-simplification" + suffix)
    for suffix in ("-design.py", ".py", ".cpp", ".sv")
]
(build / "candidate.json").write_text(
    json.dumps(
        {
            "fixture_sha256": {
                str(path.relative_to(repo)): digest(path) for path in paths
            },
            "verified_final_sha256": digest(final),
            "workers": [1, 2],
            "known_frames": 24,
            "four_state_frames": 4,
            "packed_output_width": result_width,
            "native_exact_value_known_z_planes": True,
            "icarus_symbolic_four_state": True,
            "verilator_default_known_oracle": True,
            "rejected_cases": sorted(cases),
            "failed_compile_preserved_unit_and_products": True,
            "state_defaults_branches_authority": "existing focused regression gates",
            "transaction_stage_retention": {
                "native_body_interface_match_public_compile": True,
                "ordinary_stdout_bytes_unchanged": True,
                "before_and_after_parsed_verified": True,
                "after_matches_native_replay_and_public_final": True,
                "projection_retained_across_final_lowering": True,
                "protected_stage_rejections": stage_rejections,
                "stage_stream_failure": stream_failure,
                "artifacts_sha256": {
                    str(path.relative_to(native)): digest(path)
                    for path in sorted(native.rglob("*"))
                    if path.is_file() and not path.is_symlink()
                },
            },
        },
        indent=2,
    )
    + "\n"
)
print(
    f"record projection public-flow evidence: {build}"
)  # noqa: T201  # Report retained gate evidence.
