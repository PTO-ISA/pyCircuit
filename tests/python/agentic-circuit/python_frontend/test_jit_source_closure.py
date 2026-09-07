from __future__ import annotations

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


if __name__ == "__main__":
    unittest.main()
