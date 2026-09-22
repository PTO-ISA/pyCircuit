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
import subprocess
import tempfile
import unittest
from pathlib import Path

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


REPOSITORY = Path(__file__).resolve().parents[4]
RULE_LOWERING_PIPELINE = (
    "builtin.module("
    "ac-lower-rules,"
    "ac-inline-pure-helpers,"
    "canonicalize,cse,"
    "ac-verify-rule-closure,"
    "ac-freeze-topology)"
)


def _repository_tool(name: str) -> Path | None:
    for candidate in (
        REPOSITORY / ".pycircuit_out/toolchain/build/bin" / name,
        REPOSITORY / ".pycircuit_out/acir/dev-llvm22/bin" / name,
    ):
        if candidate.is_file():
            return candidate
    return None


def _freeze(
    acir_opt: Path, lowered: str, output: Path | None = None
) -> subprocess.CompletedProcess[str]:
    with tempfile.TemporaryDirectory() as temporary:
        source = Path(temporary) / "module.mlir"
        source.write_text(lowered, encoding="utf-8")
        frozen = output if output is not None else Path(temporary) / "frozen.mlir"
        return subprocess.run(
            [
                str(acir_opt),
                f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                str(source),
                "-o",
                str(frozen),
            ],
            text=True,
            capture_output=True,
            check=False,
        )


def _module_ports(lowered: str) -> dict[str, tuple[list[str], list[str]]]:
    """Map each emitted module symbol to its (input, output) interface ports."""

    ports: dict[str, tuple[list[str], list[str]]] = {}
    for line in lowered.splitlines():
        match = re.match(r"\s*ac\.module @(\w+) source ", line)
        if match is None:
            continue
        interface = re.search(
            r"#ac\.module_interface<\[(.*?)\]>, #ac\.source_owner<", line
        )
        if interface is None:
            raise AssertionError(line)
        entries = re.findall(
            r'#ac\.interface_port<"([^"]+)", "(input|output)"', interface.group(1)
        )
        inputs = [name for name, direction in entries if direction == "input"]
        outputs = [name for name, direction in entries if direction == "output"]
        ports[match.group(1)] = (inputs, outputs)
    return ports


def _module_case_arity(lowered: str) -> dict[str, tuple[int, int]]:
    """Map each emitted module symbol to its case (input, result) arity."""

    arity: dict[str, tuple[int, int]] = {}
    symbol = None
    for line in lowered.splitlines():
        match = re.match(r"\s*ac\.module @(\w+) source ", line)
        if match is not None:
            symbol = match.group(1)
            continue
        case = re.search(
            r"ac\.module\.case arguments .*?type \((.*?)\) -> (.*?) \{", line
        )
        if case is None or symbol is None:
            continue
        inputs = [item for item in case.group(1).split(",") if item.strip()]
        results = [item for item in case.group(2).split(",") if item.strip()]
        arity[symbol] = (len(inputs), len(results))
        symbol = None
    return arity


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
        flat_counts = _op_counts(flat)
        hierarchy_counts = _op_counts(hierarchy)
        # The added boundary also declares the hierarchy module's own two
        # interface ports (one input, one result); the rest is shared.
        added_ports = hierarchy_counts.pop("ac.interface_port") - flat_counts.pop(
            "ac.interface_port"
        )
        self.assertEqual(2, added_ports)
        self.assertEqual(flat_counts, hierarchy_counts)
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


class FamilyInterfaceMaterializationTest(unittest.TestCase):
    """Every emitted ``ac.module`` declares a materializable interface.

    ``ModuleCaseOp`` verification materializes the family interface against the
    concrete case signature, so each ``ac.module`` has to declare exactly one
    port per case input and per case result. A hierarchy module is emitted for
    one resolved signature, so an empty interface makes the whole lowering
    unverifiable even though the frontend accepts the source.
    """

    HIERARCHY = """
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

    def test_every_module_interface_matches_its_case_arity(self) -> None:
        lowered = _lower(self.HIERARCHY)

        ports = _module_ports(lowered)
        arity = _module_case_arity(lowered)

        self.assertEqual({"parent", "child_a", "child_b", "Top"}, set(ports))
        self.assertEqual(set(ports), set(arity))
        for symbol, (inputs, outputs) in ports.items():
            self.assertEqual(
                arity[symbol],
                (len(inputs), len(outputs)),
                f"{symbol} declares {len(inputs)} input and {len(outputs)} "
                f"output ports for case arity {arity[symbol]}",
            )
            self.assertEqual(len(inputs) + len(outputs), len(set(inputs + outputs)))

    def test_every_module_interface_matches_its_arity_in_the_flat_system(self) -> None:
        """The same invariant holds without a hierarchy boundary."""

        lowered = _lower(FLAT_COMPOSITION)

        ports = _module_ports(lowered)
        arity = _module_case_arity(lowered)

        self.assertEqual(set(ports), set(arity))
        for symbol, (inputs, outputs) in ports.items():
            self.assertEqual(arity[symbol], (len(inputs), len(outputs)), symbol)

    def test_hierarchy_lowering_freezes_in_the_rule_lowering_pipeline(self) -> None:
        acir_opt = _repository_tool("acir-opt")
        if acir_opt is None:
            self.skipTest("acir-opt is not built")

        lowered = _lower(self.HIERARCHY)

        completed = _freeze(acir_opt, lowered)

        self.assertEqual(0, completed.returncode, completed.stderr)

    def test_flat_composition_without_hierarchy_still_freezes(self) -> None:
        acir_opt = _repository_tool("acir-opt")
        if acir_opt is None:
            self.skipTest("acir-opt is not built")

        completed = _freeze(acir_opt, _lower(FLAT_COMPOSITION))

        self.assertEqual(0, completed.returncode, completed.stderr)


PURE_MODULE = """
@ac.module_decl(source="generated/module.py")
def stage_decl(x: In) -> Out:
    ...

stage_decl_ref = stage_decl

@ac.module(declaration=stage_decl_ref)
def stage(x: In) -> Out:
    return Out(v=x.a)

@ac.system
def probe(x: In) -> Out:
    return stage(x)
"""

DECLARED_INTERFACE = """
@ac.module_decl(source="generated/module.py")
def parent(declared_input: In) -> Out:
    ...

parent_ref = parent

@ac.module(declaration=parent_ref)
def parent(runtime_input: In) -> Out:
    mid = child_a(runtime_input)
    out = child_b(mid)
    return out

@ac.system
def probe(x: In) -> Out:
    return parent(x)
"""

PRIMITIVE_HIERARCHY = """
import agentic_circuit as ac

@ac.module_decl(source="generated/module.py")
def child_a_decl(x: ac.u8) -> ac.u8:
    ...

child_a_decl_ref = child_a_decl

@ac.module(declaration=child_a_decl_ref)
def child_a(x: ac.u8) -> ac.u8:
    return x + 1

@ac.module_decl(source="generated/module.py")
def child_b_decl(x: ac.u8) -> ac.u8:
    ...

child_b_decl_ref = child_b_decl

@ac.module(declaration=child_b_decl_ref)
def child_b(x: ac.u8) -> ac.u8:
    return x + 2

@ac.module_decl(source="generated/module.py")
def parent_decl(x: ac.u8) -> ac.u8:
    ...

parent_decl_ref = parent_decl

@ac.module(declaration=parent_decl_ref)
def parent(x: ac.u8) -> ac.u8:
    mid = child_a(x)
    out = child_b(mid)
    return out

@ac.system
def probe(x: ac.u8) -> ac.u8:
    return parent(x)
"""


class FamilyInterfaceFallbackTest(unittest.TestCase):
    """The fallback covers every module shape, not only hierarchy modules.

    A module whose implementation symbol differs from its declaration name has
    no registry entry, so the interface has to come from the resolved
    signature. A module whose declaration name matches its symbol keeps the
    declared interface, which is the dependent skeleton for parameterized
    families.
    """

    def _assert_interfaces_match(self, lowered: str) -> None:
        ports = _module_ports(lowered)
        arity = _module_case_arity(lowered)

        self.assertEqual(set(ports), set(arity))
        for symbol, (inputs, outputs) in ports.items():
            self.assertEqual(arity[symbol], (len(inputs), len(outputs)), symbol)

    def test_pure_expression_module_materializes_its_interface(self) -> None:
        lowered = _lower(PURE_MODULE)

        self._assert_interfaces_match(lowered)
        inputs, outputs = _module_ports(lowered)["stage"]
        self.assertEqual((1, 1), (len(inputs), len(outputs)))

    def test_pure_expression_module_freezes_in_the_rule_lowering_pipeline(self) -> None:
        acir_opt = _repository_tool("acir-opt")
        if acir_opt is None:
            self.skipTest("acir-opt is not built")

        lowered = _lower(PURE_MODULE)

        completed = _freeze(acir_opt, lowered)

        self.assertEqual(0, completed.returncode, completed.stderr)

    def test_primitive_hierarchy_freezes_in_the_rule_lowering_pipeline(self) -> None:
        acir_opt = _repository_tool("acir-opt")
        if acir_opt is None:
            self.skipTest("acir-opt is not built")

        lowered = lower_queue_source(
            PRIMITIVE_HIERARCHY, "probe", source_path="generated/module.py"
        )

        completed = _freeze(acir_opt, lowered)

        self.assertEqual(0, completed.returncode, completed.stderr)

    def test_primitive_hierarchy_lowers_to_pyc(self) -> None:
        acir_opt = _repository_tool("acir-opt")
        pycgen = _repository_tool("acir-queue-pycgen")
        pycc = _repository_tool("pycc")
        if acir_opt is None or pycgen is None or pycc is None:
            self.skipTest("the ACIR code generation tools are not built")

        lowered = lower_queue_source(
            PRIMITIVE_HIERARCHY, "probe", source_path="generated/module.py"
        )
        with tempfile.TemporaryDirectory() as temporary:
            frozen = Path(temporary) / "frozen.mlir"
            completed = _freeze(acir_opt, lowered, frozen)
            self.assertEqual(0, completed.returncode, completed.stderr)

            generated = subprocess.run(
                [str(pycgen), str(frozen)], text=True, capture_output=True, check=False
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            self.assertIn("ac.module @parent", lowered)

            pyc = Path(temporary) / "hierarchy.pyc"
            pyc.write_text(generated.stdout, encoding="utf-8")
            compiled = subprocess.run(
                [str(pycc), str(pyc), "--emit=none"],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, compiled.returncode, compiled.stderr)

    def test_matching_declaration_keeps_its_declared_interface(self) -> None:
        lowered = _lower(DECLARED_INTERFACE)

        parent = _module_ports(lowered)["parent"]

        self.assertEqual((["declared_input"], ["result"]), parent)
        # The declared skeleton keeps its dependent queue encoding instead of
        # being replaced by the fallback for the resolved signature.
        self.assertIn(
            '#ac.interface_port<"declared_input", "input", '
            "#ac.type_expr<#ac.type_expr_queue<",
            lowered,
        )

    def test_matching_declaration_still_freezes(self) -> None:
        acir_opt = _repository_tool("acir-opt")
        if acir_opt is None:
            self.skipTest("acir-opt is not built")

        lowered = _lower(DECLARED_INTERFACE)

        completed = _freeze(acir_opt, lowered)

        self.assertEqual(0, completed.returncode, completed.stderr)


if __name__ == "__main__":
    unittest.main()
