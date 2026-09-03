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
    frontend = f"{module_prefix}test_frontend_commands.FrontendCommandTest"
    compile_command = f"{module_prefix}test_compile_command.CompileCommandTest"
    build = f"{module_prefix}test_build_command.BuildCommandTest"
    run = f"{module_prefix}test_run_command.RunCommandTest"
    inspect = f"{module_prefix}test_inspect_command.InspectCommandTest"
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
        "check": CommandCoverage(
            (f"{frontend}.test_check_is_fast_machine_readable_and_writes_no_build",),
            (f"{parser}.test_unknown_toml_key_is_exit_two",),
            (f"{frontend}.test_check_can_stop_after_verified_acpy",),
            (f"{frontend}.test_check_is_fast_machine_readable_and_writes_no_build",),
        ),
        "elaborate": CommandCoverage(
            (
                f"{frontend}.test_elaborate_is_deterministic_and_captures_project_output",
            ),
            (f"{compile_command}.test_invalid_options_fail_before_publishing",),
            (
                f"{frontend}.test_elaborate_is_deterministic_and_captures_project_output",
            ),
            (f"{frontend}.test_elaborate_acir_is_verified_and_atomically_replaced",),
        ),
        "compile": CommandCoverage(
            (f"{compile_command}.test_all_exact_emits_are_published_in_fixed_order",),
            (f"{compile_command}.test_invalid_options_fail_before_publishing",),
            (
                f"{compile_command}.test_dump_after_each_uses_every_selected_logical_stage",
            ),
            (f"{compile_command}.test_all_exact_emits_are_published_in_fixed_order",),
        ),
        "build": CommandCoverage(
            (f"{build}.test_manifest_records_frontend_and_exact_profile",),
            (f"{exits}.test_missing_cpp_compiler_is_four",),
            (f"{build}.test_identical_build_reports_cache_hit",),
            (f"{build}.test_identical_build_reports_cache_hit",),
        ),
        "run": CommandCoverage(
            (f"{run}.test_completed_run_publishes_exact_documents",),
            (f"{run}.test_invalid_trace_is_preflight_five_and_preserves_output",),
            (f"{run}.test_replay_uses_only_the_immutable_bundle",),
            (f"{run}.test_tick_cap_is_incomplete_exit_seven",),
        ),
        "inspect": CommandCoverage(
            (f"{inspect}.test_every_exact_view_is_machine_readable_and_read_only",),
            (
                f"{inspect}.test_hierarchy_path_is_canonical_and_unknown_path_is_diagnostic",
            ),
            (f"{inspect}.test_graphviz_output_is_deterministic_and_host_independent",),
            (f"{inspect}.test_every_exact_view_is_machine_readable_and_read_only",),
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
