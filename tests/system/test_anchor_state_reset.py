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
    records, *, zero_payload=False, push_instance=None, pop_event="fifo pop", capacity=4
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
                assert len(queue) <= capacity
                max_depth = max(max_depth, len(queue))
        previous = row
    assert pushes > 20 and pops > 16, "resource failed to make progress or wrap"
    assert (
        max_depth == capacity
    ), "write-only phase did not exercise capacity/backpressure"
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


@pytest.mark.parametrize("bound", [False, True], ids=["same_depth", "bound_depths"])
def test_mixed_actual_fifo_saved_anchor_independent_scoreboards(
    tmp_path, monkeypatch, bound
):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = (
        "PYCIRCUIT_TEST_BOUND_ACTUAL_FIFO_ANCHOR"
        if bound
        else "PYCIRCUIT_TEST_MIXED_ACTUAL_FIFO_ANCHOR"
    )
    fixture = (
        "PublishBoundActualFIFOSystemForIndependentScoreboards"
        if bound
        else "PublishMixedActualFIFOSystemForIndependentScoreboards"
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
            push_instance="root/scalar_fifo/fifo" if bound else "root/scalar_fifo",
            pop_event="scalar fifo pop",
            capacity=8 if bound else 4,
        )
        # Each scoreboard independently requires pushes, reads, wrap, full,
        # backpressure and final drain; an unused shared instance cannot pass.
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


FIFO_PAYLOAD_CPP = r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
using Bit=gfsim::wire<gfsim::Bits<1>>;
using Word=gfsim::wire<gfsim::Bits<9>>;
static Bit bit(unsigned v){return Bit::known(gfsim::Bits<1>{v});}
static void require(bool v){if(!v)throw std::runtime_error("FIFO external X/Z payload oracle");}
int main(){for(unsigned workers:{1u,2u}){
  gfsim::WorkExecutor executor(workers);pyc_root dut("root",&executor);
  // 9'b10xz01zx1: separate known value, known mask and Z mask.
  const auto expected=gfsim::FourState<9>::fromMasks(
    gfsim::Bits<9>{0x109},gfsim::Bits<9>{0x199},gfsim::Bits<9>{0x24});
  auto pins=[&](unsigned clock,unsigned reset){
    dut.PYC_WRITE_CLOCK=bit(clock);dut.PYC_READ_CLOCK=bit(clock);
    dut.PYC_RESET=bit(reset);dut.data=Word::fromPacked(expected);
  };
  pins(0,1);dut.Reset();dut.Xfer();unsigned reads=0;
  for(unsigned cycle=0;cycle<32;++cycle)for(unsigned clock:{0u,1u}){
    pins(clock,cycle==0);dut.Work();
    gfsim::SimFailureInfo failure{};require(dut.pyc_validate_checks(failure));
    require(dut.available.packed().isFullyKnown());
    if(cycle && dut.available.value().value()){
      require(dut.head.packed().parityEquivalent(expected));reads+=clock;
    }else require(dut.head.packed().parityEquivalent(
      gfsim::FourState<9>::known(gfsim::Bits<9>{0})));
    dut.Xfer();
  }
  require(reads>12);
}}
"""


def _fifo_payload_cpp(ports):
    names = {row["name"]: row["wire"] for row in ports["inputs"]}
    return (
        FIFO_PAYLOAD_CPP.replace("PYC_WRITE_CLOCK", names["write_clock"])
        .replace("PYC_READ_CLOCK", names["pyc_clk"])
        .replace("PYC_RESET", names["pyc_rst"])
    )


FIFO_PAYLOAD_RTL = r"""
module fifo_payload_probe;
  logic clock=0,rst=1,permit=0;logic[2:0]phase=0;
  logic[8:0]data=9'b10xz01zx1;wire[8:0]head;
  wire available,complete,error,sampling;integer reads=0;
  pyc_root dut(.write_clock(clock),.pyc_7079635f636c6b(clock),
    .pyc_7079635f727374(rst),.data(data),.head(head),.available(available),
    .pyc_phase(phase),.pyc_root_commit_ok(permit),.pyc_check_complete(complete),
    .pyc_sampling_error(sampling),.pyc_local_error(error));
  task commit;
    permit=1;phase=2;#1;phase=0;permit=0;#1;
  endtask
  task step(input logic level,reset);
    clock=level;rst=reset;#1;phase=1;#1;
    if(error!==0 || sampling!==0 || complete!==1)$fatal(1,"FIFO payload work failed");
    if(!reset && available===1)begin
      if(head!==9'b10xz01zx1)$fatal(1,"FIFO lost external payload X/Z planes");
      if(level)reads=reads+1;
    end else if(available!==0 || head!==9'b0)$fatal(1,"FIFO empty head differs");
    commit();
  endtask
  initial begin
    phase=4;#1;commit();
    for(integer cycle=0;cycle<32;cycle=cycle+1)begin
      step(0,cycle==0);step(1,cycle==0);
    end
    if(reads<=12)$fatal(1,"FIFO X/Z payload did not traverse storage");
    $display("FIFO_PAYLOAD_XZ_OK");$finish;
  end
endmodule
"""


def test_fifo_external_payload_preserves_x_and_z(tmp_path, monkeypatch):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = "PYCIRCUIT_TEST_FIFO_PAYLOAD_ANCHOR"
    monkeypatch.setenv(variable, str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest."
                "PublishFIFOExternalPayloadForIndependentFourState",
            ]
        )
    )
    monkeypatch.delenv(variable)
    original = {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = tmp_path / "payload.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    _build_native_probe(final, tmp_path, _fifo_payload_cpp)
    generated = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(generated)))
    bench = tmp_path / "payload.sv"
    bench.write_text(FIFO_PAYLOAD_RTL)
    assert "FIFO_PAYLOAD_XZ_OK" in _simulate_rtl(
        generated, bench, tmp_path / "payload.vvp", "fifo_payload_probe"
    )
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
    assert original == {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }


def test_unchecked_reset_free_storage_exports_endpoints(tmp_path, monkeypatch):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = "PYCIRCUIT_TEST_UNCHECKED_STORAGE_ANCHOR"
    monkeypatch.setenv(variable, str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest.PublishUncheckedResetFreeStorageForConsumers",
            ]
        )
    )
    monkeypatch.delenv(variable)
    final = tmp_path / "final.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    for target, mode in (
        ("cpp", "managed"),
        ("verilog", "managed"),
        ("verilog", "physical"),
    ):
        generated = tmp_path / f"{target}-{mode}"
        options = ["--rtl-mode", mode] if target == "verilog" else []
        _ok(
            _cli("emit", str(final), "--target", target, *options, "-o", str(generated))
        )
        ports = json.loads((generated / "ports.json").read_text())
        assert ports["schema_version"] == 2
        endpoints = ports["domains"]["endpoints"]
        assert len(endpoints) == 1
        endpoint = endpoints[0]
        assert endpoint["instance"] == "held"
        assert (endpoint["clock_input"], endpoint["edge"], endpoint["resets"]) == (
            1,
            "posedge",
            [],
        )
        if mode == "physical":
            _compile_synthesis(generated, tmp_path / "unchecked-storage.vvp")


def test_schema6_deep_hardware_path_reaches_native_link(tmp_path, monkeypatch):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = "PYCIRCUIT_TEST_DEEP_HIERARCHY_ANCHOR"
    monkeypatch.setenv(variable, str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest.PublishDeepHardwareHierarchyForTransport",
            ]
        )
    )
    monkeypatch.delenv(variable)
    manifest = json.loads((anchor / "anchor.json").read_text())
    assert (manifest["schema"], manifest["routed_contract"]) == (6, 7)
    assert (
        max(
            len(row["instance_path"])
            for row in manifest["configuration"]["domain_bindings"]
        )
        == 33
    )
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))


PORT_DOMAIN_CPP = r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
using Bit=gfsim::wire<gfsim::Bits<1>>;
static Bit bit(unsigned v){
  if(v<2)return Bit::known(gfsim::Bits<1>{v});
  return Bit::fromPacked(v==2 ? gfsim::FourState<1>::unknown()
                             : gfsim::FourState<1>::highImpedance());
}
static void require(bool v){if(!v)throw std::runtime_error("port domain CDC oracle");}
int main(){for(unsigned workers:{1u,2u}){
  // The public adapter handles plain roots and checks primitive Work failures.
  pyc_dut dut(workers);
  unsigned wc=0,rc=0,reset=1,a=0,b=0;
  auto pins=[&]{dut.drive({bit(a),bit(wc),bit(rc),bit(reset),bit(b)});};
  auto step=[&]{pins();require(dut.system().Step()==gfsim::SimStepResult::Running);};
  auto check=[&](unsigned expected_a,unsigned expected_b){
    step();const auto output=dut.sample();
    require(output.aq2_data.packed().parityEquivalent(bit(expected_a).packed()));
    require(output.bq2_data.packed().parityEquivalent(bit(expected_b).packed()));
    require(output.aq2_available.packed().parityEquivalent(bit(1).packed()));
    require(output.bq2_available.packed().parityEquivalent(bit(1).packed()));
  };
  auto pulse=[&](bool write){for(unsigned i=0;i<8;++i){
    (write?wc:rc)=0;step();(write?wc:rc)=1;step();
  }(write?wc:rc)=0;step();};
  pins();dut.system().Build();dut.system().Reset();step();check(0,0);
  reset=0;a=1;b=1;step();pulse(true);check(1,0);
  pulse(false);check(1,1);
  a=2;b=0;step();pulse(true);check(0,1);
  pulse(false);check(0,2);
  a=3;b=1;step();pulse(true);check(1,2);
  pulse(false);check(1,3);
  reset=1;step();check(0,0); // asynchronous reset without either edge
  reset=0;a=1;step();pulse(true);check(1,0);
  pulse(false);check(1,1);
}}
"""


PORT_DOMAIN_PHYSICAL_RTL = r"""
module port_domain_probe;
  logic wclk=0,rclk=0,rst=1,adata=0,bdata=0;
  wire aq2_data,bq2_data,aq2_available,bq2_available;
  pyc_root dut(.wclk(wclk),.rclk(rclk),.rst(rst),.adata(adata),.bdata(bdata),
    .aq2_data(aq2_data),.bq2_data(bq2_data),
    .aq2_available(aq2_available),.bq2_available(bq2_available));
  task check(input logic a,b);
    #1;if(aq2_data!==a || bq2_data!==b || aq2_available!==1 || bq2_available!==1)
      $fatal(1,"independent W/R port domain oracle");
  endtask
  task pulse(input bit write_side);
    repeat(8)begin
      if(write_side)wclk<=0;else rclk<=0;#2;
      if(write_side)wclk<=1;else rclk<=1;#2;
    end
    if(write_side)wclk<=0;else rclk<=0;#2;
  endtask
  initial begin
    check(0,0);rst<=0;adata<=1;bdata<=1;#2;
    pulse(1);check(1,0);pulse(0);check(1,1);
    adata<=1'bx;bdata<=0;#2;
    pulse(1);check(0,1);pulse(0);check(0,1'bx);
    adata<=1'bz;bdata<=1;#2;
    pulse(1);check(1,1'bx);pulse(0);check(1,1'bz);
    rst<=1;check(0,0); // reset between clock edges flushes both synchronizers
    rst<=0;adata<=1;#2;
    pulse(1);check(1,0);pulse(0);check(1,1);
    $display("PORT_DOMAIN_CDC_OK");$finish;
  end
endmodule
"""


def _assert_port_domain_metadata(ports):
    inputs = {row["name"]: row for row in ports["inputs"]}
    outputs = {row["name"]: row for row in ports["outputs"]}
    for rows, name, domain, clock in (
        (inputs, "adata", "W", 1),
        (inputs, "bdata", "R", 2),
        (outputs, "aq2_data", "W", 1),
        (outputs, "bq2_data", "R", 2),
    ):
        assert (rows[name]["domain"], rows[name]["clock_input"]) == (domain, clock)
    controls = {row["wire"]: row for row in ports["controls"]}
    for name, domain, clock in (("aq2_data", "W", 1), ("bq2_data", "R", 2)):
        companion = controls[outputs[name]["availability"]]
        assert companion["role"] == "availability"
        assert (companion["domain"], companion["clock_input"]) == (domain, clock)


@pytest.mark.parametrize("boundary", [False, True], ids=["direct", "entry_boundary"])
def test_port_domain_cdc_public_consumers(tmp_path, monkeypatch, boundary):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = (
        "PYCIRCUIT_TEST_PORT_DOMAIN_BOUNDARY_ANCHOR"
        if boundary
        else "PYCIRCUIT_TEST_PORT_DOMAIN_CDC_ANCHOR"
    )
    fixture = (
        "PublishPortDomainEntryBoundaryForIndependentConsumers"
        if boundary
        else "PublishPortDomainCDCForIndependentConsumers"
    )
    monkeypatch.setenv(variable, str(anchor))
    _ok(_run([str(producer), f"--gtest_filter=StateResetValueTest.{fixture}"]))
    monkeypatch.delenv(variable)
    original = {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }
    manifest = json.loads((anchor / "anchor.json").read_text())
    bindings = manifest["configuration"]["port_domain_bindings"]
    for direction, ordinal, domain, clock in (
        ("input", 0, "W", 1),
        ("input", 4, "R", 2),
        ("output", 0, "W", 1),
        ("output", 1, "R", 2),
    ):
        row = next(
            row
            for row in bindings
            if (row["direction"], row["source_ordinal"]) == (direction, ordinal)
        )
        assert (row["domain"], row["clock_input"]) == (domain, clock)
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = tmp_path / "domains.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    _build_native_probe(final, tmp_path, PORT_DOMAIN_CPP)
    cpp = tmp_path / "cpp"
    _assert_port_domain_metadata(json.loads((cpp / "ports.json").read_text()))
    rtl = tmp_path / "rtl"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(rtl),
        )
    )
    _assert_port_domain_metadata(json.loads((rtl / "ports.json").read_text()))
    bench = tmp_path / "domains.sv"
    bench.write_text(PORT_DOMAIN_PHYSICAL_RTL)
    assert "PORT_DOMAIN_CDC_OK" in _simulate_rtl(
        rtl, bench, tmp_path / "domains.vvp", "port_domain_probe"
    )
    # The declaration cannot cast an R result into W. Failed replacement must
    # leave a complete previously emitted target intact.
    wrong = tmp_path / "wrong.ac"
    correct_map = 'bq2_data = "R"'
    assert correct_map in final.read_text()
    wrong.write_text(final.read_text().replace(correct_map, 'bq2_data = "W"'))
    before = {p.relative_to(cpp): p.read_bytes() for p in cpp.rglob("*") if p.is_file()}
    assert _cli("emit", str(wrong), "--target", "cpp", "-o", str(cpp)).returncode != 0
    assert before == {
        p.relative_to(cpp): p.read_bytes() for p in cpp.rglob("*") if p.is_file()
    }
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
    assert original == {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }


def _independent_reset_fifo_rows():
    """Literal transfer/reset expectations; no emitted graph or DUT values."""
    rows = []
    pins = {"wclk": 0, "rclk": 0, "wrst": 1, "rrst": 0, "data": 0, "push": 0, "pop": 0}

    def step(expected=(-1, -1, -1), failure=False, **changes):
        pins.update(changes)
        rows.append((*pins.values(), *expected, failure))

    def pulse(side=None, blocked=False):
        for _ in range(8):
            expected = (0, 0, 0) if blocked else (-1, -1, -1)
            step(expected, wclk=0, rclk=0)
            step(expected, wclk=int(side != "read"), rclk=int(side != "write"))
        step(expected, wclk=0, rclk=0)

    def enqueue(value):
        step((1, -1, -1), data=value, push=1)
        step(wclk=1)
        step(wclk=0, push=0)

    def consume(value):
        pulse()
        step((1, 1, value))
        step(rclk=1, pop=1)
        step(rclk=0, pop=0)
        pulse()
        step((1, 0, 0))  # one fresh token, consumed once; no stale/duplicate head

    step((0, 0, 0))
    step(wrst=0)
    pulse()
    enqueue(11)
    enqueue(22)
    pulse()
    step((1, 1, 11))
    step((0, 0, 0), rrst=1)  # read-only reset, no clock event
    step((0, 0, 0), rrst=0)
    pulse("write", blocked=True)  # stopped read clock prevents both releases
    pulse()
    step((1, 0, 0))
    enqueue(33)
    consume(33)
    enqueue(44)
    enqueue(55)
    pulse()
    step((1, 1, 44))
    step((0, 0, 0), wrst=1)  # write-only reset, no clock event
    step((0, 0, 0), wrst=0)
    pulse("read", blocked=True)  # stopped write clock also prevents recovery
    pulse()
    step((1, 0, 0))
    enqueue(66)
    consume(66)
    enqueue(77)
    pulse()
    step((1, 1, 77))
    # Both intents are zero. Strict raw-pin knownness is autonomous, including
    # when the asserted peer would mask X in a simple Boolean reset OR.
    step(failure=True, wrst=1, rrst=2)
    step((1, 1, 77), wrst=0, rrst=0)  # discarded flush preserves queued data
    return rows


INDEPENDENT_RESET_FIFO_CPP = r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
#include <string>
using Bit=gfsim::wire<gfsim::Bits<1>>;
using Word=gfsim::wire<gfsim::Bits<9>>;
static Bit bit(unsigned v){return v<2 ? Bit::known(gfsim::Bits<1>{v}) :
  Bit::fromPacked(gfsim::FourState<1>::unknown());}
static unsigned row_index;
static void require(bool ok){if(!ok)throw std::runtime_error(
  "independent FIFO reset oracle at row "+std::to_string(row_index));}
struct Row{unsigned wc,rc,wr,rr,data,push,pop;int wa,ra,head;bool failure;};
static const Row rows[]={PYC_ROWS};
int main(){for(unsigned workers:{1u,2u}){
  gfsim::WorkExecutor executor(workers);pyc_root dut("root",&executor);
  auto pins=[&](const Row &r){dut.wclk=bit(r.wc);dut.rclk=bit(r.rc);
    dut.wrst=bit(r.wr);dut.rrst=bit(r.rr);dut.push=bit(r.push);dut.pop=bit(r.pop);
    dut.data=Word::known(gfsim::Bits<9>{r.data});};
  auto check=[](auto value,int expected){if(expected<0)return;
    require(value.packed().isFullyKnown());require(value.value().value()==unsigned(expected));};
  pins(rows[0]);dut.Reset();dut.Xfer();
  for(row_index=0;row_index<sizeof(rows)/sizeof(rows[0]);++row_index){
    const auto &r=rows[row_index];pins(r);bool accepted=false;
    // Strict primitive controls may fail before the root Check phase.
    try{
      dut.Work();gfsim::SimFailureInfo failure{};
      accepted=dut.pyc_validate_checks(failure);
    }catch(const gfsim::FourStateViolation &){}
    require(accepted!=r.failure);
    if(r.failure){dut.DiscardNext();continue;}
    check(dut.write_available,r.wa);check(dut.read_available,r.ra);check(dut.head,r.head);
    dut.Xfer();
  }
}}
"""


def _independent_reset_fifo_rtl(rows, *, physical):
    controls = (
        ""
        if physical
        else r"""
    ,.pyc_phase(phase),.pyc_root_commit_ok(permit),.pyc_check_complete(complete),
    .pyc_sampling_error(sampling),.pyc_local_error(error)
"""
    )
    prepare = "#2;" if physical else "#1;phase=1;#1;"
    validate = (
        ""
        if physical
        else r"""
    if(error!==failed || complete!==1 || $isunknown(sampling) || (!failed && sampling!==0))
      $fatal(1,"FIFO reset check status at row %0d",row_index);
"""
    )
    finish = (
        ""
        if physical
        else r"""
    permit=!failed;phase=failed?3:2;#1;phase=0;permit=0;#1;
"""
    )
    initialize = (
        "#2;" if physical else "phase=4;#1;permit=1;phase=2;#1;phase=0;permit=0;#1;"
    )
    calls = []
    for wc, rc, wr, rr, data, push, pop, wa, ra, head, failure in rows:
        calls.append(
            f"step(1'b{wc},1'b{rc},1'b{wr},1'b{'x' if rr == 2 else rr},"
            f"9'd{data},1'b{push},1'b{pop},{wa},{ra},{head},1'b{int(failure)});"
        )
    return f"""
module independent_reset_fifo_probe;
  logic wclk=0,rclk=0,wrst=1,rrst=0,push=0,pop=0,permit=0;
  logic[8:0]data=0;logic[2:0]phase=0;
  wire[8:0]head;wire write_available,read_available,complete,sampling,error;
  integer row_index=0;
  pyc_root dut(.wclk(wclk),.rclk(rclk),.wrst(wrst),.rrst(rrst),.data(data),
    .push(push),.pop(pop),.head(head),.write_available(write_available),
    .read_available(read_available){controls});
  task step(input logic wc,rc,wr,rr,input logic[8:0]value,input logic pi,po,
            input integer wa,ra,h,input bit failed);
    data=value;push=pi;pop=po;#1;
    wclk<=wc;rclk<=rc;wrst<=wr;rrst<=rr;{prepare}{validate}
    if(!failed)begin
      if(wa>=0 && write_available!==wa[0])$fatal(1,"FIFO write availability at row %0d",row_index);
      if(ra>=0 && read_available!==ra[0])$fatal(1,"FIFO read availability at row %0d",row_index);
      if(h>=0 && head!==h[8:0])$fatal(1,"FIFO flush/recovery/order oracle at row %0d",row_index);
    end
    {finish}
    row_index=row_index+1;
  endtask
  initial begin
    {initialize}
    {''.join(calls)}
    $display("INDEPENDENT_RESET_FIFO_OK");$finish;
  end
endmodule
"""


def test_independent_reset_fifo_flush_recovery_and_fault_rollback(
    tmp_path, monkeypatch
):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    variable = "PYCIRCUIT_TEST_INDEPENDENT_RESET_FIFO_ANCHOR"
    monkeypatch.setenv(variable, str(anchor))
    _ok(
        _run(
            [
                str(producer),
                "--gtest_filter=StateResetValueTest.PublishIndependentResetFIFOForConsumers",
            ]
        )
    )
    monkeypatch.delenv(variable)
    original = {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = tmp_path / "fifo.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    rows = _independent_reset_fifo_rows()
    cpp_rows = ",".join("{" + ",".join(str(int(v)) for v in row) + "}" for row in rows)
    _build_native_probe(
        final, tmp_path, INDEPENDENT_RESET_FIFO_CPP.replace("PYC_ROWS", cpp_rows)
    )
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
        ports = json.loads((generated / "ports.json").read_text())
        for name in ("wrst", "rrst"):
            assert (
                "reset"
                in next(row for row in ports["inputs"] if row["name"] == name)["roles"]
            )
        bench = tmp_path / f"{mode}.sv"
        bench.write_text(_independent_reset_fifo_rtl(rows, physical=mode == "physical"))
        transcript = _simulate_rtl(
            generated, bench, tmp_path / f"{mode}.vvp", "independent_reset_fifo_probe"
        )
        assert "INDEPENDENT_RESET_FIFO_OK" in transcript
        if mode == "physical":
            assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 1
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
    assert original == {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }


def _derived_clock_rows(initial_high):
    """Independent divider/counter recurrence over committed macrosteps."""
    rows = []
    previous_clock = 0
    count, phase, child = 0, int(initial_high), 0
    echo = 0

    def step(clock, limit, guard=1, reset=0, failure=False):
        nonlocal previous_clock, count, phase, child, echo
        next_count, next_phase, next_child = count, phase, child
        if reset:
            next_count, next_phase, next_child = 0, 0, 0
        elif clock and not previous_clock:
            if count == limit:
                next_count, next_phase = 0, 1 - phase
                if not phase:
                    assert (guard == 2) == failure
                    if guard == 1:
                        next_child = (child + 1) & 511
            else:
                next_count = (count + 1) & 255
        if not failure:
            previous_clock = clock
            count, phase, child = next_count, next_phase, next_child
            echo = limit
        rows.append(
            (clock, reset, limit, guard, phase * 256 + count, child, echo, failure)
        )

    step(0, 0)  # an initially-high derived source is history, not a new edge
    for limit in (0, 2, 1, 0):
        for _ in range(8):
            step(1, limit)
            step(1, limit)  # held root level cannot produce another child event
            step(0, limit)
    # A dormant child's X guard does not fault until a real derived edge.
    step(0, 0, guard=2)
    if phase:
        step(1, count)  # falling derived edge, no child write
        step(0, 0)
    # Publish a different combinational value on an actual root-clock event.
    # The next failure changes limit, and must retain this committed echo.
    committed_limit = (count + 2) & 255
    step(1, committed_limit)
    step(0, committed_limit)
    assert count != committed_limit
    # Parent is ready to toggle 0->1. The late child failure must discard the
    # parent counter/phase and root-clock history along with the child's state.
    step(1, count, guard=2, failure=True)
    step(1, count, guard=1)  # retry exactly the same root edge
    step(1, 0)
    step(0, 0)
    step(0, 0, reset=1)  # asynchronous reset with both clocks stopped
    step(0, 0)
    step(1, 0)
    step(1, 0)
    step(0, 0)
    return rows


DERIVED_CLOCK_CPP = r"""
#include "pycircuit_system.hpp"
#include <stdexcept>
#include <string>
using Bit=gfsim::wire<gfsim::Bits<1>>;
static Bit bit(unsigned v){return v<2 ? Bit::known(gfsim::Bits<1>{v}) :
  Bit::fromPacked(gfsim::FourState<1>::unknown());}
struct Row{unsigned clock,reset,limit,guard,divider,child,echo;bool failed;};
static const Row rows[]={PYC_ROWS};
int main(){for(unsigned workers:{1u,2u}){
  gfsim::WorkExecutor executor(workers);pyc_root dut("root",&executor);
  auto pins=[&](const Row &r){dut.clock=bit(r.clock);dut.reset=bit(r.reset);
    dut.guard=bit(r.guard);dut.limit=gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{r.limit});};
  pins(rows[0]);dut.Reset();dut.Xfer();
  for(unsigned i=0;i<sizeof(rows)/sizeof(rows[0]);++i){
    const auto &r=rows[i];pins(r);bool accepted=false;
    try{dut.Work();gfsim::SimFailureInfo failure{};
      accepted=dut.pyc_validate_checks(failure) && dut.CanXfer();
    }catch(const gfsim::FourStateViolation &){}
    auto require=[&](bool ok){if(!ok)throw std::runtime_error(
      "derived clock oracle at row "+std::to_string(i));};
    require(accepted!=r.failed);
    if(r.failed)dut.DiscardNext();else dut.Xfer();
    require(dut.divider.isFullyKnown() && dut.divider.value().value()==r.divider);
    require(dut.child_q.isFullyKnown() && dut.child_q.value().value()==r.child);
    require(dut.echo.isFullyKnown() && dut.echo.value().value()==r.echo);
  }
}}
"""


def _derived_clock_rtl(rows, *, physical):
    # Physical discards the same late failure, then remains sticky. Managed and
    # native instead discard/retry and commit the exact same root edge.
    selected = rows
    if physical:
        failure = next(i for i, row in enumerate(rows) if row[-1])
        frozen = rows[failure][4:7]
        selected = []
        previous_event, echo = (0, 0), 0
        for i, row in enumerate(rows):
            if i >= failure:
                selected.append((*row[:4], *frozen, row[-1]))
                continue
            event = row[:2]
            if event != previous_event:
                echo = row[2]
            previous_event = event
            selected.append((*row[:6], echo, row[-1]))
    controls = (
        ""
        if physical
        else (
            ",.pyc_phase(phase),.pyc_root_commit_ok(permit),"
            ".pyc_check_complete(complete),.pyc_sampling_error(sampling),.pyc_local_error(error)"
        )
    )
    initialize = (
        "#2;"
        if physical
        else ("#1;phase=4;#2;permit=1;phase=2;#2;phase=0;permit=0;#2;")
    )
    prepare = "#2;" if physical else "#1;phase=1;#2;"
    finish = (
        ""
        if physical
        else r"""
    if(complete!==1 || error!==failed || $isunknown(sampling) || (!failed && sampling!==0))
      $fatal(1,"derived clock check status at row %0d",row_index);
    permit=!failed;phase=failed?3:2;#2;phase=0;permit=0;#2;
"""
    )
    calls = []
    for clock, reset, limit, guard, divider, child, echo, failed in selected:
        calls.append(
            f"step(1'b{clock},1'b{reset},8'd{limit},"
            f"1'b{'x' if guard == 2 else guard},9'd{divider},9'd{child},8'd{echo},1'b{int(failed)});"
        )
    return f"""
module derived_clock_probe;
  logic clock=0,reset=0,guard=1,permit=0;logic[7:0]limit=0;logic[2:0]phase=0;
  wire[8:0]divider,child_q;wire[7:0]echo;wire complete,sampling,error;integer row_index=0;
  pyc_root dut(.clock(clock),.reset(reset),.limit(limit),.guard(guard),
    .divider(divider),.child_q(child_q),.echo(echo){controls});
  task step(input logic c,r,input logic[7:0]threshold,input logic g,
            input logic[8:0]expected_divider,expected_child,
            input logic[7:0]expected_echo,input bit failed);
    limit=threshold;guard=g;#1;clock<=c;reset<=r;{prepare}{finish}
    if(divider!==expected_divider || child_q!==expected_child || echo!==expected_echo)
      $fatal(1,"derived clock counter/rollback oracle at row %0d",row_index);
    row_index=row_index+1;
  endtask
  initial begin
    {initialize}
    {''.join(calls)}
    $display("DERIVED_CLOCK_OK");$finish;
  end
endmodule
"""


@pytest.mark.parametrize("initial_high", [False, True], ids=["low", "high"])
def test_programmable_derived_clock_cascades_and_rolls_back(
    tmp_path, monkeypatch, initial_high
):
    producer = Path(os.environ["PYC_BUILD_DIR"]) / "bin/ACIRAnchorArtifactTests"
    anchor = tmp_path / "anchor"
    fixture = (
        "PublishInitiallyHighDerivedClockForConsumers"
        if initial_high
        else "PublishDerivedClockDividerForConsumers"
    )
    variable = "PYCIRCUIT_TEST_DERIVED_CLOCK_ANCHOR"
    monkeypatch.setenv(variable, str(anchor))
    _ok(_run([str(producer), f"--gtest_filter=StateResetValueTest.{fixture}"]))
    monkeypatch.delenv(variable)
    original = {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }
    configuration = json.loads((anchor / "anchor.json").read_text())["configuration"]
    parent, derived = configuration["clock_sources"]
    assert parent["identity"] == {"kind": "root_input", "input": 1}
    assert derived["parent"] == parent["identity"]
    assert derived["identity"]["kind"] == "registered"
    assert derived["initial_level"] == int(initial_high) and derived["bit_offset"] == 8
    assert {
        len(row["instance_path"])
        for row in configuration["domain_bindings"]
        if row.get("clock_source") == derived["identity"]
    } == {0, 1, 2}
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = tmp_path / "divider.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    rows = _derived_clock_rows(initial_high)
    cpp_rows = ",".join("{" + ",".join(str(int(v)) for v in row) + "}" for row in rows)
    _build_native_probe(
        final, tmp_path, DERIVED_CLOCK_CPP.replace("PYC_ROWS", cpp_rows)
    )
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
        bench = tmp_path / f"{mode}.sv"
        bench.write_text(_derived_clock_rtl(rows, physical=mode == "physical"))
        transcript = _simulate_rtl(
            generated, bench, tmp_path / f"{mode}.vvp", "derived_clock_probe"
        )
        assert "DERIVED_CLOCK_OK" in transcript
        assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == int(mode == "physical")
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
    assert original == {
        p.relative_to(anchor): p.read_bytes() for p in anchor.rglob("*") if p.is_file()
    }
