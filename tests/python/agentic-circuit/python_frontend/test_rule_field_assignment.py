from __future__ import annotations

import ast
import unittest

LOCAL_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Entry:
    index: ac.u2
    value: ac.u8
    valid: bool

@ac.rule
def mark(command):
    local = command
    local.valid = True
    return local

@ac.system
def local_update(command: Entry) -> Entry:
    result = mark(command)
    return result
"""


STATE_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Entry:
    index: ac.u2
    value: ac.u8
    valid: bool

@ac.rule
def write(entries, command):
    entries[command.index].valid = True
    return command

@ac.system
def state_update(command: Entry) -> Entry:
    entries: list[Entry] = [0] * 4
    result = write(entries, command)
    return result
"""

STATE_EXPLICIT_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n",
    "    __ac_field_index_0 = command.index\n"
    "    entries[__ac_field_index_0] = "
    "entries[__ac_field_index_0].with_fields(valid=True)\n",
)

LOCAL_SERIAL_SOURCE = LOCAL_SOURCE.replace(
    "    local.valid = True\n",
    "    local.valid = True\n    local.value = local.value + 1\n",
)

LOCAL_SERIAL_EXPLICIT_SOURCE = LOCAL_SERIAL_SOURCE.replace(
    "    local.valid = True\n    local.value = local.value + 1\n",
    "    local = local.with_fields(valid=True)\n"
    "    local = local.with_fields(value=local.value + 1)\n",
)

SCALAR_STATE_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Entry:
    value: ac.u8
    valid: bool

@ac.rule
def write(state, command):
    state.valid = True
    return state

@ac.system
def scalar_update(command: Entry) -> Entry:
    state: Entry = 0
    result = write(state, command)
    return result
"""

SCALAR_STATE_EXPLICIT_SOURCE = SCALAR_STATE_SOURCE.replace(
    "    state.valid = True\n",
    "    state = state.with_fields(valid=True)\n",
)

SCALAR_STATE_SERIAL_SOURCE = SCALAR_STATE_SOURCE.replace(
    "    state.valid = True\n",
    "    state.valid = True\n    state.value = state.value + 1\n",
)

SCALAR_STATE_SERIAL_EXPLICIT_SOURCE = SCALAR_STATE_SERIAL_SOURCE.replace(
    "    state.valid = True\n    state.value = state.value + 1\n",
    "    state = state.with_fields(valid=True)\n"
    "    state = state.with_fields(value=state.value + 1)\n",
)

LOCAL_COPY_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n    return command\n",
    "    local = entries[command.index]\n"
    "    local.valid = True\n"
    "    return local\n",
)

BRANCH_STATE_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n",
    "    if command.valid:\n"
    "        entries[command.index].value = command.value\n",
)

BRANCH_STATE_EXPLICIT_SOURCE = BRANCH_STATE_SOURCE.replace(
    "        entries[command.index].value = command.value\n",
    "        __ac_field_index_0 = command.index\n"
    "        entries[__ac_field_index_0] = "
    "entries[__ac_field_index_0].with_fields(value=command.value)\n",
)


class RuleFieldAssignmentTest(unittest.TestCase):
    def test_local_field_assignment_lowers_as_immutable_rebinding(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(LOCAL_SOURCE, "local_update")

        self.assertIn("ac.var.with", lowered)
        self.assertNotIn("ac.table", lowered)

    def test_serial_local_field_assignments_observe_latest_ssa_value(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        shorthand = lower_queue_source(LOCAL_SERIAL_SOURCE, "local_update")
        explicit = lower_queue_source(LOCAL_SERIAL_EXPLICIT_SOURCE, "local_update")

        self.assertEqual(explicit, shorthand)
        self.assertEqual(2, shorthand.count("ac.var.with"))

    def test_persistent_scalar_field_assignment_is_one_state_proposal(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        shorthand = lower_queue_source(SCALAR_STATE_SOURCE, "scalar_update")
        explicit = lower_queue_source(SCALAR_STATE_EXPLICIT_SOURCE, "scalar_update")

        self.assertEqual(explicit, shorthand)
        self.assertEqual(1, shorthand.count("ac.var.assign @state"))

    def test_serial_scalar_field_assignments_coalesce_to_latest_proposal(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        shorthand = lower_queue_source(SCALAR_STATE_SERIAL_SOURCE, "scalar_update")
        explicit = lower_queue_source(
            SCALAR_STATE_SERIAL_EXPLICIT_SOURCE, "scalar_update"
        )

        self.assertEqual(explicit, shorthand)
        self.assertEqual(2, shorthand.count("ac.var.with"))
        self.assertEqual(1, shorthand.count("ac.var.assign @state"))

    def test_local_copy_update_does_not_write_back_persistent_state(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(LOCAL_COPY_SOURCE, "state_update")

        self.assertIn("ac.var.read_element @entries", lowered)
        self.assertIn("ac.var.with", lowered)
        self.assertNotIn("ac.var.assign_element @entries", lowered)

    def test_indexed_field_assignment_lowers_as_one_state_proposal(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(STATE_SOURCE, "state_update")

        self.assertEqual(1, lowered.count("ac.var.assign_element @entries"))
        self.assertIn("ac.var.with", lowered)

    def test_field_assignment_matches_explicit_immutable_update_acir(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        shorthand = lower_queue_source(STATE_SOURCE, "state_update")
        explicit = lower_queue_source(STATE_EXPLICIT_SOURCE, "state_update")

        self.assertEqual(explicit, shorthand)

    def test_branch_field_assignment_matches_explicit_update_acir(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        shorthand = lower_queue_source(BRANCH_STATE_SOURCE, "state_update")
        explicit = lower_queue_source(BRANCH_STATE_EXPLICIT_SOURCE, "state_update")

        self.assertEqual(explicit, shorthand)
        self.assertIn(" when %", shorthand)

    def test_complex_field_index_is_evaluated_once(self) -> None:
        from agentic_circuit._queue_frontend import (
            _normalize_rule_field_assignments,
        )

        statement = ast.parse("entries[next_index()].valid = True").body
        normalized = _normalize_rule_field_assignments(
            statement, reserved_names={"entries", "next_index"}
        )
        source = ast.unparse(ast.Module(body=normalized, type_ignores=[]))

        self.assertEqual(1, source.count("next_index()"))
        self.assertEqual(3, source.count("__ac_field_index_0"))

    def test_generated_index_name_avoids_rule_local_collisions(self) -> None:
        from agentic_circuit._queue_frontend import (
            _normalize_rule_field_assignments,
        )

        normalized = _normalize_rule_field_assignments(
            ast.parse("entries[index].valid = True").body,
            reserved_names={"__ac_field_index_0", "entries", "index"},
        )
        source = ast.unparse(ast.Module(body=normalized, type_ignores=[]))

        self.assertNotIn("__ac_field_index_0 = index", source)
        self.assertEqual(3, source.count("__ac_field_index_1"))

    def test_nested_augmented_and_slice_field_updates_fail_closed(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            _normalize_rule_field_assignments,
        )

        for source in (
            "local.meta.valid = True",
            "local.valid += 1",
            "entries[0:2].valid = True",
        ):
            with self.subTest(source=source), self.assertRaisesRegex(
                QueueFrontendError, "ACPY-RULE-002"
            ):
                _normalize_rule_field_assignments(
                    ast.parse(source).body, reserved_names={"entries", "local"}
                )

    def test_field_assignment_preserves_semantic_diagnostics(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        cases = (
            (LOCAL_SOURCE.replace("local.valid", "local.missing"), "unknown field"),
            (
                LOCAL_SOURCE.replace(
                    "local.valid = True", "local.value = local.valid"
                ),
                "field update type mismatch",
            ),
            (STATE_SOURCE.replace("command.index", "4"), "index is out of range"),
        )
        for source, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(
                QueueFrontendError, message
            ):
                lower_queue_source(
                    source,
                    "local_update"
                    if "def local_update" in source
                    else "state_update",
                )
