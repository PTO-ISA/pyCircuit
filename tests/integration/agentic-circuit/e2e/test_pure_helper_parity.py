from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
from unittest.mock import patch

import agentic_circuit as ac
from agentic_circuit._queue_frontend import lower_queue_source

ROOT = Path(__file__).resolve().parents[4]
FIXTURE = (
    ROOT / "tests/integration/agentic-circuit/e2e/fixtures/pure_helpers/architecture.py"
)
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/toolchain/build/bin"))
PYC_TOOLCHAIN = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
)


class PureHelperParityTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.acir_opt = ACIR_BIN / "acir-opt"
        cls.cxxgen = ACIR_BIN / "acir-queue-cxxgen"
        cls.pycgen = ACIR_BIN / "acir-queue-pycgen"
        cls.pycc = Path(
            os.environ.get("PYCC", ROOT / ".pycircuit_out/toolchain/build/bin/pycc")
        )
        cls.compiler = shutil.which("c++")
        cls.runtime = PYC_TOOLCHAIN / "lib/libpyc6_runtime.a"
        cls.include = PYC_TOOLCHAIN / "include"
        required = (
            cls.acir_opt,
            cls.cxxgen,
            cls.pycgen,
            cls.pycc,
            cls.runtime,
            cls.include,
        )
        if cls.compiler is None or not all(path.exists() for path in required):
            raise unittest.SkipTest("integrated ACIR/PYC C++ toolchain is required")

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

    def test_jit_routes_helper_programs_through_native_queuegraph_codegen(
        self,
    ) -> None:
        spec = spec_from_file_location("pure_helper_fixture", FIXTURE)
        self.assertIsNotNone(spec)
        assert spec is not None and spec.loader is not None
        module = module_from_spec(spec)
        sys.modules[spec.name] = module
        try:
            spec.loader.exec_module(module)
            with patch.dict(
                os.environ,
                {
                    "ACIR_OPT": str(self.acir_opt),
                    "ACIR_QUEUE_CXXGEN": str(self.cxxgen),
                },
            ):
                generated = ac.jit(module.pure_helper_pipeline).lower_cpp()
        finally:
            sys.modules.pop(spec.name, None)
        self.assertIn("helper_add_one", generated)
        self.assertNotIn("choose_increment", generated)

    def test_ordinary_and_forced_inline_helpers_match_gfsim_and_pyc(self) -> None:
        inputs = (0, 1, 127, 254, 255)
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            raw = work / "model.raw.mlir"
            frozen = work / "model.frozen.mlir"
            gfsim_source = work / "gfsim_model.cpp"
            pyc_source = work / "model.pyc"
            pyc_output = work / "pyc"

            raw.write_text(
                lower_queue_source(
                    FIXTURE.read_text(encoding="utf-8"),
                    "pure_helper_pipeline",
                ),
                encoding="utf-8",
            )
            raw_text = raw.read_text(encoding="utf-8")
            self.assertIn("func.func private @add_one", raw_text)
            self.assertIn("func.func private @choose_increment", raw_text)
            self.assertIn("attributes {ac.inline = true}", raw_text)

            self._run(
                (
                    self.acir_opt,
                    "--pass-pipeline=builtin.module(ac-inline-pure-helpers,"
                    "canonicalize,cse,ac-freeze-topology)",
                    raw,
                    "-o",
                    frozen,
                ),
                cwd=ROOT,
            )
            frozen_text = frozen.read_text(encoding="utf-8")
            self.assertIn("func.func private @add_one", frozen_text)
            self.assertIn("func.call @add_one", frozen_text)
            self.assertNotIn("@choose_increment", frozen_text)

            gfsim_source.write_text(
                self._run((self.cxxgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            pyc_source.write_text(
                self._run((self.pycgen, frozen), cwd=ROOT), encoding="utf-8"
            )
            generated_gfsim = gfsim_source.read_text(encoding="utf-8")
            self.assertIn("helper_add_one", generated_gfsim)
            self.assertNotIn("choose_increment", generated_gfsim)
            self.assertNotIn("func.call", pyc_source.read_text(encoding="utf-8"))

            self._run(
                (
                    self.pycc,
                    pyc_source,
                    "--emit=cpp",
                    "--out-dir",
                    pyc_output,
                    "--hierarchy-policy=strict",
                    "--inline-policy=off",
                ),
                cwd=ROOT,
            )

            gfsim_harness = work / "gfsim_harness.cpp"
            gfsim_binary = work / "gfsim_model"
            gfsim_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "{gfsim_source.name}"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>

                    int main() {{
                      ac_generated::PureHelperPipeline model;
                      const std::array<std::uint8_t, {len(inputs)}> inputs{{
                          {", ".join(str(value) for value in inputs)}}};
                      auto rows = model.dispatch_rows();
                      std::uint64_t epoch = 0;
                      for (std::uint8_t input : inputs) {{
                        if (!model.incoming().proposePush(
                                ac_generated::Item{{input, 0, 0}})) return 1;
                        for (unsigned step = 0; step < 4; ++step, ++epoch) {{
                          const gfsim::Epoch current{{epoch, 0}};
                          for (auto &row : rows) row.work(row.object, current);
                          for (auto &row : rows)
                            row.xfer(row.object, current, gfsim::XferPhase::Arbitrate);
                          for (auto &row : rows)
                            row.xfer(row.object, current, gfsim::XferPhase::Commit);
                        }}
                      }}
                      for (const auto &value : model.sink_0_values())
                        std::cout << ((value.value.value() << 16) |
                                      (value.ordinary.value() << 8) |
                                      value.forced.value()) << "\\n";
                    }}
                    """
                ),
                encoding="utf-8",
            )
            self._run(
                (
                    self.compiler,
                    "-std=c++20",
                    "-I",
                    ROOT / "simulator/gfsim/include",
                    gfsim_harness,
                    "-o",
                    gfsim_binary,
                ),
                cwd=work,
            )

            pyc_harness = work / "pyc_harness.cpp"
            pyc_binary = work / "pyc_model"
            pyc_harness.write_text(
                textwrap.dedent(
                    f"""
                    #include "pure_helper_pipeline.hpp"
                    #include <array>
                    #include <cstdint>
                    #include <iostream>
                    #include <cpp/pyc_tb.hpp>

                    int main() {{
                      pyc::gen::pure_helper_pipeline dut;
                      pyc::cpp::Testbench<pyc::gen::pure_helper_pipeline> tb(dut);
                      tb.addClock(dut.clk, 1, 0, false);
                      dut.in_valid = pyc::cpp::Wire<1>(0);
                      dut.out_ready = pyc::cpp::Wire<1>(1);
                      tb.reset(dut.rst, 2, 1);
                      const std::array<std::uint8_t, {len(inputs)}> inputs{{
                          {", ".join(str(value) for value in inputs)}}};
                      std::uint64_t cycle = 0;
                      for (std::uint8_t input : inputs) {{
                        bool accepted = false;
                        bool observed = false;
                        for (unsigned step = 0; step < 12 && !observed; ++step) {{
                          dut.in_valid = pyc::cpp::Wire<1>(accepted ? 0 : 1);
                          dut.in_data = pyc::cpp::Wire<24>(
                              static_cast<std::uint32_t>(input) << 16);
                          tb.runCycleAutoTrace(cycle++, nullptr);
                          if (dut.in_valid.value() && dut.in_ready.value())
                            accepted = true;
                          if (dut.out_valid.value()) {{
                            std::cout << dut.out_data.value() << "\\n";
                            observed = true;
                          }}
                        }}
                        if (!accepted || !observed) return 2;
                      }}
                    }}
                    """
                ),
                encoding="utf-8",
            )
            self._run(
                (
                    self.compiler,
                    "-std=c++20",
                    "-I",
                    pyc_output,
                    "-I",
                    self.include,
                    pyc_output / "pure_helper_pipeline.cpp",
                    pyc_harness,
                    self.runtime,
                    "-o",
                    pyc_binary,
                ),
                cwd=work,
            )

            gfsim = self._run((gfsim_binary,), cwd=work)
            pyc = self._run((pyc_binary,), cwd=work)

        expected = "".join(
            f"{(value << 16) | (((value + 1) & 0xFF) << 8) | ((value + 1) & 0xFF)}\n"
            for value in inputs
        )
        self.assertEqual(expected, gfsim)
        self.assertEqual(gfsim, pyc)


if __name__ == "__main__":
    unittest.main()
