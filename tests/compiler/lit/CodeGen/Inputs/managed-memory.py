"""Independent scalar memory and read-age oracles against owning RTL/kernels."""

import argparse
import hashlib
import json
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path


def known(word):
    return all(bit in "01" for bit in word)


class MemoryFrames:
    def __init__(self, kind, width, address, depth, managed=True, synthesis=False):
        self.kind, self.width, self.address, self.depth = kind, width, address, depth
        self.ports = 2 if kind == 2 else 1
        self.strobes = width // 8 if kind == 0 else (width + 7) // 8
        self.managed, self.synthesis = managed, synthesis
        cell_width = 8 if kind == 0 else width
        self.memory = [("x" if synthesis else "0") * cell_width for _ in range(depth)]
        self.q = ["x" * width, "x" * width if kind == 2 else "0" * width]
        self.last_read = [None, None]
        self.edge = 0
        self.clock = "0"
        self.pending = None
        self.rows, self.labels = [], []
        self.pins = {
            "clk": "0",
            "rst": "0",
            "write": "0",
            "ren0": "0",
            "ren1": "0",
            "r0": self.number(0, address),
            "r1": self.number(0, address),
            "wa": self.number(0, address),
            "data": self.number(0, width),
            "strobe": self.number(0, self.strobes),
            "permit": "1",
        }

    @staticmethod
    def number(value, width):
        assert 0 <= value < (1 << width)
        return format(value, f"0{width}b")

    def read(self, address):
        if not known(address):
            return "x" * self.width
        index = int(address, 2)
        if self.kind:
            return self.memory[index] if index < self.depth else "0" * self.width
        return "".join(
            self.memory[index + lane] if index + lane < self.depth else "0" * 8
            for lane in reversed(range(self.strobes))
        )

    def outputs(self):
        return (
            [self.read(self.pins["r0"]), "0" * self.width] if self.kind == 0 else self.q
        )

    def candidate(self, physical=False, host_reset=False):
        pins = self.pins
        candidate = {
            "writes": [],
            "q": self.q.copy(),
            "last": self.last_read.copy(),
            "edge": self.edge,
            "clock": "0" if host_reset else pins["clk"],
        }
        if host_reset:
            if self.kind:
                candidate["q"][: self.ports] = ["x" * self.width] * self.ports
                candidate["last"] = [None, None]
            return candidate
        # Continuous byte reads validate even during held/falling/reset samples.
        if self.kind == 0 and not known(pins["r0"]):
            return None
        if pins["clk"] not in "01":
            return None
        rising = physical or self.clock == "0" and pins["clk"] == "1"
        if not rising:
            return candidate
        if pins["rst"] not in "01":
            return None
        candidate["edge"] += 1
        if pins["rst"] == "1":
            if self.kind:
                candidate["q"][: self.ports] = [
                    ("0" if self.synthesis else "x") * self.width
                ] * self.ports
                candidate["last"] = [None, None]
            return candidate
        controls = [pins["write"]] + (
            [pins[f"ren{port}"] for port in range(self.ports)] if self.kind else []
        )
        if not all(value in "01" for value in controls):
            return None
        if pins["write"] == "1" and not all(
            known(pins[key]) for key in ("wa", "data", "strobe")
        ):
            return None
        for port in range(self.ports if self.kind else 0):
            if pins[f"ren{port}"] == "1":
                if not known(pins[f"r{port}"]):
                    return None
                candidate["q"][port] = self.read(pins[f"r{port}"])
                candidate["last"][port] = candidate["edge"]
            elif (
                not self.synthesis
                and self.last_read[port] is not None
                and candidate["edge"] >= self.last_read[port] + 2
            ):
                candidate["q"][port] = "x" * self.width
                candidate["last"][port] = None
        if pins["write"] == "1":
            address = int(pins["wa"], 2)
            strobes = int(pins["strobe"], 2)
            if self.kind == 0:
                for lane in range(self.strobes):
                    if address + lane < self.depth and (strobes >> lane) & 1:
                        last = self.width - 8 * lane
                        candidate["writes"].append(
                            (address + lane, pins["data"][last - 8 : last])
                        )
            elif address < self.depth:
                merged = "".join(
                    new if (strobes >> ((self.width - bit - 1) // 8)) & 1 else old
                    for bit, (old, new) in enumerate(
                        zip(self.memory[address], pins["data"], strict=True)
                    )
                )
                candidate["writes"].append((address, merged))
        return candidate

    def commit(self, candidate):
        for address, word in candidate["writes"]:
            self.memory[address] = word
        self.q, self.last_read = candidate["q"], candidate["last"]
        self.edge, self.clock = candidate["edge"], candidate["clock"]

    def emit(self, command, label, **pins):
        previous = self.pins["clk"]
        for key, value in pins.items():
            if isinstance(value, int):
                width = (
                    self.address
                    if key in {"r0", "r1", "wa"}
                    else self.width if key == "data" else self.strobes
                )
                value = self.number(value, width)
            self.pins[key] = value
        if self.managed:
            if command == 1:
                self.pending = self.candidate()
            elif command == 4:
                self.pending = self.candidate(host_reset=True)
            elif command == 2:
                if self.pending and self.pins["permit"] in "1z":
                    self.commit(self.pending)
                self.pending = None
            elif command == 3:
                self.pending = None
            assert command in (0, 1, 2, 3, 4, 5)
            error = "0" if self.pending else "1"
        else:
            if previous == "0" and self.pins["clk"] == "1":
                candidate = self.candidate(physical=True)
                assert candidate is not None
                if self.pins["permit"] in "1z":
                    self.commit(candidate)
            error = "0"
        q0, q1 = self.outputs()
        fields = [
            str(command),
            *(
                self.pins[key]
                for key in (
                    "clk",
                    "rst",
                    "write",
                    "ren0",
                    "ren1",
                    "r0",
                    "r1",
                    "wa",
                    "data",
                    "strobe",
                    "permit",
                )
            ),
            q0,
            q1,
            str(int(known(q0))),
            str(int(known(q1))),
            error,
        ]
        self.rows.append(" ".join(fields))
        self.labels.append(label)

    def attempt(self, label, decision=2, **pins):
        self.emit(0, label + "-idle", **pins)
        self.emit(1, label + "-prepare")
        self.emit(decision, label + "-decision")

    def rise(self, label, **pins):
        if self.managed:
            self.attempt(
                label + "-fall",
                clk="0",
                rst="0",
                write="0",
                ren0="0",
                ren1="0",
                permit="1",
            )
            self.attempt(label, clk="1", **pins)
        else:
            self.emit(0, label + "-fall", clk="0", **pins)
            self.emit(0, label, clk="1")


def stimulus(kind, width, address, depth, four_state, managed=True, synthesis=False):
    frames = MemoryFrames(kind, width, address, depth, managed, synthesis)

    def pattern(seed):
        return int(
            "".join("1" if (bit * 3 + seed) % 7 < 3 else "0" for bit in range(width)), 2
        )

    a, b, c = pattern(0), pattern(2), pattern(5)
    full = (1 << frames.strobes) - 1
    high = (1 << (address - 1)) | 1
    zero_addr = frames.number(0, address)
    bad_addr = "x" + zero_addr[1:]
    outside_unknown_addr = "1" + "0" * (address - 2) + "x"
    bad_data, bad_strobe = "x" * width, "z" * frames.strobes
    frames.emit(0, "initial")
    if managed:
        frames.emit(2, "missing-prepare")
        frames.emit(0, "host-reset-idle", clk="1")
        frames.emit(4, "host-reset-prepare")
        frames.emit(2, "host-reset-commit")
    else:
        frames.rise("physical-reset", rst="1", write="0", ren0="0", ren1="0")
    # Fully seed synthesis RAM because its projection has no initialization loop.
    if synthesis:
        for entry in range(depth):
            frames.rise(
                f"synthesis-seed-{entry}",
                rst="0",
                write="1",
                ren0="0",
                ren1="0",
                wa=entry,
                data=a,
                strobe=full,
            )
    frames.rise(
        "old-data-collision",
        rst="0",
        write="1",
        ren0="1",
        ren1="1",
        r0=0,
        r1=1,
        wa=0,
        data=a,
        strobe=full,
    )
    frames.rise("read-written-data", rst="0", write="0", ren0="1", ren1="1", r0=0, r1=0)
    # Low-byte/partial-tail masks and unaligned byte addressing.
    frames.rise(
        "masked-unaligned-write",
        rst="0",
        write="1",
        ren0="1",
        ren1="1",
        r0=1,
        r1=0,
        wa=1,
        data=b,
        strobe=1,
    )
    frames.rise(
        "last-tail-strobe",
        rst="0",
        write="1",
        ren0="1",
        ren1="1",
        r0=1,
        r1=0,
        wa=1,
        data=c,
        strobe=1 << (frames.strobes - 1),
    )
    frames.rise("observe-masks", rst="0", write="0", ren0="1", ren1="1", r0=1, r1=0)
    frames.rise(
        "boundary-write",
        rst="0",
        write="1",
        ren0="1",
        ren1="1",
        r0=depth - 1,
        r1=0,
        wa=depth - 1,
        data=a,
        strobe=full,
    )
    frames.rise(
        "observe-boundary", rst="0", write="0", ren0="1", ren1="1", r0=depth - 1, r1=0
    )
    frames.rise(
        "full-width-outside-write",
        rst="0",
        write="1",
        ren0="1",
        ren1="1",
        r0=high,
        r1=high,
        wa=high,
        data=c,
        strobe=full,
    )
    frames.rise(
        "outside-not-low-alias", rst="0", write="0", ren0="1", ren1="1", r0=1, r1=0
    )
    if managed:
        frames.attempt(
            "candidate-fall", clk="0", rst="0", write="0", ren0="0", ren1="0"
        )
        for rejected in range(5):
            frames.attempt(
                f"discard-write-and-reads-{rejected}",
                decision=3,
                clk="1",
                rst="0",
                write="1",
                ren0="1",
                ren1="1",
                r0=0,
                r1=1,
                wa=0,
                data=b,
                strobe=full,
            )
        frames.attempt("permission-denied-write-read", clk="1", permit="0")
        frames.emit(0, "denied-replay-idle", permit="1")
        frames.emit(2, "denied-replay")
        frames.attempt(
            "same-high-clock-retry",
            clk="1",
            rst="0",
            write="1",
            ren0="1",
            ren1="1",
            r0=0,
            r1=1,
            wa=0,
            data=b,
            strobe=full,
            permit="1",
        )
        frames.rise("observe-retry", rst="0", write="0", ren0="1", ren1="1", r0=0, r1=1)
        frames.attempt("frozen-fall", clk="0", write="0", ren0="0", ren1="0")
        frames.emit(
            0,
            "frozen-idle",
            clk="1",
            rst="0",
            write="1",
            ren0="1",
            ren1="1",
            r0=0,
            r1=1,
            wa=2,
            data=a,
            strobe=full,
        )
        frames.emit(1, "frozen-prepare")
        frames.emit(
            5,
            "live-input-mutation",
            clk="0",
            rst="1",
            write="0",
            ren0="0",
            ren1="0",
            r0=2,
            r1=2,
            wa=0,
            data=c,
            strobe=0,
        )
        frames.emit(2, "frozen-commit")
        frames.attempt(
            "held-high-after-frozen-clock",
            clk="1",
            rst="0",
            write="0",
            ren0="0",
            ren1="0",
        )
        frames.rise(
            "observe-frozen-write", rst="0", write="0", ren0="1", ren1="1", r0=2, r1=0
        )
    # Successful-edge age is independent for each read port and never changes
    # on held/falling Work or failed/denied idle candidates.
    frames.rise("fresh-both-reads", rst="0", write="0", ren0="1", ren1="1", r0=0, r1=1)
    if managed:
        frames.attempt(
            "idle-discard-fall", clk="0", rst="0", write="0", ren0="0", ren1="0"
        )
        for rejected in range(5):
            frames.attempt(
                f"discard-idle-lifetime-{rejected}",
                decision=3,
                clk="1",
                write="0",
                ren0="0",
                ren1="0",
            )
        frames.attempt("denied-idle-lifetime", clk="1", permit="0")
        frames.attempt("retry-idle-lifetime", clk="1", permit="1")
        frames.attempt("held-idle-no-age", clk="1", write="0", ren0="0", ren1="0")
        frames.attempt("fall-idle-no-age", clk="0")
    else:
        frames.rise(
            "denied-idle-lifetime", rst="0", write="0", ren0="0", ren1="0", permit="0"
        )
    frames.rise(
        "idle-first-or-second", rst="0", write="0", ren0="0", ren1="1", permit="1"
    )
    frames.rise(
        "idle-second-or-third", rst="0", write="0", ren0="0", ren1="1", permit="1"
    )
    for idle in range(3):
        frames.rise(
            f"idle-both-{idle}", rst="0", write="0", ren0="0", ren1="0", permit="1"
        )
    frames.rise(
        "read-lifetime-recovery",
        rst="0",
        write="0",
        ren0="1",
        ren1="1",
        r0=0,
        r1=2,
        permit="1",
    )
    if not managed:
        # Establish three distinguishable locations and old read outputs, then
        # deny a reset0 edge with BOTH write and read work actively requested.
        d = ((1 << width) - 1) ^ a
        assert len({a, b, c, d}) == 4
        locations = (0, 2, 4) if kind == 0 else (0, 1, 2)
        for slot, value in zip(locations, (a, b, c), strict=True):
            frames.rise(
                f"active-permit-seed-{slot}",
                rst="0",
                write="1",
                ren0="0",
                ren1="0",
                wa=slot,
                data=value,
                strobe=full,
                permit="1",
            )
        frames.rise(
            "active-permit-old-q",
            rst="0",
            write="0",
            ren0="1",
            ren1="1",
            r0=locations[0],
            r1=locations[1],
            permit="1",
        )

        def word(value):
            return frames.number(value, width)

        assert frames.outputs()[0] == word(a)
        if kind == 2:
            assert frames.outputs()[1] == word(b)
        target = locations[1] if kind == 0 else locations[2]
        old_target = word(b if kind == 0 else c)
        frames.rise(
            "active-write-read-permit-denied",
            rst="0",
            write="1",
            ren0="1",
            ren1="1",
            r0=locations[1],
            r1=locations[2],
            wa=target,
            data=d,
            strobe=full,
            permit="0",
        )
        assert frames.read(frames.number(target, address)) == old_target
        assert frames.outputs()[0] == word(b if kind == 0 else a)
        if kind == 2:
            assert frames.outputs()[1] == word(b)
        # Byte reads stay continuously active under permit0 at the new address;
        # synchronous Q holds independently of the new requested read addresses.
        frames.emit(
            0,
            "active-denial-before-reset-ram-probe",
            clk="0",
            write="0",
            ren0="0",
            ren1="0",
            r0=target,
            permit="0",
        )
        assert frames.read(frames.number(target, address)) == old_target
        assert frames.outputs()[0] == (old_target if kind == 0 else word(a))
        frames.rise(
            "active-write-read-permit-retry",
            rst="0",
            write="1",
            ren0="1",
            ren1="1",
            r0=locations[1],
            r1=locations[2],
            wa=target,
            data=d,
            strobe=full,
            permit="1",
        )
        assert frames.read(frames.number(target, address)) == word(d)
        assert frames.outputs()[0] == word(d if kind == 0 else b)
        if kind == 2:
            assert frames.outputs()[1] == word(
                c
            ), "dual read must sample old C before the simultaneous D write"
        frames.rise(
            "active-retry-observe-new-ram",
            rst="0",
            write="0",
            ren0="1",
            ren1="1",
            r0=target,
            r1=locations[0],
            permit="1",
        )
        assert frames.outputs()[0] == word(d)
        if kind == 2:
            assert frames.outputs()[1] == word(a)
    if managed:
        frames.attempt(
            "reset-candidate-fall", clk="0", rst="0", write="0", ren0="0", ren1="0"
        )
        frames.attempt("physical-reset-discard", decision=3, clk="1", rst="1")
        frames.attempt("physical-reset-denied", clk="1", rst="1", permit="0")
        frames.emit(0, "physical-reset-denied-idle", permit="1")
        frames.emit(2, "physical-reset-no-replay")
        frames.attempt("physical-reset-retry", clk="1", rst="1", permit="1")
        frames.emit(0, "host-reset-retains-idle", clk="1")
        frames.emit(4, "host-reset-retains-prepare")
        frames.emit(2, "host-reset-retains-commit")
    else:
        frames.rise(
            "physical-reset-denied", rst="1", write="0", ren0="0", ren1="0", permit="0"
        )
        frames.rise(
            "physical-reset-retry", rst="1", write="0", ren0="0", ren1="0", permit="1"
        )
    frames.rise(
        "read-retained-content",
        rst="0",
        write="0",
        ren0="1",
        ren1="1",
        r0=0,
        r1=2,
        permit="1",
    )
    if four_state and managed:
        frames.attempt(
            "four-state-low",
            clk="0",
            rst="0",
            write="0",
            ren0="0",
            ren1="0",
            r0=0,
            r1=0,
        )
        frames.attempt("unknown-clock-x", decision=3, clk="x")
        frames.attempt("unknown-clock-z", decision=3, clk="z")
        frames.attempt("unknown-rising-reset", decision=3, clk="1", rst="x")
        frames.attempt("unknown-write-control", decision=3, clk="1", rst="0", write="z")
        frames.attempt(
            "unknown-enabled-data-zero-mask-outside",
            decision=3,
            clk="1",
            write="1",
            wa=high,
            data=bad_data,
            strobe=0,
        )
        frames.attempt(
            "z-enabled-data-zero-mask-outside",
            decision=3,
            clk="1",
            write="1",
            wa=high,
            data="z" * width,
            strobe=0,
        )
        frames.attempt(
            "unknown-enabled-address-zero-mask",
            decision=3,
            clk="1",
            wa=outside_unknown_addr,
            data=a,
            strobe=0,
        )
        frames.attempt(
            "unknown-enabled-strobes-outside",
            decision=3,
            clk="1",
            wa=high,
            data=a,
            strobe=bad_strobe,
        )
        if kind:
            frames.attempt(
                "unknown-read-enable",
                decision=3,
                clk="1",
                write="0",
                ren0="x",
                ren1="0",
            )
            frames.attempt(
                "unknown-enabled-read-address",
                decision=3,
                clk="1",
                ren0="1",
                r0=outside_unknown_addr,
            )
            if kind == 2:
                frames.attempt(
                    "unknown-second-read-enable",
                    decision=3,
                    clk="1",
                    write="0",
                    ren0="1",
                    ren1="z",
                    r0=0,
                    r1=1,
                )
                frames.attempt(
                    "unknown-port1-atomic-discard",
                    decision=3,
                    clk="1",
                    write="1",
                    ren0="1",
                    ren1="1",
                    r0=0,
                    r1=bad_addr,
                    wa=0,
                    data=c,
                    strobe=full,
                )
        else:
            frames.attempt(
                "continuous-unknown-read-low",
                decision=3,
                clk="0",
                write="0",
                r0=bad_addr,
            )
            frames.attempt(
                "continuous-unknown-read-reset",
                decision=3,
                clk="1",
                rst="1",
                r0=bad_addr,
            )
        frames.attempt(
            "disabled-fields-rising",
            clk="1",
            rst="0",
            write="0",
            ren0="0",
            ren1="0",
            r0=0 if kind == 0 else bad_addr,
            r1=bad_addr,
            wa=bad_addr,
            data=bad_data,
            strobe=bad_strobe,
            permit="1",
        )
        if kind == 0:
            frames.attempt(
                "continuous-unknown-read-held", decision=3, clk="1", r0=bad_addr
            )
            frames.attempt(
                "continuous-unknown-read-falling", decision=3, clk="0", r0=bad_addr
            )
            frames.attempt(
                "continuous-known-read-fall-retry", clk="0", r0=0, rst="0", write="0"
            )
            frames.attempt(
                "continuous-known-read-rise", clk="1", r0=0, rst="0", write="0"
            )
        frames.attempt(
            "held-unknown-controls", clk="1", rst="z", write="x", ren0="z", ren1="x"
        )
        frames.attempt(
            "fall-unknown-controls", clk="0", rst="x", write="z", ren0="x", ren1="z"
        )
        frames.emit(
            0,
            "host-reset-unknown-inputs-idle",
            clk="x",
            rst="z",
            write="x",
            ren0="x",
            ren1="z",
            r0=bad_addr,
            r1=bad_addr,
        )
        frames.emit(4, "host-reset-unknown-inputs-prepare")
        frames.emit(2, "host-reset-unknown-inputs-commit")
        frames.attempt(
            "physical-reset-masks-unknown",
            clk="1",
            rst="1",
            r0=0,
            ren0="x",
            ren1="z",
            write="x",
            permit="1",
        )
        frames.attempt(
            "permit-test-fall",
            clk="0",
            rst="0",
            write="0",
            ren0="0",
            ren1="0",
            r0=0,
            r1=0,
        )
        frames.attempt(
            "unknown-permit-denies",
            clk="1",
            rst="0",
            write="1",
            ren0="1",
            ren1="1",
            r0=0,
            r1=2,
            wa=0,
            data=a,
            strobe=full,
            permit="x",
        )
        frames.attempt("tri1-z-permit", clk="1", permit="z")
        frames.rise(
            "observe-z-permitted-write",
            rst="0",
            write="0",
            ren0="1",
            ren1="1",
            r0=0,
            r1=2,
            permit="1",
        )
    assert len(frames.rows) >= 32
    return frames


def run_memory(args):
    repo, scratch = Path(args.repo).resolve(), Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    base = Path(__file__).resolve().parent
    bench, native = base / "managed-memory.sv", base / "managed-memory.cpp"
    kinds = [0] if args.case == "byte_mem" else [1, 2]
    leaf_names = {0: "byte_mem", 1: "sync_mem", 2: "sync_mem_dp"}
    leaves = [repo / "include/verilog" / (name + ".v") for name in leaf_names.values()]
    inputs = [
        bench,
        native,
        Path(__file__).resolve(),
        *leaves,
        repo / "include/gfsim/byte_mem.h",
        repo / "include/gfsim/sync_mem.h",
    ]
    before = {
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

    def trace(text):
        return [line for line in text.splitlines() if line.startswith("ROW ")]

    def engines(
        name, params, defines, vectors_by_engine, native_runner=None, synthesis=False
    ):
        receipts = {}
        for engine in ("icarus", "verilator"):
            vectors = vectors_by_engine[engine]
            path = scratch / (name + "-" + engine + ".txt")
            path.write_text("\n".join(vectors.rows) + "\n")
            (scratch / (name + "-" + engine + "-labels.json")).write_text(
                json.dumps(vectors.labels) + "\n"
            )
            reference = (
                run([native_runner, path], name + "-" + engine + "-native")
                if native_runner
                else None
            )
            if engine == "icarus":
                executable = scratch / (name + ".vvp")
                parameters = [
                    item
                    for key, value in params.items()
                    for item in ("-P", f"tb_memory.{key}={value}")
                ]
                run(
                    [
                        args.iverilog,
                        "-g2012",
                        "-s",
                        "tb_memory",
                        *parameters,
                        *defines,
                        "-o",
                        executable,
                        bench,
                        *leaves,
                    ],
                    name + "-icarus-build",
                )
                observed = run(
                    [args.vvp, executable, "+VECTORS=" + str(path)],
                    name + "-icarus-run",
                    marker="PASS memory",
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
                        "tb_memory",
                        "--Mdir",
                        directory,
                        *parameters,
                        *defines,
                        bench,
                        *leaves,
                    ],
                    name + "-verilator-build",
                )
                observed = run(
                    [directory / "Vtb_memory", "+VECTORS=" + str(path)],
                    name + "-verilator-run",
                    marker="PASS memory",
                )
                if synthesis:
                    xml = scratch / (name + ".xml")
                    run(
                        [
                            args.verilator,
                            "--xml-only",
                            "--timing",
                            "-Wno-fatal",
                            "--top-module",
                            "tb_memory",
                            "--xml-output",
                            xml,
                            *parameters,
                            *defines,
                            bench,
                            *leaves,
                        ],
                        name + "-synthesis-declarations",
                    )
                    names = [
                        node.attrib.get("name", "")
                        for node in ET.parse(xml).iter("var")
                    ]
                    assert not any(
                        name.startswith(
                            (
                                "idle",
                                "live",
                                "clock_current",
                                "clock_pending",
                                "pending_valid",
                                "preparation_error",
                            )
                        )
                        for name in names
                    ), names
            if reference:
                assert trace(observed.stdout) == trace(reference.stdout), (
                    name,
                    engine,
                    "RTL/native mismatch",
                )
            assert len(trace(observed.stdout)) == len(vectors.rows), (
                name,
                engine,
                "missing actual rows",
            )
            receipts[engine] = {
                "rows": len(vectors.rows),
                "four_state": engine == "icarus",
                "unknown_Q_asserted": engine == "icarus",
                "stimulus_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            }
        return receipts

    for command in (
        [args.iverilog, "-V"],
        [args.vvp, "-V"],
        [args.verilator, "--version"],
        [args.cxx, "--version"],
    ):
        run(command, "version")
    for kind in kinds:
        configs = (
            [(16, 4, 7, False), (64, 65, 7, False), (16, 65, 7, True)]
            if kind == 0
            else [(13, 4, 3, False), (65, 65, 3, False), (13, 65, 3, True)]
        )
        for width, address, depth, structured in configs:
            name = f"{leaf_names[kind]}-w{width}-a{address}" + (
                "-struct" if structured else ""
            )
            params = {
                "KIND": kind,
                "WIDTH": width,
                "ADDR_WIDTH": address,
                "DEPTH": depth,
                "MANAGED": 1,
            }
            defines = ["-DSTRUCT_PAYLOAD"] if structured else []
            native_runner = scratch / (name + "-native")
            run(
                [
                    args.cxx,
                    "-std=c++20",
                    "-O0",
                    f"-DMEM_KIND={kind}",
                    f"-DMEM_WIDTH={width}",
                    f"-DMEM_ADDRESS={address}",
                    f"-DMEM_DEPTH={depth}",
                    "-I" + str(repo / "include"),
                    native,
                    "-o",
                    native_runner,
                ],
                name + "-native-build",
            )
            vectors = {
                engine: stimulus(kind, width, address, depth, engine == "icarus")
                for engine in ("icarus", "verilator")
            }
            receipts = engines(name, params, defines, vectors, native_runner)
            cases.append(
                {
                    "case": name,
                    "managed": True,
                    "native_kernel": True,
                    "independent_absolute_read_age_oracle": True,
                    "engines": receipts,
                }
            )
        for synthesis in (False, True):
            width, address, depth = (16, 65, 7) if kind == 0 else (13, 65, 3)
            name = leaf_names[kind] + ("-synthesis" if synthesis else "-autonomous")
            params = {
                "KIND": kind,
                "WIDTH": width,
                "ADDR_WIDTH": address,
                "DEPTH": depth,
                "MANAGED": 1 if synthesis else 0,
            }
            defines = ["-DSYNTHESIS"] if synthesis else []
            vectors = {
                engine: stimulus(
                    kind,
                    width,
                    address,
                    depth,
                    False,
                    managed=False,
                    synthesis=synthesis,
                )
                for engine in ("icarus", "verilator")
            }
            receipts = engines(name, params, defines, vectors, synthesis=synthesis)
            cases.append(
                {
                    "case": name,
                    "managed": False,
                    "synthesis_projection": synthesis,
                    "engines": receipts,
                }
            )
            if not synthesis:
                executable = scratch / (name + ".vvp")
                run(
                    [args.vvp, executable, "+BAD_AUTO_WRITE"],
                    name + "-invalid-enabled-write",
                    rejected=True,
                    marker=leaf_names[kind]
                    + ": enabled write address, data and strobes must be known",
                )
                if kind == 0:
                    run(
                        [args.vvp, executable, "+BAD_AUTO_READ"],
                        name + "-invalid-continuous-read",
                        rejected=True,
                        marker="byte_mem: read address must be known",
                    )
                if kind == 2:
                    run(
                        [args.vvp, executable, "+BAD_AUTO_PORT1"],
                        name + "-invalid-second-read",
                        rejected=True,
                        marker="sync_mem_dp: enabled read address must be known",
                    )
        dimension_guard = (
            "byte_mem: positive widths/depth and whole-byte data required"
            if kind == 0
            else leaf_names[kind] + ": widths and depth must be positive"
        )
        for kind_parameter, value in (("DEPTH", 0), ("ADDR_WIDTH", 0)):
            executable = scratch / (
                leaf_names[kind] + "-invalid-" + kind_parameter + ".vvp"
            )
            run(
                [
                    args.iverilog,
                    "-g2012",
                    "-s",
                    "tb_memory",
                    "-P",
                    f"tb_memory.KIND={kind}",
                    "-P",
                    f"tb_memory.{kind_parameter}={value}",
                    "-P",
                    f"tb_memory.WIDTH={16 if kind == 0 else 13}",
                    "-o",
                    executable,
                    bench,
                    *leaves,
                ],
                leaf_names[kind] + "-invalid-" + kind_parameter + "-build",
            )
            run(
                [args.vvp, executable],
                leaf_names[kind] + "-invalid-" + kind_parameter,
                rejected=True,
                marker=dimension_guard,
            )
        executable = scratch / (leaf_names[kind] + "-two-state.vvp")
        run(
            [
                args.iverilog,
                "-g2012",
                "-DTWO_STATE_PAYLOAD",
                "-s",
                "tb_memory",
                "-P",
                f"tb_memory.KIND={kind}",
                "-o",
                executable,
                bench,
                *leaves,
            ],
            leaf_names[kind] + "-two-state-build",
        )
        run(
            [args.vvp, executable],
            leaf_names[kind] + "-two-state-rejected",
            rejected=True,
            marker="four-state",
        )
    if 0 in kinds:
        executable = scratch / "byte-non-byte-width.vvp"
        run(
            [
                args.iverilog,
                "-g2012",
                "-s",
                "tb_memory",
                "-P",
                "tb_memory.WIDTH=13",
                "-o",
                executable,
                bench,
                *leaves,
            ],
            "byte-non-byte-build",
        )
        run(
            [args.vvp, executable],
            "byte-non-byte-rejected",
            rejected=True,
            marker="whole-byte data required",
        )
    assert {
        str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs
    } == before, "memory input drift"
    (scratch / "results.json").write_text(
        json.dumps(
            {
                "scope": "T2 "
                + args.case
                + " actual owning RTL/native memory lifecycle",
                "input_sha256": before,
                "cases": cases,
                "historical_roots_closed": 0,
                "two_state_Q_witness_gap": "Verilator cannot prove X expiry; Icarus/native assert full unknown planes",
            },
            indent=2,
        )
        + "\n"
    )
    print(
        "PASS T2 "
        + args.case
        + ": actual RTL/native, full-width addresses, masks, read lifetime and complete-update permission"
    )  # noqa: T201 - standalone gate receipt


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("repo", "scratch", "iverilog", "vvp", "verilator", "cxx"):
        parser.add_argument("--" + option, required=True)
    parser.add_argument("--case", required=True, choices=("byte_mem", "sync_mem"))
    run_memory(parser.parse_args())
