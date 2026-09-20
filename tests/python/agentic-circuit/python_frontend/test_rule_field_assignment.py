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
    entries = ac.table[4, Entry](init=0)
    result = write(entries, command)
    return result
"""

STATE_EXPLICIT_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n",
    "    compiler_field_index_0 = command.index\n"
    "    entries[compiler_field_index_0] = "
    "entries[compiler_field_index_0].with_fields(valid=True)\n",
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
    "    local = entries[command.index]\n    local.valid = True\n    return local\n",
)

BRANCH_STATE_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n",
    "    if command.valid:\n        entries[command.index].value = command.value\n",
)

BRANCH_STATE_EXPLICIT_SOURCE = BRANCH_STATE_SOURCE.replace(
    "        entries[command.index].value = command.value\n",
    "        compiler_field_index_0 = command.index\n"
    "        entries[compiler_field_index_0] = "
    "entries[compiler_field_index_0].with_fields(value=command.value)\n",
)

WITH_FIELDS_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n",
    "    old = entries[command.index]\n"
    "    entries[command.index] = old.with_fields(\n"
    "        valid=True, value=command.value\n"
    "    )\n",
)

MULTI_FIELD_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n",
    "    entries[command.index].valid = True\n"
    "    entries[command.index].value = command.value\n",
)

REPEATED_FIELD_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n",
    "    entries[command.index].valid = False\n"
    "    entries[command.index].valid = True\n",
)

OUTPUTLESS_SOURCE = STATE_SOURCE.replace(
    "def write(entries, command):\n"
    "    entries[command.index].valid = True\n"
    "    return command\n",
    "def write(entries, command) -> None:\n    entries[command.index].valid = True\n",
).replace(
    "def state_update(command: Entry) -> Entry:\n"
    "    entries = ac.table[4, Entry](init=0)\n"
    "    result = write(entries, command)\n"
    "    return result\n",
    "def state_update(command: Entry) -> None:\n"
    "    entries = ac.table[4, Entry](init=0)\n"
    "    write(entries, command)\n",
)

BRANCH_SAME_FIELDS_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n",
    "    if command.valid:\n"
    "        entries[command.index].valid = True\n"
    "    else:\n"
    "        entries[command.index].valid = False\n",
)

BRANCH_DIFFERENT_FIELDS_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n",
    "    if command.valid:\n"
    "        entries[command.index].valid = True\n"
    "    else:\n"
    "        entries[command.index].value = command.value\n",
)

CROSS_BRANCH_SOURCE = STATE_SOURCE.replace(
    "    entries[command.index].valid = True\n",
    "    entries[command.index].valid = True\n"
    "    if command.valid:\n"
    "        entries[command.index].value = command.value\n",
)

UNPROVEN_SOURCES = (
    WITH_FIELDS_SOURCE.replace(
        "entries[command.index] = old.with_fields(",
        "entries[0] = old.with_fields(",
    ),
    WITH_FIELDS_SOURCE.replace(
        "old.with_fields(\n        valid=True, value=command.value\n    )",
        "command.with_fields(valid=True)",
    ),
    WITH_FIELDS_SOURCE.replace(
        "old.with_fields(\n        valid=True, value=command.value\n    )",
        "command",
    ),
)

OTHER_OWNER_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Entry:
    index: ac.u2
    value: ac.u8
    valid: bool

@ac.rule
def write(left, right, command):
    old = left[command.index]
    right[command.index] = old.with_fields(valid=True)
    return old

@ac.system
def state_update(command: Entry) -> Entry:
    left = ac.table[4, Entry](init=0)
    right = ac.table[4, Entry](init=0)
    result = write(left, right, command)
    return result
"""

MODULE_LOCAL_SOURCE = """
import agentic_circuit as ac

@ac.struct
class Entry:
    index: ac.u2
    value: ac.u8
    valid: bool

@ac.module_decl(source="tests/python/agentic-circuit/python_frontend/test_rule_field_assignment.py")
def stateful(command: Entry) -> Entry:
    ...

stateful_decl = stateful

@ac.module(declaration=stateful_decl)
def stateful(command: Entry) -> Entry:
    entries = ac.table[4, Entry](init=0)

    @ac.rule
    def mark(command) -> None:
        entries[command.index].valid = True

    mark(command)
    return command

@ac.system
def state_update(command: Entry) -> Entry:
    result = stateful(command)
    return result
"""

ALIAS_WITH_FIELDS_SOURCE = WITH_FIELDS_SOURCE.replace(
    "    entries[command.index] = old.with_fields(\n",
    "    alias = old\n    entries[command.index] = alias.with_fields(\n",
)

HELPER_WITH_FIELDS_SOURCE = WITH_FIELDS_SOURCE.replace(
    "@ac.rule\ndef write(entries, command):\n",
    "def patch(entry: Entry, value: ac.u8) -> Entry:\n"
    "    return entry.with_fields(valid=True, value=value)\n\n"
    "@ac.rule\ndef write(entries, command):\n",
).replace(
    "old.with_fields(\n        valid=True, value=command.value\n    )",
    "patch(old, command.value)",
)

INLINE_HELPER_WITH_FIELDS_SOURCE = HELPER_WITH_FIELDS_SOURCE.replace(
    "def patch(entry: Entry, value: ac.u8) -> Entry:\n",
    "@ac.inline\ndef patch(entry: Entry, value: ac.u8) -> Entry:\n",
)

MODULE_HELPER_WITH_FIELDS_SOURCE = MODULE_LOCAL_SOURCE.replace(
    "@ac.module(declaration=stateful_decl)\n"
    "def stateful(command: Entry) -> Entry:\n",
    "def patch(entry: Entry) -> Entry:\n"
    "    return entry.with_fields(valid=True)\n\n"
    "@ac.module(declaration=stateful_decl)\n"
    "def stateful(command: Entry) -> Entry:\n",
).replace(
    "        entries[command.index].valid = True\n",
    "        old = entries[command.index]\n"
    "        entries[command.index] = patch(old)\n",
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

        self.assertIn("ac.table.get @entries", lowered)
        self.assertIn("ac.var.with", lowered)
        self.assertNotIn("ac.var.assign_element @entries", lowered)

    def test_indexed_field_assignment_lowers_as_one_state_proposal(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(STATE_SOURCE, "state_update")

        self.assertEqual(1, lowered.count("ac.table.propose @entries"))
        self.assertIn("ac.var.with", lowered)
        self.assertIn('mode "field" write_fields ["valid"]', lowered)

    def test_proven_with_fields_chain_uses_canonical_field_footprint(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(WITH_FIELDS_SOURCE, "state_update")

        self.assertEqual(1, lowered.count("ac.table.propose @entries"))
        self.assertIn('mode "field" write_fields ["value", "valid"]', lowered)

    def test_unproven_entry_values_remain_complete_replacements(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        for source in UNPROVEN_SOURCES:
            with self.subTest(source=source):
                lowered = lower_queue_source(source, "state_update")
                self.assertIn(
                    'mode "replace" write_fields ["index", "value", "valid"]',
                    lowered,
                )
        other_owner = lower_queue_source(OTHER_OWNER_SOURCE, "state_update")
        self.assertIn(
            'mode "replace" write_fields ["index", "value", "valid"]',
            other_owner,
        )

    def test_serial_indexed_updates_coalesce_in_declaration_order(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(MULTI_FIELD_SOURCE, "state_update")

        self.assertEqual(1, lowered.count("ac.table.get @entries"))
        self.assertEqual(1, lowered.count("ac.table.propose @entries"))
        self.assertEqual(2, lowered.count("ac.var.with"))
        self.assertIn('mode "field" write_fields ["value", "valid"]', lowered)

    def test_repeated_field_update_keeps_one_last_value_proposal(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(REPEATED_FIELD_SOURCE, "state_update")

        self.assertEqual(1, lowered.count("ac.table.get @entries"))
        self.assertEqual(1, lowered.count("ac.table.propose @entries"))
        self.assertIn('mode "field" write_fields ["valid"]', lowered)

    def test_outputless_rule_emits_a_field_proposal(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(OUTPUTLESS_SOURCE, "state_update")

        self.assertIn("ac.rule %command depths [] latencies []", lowered)
        self.assertIn('mode "field" write_fields ["valid"]', lowered)
        self.assertNotIn("ac.marker.obligation", lowered)

    def test_module_local_table_keeps_field_intent_for_storage_selection(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(MODULE_LOCAL_SOURCE, "state_update")

        self.assertIn("ac.var.decl @entries", lowered)
        self.assertIn("ac.var.assign_element @entries", lowered)
        self.assertEqual(1, lowered.count("ac.var.with"))

    def test_alias_and_pure_helpers_share_the_same_table_field_footprint(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        sources = (
            ALIAS_WITH_FIELDS_SOURCE,
            HELPER_WITH_FIELDS_SOURCE,
            INLINE_HELPER_WITH_FIELDS_SOURCE,
        )
        for source in sources:
            with self.subTest(source=source):
                lowered = lower_queue_source(source, "state_update")
                self.assertEqual(1, lowered.count("ac.table.propose @entries"))
                self.assertIn('mode "field" write_fields ["value", "valid"]', lowered)

        module = lower_queue_source(MODULE_HELPER_WITH_FIELDS_SOURCE, "state_update")
        self.assertIn("ac.var.assign_element @entries", module)
        self.assertIn('field "valid"', module)
        self.assertNotIn("func.call @patch", module)

    def test_complementary_branches_join_only_matching_footprints(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        same = lower_queue_source(BRANCH_SAME_FIELDS_SOURCE, "state_update")
        different = lower_queue_source(BRANCH_DIFFERENT_FIELDS_SOURCE, "state_update")

        self.assertEqual(1, same.count("ac.table.propose @entries"))
        self.assertIn('mode "field" write_fields ["valid"]', same)
        self.assertEqual(2, different.count("ac.table.propose @entries"))
        self.assertIn('mode "field" write_fields ["valid"]', different)
        self.assertIn('mode "field" write_fields ["value"]', different)

    def test_field_updates_cannot_cross_a_branch_boundary(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        with self.assertRaisesRegex(QueueFrontendError, "ACPY-RULE-011"):
            lower_queue_source(CROSS_BRANCH_SOURCE, "state_update")

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
        self.assertEqual(3, source.count("compiler_field_index_0"))

        serial = ast.parse(
            "entries[next_index()].valid = True\nentries[next_index()].value = value"
        ).body
        normalized_serial = _normalize_rule_field_assignments(
            serial, reserved_names={"entries", "next_index", "value"}
        )
        serial_source = ast.unparse(ast.Module(body=normalized_serial, type_ignores=[]))
        self.assertEqual(1, serial_source.count("next_index()"))

    def test_generated_index_name_avoids_rule_local_collisions(self) -> None:
        from agentic_circuit._queue_frontend import (
            _normalize_rule_field_assignments,
        )

        normalized = _normalize_rule_field_assignments(
            ast.parse("entries[index].valid = True").body,
            reserved_names={"compiler_field_index_0", "entries", "index"},
        )
        source = ast.unparse(ast.Module(body=normalized, type_ignores=[]))

        self.assertNotIn("compiler_field_index_0 = index", source)
        self.assertEqual(3, source.count("compiler_field_index_1"))

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
            with (
                self.subTest(source=source),
                self.assertRaisesRegex(QueueFrontendError, "ACPY-RULE-002"),
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
                LOCAL_SOURCE.replace("local.valid = True", "local.value = local.valid"),
                "field update type mismatch",
            ),
            (STATE_SOURCE.replace("command.index", "4"), "index is out of range"),
        )
        for source, message in cases:
            with (
                self.subTest(message=message),
                self.assertRaisesRegex(QueueFrontendError, message),
            ):
                lower_queue_source(
                    source,
                    "local_update" if "def local_update" in source else "state_update",
                )
