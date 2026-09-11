from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


class JitSourceClosureTest(unittest.TestCase):
    def test_module_qualified_local_import_is_rejected_during_capture(self) -> None:
        from agentic_circuit._source_closure import (
            SourceClosureError,
            capture_source_closure,
        )

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "contracts.py").write_text("VALUE = 1\n", encoding="utf-8")
            entry = root / "top.py"
            entry.write_text("import contracts\n", encoding="utf-8")

            with self.assertRaisesRegex(
                SourceClosureError, "module-qualified local import"
            ):
                capture_source_closure(entry, root)

    def test_renamed_local_import_is_rejected_during_capture(self) -> None:
        from agentic_circuit._source_closure import (
            SourceClosureError,
            capture_source_closure,
        )

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "contracts.py").write_text("VALUE = 1\n", encoding="utf-8")
            entry = root / "top.py"
            entry.write_text(
                "from contracts import VALUE as RENAMED\n", encoding="utf-8"
            )

            with self.assertRaisesRegex(SourceClosureError, "renamed local import"):
                capture_source_closure(entry, root)

    def test_distinct_flattened_definitions_with_one_name_are_rejected(self) -> None:
        from agentic_circuit._source_closure import (
            SourceClosureError,
            capture_source_closure,
        )

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "left.py").write_text(
                "def helper():\n    return 1\n\nLEFT = helper()\n", encoding="utf-8"
            )
            (root / "right.py").write_text(
                "def helper():\n    return 2\n\nRIGHT = helper()\n", encoding="utf-8"
            )
            entry = root / "top.py"
            entry.write_text(
                "from left import LEFT\nfrom right import RIGHT\n", encoding="utf-8"
            )

            with self.assertRaisesRegex(SourceClosureError, "symbol 'helper'") as caught:
                capture_source_closure(entry, root)
            self.assertEqual("ACPY-JIT-006", caught.exception.code)
            self.assertIn("left.py", str(caught.exception))
            self.assertIn("right.py", str(caught.exception))

    def test_one_imported_definition_can_be_reused_by_multiple_modules(self) -> None:
        from agentic_circuit._source_closure import capture_source_closure

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "common.py").write_text("COMMON = 4\n", encoding="utf-8")
            (root / "left.py").write_text(
                "from common import COMMON\nLEFT = COMMON\n", encoding="utf-8"
            )
            (root / "right.py").write_text(
                "from common import COMMON\nRIGHT = COMMON\n", encoding="utf-8"
            )
            entry = root / "top.py"
            entry.write_text(
                "from left import LEFT\nfrom right import RIGHT\n", encoding="utf-8"
            )

            closure = capture_source_closure(entry, root)

        self.assertEqual(
            ("common.py", "left.py", "right.py", "top.py"),
            tuple(item.path for item in closure.entries),
        )

    def test_explicit_symbol_import_captures_payload_invariant_definition(self) -> None:
        from agentic_circuit._source_closure import capture_source_closure

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "contracts.py").write_text(
                "import agentic_circuit as ac\n\n"
                "@ac.struct\n"
                "class Payload:\n"
                "    value: ac.u8\n\n"
                "@ac.invariant\n"
                "def valid_payload(value: Payload) -> bool:\n"
                "    return value.value != 0\n",
                encoding="utf-8",
            )
            entry = root / "top.py"
            entry.write_text(
                "from contracts import Payload, valid_payload\n",
                encoding="utf-8",
            )

            closure = capture_source_closure(entry, root)

        self.assertEqual(
            ("contracts.py", "top.py"),
            tuple(item.path for item in closure.entries),
        )

    def test_imported_invariant_caller_and_callee_lower_from_source_closure(
        self,
    ) -> None:
        import agentic_circuit as ac

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "opt01_contracts.py").write_text(
                "import agentic_circuit as ac\n\n"
                "@ac.struct\n"
                "class Inner:\n"
                "    tag: ac.u8\n"
                "    valid: bool\n\n"
                "@ac.struct\n"
                "class Payload:\n"
                "    inner: Inner\n"
                "    valid: bool\n",
                encoding="utf-8",
            )
            (root / "opt01_z_callee.py").write_text(
                "import agentic_circuit as ac\n"
                "from opt01_contracts import Inner\n\n"
                "@ac.invariant\n"
                "def valid_inner(value: Inner) -> bool:\n"
                "    return value.valid and value.tag != 0\n",
                encoding="utf-8",
            )
            (root / "opt01_a_caller.py").write_text(
                "import agentic_circuit as ac\n"
                "from opt01_contracts import Payload\n"
                "from opt01_z_callee import valid_inner\n\n"
                "@ac.invariant\n"
                "def valid_payload(value: Payload) -> bool:\n"
                "    return valid_inner(value.inner) and value.valid\n",
                encoding="utf-8",
            )
            top = root / "opt01_top.py"
            top.write_text(
                "import agentic_circuit as ac\n"
                "from opt01_contracts import Payload\n"
                "from opt01_a_caller import valid_payload\n\n"
                "@ac.module\n"
                "def validate(value: Payload) -> Payload:\n"
                "    return value.with_fields(valid=valid_payload(value))\n\n"
                "@ac.system\n"
                "def composed(value: Payload) -> Payload:\n"
                "    return validate(value)\n",
                encoding="utf-8",
            )

            sys.path.insert(0, str(root))
            self.addCleanup(sys.path.remove, str(root))
            module_names = (
                "opt01_contracts",
                "opt01_z_callee",
                "opt01_a_caller",
                "opt01_top",
            )
            for name in module_names:
                sys.modules.pop(name, None)
                self.addCleanup(sys.modules.pop, name, None)
            spec = importlib.util.spec_from_file_location("opt01_top", top)
            if spec is None or spec.loader is None:
                raise RuntimeError("cannot load composed invariant fixture")
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            spec.loader.exec_module(module)

            specialization = ac.jit(module.composed, workspace=root)
            lowered = specialization.lower_acir()

        self.assertEqual(
            (
                "opt01_a_caller.py",
                "opt01_contracts.py",
                "opt01_top.py",
                "opt01_z_callee.py",
            ),
            tuple(item.path for item in specialization.sources),
        )
        self.assertIn('name "Payload.valid_payload"', lowered)
        self.assertIn('name "Inner.valid_inner"', lowered)
        self.assertIn("%invariant0_invariant1_value", lowered)


if __name__ == "__main__":
    unittest.main()
