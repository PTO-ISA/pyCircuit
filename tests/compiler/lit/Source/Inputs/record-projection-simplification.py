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
    PYTHONPATH=str(repo / "python/pycircuit/src"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def run(command, accepted=True, file_size_limit=None):
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
    ] + [str(1 - int(row["flag"])), row["flag"]]


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
result_width = 107
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
sys.path.insert(0, str(repo / "python/pycircuit/src"))


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
    diagnostic=None,
    headers=(),
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
    for header_path in headers:
        command.extend(["--header", header_path])
    if diagnostic is not None:
        command.append("--source-import-out=" + str(diagnostic))
    return run(command, accepted, file_size_limit)


ordinary_names = ("design.ac", "design.interface.ac", "consumed.json")
default = native / "default"
enabled = native / "enabled"
for directory in (default, enabled):
    directory.mkdir()
off_outputs = [default / name for name in ordinary_names]
on_outputs = [enabled / name for name in ordinary_names]
native_compile(transport, "design.py", off_outputs)
assert sorted(path.name for path in default.iterdir()) == sorted(ordinary_names)
source_import = enabled / "source-import.ac"
native_compile(transport, "design.py", on_outputs, source_import)
for before, after in zip(off_outputs, on_outputs, strict=True):
    assert before.read_bytes() == after.read_bytes(), before.name
for name in ordinary_names[:2]:
    assert (enabled / name).read_bytes() == (unit / name).read_bytes(), name
imported_text = source_import.read_text()
assert imported_text.endswith("\n")
assert "ac.struct.create" in imported_text and "ac.struct.get" in imported_text
assert "ac.struct.get" not in (enabled / "design.ac").read_text()
# The existing optimizer parser/verifier consumes the complete diagnostic; no
# alternate lowering pipeline supplies it. Typed API checks own the relation.
run(
    [
        Path(args.source_compiler).resolve().parent / "pycircuit-opt",
        source_import,
        "-o",
        enabled / "parsed-source-import.ac",
    ]
)
stdout_import = native / "stdout-source-import.ac"
stdout_result = native_compile(transport, "design.py", ["-", "-", "-"], stdout_import)
assert stdout_result.stdout == "".join(
    (enabled / name).read_text() for name in (ordinary_names[2], *ordinary_names[:2])
)

# Supply a real independently compiled header for input-alias protection.
provider = source / "provider.py"
provider.write_text(
    "import pycircuit as ac\n@ac.struct\n" "class HeaderPayload:\n    value: ac.u1\n"
)
provider_capture = capture_file(provider)
provider_outputs = [native / ("provider-" + name) for name in ordinary_names]
native_compile(provider_capture, "provider.py", provider_outputs)
provider_header = provider_outputs[1]

diagnostic_rejections = []
for name in (
    "existing-file",
    "existing-directory",
    "existing-symlink",
    "dangling-diagnostic-symlink",
    "capture-alias",
    "header-alias",
    "body-alias",
    "interface-alias",
    "deps-alias",
    "symlink-parent-alias",
    "dangling-output-alias",
    "missing-parent",
    "empty",
    "stdout",
    "late-body-open-error",
    "late-deps-open-error",
    "invalid-source",
):
    directory = native / name
    directory.mkdir()
    outputs = [directory / filename for filename in ordinary_names]
    for path in outputs:
        path.write_bytes(b"ordinary output sentinel\n")
    protected = {path: path.read_bytes() for path in outputs}
    protected.update(
        {
            transport: transport.read_bytes(),
            provider_header: provider_header.read_bytes(),
        }
    )
    destination = directory / "source-import.ac"
    captured, owner_path, headers = transport, "design.py", ()
    existing_link = None
    if name == "existing-file":
        destination.write_bytes(b"diagnostic sentinel\n")
        protected[destination] = destination.read_bytes()
    elif name == "existing-directory":
        destination.mkdir()
        marker = destination / "sentinel"
        marker.write_bytes(b"directory sentinel\n")
        protected[marker] = marker.read_bytes()
    elif name in ("existing-symlink", "dangling-diagnostic-symlink"):
        target = directory / "target"
        if name == "existing-symlink":
            target.write_bytes(b"symlink target sentinel\n")
            protected[target] = target.read_bytes()
        destination.symlink_to(target)
        existing_link = (destination, target)
    elif name == "capture-alias":
        destination = transport
    elif name == "header-alias":
        destination, headers = provider_header, (provider_header,)
    elif name in ("body-alias", "interface-alias", "deps-alias"):
        index = {"body-alias": 0, "interface-alias": 1, "deps-alias": 2}[name]
        outputs[index].unlink()
        protected.pop(outputs[index])
        destination = outputs[index]
    elif name == "symlink-parent-alias":
        real = directory / "real"
        real.mkdir()
        alias = directory / "alias"
        alias.symlink_to(real, target_is_directory=True)
        protected.pop(outputs[0])
        outputs[0].unlink()
        destination, outputs[0] = real / "fresh.ac", alias / "fresh.ac"
    elif name == "dangling-output-alias":
        protected.pop(outputs[0])
        outputs[0].unlink()
        outputs[0].symlink_to(destination)
        existing_link = (outputs[0], destination)
    elif name == "missing-parent":
        destination = directory / "missing" / "source-import.ac"
    elif name == "empty":
        destination = ""
    elif name == "stdout":
        destination = "-"
    elif name in ("late-body-open-error", "late-deps-open-error"):
        index = 0 if name == "late-body-open-error" else 2
        protected.pop(outputs[index])
        outputs[index].unlink()
        outputs[index].mkdir()
        if index == 0:
            # Dependencies precede the body in the existing sequential writer;
            # this is deliberately not a cross-file atomicity assertion.
            protected.pop(outputs[2])
    elif name == "invalid-source":
        invalid = source / "invalid.py"
        invalid.write_text(
            "import pycircuit as ac\n@ac.struct\nclass Payload:\n    value: ac.u1\n"
            "@ac.module\ndef Invalid(value: ac.u1) -> Payload:\n    return Payload(value=missing)\n"
        )
        captured, owner_path = capture_file(invalid), "invalid.py"
    rejected = native_compile(
        captured, owner_path, outputs, destination, headers, accepted=False
    )
    assert all(path.read_bytes() == value for path, value in protected.items()), name
    if existing_link is not None:
        link, target = existing_link
        assert link.is_symlink() and link.readlink() == target, name
        if name in ("dangling-diagnostic-symlink", "dangling-output-alias"):
            assert not target.exists(), name
    elif (
        isinstance(destination, Path)
        and destination not in protected
        and name != "existing-directory"
    ):
        assert not destination.exists(), name
    if name == "invalid-source":
        assert "unknown hardware value 'missing'" in rejected.stderr
    diagnostic_rejections.append(name)

# A child-only POSIX limit induces a real regular-file stream error, without
# changing product behavior or writing to a special device. The bound derives
# from the complete successful diagnostic rather than a compiler magic limit.
try:
    import resource
    import signal
except ImportError:
    stream_failure = "skipped: POSIX resource/signal modules unavailable"
else:
    if not hasattr(resource, "RLIMIT_FSIZE") or not hasattr(signal, "SIGXFSZ"):
        stream_failure = "skipped: POSIX file-size limit or signal unavailable"
    else:
        directory = native / "diagnostic-stream-error"
        directory.mkdir()
        outputs = [directory / filename for filename in ordinary_names]
        for path in outputs:
            path.write_bytes(b"ordinary stream-error sentinel\n")
        protected = {path: path.read_bytes() for path in outputs}
        destination = directory / "source-import.ac"
        limit = max(1, len(source_import.read_bytes()) // 2)
        native_compile(
            transport,
            "design.py",
            outputs,
            destination,
            accepted=False,
            file_size_limit=limit,
        )
        assert not destination.exists()
        assert all(path.read_bytes() == value for path, value in protected.items())
        diagnostic_rejections.append("diagnostic-stream-error")
        stream_failure = {
            "child_file_size_limit": limit,
            "fresh_diagnostic_removed": True,
            "ordinary_sentinels_unchanged": True,
        }

final = build / "design.ac"
cli("link", unit, "--top", "record_probe.design.Top", "-o", final)
for target in ("cpp", "verilog"):
    cli("emit", final, "--target", target, "-o", build / target)
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
            "source_import_retention": {
                "default_and_enabled_ordinary_bytes_equal": True,
                "enabled_body_interface_match_public_compile": True,
                "complete_pre_simplify_parsed_verified": True,
                "ordinary_stdout_bytes_unchanged": True,
                "protected_diagnostic_rejections": diagnostic_rejections,
                "late_output_failure_removes_fresh_diagnostic": True,
                "diagnostic_stream_failure": stream_failure,
                "later_semantic_pipeline_failure": "no genuine post-Lower rejection fixture found; not executed",
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
