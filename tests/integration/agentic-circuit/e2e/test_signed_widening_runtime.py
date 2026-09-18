"""Runtime evidence for explicit signed widening multiplication."""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from agentic_circuit._jit import _lower_queue_acir
from agentic_circuit._queue_frontend import lower_queue_source

ROOT = Path(__file__).resolve().parents[4]
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/toolchain/build/bin"))

SOURCE = """
import agentic_circuit as ac

@ac.struct
class SignedPair:
    a: ac.s16
    b: ac.s16

@ac.module
def widen(pair: SignedPair) -> ac.s32:
    return ac.sext(pair.a, ac.s32) * ac.sext(pair.b, ac.s32)

@ac.system
def signed_widening(pair: SignedPair) -> ac.s32:
    return widen(pair)
"""

# Signed 16-bit operands and the signed 32-bit product they must produce.
CASES = (
    (-32768, -32768, 1073741824),
    (-32768, 32767, -1073709056),
    (-1, 1, -1),
)

HARNESS = """
#include "__GFSIM__"
#include <cstdint>
#include <iostream>

int main() {
  ac_generated::SignedWidening model;
  auto rows = model.dispatch_rows();
  std::uint64_t epoch = 0;
  const std::uint16_t cases[][2] = {
      __CASES__
  };
  for (const auto &pair : cases) {
    ac_generated::SignedPair item{};
    item.a = gfsim::UInt<16>{pair[0]};
    item.b = gfsim::UInt<16>{pair[1]};
    if (!model.pair().proposePush(item)) return 1;
    model.pair().doXfer({epoch++, 0});
    for (unsigned step = 0; step < 8; ++step, ++epoch) {
      const gfsim::Epoch current{epoch, 0};
      for (auto &row : rows) row.work(row.object, current);
      for (auto &row : rows)
        row.xfer(row.object, current, gfsim::XferPhase::Arbitrate);
      for (auto &row : rows)
        row.xfer(row.object, current, gfsim::XferPhase::Commit);
    }
  }
  if (model.sink_0_values().size() != __COUNT__) return 2;
  for (const auto &value : model.sink_0_values())
    std::cout << value.value() << "\\n";
  return 0;
}
"""


class SignedWideningRuntimeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.acir_opt = ACIR_BIN / "acir-opt"
        cls.cxxgen = ACIR_BIN / "acir-queue-cxxgen"
        cls.compiler = shutil.which("c++")
        if (
            cls.compiler is None
            or not cls.acir_opt.exists()
            or not cls.cxxgen.exists()
        ):
            raise unittest.SkipTest("acir-opt and acir-queue-cxxgen are required")

    def _run(self, command: tuple[object, ...], *, cwd: Path) -> str:
        completed = subprocess.run(
            tuple(str(item) for item in command),
            cwd=cwd,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, completed.returncode, completed.stderr)
        return completed.stdout

    def test_boundary_products_match_explicit_sign_extension(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            raw = lower_queue_source(SOURCE, "signed_widening")
            frozen = work / "model.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt), encoding="utf-8"
            )
            frozen_text = frozen.read_text(encoding="utf-8")
            # The product is a real 32-bit multiply fed by explicit sign
            # extension, not a 16-bit product widened afterwards.
            self.assertRegex(frozen_text, r"ac\.var\.mul %\w+, %\w+ : !ac\.var<i32>")
            self.assertRegex(frozen_text, r"ac\.var\.extract %\w+ from 15 width 1")

            gfsim_source = work / "gfsim.cpp"
            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )

            case_rows = ", ".join(
                f"{{0x{left & 0xFFFF:04X}u, 0x{right & 0xFFFF:04X}u}}"
                for left, right, _ in CASES
            )
            harness = work / "harness.cpp"
            harness.write_text(
                HARNESS.replace("__GFSIM__", gfsim_source.name)
                .replace("__CASES__", case_rows)
                .replace("__COUNT__", str(len(CASES))),
                encoding="utf-8",
            )
            binary = work / "signed_widening"
            self._run(
                (
                    self.compiler,
                    "-std=c++20",
                    "-I",
                    ROOT / "simulator/gfsim/include",
                    harness,
                    "-o",
                    binary,
                ),
                cwd=work,
            )

            observed = [int(line) for line in self._run((binary,), cwd=work).split()]
            expected = [product & 0xFFFFFFFF for _, _, product in CASES]
            self.assertEqual(expected, observed)


if __name__ == "__main__":
    unittest.main()
