from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


WORKSPACE = Path(__file__).resolve().parent / "fixtures" / "process"


def load_fixture(name: str):
    path = WORKSPACE / name
    spec = importlib.util.spec_from_file_location(f"process_{path.stem}", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import process fixture {path}")
    loaded = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(loaded)
    return loaded


def process_site(fixture: str, name: str):
    from agentic_circuit._source import load_source_unit

    unit = load_source_unit(WORKSPACE / fixture, WORKSPACE)
    return next(site for site in unit.definitions if site.name == name)


def effect_registry():
    from agentic_circuit._process import EffectDeclaration, EffectRegistry

    return EffectRegistry(
        (
            EffectDeclaration("try_recv", "queue"),
            EffectDeclaration("schedule", "event"),
            EffectDeclaration("wait_for", "suspension", suspension=True),
            EffectDeclaration("record_stat", "statistics"),
            EffectDeclaration("trace_next", "trace", linear_arguments=(0,)),
        )
    )


class ProcessFrontendTest(unittest.TestCase):
    def test_nested_control_and_suspension_build_closed_cfg(self) -> None:
        from agentic_circuit._process import construct_process

        process = construct_process(
            process_site("suspended.py", "controller"), effect_registry()
        )

        self.assertEqual("entry", process.entry)
        self.assertEqual(
            ["entry", "then", "else", "resume", "done"],
            [block.name for block in process.blocks],
        )
        self.assertTrue(any(block.edge.kind == "suspend" for block in process.blocks))
        self.assertEqual(
            ("ready", "requests", "memory"),
            tuple(value.source_name for value in process.captures),
        )
        self.assertEqual(
            ("queue", "event", "suspension", "statistics"),
            tuple(effect.kind for effect in process.effects),
        )
        done = next(block for block in process.blocks if block.name == "done")
        self.assertEqual(
            ("message",), tuple(value.source_name for value in done.arguments)
        )

    def test_captured_program_constructs_selected_process(self) -> None:
        from agentic_circuit._frontend import (
            CaptureRequest,
            capture_definitions,
            construct_captured_process,
        )
        from agentic_circuit._schemas import SchemaRegistry

        loaded = load_fixture("suspended.py")
        captured = capture_definitions(
            CaptureRequest(
                entry=WORKSPACE / "suspended.py",
                workspace=WORKSPACE,
                system="main",
            ),
            vars(loaded),
            SchemaRegistry({}),
        )
        self.assertEqual((), captured.diagnostics)

        process = construct_captured_process(captured, "controller", effect_registry())

        self.assertEqual("controller", process.name)

    def test_bounded_for_is_an_explicit_closed_loop(self) -> None:
        from agentic_circuit._process import construct_process

        process = construct_process(
            process_site("suspended.py", "bounded_counter"), effect_registry()
        )

        self.assertEqual(
            ["entry", "for_loop", "for_body", "after_for", "resume"],
            [block.name for block in process.blocks],
        )
        body = next(block for block in process.blocks if block.name == "for_body")
        self.assertEqual(
            ("index",), tuple(value.source_name for value in body.arguments)
        )
        names = {block.name for block in process.blocks}
        self.assertTrue(
            all(
                target in names
                for block in process.blocks
                for target in block.edge.targets
            )
        )

    def test_fully_terminating_branch_has_no_unreachable_join(self) -> None:
        from agentic_circuit._process import construct_process

        process = construct_process(
            process_site("suspended.py", "terminating_process"), effect_registry()
        )

        self.assertEqual(
            ["entry", "then", "else"], [block.name for block in process.blocks]
        )

    def test_busy_wait_coroutine_generator_and_undeclared_effect_are_rejected(
        self,
    ) -> None:
        from agentic_circuit._process import ProcessConstructionError, construct_process

        expected = {
            "coroutine_process": "ACPY-PROCESS-002",
            "generator_process": "ACPY-PROCESS-002",
            "busy_process": "ACPY-PROCESS-006",
            "partial_busy_process": "ACPY-PROCESS-006",
            "forked_cursor_process": "ACPY-PROCESS-007",
            "undeclared_effect_process": "ACPY-EFFECT-003",
        }
        observed: set[str] = set()
        for name, code in expected.items():
            with self.subTest(process=name):
                with self.assertRaisesRegex(ProcessConstructionError, code) as raised:
                    construct_process(
                        process_site("invalid.py", name), effect_registry()
                    )
                observed.add(raised.exception.code)

        self.assertEqual(set(expected.values()), observed)


if __name__ == "__main__":
    unittest.main()
