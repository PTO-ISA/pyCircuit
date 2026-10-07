#!/usr/bin/env python3
"""Independent FIFO oracle: absolute bigint birth/maturity edges, never ring time."""

import argparse
import hashlib
import json
import random
import subprocess
import sys
from collections import deque
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("helper", "iverilog", "vvp", "scratch"):
    parser.add_argument("--" + name, required=True)
args = parser.parse_args()
BASE = Path(args.scratch).resolve()
BASE.mkdir(parents=True, exist_ok=True)
HELPER = Path(args.helper).resolve()
IVERILOG = args.iverilog
VVP = args.vvp
INITIAL_HELPER_HASH = hashlib.sha256(HELPER.read_bytes()).hexdigest()


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def known(n, width):
    return f"{n & ((1 << width) - 1):0{width}b}"


def bitand(a, b):
    if "0" in (a, b):
        return "0"
    return "1" if a == b == "1" else "x"


def bitor(a, b):
    if "1" in (a, b):
        return "1"
    return "0" if a == b == "0" else "x"


def bitliteral(value):
    return "1'b" + str(value)


def data_literal(value):
    return f"{len(value)}'b{value}"


class Oracle:
    def __init__(self, depth, policy, width, latency=1):
        self.depth, self.policy, self.width = depth, policy, width
        self.latency = latency
        self.time = -1
        self.tokens = None
        self.clock = 0
        self.rows = []
        self.events = {
            "rising": 0,
            "push": 0,
            "pop": 0,
            "full_both": 0,
            "interior_both": 0,
            "resets": 0,
            "no_edge": 0,
            "mature": 0,
            "pop_mature_push": 0,
            "ineligible_full": 0,
        }

    def read(self, take):
        if self.tokens is None:
            return "x", "x", "x" * self.width
        valid = str(int(bool(self.tokens) and self.tokens[0][2] <= self.time))
        capacity = str(int(len(self.tokens) < self.depth))
        ready = capacity if self.policy == 0 else bitor(capacity, bitand(valid, take))
        return ready, valid, self.tokens[0][0] if valid == "1" else "0" * self.width

    def row(self, clock, reset="0", valid="0", take="0", data=0):
        reset, valid, take = str(reset), str(valid), str(take)
        if isinstance(data, int):
            data = known(data, self.width)
        before = self.read(take)
        if clock and not self.clock:
            self.events["rising"] += 1
            assert reset in "01"
            if reset == "1":
                self.tokens = deque()
                self.time = -1
                self.events["resets"] += 1
            else:
                push, pop = bitand(valid, before[0]), bitand(before[1], take)
                assert push in "01" and pop in "01"
                if self.tokens is not None:
                    n = len(self.tokens)
                    self.time += 1
                    mature = sum(token[2] == self.time for token in self.tokens)
                    self.events["mature"] += mature
                    self.events["pop_mature_push"] += int(
                        pop == push == "1" and mature != 0
                    )
                    self.events["ineligible_full"] += int(
                        n == self.depth and before[1] == "0"
                    )
                    if valid == take == "1":
                        self.events["full_both"] += int(n == self.depth)
                        self.events["interior_both"] += int(0 < n < self.depth)
                    if pop == "1":
                        self.tokens.popleft()
                        self.events["pop"] += 1
                    if push == "1":
                        self.tokens.append(
                            (data, self.time, self.time + self.latency - 1)
                        )
                        self.events["push"] += 1
                    assert len(self.tokens) <= self.depth
                else:
                    assert push == pop == "0"
        else:
            self.events["no_edge"] += 1
        self.clock = clock
        after = self.read(take)
        args = [bitliteral(x) for x in (clock, reset, valid, take)]
        args += [
            data_literal(data),
            bitliteral(before[0]),
            bitliteral(before[1]),
            data_literal(before[2]),
            bitliteral(after[0]),
            bitliteral(after[1]),
            data_literal(after[2]),
        ]
        self.rows.append("    row(" + ", ".join(args) + ");")

    def edge(self, reset="0", valid="0", take="0", data=0):
        self.row(0, reset, valid, take, data)
        self.row(1, reset, valid, take, data)


def tb_header(depth, policy, width, payload, latency=1):
    declaration = (
        f"typedef logic [{width - 1}:0] Payload;"
        if payload == "bits"
        else "typedef struct packed { logic [4:0] tag; logic [63:0] low; } Inner;\n"
        "  typedef struct packed { logic [60:0] high; Inner body; } Payload;"
    )
    return f"""module tb;
  {declaration}
  logic clk=0, rst=0, in_valid=0, out_ready=0;
  Payload in_data;
  wire in_ready,out_valid;
  wire Payload out_data;
  integer index=0;
  fifo #(.T(Payload), .DEPTH({depth}), .READY_POLICY({policy}),
         .AVAILABILITY_LATENCY(64'd{latency})) dut(.clk(clk), .rst(rst), .in_valid(in_valid), .in_data(in_data),
         .out_ready(out_ready), .in_ready(in_ready), .out_valid(out_valid), .out_data(out_data));
  task row(input logic c,r,v,t,
           input logic [{width - 1}:0] data,
           input logic br,bv,
           input logic [{width - 1}:0] bd,
           input logic ar,av,
           input logic [{width - 1}:0] ad);
    rst=r;in_valid=v;out_ready=t;in_data=data;#1;
    if(in_ready !== br || out_valid !== bv || out_data !== bd)
      $fatal(1,"before row%0d got ready%b valid%b data%b expected %b %b %b",
             index,in_ready,out_valid,out_data,br,bv,bd);
    clk=c;#1;
    if(in_ready !== ar || out_valid !== av || out_data !== ad)
      $fatal(1,"after row%0d got ready%b valid%b data%b expected %b %b %b",
             index,in_ready,out_valid,out_data,ar,av,ad);
    index=index+1;
  endtask
  initial begin
"""


def positive(depth, policy, width=13, payload="bits"):
    o = Oracle(depth, policy, width)
    # Cold reads and legal cold no-op; unknown reset/control while held high.
    o.row(0)
    o.row(1)
    o.row(1, "x", "x", "z", "z" * width)
    o.edge("1", "x", "z", "x" * width)
    # Empty masks unknown downstream ready. Reset dominates all unknown controls.
    o.edge("0", "0", "x", "z" * width)
    o.edge("0", "0", "z", "x" * width)
    for _ in range(4):
        o.edge("1", "z", "x", "z" * width)
        for n in range(depth):
            o.edge("0", "1", "0", (1 << (width - 1)) | (n + 1) * 73)
        # Hold a full queue while downstream ready changes: only bypass ready
        # may follow it; data/valid and accepted-token deque remain unchanged.
        for take in ("0", "1", "x", "z"):
            o.row(1, "x", "0", take, 997)
        o.row(0, "x", "0", "z", 998)
        # Full queue masks unknown valid when no pop is requested, both policies.
        o.edge("0", "x", "0", 17)
        o.edge("0", "z", "0", 19)
        # Exact full simultaneous case distinguishes local and bypass.
        o.edge("0", "1", "1", 271)
        for n in range(depth + 2):
            o.edge("0", "1", "1", 300 + n)
        for _ in range(depth + 2):
            o.edge("0", "0", "1", 999)
    # Pure output data remains exact X/Z, including high bits and word boundaries.
    patterns = [
        "x" * width,
        "z" * width,
        "".join(
            "z" if i % 11 == 0 else "x" if i % 7 == 0 else str(i % 2)
            for i in range(width)
        ),
    ]
    for pattern in patterns:
        o.edge("1", "x", "z", 0)
        o.edge("0", "1", "0", pattern)
        o.row(1, "x", "z", "0", 123)
        o.row(0, "x", "z", "0", 321)
        o.edge("0", "0", "0", 777)
        o.edge("0", "0", "1", 0)
        o.edge("0", "0", "x", 0)
    # Independent deterministic traffic; repeated levels cannot invent edges.
    randomizer = random.Random(0x5173 + depth * 31 + policy + width)
    for n in range(180):
        o.row(
            randomizer.randrange(2),
            "1" if n in (59, 121) else "0",
            str(randomizer.randrange(2)),
            str(randomizer.randrange(2)),
            randomizer.getrandbits(width),
        )
    for _ in range(depth + 2):
        o.edge("0", "0", "1", 0)
    # Retained old data cannot revive after reset with occupied storage.
    for n in range(depth):
        o.edge("0", "1", "0", 41 + n)
    o.edge("1", "x", "z", "z" * width)
    o.edge("0", "0", "1", 0)
    body = tb_header(depth, policy, width, payload) + "\n".join(o.rows)
    body += f'\n    $display("PASS rows%0d D{depth} P{policy} W{width} {payload}",index);$finish;\n  end\nendmodule\n'
    return body, dict(rows=len(o.rows), **o.events)


def latency_positive(depth, policy, latency, width=13, payload="bits"):
    o = Oracle(depth, policy, width, latency)
    o.row(0)
    o.row(1)  # Known-false requests mask cold transfers.
    o.row(1, "x", "z", "x", "z" * width)
    o.edge("1", "x", "z", "x" * width)
    # Exact birth E0, no empty flow even when the consumer requests a pop.
    o.edge("0", "1", "1", 73)
    for age in range(1, latency):
        o.row(1, "x", "0", "1", 99)
        o.row(0, "x", "0", "1", 98)
        o.edge("0", "0", "1", 97)
        assert (o.read("1")[1] == "1") == (age == latency - 1)
    assert o.events["pop"] == 0
    o.row(1, "x", "z", "0", 0)
    o.edge("0", "0", "1", 0)  # Earliest pop is EL, from old eligibility.
    assert o.events["pop"] == 1

    # Capacity includes waiting tokens. Unknown controls are legal when their
    # effective transfers are masked; a maturity edge still ages the token.
    o.edge("1", "z", "x", 0)
    for n in range(depth):
        o.edge("0", "1", "0", 173 + n)
    if o.read("0")[1] == "0":
        o.edge("0", "x", "z", "x" * width)
    for _ in range(latency + 3):
        o.edge("0", "0", "0", 0)
    # Backpressure persists for many timestamp revolutions without revocation.
    for _ in range(8 * (1 << (latency - 1).bit_length()) + 3):
        o.edge("0", "0", "0", 0)
    o.edge("0", "1", "1", 271)
    for n in range(5 * (depth + latency) + 7):
        o.edge("0", "1", "1", 300 + n)
    for _ in range(depth + latency + 2):
        o.edge("0", "0", "1", 0)

    # Empty wraps cannot revive stale payload/deadlines. Reset while a queue
    # contains both eligible and delayed tokens clears all token ownership.
    for _ in range(3 * (1 << (latency - 1).bit_length()) + 1):
        o.edge()
    o.edge("0", "1", "0", 431)
    for _ in range(latency - 1):
        o.edge()
    o.edge("0", "1", "0", 432)
    o.edge("1", "x", "z", "z" * width)
    for _ in range(latency + depth + 1):
        o.edge("0", "0", "1", 0)

    # Raw X/Z planes are hidden by packed zero while waiting and transported
    # exactly after maturation, including widths crossing machine words.
    for pattern in (
        "x" * width,
        "z" * width,
        "".join(
            "z" if i % 11 == 0 else "x" if i % 7 == 0 else str(i % 2)
            for i in range(width)
        ),
    ):
        o.edge("1", "x", "z", 0)
        o.edge("0", "1", "0", pattern)
        for _ in range(latency - 1):
            o.edge("0", "0", "x", 0)
        o.row(1, "x", "z", "0", 0)
        o.row(0, "x", "z", "0", 0)
        o.edge("0", "0", "1", 0)

    # Independent deterministic traffic wraps timestamps and all ring slots.
    randomizer = random.Random(0x6115173 + depth * 31 + policy + latency * 73)
    for n in range(240 + 12 * latency):
        o.row(
            randomizer.randrange(2),
            "1" if n in (59, 121) else "0",
            randomizer.randrange(2),
            randomizer.randrange(2),
            randomizer.getrandbits(width),
        )
    for _ in range(depth + latency + 2):
        o.edge("0", "0", "1", 0)
    body = tb_header(depth, policy, width, payload, latency) + "\n".join(o.rows)
    body += (
        f'\n    $display("PASS absolute-edge rows%0d D{depth} '
        f'P{policy} L{latency} W{width}",index);$finish;\n  end\nendmodule\n'
    )
    return body, dict(
        oracle="absolute bigint birth/maturity deque",
        latency=latency,
        rows=len(o.rows),
        **o.events,
    )


def huge_early(policy, latency):
    """Bounded executed early-state/reset evidence, not a huge wait claim."""
    o = Oracle(2, policy, 13, latency)
    o.row(0)
    o.edge("1", "x", "z", 0)
    for _ in range(2):
        o.edge("0", "1", "1", 73)
    for _ in range(17):
        o.row(1, "x", "z", "x", 0)
        o.edge("0", "x", "z", 0)
    o.edge("1", "x", "z", 0)
    o.edge("0", "1", "1", 97)
    for _ in range(9):
        o.edge("0", "0", "1", 0)
    assert o.events["pop"] == o.events["mature"] == 0
    body = tb_header(2, policy, 13, "bits", latency)
    k = (latency - 1).bit_length()
    body += (
        f"    if (dut.AVAILABILITY_LATENCY !== 64'd{latency} || "
        f"$bits(dut.latency_delayed.tick) != {k} || "
        f"$bits(dut.latency_delayed.deadline[0]) != {k})\n"
        '      $fatal(1,"unsigned latency/timestamp sizing");\n'
    )
    body += "\n".join(o.rows)
    body += '\n    $display("PASS bounded huge-latency early/reset");$finish;\n  end\nendmodule\n'
    return body, dict(
        scope="bounded early-state/reset; no full waiting execution",
        latency=latency,
        timestamp_bits=k,
        rows=len(o.rows),
        **o.events,
    )


def near_wrap(policy, latency):
    """Labelled valid seeded arithmetic probe; not naturally executed waiting."""
    k = (latency - 1).bit_length()
    modulus = 1 << k  # Python bigint, never a DUT-width modulus expression.
    # Reset sets tick=0 before E0, so an absolute committed edge t corresponds
    # to tick=(t+1) mod M. The seeded state preserves that reachable phase.
    now = 2 * modulus - 3
    births = (now - latency, now - latency + 2, now - latency + 3)
    o = Oracle(3, policy, 13, latency)
    o.edge("1")
    o.row(0)
    body = tb_header(3, policy, 13, "bits", latency) + "\n".join(o.rows)
    body += f"""
    // SEEDED ARITHMETIC PROBE: three legal births on absolute edges {births}.
    // The eligible head matured at {now - 1}; the waiting suffix matures at
    // {now + 1} and {now + 2}. No claim of executing the preceding huge wait.
    dut.rd=0; dut.wr=0; dut.count=3;
    dut.storage[0]=13'd41; dut.storage[1]=13'd42; dut.storage[2]=13'd43;
    dut.latency_delayed.tick={k}'d{modulus - 2};
    dut.latency_delayed.mature_ptr=1; dut.latency_delayed.eligible_count=1;
    dut.latency_delayed.deadline[0]={k}'d{modulus - 3};
    dut.latency_delayed.deadline[1]={k}'d{modulus - 1};
    dut.latency_delayed.deadline[2]={k}'d0;
    #1;
    if ($bits(dut.latency_delayed.tick) != {k})
      $fatal(1,"boundary timestamp sizing");
"""
    o.rows.clear()
    o.time = now
    o.tokens = deque(
        (known(41 + n, 13), birth, birth + latency - 1)
        for n, birth in enumerate(births)
    )
    assert births[-1] <= now
    o.edge("0", "1", "1", 44)  # Pop + maturity + full replacement for policy1.
    body += "\n".join(o.rows)
    accepted = int(policy == 1)
    body += (
        f"\n    if (dut.latency_delayed.tick !== {k}'d{modulus - 1})\n"
        '      $fatal(1,"advance to maximum timestamp");\n'
    )
    if accepted:
        body += (
            f"    if (dut.latency_delayed.deadline[0] !== {k}'d{latency - 2})\n"
            '      $fatal(1,"new birth deadline unsigned wrap");\n'
        )
    o.rows.clear()
    o.edge("0", "1", "1", 45)
    body += "\n".join(o.rows)
    body += (
        f"\n    if (dut.latency_delayed.tick !== {k}'d0)\n"
        '      $fatal(1,"timestamp wraps to zero");\n'
    )
    o.rows.clear()
    for _ in range(7):
        o.edge("0", "0", "1", 0)
    o.edge("1", "x", "z", 0)
    o.edge("0", "0", "1", 0)
    body += "\n".join(o.rows)
    body += '\n    $display("PASS labelled near-wrap arithmetic");$finish;\n  end\nendmodule\n'
    return body, dict(
        scope="valid seeded near-wrap arithmetic, not huge waiting execution",
        latency=latency,
        timestamp_bits=k,
        absolute_time=now,
        seeded_birth_edges=births,
        **o.events,
    )


def negative(name):
    depth, policy, typ, expected = 3, 0, "logic [12:0]", ""
    latency = 1
    if name.startswith("delayed_"):
        latency = 3
        name = name.removeprefix("delayed_")
    prefix = "#1;"
    trigger = ""
    if name == "latency_zero":
        latency = 0
        expected = "fifo: latency must be positive"
    elif name == "depth_zero":
        depth = 0
        expected = "fifo: depth must be positive"
    elif name == "depth_negative":
        depth = -1
        expected = "fifo: depth must be positive"
    elif name == "policy_sentinel":
        policy = -1
        expected = "fifo: unsupported ready policy"
    elif name == "policy_other":
        policy = 2
        expected = "fifo: unsupported ready policy"
    elif name == "two_state_payload":
        typ = "bit [12:0]"
        expected = "fifo: T must preserve four-state packed payloads"
    elif name == "cold_push":
        trigger = "in_valid=1;#1;clk=1;"
        expected = "fifo: effective transfers must be known"
    elif name == "cold_pop":
        trigger = "out_ready=1;#1;clk=1;"
        expected = "fifo: effective transfers must be known"
    else:
        prefix += "rst=1;#1;clk=1;#1;clk=0;rst=0;#1;"
        if name == "unknown_reset":
            trigger = "rst=1'bx;#1;clk=1;"
            expected = "fifo: reset must be known at the rising edge"
        elif name == "unknown_push":
            trigger = "in_valid=1'bz;#1;clk=1;"
            expected = "fifo: effective transfers must be known"
        elif name == "unknown_pop":
            prefix += "in_valid=1;in_data=13'd37;#1;clk=1;#1;clk=0;in_valid=0;#1;"
            if latency > 1:
                # Old ineligible out_valid masks an unknown take through both
                # aging edges. It becomes effective only after maturation.
                prefix += "out_ready=1'bx;clk=1;#1;clk=0;#1;clk=1;#1;clk=0;#1;"
            trigger = "out_ready=1'bx;#1;clk=1;"
            expected = "fifo: effective transfers must be known"
        else:
            raise AssertionError(name)
    source = f"""module tb;
  typedef {typ} Payload;
  logic clk=0,rst=0,in_valid=0,out_ready=0;
  Payload in_data;
  wire in_ready,out_valid;
  wire Payload out_data;
  fifo #(.T(Payload),.DEPTH({depth}),.READY_POLICY({policy}),
         .AVAILABILITY_LATENCY(64'd{latency})) dut(.clk(clk), .rst(rst), .in_valid(in_valid), .in_data(in_data),
         .out_ready(out_ready), .in_ready(in_ready), .out_valid(out_valid), .out_data(out_data));
  initial begin
    {prefix}{trigger}
    #3;$fatal(1,"EXPECTED_REJECTION_MISSING");
  end
endmodule
"""
    return source, expected


commands = []


def run(command, label):
    result = subprocess.run(
        command, cwd=BASE, text=True, capture_output=True, timeout=40
    )
    (BASE / (label + ".stdout")).write_text(result.stdout)
    (BASE / (label + ".stderr")).write_text(result.stderr)
    commands.append(
        {
            "command": command,
            "cwd": str(BASE),
            "exit_status": result.returncode,
            "stdout": label + ".stdout",
            "stderr": label + ".stderr",
        }
    )
    return result


def test(name, source, expected=None, metadata=None):
    path = BASE / (name + ".sv")
    path.write_text(source)
    assert sha(HELPER) == INITIAL_HELPER_HASH, (
        "helper changed during independent validation"
    )
    compiled = run(
        [
            IVERILOG,
            "-g2012",
            "-s",
            "tb",
            "-o",
            str(BASE / (name + ".vvp")),
            str(HELPER),
            str(path),
        ],
        name + "-compile",
    )
    passed = False
    if compiled.returncode == 0:
        result = run([VVP, str(BASE / (name + ".vvp"))], name + "-run")
        passed = (
            (result.returncode == 0 and "PASS" in result.stdout)
            if expected is None
            else (
                result.returncode != 0
                and expected in result.stdout + result.stderr
                and "EXPECTED_REJECTION_MISSING" not in result.stdout + result.stderr
            )
        )
    row = {
        "name": name,
        "source": str(path),
        "sha256": sha(path),
        "passed": passed,
        "expected_diagnostic": expected,
        "metadata": metadata,
    }
    print(json.dumps(row), flush=True)  # noqa: T201
    return row


results = []
run([IVERILOG, "-V"], "iverilog-version")
run([VVP, "-V"], "vvp-version")
for depth in (1, 2, 3, 5):
    for policy in (0, 1):
        source, meta = positive(depth, policy)
        results.append(test(f"matrix_d{depth}_p{policy}", source, metadata=meta))
for payload, depth in (("bits", 3), ("nested", 5)):
    for policy in (0, 1):
        source, meta = positive(depth, policy, 130, payload)
        results.append(
            test(f"wide_{payload}_d{depth}_p{policy}", source, metadata=meta)
        )
for depth in (1, 2, 3, 5):
    for policy in (0, 1):
        for latency in (1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33):
            source, meta = latency_positive(depth, policy, latency)
            results.append(
                test(f"latency_d{depth}_p{policy}_l{latency}", source, metadata=meta)
            )
for width, payload, depth in (
    (1, "bits", 1),
    (65, "bits", 3),
    (130, "bits", 3),
    (130, "nested", 5),
):
    for policy in (0, 1):
        for latency in (3, 9):
            source, meta = latency_positive(depth, policy, latency, width, payload)
            results.append(
                test(
                    f"raw_{payload}_w{width}_d{depth}_p{policy}_l{latency}",
                    source,
                    metadata=meta,
                )
            )
huge_latencies = sorted(
    {(1 << k) + delta for k in (31, 32, 63) for delta in (-1, 0, 1)}
    | {(1 << 64) - 2, (1 << 64) - 1}
)
for policy in (0, 1):
    for latency in huge_latencies:
        source, meta = huge_early(policy, latency)
        results.append(test(f"huge_early_p{policy}_l{latency}", source, metadata=meta))
        source, meta = near_wrap(policy, latency)
        results.append(
            test(f"seeded_near_wrap_p{policy}_l{latency}", source, metadata=meta)
        )
for name in (
    "latency_zero",
    "depth_zero",
    "depth_negative",
    "policy_sentinel",
    "policy_other",
    "two_state_payload",
    "cold_push",
    "cold_pop",
    "unknown_reset",
    "unknown_push",
    "unknown_pop",
    "delayed_unknown_reset",
    "delayed_unknown_push",
    "delayed_unknown_pop",
):
    source, expected = negative(name)
    results.append(test(name, source, expected))
assert sha(HELPER) == INITIAL_HELPER_HASH, (
    "helper changed during independent validation"
)
receipt = {
    "role": "independent Q6-TH RTL helper tests",
    "configured_model": "gpt-6.1-sol",
    "configured_effort": "high",
    "served_model": "unavailable",
    "original_oracle_actor": "architect/gpt-6-astra/xhigh",
    "oracle": "Python arbitrary-precision absolute birth/maturity edges in a deque",
    "scope": "Direct RTL helper only; no generated/source or Q6 acceptance claim",
    "test": str(Path(__file__).resolve()),
    "test_sha256": sha(__file__),
    "invocation": sys.argv,
    "helper": str(HELPER),
    "helper_sha256": INITIAL_HELPER_HASH,
    "tools": {
        x: {"resolved": str(Path(x).resolve()), "sha256": sha(x)}
        for x in (IVERILOG, VVP)
    },
    "results": results,
    "commands": commands,
}
(BASE / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(  # noqa: T201
    "RESULT", sum(row["passed"] for row in results), "/", len(results), flush=True
)  # noqa: T201
raise SystemExit(0 if all(row["passed"] for row in results) else 1)
