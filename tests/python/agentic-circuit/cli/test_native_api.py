from __future__ import annotations

import unittest
from dataclasses import FrozenInstanceError

import agentic_circuit
from agentic_circuit._native_api import NativeRequest, capabilities, run_native_compiler


VALID_ACIR = b"""
module attributes {ac.contract_epoch = "0.5"} {
  ac.system @main root @top as "root" tick 0 "cycle"
      workload @top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @top() parameters {} graph {
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}
"""


class NativeApiTest(unittest.TestCase):
    def test_private_extension_compiles_verified_acir(self) -> None:
        result = run_native_compiler(
            NativeRequest(
                acir=VALID_ACIR,
                stop_after="acsim-verify",
                emits=("frozen-acir", "acsim"),
            )
        )

        self.assertEqual((), result.diagnostics)
        self.assertEqual(
            ("frozen.ac.mlir", "model.acsim.mlir"),
            tuple(item.path for item in result.artifacts),
        )
        self.assertTrue(
            all(item.sha256.startswith("sha256:") for item in result.artifacts)
        )

    def test_compiler_failure_is_a_structured_result(self) -> None:
        result = run_native_compiler(
            NativeRequest(
                acir=b"not mlir",
                stop_after="acir-parse",
                emits=(),
            )
        )

        self.assertEqual((), result.artifacts)
        self.assertEqual("ACIR-PARSE-001", result.diagnostics[0].code)

    def test_private_extension_is_not_exported_publicly(self) -> None:
        self.assertNotIn("_native", agentic_circuit.__all__)
        self.assertNotIn("_native_api", agentic_circuit.__all__)

    def test_wrapper_records_are_immutable(self) -> None:
        result = run_native_compiler(
            NativeRequest(acir=VALID_ACIR, stop_after="acir-parse", emits=())
        )
        with self.assertRaises(FrozenInstanceError):
            result.executable = "changed"  # type: ignore[misc]

    def test_capabilities_have_stable_build_identity(self) -> None:
        found = capabilities()
        self.assertTrue(found.compiler_build_id)
        self.assertTrue(found.runtime_build_id)
        self.assertIsInstance(found.items, tuple)


if __name__ == "__main__":
    unittest.main()
