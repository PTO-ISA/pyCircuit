"""Consume a hand-authored logical anchor without a Python DUT or source routing."""

from __future__ import annotations

import hashlib
import json
import os
from collections import deque
from pathlib import Path

import pytest
from test_source_system_execution import _bundle, _cli, _install, _ok, _records, _run

pytestmark = pytest.mark.system


CPP = r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
using Bit = gfsim::wire<gfsim::Bits<1>>;
static Bit bit(unsigned v) {
  return v < 2 ? Bit::known(gfsim::Bits<1>{v})
               : Bit::fromPacked(gfsim::FourState<1>::unknown());
}
static void require(bool v) { if (!v) throw std::runtime_error("reset-value oracle"); }
int main() {
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    dut.drive({bit(0), bit(0), bit(0)});
    dut.system().Build();
    dut.system().Reset();
    auto sample = [&](unsigned clk, unsigned rst, unsigned en, unsigned old) {
      dut.drive({bit(en), bit(clk), bit(rst)});
      require(dut.system().Step() == gfsim::SimStepResult::Running);
      const auto q = dut.sample().q.packed();
      require(q.value().value() == old && q.knownMask().value() == 511
              && q.zMask().value() == 0);
    };
    sample(0, 0, 0, 5); // host initialization
    sample(0, 1, 0, 5); // synchronous reset cannot change Q at low clock
    sample(1, 1, 0, 5); // Work observes old Q, then reset commits zero
    sample(0, 0, 0, 0);
    sample(1, 0, 1, 0); // normal increment commits one
    sample(0, 0, 0, 1);
    sample(1, 0, 0, 1); // false guard holds
    sample(0, 0, 0, 1);
    auto epoch = dut.system().cycle();
    dut.drive({bit(2), bit(1), bit(0)});
    require(dut.system().Step() == gfsim::SimStepResult::Failed);
    require(dut.system().cycle() == epoch);
    // Host Reset restores init=5, not the synchronous reset target zero.
    dut.drive({bit(0), bit(0), bit(0)});
    dut.system().Reset();
    sample(0, 0, 0, 5);
  }
}
"""


RTL = r"""
module state_reset_probe;
  logic clk=0, rst=0, enable=0, permit=0;
  logic [2:0] phase=0;
  wire [8:0] q;
  wire complete, sampling_error, error;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .enable(enable), .q(q), .pyc_phase(phase),
    .pyc_root_commit_ok(permit), .pyc_check_complete(complete),
    .pyc_sampling_error(sampling_error), .pyc_local_error(error));
  task host_reset;
    permit=1; phase=4; #1; phase=2; #1; phase=0; permit=0; #1;
    if(q !== 9'd5) $fatal(1,"host initialization differs");
  endtask
  task step(input logic c, input logic r, input logic e,
            input integer old_q, input integer new_q);
    clk=c; rst=r; enable=e; #1; phase=1; #1;
    if(q !== old_q || error !== 0 || sampling_error !== 0 || complete !== 1)
      $fatal(1,"prepare or old snapshot differs");
    permit=1; phase=2; #1; phase=0; permit=0; #1;
    if(q !== new_q) $fatal(1,"committed value differs");
  endtask
  initial begin
    host_reset();
    step(0,0,0,5,5);
    step(0,1,0,5,5);
    step(1,1,0,5,0);
    step(0,0,0,0,0);
    step(1,0,1,0,1);
    step(0,0,0,1,1);
    step(1,0,0,1,1);
    step(0,0,0,1,1);
    clk=1; enable=1'bx; #1; phase=1; #1;
    if(error !== 1 || q !== 9'd1) $fatal(1,"missing four-state failure");
    permit=0; phase=3; #1; phase=0; #1;
    if(q !== 9'd1) $fatal(1,"failed work changed state");
    // Same high sample after explicit discard remains an uncommitted edge.
    step(1,0,1,1,2);
    clk=0; rst=0; enable=0; host_reset();
    $display("STATE_RESET_VALUE_OK"); $finish;
  end
endmodule
"""


PHYSICAL_RTL = r"""
module state_reset_physical;
  logic clk=0, rst=0, enable=0;
  wire [8:0] q;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .enable(enable), .q(q));
  task check(input integer expected);
    #1;
    if(q !== expected) $fatal(1,"physical reset-value oracle");
  endtask
  task batch(input logic c,r,e,input integer expected);
    enable=e;#1;clk<=c;rst<=r;check(expected);
  endtask
  initial begin
    check(5);
    batch(0,1,0,5); // synchronous assertion between edges holds
    batch(1,1,0,0);
    batch(0,0,1,0);
    batch(1,0,1,1);
    batch(0,0,0,1);
    batch(1,0,0,1);
    batch(0,0,1'bx,1);
    batch(1,0,1'bx,1); // knownness failure prevents the edge from committing
    batch(0,0,1,1);
    batch(1,0,1,1); // physical failure is sticky
    batch(0,1,1,1);
    batch(1,1,1,1); // physical reset cannot recover a failed simulation
    $display("STATE_RESET_PHYSICAL_OK"); $finish;
  end
endmodule
"""


def _simulate_rtl(generated, bench, binary, top):
    receipt = json.loads((generated / "generated.json").read_text())
    sources = [
        generated / row["path"] for row in receipt["files"] if row["role"] == "rtl"
    ]
    standard = _install() / "include/verilog"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                top,
                "-o",
                str(binary),
                str(standard / "dff.v"),
                str(standard / "dffe.v"),
                *map(str, sources),
                str(bench),
            ]
        )
    )
    return _ok(_run(["vvp", str(binary)]))


ASYNC_CPP = r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
using Bit = gfsim::wire<gfsim::Bits<1>>;
static Bit bit(unsigned v) {
  return v < 2 ? Bit::known(gfsim::Bits<1>{v})
               : Bit::fromPacked(gfsim::FourState<1>::unknown());
}
static void require(bool v) { if (!v) throw std::runtime_error("async oracle"); }
int main() {
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    dut.drive({bit(0),bit(0),bit(1)});
    dut.system().Build(); dut.system().Reset();
    auto step = [&](unsigned c,unsigned r,unsigned e,unsigned old) {
      dut.drive({bit(e),bit(c),bit(r)});
      require(dut.system().Step()==gfsim::SimStepResult::Running);
      auto q=dut.sample().q.packed();
      require(q.value().value()==old && q.knownMask().value()==511);
    };
    step(1,1,1,5); step(0,1,1,5); // only falling edge increments
    step(0,0,0,6); step(0,1,0,0); // reset without any clock edge
    step(1,1,0,0); step(0,1,0,0); // held by normal admission
    step(1,1,1,0); step(0,1,1,0); step(0,1,0,1);
    auto epoch=dut.system().cycle();
    dut.drive({bit(0),bit(0),bit(2)}); // unknown async reset with no clock event
    require(dut.system().Step()==gfsim::SimStepResult::Failed);
    require(dut.system().cycle()==epoch);
    dut.drive({bit(0),bit(0),bit(1)}); dut.system().Reset();
    step(0,1,0,5);
  }
}
"""


ASYNC_MANAGED_RTL = r"""
module async_state_probe;
  logic clk=0,rst=1,enable=0,permit=0; logic[2:0]phase=0;
  wire[8:0]q; wire complete,sampling_error,error;
  pyc_root dut(.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),
    .enable(enable),.q(q),.pyc_phase(phase),.pyc_root_commit_ok(permit),
    .pyc_check_complete(complete),.pyc_sampling_error(sampling_error),.pyc_local_error(error));
  task step(input logic c,r,e,input integer old_q,new_q);
    clk=c;rst=r;enable=e;#1;phase=1;#1;
    if(q!==old_q || error!==0 || sampling_error!==0 || complete!==1)$fatal(1,"async prepare");
    permit=1;phase=2;#1;phase=0;permit=0;#1;
    if(q!==new_q)$fatal(1,"async commit");
  endtask
  initial begin
    permit=1;phase=4;#1;phase=2;#1;phase=0;permit=0;#1;
    if(q!==9'd5)$fatal;
    step(1,1,1,5,5);step(0,1,1,5,6);
    step(0,0,0,6,0);step(0,1,0,0,0);
    step(1,1,0,0,0);step(0,1,0,0,0);
    step(1,1,1,0,0);step(0,1,1,0,1);step(0,1,0,1,1);
    rst=1'bx;#1;phase=1;#1;
    if(error!==1)$fatal(1,"missing asynchronous reset fault");
    phase=3;#1;phase=0;#1;if(q!==9'd1)$fatal;
    $display("ASYNC_STATE_OK");$finish;
  end
endmodule
"""


ASYNC_PHYSICAL_RTL = r"""
module async_state_physical;
  logic clk=0,rst=1,enable=0; wire[8:0]q;
  logic bad_clock=1,bad_reset=0;wire[8:0]bad_q;
  pyc_root dut(.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),.enable(enable),.q(q));
  pyc_root invalid_start(.pyc_7079635f636c6b(bad_clock),.pyc_7079635f727374(bad_reset),
    .enable(1'b0),.q(bad_q));
  task batch(input logic c,r,e,input integer expected);
    enable=e;#1;clk<=c;rst<=r;bad_clock<=c;bad_reset<=r;#1;
    if(q!==expected)$fatal(1,"async physical batch");
    if(bad_q!==9'd5)$fatal(1,"startup failure committed asynchronous reset");
  endtask
  initial begin
    #1;if(q!==9'd5 || bad_q!==9'd5)$fatal;
    batch(1,1,1,5);batch(0,1,1,6);
    batch(0,0,0,0);batch(0,1,0,0);
    batch(1,1,0,0);batch(0,1,0,0);
    batch(1,1,1,0);batch(0,1,1,1);
    batch(0,1'bx,0,1); // asynchronous fault without clock edge
    batch(1,1,1,1);batch(0,1,1,1); // sticky fault forbids recovery
    $display("ASYNC_PHYSICAL_OK");$finish;
  end
endmodule
"""


@pytest.mark.parametrize("boundary", [False, True], ids=["direct", "entry_boundary"])
def test_native_contract_async_negative_public_consumers(
    tmp_path, monkeypatch, boundary
):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = (
        "PYCIRCUIT_TEST_BOUNDARY_STATE_ANCHOR"
        if boundary
        else "PYCIRCUIT_TEST_ASYNC_STATE_ANCHOR"
    )
    fixture = (
        "PublishNativeEntryBoundaryForIndependentConsumers"
        if boundary
        else "PublishAsyncNegativeStateForIndependentConsumers"
    )
    monkeypatch.setenv(variable, str(anchor))
    _ok(
        _run(
            [
                str(producer),
                f"--gtest_filter=StateResetValueTest.{fixture}",
            ]
        )
    )
    monkeypatch.delenv(variable)
    manifest = json.loads((anchor / "anchor.json").read_text())
    assert (manifest["schema"], manifest["routed_contract"]) == (6, 7)
    configuration = manifest["configuration"]
    assert configuration["root"]["definition"] == '@"reset_value.fixture.Held"'
    if boundary:
        assert (
            configuration["implementation_root"]["definition"]
            == '@"pycircuit.__link_entry__"'
        )
    else:
        assert "implementation_root" not in configuration
    original = {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = tmp_path / "final.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    identity = hashlib.sha256(final.read_bytes()).hexdigest()
    cpp = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(cpp)))
    (cpp / "async_probe.cpp").write_text(ASYNC_CPP)
    with (cpp / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(async_probe async_probe.cpp)\ntarget_link_libraries(async_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(cpp),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(_run(["cmake", "--build", str(build), "--target", "async_probe"]))
    _ok(_run([str(next(build.rglob("async_probe")))]))
    for mode, bench_text, top, marker in (
        ("managed", ASYNC_MANAGED_RTL, "async_state_probe", "ASYNC_STATE_OK"),
        ("physical", ASYNC_PHYSICAL_RTL, "async_state_physical", "ASYNC_PHYSICAL_OK"),
    ):
        generated = tmp_path / mode
        _ok(
            _cli(
                "emit",
                str(final),
                "--target",
                "verilog",
                "--rtl-mode",
                mode,
                "-o",
                str(generated),
            )
        )
        ports = json.loads((generated / "ports.json").read_text())
        assert ports["schema_version"] == 2
        q_port = next(row for row in ports["outputs"] if row["name"] == "q")
        assert (q_port["domain"], q_port["clock_input"]) == ("D", 1)
        assert "reset" in ports["inputs"][2]["roles"]
        endpoints = ports["domains"]["endpoints"]
        assert endpoints and all(row["edge"] == "negedge" for row in endpoints)
        for row in endpoints:
            assert row["clock_input"] == 1
            assert row["resets"]
            for reset in row["resets"]:
                assert (reset["kind"], reset["polarity"], reset["input_ordinal"]) == (
                    "async",
                    "low",
                    2,
                )
                assert reset["input_dependencies"] == [2]
        if mode == "physical":
            _compile_synthesis(generated, tmp_path / "synthesis.vvp")
        bench = tmp_path / f"{mode}.sv"
        bench.write_text(bench_text)
        transcript = _simulate_rtl(generated, bench, tmp_path / f"{mode}.vvp", top)
        assert marker in transcript
        if mode == "physical":
            assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 2
    assert hashlib.sha256(final.read_bytes()).hexdigest() == identity
    assert original == {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }


def _fifo_rows():
    rows = [(0, 0, 1), (0, 0, 0)]
    stopped_release = set()
    for phase in range(7):
        if phase == 4:
            rows.extend([(0, 0, 1), (0, 0, 0)])
        for _ in range(32 if phase == 3 else 12):
            if phase == 4:
                stopped_release.update((len(rows), len(rows) + 1))
            rows.append((0, 0, 0))
            rows.append(
                (1, 0, 0)
                if phase in (1, 4)
                else (0, 1, 0) if phase in (2, 6) else (1, 1, 0)
            )
    return rows, stopped_release


def _build_native_probe(final, root, source):
    generated = root / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(generated)))
    if callable(source):
        source = source(json.loads((generated / "ports.json").read_text()))
    (generated / "probe.cpp").write_text(source)
    with (generated / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(probe probe.cpp)\ntarget_link_libraries(probe PRIVATE pycircuit_modules)\n"
        )
    build = root / "build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(generated),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(_run(["cmake", "--build", str(build), "--target", "probe"]))
    _ok(_run([str(next(build.rglob("probe")))]))


MIXED_CPP = r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
using Bit=gfsim::wire<gfsim::Bits<1>>;
static Bit b(unsigned x){return Bit::known(gfsim::Bits<1>{x});}
static void require(bool v){if(!v)throw std::runtime_error("mixed reset oracle");}
int main(){for(unsigned workers:{1u,2u}){
 pyc_dut dut(workers);dut.drive({b(0),b(0),b(1)});dut.system().Build();dut.system().Reset();
 auto step=[&](unsigned c,unsigned r,unsigned e,unsigned a,unsigned held){
  dut.drive({b(e),b(c),b(r)});require(dut.system().Step()==gfsim::SimStepResult::Running);
  auto q=dut.sample();require(q.q.isFullyKnown()&&q.retained.isFullyKnown());
  require(q.q.value().value()==a&&q.retained.value().value()==held);
 };
 step(1,1,1,5,5);step(0,1,1,5,5);step(0,1,0,6,6);
 step(0,0,1,6,6);step(1,0,1,0,6);step(0,0,1,0,6);
 step(1,1,1,0,6);step(0,1,1,0,6);step(0,1,0,1,1);
}}
"""


def _mixed_rtl(physical):
    ports = (
        ""
        if physical
        else ",.pyc_phase(phase),.pyc_root_commit_ok(permit),.pyc_check_complete(complete),.pyc_sampling_error(sampling_error),.pyc_local_error(error)"
    )
    action = (
        "enable=e;#1;clk<=c;rst<=r;#1;"
        if physical
        else "enable=e;clk=c;rst=r;#1;phase=1;#1;if(error!==0||sampling_error!==0||complete!==1)$fatal;permit=1;phase=2;#1;phase=0;permit=0;#1;"
    )
    initialize = (
        "#1;" if physical else "permit=1;phase=4;#1;phase=2;#1;phase=0;permit=0;#1;"
    )
    return f"""module mixed_reset_probe;
logic clk=0,rst=1,enable=0,permit=0;logic[2:0]phase=0;
wire[8:0]q,retained;wire complete,sampling_error,error;
pyc_root dut(.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),.enable(enable),.q(q),.retained(retained){ports});
task step(input logic c,r,e,input integer a,held);
 {action}
 if(q!==a||retained!==held)$fatal(1,"mixed reset committed values");
endtask
initial begin
 {initialize}
 if(q!==9'd5||retained!==9'd5)$fatal;
 step(1,1,1,5,5);step(0,1,1,6,6);step(0,1,0,6,6);
 step(0,0,1,0,6);step(1,0,1,0,6);step(0,0,1,0,6);
 step(1,1,1,0,6);step(0,1,1,1,1);
 $display("MIXED_RESET_OK");$finish;
end
endmodule
"""


def test_mixed_reset_whole_transaction_public_consumers(tmp_path, monkeypatch):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    monkeypatch.setenv("PYCIRCUIT_TEST_MIXED_STATE_ANCHOR", str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest.PublishMixedResetStateForIndependentConsumers",
            ]
        )
    )
    monkeypatch.delenv("PYCIRCUIT_TEST_MIXED_STATE_ANCHOR")
    final = tmp_path / "mixed.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    _build_native_probe(final, tmp_path, MIXED_CPP)
    for mode in ("managed", "physical"):
        generated = tmp_path / mode
        _ok(
            _cli(
                "emit",
                str(final),
                "--target",
                "verilog",
                "--rtl-mode",
                mode,
                "-o",
                str(generated),
            )
        )
        if mode == "physical":
            _compile_synthesis(generated, tmp_path / "synthesis.vvp")
        bench = tmp_path / f"{mode}.sv"
        bench.write_text(_mixed_rtl(mode == "physical"))
        assert "MIXED_RESET_OK" in _simulate_rtl(
            generated, bench, tmp_path / f"{mode}.vvp", "mixed_reset_probe"
        )
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def _fifo_scoreboard(
    records, *, zero_payload=False, push_instance=None, pop_event="fifo pop"
):
    rows, stopped_release = _fifo_rows()
    observations = {}
    for record in records:
        if record.get("kind") != "log":
            continue
        event = record["spec"]["event"]
        if event not in ("fifo push", pop_event):
            continue
        if event == "fifo push" and push_instance is not None:
            if record["instance"] != push_instance:
                continue
        values = tuple(int(value["value"]) for value in record["values"])
        if event == pop_event:
            event = "fifo pop"
        key = (int(record["evaluation_epoch"]), event)
        assert key not in observations
        observations[key] = values
    queue = deque()
    previous = (0, 0, 0)
    next_token = pushes = pops = max_depth = 0
    for epoch, row in enumerate(rows):
        if row[2]:
            queue.clear()
            next_token = 0
        push = observations.get((epoch, "fifo push"))
        pop = observations.get((epoch, "fifo pop"))
        if epoch in stopped_release:
            assert push is None and pop is None, "stopped peer must prevent release"
        # Process reads against the old queue: same-epoch writes cannot bypass.
        if pop is not None:
            if pop_event == "scalar fifo pop":
                value, clock, reset = pop
            else:
                value, checksum, clock, reset = pop
            assert (clock, reset) == (row[1], row[2])
            if clock and not previous[1] and not reset:
                assert queue, "accepted read from an empty queue"
                assert value == queue.popleft(), "FIFO reordered or duplicated data"
                if pop_event != "scalar fifo pop":
                    expected_checksum = 0 if zero_payload else value ^ 511
                    assert (
                        checksum == expected_checksum
                    ), "FIFO corrupted a separate payload field"
                pops += 1
        if push is not None:
            value, clock, reset = push
            assert (clock, reset) == (row[0], row[2])
            if clock and not previous[0] and not reset:
                assert (
                    value == next_token
                ), "producer advanced without a successful push"
                next_token = (next_token + 1) & 511
                queue.append(0 if zero_payload else value)
                pushes += 1
                assert len(queue) <= 4
                max_depth = max(max_depth, len(queue))
        previous = row
    assert pushes > 20 and pops > 16, "resource failed to make progress or wrap"
    assert max_depth == 4, "write-only phase did not exercise capacity/backpressure"
    assert not queue, "final read-only phase failed to drain accepted data"
    return observations


@pytest.mark.parametrize("generic", [False, True], ids=["packet", "generic_packet"])
def test_fifo_system_saved_anchor_independent_scoreboard(
    tmp_path, monkeypatch, generic
):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = (
        "PYCIRCUIT_TEST_GENERIC_FIFO_ANCHOR"
        if generic
        else "PYCIRCUIT_TEST_FIFO_ANCHOR"
    )
    fixture = (
        "PublishGenericFIFOSystemForIndependentScoreboard"
        if generic
        else "PublishFIFOSystemForIndependentScoreboard"
    )
    monkeypatch.setenv(variable, str(anchor))
    _ok(_run([str(producer), f"--gtest_filter=StateResetValueTest.{fixture}"]))
    monkeypatch.delenv(variable)
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = tmp_path / "fifo.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    traces = [
        _fifo_scoreboard(records, zero_payload=generic)
        for records in _system_target_records(final, tmp_path)
    ]
    assert traces[0] == traces[1] == traces[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def test_mixed_actual_fifo_saved_anchor_independent_scoreboards(tmp_path, monkeypatch):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = "PYCIRCUIT_TEST_MIXED_ACTUAL_FIFO_ANCHOR"
    monkeypatch.setenv(variable, str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest."
                "PublishMixedActualFIFOSystemForIndependentScoreboards",
            ]
        )
    )
    monkeypatch.delenv(variable)
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = tmp_path / "mixed_fifo.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    traces = []
    for records in _system_target_records(final, tmp_path):
        packet = _fifo_scoreboard(
            records, zero_payload=True, push_instance="root/generic_fifo"
        )
        scalar = _fifo_scoreboard(
            records,
            zero_payload=True,
            push_instance="root/scalar_fifo",
            pop_event="scalar fifo pop",
        )
        # Each scoreboard independently requires pushes, reads, wrap, full,
        # backpressure and final drain; an unused specialization cannot pass.
        traces.append((packet, scalar))
    assert traces[0] == traces[1] == traces[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def _system_target_records(final, tmp_path):
    cpp = tmp_path / "cpp"
    cpp.mkdir()
    executable = _bundle(final, cpp, "cpp")
    traces = []
    for workers in (1, 2):
        records = _records(_ok(_run([str(executable), "--workers", str(workers)])))
        traces.append(records)
    rtl = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(rtl)))
    # Use the generated System driver and its finite multi-domain rows, with a
    # true four-state engine regardless of the current trace's known payloads.
    receipt = json.loads((rtl / "generated.json").read_text())
    sources = [rtl / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
    drivers = [
        rtl / row["path"]
        for row in receipt["files"]
        if Path(row["path"]).name == "simulation_top.sv"
    ]
    assert len(drivers) == 1
    sources.extend(drivers)
    executable_rtl = tmp_path / "system.vvp"
    standard = _install() / "include/verilog"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "pycircuit_sim",
                "-o",
                str(executable_rtl),
                str(standard / "dff.v"),
                str(standard / "dffe.v"),
                *map(str, sources),
            ]
        )
    )
    traces.append(_records(_ok(_run(["vvp", str(executable_rtl)]))))
    return traces


def _async_reg_trace(records):
    observed = {}
    for record in records:
        if record.get("kind") != "log" or record["spec"]["event"] != "async level":
            continue
        epoch = int(record["evaluation_epoch"])
        assert epoch not in observed
        observed[epoch] = tuple(int(value["value"]) for value in record["values"])
    # Source clock stays low for every row. Stable-level eventual visibility is
    # independent of that clock and does not assert an internal stage count.
    rows = [(0, 0, 0, 1), (0, 0, 0, 0)]
    checkpoints = []
    for phase in range(4):
        if phase == 2:
            rows.extend([(0, 0, 0, 1), (0, 0, 0, 0)])
        if phase == 3:
            rows.extend([(1, 0, 0, 0), (0, 0, 0, 0)])
        for _ in range(10):
            level = int(phase == 1)
            rows.extend([(level, 0, 0, 0), (level, 0, 1, 0)])
        checkpoints.append((len(rows) - 1, int(phase == 1)))
    for epoch, (value, clock, reset) in observed.items():
        assert (clock, reset) == rows[epoch][2:]
        assert value in (0, 1)
    for epoch, expected in checkpoints:
        assert epoch in observed, "target observation made no progress"
        assert observed[epoch][0] == expected
    # A short pulse with no target edge is not a queued event.
    pulse_start = checkpoints[2][0] + 1
    assert all(
        value[0] == 0 for epoch, value in observed.items() if epoch >= pulse_start
    )
    return observed


def test_async_reg_system_saved_anchor_independent_trace(tmp_path, monkeypatch):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    monkeypatch.setenv("PYCIRCUIT_TEST_ASYNC_REG_ANCHOR", str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest.PublishAsyncRegSystemForIndependentTrace",
            ]
        )
    )
    monkeypatch.delenv("PYCIRCUIT_TEST_ASYNC_REG_ANCHOR")
    final = tmp_path / "async_reg.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    traces = [
        _async_reg_trace(records) for records in _system_target_records(final, tmp_path)
    ]
    assert traces[0] == traces[1] == traces[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


PHYSICAL_BARRIER_RTL = r"""
module physical_barrier_probe;
  logic af=0,aslow=0,bf=0,bslow=0,rst=0;
  logic bad_fast=1,bad_slow=0;
  pyc_root a(.write_clock(af),.pyc_7079635f636c6b(aslow),.pyc_7079635f727374(rst));
  pyc_root b(.write_clock(bf),.pyc_7079635f636c6b(bslow),.pyc_7079635f727374(rst));
  pyc_root invalid_start(.write_clock(bad_fast),.pyc_7079635f636c6b(bad_slow),.pyc_7079635f727374(rst));
  task check(input integer expected);
    if(a.dut.pyc_instance_fast.q!==expected || a.dut.pyc_instance_slow.q!==expected ||
       b.dut.pyc_instance_fast.q!==expected || b.dut.pyc_instance_slow.q!==expected)
      $fatal(1,"multi-domain commit barrier or drive order changed observable Q");
    if(invalid_start.dut.pyc_instance_fast.q!==9'd5 ||
       invalid_start.dut.pyc_instance_slow.q!==9'd5)
      $fatal(1,"invalid initial clock committed state");
  endtask
  task batch(input logic level,input integer expected);
    #1;af<=level;aslow<=level; // same-region batch, fast written first
    bslow<=level;bf<=level;   // equivalent batch, slow written first
    bad_fast<=level;bad_slow<=level; // rejected startup remains frozen
    #1;check(expected);
  endtask
  initial begin
    #1;check(5);
    batch(1,6);batch(0,6);batch(1,7);batch(0,7);
    batch(1,7); // only slow has X guard; fast must not commit eight
    batch(0,7);batch(1,7); // sticky failure freezes both domains
    $display("PHYSICAL_BARRIER_OK");$finish;
  end
endmodule
"""


BARRIER_CPP = r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
using Bit=gfsim::wire<gfsim::Bits<1>>;
static Bit b(unsigned x){return Bit::known(gfsim::Bits<1>{x});}
int main(){for(unsigned workers:{1u,2u}){
  pyc_dut dut(workers);dut.drive({b(0),b(0),b(0)});
  dut.system().Build();dut.system().Reset();
  for(unsigned level:{0u,1u,0u,1u,0u}){
    dut.drive({b(level),b(level),b(0)});
    if(dut.system().Step()!=gfsim::SimStepResult::Running)
      throw std::runtime_error("early multi-domain failure");
  }
  auto epoch=dut.system().cycle();dut.drive({b(1),b(1),b(0)});
  if(dut.system().Step()!=gfsim::SimStepResult::Failed || dut.system().cycle()!=epoch)
    throw std::runtime_error("failed multi-domain epoch committed");
}}
"""


MANAGED_BARRIER_RTL = r"""
module managed_barrier_probe;
  logic fast=0,slow=0,rst=0,permit=0;
  logic[2:0]phase=0;wire complete,sampling_error,error;
  pyc_root dut(.write_clock(fast),.pyc_7079635f636c6b(slow),.pyc_7079635f727374(rst),
    .pyc_phase(phase),.pyc_root_commit_ok(permit),.pyc_check_complete(complete),
    .pyc_sampling_error(sampling_error),.pyc_local_error(error));
  task check(input integer expected);
    if(dut.dut.pyc_instance_fast.q!==expected || dut.dut.pyc_instance_slow.q!==expected)
      $fatal(1,"managed multi-domain Q differs");
  endtask
  task commit;
    permit=1;phase=2;#1;phase=0;permit=0;#1;
  endtask
  task step(input logic level,input integer expected);
    fast=level;slow=level;#1;phase=1;#1;
    if(error!==0 || sampling_error!==0 || complete!==1)$fatal(1,"prepare failed");
    commit();check(expected);
  endtask
  initial begin
    phase=4;#1;commit();check(5);
    step(0,5);step(1,6);step(0,6);step(1,7);step(0,7);
    fast=1;slow=1;#1;phase=1;#1;
    if(error!==1)$fatal(1,"slow X guard did not fail");
    check(7);phase=3;#1;phase=0;#1;check(7);
    // Retry the same high levels with synchronous reset. If either clock
    // history committed on failure, that child's reset edge would be lost.
    rst=1;step(1,0);
    $display("MANAGED_BARRIER_OK");$finish;
  end
endmodule
"""


def test_two_domain_event_batch_has_one_commit_barrier(tmp_path, monkeypatch):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    monkeypatch.setenv("PYCIRCUIT_TEST_PHYSICAL_BARRIER_ANCHOR", str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest.PublishTwoDomainPhysicalBarrierForIndependentTrace",
            ]
        )
    )
    monkeypatch.delenv("PYCIRCUIT_TEST_PHYSICAL_BARRIER_ANCHOR")
    final = tmp_path / "barrier.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    _build_native_probe(final, tmp_path, BARRIER_CPP)
    for mode, source, top, marker in (
        ("managed", MANAGED_BARRIER_RTL, "managed_barrier_probe", "MANAGED_BARRIER_OK"),
        (
            "physical",
            PHYSICAL_BARRIER_RTL,
            "physical_barrier_probe",
            "PHYSICAL_BARRIER_OK",
        ),
    ):
        generated = tmp_path / mode
        _ok(
            _cli(
                "emit",
                str(final),
                "--target",
                "verilog",
                "--rtl-mode",
                mode,
                "-o",
                str(generated),
            )
        )
        ports = json.loads((generated / "ports.json").read_text())
        assert ports["schema_version"] == 2
        assert {row["clock_input"] for row in ports["domains"]["endpoints"]} == {0, 1}
        if mode == "physical":
            _compile_synthesis(generated, tmp_path / "synthesis.vvp")
        bench = tmp_path / f"{mode}.sv"
        bench.write_text(source)
        transcript = _simulate_rtl(generated, bench, tmp_path / f"{mode}.vvp", top)
        assert marker in transcript
        if mode == "physical":
            assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 3
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def test_ir_anchor_reset_value_public_consumers(tmp_path, monkeypatch):
    build = os.environ.get("PYC_BUILD_DIR")
    assert build, "set PYC_BUILD_DIR to this candidate's build"
    producer = Path(build) / "bin/ACIRAnchorArtifactTests"
    assert producer.is_file(), producer
    anchor = tmp_path / "anchor"
    monkeypatch.setenv("PYCIRCUIT_TEST_STATE_RESET_ANCHOR", str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest."
                "SavedLogicalStateRetainsDistinctInitializationAndReset",
            ]
        )
    )
    monkeypatch.delenv("PYCIRCUIT_TEST_STATE_RESET_ANCHOR")
    manifest = json.loads((anchor / "anchor.json").read_text())
    assert (manifest["schema"], manifest["routed_contract"]) == (6, 7)
    original = {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = tmp_path / "final.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    identity = hashlib.sha256(final.read_bytes()).hexdigest()
    cpp = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(cpp)))
    (cpp / "state_reset_probe.cpp").write_text(CPP)
    with (cpp / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(state_reset_probe state_reset_probe.cpp)\n"
            "target_link_libraries(state_reset_probe PRIVATE pycircuit_modules)\n"
        )
    consumer = tmp_path / "consumer-build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(cpp),
                "-B",
                str(consumer),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(_run(["cmake", "--build", str(consumer), "--target", "state_reset_probe"]))
    _ok(_run([str(next(consumer.rglob("state_reset_probe")))]))
    rtl = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(rtl)))
    bench = tmp_path / "state_reset_probe.sv"
    bench.write_text(RTL)
    binary = tmp_path / "state_reset_probe.vvp"
    assert "STATE_RESET_VALUE_OK" in _simulate_rtl(
        rtl, bench, binary, "state_reset_probe"
    )
    physical = tmp_path / "physical"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(physical),
        )
    )
    physical_bench = tmp_path / "state_reset_physical.sv"
    physical_bench.write_text(PHYSICAL_RTL)
    transcript = _simulate_rtl(
        physical, physical_bench, tmp_path / "physical.vvp", "state_reset_physical"
    )
    assert "STATE_RESET_PHYSICAL_OK" in transcript
    assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 1
    assert hashlib.sha256(final.read_bytes()).hexdigest() == identity
    assert original == {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }


CDC_ROLLBACK_CPP = r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
using Bit=gfsim::wire<gfsim::Bits<1>>;
static Bit bit(unsigned x){return x<2 ? Bit::known(gfsim::Bits<1>{x}) :
  Bit::fromPacked(gfsim::FourState<1>::unknown());}
static void require(bool v){if(!v)throw std::runtime_error("CDC rollback oracle");}
int main(){for(unsigned workers:{1u,2u}){
  gfsim::WorkExecutor executor(workers);
  pyc_root reference("reference",&executor),replay("replay",&executor);
  auto pins=[](pyc_root &root,unsigned clock,unsigned reset,unsigned enable){
    root.PYC_CLOCK=bit(clock);root.PYC_RESET=bit(reset);root.PYC_ENABLE=bit(enable);
  };
  pins(reference,0,1,0);pins(replay,0,1,0);
  reference.Reset();replay.Reset();reference.Xfer();replay.Xfer();
  auto epoch=[&](unsigned rc,unsigned tc,unsigned reset,unsigned re,unsigned te,bool fail=false){
    pins(reference,rc,reset,re);pins(replay,tc,reset,te);
    reference.Work();replay.Work();
    require(reference.q.packed().parityEquivalent(replay.q.packed()));
    require(reference.synced.packed().parityEquivalent(replay.synced.packed()));
    gfsim::SimFailureInfo referenceFailure{},replayFailure{};
    require(reference.pyc_validate_checks(referenceFailure));
    require(replay.pyc_validate_checks(replayFailure)!=fail);
    reference.Xfer();if(fail)replay.DiscardNext();else replay.Xfer();
  };
  epoch(0,0,0,0,0);epoch(1,1,1,1,1);
  epoch(0,0,1,1,1);epoch(1,1,1,1,1);
  epoch(1,0,1,1,2,true); // failed second release event
  epoch(0,0,1,1,1);      // replay that exact event
  for(unsigned i=0;i<10;++i){epoch(1,1,1,1,1);epoch(0,0,1,1,1);}
  epoch(1,1,1,1,1);require(reference.synced.packed().isFullyKnown());
  require(reference.synced.value().value()==1);
  epoch(1,0,1,1,2,true); // failed X data capture after release
  epoch(0,0,1,0,0);
  for(unsigned i=0;i<10;++i){epoch(1,1,1,0,0);epoch(0,0,1,0,0);}
  epoch(1,1,1,0,0);require(reference.synced.packed().isFullyKnown());
  require(reference.synced.value().value()==0);
}}
"""


def _cdc_rollback_cpp(ports):
    names = {row["name"]: row["wire"] for row in ports["inputs"]}
    return (
        CDC_ROLLBACK_CPP.replace("PYC_CLOCK", names["pyc_clk"])
        .replace("PYC_RESET", names["pyc_rst"])
        .replace("PYC_ENABLE", names["enable"])
    )


CDC_ROLLBACK_RTL = r"""
module cdc_rollback_probe;
  logic rc=0,tc=0,rst=1,re=0,te=0,rpermit=0,tpermit=0;
  logic[2:0]rp=0,tp=0;wire[8:0]rq,tq;wire rs,ts;
  wire rcomplete,tcomplete,rerror,terror,rsampling,tsampling;
  pyc_root reference(.pyc_7079635f636c6b(rc),.pyc_7079635f727374(rst),.enable(re),.q(rq),.synced(rs),
    .pyc_phase(rp),.pyc_root_commit_ok(rpermit),.pyc_check_complete(rcomplete),
    .pyc_sampling_error(rsampling),.pyc_local_error(rerror));
  pyc_root replay(.pyc_7079635f636c6b(tc),.pyc_7079635f727374(rst),.enable(te),.q(tq),.synced(ts),
    .pyc_phase(tp),.pyc_root_commit_ok(tpermit),.pyc_check_complete(tcomplete),
    .pyc_sampling_error(tsampling),.pyc_local_error(terror));
  task same;
    if(rq!==tq || rs!==ts)$fatal(1,"discarded CDC epoch changed later behavior");
  endtask
  task epoch(input logic rclock,tclock,reset,ren,ten,failed=0);
    rc=rclock;tc=tclock;rst=reset;re=ren;te=ten;#1;rp=1;tp=1;#1;
    same();
    if(rerror!==0 || rsampling!==0 || rcomplete!==1 || tcomplete!==1)
      $fatal(1,"incomplete CDC work");
    if(terror!==failed || tsampling!==0)$fatal(1,"unexpected CDC precheck result");
    rpermit=1;rp=2;tpermit=!failed;tp=failed?3:2;#1;
    rp=0;tp=0;rpermit=0;tpermit=0;#1;same();
  endtask
  initial begin
    rp=4;tp=4;#1;rpermit=1;tpermit=1;rp=2;tp=2;#1;
    rp=0;tp=0;rpermit=0;tpermit=0;#1;same();
    epoch(0,0,0,0,0);epoch(1,1,1,1,1);
    epoch(0,0,1,1,1);epoch(1,1,1,1,1);
    epoch(1,0,1,1,1'bx,1);epoch(0,0,1,1,1);
    repeat(10)begin epoch(1,1,1,1,1);epoch(0,0,1,1,1);end
    if(rs!==1)$fatal(1,"CDC failed to release and convey stable one");
    epoch(1,1,1,1,1);epoch(1,0,1,1,1'bx,1);epoch(0,0,1,0,0);
    repeat(10)begin epoch(1,1,1,0,0);epoch(0,0,1,0,0);end
    if(rs!==0)$fatal(1,"CDC failed to convey stable zero");
    $display("CDC_ROLLBACK_OK");$finish;
  end
endmodule
"""


def test_cdc_release_and_data_rollback_match_successful_replay(tmp_path, monkeypatch):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = "PYCIRCUIT_TEST_CDC_ROLLBACK_ANCHOR"
    monkeypatch.setenv(variable, str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest.PublishCDCRollbackForIndependentReplay",
            ]
        )
    )
    monkeypatch.delenv(variable)
    final = tmp_path / "rollback.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    _build_native_probe(final, tmp_path, _cdc_rollback_cpp)
    generated = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(generated)))
    bench = tmp_path / "rollback.sv"
    bench.write_text(CDC_ROLLBACK_RTL)
    assert "CDC_ROLLBACK_OK" in _simulate_rtl(
        generated, bench, tmp_path / "rollback.vvp", "cdc_rollback_probe"
    )
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def test_unsupported_fifo_capacity_preserves_existing_public_output(
    tmp_path, monkeypatch
):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchors = []
    for label, variable, fixture in (
        (
            "supported",
            "PYCIRCUIT_TEST_FIFO_ANCHOR",
            "PublishFIFOSystemForIndependentScoreboard",
        ),
        (
            "logical_three",
            "PYCIRCUIT_TEST_DEPTH_THREE_FIFO_ANCHOR",
            "PublishLogicalDepthThreeFIFOWithoutPhysicalization",
        ),
    ):
        anchor = tmp_path / label
        monkeypatch.setenv(variable, str(anchor))
        _ok(_run([str(producer), f"--gtest_filter=StateResetValueTest.{fixture}"]))
        monkeypatch.delenv(variable)
        _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
        anchors.append(anchor)
    final = tmp_path / "protected.ac"
    _ok(
        _cli(
            "link",
            "--from-anchor",
            str(anchors[0]),
            "--target",
            "final",
            "-o",
            str(final),
        )
    )
    before = final.read_bytes()
    rejected = _cli(
        "link",
        "--from-anchor",
        str(anchors[1]),
        "--target",
        "final",
        "-o",
        str(final),
        "--replace",
    )
    assert rejected.returncode != 0
    assert final.read_bytes() == before


def _compile_synthesis(generated, binary):
    """Elaborate the synthesis branch; this is not a functional simulation."""
    receipt = json.loads((generated / "generated.json").read_text())
    sources = [
        generated / row["path"] for row in receipt["files"] if row["role"] == "rtl"
    ]
    standard = _install() / "include/verilog"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-DSYNTHESIS",
                "-s",
                "pyc_root",
                "-o",
                str(binary),
                str(standard / "dff.v"),
                str(standard / "dffe.v"),
                *map(str, sources),
            ]
        )
    )


def test_table_selected_reset_retains_index_dependency_in_managed_metadata(
    tmp_path, monkeypatch
):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = "PYCIRCUIT_TEST_TABLE_RESET_ANCHOR"
    monkeypatch.setenv(variable, str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest."
                "PublishTableSelectedResetForManagedMetadata",
            ]
        )
    )
    monkeypatch.delenv(variable)
    final = tmp_path / "table-reset.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    generated = tmp_path / "managed"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "managed",
            "-o",
            str(generated),
        )
    )
    ports = json.loads((generated / "ports.json").read_text())
    inputs = {row["name"]: row for row in ports["inputs"]}
    assert inputs["enable"]["ordinal"] == 0
    assert "reset" in inputs["enable"]["roles"]
    assert "reset" not in inputs["pyc_rst"]["roles"]
    resets = [
        reset
        for endpoint in ports["domains"]["endpoints"]
        for reset in endpoint["resets"]
    ]
    assert resets
    for reset in resets:
        assert reset["input_ordinal"] is None
        assert reset["input_dependencies"] == [0]
        assert (reset["kind"], reset["polarity"]) == ("async", "low")
