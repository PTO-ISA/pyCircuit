"""Owning FIFO RTL/native kernel vs independent absolute-age token oracles."""

import hashlib
import json
import subprocess
from pathlib import Path


def bit_and(a, b):
    return "0" if "0" in (a, b) else "1" if a == b == "1" else "x"


def bit_or(a, b):
    return "1" if "1" in (a, b) else "0" if a == b == "0" else "x"


class Frames:
    """Architectural token order and unbounded successful edge age, not RTL pointers."""

    def __init__(self, width, depth, policy, latency, managed=True, omitted=False):
        self.width, self.depth, self.policy, self.latency = (
            width,
            depth,
            policy,
            latency,
        )
        self.managed, self.omitted = managed, omitted
        self.queue = []
        self.epoch = 0
        self.clock = "0"
        self.initialized = False
        self.pending = None
        self.rows, self.labels = [], []
        self.pins = {
            "clk": "0",
            "rst": "0",
            "valid": "0",
            "take": "0",
            "permit": "1",
            "data": "0" * width,
        }

    def outputs(self):
        if not self.initialized:
            return "x", "x", "x" * self.width
        available = bool(self.queue and self.epoch >= self.queue[0][1])
        valid = "1" if available else "0"
        capacity = "1" if len(self.queue) < self.depth else "0"
        ready = (
            capacity
            if self.policy == 0
            else bit_or(capacity, bit_and(valid, self.pins["take"]))
        )
        return ready, valid, self.queue[0][0] if available else "0" * self.width

    def candidate(self, physical_edge=False, host_reset=False):
        clk, rst = self.pins["clk"], self.pins["rst"]
        if host_reset:
            return {"queue": [], "epoch": 0, "clock": "0", "initialized": True}
        if clk not in "01":
            return None
        next_state = {
            "queue": self.queue.copy(),
            "epoch": self.epoch,
            "clock": clk,
            "initialized": self.initialized,
        }
        rising = physical_edge or (self.clock == "0" and clk == "1")
        if not rising:
            return next_state
        if rst not in "01":
            return None
        if rst == "1":
            return {"queue": [], "epoch": 0, "clock": clk, "initialized": True}
        ready, valid, _ = self.outputs()
        push, pop = bit_and(self.pins["valid"], ready), bit_and(
            self.pins["take"], valid
        )
        if push not in "01" or pop not in "01":
            return None
        if not self.initialized:
            return next_state
        next_state["epoch"] += 1
        if pop == "1":
            next_state["queue"].pop(0)
        if push == "1":
            next_state["queue"].append(
                (self.pins["data"], next_state["epoch"] + self.latency - 1)
            )
        assert len(next_state["queue"]) <= self.depth
        return next_state

    def commit(self, candidate):
        self.queue = candidate["queue"]
        self.epoch = candidate["epoch"]
        self.clock = candidate["clock"]
        self.initialized = candidate["initialized"]

    def emit(self, command, label, **pins):
        previous_clock = self.pins["clk"]
        self.pins.update(pins)
        permission = "1" if self.omitted else self.pins["permit"]
        if self.managed:
            if command == 1:
                self.pending = self.candidate()
            elif command == 4:
                self.pending = self.candidate(host_reset=True)
            elif command == 2:
                if self.pending and permission in "1z":
                    self.commit(self.pending)
                self.pending = None
            elif command == 3:
                self.pending = None
            assert command in (0, 1, 2, 3, 4, 5)
            error = "0" if self.pending else "1"
        else:
            if previous_clock == "0" and self.pins["clk"] == "1":
                candidate = self.candidate(physical_edge=True)
                assert (
                    candidate is not None
                ), "invalid autonomous input belongs to a subprocess rejection gate"
                if permission in "1z":
                    self.commit(candidate)
            error = "0"
        ready, valid, data = self.outputs()
        values = [
            str(command),
            *(self.pins[k] for k in ("clk", "rst", "valid", "take", "permit", "data")),
            str(int(self.initialized)),
            ready,
            valid,
            data,
            error,
        ]
        self.rows.append(" ".join(values))
        self.labels.append(label)

    def attempt(self, label, decision=2, **pins):
        self.emit(0, label + "-idle", **pins)
        self.emit(1, label + "-prepare")
        self.emit(decision, label + "-decision")

    def rising(self, label, **pins):
        if self.managed:
            self.attempt(
                label + "-fall", clk="0", rst="0", valid="0", take="0", permit="1"
            )
            self.attempt(label, clk="1", **pins)
        else:
            self.emit(0, label + "-fall", clk="0", **pins)
            self.emit(0, label, clk="1")


def stimulus(width, depth, policy, latency, four_state, mode="managed", omitted=False):
    frames = Frames(width, depth, policy, latency, mode == "managed", omitted)

    def pattern(seed):
        return "".join("1" if (i * 3 + seed) % 7 < 3 else "0" for i in range(width))

    a, b, c = pattern(0), pattern(2), pattern(5)
    zero = "0" * width
    frames.emit(0, "cold")
    if frames.managed:
        # Cold known-zero effective transfers remain admissible even though
        # ready/valid are X. Unknown effective pushes/pops cannot be prepared.
        frames.attempt("cold-known-zero-fall", clk="0", rst="0", valid="0", take="0")
        frames.attempt(
            "cold-known-zero-effective-work", clk="1", rst="0", valid="0", take="0"
        )
        frames.attempt("cold-return-low", clk="0", rst="0", valid="0", take="0")
        if four_state:
            frames.attempt(
                "cold-unknown-push", decision=3, clk="1", rst="0", valid="x", take="0"
            )
            frames.attempt(
                "cold-unknown-pop", decision=3, clk="1", rst="0", valid="0", take="z"
            )
            frames.attempt(
                "cold-physical-reset-masks-unknown",
                decision=3,
                clk="1",
                rst="1",
                valid="x",
                take="z",
            )
        # Restore exact old cold inputs/state before the existing host-reset flow.
        frames.emit(
            0,
            "cold-restore-original-frame",
            clk="0",
            rst="0",
            valid="0",
            take="0",
            permit="1",
            data=zero,
        )
        frames.emit(2, "missing-prepare-no-replay")
        frames.emit(0, "host-reset-idle", clk="1", data=a)
        frames.emit(4, "host-reset-prepare")
        frames.emit(2, "host-reset-commit")
        frames.emit(0, "consumed-reset-idle")
        frames.emit(2, "consumed-reset-no-replay")
        # First high after host Reset rises despite unchanged physical high.
        frames.attempt("first-high", clk="1", rst="0", valid="1", take="1", data=a)
        frames.attempt("held-high", clk="1", rst="0", valid="1", take="1", data=b)
    else:
        frames.rising("autonomous-reset", rst="1", valid="0", take="0", data=a)
        frames.rising("empty-no-flow", rst="0", valid="1", take="1", data=a)
    if not frames.managed and latency > 1 and not omitted:
        # Denied physical edges must not age or mature a waiting head, even
        # without a token write. Read the exact first allowed maturity boundary.
        for i in range(latency + 4):
            frames.rising(
                f"autonomous-denied-waiting-age-{i}",
                rst="0",
                valid="0",
                take="0",
                permit="0",
                data=b,
            )
            assert frames.outputs()[1:] == ("0", zero)
        for i in range(latency - 1):
            frames.rising(
                f"autonomous-allowed-maturity-{i}",
                rst="0",
                valid="0",
                take="0",
                permit="1",
                data=b,
            )
            assert frames.outputs()[1] == ("1" if i == latency - 2 else "0")
        assert frames.outputs()[2] == a
    # Mature delayed head, then retain it through many timestamp wraps.
    for i in range(latency + 12):
        frames.rising(f"maturity-idle-{i}", rst="0", valid="0", take="0", data=b)
    for i in range(depth - 1):
        frames.rising(f"fill-{i}", rst="0", valid="1", take="0", data=b)
    for i in range(latency + 1):
        frames.rising(f"full-idle-{i}", rst="0", valid="0", take="0", data=b)
    if four_state and frames.managed:
        frames.attempt(
            "full-masked-controls-fall", clk="0", rst="0", valid="0", take="0"
        )
        frames.attempt(
            "full-masked-unknown-valid", clk="1", rst="0", valid="x", take="0"
        )
        frames.attempt("full-unknown-transfer-fall", clk="0", valid="0", take="0")
        # Local-occupancy masks the offered X when full; downstream-pop admits
        # capacity on the pop and must reject its effective unknown push.
        frames.attempt(
            "full-policy-sensitive-unknown-transfer",
            decision=3,
            clk="1",
            valid="x",
            take="1",
        )
    if frames.managed:
        # Full pop+push owns old head. Rejected candidates cannot replace tokens,
        # mature them, age the clock, or consume the full queue.
        frames.attempt("full-fall", clk="0", rst="0", valid="0", take="0")
        for i in range(5):
            frames.attempt(
                f"full-replacement-discard-{i}",
                decision=3,
                clk="1",
                valid="1",
                take="1",
                data=c,
            )
        frames.attempt(
            "full-replacement-denied", clk="1", valid="1", take="1", data=c, permit="0"
        )
        frames.emit(0, "denied-no-replay-idle", permit="1")
        frames.emit(2, "denied-no-replay")
        replacement_start = len(frames.rows)
        frames.attempt(
            "full-replacement-retry", clk="1", valid="1", take="1", data=c, permit="1"
        )
        # Observe accepted replacement C before any reset, then empty the queue
        # so the following frozen push has real capacity under BOTH policies.
        for i in range(depth + latency + 2):
            frames.rising(
                f"observe-accepted-replacement-{i}",
                rst="0",
                valid="0",
                take="1",
                data=b,
            )
        if policy == 1:
            assert any(
                row.split()[10] == c and row.split()[9] == "1"
                for row in frames.rows[replacement_start:]
            )
        assert not frames.queue
        frames.attempt("frozen-fall", clk="0", valid="0", take="0")
        frames.emit(
            0, "frozen-frame-idle", clk="1", rst="0", valid="1", take="0", data=a
        )
        frames.emit(1, "frozen-frame-prepare")
        frames.emit(
            5, "live-frame-mutation", clk="0", rst="1", valid="0", take="1", data=b
        )
        frames.emit(2, "frozen-frame-commit")
        frames.attempt(
            "frozen-high-held", clk="1", rst="0", valid="0", take="0", data=c
        )
        for i in range(latency + 1):
            frames.rising(
                f"observe-frozen-payload-{i}", rst="0", valid="0", take="0", data=c
            )
        assert frames.outputs()[1:] == ("1", a), "frozen A must be visible before reset"
        frames.attempt("discard-falling", decision=3, clk="0")
        frames.attempt("old-high-after-falling-discard", clk="1")
        # Ordinary physical Reset is subject to complete-update permission.
        frames.attempt("reset-fall", clk="0")
        frames.attempt("reset-denied", clk="1", rst="1", permit="0")
        frames.emit(0, "reset-denied-idle", permit="1")
        frames.emit(2, "reset-denied-cannot-replay")
        frames.attempt("reset-discard", decision=3, clk="1", rst="1")
        frames.attempt("reset-retry", clk="1", rst="1", permit="1")
    else:
        frames.rising("full-replacement", rst="0", valid="1", take="1", data=c)
        if not omitted:
            frames.rising(
                "denied-reset", rst="1", valid="0", take="0", permit="0", data=a
            )
            frames.rising(
                "denied-push", rst="0", valid="1", take="0", permit="0", data=b
            )
        frames.rising("allowed-reset", rst="1", valid="0", take="0", permit="1", data=c)
    # A new birth after wrapped absolute time exercises fresh deadlines.
    frames.rising("post-reset-birth", rst="0", valid="1", take="0", permit="1", data=c)
    if frames.managed:
        frames.attempt("delayed-fall", clk="0", valid="0", take="0")
        for i in range(5):
            frames.attempt(
                f"discard-idle-age-{i}", decision=3, clk="1", valid="0", take="0"
            )
        frames.attempt("idle-age-denied", clk="1", permit="0")
        frames.attempt("idle-age-retry", clk="1", permit="1")
    for i in range(latency + 8):
        frames.rising(
            f"post-birth-maturity-{i}", rst="0", valid="0", take="0", permit="1", data=a
        )
    if four_state and frames.managed:
        frames.attempt("known-low", clk="0", rst="0", valid="0", take="0")
        frames.attempt("unknown-clock-x", decision=3, clk="x")
        frames.attempt("unknown-clock-z", decision=3, clk="z")
        frames.attempt("unknown-reset", decision=3, clk="1", rst="x")
        frames.attempt("unknown-pop", decision=3, clk="1", rst="0", valid="0", take="x")
        frames.attempt("unknown-commit-permit", clk="1", take="0", permit="x")
        frames.attempt("tri1-z-commit", clk="1", permit="z")
        frames.attempt("held-unknown-controls", clk="1", rst="x", valid="z", take="x")
        frames.attempt("fall-unknown-controls", clk="0", rst="z", valid="x", take="z")
        frames.emit(0, "unknown-host-reset-idle", clk="x", rst="z", valid="x", take="z")
        frames.emit(4, "unknown-host-reset-prepare")
        frames.emit(2, "unknown-host-reset-commit", permit="1")
        # Empty out_ready X is harmless; selected payload X/Z must survive.
        xz = "".join("x" if i % 2 else "z" for i in range(width))
        frames.attempt("xz-birth", clk="1", rst="0", valid="1", take="x", data=xz)
        for i in range(latency + 2):
            frames.rising(
                f"xz-maturity-{i}", rst="0", valid="0", take="0", permit="1", data=zero
            )
        frames.rising("xz-pop", rst="0", valid="0", take="1", data=zero)
        frames.attempt("unknown-push-empty", clk="0", valid="0", take="0")
        frames.attempt("unknown-push-rising", decision=3, clk="1", valid="x", take="0")
        frames.attempt("known-control-retry", clk="1", valid="1", take="0", data=a)
    if four_state and not frames.managed:
        frames.rising(
            "autonomous-xz-reset", rst="1", valid="0", take="0", permit="1", data=zero
        )
        xz = "".join("x" if i % 2 else "z" for i in range(width))
        frames.rising(
            "autonomous-xz-birth", rst="0", valid="1", take="0", permit="1", data=xz
        )
        for i in range(latency + 2):
            frames.rising(
                f"autonomous-xz-maturity-{i}",
                rst="0",
                valid="0",
                take="0",
                permit="1",
                data=zero,
            )
        frames.rising(
            "autonomous-xz-pop", rst="0", valid="0", take="1", permit="1", data=zero
        )
    for i in range(depth + latency + 4):
        frames.rising(
            f"final-drain-{i}", rst="0", valid="0", take="1", permit="1", data=zero
        )
    if frames.managed:
        # Directed old-eligibility witness: demand cannot pop the token on the
        # very edge that first matures it. The following edge may consume it.
        frames.emit(
            0,
            "maturing-demand-reset-idle",
            clk="0",
            rst="0",
            valid="0",
            take="0",
            permit="1",
        )
        frames.emit(4, "maturing-demand-reset-prepare")
        frames.emit(2, "maturing-demand-reset-commit")
        frames.attempt(
            "maturing-demand-birth", clk="1", rst="0", valid="1", take="1", data=a
        )
        for i in range(latency - 1):
            frames.rising(
                f"first-maturing-edge-demand-{i}", rst="0", valid="0", take="1", data=b
            )
        assert frames.outputs()[1:] == ("1", a)
        frames.rising("after-maturity-demand-pop", rst="0", valid="0", take="1", data=b)
        assert not frames.queue
        if four_state and depth == 1 and latency == 3:
            frames.emit(
                0,
                "full-waiting-reset-idle",
                clk="0",
                rst="0",
                valid="0",
                take="0",
                permit="1",
            )
            frames.emit(4, "full-waiting-reset-prepare")
            frames.emit(2, "full-waiting-reset-commit")
            frames.attempt(
                "full-waiting-birth", clk="1", rst="0", valid="1", take="0", data=b
            )
            for i in range(latency - 1):
                frames.attempt(
                    f"full-waiting-masked-fall-{i}",
                    clk="0",
                    rst="0",
                    valid="0",
                    take="0",
                )
                # Both effective transfers read OLD invalid/ready0, even on the
                # first maturity edge. X demand/offering therefore stay masked.
                frames.attempt(
                    f"full-waiting-masked-x-transfer-{i}",
                    clk="1",
                    rst="0",
                    valid="x",
                    take="x",
                )
            assert frames.outputs()[1:] == ("1", b)
    assert len(frames.rows) >= 32
    return frames


def run_fifo(args):
    repo, scratch = Path(args.repo).resolve(), Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    base = Path(__file__).resolve().parent
    bench, native = base / "managed-fifo.sv", base / "managed-fifo.cpp"
    rtl = repo / "include/verilog/fifo.v"
    inputs = [
        bench,
        native,
        Path(__file__).resolve(),
        rtl,
        repo / "include/gfsim/fifo.h",
    ]
    hashes = {
        str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs
    }
    commands, cases = [], []

    def run(command, label, rejected=False, marker=None):
        result = subprocess.run(
            list(map(str, command)),
            cwd=repo,
            capture_output=True,
            text=True,
            timeout=180,
        )
        row = {
            "label": label,
            "command": list(map(str, command)),
            "exit_status": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
        commands.append(row)
        (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert (result.returncode != 0) if rejected else (result.returncode == 0), row
        if marker:
            assert marker in result.stdout + result.stderr, row
        return result

    def rows(text):
        return [line for line in text.splitlines() if line.startswith("ROW ")]

    for command in (
        [args.iverilog, "-V"],
        [args.vvp, "-V"],
        [args.verilator, "--version"],
        [args.cxx, "--version"],
    ):
        run(command, "version")
    configurations = [
        (13, depth, policy, latency, False)
        for depth in (1, 3)
        for policy in (0, 1)
        for latency in (1, 3)
    ]
    configurations += [(65, 3, 1, 3, False), (13, 3, 0, 1, True), (65, 3, 1, 3, True)]
    for width, depth, policy, latency, structured in configurations:
        name = f"fifo-w{width}-d{depth}-p{policy}-l{latency}" + (
            "-struct" if structured else ""
        )
        defines = ["-DSTRUCT_PAYLOAD"] if structured else []
        params = {
            "WIDTH": width,
            "DEPTH": depth,
            "POLICY": policy,
            "LATENCY": latency,
            "MANAGED": 1,
        }
        native_runner = scratch / (name + "-native")
        run(
            [
                args.cxx,
                "-std=c++20",
                "-O0",
                "-DFIFO_WIDTH=" + str(width),
                "-DFIFO_DEPTH=" + str(depth),
                "-DFIFO_POLICY=" + str(policy),
                "-DFIFO_LATENCY=" + str(latency),
                "-I" + str(repo / "include"),
                native,
                "-o",
                native_runner,
            ],
            name + "-native-build",
        )
        traces = {}
        for four_state, engine in ((True, "icarus"), (False, "verilator")):
            vectors = stimulus(width, depth, policy, latency, four_state)
            path = scratch / (name + "-" + engine + ".txt")
            path.write_text("\n".join(vectors.rows) + "\n")
            (scratch / (name + "-" + engine + "-labels.json")).write_text(
                json.dumps(vectors.labels) + "\n"
            )
            reference = run([native_runner, path], name + "-" + engine + "-native")
            if engine == "icarus":
                executable = scratch / (name + ".vvp")
                parameters = [
                    item
                    for key, value in params.items()
                    for item in ("-P", f"tb_fifo.{key}={value}")
                ]
                run(
                    [
                        args.iverilog,
                        "-g2012",
                        "-s",
                        "tb_fifo",
                        *parameters,
                        *defines,
                        "-o",
                        executable,
                        bench,
                        rtl,
                    ],
                    name + "-icarus-build",
                )
                result = run(
                    [args.vvp, executable, "+VECTORS=" + str(path)],
                    name + "-icarus-run",
                    marker="PASS FIFO",
                )
            else:
                directory = scratch / (name + "-verilator")
                parameters = [f"-G{key}={value}" for key, value in params.items()]
                run(
                    [
                        args.verilator,
                        "--binary",
                        "--timing",
                        "-Wno-fatal",
                        "--top-module",
                        "tb_fifo",
                        "--Mdir",
                        directory,
                        *parameters,
                        *defines,
                        bench,
                        rtl,
                    ],
                    name + "-verilator-build",
                )
                result = run(
                    [directory / "Vtb_fifo", "+VECTORS=" + str(path)],
                    name + "-verilator-run",
                    marker="PASS FIFO",
                )
            assert rows(result.stdout) == rows(reference.stdout), (
                name,
                engine,
                "owning RTL/native trace mismatch",
            )
            traces[engine] = len(vectors.rows)
        cases.append(
            {
                "case": name,
                "managed": True,
                "architectural_absolute_age_oracle": True,
                "native_kernel": True,
                "rows": traces,
            }
        )

    for synthesis in (False, True):
        for policy, latency in ((0, 1), (1, 3)):
            for omitted in (False, True):
                name = f"fifo-{'synthesis' if synthesis else 'autonomous'}-p{policy}-l{latency}-{'omitted' if omitted else 'permit'}"
                params = {
                    "WIDTH": 13,
                    "DEPTH": 3,
                    "POLICY": policy,
                    "LATENCY": latency,
                    "MANAGED": 1 if synthesis else 0,
                    "OMIT_PRIVATE": int(omitted),
                }
                defines = ["-DSYNTHESIS"] if synthesis else []
                for engine in ("icarus", "verilator"):
                    vectors = stimulus(
                        13,
                        3,
                        policy,
                        latency,
                        engine == "icarus" and not synthesis,
                        mode="autonomous",
                        omitted=omitted,
                    )
                    path = scratch / (name + "-" + engine + ".txt")
                    path.write_text("\n".join(vectors.rows) + "\n")
                    if engine == "icarus":
                        executable = scratch / (name + ".vvp")
                        parameters = [
                            item
                            for key, value in params.items()
                            for item in ("-P", f"tb_fifo.{key}={value}")
                        ]
                        run(
                            [
                                args.iverilog,
                                "-g2012",
                                "-s",
                                "tb_fifo",
                                *parameters,
                                *defines,
                                "-o",
                                executable,
                                bench,
                                rtl,
                            ],
                            name + "-icarus-build",
                        )
                        run(
                            [args.vvp, executable, "+VECTORS=" + str(path)],
                            name + "-icarus-run",
                            marker="PASS FIFO",
                        )
                        if not synthesis and not omitted:
                            run(
                                [args.vvp, executable, "+BAD_AUTO_RESET"],
                                name + "-bad-reset",
                                rejected=True,
                                marker="fifo: reset must be known",
                            )
                            run(
                                [args.vvp, executable, "+BAD_AUTO_TRANSFERS"],
                                name + "-bad-transfers",
                                rejected=True,
                                marker="fifo: effective transfers must be known",
                            )
                    else:
                        directory = scratch / (name + "-verilator")
                        parameters = [
                            f"-G{key}={value}" for key, value in params.items()
                        ]
                        run(
                            [
                                args.verilator,
                                "--binary",
                                "--timing",
                                "-Wno-fatal",
                                "--top-module",
                                "tb_fifo",
                                "--Mdir",
                                directory,
                                *parameters,
                                *defines,
                                bench,
                                rtl,
                            ],
                            name + "-verilator-build",
                        )
                        run(
                            [directory / "Vtb_fifo", "+VECTORS=" + str(path)],
                            name + "-verilator-run",
                            marker="PASS FIFO",
                        )
                cases.append(
                    {
                        "case": name,
                        "managed": False,
                        "synthesis_projection": synthesis,
                        "omitted_private_pins": omitted,
                    }
                )

    executable = scratch / "fifo-two-state.vvp"
    run(
        [
            args.iverilog,
            "-g2012",
            "-DTWO_STATE_PAYLOAD",
            "-s",
            "tb_fifo",
            "-o",
            executable,
            bench,
            rtl,
        ],
        "two-state-build",
    )
    run(
        [args.vvp, executable],
        "two-state-rejected",
        rejected=True,
        marker="fifo: T must preserve four-state",
    )
    for parameter, value, diagnostic in (
        ("DEPTH", 0, "fifo: depth must be positive"),
        ("LATENCY", 0, "fifo: latency must be positive"),
        ("POLICY", 2, "fifo: unsupported ready policy"),
    ):
        executable = scratch / ("fifo-invalid-" + parameter + ".vvp")
        run(
            [
                args.iverilog,
                "-g2012",
                "-s",
                "tb_fifo",
                "-P",
                f"tb_fifo.{parameter}={value}",
                "-o",
                executable,
                bench,
                rtl,
            ],
            "invalid-" + parameter + "-build",
        )
        run(
            [args.vvp, executable],
            "invalid-" + parameter,
            rejected=True,
            marker=diagnostic,
        )
    assert {
        str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs
    } == hashes, "FIFO candidate drift"
    (scratch / "results.json").write_text(
        json.dumps(
            {
                "scope": "T2 FIFO managed/autonomous/native/four-state/synthesis",
                "input_sha256": hashes,
                "cases": cases,
                "remaining_cases": ["byte_mem", "sync_mem", "all"],
                "historical_roots_closed": 0,
            },
            indent=2,
        )
        + "\n"
    )
    print(
        "PASS T2 FIFO: owning RTL, native kernel, absolute-age oracle, four-state and synthesis; memory cases unfinished"
    )  # noqa: T201 - standalone gate receipt
