"""Diagnostic clarity regressions for the Agentic Circuit queue frontend.

Issue #159 parts (a) and (b): two shapes failed with text that pointed at the
wrong cause.

* ``ac.sink(keep(q))`` reported ``ACPY-QUEUE-005: collection reference must be
  statically resolvable``, which sends the reader looking for an
  ``ac.array``/``ac.map`` mistake. The rule is that a Queue reference is a name
  or a static collection member, so a call result has to be bound to a name
  first.
* ``@ac.struct`` with a class docstring reported ``ACPY-QUEUE-002: struct body
  requires annotated fields`` for a payload whose fields *were* annotated.
  ``@ac.config`` and ``@ac.Enum`` already skip a docstring.
"""

from __future__ import annotations

import unittest

from agentic_circuit._queue_frontend import QueueFrontendError, lower_queue_source

DOCUMENTED_STRUCT = '''
import agentic_circuit as ac

@ac.struct
class S:
    """A documented payload."""

    lane: ac.u8

@ac.rule
def keep(v):
    return v
'''

SINK_NESTED_CALL = (
    DOCUMENTED_STRUCT
    + """
@ac.system
def top() -> None:
    q = ac.source(S, depth=2, latency=1)
    ac.sink(keep(q))
"""
)

SINK_BOUND_CALL = (
    DOCUMENTED_STRUCT
    + """
@ac.system
def top() -> None:
    q = ac.source(S, depth=2, latency=1)
    r = keep(q)
    ac.sink(r)
"""
)

FOR_OVER_NESTED_CALL = (
    DOCUMENTED_STRUCT
    + """
@ac.system
def top() -> None:
    q = ac.source(S, depth=2, latency=1)
    for item in keep(q):
        ac.sink(item)
"""
)

UNKNOWN_NAME = (
    DOCUMENTED_STRUCT
    + """
@ac.system
def top() -> None:
    q = ac.source(S, depth=2, latency=1)
    ac.sink(missing_name)
"""
)


class StructDocstringTest(unittest.TestCase):
    def test_documented_struct_lowers(self) -> None:
        lowered = lower_queue_source(SINK_BOUND_CALL, "top")
        self.assertIn("ac.struct", lowered)
        self.assertIn("lane", lowered)

    def test_docstring_is_not_reported_as_a_missing_field(self) -> None:
        # The previous rejection claimed the body had no annotated field, while
        # `lane: ac.u8` was present and annotated.
        try:
            lower_queue_source(SINK_BOUND_CALL, "top")
        except QueueFrontendError as error:  # pragma: no cover - regression guard
            self.fail(f"documented struct rejected: {error}")

    def test_non_field_statement_names_the_offending_statement(self) -> None:
        source = DOCUMENTED_STRUCT.replace("    lane: ac.u8", "    lane: ac.u8\n    other = 3")
        with self.assertRaises(QueueFrontendError) as caught:
            lower_queue_source(source, "top")
        self.assertEqual("ACPY-QUEUE-002", caught.exception.code)
        message = str(caught.exception)
        self.assertIn("docstring and annotated fields only", message)
        self.assertIn("other = 3", message)


class QueueReferenceDiagnosticTest(unittest.TestCase):
    def assert_call_reference_rejected(self, source: str, callee: str) -> None:
        with self.assertRaises(QueueFrontendError) as caught:
            lower_queue_source(source, "top")
        self.assertEqual("ACPY-QUEUE-005", caught.exception.code)
        message = str(caught.exception)
        self.assertIn("a call result cannot be used directly", message)
        self.assertIn(f"`result = {callee}(...)`", message)
        # The old text must be gone: it named a collection, which the user never
        # wrote.
        self.assertNotIn("collection reference must be statically resolvable", message)

    def test_statement_handler_path_reports_the_binding_rule(self) -> None:
        self.assert_call_reference_rejected(SINK_NESTED_CALL, "keep")

    def test_parser_closure_path_reports_the_binding_rule(self) -> None:
        # `static_reference` has two implementations (the parser closure and the
        # statement handlers). A `for` iterable goes through the parser one, so
        # this pins both to the same shared diagnostic.
        self.assert_call_reference_rejected(FOR_OVER_NESTED_CALL, "keep")

    def test_unknown_name_keeps_the_precise_reference_message(self) -> None:
        with self.assertRaises(QueueFrontendError) as caught:
            lower_queue_source(UNKNOWN_NAME, "top")
        self.assertEqual("ACPY-QUEUE-005", caught.exception.code)
        message = str(caught.exception)
        self.assertIn("collection reference must be statically resolvable", message)
        self.assertIn("missing_name", message)

    def test_bound_call_result_is_accepted(self) -> None:
        lowered = lower_queue_source(SINK_BOUND_CALL, "top")
        self.assertIn("ac.sink", lowered)


if __name__ == "__main__":
    unittest.main()
