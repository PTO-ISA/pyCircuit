from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

import agentic_circuit as ac
from agentic_circuit._jit import lower_source_unit

SOURCE = """
import agentic_circuit as ac

@ac.rule
def retain(value: ac.u8) -> ac.u8:
    return value

@ac.module
def bank(value: ac.u8, *, index: ac.const[int]) -> ac.u8:
    result = retain(value)
    return result

"""


class SourceUnitTest(unittest.TestCase):
    def test_zero_port_module_is_a_real_source_unit(self) -> None:
        with tempfile.TemporaryDirectory(prefix="agentic-empty-source-unit-") as temporary:
            root = Path(temporary)
            source = root / "idle.py"
            source.write_text(
                "import agentic_circuit as ac\n\n"
                "@ac.module\n"
                "def idle() -> None:\n"
                "    pass\n",
                encoding="utf-8",
            )
            spec = importlib.util.spec_from_file_location("idle", source)
            assert spec is not None and spec.loader is not None
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            try:
                spec.loader.exec_module(module)
                rendered = lower_source_unit(
                    (ac.jit(module.idle, workspace=root),)
                )
            finally:
                sys.modules.pop(spec.name, None)

        self.assertIn("ac.module @idle()", rendered)
        self.assertIn("ac.instance @idle_0 of @idle()", rendered)
        self.assertIn(": () -> ()", rendered)

    def test_one_lowering_contains_one_modules_typed_specializations(self) -> None:
        with tempfile.TemporaryDirectory(prefix="agentic-source-unit-") as temporary:
            root = Path(temporary)
            source = root / "pipeline.py"
            source.write_text(SOURCE, encoding="utf-8")
            spec = importlib.util.spec_from_file_location("pipeline", source)
            assert spec is not None and spec.loader is not None
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            try:
                spec.loader.exec_module(module)
                rendered = lower_source_unit(
                    (
                        ac.jit(module.bank, workspace=root, index=0),
                        ac.jit(module.bank, workspace=root, index=1),
                    )
                )
            finally:
                sys.modules.pop(spec.name, None)

        self.assertEqual(1, rendered.count("ac.module @bank__index_0"))
        self.assertEqual(1, rendered.count("ac.module @bank__index_1"))
        self.assertIn('ac.definition_name = "bank"', rendered)

    def test_source_unit_rejects_multiple_public_modules(self) -> None:
        with tempfile.TemporaryDirectory(prefix="agentic-source-unit-") as temporary:
            root = Path(temporary)
            source = root / "pipeline.py"
            source.write_text(
                SOURCE
                + "\n@ac.module\ndef stage(value: ac.u8) -> ac.u8:\n"
                + "    return value\n",
                encoding="utf-8",
            )
            spec = importlib.util.spec_from_file_location("pipeline", source)
            assert spec is not None and spec.loader is not None
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            try:
                spec.loader.exec_module(module)
                with self.assertRaisesRegex(
                    TypeError, "exactly one public module"
                ):
                    lower_source_unit(
                        (
                            ac.jit(module.bank, workspace=root, index=0),
                            ac.jit(module.stage, workspace=root),
                        )
                    )
            finally:
                sys.modules.pop(spec.name, None)


if __name__ == "__main__":
    unittest.main()
