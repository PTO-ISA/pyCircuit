"""State writer admission follows actual instance paths and selector SSA."""

from __future__ import annotations

import hashlib
import json
import shutil

import pytest
from test_source_system_execution import (
    FIXTURE,
    _assert_closed_observation_identities,
    _bundle,
    _cli,
    _compile,
    _emit_branch_probe_rtl,
    _install,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


def _program(tmp_path, conflict=None):
    source = tmp_path / "hierarchical state sources"
    source.mkdir()
    for filename in ("hierarchical_state_provider.py", "hierarchical_state_bench.py"):
        shutil.copy2(FIXTURE / filename, source)
    if conflict == "same-selector":
        provider_source = source / "hierarchical_state_provider.py"
        text = provider_source.read_text()
        assert text.count("if not selector:") == 2
        provider_source.write_text(text.replace("if not selector:", "if selector:", 1))
    elif conflict == "unrelated-selector":
        bench_source = source / "hierarchical_state_bench.py"
        text = bench_source.read_text()
        left_branch = "def update_left() -> None:\n        if selector:"
        assert text.count(left_branch) == 1
        bench_source.write_text(
            text.replace(
                "selector: ac.u1 = 0", "selector: ac.u1 = 0\n    other: ac.u1 = 0", 1
            )
            .replace(left_branch, "def update_left() -> None:\n        if other:")
            .replace(
                "selector = ~selector",
                "selector = ~selector\n        other = ~other",
                1,
            )
        )
    else:
        assert conflict is None
    provider = tmp_path / "hierarchical state provider"
    consumer = tmp_path / "hierarchical state consumer"
    _ok(_compile(source, "hierarchical_state_provider.py", provider))
    # Consumer compilation must use the published interface, not provider source/body.
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "unavailable hierarchical provider body.ac"
    (source / "hierarchical_state_provider.py").unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "hierarchical_state_bench.py", consumer, [provider]))
    finally:
        hidden.rename(body)
    return provider, consumer


def _link(provider, consumer, final, top="ExerciseHierarchicalState"):
    return _cli(
        "link",
        str(provider),
        str(consumer),
        "--top",
        f"checks.hierarchical_state_bench.{top}",
        "-o",
        str(final),
    )


def test_parent_and_child_disjoint_state_writers_preserve_old_q_and_identity(tmp_path):
    provider, consumer = _program(tmp_path)
    final = tmp_path / "hierarchical-state-system.ac"
    _ok(_link(provider, consumer, final))
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    for target in ("cpp", "verilog"):
        target_root = tmp_path / target
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "6", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=6"]
            )
            records = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            events = [
                row
                for row in records
                if row["kind"] == "log" and row["spec"]["event"] == "hierarchical state"
            ]
            # Independent outer Rules inspect old Q. Each child takes the local
            # +1 or its parent's distinct +9/+17, never both, on each rising edge.
            left = right = 7
            expected = []
            for cycle in range(6):
                selector = cycle % 2
                expected.extend([(left, right, selector)] * 2)
                left = (left + (9 if selector else 1)) & 255
                right = (right + (1 if selector else 17)) & 255
            assert [
                tuple(int(value["value"]) for value in row["values"]) for row in events
            ] == expected
            assert [int(row["evaluation_epoch"]) for row in events] == list(range(12))
            assert [int(row["commit_epoch"]) for row in events] == list(range(1, 13))
            assert {row["instance"] for row in events} == {"root"}
            payload_events = [
                row
                for row in records
                if row["kind"] == "log"
                and row["spec"]["event"] == "hierarchical payload"
            ]
            data = spare = 7
            tag = spare_tag = 0
            expected_payload = []
            for cycle in range(6):
                selector = cycle % 2
                expected_payload.extend([(data, tag, spare, spare_tag)] * 2)
                data = (data + (11 if selector else 2)) & 255
                tag = 5 if tag == 0 else 0
                if selector:
                    spare = (spare + 2) & 255
                    spare_tag = 5 if spare_tag == 0 else 0
            assert [
                tuple(int(value["value"]) for value in row["values"])
                for row in payload_events
            ] == expected_payload
            assert [int(row["evaluation_epoch"]) for row in payload_events] == list(
                range(12)
            )
            assert [int(row["commit_epoch"]) for row in payload_events] == list(
                range(1, 13)
            )
            assert {row["instance"] for row in payload_events} == {"root"}
            terminal = [row for row in records if row["kind"] == "result"]
            assert len(terminal) == 1
            assert terminal[0]["status"] == "TERMINATED"
            assert int(terminal[0]["epoch_time"]) == 12
            _assert_closed_observation_identities(records)
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def test_sequential_child_methods_forward_one_candidate_and_preserve_old_q(tmp_path):
    provider, consumer = _program(tmp_path)
    final = tmp_path / "ordered-hierarchical-state-system.ac"
    _ok(_link(provider, consumer, final, "ExerciseSequentialHierarchicalState"))
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    for target in ("cpp", "verilog"):
        target_root = tmp_path / target
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "6", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=6"]
            )
            records = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            expected = {
                "hierarchical ordered first": [],
                "hierarchical ordered second": [],
                "hierarchical ordered old q": [],
            }
            # Same-Rule Method calls follow the declared additions in sequence.
            # The independent autonomous reader observes committed old Q.
            committed = 7
            for _ in range(6):
                first = (committed + 3) & 255
                second = (first + 5) & 255
                expected["hierarchical ordered first"].extend([(committed, first)] * 2)
                expected["hierarchical ordered second"].extend([(first, second)] * 2)
                expected["hierarchical ordered old q"].extend([(committed,)] * 2)
                committed = second
            for event, values in expected.items():
                events = [
                    row
                    for row in records
                    if row["kind"] == "log" and row["spec"]["event"] == event
                ]
                assert [
                    tuple(int(value["value"]) for value in row["values"])
                    for row in events
                ] == values
                assert [int(row["evaluation_epoch"]) for row in events] == list(
                    range(12)
                )
                assert [int(row["commit_epoch"]) for row in events] == list(
                    range(1, 13)
                )
                assert {row["instance"] for row in events} == {"root"}
            terminal = [row for row in records if row["kind"] == "result"]
            assert len(terminal) == 1
            assert terminal[0]["status"] == "TERMINATED"
            assert int(terminal[0]["epoch_time"]) == 12
            _assert_closed_observation_identities(records)
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest

    probe_root = tmp_path / "ordered-icarus"
    probe_root.mkdir()
    _ordered_hierarchy_four_state_rtl(final, probe_root)


def test_hierarchical_state_four_state_failure_and_discard_preserve_storage(tmp_path):
    provider, consumer = _program(tmp_path)
    final = tmp_path / "hierarchical-state-probe.ac"
    _ok(_link(provider, consumer, final, "HierarchicalStateProbe"))
    generated = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(generated)))
    # Observe and fault-stimulate real emitted storage, following the existing
    # conditional-method probe. No calculated oracle is written into DUT state.
    for header in generated.rglob("*.hpp"):
        header.write_text(header.read_text().replace("private:", "public:"))
    (generated / "hierarchical_probe.cpp").write_text(
        r"""
#include "pycircuit_system.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>
using Bit = gfsim::wire<gfsim::Bits<1>>;
static Bit bit(unsigned value) {
  if (value < 2) return Bit::known(gfsim::Bits<1>{value});
  return Bit::fromPacked(value == 2 ? gfsim::FourState<1>::unknown()
                                  : gfsim::FourState<1>::highImpedance());
}
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
template<class Storage, class Output, class Wire>
void inject(Storage &storage, Output &output, Wire value) {
  using Current = std::remove_cvref_t<decltype(storage->current(0))>;
  auto &state = const_cast<Current &>(storage->current(0));
  state.q = value;
  output.element(0) = value;
}
int main() {
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    auto drive = [&](bool clock) { dut.drive({bit(clock), bit(0)}); };
    drive(false); dut.system().Build();
    auto root = dut.root_->pyc_implementation;
    auto left = root->pyc_instance_left;
    auto right = root->pyc_instance_right;
    auto snapshot = [&]() {
      std::vector<std::uint64_t> result;
      auto add = [&](const auto &storage) {
        const auto &q = storage->current(0);
        result.insert(result.end(), {q.q.packed().value().value(),
          q.q.packed().knownMask().value(), q.q.packed().zMask().value(), q.clock});
      };
      add(left->pyc_instance_state_state);
      add(right->pyc_instance_state_state);
      add(root->pyc_instance_selector_state);
      return result;
    };
    auto start = [&]() {
      drive(false); dut.system().Reset();
      require(dut.system().cycle() == 0 && dut.observations().Events().empty(), "reset publications");
      require(snapshot() == std::vector<std::uint64_t>{7,255,0,0,7,255,0,0,0,1,0,0}, "reset state and history");
    };
    auto selector = [&](unsigned value) {
      inject(root->pyc_instance_selector_state, root->pyc_instance_selector_q, bit(value));
    };
    for (unsigned unknown : {2u, 3u}) {
      start(); selector(unknown);
      // Preserve a real X/Z child payload as well as an unknown enable. The
      // test checks every stored plane and clock, not just the numeric value.
      auto payload = gfsim::wire<gfsim::Bits<8>>::fromPacked(
          gfsim::FourState<8>::fromMasks(gfsim::Bits<8>{0xa0},
                                       gfsim::Bits<8>{0xf0},
                                       gfsim::Bits<8>{0x0f}));
      inject(left->pyc_instance_state_state, left->pyc_instance_state_q, payload);
      const auto before = snapshot();
      drive(true);
      require(dut.system().Step() == gfsim::SimStepResult::Failed, "unknown reached selector must fail");
      // DFFE rejects unknown E during Work, before any state can transfer.
      require(dut.system().failureInfo().phase == gfsim::SimFailurePhase::Evaluate,
              "unknown storage enable evaluation phase");
      require(dut.system().failureInfo().code == "runtime_failure", "runtime failure code");
      require(snapshot() == before && dut.system().cycle() == 0, "failure changed V/K/Z or clock history");
      require(dut.observations().Events().empty(), "failure published observations");
      selector(0); const auto failed = snapshot();
      require(dut.system().Step() == gfsim::SimStepResult::Failed && snapshot() == failed, "sticky failure");
      start(); selector(unknown); const auto inactive = snapshot();
      require(dut.system().Step() == gfsim::SimStepResult::Running, "no edge must hold unknown selector");
      require(snapshot() == inactive, "no edge changed four-state storage");
    }
    start(); selector(1); drive(true);
    const auto before = snapshot();
    dut.root_->Work(); dut.root_->DiscardNext(); dut.observations().DiscardNext();
    require(snapshot() == before && dut.system().cycle() == 0, "Discard changed data or clock history");
    require(dut.observations().Events().empty(), "Discard published observations");
    require(dut.system().Step() == gfsim::SimStepResult::Running, "same-edge retry failed");
    require(snapshot() == std::vector<std::uint64_t>{16,255,0,1,8,255,0,1,0,1,0,1}, "retry did not commit one parent/local update");
    require(dut.system().cycle() == 1 && !dut.observations().Events().empty(), "retry publication");
  }
  std::cout << "HIERARCHICAL_FOUR_STATE_DISCARD_OK\n";
}
"""
    )
    with (generated / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(hierarchical_probe hierarchical_probe.cpp)\n"
            "target_compile_features(hierarchical_probe PRIVATE cxx_std_20)\n"
            "target_link_libraries(hierarchical_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "probe-build"
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
    _ok(
        _run(
            [
                "cmake",
                "--build",
                str(build),
                "--target",
                "hierarchical_probe",
                "--parallel",
                "2",
            ]
        )
    )
    assert _ok(_run([str(build / "hierarchical_probe")], timeout=30)).strip() == (
        "HIERARCHICAL_FOUR_STATE_DISCARD_OK"
    )
    _hierarchical_four_state_rtl(final, tmp_path)


def _hierarchical_four_state_rtl(final, tmp_path):
    _, rtl = _emit_branch_probe_rtl(final, tmp_path)
    testbench = tmp_path / "hierarchical_four_state.sv"
    testbench.write_text(
        r"""
module hierarchical_four_state;
  logic clk=0, rst=0, permit=0;
  logic [2:0] phase=0;
  wire complete, sampling_error, error;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .pyc_phase(phase), .pyc_root_commit_ok(permit),
    .pyc_check_complete(complete), .pyc_sampling_error(sampling_error),
    .pyc_local_error(error));
  `define R dut.dut
  `define LEFT `R.pyc_instance_left.pyc_instance_state
  `define RIGHT `R.pyc_instance_right.pyc_instance_state
  `define SELECTOR `R.pyc_instance_selector
  function automatic logic four(input integer n);
    case(n) 0: four=0; 1: four=1; 2: four=1'bx; 3: four=1'bz; endcase
  endfunction
  task automatic check_current(input logic [7:0] left, right,
                               input logic selector, history);
    if (`LEFT.q_current !== left || `RIGHT.q_current !== right ||
        `SELECTOR.q_current !== selector ||
        `LEFT.managed.clock_current !== history ||
        `RIGHT.managed.clock_current !== history ||
        `SELECTOR.managed.clock_current !== history)
      $fatal(1,"hierarchy changed four-state Q or clock history");
  endtask
  task automatic prepare; #1; phase=1; #1; endtask
  task automatic expect_error(input logic expected);
    if (error !== expected || sampling_error !== 0 || complete !== 1)
      $fatal(1,"hierarchy check mismatch: expected %b, got %b/%b/%b",
             expected,error,sampling_error,complete);
  endtask
  task automatic commit;
    permit=1; phase=2; #1; phase=0; permit=0; #1;
  endtask
  task automatic discard;
    permit=0; phase=2; #1; phase=3; #1; phase=0; #1;
  endtask
  task automatic reset_model;
    clk=0; rst=0; permit=1; phase=4; #1; phase=2; #1;
    phase=0; permit=0; #1; check_current(7,7,0,0);
  endtask
  initial begin
    #1;
    for (integer s=2; s<4; s=s+1) begin
      // A fault in one incoming/local enable denies the entire hierarchy,
      // preserving X and Z payload bits and every owner's clock history.
      reset_model(); `LEFT.q_current=8'b10xz01z0;
      `SELECTOR.q_current=four(s); clk=1; prepare(); expect_error(1);
      discard(); check_current(8'b10xz01z0,7,four(s),0);
      // The same unknown selector is harmless without a new rising edge.
      reset_model(); `SELECTOR.q_current=four(s);
      prepare(); expect_error(0); commit(); check_current(7,7,four(s),0);
      // Synchronous reset masks unknown grants and restores all initializers.
      reset_model(); `LEFT.q_current=8'b10xz01z0;
      `SELECTOR.q_current=four(s); clk=1; rst=1;
      prepare(); expect_error(0); commit(); check_current(7,7,0,1);
      // Establish real history, then hold/fall/rise with an unknown selector.
      reset_model(); `SELECTOR.q_current=1; clk=1;
      prepare(); expect_error(0); commit(); check_current(16,8,0,1);
      `SELECTOR.q_current=four(s);
      prepare(); expect_error(0); commit(); check_current(16,8,four(s),1);
      clk=0; prepare(); expect_error(0); commit(); check_current(16,8,four(s),0);
      clk=1; prepare(); expect_error(1); discard(); check_current(16,8,four(s),0);
    end
    // Successful work can be discarded, including all pending clock history,
    // then retried at the same physical edge and committed exactly once.
    reset_model(); `SELECTOR.q_current=1; clk=1;
    prepare(); expect_error(0); discard(); check_current(7,7,1,0);
    prepare(); expect_error(0); commit(); check_current(16,8,0,1);
    $display("HIERARCHICAL_ICARUS_FOUR_STATE_DISCARD_OK"); $finish;
  end
endmodule
"""
    )
    binary = tmp_path / "hierarchical_four_state"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "hierarchical_four_state",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dff.v"),
                str(_install() / "include/verilog/dffe.v"),
                *[str(path) for path in rtl if path.name != "simulation_top.sv"],
                str(testbench),
            ]
        )
    )
    stdout = _ok(_run(["vvp", str(binary)], timeout=30))
    assert "HIERARCHICAL_ICARUS_FOUR_STATE_DISCARD_OK" in stdout
    assert _records(stdout) == []


def _ordered_hierarchy_four_state_rtl(final, tmp_path):
    _, rtl = _emit_branch_probe_rtl(final, tmp_path)
    testbench = tmp_path / "ordered_hierarchical_four_state.sv"
    testbench.write_text(
        r"""
module ordered_hierarchical_four_state;
  logic clk=0, rst=0, permit=0;
  logic [2:0] phase=0;
  wire complete, sampling_error, error;
  pyc_root dut(.pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    .pyc_phase(phase), .pyc_root_commit_ok(permit),
    .pyc_check_complete(complete), .pyc_sampling_error(sampling_error),
    .pyc_local_error(error));
  `define STATE dut.dut.pyc_instance_child.pyc_instance_state
  task automatic check_current(input logic [7:0] value, input logic history);
    if (`STATE.q_current !== value || `STATE.managed.clock_current !== history)
      $fatal(1,"ordered hierarchy changed Q/history");
  endtask
  task automatic prepare;
    #1; phase=1; #1;
    if (error !== 0 || sampling_error !== 0 || complete !== 1)
      $fatal(1,"ordered hierarchy rejected known enables/four-state payload");
  endtask
  task automatic commit;
    permit=1; phase=2; #1; phase=0; permit=0; #1;
  endtask
  task automatic discard;
    permit=0; phase=2; #1; phase=3; #1; phase=0; #1;
  endtask
  task automatic reset_model;
    clk=0; rst=0; permit=1; phase=4; #1; phase=2; #1;
    phase=0; permit=0; #1; check_current(7,0);
  endtask
  initial begin
    #1; reset_model(); clk=1;
    prepare(); check_current(7,0); discard(); check_current(7,0);
    prepare(); commit(); check_current(15,1);
    prepare(); commit(); check_current(15,1);
    clk=0; prepare(); commit(); check_current(15,0);
    clk=1; prepare(); commit(); check_current(23,1);
    // Both parent registrations share this child's candidate in source order.
    // Unknown payload is legal with known enables, and Discard preserves Z.
    reset_model(); `STATE.q_current=8'bz01x1010; clk=1;
    prepare(); discard(); check_current(8'bz01x1010,0);
    prepare(); commit(); check_current(8'bxxxxxxxx,1);
    reset_model(); check_current(7,0);
    $display("ORDERED_HIERARCHICAL_ICARUS_FOUR_STATE_DISCARD_OK"); $finish;
  end
endmodule
"""
    )
    binary = tmp_path / "ordered_hierarchical_four_state"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "ordered_hierarchical_four_state",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dff.v"),
                str(_install() / "include/verilog/dffe.v"),
                *[str(path) for path in rtl if path.name != "simulation_top.sv"],
                str(testbench),
            ]
        )
    )
    stdout = _ok(_run(["vvp", str(binary)], timeout=30))
    assert "ORDERED_HIERARCHICAL_ICARUS_FOUR_STATE_DISCARD_OK" in stdout
    assert _records(stdout) == []


def test_hierarchical_whole_table_writers_keep_each_lane_and_instance(tmp_path):
    source = tmp_path / "hierarchical table sources"
    source.mkdir()
    for filename in ("hierarchical_table_provider.py", "hierarchical_table_bench.py"):
        shutil.copy2(FIXTURE / filename, source)
    provider, consumer = tmp_path / "table provider", tmp_path / "table consumer"
    _ok(_compile(source, "hierarchical_table_provider.py", provider))
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "unavailable table provider body.ac"
    (source / "hierarchical_table_provider.py").unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "hierarchical_table_bench.py", consumer, [provider]))
    finally:
        hidden.rename(body)
    final = tmp_path / "hierarchical-table-system.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(consumer),
            "--top",
            "checks.hierarchical_table_bench.ExerciseHierarchicalTable",
            "-o",
            str(final),
        )
    )
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    for target in ("cpp", "verilog"):
        target_root = tmp_path / target
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "6", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=6"]
            )
            records = _records(_ok(_run([str(binary), *arguments], timeout=30)))
            events = [
                row
                for row in records
                if row["kind"] == "log" and row["spec"]["event"] == "hierarchical table"
            ]
            first, second, spare_first, spare_second = 7, 7, 7, 7
            expected = []
            for cycle in range(6):
                expected.extend([(first, second, spare_first, spare_second)] * 2)
                parent = cycle % 2
                first = (first + (9 if parent else 2)) & 255
                second = (second + (17 if parent else 4)) & 255
                if parent:
                    spare_first = (spare_first + 2) & 255
                    spare_second = (spare_second + 4) & 255
            assert [
                tuple(int(value["value"]) for value in row["values"]) for row in events
            ] == expected
            assert [int(row["evaluation_epoch"]) for row in events] == list(range(12))
            assert [int(row["commit_epoch"]) for row in events] == list(range(1, 13))
            assert {row["instance"] for row in events} == {"root"}
            terminal = [row for row in records if row["kind"] == "result"]
            assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"
            assert int(terminal[0]["epoch_time"]) == 12
            _assert_closed_observation_identities(records)
            transcripts.append([row for row in records if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


@pytest.mark.parametrize("conflict", ["same-selector", "unrelated-selector"])
def test_hierarchical_state_conflicts_reject_without_replacing_output(
    tmp_path, conflict
):
    provider, consumer = _program(tmp_path, conflict)
    final = tmp_path / "protected-final.ac"
    sentinel = b"preexisting output must survive rejected writer composition\n"
    final.write_bytes(sentinel)
    result = _link(provider, consumer, final)
    assert result.returncode != 0
    assert "multiple outer writers" in result.stdout + result.stderr
    assert final.read_bytes() == sentinel


@pytest.mark.parametrize("kind", ["rule", "method"])
def test_retired_after_does_not_replace_hierarchical_unit(tmp_path, kind):
    source = tmp_path / "retired-order"
    source.mkdir()
    path = source / "owner.py"
    path.write_text(
        "import pycircuit as ac\n@ac.module\ndef Owner() -> ac.u8:\n    return 7\n"
    )
    unit = tmp_path / "unit"
    _ok(_compile(source, path.name, unit))
    before = {
        item.relative_to(unit): item.read_bytes()
        for item in unit.rglob("*")
        if item.is_file()
    }
    path.write_text(
        "import pycircuit as ac\n@ac.module\ndef Owner():\n"
        "    count: ac.u8 = 7\n"
        f"    @ac.{kind}\n    def first() -> None:\n        count = count + 3\n"
        f"    @ac.{kind}(after=(first,))\n    def second() -> None:\n        count = count + 5\n"
    )
    rejected = _cli(
        "compile",
        "-c",
        str(path),
        "--source-root",
        str(source),
        "--package-prefix",
        "checks",
        "-o",
        str(unit),
        "--replace",
    )
    assert rejected.returncode != 0, rejected.stdout + rejected.stderr
    assert "after" in rejected.stderr.lower() and "retired" in rejected.stderr.lower()
    assert {
        item.relative_to(unit): item.read_bytes()
        for item in unit.rglob("*")
        if item.is_file()
    } == before
