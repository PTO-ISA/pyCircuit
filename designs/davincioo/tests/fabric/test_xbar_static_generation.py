from __future__ import annotations

import ast
import os
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.gpe.ipf.xbar import gpe_ipf_xbar_system
from designs.davincioo.mem.noc.xbar import mem_noc_xbar_system
from designs.davincioo.tmu.bgf.xbar import bgf_xbar_system

ROOT = Path(__file__).resolve().parents[4]


@dataclass(frozen=True)
class FabricSpec:
    name: str
    path: Path
    system: object
    model: str
    packet: str
    destination: str
    delivered: str
    completed: str
    latencies: tuple[int, int, int, int]
    fields: tuple[tuple[str, int | None], ...]


SPECS = (
    FabricSpec(
        "tmu_bgf",
        ROOT / "designs/davincioo/tmu/bgf/xbar.py",
        bgf_xbar_system,
        "BgfXbarSystem",
        "BGFPacket",
        "destination_bank",
        "delivered_bank",
        "completed",
        (1, 2, 4, 7),
        (
            ("sequence", 16),
            ("requester", 8),
            ("thread_id", 16),
            ("block_id", 16),
            ("operation_id", 16),
            ("tile_id", 16),
            ("allocation_generation", 16),
            ("destination_bank", 8),
            ("element_begin", 32),
            ("element_count", 16),
            ("byte_mask", 64),
            ("is_write", None),
            ("ordering_tag", 16),
            ("definedness_tag", 16),
            ("payload", 64),
            ("delivered_bank", 8),
            ("completed", None),
        ),
    ),
    FabricSpec(
        "gpe_ipf",
        ROOT / "designs/davincioo/gpe/ipf/xbar.py",
        gpe_ipf_xbar_system,
        "GpeIpfXbarSystem",
        "GPEPacket",
        "destination_pe",
        "delivered_pe",
        "completed",
        (1, 2, 3, 4),
        (
            ("sequence", 16),
            ("source_pe", 8),
            ("destination_pe", 8),
            ("thread_id", 16),
            ("block_id", 16),
            ("operation_id", 16),
            ("group_id", 16),
            ("participant_mask", 64),
            ("producer_generation", 16),
            ("token_kind", 8),
            ("ordering_tag", 16),
            ("payload", 64),
            ("delivered_pe", 8),
            ("completed", None),
        ),
    ),
    FabricSpec(
        "mem_noc",
        ROOT / "designs/davincioo/mem/noc/xbar.py",
        mem_noc_xbar_system,
        "MemNocXbarSystem",
        "NoCPacket",
        "destination_port",
        "delivered_port",
        "transport_completed",
        (1, 2, 3, 5),
        (
            ("sequence", 16),
            ("transaction_id", 16),
            ("transaction_generation", 16),
            ("request_id", 16),
            ("beat_id", 16),
            ("beat_count", 16),
            ("memory_agent", 8),
            ("thread_id", 16),
            ("block_id", 16),
            ("instruction_id", 16),
            ("ordering_tag", 16),
            ("source_port", 8),
            ("destination_port", 8),
            ("is_response", None),
            ("address", 64),
            ("operation", 8),
            ("byte_mask", 64),
            ("return_target", 8),
            ("data_slot", 16),
            ("mrob_ref", 16),
            ("tile_id", 16),
            ("element_begin", 32),
            ("element_count", 16),
            ("fault_status", 8),
            ("payload", 64),
            ("delivered_port", 8),
            ("transport_completed", None),
        ),
    ),
)


def _packet_fields(spec: FabricSpec) -> tuple[tuple[str, int | None], ...]:
    tree = ast.parse(spec.path.read_text(encoding="utf-8"))
    packet = next(
        node
        for node in tree.body
        if isinstance(node, ast.ClassDef) and node.name == spec.packet
    )
    fields: list[tuple[str, int | None]] = []
    for node in packet.body:
        if not isinstance(node, ast.AnnAssign) or not isinstance(node.target, ast.Name):
            continue
        if isinstance(node.annotation, ast.Name) and node.annotation.id == "bool":
            fields.append((node.target.id, None))
            continue
        assert isinstance(node.annotation, ast.Attribute)
        assert isinstance(node.annotation.value, ast.Name)
        assert node.annotation.value.id == "ac"
        assert node.annotation.attr.startswith("u")
        fields.append((node.target.id, int(node.annotation.attr[1:])))
    return tuple(fields)


def _harness(spec: FabricSpec) -> str:
    initializers: list[str] = []
    preserved_checks: list[str] = []
    updated = {spec.delivered, spec.completed}
    for ordinal, (name, width) in enumerate(spec.fields, start=1):
        if name not in {"sequence", spec.destination, *updated}:
            if width is None:
                initializers.append(
                    f"    packets[index].{name} = ((index + {ordinal}u) & 1u) != 0;"
                )
            else:
                initializers.append(
                    f"    packets[index].{name} = UInt<{width}>{{"
                    f"(index + 1u) * {ordinal + 2}u}};"
                )
        if name in updated:
            continue
        if width is None:
            preserved_checks.append(
                f"          static_cast<bool>(packet.{name}) != "
                f"static_cast<bool>(packets[sequence].{name})"
            )
        else:
            preserved_checks.append(
                f"          packet.{name}.value() != packets[sequence].{name}.value()"
            )
    preserved_condition = " ||\n".join(preserved_checks)
    field_initializers = "\n".join(initializers)
    return f"""
#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

int main(int argc, char **argv) {{
  using namespace ac_generated;
  using gfsim::UInt;
  const bool blocked = argc == 2 && std::string_view(argv[1]) == "blocked";
  const auto push = [](auto &queue, const auto &packet, gfsim::Epoch epoch) {{
    if (!queue.proposePush(packet)) return false;
    queue.doXfer(epoch);
    return true;
  }};
  const auto run_epoch = [](auto &target, unsigned tick, bool block_sink) {{
    const gfsim::Epoch epoch{{tick, 0}};
    auto rows = target.dispatch_rows();
    for (auto &row : rows) {{
      auto *object = static_cast<gfsim::SimObject *>(row.object);
      if (!(block_sink && object->name().starts_with("sink_")))
        row.work(row.object, epoch);
    }}
    for (auto phase : {{gfsim::XferPhase::Arbitrate, gfsim::XferPhase::Probe,
                        gfsim::XferPhase::Commit}})
      for (auto &row : rows)
        row.xfer(row.object, epoch, phase);
  }};
  {spec.model} model;
  std::array<{spec.packet}, 4> packets{{}};
  for (unsigned index = 0; index < packets.size(); ++index) {{
    packets[index].sequence = UInt<16>{{index}};
    packets[index].{spec.destination} = UInt<8>{{index}};
{field_initializers}
  }}
  if (!push(model.ingress_0(), packets[0], {{0, 0}}) ||
      !push(model.ingress_1(), packets[1], {{0, 0}}) ||
      !push(model.ingress_2(), packets[2], {{0, 0}}) ||
      !push(model.ingress_3(), packets[3], {{0, 0}}))
    return 1;
  std::array<unsigned, 4> arrival{{}};
  size_t observed = 0;
  for (unsigned tick = 1; tick != 64 && observed != 4; ++tick) {{
    run_epoch(model, tick, blocked && tick <= 12);
    const auto &values = model.sink_0_values();
    while (observed < values.size()) {{
      const auto &packet = values[observed];
      const unsigned sequence = static_cast<unsigned>(packet.sequence.value());
      if (sequence >= 4 || arrival[sequence] != 0 ||
          packet.{spec.delivered}.value() != sequence ||
          !static_cast<bool>(packet.{spec.completed}) ||
{preserved_condition})
        return 2;
      arrival[sequence] = tick;
      ++observed;
    }}
  }}
  if (observed != 4)
    return 3;
  for (unsigned index = 0; index < arrival.size(); ++index) {{
    if (index) std::cout << ',';
    std::cout << arrival[index];
  }}
  std::cout << '\\n';

  // Two packets in one ingress retain FIFO order while another ingress contends.
  {spec.model} fifo_model;
  auto fifo_first = packets[0];
  auto fifo_second = packets[1];
  auto fifo_contender = packets[2];
  fifo_first.{spec.destination} = UInt<8>{{0}};
  fifo_second.{spec.destination} = UInt<8>{{0}};
  fifo_contender.{spec.destination} = UInt<8>{{0}};
  if (!push(fifo_model.ingress_0(), fifo_first, {{0, 0}}) ||
      !push(fifo_model.ingress_1(), fifo_contender, {{0, 0}}))
    return 4;
  unsigned fifo_tick = 1;
  while (!fifo_model.ingress_0().isEmpty() && fifo_tick != 16)
    run_epoch(fifo_model, fifo_tick++, false);
  if (!fifo_model.ingress_0().isEmpty() ||
      !push(fifo_model.ingress_0(), fifo_second, {{fifo_tick, 0}}))
    return 5;
  for (; fifo_tick != 64 && fifo_model.sink_0_values().size() != 3;
       ++fifo_tick)
    run_epoch(fifo_model, fifo_tick, false);
  if (fifo_model.sink_0_values().size() != 3)
    return 6;
  std::array<unsigned, 3> fifo_position{{99, 99, 99}};
  for (unsigned index = 0; index < fifo_model.sink_0_values().size(); ++index) {{
    const auto sequence = static_cast<unsigned>(
        fifo_model.sink_0_values()[index].sequence.value());
    if (sequence >= fifo_position.size() || fifo_position[sequence] != 99)
      return 7;
    fifo_position[sequence] = index;
  }}
  if (!(fifo_position[0] < fifo_position[1]))
    return 8;

  // Reset one blocked, in-flight instance without disturbing its active peer.
  {spec.model} reset_model;
  {spec.model} peer_model;
  auto stale = packets[3];
  auto peer = packets[2];
  if (!push(reset_model.ingress_0(), stale, {{0, 0}}) ||
      !push(peer_model.ingress_0(), peer, {{0, 0}}))
    return 9;
  for (unsigned tick = 1; tick != 9; ++tick) {{
    run_epoch(reset_model, tick, true);
    run_epoch(peer_model, tick, true);
  }}
  if (!reset_model.ingress_0().isEmpty() ||
      !reset_model.sink_0_values().empty() || !peer_model.ingress_0().isEmpty() ||
      !peer_model.sink_0_values().empty())
    return 10;
  reset_model.reset();
  for (unsigned tick = 9; tick != 40; ++tick) {{
    run_epoch(reset_model, tick, false);
    run_epoch(peer_model, tick, false);
  }}
  if (!reset_model.sink_0_values().empty() ||
      peer_model.sink_0_values().size() != 1 ||
      peer_model.sink_0_values()[0].sequence.value() != 2)
    return 11;
  auto fresh = packets[1];
  if (!push(reset_model.ingress_0(), fresh, {{40, 0}}))
    return 12;
  for (unsigned tick = 40;
       tick != 72 && reset_model.sink_0_values().empty(); ++tick)
    run_epoch(reset_model, tick, false);
  if (reset_model.sink_0_values().size() != 1 ||
      reset_model.sink_0_values()[0].sequence.value() != 1 ||
      peer_model.sink_0_values().size() != 1)
    return 13;
  return 0;
}}
"""


def _verified_raw(spec: FabricSpec) -> str:
    source_text = spec.path.read_text(encoding="utf-8")
    tree = ast.parse(source_text)
    assert _packet_fields(spec) == spec.fields
    apply_calls = [
        node
        for node in ast.walk(tree)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "apply"
    ]
    assert len(apply_calls) == 1
    assert "delivered_0 =" not in source_text
    assert (
        ast.unparse(apply_calls[0].keywords[-1].value)
        == repr(spec.latencies)
        + "["
        + (
            "bank"
            if spec.name == "tmu_bgf"
            else "pe" if spec.name == "gpe_ipf" else "port"
        )
        + "]"
    )

    raw = ac.jit(spec.system, workspace=ROOT).lower_acir()
    assert raw.count("ac.instance") == 0
    operation_lines = tuple(
        line.strip() for line in raw.splitlines() if " = ac." in line
    )
    merge_lines = tuple(line for line in operation_lines if " = ac.merge " in line)
    route_lines = tuple(line for line in operation_lines if " = ac.route " in line)
    transform_lines = tuple(
        line for line in operation_lines if " = ac.transform " in line
    )
    assert len(merge_lines) == 2
    assert all('policy "round_robin" depth 2 latency 1' in line for line in merge_lines)
    assert len(route_lines) == 1
    assert "depths [1, 1, 1, 1] latencies [1, 1, 1, 1]" in route_lines[0]
    assert len(transform_lines) == 4
    transform_shape = re.compile(
        r"%delivered__(\d+) = ac\.transform %egress_(\d+) "
        r"depths \[1\] latencies \[(\d+)\] \{"
    )
    assert tuple(
        tuple(int(value) for value in match.groups())
        for line in transform_lines
        if (match := transform_shape.fullmatch(line)) is not None
    ) == tuple((index, index, latency) for index, latency in enumerate(spec.latencies))
    return raw


@pytest.mark.parametrize("spec", SPECS, ids=lambda spec: spec.name)
def test_static_fabric_generation_preserves_source_and_topology(
    spec: FabricSpec,
) -> None:
    _verified_raw(spec)


@pytest.mark.parametrize("spec", SPECS, ids=lambda spec: spec.name)
def test_static_fabric_gfsim_preserves_behavior(spec: FabricSpec) -> None:
    if os.environ.get("AC_PYTHON_ONLY") == "1":
        pytest.skip("native gfsim generator is unavailable in Python-only CI")

    from agentic_circuit._jit import _lower_acir_to_cpp

    raw = _verified_raw(spec)
    cpp = _lower_acir_to_cpp(raw)
    compiler = os.environ.get("CXX", "c++")
    with tempfile.TemporaryDirectory(prefix=f"davincioo-{spec.name}-") as directory:
        source = Path(directory) / "model.cpp"
        executable = Path(directory) / "model"
        source.write_text(cpp + _harness(spec), encoding="utf-8")
        compiled = subprocess.run(
            (
                compiler,
                "-std=c++20",
                "-O2",
                "-I",
                str(ROOT / "simulator/gfsim/include"),
                str(source),
                "-o",
                str(executable),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert compiled.returncode == 0, compiled.stderr
        unblocked = subprocess.run(
            (str(executable),), text=True, capture_output=True, check=False
        )
        assert unblocked.returncode == 0, unblocked.stderr
        blocked = subprocess.run(
            (str(executable), "blocked"),
            text=True,
            capture_output=True,
            check=False,
        )
        assert blocked.returncode == 0, blocked.stderr
        unblocked_ticks = tuple(
            int(value) for value in unblocked.stdout.strip().split(",")
        )
        blocked_ticks = tuple(int(value) for value in blocked.stdout.strip().split(","))
        assert len(unblocked_ticks) == len(spec.latencies)
        assert all(tick > 12 for tick in blocked_ticks)
        assert tuple(
            unblocked_ticks[index] - unblocked_ticks[0] for index in range(4)
        ) == tuple(
            index + latency - spec.latencies[0]
            for index, latency in enumerate(spec.latencies)
        )


@pytest.mark.parametrize("spec", SPECS, ids=lambda spec: spec.name)
def test_static_fabric_builds_pyc_cpp_and_verilog(spec: FabricSpec) -> None:
    optimizer = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"
    pycgen = optimizer.with_name("acir-queue-pycgen")
    pycc = ROOT / ".pycircuit_out/toolchain/install/bin/pycc"
    metadata = (
        ROOT
        / ".pycircuit_out/toolchain/install/share/pycircuit/toolchain-metadata.json"
    )
    verilator = shutil.which("verilator")
    missing = [
        str(path) for path in (optimizer, pycgen, pycc, metadata) if not path.is_file()
    ]
    if verilator is None:
        missing.append("verilator")
    if missing:
        pytest.skip("PYC C++/Verilog build tools unavailable: " + ", ".join(missing))

    from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

    raw = ac.jit(spec.system, workspace=ROOT).lower_acir()
    compiler = os.environ.get("CXX", "c++")
    with tempfile.TemporaryDirectory(prefix=f"davincioo-{spec.name}-pyc-") as directory:
        raw_path = Path(directory) / "raw.mlir"
        frozen_path = Path(directory) / "frozen.mlir"
        output = Path(directory) / "pyc"
        raw_path.write_text(raw, encoding="utf-8")
        frozen = subprocess.run(
            (
                str(optimizer),
                f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                str(raw_path),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert frozen.returncode == 0, frozen.stderr
        frozen_path.write_text(frozen.stdout, encoding="utf-8")
        built = subprocess.run(
            (
                str(ROOT / "compiler/acir/tools/ac-queue-pyc-build.py"),
                str(frozen_path),
                "--pycgen-tool",
                str(pycgen),
                "--pycc",
                str(pycc),
                "--toolchain-lock",
                str(ROOT / "toolchains/agentic-circuit/pyc.lock.json"),
                "--toolchain-metadata",
                str(metadata),
                "--cxx",
                compiler,
                "--verilator",
                verilator,
                "--pyc-output",
                str(output / "model.pyc"),
                "--cpp-output-dir",
                str(output / "cpp"),
                "--verilog-output-dir",
                str(output / "verilog"),
                "--manifest",
                str(output / "manifest.json"),
            ),
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        assert built.returncode == 0, built.stderr
        assert (output / "model.pyc").is_file()
        assert (output / "manifest.json").is_file()
        assert any((output / "cpp").glob("*.cpp"))
        assert any((output / "verilog").glob("*.v"))
