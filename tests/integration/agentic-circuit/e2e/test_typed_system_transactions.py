from __future__ import annotations

import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[4]
FIXTURE_ROOT = (
    ROOT / "tests/integration/agentic-circuit/e2e/fixtures/typed_transactions"
)
FIXTURE = FIXTURE_ROOT / "atomic_shift.py"
NATIVE_ROOT = ROOT / ".pycircuit_out/layout-root"


class TypedSystemTransactionTest(unittest.TestCase):
    def test_owner_write_batch_executes_atomically_under_backpressure(self) -> None:
        compiler = shutil.which("c++")
        toolchain = Path(os.environ.get("PYC_TOOLCHAIN_ROOT", NATIVE_ROOT))
        tools = {
            "opt": Path(os.environ.get("ACIR_OPT", toolchain / "bin/acir-opt")),
            "plan": Path(
                os.environ.get("ACIR_QUEUE_PLAN", toolchain / "bin/acir-queue-plan")
            ),
            "cxxgen": Path(
                os.environ.get("ACIR_QUEUE_CXXGEN", toolchain / "bin/acir-queue-cxxgen")
            ),
        }
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")
        if any(not path.is_file() for path in tools.values()):
            self.skipTest("typed transaction toolchain is unavailable")

        import agentic_circuit as ac
        from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

        spec = importlib.util.spec_from_file_location("ac_typed_atomic_shift", FIXTURE)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load typed transaction fixture")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        specialization = ac.jit(module.atomic_shift, workspace=FIXTURE_ROOT)
        raw_acir = specialization.lower_acir()
        self.assertEqual(4, raw_acir.count("ac.var.assign_element @entries"))
        self.assertEqual(3, raw_acir.count("ac.var.read_element @entries"))

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            raw = output / "atomic_shift.raw.mlir"
            frozen = output / "atomic_shift.frozen.mlir"
            generated = output / "atomic_shift.cpp"
            raw.write_text(raw_acir, encoding="utf-8")
            optimized = subprocess.run(
                (
                    str(tools["opt"]),
                    f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                    str(raw),
                    "-o",
                    str(frozen),
                ),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, optimized.returncode, optimized.stderr)
            frozen_text = frozen.read_text(encoding="utf-8")
            self.assertEqual(4, frozen_text.count("ac.table.propose @entries"))

            planned = subprocess.run(
                (str(tools["plan"]), str(frozen)),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, planned.returncode, planned.stderr)
            plan = json.loads(planned.stdout)
            firing = next(
                block for block in plan["blocks"] if block["kind"] == "firing"
            )
            self.assertEqual(4, len(firing["state_writes"]))
            self.assertEqual(
                ["entries"] * 4,
                [write["table"] for write in firing["state_writes"]],
            )
            self.assertEqual(["command"], firing["inputs"])
            self.assertEqual(["snapshot"], firing["outputs"])

            with mock.patch.dict(
                os.environ,
                {
                    "ACIR_OPT": str(tools["opt"]),
                    "ACIR_QUEUE_CXXGEN": str(tools["cxxgen"]),
                },
            ):
                generated_text = specialization.lower_cpp()
            generated.write_text(generated_text, encoding="utf-8")
            self.assertIn("gfsim::OwnerWriteBatch<gfsim::UInt<8>>", generated_text)
            self.assertEqual(4, generated_text.count("owner_writes0.emplace_back"))

            harness = output / "harness.cpp"
            executable = output / "atomic_shift"
            harness.write_text(
                f'''#include "{generated.name}"

#include <array>

using Model = ac_generated::AtomicShift;
using Command = ac_generated::Command;

std::array<unsigned long long, 4> state(const Model &model) {{
  const auto &entries = model.table_entries();
  return {{static_cast<unsigned long long>(entries.at(0)),
           static_cast<unsigned long long>(entries.at(1)),
           static_cast<unsigned long long>(entries.at(2)),
           static_cast<unsigned long long>(entries.at(3))}};
}}

int main() {{
  Model model;
  auto rows = model.dispatch_rows();
  unsigned tick = 0;
  auto cycle = [&](bool run_sink) {{
    const gfsim::Epoch epoch{{++tick, 0}};
    for (auto &row : rows)
      if (run_sink || row.kind != gfsim::ObjectKind::Sink)
        row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }};
  auto offer = [&](unsigned value) {{
    if (!model.command().proposePush(
            Command{{gfsim::UInt<1>{{1}}, gfsim::UInt<8>{{value}}}}))
      return false;
    model.command().doXfer(gfsim::Epoch{{tick, 0}});
    return true;
  }};
  auto transact_and_drain = [&](unsigned value) {{
    if (!offer(value))
      return false;
    cycle(false);
    cycle(true);
    return true;
  }};

  if (!transact_and_drain(10) || !transact_and_drain(20) ||
      !transact_and_drain(30))
    return 1;
  if (state(model) != std::array<unsigned long long, 4>{{30, 20, 10, 0}})
    return 2;

  // Leave value 40 in the depth-one output Queue, then offer value 50.
  if (!offer(40))
    return 3;
  cycle(false);
  const std::array<unsigned long long, 4> blocked{{40, 30, 20, 10}};
  if (state(model) != blocked || !offer(50))
    return 4;
  cycle(false);
  cycle(false);
  if (state(model) != blocked || model.command().committedValues().size() != 1)
    return 5;

  const std::array<unsigned long long, 4> committed{{50, 40, 30, 20}};
  unsigned commit_boundaries = 0;
  auto previous = state(model);
  for (unsigned attempt = 0; attempt != 3; ++attempt) {{
    cycle(true);
    const auto current = state(model);
    if (current != previous) {{
      if (current != committed)
        return 6;
      ++commit_boundaries;
    }}
    previous = current;
  }}
  if (commit_boundaries != 1 || state(model) != committed ||
      !model.command().isEmpty())
    return 7;
  cycle(true);
  cycle(true);
  if (state(model) != committed || model.sink_0_values().size() != 5)
    return 8;
  const unsigned expected[] = {{10, 20, 30, 40, 50}};
  for (size_t index = 0; index != 5; ++index)
    if (static_cast<unsigned long long>(model.sink_0_values()[index].value) !=
        expected[index])
      return 9;

  // Two objects use the same generated specialization but own separate state.
  Model peer;
  auto peer_rows = peer.dispatch_rows();
  if (!peer.command().proposePush(
          Command{{gfsim::UInt<1>{{1}}, gfsim::UInt<8>{{77}}}}))
    return 10;
  peer.command().doXfer(gfsim::Epoch{{0, 0}});
  for (auto &row : peer_rows)
    row.work(row.object, gfsim::Epoch{{1, 0}});
  for (auto &row : peer_rows)
    row.xfer(row.object, gfsim::Epoch{{1, 0}}, gfsim::XferPhase::Arbitrate);
  for (auto &row : peer_rows)
    row.xfer(row.object, gfsim::Epoch{{1, 0}}, gfsim::XferPhase::Probe);
  for (auto &row : peer_rows)
    row.xfer(row.object, gfsim::Epoch{{1, 0}}, gfsim::XferPhase::Commit);
  if (state(peer) != std::array<unsigned long long, 4>{{77, 0, 0, 0}} ||
      state(model) != committed)
    return 11;
  return 0;
}}
''',
                encoding="utf-8",
            )
            linked = subprocess.run(
                (
                    compiler,
                    "-std=c++20",
                    "-I",
                    str(ROOT / "simulator/gfsim/include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                cwd=output,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, linked.returncode, linked.stderr)
            executed = subprocess.run(
                (str(executable),),
                cwd=output,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, executed.returncode, executed.stderr)


if __name__ == "__main__":
    unittest.main()
