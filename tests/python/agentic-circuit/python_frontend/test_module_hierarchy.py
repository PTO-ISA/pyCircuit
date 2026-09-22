"""Hierarchy modules compose child ``@ac.module`` instances.

Issue #197 recorded that ``@ac.module`` supported only a single-input/single-result
slice or a rule-calling leaf, so an H1 -> H2 -> H3 hierarchy had to be flattened
into the top-level ``@ac.system``. The typed family/case model admits a composite
body that binds named intermediates from child instances, takes N runtime inputs,
and reads keyword-only ``ac.const`` geometry.

These tests pin the shapes that lower, the structural relation between a
hierarchy module and the equivalent flat composition, and the two rejections that
remain by design.
"""

from __future__ import annotations

import re
import unittest

from agentic_circuit._queue_frontend import QueueFrontendError, lower_queue_source

PRELUDE = """
import agentic_circuit as ac

@ac.struct
class In:
    a: ac.u8
    b: ac.u8

@ac.struct
class Mid:
    v: ac.u8

@ac.struct
class Out:
    v: ac.u8

@ac.rule
def form_mid(x: In) -> Mid:
    return Mid(v=x.a)

@ac.rule
def form_out(m: Mid) -> Out:
    return Out(v=m.v)

@ac.module_decl(source="generated/module.py")
def child_a_decl(x: In) -> Mid:
    ...

child_a_decl_ref = child_a_decl

@ac.module(declaration=child_a_decl_ref)
def child_a(x: In) -> Mid:
    mid = form_mid(x)
    return mid

@ac.module_decl(source="generated/module.py")
def child_b_decl(m: Mid) -> Out:
    ...

child_b_decl_ref = child_b_decl

@ac.module(declaration=child_b_decl_ref)
def child_b(m: Mid) -> Out:
    out = form_out(m)
    return out
"""

FLAT_COMPOSITION = """
@ac.system
def probe(x: In) -> Out:
    mid = child_a(x)
    out = child_b(mid)
    return out
"""

# Rule-body operations. A hierarchy module adds a definition boundary, so the
# two lowerings are not byte-identical, but the rule graph they describe is.
_RULE_BODY_OPS = (
    "ac.rule ",
    "ac.rule.return",
    "ac.var.get",
    "ac.var.record",
    "ac.interface_port",
)


def _op_counts(lowered: str) -> dict[str, int]:
    counts: dict[str, int] = {}
    for token in _RULE_BODY_OPS:
        counts[token] = lowered.count(token)
    return counts


def _lower(body: str, system: str = "probe") -> str:
    return lower_queue_source(PRELUDE + body, system, source_path="generated/module.py")


def _lower_or_error(body: str, system: str = "probe") -> tuple[str | None, str]:
    try:
        return _lower(body, system), ""
    except QueueFrontendError as error:
        return None, str(error)


class HierarchyShapeTest(unittest.TestCase):
    def test_named_intermediates_between_two_children(self) -> None:
        lowered = _lower(
            """
@ac.module_decl(source="generated/module.py")
def parent_decl(x: In) -> Out:
    ...

parent_decl_ref = parent_decl

@ac.module(declaration=parent_decl_ref)
def parent(x: In) -> Out:
    mid = child_a(x)
    out = child_b(mid)
    return out

@ac.system
def probe(x: In) -> Out:
    return parent(x)
"""
        )
        self.assertIn("ac.module @parent", lowered)
        # parent, child_a, and child_b are each placed once.
        self.assertEqual(3, lowered.count("ac.instance"))

    def test_two_runtime_inputs_are_admitted_when_consumed(self) -> None:
        lowered = _lower(
            """
@ac.rule
def pick_mid(x: In, y: Mid) -> Mid:
    return y

@ac.module_decl(source="generated/module.py")
def parent_decl(x: In, y: Mid) -> Out:
    ...

parent_decl_ref = parent_decl

@ac.module(declaration=parent_decl_ref)
def parent(x: In, y: Mid) -> Out:
    mid = child_a(x)
    other = pick_mid(x, y)
    out = child_b(mid)
    return out

@ac.system
def probe(x: In, y: Mid) -> Out:
    return parent(x, y)
"""
        )
        self.assertIn("ac.module @parent", lowered)

    def test_static_geometry_is_read_by_the_hierarchy_module(self) -> None:
        lowered = _lower(
            """
@ac.module_decl(source="generated/module.py")
def parent_decl(x: In, *, width: ac.const[int] = 4) -> Out:
    ...

parent_decl_ref = parent_decl

@ac.module(declaration=parent_decl_ref)
def parent(x: In, *, width: ac.const[int] = 4) -> Out:
    ac.static_assert(1 <= width <= 8, "width must be in 1..8")
    mid = child_a(x)
    out = child_b(mid)
    return out

@ac.system
def probe(x: In) -> Out:
    return parent(x, width=4)
"""
        )
        self.assertIn("ac.module @parent", lowered)

    def test_single_child_forwarding(self) -> None:
        lowered = _lower(
            """
@ac.module_decl(source="generated/module.py")
def parent_decl(x: In) -> Mid:
    ...

parent_decl_ref = parent_decl

@ac.module(declaration=parent_decl_ref)
def parent(x: In) -> Mid:
    mid = child_a(x)
    return mid

@ac.system
def probe(x: In) -> Mid:
    return parent(x)
"""
        )
        self.assertIn("ac.module @parent", lowered)

    def test_one_intermediate_feeds_two_children(self) -> None:
        lowered = _lower(
            """
@ac.module_decl(source="generated/module.py")
def parent_decl(x: In) -> tuple[Out, Out]:
    ...

parent_decl_ref = parent_decl

@ac.module(declaration=parent_decl_ref)
def parent(x: In) -> tuple[Out, Out]:
    mid = child_a(x)
    left = child_b(mid)
    right = child_b(mid)
    return left, right

@ac.system
def probe(x: In) -> tuple[Out, Out]:
    return parent(x)
"""
        )
        # parent, child_a, and two child_b placements, with a broadcast on the
        # shared intermediate edge.
        self.assertEqual(4, lowered.count("ac.instance"))
        self.assertIn("ac.broadcast", lowered)

    def test_two_level_hierarchy_nests(self) -> None:
        lowered = _lower(
            """
@ac.module_decl(source="generated/module.py")
def parent_decl(x: In) -> Out:
    ...

parent_decl_ref = parent_decl

@ac.module(declaration=parent_decl_ref)
def parent(x: In) -> Out:
    mid = child_a(x)
    out = child_b(mid)
    return out

@ac.module_decl(source="generated/module.py")
def grand_decl(x: In) -> Out:
    ...

grand_decl_ref = grand_decl

@ac.module(declaration=grand_decl_ref)
def grand(x: In) -> Out:
    out = parent(x)
    return out

@ac.system
def probe(x: In) -> Out:
    return grand(x)
"""
        )
        self.assertIn("ac.module @grand", lowered)
        self.assertIn("ac.module @parent", lowered)

    def test_single_child_chain_lowers_four_levels_deep(self) -> None:
        text = ""
        previous = "child_a"
        for level in range(1, 5):
            text += f"""
@ac.module_decl(source="generated/module.py")
def level{level}_decl(x: In) -> Mid:
    ...

level{level}_decl_ref = level{level}_decl

@ac.module(declaration=level{level}_decl_ref)
def level{level}(x: In) -> Mid:
    mid = {previous}(x)
    return mid
"""
            previous = f"level{level}"
        text += f"""
@ac.system
def probe(x: In) -> Mid:
    out = {previous}(x)
    return out
"""
        lowered = _lower(text)
        for level in range(1, 5):
            self.assertIn(f"ac.module @level{level}", lowered)


class HierarchyEquivalenceTest(unittest.TestCase):
    def test_hierarchy_describes_the_same_rule_graph_as_the_flat_system(self) -> None:
        """A1: the hierarchy lowering adds a boundary, not a different graph."""

        flat = _lower(FLAT_COMPOSITION)
        hierarchy = _lower(
            """
@ac.module_decl(source="generated/module.py")
def parent_decl(x: In) -> Out:
    ...

parent_decl_ref = parent_decl

@ac.module(declaration=parent_decl_ref)
def parent(x: In) -> Out:
    mid = child_a(x)
    out = child_b(mid)
    return out

@ac.system
def probe(x: In) -> Out:
    return parent(x)
"""
        )
        self.assertEqual(_op_counts(flat), _op_counts(hierarchy))
        # The only structural addition is the hierarchy module and its placement.
        self.assertEqual(2, flat.count("ac.instance"))
        self.assertEqual(3, hierarchy.count("ac.instance"))
        self.assertEqual(
            flat.count("ac.module @") + 1, hierarchy.count("ac.module @")
        )
        self.assertEqual(
            flat.count("ac.module.case") + 1, hierarchy.count("ac.module.case")
        )


class HierarchyRejectionTest(unittest.TestCase):
    def test_inline_nested_child_call_reports_the_binding_rule(self) -> None:
        """R2: inline nesting stays rejected, but the diagnostic says why."""

        lowered, message = _lower_or_error(
            """
@ac.module_decl(source="generated/module.py")
def parent_decl(x: In) -> Out:
    ...

parent_decl_ref = parent_decl

@ac.module(declaration=parent_decl_ref)
def parent(x: In) -> Out:
    return child_b(child_a(x))

@ac.system
def probe(x: In) -> Out:
    return parent(x)
"""
        )
        self.assertIsNone(lowered)
        self.assertIn("ACPY-MODULE-010", message)
        self.assertIn("composite child inputs must be named", message)

    def test_unconsumed_composite_input_is_rejected(self) -> None:
        lowered, message = _lower_or_error(
            """
@ac.module_decl(source="generated/module.py")
def parent_decl(x: In, y: Mid) -> Out:
    ...

parent_decl_ref = parent_decl

@ac.module(declaration=parent_decl_ref)
def parent(x: In, y: Mid) -> Out:
    mid = child_a(x)
    out = child_b(mid)
    return out

@ac.system
def probe(x: In, y: Mid) -> Out:
    return parent(x, y)
"""
        )
        self.assertIsNone(lowered)
        self.assertIn("ACPY-MODULE-010", message)
        self.assertIn("requires a consumer: 'y'", message)


if __name__ == "__main__":
    unittest.main()
