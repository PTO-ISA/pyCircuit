"""CHK executed in gfsim, not merely compiled.

designs/davincioo/tmu/trn/chk.md states that gfsim execution is the first
implementation gate. This test is that gate for CHK: it lowers the system to
typed gfsim C++, compiles a harness against it, and runs the card's behavioral
acceptances as real transactions.

It skips rather than fails when the native ACIR tools or a C++20 compiler are
unavailable, because their absence is an environment fact and not a defect in
the design. Build the tools with:

    cmake --preset dev-llvm22 -S compiler/acir \\
      -DMLIR_DIR=<llvm22>/lib/cmake/mlir -DLLVM_DIR=<llvm22>/lib/cmake/llvm \\
      -DCMAKE_PREFIX_PATH=<llvm22> -DACIR_BUILD_TESTING=OFF \\
      -DCMAKE_CXX_COMPILER=<c++20 compiler>
    cmake --build .pycircuit_out/acir/dev-llvm22 --target acir-opt acir-queue-cxxgen

The generated model is the evidence, so the harness reads state through the
model's own accessors instead of re-deriving it.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.contracts.tmu_trn import (
    CHECKPOINT_SLOTS,
    CHK_CAPTURE,
    CHK_RELEASE,
    CHK_UNKNOWN,
    CHK_UNWIND,
)
from designs.davincioo.tmu.trn.chk import trn_chk_system

ROOT = Path(__file__).resolve().parents[4]
TOOLS = ROOT / ".pycircuit_out/acir/dev-llvm22/bin"

HARNESS = f"""
#include "chk_model.h"

#include <cstdio>

using ac_generated::CheckpointRequest;

namespace {{
CheckpointRequest req(unsigned op, unsigned flow, unsigned id, unsigned frontier) {{
  CheckpointRequest value{{}};
  value.operation = gfsim::UInt<8>{{op}};
  value.flow_key = gfsim::UInt<32>{{flow}};
  value.checkpoint_id = gfsim::UInt<16>{{id}};
  value.current_frontier = gfsim::UInt<6>{{frontier}};
  return value;
}}
int failures = 0;
void check(bool ok, const char *what) {{
  std::printf("%s %s\\n", ok ? "ok  " : "FAIL", what);
  if (!ok) ++failures;
}}
}}  // namespace

int main() {{
  ac_generated::TrnChkSystem model;
  auto rows = model.dispatch_rows();
  std::uint64_t tick = 0;
  auto cycle = [&]() {{
    const gfsim::Epoch epoch{{tick++, 0}};
    for (auto &row : rows) row.work(row.object, epoch);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }};
  auto settle = [&](const CheckpointRequest &value) {{
    if (!model.request().proposePush(value)) {{ check(false, "queue accepted the offer"); return; }}
    model.request().doXfer({{tick++, 0}});
    for (int step = 0; step < 6; ++step) cycle();
  }};
  auto live_records = [&]() {{
    const auto &table = model.table_checkpoints();
    unsigned count = 0;
    for (std::size_t index = 0; index < table.size(); ++index)
      if (static_cast<bool>(table.at(index).live)) ++count;
    return count;
  }};

  settle(req({CHK_CAPTURE}u, 7, 100, 5));
  check(model.sink_0_values().size() == 1, "a capture is acknowledged on the capture port");
  check(static_cast<bool>(model.sink_0_values().back().accepted), "the capture is accepted");
  check(live_records() == 1, "one checkpoint record is live");

  settle(req({CHK_CAPTURE}u, 7, 100, 9));
  check(static_cast<bool>(model.sink_0_values().back().matched), "a repeated capture reports matched");
  check(!static_cast<bool>(model.sink_0_values().back().accepted),
        "a repeated capture changes nothing, so it is not accepted");
  check(live_records() == 1, "a repeated capture consumes no second record");

  settle(req({CHK_UNWIND}u, 7, 100, 9));
  check(model.sink_1_values().size() == 1, "an unwind is acknowledged on the unwind port");
  check(static_cast<unsigned>(model.sink_1_values().back().target_frontier) == 5,
        "recovery stops at the captured frontier");
  check(static_cast<unsigned>(model.sink_1_values().back().steps) == 4,
        "the answer is the distance in RAT rollbacks, 9 -> 5");
  check(live_records() == 1, "an unwind writes no checkpoint state");

  settle(req({CHK_CAPTURE}u, 7, 200, 12));
  settle(req({CHK_UNWIND}u, 7, 200, 3));
  check(static_cast<bool>(model.sink_1_values().back().stale),
        "a checkpoint deeper than the map is stale");
  check(!static_cast<bool>(model.sink_1_values().back().accepted), "a stale unwind is refused");
  check(static_cast<unsigned>(model.sink_1_values().back().steps) == 0,
        "a refused unwind recovers nothing");

  settle(req({CHK_RELEASE}u, 7, 100, 9));
  check(static_cast<bool>(model.sink_2_values().back().accepted), "a release is accepted");
  check(live_records() == 1, "the released record is no longer live");

  settle(req({CHK_UNKNOWN}u, 7, 100, 9));
  check(model.sink_3_values().size() == 1, "an unknown operation is answered on the refused port");
  check(!static_cast<bool>(model.sink_3_values().back().accepted), "and is refused");

  for (unsigned id = 300; id < 315; ++id) settle(req({CHK_CAPTURE}u, 7, id, 4));
  check(live_records() == {CHECKPOINT_SLOTS}u, "the pool holds CHECKPOINT_SLOTS records");
  settle(req({CHK_CAPTURE}u, 7, 999, 4));
  check(static_cast<bool>(model.sink_0_values().back().exhausted), "a full pool reports exhausted");
  check(!static_cast<bool>(model.sink_0_values().back().accepted), "and accepts nothing");

  return failures == 0 ? 0 : 1;
}}
"""


def _compiler() -> str | None:
    """Return a C++20 compiler, or None.

    `-std=c++20` is spelled that way only from GCC 10 and Clang 10 on, and the
    generated model uses defaulted comparisons, so an older default compiler
    cannot build it at all.
    """

    for candidate in (os.environ.get("CXX"), "c++", "g++", "clang++"):
        if not candidate:
            continue
        resolved = shutil.which(candidate)
        if resolved is None:
            continue
        probe = subprocess.run(
            (resolved, "-std=c++20", "-E", "-x", "c++", "-"),
            input="int main(){}\n",
            text=True,
            capture_output=True,
            check=False,
        )
        if probe.returncode == 0:
            return resolved
    return None


def test_chk_runs_its_acceptances_in_gfsim() -> None:
    for tool in ("acir-opt", "acir-queue-cxxgen"):
        if not (TOOLS / tool).is_file():
            pytest.skip(f"native {tool} is unavailable; see this module's docstring")
    compiler = _compiler()
    if compiler is None:
        pytest.skip("no C++20 compiler is available")

    from agentic_circuit._jit import _lower_acir_to_cpp

    model = _lower_acir_to_cpp(ac.jit(trn_chk_system, workspace=str(ROOT)).lower_acir())
    assert "gfsim::QueueTableTransition" in model, (
        "the pool must lower to a stateful transition"
    )

    with tempfile.TemporaryDirectory(prefix="davincioo-chk-") as directory:
        root = Path(directory)
        (root / "chk_model.h").write_text(model, encoding="utf-8")
        harness = root / "harness.cpp"
        harness.write_text(HARNESS, encoding="utf-8")
        executable = root / "chk"
        compiled = subprocess.run(
            (
                compiler,
                "-std=c++20",
                "-O1",
                "-I",
                str(ROOT / "simulator/gfsim/include"),
                "-I",
                str(root),
                str(harness),
                "-o",
                str(executable),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert compiled.returncode == 0, compiled.stderr
        run = subprocess.run(
            (str(executable),), text=True, capture_output=True, check=False
        )
        assert run.returncode == 0, run.stdout + run.stderr
        # Every line is one acceptance, so an empty run would pass vacuously.
        assert run.stdout.count("ok  ") >= 20, run.stdout
        assert "FAIL" not in run.stdout, run.stdout
