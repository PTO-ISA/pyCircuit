from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


REPOSITORY = Path(__file__).resolve().parents[4]
WORKSPACE = Path(__file__).resolve().parent / "fixtures" / "normalize"
ZERO_DIGEST = "sha256:" + "0" * 64


def load_fixture(name: str):
    path = WORKSPACE / name
    spec = importlib.util.spec_from_file_location(f"normalize_{path.stem}", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import fixture {path}")
    loaded = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(loaded)
    return loaded


def component_schema(
    identity: str,
    *,
    ports: tuple[str, ...],
    results: tuple[str, ...],
    parameters: tuple[tuple[str, bool, object], ...] = (),
):
    from agentic_circuit._schemas import (
        ComponentSchema,
        ParameterSchema,
        PortSchema,
        ResultSchema,
    )

    return ComponentSchema(
        identity=identity,
        fingerprint=ZERO_DIGEST,
        ports=tuple(
            PortSchema(name, "flow", f"!test.flow<{name}>", "in", "consumer", 1)
            for name in ports
        ),
        results=tuple(
            ResultSchema(name, f"!test.flow<{name}>", None) for name in results
        ),
        parameters=tuple(
            ParameterSchema(name, "!test.static", required, default, None)
            for name, required, default in parameters
        ),
        availability="available",
    )


def registry_for(name: str):
    from agentic_circuit._schemas import SchemaRegistry

    decode = component_schema(
        "test.Decode",
        ports=("input",),
        results=("decoded", "accepted"),
        parameters=(("mode", False, "strict"),),
    )
    if name == "ambiguous.py":
        alternate = component_schema(
            "alternate.Decode", ports=("input",), results=("decoded",)
        )
        decode = component_schema("test.Decode", ports=("input",), results=("decoded",))
        return SchemaRegistry({decode.identity: decode, alternate.identity: alternate})
    refine = component_schema("test.Refine", ports=("input",), results=("output",))
    return SchemaRegistry({decode.identity: decode, refine.identity: refine})


def normalize_fixture(name: str):
    from agentic_circuit._frontend import CaptureRequest, capture_definitions
    from agentic_circuit._normalize import normalize_program

    loaded = load_fixture(name)
    registry = registry_for(name)
    captured = capture_definitions(
        CaptureRequest(entry=WORKSPACE / name, workspace=WORKSPACE, system="main"),
        vars(loaded),
        registry,
    )
    if captured.diagnostics:
        raise AssertionError(captured.diagnostics)
    return normalize_program(captured, definition="pipeline")


class NormalizationTest(unittest.TestCase):
    def test_calls_become_explicit_ssa_and_results(self) -> None:
        program = normalize_fixture("calls.py")

        self.assertEqual(
            ("decoded#0", "accepted#0", "decoded#1"), program.value_names()
        )
        self.assertEqual(
            ("input",), tuple(item.port for item in program.calls[0].inputs)
        )
        self.assertEqual(
            ("decoded", "accepted"),
            tuple(item.result for item in program.calls[0].results),
        )
        self.assertEqual("decoded#0", program.calls[1].inputs[0].value.name)
        self.assertEqual(("decoded#1", "accepted#0"), program.return_names())

    def test_ambiguity_reports_each_attempted_binding(self) -> None:
        program = normalize_fixture("ambiguous.py")

        matches = [item for item in program.diagnostics if item.code == "ACPY-CALL-006"]
        self.assertEqual(1, len(matches))
        self.assertEqual(2, len(matches[0].related))
        self.assertTrue(
            all("attempted" in related.message for related in matches[0].related)
        )

    def test_naming_precedence_and_collision_are_explicit(self) -> None:
        from agentic_circuit._naming import StableNameAllocator, StableNameError

        allocator = StableNameAllocator()

        self.assertEqual(
            "front", allocator.allocate("test.Decode", "decoded", "front", (3, 5))
        )
        self.assertEqual(
            "accepted",
            allocator.allocate("test.Decode", "accepted", None, (4, 5)),
        )
        self.assertEqual(
            "Decode_5_7", allocator.allocate("test.Decode", None, None, (5, 7))
        )
        with self.assertRaisesRegex(StableNameError, "ACPY-NAME-002"):
            allocator.allocate("test.Decode", "accepted", None, (6, 5))


if __name__ == "__main__":
    unittest.main()
