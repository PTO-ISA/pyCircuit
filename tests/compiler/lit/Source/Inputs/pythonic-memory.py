"""Independent direct-memory inference, interface and generated-hardware gate."""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
for option in ("repo", "source-compiler", "linker", "emitter", "opt", "cxx",
               "verilator", "scratch"):
    parser.add_argument("--" + option, required=True)
for option in ("iverilog", "vvp"):
    parser.add_argument("--" + option, default=shutil.which(option))
args = parser.parse_args()
repo, scratch_root = Path(args.repo).resolve(), Path(args.scratch).resolve()
scratch_root.mkdir(parents=True, exist_ok=True)
# lit reuses %t.dir. Give every publication fresh destinations and retain the
# complete run, including failure logs, instead of replacing earlier evidence.
scratch = Path(tempfile.mkdtemp(prefix="pythonic-memory-", dir=scratch_root))
fixtures = Path(__file__).resolve().parent
source = scratch / "source"
source.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(repo / "python/pycircuit/src"))
# The requested checkout supplies capture; never import an installed compiler.
from pycircuit._source_capture import _capture_source_file  # noqa: E402
from pycircuit._source_transport import _emit_source_transport  # noqa: E402

env = dict(os.environ, PYTHONPATH=str(repo / "python/pycircuit/src"),
           PYTHONDONTWRITEBYTECODE="1", PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
           PYCIRCUIT_LINKER=args.linker, PYCIRCUIT_EMITTER=args.emitter)
commands = []


def run(command, success=True):
    command = list(map(str, command))
    result = subprocess.run(command, env=env, cwd=repo, text=True,
                            capture_output=True, timeout=240)
    commands.append({"command": command, "exit_status": result.returncode,
                     "stdout": result.stdout, "stderr": result.stderr})
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if success else 1), commands[-1]
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr, commands[-1]
    return result


def cli(*options, success=True):
    return run([sys.executable, "-m", "pycircuit.cli", *options], success)


def compile_source(path, output, interfaces=(), success=True, replace=False):
    options = ["compile", "-c", path, "--source-root", source,
               "--package-prefix", "memory_suite", "-o", output]
    for interface in interfaces:
        options += ["-I", interface]
    if replace:
        options.append("--replace")
    return cli(*options, success=success)


def payload(unit, kind):
    receipt = json.loads((unit / "unit.json").read_text())
    return unit / receipt["files"][kind]


def infer(path, interfaces=(), success=True):
    capture = scratch / (path.stem + ".capture.mlir")
    capture.write_text(_emit_source_transport(_capture_source_file(path, source_root=source)))
    options = f"package=memory_suite source-path={path.name} report-plan=true"
    if interfaces:
        options += " headers=" + ",".join(str(payload(unit, "interface")) for unit in interfaces)
    output = scratch / (path.stem + ".inferred.mlir")
    result = run([args.opt, capture, "--ac-infer-source-bindings=" + options,
                  "-o", output], success)
    if success:
        text = output.read_text()
        assert "ac.python_capture =" in text
        assert '"ac.instance"' not in text and '"ac.rule"' not in text, "inference lowered hardware bodies"
    return result


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot(directory):
    return {str(path.relative_to(directory)): digest(path) for path in directory.rglob("*") if path.is_file()}


for path in (fixtures / "pythonic-memory").glob("*.py"):
    shutil.copyfile(path, source / path.name)
design = source / "design.py"
report = infer(design).stderr
for geometry in ("depth=3 address-width=2 payload-width=13 strobe-width=2",
                 "depth=5 address-width=3 payload-width=16 strobe-width=2",
                 "depth=1 address-width=1 payload-width=13 strobe-width=2",
                 "depth=4 address-width=2 payload-width=32 strobe-width=4"):
    assert geometry in report, report
for top in ("Sync", "Dual", "Byte", "DepthOne", "Forward", "Sync32"):
    assert re.search(rf"inferred module @memory_suite\.design\.{top} domain=1", report), report
unit = scratch / "unit"
compile_source(design, unit)
before_unit = snapshot(unit)

# Published interfaces are the only provider material present during consumer compile.
provider, facade, consumer = (scratch / name for name in ("provider-unit", "facade-unit", "consumer-unit"))
compile_source(source / "provider.py", provider)
(source / "provider.py").unlink()
compile_source(source / "facade.py", facade, [provider])
(source / "facade.py").unlink()
consumer_report = infer(source / "consumer.py", [provider, facade]).stderr
for top in ("Middle", "Top"):
    assert re.search(rf"inferred module @memory_suite\.consumer\.{top} domain=1", consumer_report), consumer_report
compile_source(source / "consumer.py", consumer, [provider, facade])
cli("link", provider, facade, consumer, "--top", "memory_suite.consumer.Top", "-o", scratch / "consumer.ac")
infer(source / "consumer.py", success=False)
missing = scratch / "missing-headers-unit"
compile_source(source / "consumer.py", missing, success=False)
assert not missing.exists()
header = payload(provider, "interface").read_text()
assert "ac.domain_inputs" in header and "ac.parameters" in header and "ac.result_constraints" in header
malformed_header = scratch / "tampered-domain-header.ac"
malformed = header.replace("ac.domain_inputs = {", "ac.domain_inputs = {bogus = 99 : i64, ", 1)
assert malformed != header
malformed_header.write_text(malformed)
run([args.opt, scratch / "consumer.capture.mlir",
     "--ac-infer-source-bindings=package=memory_suite source-path=consumer.py headers=" +
     str(malformed_header) + "," + str(payload(facade, "interface")),
     "-o", scratch / "bad-header-result.ac"], success=False)

# Existing explicit physical-pin structural authoring remains admitted.
explicit = source / "explicit.py"
explicit.write_text('''import pycircuit as ac
from pycircuit import sync_mem
@ac.module
def Explicit(clk: bool, rst: bool, ren: bool, addr: ac.u2,
             valid: bool, data: ac.u13, strobe: ac.u2) -> {"rdata": ac.u13}:
    memory = sync_mem(T=ac.u13, ADDR_WIDTH=2, DEPTH=3)
    @ac.rule
    def bind():
        memory(clk=clk, rst=rst, ren=ren, raddr=addr,
               wvalid=valid, waddr=addr, wdata=data, wstrb=strobe)
    bind()
    return {"rdata": memory.rdata}
''')
explicit_report = infer(explicit).stderr
assert "domain=0" in explicit_report and "inferred memory" not in explicit_report
compile_source(explicit, scratch / "explicit-unit")

original = design.read_text()
body_only = source / "unlowerable_body.py"
body_only.write_text(original.replace("    return BitsResult(r0=first, r1=second)",
                                      "    unused = unsupported_runtime_expression(first)\n"
                                      "    return BitsResult(r0=first, r1=second)", 1))
assert "depth=3 address-width=2" in infer(body_only).stderr
rejected = compile_source(body_only, scratch / "unlowerable-body-unit", success=False)
assert re.search(r"unsupported|unknown", rejected.stderr, re.I), rejected.stderr
first_call = "ram[ac.u13](ren0, raddr0, wvalid, waddr, wdata, wstrb, depth=3)"
negatives = {
    "zero-depth": original.replace("depth=3", "depth=0", 1),
    "dynamic-depth": original.replace("depth=3", "depth=wdata", 1),
    "missing-depth": original.replace(", depth=3", "", 1),
    "wide-address": original.replace("def Sync(ren0: ac.u1, raddr0: ac.u2", "def Sync(ren0: ac.u1, raddr0: ac.u3", 1),
    "narrow-address": original.replace("def Sync(ren0: ac.u1, raddr0: ac.u2", "def Sync(ren0: ac.u1, raddr0: ac.u1", 1),
    "duplicate-input": original.replace(first_call, first_call[:-1] + ", wstrb=wstrb)", 1),
    "conditional-allocation": original.replace("    first = " + first_call, "    if ren0:\n        first = " + first_call, 1),
    "rebound-result": original.replace("    second = ac.sync_mem", "    first = wdata\n    second = ac.sync_mem", 1),
    "wrong-dual-target": original.replace("    first, second = ac.sync_mem_dp", "    first = ac.sync_mem_dp", 1),
    "table-payload": original.replace("ac.sync_mem_dp[Pair13]", "ac.sync_mem_dp[ac.table[2, ac.u8]]", 1),
    "byte-alignment": original.replace("ac.byte_mem[ac.u16]", "ac.byte_mem[ac.u13]", 1),
    "shadowed-builtin": original.replace("def Sync(ren0:", "def Sync(ram: ac.u13, ren0:", 1),
}
for name, text in negatives.items():
    assert text != original
    path = source / (name.replace("-", "_") + ".py")
    path.write_text(text)
    infer(path, success=False)
    absent = scratch / ("invalid-" + name)
    rejected = compile_source(path, absent, success=False)
    assert rejected.stderr.strip() and not absent.exists()
    if name == "wide-address":
        compile_source(path, unit, success=False, replace=True)
        assert snapshot(unit) == before_unit

cycle = source / "async_cycle.py"
cycle.write_text('''import pycircuit as ac
@ac.struct
class Result:
    value: ac.u16
@ac.module
def Cycle(write: ac.u1, addr: ac.u3, data: ac.u16, strobe: ac.u2) -> Result:
    first = ac.byte_mem[ac.u16](second[0:3], write, addr, data, strobe, depth=5)
    second = ac.byte_mem[ac.u16](first[0:3], write, addr, data, strobe, depth=5)
    return Result(value=first)
''')
# Binding inference can form the graph; common hardware verification owns cycles.
infer(cycle)
rejected = compile_source(cycle, scratch / "invalid-cycle", success=False)
assert re.search(r"cycl|feedback", rejected.stderr, re.I), rejected.stderr


def rows(kind, width, depth, scenario=None):
    """Small value/lifetime oracle; no DUT/runtime evaluator is called."""
    memory = [0] * depth
    q, idle, last, trace = [None, 0 if scenario else None], [0, 0], 0, []
    lanes = (width + 7) // 8

    def read(address):
        if kind == 0:
            return sum((memory[address + lane] if address + lane < len(memory) else 0) << (8 * lane)
                       for lane in range(2))
        return memory[address] if address < len(memory) else 0

    def sample(clock, reset=0, ren0=1, ra0=2, ren1=1, ra1=1,
               write=0, wa=2, word=0x1abc, strobe=None):
        nonlocal last
        if strobe is None:
            strobe = (1 << lanes) - 1
        expected = [read(ra0), 0] if kind == 0 else q.copy()
        trace.append([clock, reset, ren0, ra0, ren1, ra1, write, wa, word, strobe,
                      *(value or 0 for value in expected), *(int(value is not None) for value in expected)])
        if clock and not last:
            if reset:
                q[:] = [None, 0 if scenario else None]
                idle[:] = [0, 0]
            else:
                if kind:
                    for port, (enabled, address) in enumerate(((ren0, ra0), (ren1, ra1))):
                        if scenario and port == 1:
                            continue  # The single-port fixture's second output is constant zero.
                        if enabled:
                            q[port] = 0 if kind == 1 and port == 1 else read(address)
                            idle[port] = 0
                        elif q[port] is not None:
                            idle[port] += 1
                            if idle[port] == 2:
                                q[port] = None
                if write:
                    for lane in range(lanes):
                        if strobe & (1 << lane):
                            if kind == 0:
                                if wa + lane < len(memory):
                                    memory[wa + lane] = (word >> (8 * lane)) & 255
                            elif wa < len(memory):
                                mask = (255 << (8 * lane)) & ((1 << width) - 1)
                                memory[wa] = (memory[wa] & ~mask) | (word & mask)
        last = clock

    def edge(**pins):
        sample(1, **pins)
        sample(0, **pins)

    if scenario:
        # Historical reset: two asserted rising edges and one deasserted edge.
        pins = {"ren0": 0, "ra0": 0, "ren1": 0, "ra1": 0, "write": 0,
                "wa": 0, "word": 0, "strobe": 0}
        sample(0, reset=1, **pins)
        edge(reset=1, **pins)
        edge(reset=1, **pins)
        edge(**pins)
        if scenario == "read-during-write":
            pins.update(write=1, word=0x11111111, strobe=(1 << lanes) - 1)
            edge(**pins)
            pins.update(ren0=1, word=0x22222222)
            edge(**pins)
            assert trace[-1][-4:] == [0x11111111, 0, 1, 1]
            pins.update(write=0, strobe=0)
            edge(**pins)
            assert trace[-1][-4:] == [0x22222222, 0, 1, 1]
        elif scenario == "initialized-zero":
            pins.update(ren0=1, ra0=1)
            edge(**pins)
            assert trace[-1][-4:] == [0, 0, 1, 1]
            pins.update(ra0=3)
            edge(**pins)
            assert trace[-1][-4:] == [0, 0, 1, 1]
            assert all(row[6] == 0 for row in trace), "init-zero scenario must not write"
        else:
            raise AssertionError("unknown historical memory oracle")
        return trace

    sample(0, reset=1)
    edge(reset=1)
    edge(write=1, strobe=1)
    sample(1, write=0)
    sample(1, write=1, word=0)  # held high cannot write or age Q
    sample(0, write=0)
    edge(write=1, strobe=2)
    edge()
    edge(ren0=0)
    edge(ren0=0)
    edge(ra1=2)
    edge(reset=1, write=1, word=0)
    edge()
    edge(ra0=3)
    if kind == 0:
        edge(write=1, wa=4, ra0=4, word=0xa5bc)
        edge(ra0=4)
        edge(ra0=3)
        edge(ra0=5)
    else:
        edge(ra0=1, ra1=0)
    return trace



def protocol_rows(mode):
    """Independent queue/deadline/RAM scoreboard; captures old data at acceptance."""
    from collections import deque

    shared = mode in (5, 6)
    queued = mode in (3, 4, 6)
    packet_bits = 28 if mode in (3, 5, 6) else 29
    latency = 1 if mode in (0, 5, 6) else 3
    capacity = 2 if shared else 4 if mode in (2, 4) else 1
    queues = [deque(), deque()]
    responses = [deque(), deque()]
    memory = [0] * 16
    flight = None
    epoch, last = -2, 0
    trace, events = [], {"accepted": [], "enqueued": [], "delivered": [], "cancelled": []}
    fault_rows = {}
    empty = (0, 0, 0, 0)

    def pack(packet):
        address, write, data, tag = packet
        return ((address << (25 if packet_bits == 29 else 24)) |
                ((write << 24) if packet_bits == 29 else 0) | (data << 8) | tag)

    def row(clock, reset=0, v0=0, p0=empty, t0=1, v1=0, p1=empty, t1=1):
        nonlocal epoch, last, flight
        valid, packets, take = [v0, v1], [p0, p1], [t0, t1]
        heads = [queues[i][0] if queues[i] else empty for i in range(2)] if queued else packets
        available = [bool(q) for q in queues] if queued else valid
        idle = flight is None
        ready = [idle and available[0], idle and not available[0]] if shared else [idle, False]
        accepted = [bool(ready[i] and available[i]) for i in range(2)]
        out_valid = [bool(q) for q in responses]
        enqueued = [False, False]
        if flight is not None and epoch >= flight["due"]:
            end = flight["endpoint"]
            enqueued[end] = len(responses[end]) < capacity
        in_ready = [len(q) < 4 for q in queues] if queued else ready
        payloads = [pack(q[0]) if q else 0 for q in responses]
        flags = [*in_ready, *out_valid, *accepted, *enqueued] if shared else [
            in_ready[0], out_valid[0], accepted[0], enqueued[0]]
        flag_bits = 0
        for flag in flags:
            flag_bits = (flag_bits << 1) | int(flag)
        if shared:
            expected = (flag_bits << 56) | (payloads[0] << 28) | payloads[1]
            mask = (255 << 56) | (((1 << 28) - 1) << 28 if out_valid[0] else 0)
            mask |= (1 << 28) - 1 if out_valid[1] else 0
        else:
            expected = (flag_bits << packet_bits) | payloads[0]
            mask = (15 << packet_bits) | ((1 << packet_bits) - 1 if out_valid[0] else 0)
        trace.append([clock, reset, v0, pack(p0), t0, v1, pack(p1), t1, expected, mask])
        if clock and not last:
            if not reset:
                if any(accepted):
                    endpoint = accepted.index(True)
                    address = heads[endpoint][0]
                    fault_rows.setdefault("accept", [len(trace) - 1, address, memory[address]])
                    endpoint = accepted.index(True)
                    if (shared and endpoint == 0) or (packet_bits == 29 and heads[endpoint][1]):
                        fault_rows.setdefault("first-write", [len(trace) - 1, address, memory[address]])
                if flight is not None and epoch == flight["due"] - latency + 1:
                    address = flight["response"][0]
                    fault_rows.setdefault("capture", [len(trace) - 1, address, memory[address]])
                if any(enqueued):
                    address = flight["response"][0]
                    fault_rows.setdefault("release", [len(trace) - 1, address, memory[address]])
            if reset:
                if flight is not None:
                    events["cancelled"].append(flight["response"][3])
                for queue in responses:
                    events["cancelled"].extend(packet[3] for packet in queue)
                for queue in queues + responses:
                    queue.clear()
                flight = None  # Current reset cancels transactions, retaining RAM.
            else:
                old_flight = flight
                for endpoint in range(2 if shared else 1):
                    if out_valid[endpoint] and take[endpoint]:
                        delivered = responses[endpoint].popleft()
                        events["delivered"].append([epoch, endpoint, list(delivered)])
                    if enqueued[endpoint]:
                        responses[endpoint].append(old_flight["response"])
                        events["enqueued"].append([epoch, endpoint, list(old_flight["response"])])
                        flight = None
                assert sum(accepted) <= 1
                for endpoint in range(2 if shared else 1):
                    if accepted[endpoint]:
                        request = heads[endpoint]
                        address, write, data, tag = request
                        write = (endpoint == 0) if shared else bool(write) if packet_bits == 29 else False
                        response = (address, int(write), memory[address], tag)
                        if write:
                            memory[address] = data
                        flight = {"endpoint": endpoint, "response": response, "due": epoch + latency}
                        events["accepted"].append([epoch, endpoint, list(request)])
                        if queued:
                            queues[endpoint].popleft()
                    if queued and valid[endpoint] and in_ready[endpoint]:
                        queues[endpoint].append(packets[endpoint])
            epoch += 1
        last = clock

    def tick(**pins):
        row(1, **pins)
        row(0, **pins)

    row(0, reset=1)
    tick(reset=1)
    tick(reset=1)
    if mode == 3:
        # Original queued-controller schedule, including exact acceptance and
        # enqueue boundaries.
        tick(v0=1, p0=(2, 0, 0, 1))
        tick(v0=1, p0=(7, 0, 0, 2))
        for _ in range(10):
            tick()
        assert [e[0] for e in events["accepted"]] == [1, 5]
        assert [e[0] for e in events["enqueued"]] == [4, 8]
        assert [e[0] for e in events["delivered"]] == [5, 9]
        assert [e[2][2:] for e in events["delivered"]] == [[0, 1], [0, 2]]
    elif mode == 6:
        # Same three original transactions; second writer queues as A is accepted.
        tick(v0=1, p0=(3, 1, 42, 1), v1=1, p1=(3, 0, 0, 3))
        tick(v0=1, p0=(3, 1, 99, 2))
        for _ in range(10):
            tick()
        assert [e[2][2:] for e in events["delivered"]] == [[0, 1], [42, 2], [99, 3]]
    def feed(pending, cycles, writer_start=0, take_writer=lambda n: 1,
             take_reader=lambda n: 1):
        positions = [0, 0]
        for cycle in range(cycles):
            available = [positions[i] < len(pending[i]) and (i != 0 or cycle >= writer_start)
                         for i in range(2)]
            pins = {"t0": int(take_writer(cycle)), "t1": int(take_reader(cycle))}
            for endpoint in range(2 if shared else 1):
                packet = pending[endpoint][positions[endpoint]] if available[endpoint] else empty
                ready = len(queues[endpoint]) < 4 if queued else flight is None
                if shared and not queued:
                    ready = ready and (endpoint == 0 or not available[0])
                pins["v" + str(endpoint)] = int(available[endpoint])
                pins["p" + str(endpoint)] = packet
                if available[endpoint] and ready:
                    positions[endpoint] += 1
            if cycle == 3:
                row(1, **pins)
                row(1, **pins)  # Held high does not commit again.
                row(0, **pins)
            else:
                tick(**pins)
            if cycle == 27:
                row(0, **pins)  # Held low does not advance service deadlines.
        assert positions == [len(p) for p in pending]
        assert flight is None and all(not q for q in queues + responses)

    if shared:
        # Reader response queue fills, then a higher-priority writer arrives.
        # Its empty response queue must not release the saved reader transaction.
        feed([[(5, 1, 0x6543, 11)], [(3, 0, 0, tag) for tag in (8, 9, 10)]],
             35, writer_start=8, take_reader=lambda n: n >= 15)
    # Saturation, long stalls beyond RAM Q lifetime, and complete packet metadata.
    pending = [[(n % 5, int(n % 3 != 0), (n * 997 + 37) & 65535, 16 + n)
                for n in range(12)], [(n % 5, 0, 0, 64 + n) for n in range(7)] if shared else []]
    feed(pending, 110, take_writer=lambda n: n >= 25 and n % 5 != 0,
         take_reader=lambda n: n % 7 != 0)
    for _ in range(10):
        tick()
    if mode == 3:
        assert memory == [0] * 16, "the read-only controller never writes request.data into RAM"
    # Reset cancels a response after its RAM write has already committed.
    # It does not roll that earlier write back or clear the current RAM policy.
    tick(v0=1, p0=(9, 1, 0x4321, 90), t0=0, t1=0)
    if queued:
        tick(t0=0, t1=0)
    tick(reset=1, t0=0, t1=0)
    if shared:
        tick(v1=1, p1=(9, 0, 0, 91))
    else:
        tick(v0=1, p0=(9, 0, 0, 91))
    for _ in range(15):
        tick()
    assert events["cancelled"] == [90]
    assert events["delivered"][-1][2][2:] == [0 if mode == 3 else 0x4321, 91]
    assert flight is None and all(not q for q in queues + responses)
    accepted_tags = sorted(e[2][3] for e in events["accepted"] if e[2][3] not in events["cancelled"])
    delivered_tags = sorted(e[2][3] for e in events["delivered"])
    assert accepted_tags == delivered_tags and len(set(delivered_tags)) == len(delivered_tags)
    if "first-write" in fault_rows:
        fault_rows["accept"] = fault_rows.pop("first-write")
    assert set(fault_rows) == {"accept", "capture", "release"}
    return trace, events, fault_rows


toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next((path for path in (toolroot / "simulator/gfsim/libpyc6_runtime.a",
                                 toolroot / "lib/libpyc6_runtime.a") if path.is_file()), None)
assert runtime, "candidate Runtime archive missing"
summary = []
protocol_unit = scratch / "protocol-unit"
compile_source(source / "protocol.py", protocol_unit)
cases = (("Sync", 1, 13, 2, 3, None), ("Dual", 2, 13, 2, 3, None),
         ("Byte", 0, 16, 3, 5, None),
         ("Sync32", 1, 32, 2, 4, "read-during-write"),
         ("Sync32", 1, 32, 2, 4, "initialized-zero"))
protocols = ("ControllerL1", "ControllerL3", "ControllerL3Buffered", "MemoryBusy",
             "MemoryPipeline", "SharedControllerL1", "MemorySimple")
cases += tuple((top, 3, 16, 4, 16, mode) for mode, top in enumerate(protocols))
for top, kind, width, address_width, depth, scenario in cases:
    output = scratch / (top if kind == 3 else scenario or top)
    output.mkdir(exist_ok=True)
    if kind == 3:
        frames, events, fault_rows = protocol_rows(scenario)
        (output / "oracle-events.json").write_text(json.dumps(events, indent=2) + "\n")
        (output / "fault-rows.txt").write_text("".join(f"{phase} {row} {address} {value}\n" for phase, (row, address, value) in fault_rows.items()))
    else:
        frames = rows(kind, width, depth, scenario)
    defines = [f"-DMEM_WIDTH={width}", f"-DMEM_ADDR_WIDTH={address_width}",
               f"-DMEM_FRAMES={len(frames)}"]
    if kind == 3:
        request_bits = 28 if scenario in (3, 5, 6) else 29
        result_bits = 64 if scenario >= 5 else request_bits + 4
        defines += [f"-DMEM_PROTOCOL={scenario}", f"-DREQUEST_BITS={request_bits}", f"-DRESULT_BITS={result_bits}"]
        if scenario >= 5:
            defines += ["-DSHARED_PROTOCOL"]
    rowfile = output / "rows.txt"
    rowfile.write_text("".join(" ".join(map(str, row)) + "\n" for row in frames))
    final = output / "design_top.ac"
    cli("link", protocol_unit if kind == 3 else unit, "--top",
        ("memory_suite.protocol." if kind == 3 else "memory_suite.design.") + top, "-o", final)
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", output / target)
    receipt = json.loads((output / "cpp/generated.json").read_text())
    cpp = [output / "cpp" / item["path"] for item in receipt["files"] if item["path"].endswith(".cpp")]
    runner = output / "runner"
    run([args.cxx, "-std=c++20", "-pthread", f"-DMEM_KIND={kind}", *defines,
         "-I" + str(repo / "include"), "-I" + str(output / "cpp"),
         fixtures / "pythonic-memory.cpp", *cpp, runtime, "-o", runner])
    native = []
    for workers in (1, 2):
        result = run([runner, workers, rowfile] + ([output / "fault-rows.txt"] if kind == 3 else [])).stdout
        assert result.splitlines()[-1] == "PASS", result
        (output / f"native-{workers}.stdout").write_text(result)
        native.append([line for line in result.splitlines() if line.startswith("WORK ")])
    assert native[0] == native[1] and len(native[0]) == len(frames)
    receipt = json.loads((output / "verilog/generated.json").read_text())
    rtl = [output / "verilog" / item["path"] for item in receipt["files"] if item["role"] == "rtl"]
    rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
    library = sorted((repo / "include/verilog").glob("*.v"))
    rtl_build = output / "rtl-build"
    run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vmemory",
         "--Mdir", rtl_build, "-j", "2", "-Wno-fatal", *defines,
         *library, *rtl, fixtures / "pythonic-memory.sv"])
    result = run([rtl_build / "Vmemory", "+rows=" + str(rowfile)]).stdout
    (output / "rtl.stdout").write_text(result)
    assert [line for line in result.splitlines() if line.startswith("WORK ")] == native[0]
    if args.iverilog and args.vvp:
        binary = output / "rtl-four-state"
        run([args.iverilog, "-g2012", *defines, "-s", "tb", "-o", binary,
             *library, *rtl, fixtures / "pythonic-memory.sv"])
        result = run([args.vvp, binary, "+rows=" + str(rowfile)]).stdout
        (output / "rtl-four-state.stdout").write_text(result)
        assert [line for line in result.splitlines() if line.startswith("WORK ")] == native[0]
    summary.append({"top": top, "kind": kind, "width": width, "depth": depth,
                    "scenario": scenario, "frames": len(frames), "workers": [1, 2],
                    "icarus": bool(args.iverilog and args.vvp),
                    "native_fault_replays_per_worker": 18 if kind == 3 else 0})
inputs = [fixtures / ("pythonic-memory" + ext) for ext in (".py", ".cpp", ".sv")]
inputs += sorted((fixtures / "pythonic-memory").glob("*.py"))
(scratch / "candidate.json").write_text(json.dumps({
    "inputs": {str(path.relative_to(repo)): digest(path) for path in inputs},
    "execution": summary, "rejections": list(negatives) + ["async-cycle", "missing-headers", "tampered-header", "unlowerable-body"]}, indent=2) + "\n")
sys.stdout.write("pythonic-memory gate passed: inferred geometry/domains, interface-only imports, native/RTL memory semantics\n")
