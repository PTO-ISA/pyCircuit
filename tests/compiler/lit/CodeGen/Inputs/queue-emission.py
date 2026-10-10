"""Independent generated FIFO tests: deque order, explicit edge and wire planes."""

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from collections import deque
from pathlib import Path

parser = argparse.ArgumentParser()
for name in (
    "repo",
    "emitter",
    "optimizer",
    "cxx",
    "iverilog",
    "vvp",
    "verilator",
    "scratch",
):
    parser.add_argument("--" + name, required=True)
parser.add_argument(
    "--only",
    choices=("all", "tokens", "latency", "huge", "atomic", "owners", "capacity"),
    default="all",
)
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
evidence = Path(args.scratch).resolve()
evidence.mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="run-", dir=evidence))
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYCIRCUIT_EMITTER=args.emitter,
    PYTHONDONTWRITEBYTECODE="1",
)
commands = []
bound_inputs = [
    fixtures / ("queue-emission." + suffix) for suffix in ("py", "mlir", "cpp", "sv")
]
bound_inputs += [
    repo / "include/gfsim/fifo.h",
    repo / "include/verilog/fifo.v",
    Path(args.emitter).resolve(),
    Path(args.optimizer).resolve(),
    Path(args.emitter).resolve().parent.parent / "runtime/libpyc6_runtime.a",
]
initial_hashes = {
    str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in bound_inputs
}
(evidence / "inputs.json").write_text(
    json.dumps(
        {"inputs": initial_hashes, "invocation": sys.argv, "work": str(work)}, indent=2
    )
    + "\n"
)


def run(command, code=0):
    command = list(map(str, command))
    r = subprocess.run(
        command, cwd=repo, env=env, capture_output=True, text=True, timeout=240
    )
    commands.append(
        {
            "command": command,
            "cwd": str(repo),
            "exit_status": r.returncode,
            "stdout": r.stdout,
            "stderr": r.stderr,
        }
    )
    (evidence / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert r.returncode == code if code is not None else r.returncode != 0, commands[-1]
    return r


def emit_case(name, targets, contents=None):
    section = (
        contents
        if contents is not None
        else next(
            s
            for s in (fixtures / "queue-emission.mlir").read_text().split("// -----")
            if "// QUEUE-CASE: " + name in s
        )
    )
    final = work / (name + ".ac")
    final.write_text(section)
    run(
        [
            args.optimizer,
            final,
            "--ac-verify-hardware",
            "-o",
            work / (name + ".verified.ac"),
        ]
    )
    for target in targets:
        run(
            [
                sys.executable,
                "-m",
                "pycircuit.cli",
                "emit",
                final,
                "--target",
                target,
                "-o",
                work / (name + "-" + target),
            ]
        )
    return final


def native(name, define):
    bundle = work / (name + "-cpp")
    receipt = json.loads((bundle / "generated.json").read_text())
    sources = [
        bundle / x["path"] for x in receipt["files"] if x["path"].endswith(".cpp")
    ]
    runtime = Path(args.emitter).resolve().parent.parent / "runtime/libpyc6_runtime.a"
    runner = work / (name + "-runner")
    run(
        [
            args.cxx,
            "-std=c++20",
            "-O1",
            "-pthread",
            *[
                "-D" + item
                for item in (define if isinstance(define, tuple) else (define,))
            ],
            "-I" + str(repo / "include"),
            "-I" + str(bundle),
            fixtures / "queue-emission.cpp",
            *sources,
            runtime,
            "-o",
            runner,
        ]
    )
    return run([runner])


if args.only in ("all", "atomic"):
    emit_case("atomic", ("cpp",))
    # Prove the actual WorkPartition batch and join precede late parent Work.
    generated = "\n".join(
        p.read_text() for p in (work / "atomic-cpp/sources").glob("*.hpp")
    )
    begin = generated.index("std::array<gfsim::WorkItem, 2>")
    join = generated.index("pyc_work_executor_->run(pyc_tasks)", begin)
    parent = generated.index("pyc_instance_parent_reg_state->work(", join)
    assert (
        "pyc_instance_left.get()" in generated[begin:join]
        and "pyc_instance_right.get()" in generated[begin:join]
    )
    (work / "atomic-work-schedule.txt").write_text(
        generated[begin : parent + len("pyc_instance_parent_reg_state->work(")]
    )
    result = native("atomic", "QUEUE_ATOMIC")
    assert sum("passed" in line for line in result.stdout.splitlines()) == 6
    (evidence / "atomic.json").write_text(
        json.dumps(
            {
                "work": str(work),
                "workers": [1, 2],
                "schedule": str(work / "atomic-work-schedule.txt"),
                "scope": "direct same-edge retry, generated wrapper lifecycle and separate failed-Step Reset recovery",
                "stdout": result.stdout,
            },
            indent=2,
        )
        + "\n"
    )
    atomic = next(
        s
        for s in (fixtures / "queue-emission.mlir").read_text().split("// -----")
        if "// QUEUE-CASE: atomic" in s
    )
    delayed_atomic = atomic.replace(
        "availability_latency = #n1", "availability_latency = #n2"
    )
    emit_case("atomic-delayed", ("cpp",), delayed_atomic)
    delayed_result = native("atomic-delayed", ("QUEUE_ATOMIC", "QUEUE_DELAYED_ATOMIC"))
    assert sum("passed" in line for line in delayed_result.stdout.splitlines()) == 4
    (evidence / "atomic-delayed.json").write_text(
        json.dumps(
            {
                "work": str(work),
                "workers": [1, 2],
                "latency": 2,
                "scope": "late parent failure at maturity edge; global discard and same-edge owner retry",
                "stdout": delayed_result.stdout,
            },
            indent=2,
        )
        + "\n"
    )
    if args.only == "atomic":
        sys.exit(0)

if args.only in ("all", "owners"):
    owners = []
    for name in ("dead", "collision", "dead-delayed"):
        dead = name.startswith("dead")
        contents = None
        if name == "dead-delayed":
            contents = next(
                s
                for s in (fixtures / "queue-emission.mlir")
                .read_text()
                .split("// -----")
                if "// QUEUE-CASE: dead" in s
            )
            contents = contents.replace("depth = #n3", "depth = #n1").replace(
                "availability_latency = #n1", "availability_latency = #n3"
            )
        emit_case(name, ("cpp", "verilog"), contents)
        defines = ("QUEUE_OWNERS", "QUEUE_DEAD") if dead else ("QUEUE_OWNERS",)
        if name == "dead-delayed":
            defines += ("QUEUE_DELAYED_DEAD",)
        result = native(name, defines)
        assert len(result.stdout.splitlines()) == 2
        bundle = work / (name + "-verilog")
        receipt = json.loads((bundle / "generated.json").read_text())
        rtl = [bundle / r["path"] for r in receipt["files"] if r["role"] == "rtl"]
        rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
        run(
            [
                args.iverilog,
                "-g2012",
                *["-D" + item for item in defines],
                "-s",
                "tb",
                "-o",
                work / (name + ".vvp"),
                *sorted((repo / "include/verilog").glob("*.v")),
                *rtl,
                fixtures / "queue-emission.sv",
            ]
        )
        rtl_result = run([args.vvp, work / (name + ".vvp")], code=None if dead else 0)
        if dead:
            assert "DEAD constant output checked" in rtl_result.stdout
            assert "unused queue failure was removed" not in rtl_result.stdout
            assert "fifo: effective transfers must be known" in rtl_result.stdout
        else:
            assert "COLLISION fifo plus fifo_state passed" in rtl_result.stdout
        owners.append({"name": name, "native": result.stdout, "rtl": rtl_result.stdout})
    (evidence / "owners.json").write_text(
        json.dumps({"work": str(work), "results": owners}, indent=2) + "\n"
    )
    if args.only == "owners":
        sys.exit(0)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def snapshot(path):
    return {
        p.relative_to(path).as_posix(): digest(p)
        for p in sorted(path.rglob("*"))
        if p.is_file()
    }


if args.only in ("all", "capacity"):
    # Independent architect witness source SHA256:
    # 448693fe27c5a59c1997afc7a3890bbed1cbe6764dbbb30047f9d6ea1345f6ff
    text = (fixtures / "queue-emission.mlir").read_text().split("// -----")[0]
    aliases = text.split("module {", 1)[0]
    sections = text.split('  "ac.module"() ({')
    pair = '  "ac.module"() ({' + next(
        s for s in sections[1:] if 'sym_name = "QueueTablePair"' in s
    )
    pair = pair[: pair.index(" : () -> ()") + len(" : () -> ()")] + "\n"
    checks = []

    def literal(name, value):
        return f'#{name} = #ac.static_expr<{{kind = "literal", location = {{path = "queue_emission.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}, origin = {{site = {{definition = @QueueTokens, ast_path = []}}, expansion = []}}, value = {{kind = "integer", value = #ac.math_int<{value}>}}}}>\n'

    def fixture(depth=3, family=2, unused=None, latency=1):
        header = (
            aliases
            + literal("capacity_depth", depth)
            + literal("capacity_family", family)
            + literal("capacity_latency", latency)
        )
        c = "!ac.table<[#capacity_family], !b1>"
        d = "!ac.table<[#capacity_family, #n3], !b130>"
        outputs = f"{c}, {c}, {d}, {c}, {c}, {d}"
        root = f"""  "ac.module"() ({{
  ^bb0(%clk: !b1, %rst: !b1, %valid: {c}, %take: {c}, %data: {d}):
    %clocks = "ac.table.splat"(%clk) {{shape = [#capacity_family]}} : (!b1) -> {c}
    %resets = "ac.table.splat"(%rst) {{shape = [#capacity_family]}} : (!b1) -> {c}
    %lr, %lv, %ld, %pr, %pv, %pd = "ac.collection"(%valid, %data, %take, %clocks, %resets) {{instance_name = "pair", callee = @QueueTablePair, parameters = [#capacity_depth, #capacity_latency], type_arguments = [!b130], shape = [#capacity_family], occurrence = {{site = {{definition = @QueueTokens, ast_path = []}}, expansion = []}}}} : ({c}, {d}, {c}, {c}, {c}) -> ({outputs})
    "ac.yield"(%lr, %lv, %ld, %pr, %pv, %pd) : ({outputs}) -> ()
  }}) {{sym_name = "QueueTokens", source_owner = {{package = "", path = "queue_emission.py"}}, parameters = [], type_parameters = [], function_type = (!b1, !b1, {c}, {c}, {d}) -> ({outputs}), input_names = ["clk", "rst", "valid", "take", "data"], output_names = ["local_ready", "local_valid", "local_data", "pop_ready", "pop_valid", "pop_data"]}} : () -> ()
"""
        definitions = pair + root
        entry = "QueueTokens"
        if unused:
            entry = "Plain"
            plain = """  "ac.module"() ({
  ^bb0(%arg: !b1):
    "ac.yield"(%arg) : (!b1) -> ()
  }) {sym_name = "Plain", source_owner = {package = "", path = "queue_emission.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["a"], output_names = ["y"]} : () -> ()
"""
            definitions = (pair if unused == "generic" else definitions) + plain
        return (
            header
            + "module {\n"
            + definitions
            + f'  "ac.system"() {{entry = {{callee = @{entry}, parameters = [], type_arguments = []}}, domain = "default"}} : () -> ()\n}}\n'
        )

    # Bounds derive from the frozen packet, independently using arbitrary-precision arithmetic.
    limit = (1 << 63) - 1
    token_bytes = 3 * 3 * 8 * ((130 + 63) // 64)
    packed_round_bytes = 3 * 8 * ((390 + 63) // 64)
    assert token_bytes == 216 and packed_round_bytes == 168
    assert 216 * (44_000_000_000_000_000 + 5) + 192 + 128 > limit
    assert 168 * (44_000_000_000_000_000 + 5) + 192 + 128 < limit
    assert 390 * 44_000_000_000_000_000 < (1 << 64)
    assert 216 * (22_000_000_000_000_000 + 5) + 192 + 128 < limit
    assert 2 * (216 * (22_000_000_000_000_000 + 5) + 192) + 128 > limit
    assert 12_000_000 * 390 > (1 << 32) - 1
    checks.append(
        {
            "name": "independent_geometry",
            "token_bytes": token_bytes,
            "incorrect_packed_round_bytes": packed_round_bytes,
            "single_bad_depth": 44_000_000_000_000_000,
            "family_bad_depth": 22_000_000_000_000_000,
            "family_bad_F": 2,
            "plane_bad_F": 12_000_000,
            "logical_width": 390,
            "passed": True,
        }
    )
    cases = {
        "seed": (fixture(), None),
        "single_lane_large_valid": (fixture(22_000_000_000_000_000, 1), None),
        "unused_small_valid": (fixture(3, 1, "concrete"), None),
        "unused_generic": (fixture(44_000_000_000_000_000, 1, "generic"), None),
        "scalar_rounding": (
            fixture(44_000_000_000_000_000, 1),
            "queue storage including enclosing collections",
        ),
        "family_storage": (
            fixture(22_000_000_000_000_000, 2),
            "queue storage including enclosing collections",
        ),
        "family_plane": (
            fixture(3, 12_000_000),
            "hardware payload plane including enclosing collections",
        ),
        "unused_concrete_bad": (
            fixture(44_000_000_000_000_000, 1, "concrete"),
            "queue storage including enclosing collections",
        ),
        "delayed_scalar_storage": (
            fixture(41_600_000_000_000_000, 1, latency=3),
            "queue storage including enclosing collections",
        ),
        "delayed_family_storage": (
            fixture(20_800_000_000_000_000, 2, latency=3),
            "queue storage including enclosing collections",
        ),
        "delayed_unused_concrete": (
            fixture(41_600_000_000_000_000, 1, "concrete", latency=3),
            "queue storage including enclosing collections",
        ),
        "oversized_generic_latency": (
            fixture(3, 1, latency=(1 << 64) - 1),
            "emitted signed 64-bit domain",
        ),
    }
    # These fit the old payload-only allocation bound and fail only when the
    # delayed timing storage is included. Arithmetic uses Python big integers.
    for d, f in ((41_600_000_000_000_000, 1), (20_800_000_000_000_000, 2)):
        assert f * (216 * (d + 5) + 192) + 128 < limit
        assert f * (216 * (d + 5) + 8 * d + 216) + 128 > limit

    def emit(path, target, destination, label, reject=None):
        r = run(
            [
                sys.executable,
                "-m",
                "pycircuit.cli",
                "emit",
                path,
                "--target",
                target,
                "-o",
                destination,
                "--replace",
            ],
            code=None if reject else 0,
        )
        if reject:
            assert reject in r.stderr + r.stdout, (label, r.stderr, r.stdout)
        return r

    for name, (contents, reject) in cases.items():
        path = work / (name + ".ac")
        path.write_text(contents)
        run(
            [
                args.optimizer,
                path,
                "--ac-verify-hardware",
                "-o",
                work / (name + ".verified.ac"),
            ]
        )
        for target in ("cpp", "verilog"):
            dest = work / (name + "-" + target)
            emit(path, target, dest, name + "-" + target, reject)
            if reject:
                assert not dest.exists(), f"failed fresh publication created {dest}"
                seed = work / ("seed-" + target)
                prior = snapshot(seed)
                receipt = prior["generated.json"]
                control = work / ("." + seed.name + ".pycircuit-publication")
                control_prior = snapshot(control)
                emit(path, target, seed, name + "-" + target + "-replacement", reject)
                assert (
                    snapshot(seed) == prior
                ), "failed replacement altered prior generated bundle"
                assert (
                    snapshot(control) == control_prior
                ), "failed replacement altered publication controls"
                checks.append(
                    {
                        "name": name + "-" + target,
                        "passed": True,
                        "fresh_absent": True,
                        "protected_controls": len(control_prior),
                        "protected_files": len(prior),
                        "receipt_sha256": receipt,
                        "diagnostic": reject,
                    }
                )
            else:
                assert (dest / "generated.json").is_file()
                checks.append(
                    {
                        "name": name + "-" + target,
                        "passed": True,
                        "published_files": len(snapshot(dest)),
                    }
                )

    (evidence / "capacity.json").write_text(
        json.dumps(
            {
                "work": str(work),
                "checks": checks,
                "scope": "independent geometry, target preflight and fresh/replacement bundle protection",
            },
            indent=2,
        )
        + "\n"
    )
    if args.only == "capacity":
        sys.exit(0)

# Widths/layouts come from independent declared packet contracts. No DUT value
# or Runtime FIFO/select operator computes these expected token planes.
cases = [
    ("bit1", 1, 1, 3),
    ("bit13", 13, 1, 5),
    ("bit65", 65, 1, 3),
    ("bit130", 130, 1, 5),
    ("record_a", 79, 1, 3),
    ("record_b", 144, 1, 5),
    ("table130", 130, 3, 3),
    ("table_record", 79, 3, 5),
]


def plane(width, value, known=None, z=0):
    mask = (1 << width) - 1
    return (width, value & mask, mask if known is None else known & mask, z & mask)


def text(p):
    return tuple(format(v, f"0{p[0]}b") for v in p[1:])


def symbols(p):
    w, value, known, z = p
    return "".join(
        "z" if z >> b & 1 else "x" if not (known >> b & 1) else str(value >> b & 1)
        for b in range(w - 1, -1, -1)
    )


def token(width, cardinality, serial, unknown=False):
    result = []
    for element in range(cardinality):
        value = known = z = 0
        for bit in range(width):
            if (serial + element * 7 + bit * 3) % 11 < 5:
                value |= 1 << bit
            state = (serial + element * 5 + bit) % 4
            if not unknown or state < 2:
                known |= 1 << bit
            elif state == 2:
                z |= 1 << bit
        result.append(plane(width, value, known, z))
    return tuple(result)


class Oracle:
    def __init__(self, depth, width, cardinality, bypass, latency=1):
        self.tokens = deque()
        self.clock = False
        self.depth = depth
        self.width = width
        self.cardinality = cardinality
        self.bypass = bypass
        self.latency = latency
        self.time = -1

    def sample(self, clock, reset, valid, take, data):
        available = bool(self.tokens) and self.tokens[0][2] <= self.time
        capacity = len(self.tokens) < self.depth
        # Inactive unknown take is masked when empty/capacity is available.
        ready = (
            plane(1, 1)
            if capacity
            else (
                plane(1, 0)
                if not self.bypass or not available
                else plane(1, int(take)) if take in "01" else plane(1, 1, 0)
            )
        )
        result = (
            ready,
            plane(1, int(available)),
            (
                self.tokens[0][0]
                if available
                else tuple(plane(self.width, 0) for _ in range(self.cardinality))
            ),
        )
        rising = not self.clock and bool(clock)
        if rising:
            if reset:
                self.tokens.clear()
                self.time = -1
            else:
                self.time += 1
                assert valid in "01" and take in "01"
                pop = available and take == "1"
                push = valid == "1" and (capacity or (self.bypass and pop))
                if pop:
                    self.tokens.popleft()
                if push:
                    self.tokens.append((data, self.time, self.time + self.latency - 1))
        self.clock = bool(clock)
        return result


def token_gate(label, final_contents, latencies, independent=False, huge=False):
    global work
    original_work = work
    work = original_work / label
    work.mkdir()
    rows = []
    selected = cases + ([("bit13_second", 13, 1, 5)] if independent or huge else [])
    if independent:
        selected += [("bit13_default", 13, 1, 3)]
    run_cases = [
        (name, width, n, 1 if (independent or huge) and name == "bit1" else depth)
        for name, width, n, depth in selected
    ]

    def row(clock, valid="1", take="0", reset=0, unknown=False):
        rows.append(
            {
                "clk": clock,
                "rst": reset,
                "valid": [valid, valid],
                "take": [take, take],
                "unknown": unknown,
            }
        )

    # Known prefix covers startup, full replacement, interior transfers and wrap.
    row(0, "0", "0")
    for _ in range(5):
        row(0)
        row(1)
    for _ in range(16):
        row(0, "1", "1")
        row(1, "1", "1")
    for _ in range(6):
        row(0, "0", "1")
        row(1, "0", "1")
    for _ in range(130 if independent else 0):
        row(0, "0", "0")
        row(1, "0", "0")
    row(0, "0", "0", 1)
    row(1, "0", "0", 1)
    row(0, "0", "0")
    known_rows = len(rows)
    # Full with held high clock: only declared combinational readiness may vary.
    for _ in range(5):
        row(0, unknown=True)
        row(1, unknown=True)
    for take in "01xz":
        row(1, "0", take, unknown=True)
    for _ in range(8):
        row(0, "1", "1", unknown=True)
        row(1, "1", "1", unknown=True)
    row(0, "0", "0", 1, True)
    row(1, "0", "0", 1, True)
    row(0, "0", "0")
    # Distinct lane controls and data prevent family/token interleaving from hiding.
    for i, r in enumerate(rows):
        if i % 9 == 2 and r["take"][0] in "01" and not r["rst"]:
            r["valid"][1] = "0"
    models = {
        (name, lane, policy): Oracle(depth, width, n, policy == "pop", latencies[name])
        for name, width, n, depth in run_cases
        for lane in range(2)
        for policy in ("local", "pop")
    }
    inputs = []
    gold = []
    for index, r in enumerate(rows):
        if independent:
            r["lane_clk"] = [r["clk"], int((index // 3) % 2)]
            if r["rst"]:
                r["lane_clk"] = [r["clk"], r["clk"]]
            if any(t not in "01" for t in r["take"]):
                r["lane_clk"] = rows[index - 1]["lane_clk"][:]
        inp = {
            "clk": plane(1, r["clk"]),
            "rst": plane(1, r["rst"]),
            "valid": tuple(plane(1, int(v)) for v in r["valid"]),
            "take": tuple(
                plane(1, int(v)) if v in "01" else plane(1, 1, 0, 1 if v == "z" else 0)
                for v in r["take"]
            ),
        }
        if independent:
            inp["lane_clk"] = tuple(plane(1, c) for c in r["lane_clk"])
        expected = {}
        for case_index, (name, width, n, _depth) in enumerate(run_cases):
            data = tuple(
                token(width, n, index * 29 + lane * 11 + case_index * 3, r["unknown"])
                for lane in range(2)
            )
            inp[name] = tuple(p for tok in data for p in tok)
            for policy in ("local", "pop"):
                lanes = [
                    models[name, lane, policy].sample(
                        (r["lane_clk"][lane] if independent else r["clk"]),
                        r["rst"],
                        r["valid"][lane],
                        r["take"][lane],
                        data[lane],
                    )
                    for lane in range(2)
                ]
                expected[name + "_" + policy + "_ready"] = tuple(x[0] for x in lanes)
                expected[name + "_" + policy + "_valid"] = tuple(x[1] for x in lanes)
                expected[name + "_" + policy + "_data"] = tuple(
                    p for x in lanes for p in x[2]
                )
        inputs.append(inp)
        gold.append(expected)

    final = work / "tokens.ac"
    final.write_text(final_contents)
    run(
        [
            args.optimizer,
            final,
            "--ac-verify-hardware",
            "-o",
            work / "verified.ac",
        ]
    )
    for target in ("cpp", "verilog"):
        run(
            [
                sys.executable,
                "-m",
                "pycircuit.cli",
                "emit",
                final,
                "--target",
                target,
                "-o",
                work / target,
            ]
        )
    cpp = ["constexpr unsigned row_count=" + str(len(rows)) + ";"]
    for group, frames in [("input", inputs), ("gold", gold)]:
        for name in frames[0]:
            values = [
                frame[name] if isinstance(frame[name][0], tuple) else (frame[name],)
                for frame in frames
            ]
            for lane in range(len(values[0])):
                for pindex, pname in enumerate(("value", "known", "z"), 1):
                    cpp.append(
                        f"const char *{group}_{name}_{lane}_{pname}[]={{"
                        + ",".join(
                            json.dumps(text(v[lane])[pindex - 1]) for v in values
                        )
                        + "};"
                    )
    cpp += ["void drivePorts(pyc_dut::Inputs &p,unsigned row){"]
    for name, value in inputs[0].items():
        parts = value if isinstance(value[0], tuple) else (value,)
        for lane, p in enumerate(parts):
            field = (
                f"p.{name}.element({lane})"
                if isinstance(value[0], tuple)
                else f"p.{name}"
            )
            cpp.append(
                f"{field}=std::remove_cvref_t<decltype({field})>::fromPacked(input<{p[0]}>(input_{name}_{lane}_value[row],input_{name}_{lane}_known[row],input_{name}_{lane}_z[row]).packed());"
            )
    cpp += [
        "}",
        "template<class Outputs>void checkPorts(const Outputs &p,unsigned row){",
    ]
    for name, parts in gold[0].items():
        for lane, p in enumerate(parts):
            cpp.append(
                f"check<{p[0]}>(p.{name}.element({lane}),gold_{name}_{lane}_value[row],gold_{name}_{lane}_known[row],gold_{name}_{lane}_z[row]);"
            )
    traces = [
        " ".join(
            name + ":" + ",".join(symbols(p) for p in parts)
            for name, parts in g.items()
        )
        for g in gold
    ]
    cpp += [
        "}",
        "const char *golden_trace[]={" + ",".join(json.dumps(t) for t in traces) + "};",
    ]
    (work / "queue_vectors.hpp").write_text("\n".join(cpp) + "\n")
    config = work / "config.json"
    config.write_text(
        json.dumps(
            {
                "schema": "pycircuit-model-config",
                "version": "1",
                "max_ticks": len(rows) + 16,
                "max_domain_cycles": {},
                "deadlock_window": None,
            },
            sort_keys=True,
            separators=(",", ":"),
        )
        + "\n"
    )
    receipt = json.loads((work / "cpp/generated.json").read_text())
    sources = [
        work / "cpp" / x["path"] for x in receipt["files"] if x["path"].endswith(".cpp")
    ]
    runtime = Path(args.emitter).resolve().parent.parent / "runtime/libpyc6_runtime.a"
    runner = work / "runner"
    run(
        [
            args.cxx,
            "-std=c++20",
            "-O1",
            "-pthread",
            "-I" + str(repo / "include"),
            "-I" + str(work / "cpp"),
            "-I" + str(work),
            fixtures / "queue-emission.cpp",
            *sources,
            runtime,
            "-o",
            runner,
        ]
    )
    observed = []
    for workers in (1, 2):
        observed.append(run([runner, "--workers", workers, "--config", config]).stdout)
    assert observed[0] == observed[1]

    # Freeze the same token/symbol expectations into the complete RTL testbench.
    # Native assertions retain latent values; RTL asserts every visible 0/1/X/Z bit.
    ports = []
    for name, value in inputs[0].items():
        if not isinstance(value[0], tuple):
            ports.append(f"logic {name}=" + ("1" if name == "rst" else "0") + ";")
        else:
            w = value[0][0]
            n = len(value)
            shape = "[0:1][0:2]" if n == 6 else "[0:1]"
            ports.append(f"logic {shape}[{w - 1}:0] {name}='0;")
    for name, parts in gold[0].items():
        n = len(parts)
        w = parts[0][0]
        shape = "[0:1][0:2]" if n == 6 else "[0:1]"
        ports.append(f"wire {shape}[{w - 1}:0] {name};")
    cold = [
        f'if({name}!==\'x)$fatal(1,"cold queue must remain X: {name}");'
        for name in gold[0]
    ]

    def rtl_rows(limit):
        body = []
        for index in range(limit):
            frame = inputs[index]
            for name, value in frame.items():
                if name in ("clk", "lane_clk"):
                    continue
                parts = value if isinstance(value[0], tuple) else (value,)
                for lane, p in enumerate(parts):
                    target = (
                        f"{name}[{lane // 3}][{lane % 3}]"
                        if len(parts) == 6
                        else f"{name}[{lane}]" if isinstance(value[0], tuple) else name
                    )
                    body.append(f"{target}={p[0]}'b{symbols(p)};")
            body.append("#1;")
            for name, parts in gold[index].items():
                for lane, p in enumerate(parts):
                    field = (
                        f"{name}[{lane // 3}][{lane % 3}]"
                        if len(parts) == 6
                        else f"{name}[{lane}]"
                    )
                    body.append(
                        f'if({field}!=={p[0]}\'b{symbols(p)})$fatal(1,"queue row{index} {name} lane{lane}");'
                    )
            body.append(
                f'$display("WORK {index} {traces[index]}");clk=1\'b{rows[index]["clk"]};'
            )
            if not independent:
                body.append("#1;")
            if independent:
                body.append(
                    f"lane_clk[0]=1'b{rows[index]['lane_clk'][0]};lane_clk[1]=1'b{rows[index]['lane_clk'][1]};#1;"
                )
        return body

    def macro(name, lines):
        return "`define " + name + " \\\n" + " \\\n".join(lines) + "\n"

    sv = macro("QUEUE_PORTS", ports) + macro("QUEUE_COLD", cold)
    sv += (
        "`define QUEUE_RESET_CLOCKS_HIGH "
        + ("lane_clk='1;" if independent else "")
        + "\n"
    )
    sv += (
        "`define QUEUE_RESET_CLOCKS_LOW "
        + ("lane_clk='0;" if independent else "")
        + "\n"
    )
    (work / "queue_vectors.svh").write_text(sv)
    (work / "queue_rows_four.svh").write_text("\n".join(rtl_rows(len(rows))) + "\n")
    (work / "queue_rows_known.svh").write_text("\n".join(rtl_rows(known_rows)) + "\n")
    rtl_receipt = json.loads((work / "verilog/generated.json").read_text())
    rtl = [
        work / "verilog" / r["path"] for r in rtl_receipt["files"] if r["role"] == "rtl"
    ]
    rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
    primitives = sorted((repo / "include/verilog").glob("*.v"))
    expected_lines = [f"WORK {i} {t}" for i, t in enumerate(traces)]
    run(
        [
            args.verilator,
            "--binary",
            "--timing",
            "--top-module",
            "tb",
            "--prefix",
            "Vqueue",
            "--Mdir",
            work / "rtl-build",
            "-j",
            "2",
            "-Wno-fatal",
            "-I" + str(work),
            *primitives,
            *rtl,
            fixtures / "queue-emission.sv",
        ]
    )
    known = run([work / "rtl-build/Vqueue"]).stdout
    assert [x for x in known.splitlines() if x.startswith("WORK ")] == expected_lines[
        :known_rows
    ]
    run(
        [
            args.iverilog,
            "-g2012",
            "-DQUEUE_FOUR_STATE",
            "-I" + str(work),
            "-s",
            "tb",
            "-o",
            work / "four.vvp",
            *primitives,
            *rtl,
            fixtures / "queue-emission.sv",
        ]
    )
    four = run([args.vvp, work / "four.vvp"]).stdout
    assert [x for x in four.splitlines() if x.startswith("WORK ")] == expected_lines
    (evidence / (label + ".json")).write_text(
        json.dumps(
            {
                "work": str(work),
                "rows": len(rows),
                "known_rows": known_rows,
                "trace_sha256": hashlib.sha256(observed[0].encode()).hexdigest(),
                "scope": "complete native1/2+Icarus+knownVerilator absolute-birth deque",
                "latencies": latencies,
                "independent_clocks": independent,
                "huge_latency_scope": "bounded early/reset only" if huge else None,
                "final_sha256": digest(final),
            },
            indent=2,
        )
        + "\n"
    )
    print("native token gate passed", len(rows), "rows")  # noqa: T201
    work = original_work


def delayed_fixture(latencies, independent=False, huge=False):
    """Build declared generic fixture ports, without computing DUT semantics."""
    base = (fixtures / "queue-emission.mlir").read_text().split("// -----")[0]
    root_start = base.index('  "ac.module"() ({\n  ^bb0(%clk:')
    prefix = base[:root_start]
    declarations = []
    values = {1, 2, 3, 5, *latencies.values()}
    for value in sorted(values):
        declarations.append(
            f'#lat{value} = #ac.static_expr<{{kind = "literal", location = {{path = "queue_emission.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}, origin = {{site = {{definition = @QueueTokens, ast_path = []}}, expansion = []}}, value = {{kind = "integer", value = #ac.math_int<{value}>}}}}>\n'
        )
    if huge:
        for owner in ("QueuePair", "QueueTablePair"):
            prefix = prefix.replace(
                "availability_latency = #latency_" + owner,
                "availability_latency = #lat18446744073709551615",
            )
    prefix = "".join(declarations) + prefix
    c = "!controls"
    selected = cases + [("bit13_second", 13, 1, 5)]
    if independent:
        selected += [("bit13_default", 13, 1, 3)]
    types = {
        "record_a": '!ac.struct<"RecordA">',
        "record_b": '!ac.struct<"RecordB">',
        "table130": "!b130",
        "table_record": '!ac.struct<"RecordA">',
    }
    in_names = ["clk", "rst", "valid", "take"]
    in_types = ["!b1", "!b1", c, c]
    if independent:
        in_names.append("lane_clk")
        in_types.append(c)
    lines, out_names, out_types = [], [], []
    if not independent:
        lines.append(
            '    %clocks = "ac.table.splat"(%clk) {shape = [#n2]} : (!b1) -> !controls'
        )
    lines.append(
        '    %resets = "ac.table.splat"(%rst) {shape = [#n2]} : (!b1) -> !controls'
    )
    for name, width, cardinality, depth in selected:
        depth = 1 if name == "bit1" else depth
        typ = types.get(name, f"!b{width}")
        dimensions = "#n2, #n3" if cardinality == 3 else "#n2"
        data = f"!ac.table<[{dimensions}], {typ}>"
        callee = "QueueTablePair" if cardinality == 3 else "QueueForward"
        parameters = f"[#lat{depth}, #lat{1 if huge else latencies[name]}]"
        if name == "bit13_default":
            callee, parameters = "QueueDefaultForward", "[]"
        in_names.append(name)
        in_types.append(data)
        names = [
            name + "_" + policy + "_" + port
            for policy in ("local", "pop")
            for port in ("ready", "valid", "data")
        ]
        outputs = [c, c, data, c, c, data]
        clock = "%lane_clk" if independent else "%clocks"
        lines.append(
            f'    {", ".join("%" + n for n in names)} = "ac.collection"(%valid, %{name}, %take, {clock}, %resets) {{instance_name = "{name}", callee = @{callee}, parameters = {parameters}, type_arguments = [{typ}], shape = [#n2], occurrence = {{site = {{definition = @QueueTokens, ast_path = [{{kind = "field", name = "{name}"}}]}}, expansion = []}}}} : ({c}, {data}, {c}, {c}, {c}) -> ({", ".join(outputs)})'
        )
        out_names.extend(names)
        out_types.extend(outputs)
    lines.append(
        f'    "ac.yield"({", ".join("%" + n for n in out_names)}) : ({", ".join(out_types)}) -> ()'
    )
    body = ", ".join(
        "%" + n + ": " + t for n, t in zip(in_names, in_types, strict=True)
    )
    attrs = f'sym_name = "QueueTokens", source_owner = {{package = "", path = "queue_emission.py"}}, parameters = [], type_parameters = [], function_type = ({", ".join(in_types)}) -> ({", ".join(out_types)}), input_names = {json.dumps(in_names)}, output_names = {json.dumps(out_names)}'
    return (
        prefix
        + '  "ac.module"() ({\n  ^bb0('
        + body
        + "):\n"
        + "\n".join(lines)
        + "\n  }) {"
        + attrs
        + "} : () -> ()\n"
        + '  "ac.system"() {entry = {callee = @QueueTokens, parameters = [], type_arguments = []}, domain = "default"} : () -> ()\n}\n'
    )


if args.only in ("all", "tokens"):
    original = (fixtures / "queue-emission.mlir").read_text().split("// -----")[0]
    token_gate("tokens", original, {name: 1 for name, *_ in cases})
latencies = {
    "bit1": 2,
    "bit13": 2,
    "bit13_second": 3,
    "bit13_default": 2,
    "bit65": 3,
    "bit130": 7,
    "record_a": 9,
    "record_b": 8,
    "table130": 3,
    "table_record": 4,
}
if args.only != "huge":
    token_gate(
        "delayed",
        delayed_fixture(latencies, independent=True),
        latencies,
        independent=True,
    )
huge = dict.fromkeys(
    (name for name in latencies if name != "bit13_default"), (1 << 64) - 1
)
token_gate("huge", delayed_fixture(huge, huge=True), huge, huge=True)
assert {
    str(p): digest(p) for p in bound_inputs
} == initial_hashes, "test/product input changed during gate"
(evidence / "receipt.json").write_text(
    json.dumps(
        {
            "role": "independent Q6-TG CodeGen tests",
            "configured_model": "gpt-6.1-sol",
            "configured_effort": "high",
            "served_model": "unavailable",
            "inputs": initial_hashes,
            "invocation": sys.argv,
            "work": str(work),
            "commands_sha256": digest(evidence / "commands.json"),
            "generated_receipts": {
                str(p.relative_to(work)): digest(p)
                for p in sorted(work.rglob("generated.json"))
            },
            "tools": {
                name: {
                    "path": str(Path(getattr(args, name)).resolve()),
                    "sha256": digest(getattr(args, name)),
                }
                for name in ("cxx", "iverilog", "vvp", "verilator")
            },
            "scope": "generated CodeGen only; huge latency bounded early/reset; near-wrap arithmetic stays in separately labelled Runtime/helper evidence; no Q6 acceptance claim",
        },
        indent=2,
    )
    + "\n"
)
