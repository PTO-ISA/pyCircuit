from __future__ import annotations

import ast
import importlib
import re
import unittest
from pathlib import Path

SPEC_PATHS = (
    "docs/acir/spec/agentic-circuit.md",
    "docs/acir/spec/agentic-circuit.zh-CN.md",
)


def _spec_inventory(text: str, name: str) -> tuple[str, ...]:
    """Parse ``agentic_circuit.<name> = (...)`` from a specification file."""

    match = re.search(
        rf"agentic_circuit\.{name} = (\([^)]*\))",
        text,
        re.DOTALL,
    )
    assert match is not None, f"spec does not enumerate {name}"
    inventory = ast.literal_eval(match.group(1))
    assert isinstance(inventory, tuple)
    return tuple(str(item) for item in inventory)


def _spec_inventory_count(text: str, name: str) -> int:
    """Parse the ``# inventory-size: N`` annotation beside an inventory."""

    match = re.search(
        rf"# inventory-size: (\d+)\s*\nagentic_circuit\.{name} = \(",
        text,
    )
    assert match is not None, f"spec does not count {name}"
    return int(match.group(1))


class SpecInventoryTest(unittest.TestCase):
    def test_spec_inventories_match_the_implementation(self) -> None:
        api = importlib.import_module("agentic_circuit")
        markers = importlib.import_module("agentic_circuit.markers")
        root = Path(__file__).resolve().parents[4]

        for relative in SPEC_PATHS:
            with self.subTest(spec=relative):
                text = (root / relative).read_text(encoding="utf-8")
                self.assertEqual(
                    tuple(markers.CAPTURE_ONLY_API),
                    _spec_inventory(text, "CAPTURE_ONLY_API"),
                )
                self.assertEqual(
                    tuple(api.RESERVED_API),
                    _spec_inventory(text, "RESERVED_API"),
                )
                self.assertEqual(
                    len(markers.CAPTURE_ONLY_API),
                    _spec_inventory_count(text, "CAPTURE_ONLY_API"),
                )
                self.assertEqual(
                    len(api.RESERVED_API),
                    _spec_inventory_count(text, "RESERVED_API"),
                )


if __name__ == "__main__":
    unittest.main()
