"""A selected storage owner defaults compiler-only proposals for both targets."""

from __future__ import annotations

import json
import shutil

import pytest
from test_source_system_execution import FIXTURE, _cli, _compile, _install, _ok, _run

pytestmark = pytest.mark.system


@pytest.mark.parametrize("target", ["cpp", "verilog"])
def test_selected_typed_state_owner_keeps_unreachable_checked_callers(tmp_path, target):
    source = tmp_path / "selected sources"
    source.mkdir()
    for name in ("finite_state_provider.py", "finite_state_bench.py"):
        shutil.copy2(FIXTURE / name, source)
    provider, caller = tmp_path / "provider", tmp_path / "caller"
    _ok(_compile(source, "finite_state_provider.py", provider))
    _ok(_compile(source, "finite_state_bench.py", caller, [provider]))
    final = tmp_path / "selected-owner.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(caller),
            "--top",
            "checks.finite_state_provider.FiniteStateOwner",
            "-o",
            str(final),
        )
    )
    # The explicit link closure retains the caller and its checks. Selecting
    # the child must not require deleting unrelated final IR to emit it.
    text = final.read_text()
    assert '"ac.expect"' in text
    assert "ac.internal_ports" in text
    assert "!ac.table" in text
    generated, build = tmp_path / target, tmp_path / "build"
    _ok(_cli("emit", str(final), "--target", target, "-o", str(generated)))
    manifest = json.loads((generated / "generated.json").read_text())
    assert not any(row["path"].startswith("simulation_") for row in manifest["files"])
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
                "pycircuit_modules",
                "--parallel",
                "2",
            ]
        )
    )


def test_root_output_availability_matches_cpp_and_rtl_ports(tmp_path):
    source = tmp_path / "root validity sources"
    source.mkdir()
    shutil.copy2(FIXTURE / "output_validity_provider.py", source)
    unit = tmp_path / "root validity unit"
    _ok(_compile(source, "output_validity_provider.py", unit))
    final = tmp_path / "root validity.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.output_validity_provider.GuardedOutputs",
            "-o",
            str(final),
        )
    )
    maps = []
    for target in ("cpp", "verilog"):
        generated = tmp_path / ("root-" + target)
        _ok(_cli("emit", str(final), "--target", target, "-o", str(generated)))
        mapping = json.loads((generated / "ports.json").read_text())
        maps.append(mapping)
        outputs = mapping["outputs"]
        assert [row["ordinal"] for row in outputs] == [0, 1]
        assert len({row["availability"] for row in outputs}) == 2
        assert all(row["availability"] for row in outputs)
        manifest = json.loads((generated / "generated.json").read_text())
        assert {"path": "ports.json", "role": "runtime-glue"} in manifest["files"]
        build = tmp_path / ("root-build-" + target)
        if target == "cpp":
            wire = outputs[1]["wire"]
            checks = "\n".join(
                f"if (sample.{row['availability']}.value().value() != enabled) return 2;"
                for row in outputs
            )
            (generated / "availability_probe.cpp").write_text(
                r"""
#include "pycircuit_system.hpp"
#include <iostream>
using Bit = gfsim::wire<gfsim::Bits<1>>;
using Byte = gfsim::wire<gfsim::Bits<8>>;
int main() {
  auto bit = [](unsigned x) { return Bit::known(gfsim::Bits<1>{x}); };
  auto byte = [](unsigned x) { return Byte::known(gfsim::Bits<8>{x}); };
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    dut.drive({byte(0), bit(1), bit(0), bit(0)});
    dut.system().Build();
    dut.system().Reset();
    for (unsigned cycle = 0; cycle != 3; ++cycle) {
      unsigned word = 10 * (cycle + 1), enabled = cycle != 1;
      for (unsigned clock : {0u, 1u}) {
        dut.drive({byte(word), bit(enabled), bit(clock), bit(0)});
        if (dut.system().Step() != gfsim::SimStepResult::Running) return 1;
        auto sample = dut.sample();
        CHECKS
        // An invalid output is never read as a payload.
        if (enabled && sample.WIRE.value().value() != word + 3) return 3;
      }
    }
    // Known-valid X data remains data, rather than an availability fault.
    {
      unsigned enabled = 1;
      auto unknown = Byte::fromPacked(gfsim::FourState<8>::unknown());
      dut.drive({unknown, bit(1), bit(0), bit(0)});
      dut.system().Reset();
      for (unsigned clock : {0u, 1u}) {
        dut.drive({unknown, bit(1), bit(clock), bit(0)});
        if (dut.system().Step() != gfsim::SimStepResult::Running) return 4;
        auto sample = dut.sample();
        CHECKS
        if (sample.WIRE.packed().knownMask().value() == 255) return 5;
      }
    }
    for (auto control : {gfsim::FourState<1>::unknown(),
                         gfsim::FourState<1>::highImpedance()}) {
      dut.drive({byte(7), bit(1), bit(0), bit(0)});
      dut.system().Reset();
      dut.drive({byte(7), Bit::fromPacked(control), bit(0), bit(0)});
      if (dut.system().Step() != gfsim::SimStepResult::Running) return 6;
      auto epoch = dut.system().cycle();
      dut.drive({byte(7), Bit::fromPacked(control), bit(1), bit(0)});
      if (dut.system().Step() != gfsim::SimStepResult::Failed ||
          dut.system().cycle() != epoch) return 7;
      bool rejected = false;
      try { (void)dut.sample(); } catch (const std::logic_error &) { rejected = true; }
      if (!rejected) return 8;
    }
  }
  std::cout << "ROOT_AVAILABILITY_OK\n";
}
""".replace(
                    "CHECKS", checks
                ).replace(
                    "WIRE", wire
                )
            )
            with (generated / "CMakeLists.txt").open("a") as cmake:
                cmake.write(
                    "\nadd_executable(availability_probe availability_probe.cpp)\n"
                    "target_link_libraries(availability_probe PRIVATE pycircuit_modules)\n"
                )
        else:
            rtl = (generated / "design_top.sv").read_text()
            for row in outputs:
                assert row["wire"] in rtl and row["availability"] in rtl
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
                    "availability_probe" if target == "cpp" else "pycircuit_modules",
                    "--parallel",
                    "2",
                ]
            )
        )
        if target == "cpp":
            candidates = list(build.rglob("availability_probe"))
            assert len(candidates) == 1
            assert "ROOT_AVAILABILITY_OK" in _ok(_run([str(candidates[0])]))
        else:
            _root_availability_four_state_rtl(generated, outputs, tmp_path)
    assert maps[0]["target"] == "cpp" and maps[1]["target"] == "verilog"
    assert all(row["role"] == "availability" for row in maps[0]["controls"])
    assert {
        key: value
        for key, value in maps[0].items()
        if key not in {"target", "controls"}
    } == {
        key: value
        for key, value in maps[1].items()
        if key not in {"target", "controls"}
    }


def _root_availability_four_state_rtl(generated, outputs, tmp_path):
    bench = tmp_path / "root_availability_four_state.sv"
    connections = ",\n".join(
        f".{row['wire']}(payload{index}), .{row['availability']}(valid{index})"
        for index, row in enumerate(outputs)
    )
    bench.write_text(
        r"""
module root_availability_four_state;
  logic [7:0] word=0;
  logic enabled=1, clk=0, rst=0, permit=0;
  logic [2:0] phase=0;
  wire [7:0] payload0, payload1;
  wire valid0, valid1, complete, sampling_error, error;
  pyc_root dut(.word(word), .enabled(enabled),
    .pyc_7079635f636c6b(clk), .pyc_7079635f727374(rst),
    CONNECTIONS,
    .pyc_phase(phase), .pyc_root_commit_ok(permit),
    .pyc_check_complete(complete), .pyc_sampling_error(sampling_error),
    .pyc_local_error(error));
  task automatic reset_model;
    clk=0; rst=0; enabled=1; word=0; permit=1; phase=4; #1;
    phase=2; #1; phase=0; permit=0; #1;
  endtask
  task automatic prepare(input logic fault);
    #1; phase=1; #1;
    if (complete!==1 || sampling_error!==0 || error!==fault)
      $fatal(1,"wrong availability/control fault");
  endtask
  task automatic commit;
    permit=1; phase=2; #1; phase=0; permit=0; #1;
  endtask
  task automatic discard;
    permit=0; phase=2; #1; phase=3; #1; phase=0; #1;
  endtask
  initial begin
    reset_model();
    for (integer cycle=0; cycle<3; cycle=cycle+1) begin
      word=10*(cycle+1); enabled=(cycle!=1);
      for (integer edge_index=0; edge_index<2; edge_index=edge_index+1) begin
        clk=edge_index; prepare(0);
        if (valid0!==enabled || valid1!==enabled)
          $fatal(1,"wrong companion for selected root output");
        if (enabled && payload1!==(word+8'd3))
          $fatal(1,"wrong valid payload");
        commit();
      end
    end
    reset_model(); word='x; enabled=1;
    for (integer edge_index=0; edge_index<2; edge_index=edge_index+1) begin
      clk=edge_index; prepare(0);
      if (valid0!==1 || valid1!==1 || (^payload1)!==1'bx)
        $fatal(1,"known-valid unknown payload lost its meaning");
      commit();
    end
    for (integer unknown_kind=0; unknown_kind<2; unknown_kind=unknown_kind+1) begin
      reset_model(); word=7; enabled=unknown_kind ? 1'bz : 1'bx;
      clk=0; prepare(0); commit();
      clk=1; prepare(1); discard();
      // Retrying the failed edge must still fault: Discard cannot advance history.
      prepare(1); discard();
    end
    reset_model(); enabled=1'bx; clk=1; rst=1;
    prepare(0); commit();
    $display("ROOT_AVAILABILITY_FOUR_STATE_OK"); $finish;
  end
endmodule
""".replace(
            "CONNECTIONS", connections
        )
    )
    manifest = json.loads((generated / "generated.json").read_text())
    rtl = [generated / "design_top.sv"] + [
        generated / row["path"]
        for row in manifest["files"]
        if row["role"] == "rtl" and row["path"] != "design_top.sv"
    ]
    binary = tmp_path / "root_availability_four_state"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "root_availability_four_state",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dff.v"),
                str(_install() / "include/verilog/dffe.v"),
                *map(str, rtl),
                str(bench),
            ]
        )
    )
    assert "ROOT_AVAILABILITY_FOUR_STATE_OK" in _ok(_run(["vvp", str(binary)]))
