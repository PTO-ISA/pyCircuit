from __future__ import annotations

import unittest
from dataclasses import dataclass

from agentic_circuit._cli import EXACT_COMMANDS


@dataclass(frozen=True, slots=True)
class CommandCoverage:
    success: tuple[str, ...]
    error: tuple[str, ...]
    determinism: tuple[str, ...]
    machine_readable: tuple[str, ...]


def cli_test_ledger() -> dict[str, CommandCoverage]:
    module_prefix = f"{__package__}." if __package__ else ""
    parser = f"{module_prefix}test_cli_parser.CliParserTest"
    discovery = f"{module_prefix}test_discovery_commands.DiscoveryCommandTest"
    exits = f"{module_prefix}test_exit_codes.ExitCodeTest"
    return {
        "init": CommandCoverage(
            (f"{parser}.test_json_stdout_contains_one_value_and_no_prose",),
            (f"{parser}.test_init_refuses_conflicts_unless_each_is_forced",),
            (f"{parser}.test_json_stdout_contains_one_value_and_no_prose",),
            (f"{parser}.test_json_stdout_contains_one_value_and_no_prose",),
        ),
        "schema": CommandCoverage(
            (f"{discovery}.test_component_protocol_and_list_queries_are_exact",),
            (f"{discovery}.test_unknown_schema_name_is_a_structured_user_error",),
            (f"{discovery}.test_capabilities_match_schema_without_importing_project",),
            (f"{discovery}.test_component_protocol_and_list_queries_are_exact",),
        ),
        "explain": CommandCoverage(
            (f"{discovery}.test_explain_and_doctor_are_read_only",),
            (f"{discovery}.test_unknown_schema_name_is_a_structured_user_error",),
            (f"{discovery}.test_explain_and_doctor_are_read_only",),
            (f"{discovery}.test_explain_and_doctor_are_read_only",),
        ),
        "doctor": CommandCoverage(
            (f"{discovery}.test_explain_and_doctor_are_read_only",),
            (f"{exits}.test_source_checkout_doctor_reports_missing_native_tools",),
            (f"{discovery}.test_explain_and_doctor_are_read_only",),
            (f"{discovery}.test_explain_and_doctor_are_read_only",),
        ),
    }


class AllCommandsTest(unittest.TestCase):
    def test_every_command_has_required_behavior_classes(self) -> None:
        ledger = cli_test_ledger()

        self.assertEqual(set(EXACT_COMMANDS), set(ledger))
        for command, row in ledger.items():
            for behavior, tests in (
                ("success", row.success),
                ("error", row.error),
                ("determinism", row.determinism),
                ("machine_readable", row.machine_readable),
            ):
                self.assertTrue(tests, (command, behavior))
                for test_name in tests:
                    suite = unittest.defaultTestLoader.loadTestsFromName(test_name)
                    loaded = tuple(suite)
                    self.assertEqual(1, len(loaded), (command, behavior, test_name))
                    self.assertNotEqual("_FailedTest", type(loaded[0]).__name__)


if __name__ == "__main__":
    unittest.main()
