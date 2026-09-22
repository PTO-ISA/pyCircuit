"""An emitted ``ac.module`` family declares the nominals its cases are typed by.

``acir-queue-pycgen`` derives a struct payload's packed layout from
``ModuleFamilyPlan.nominalDefinitions``, which QueueGraph extraction fills from
the family schema's nominal declaration inventory
(``compiler/acir/lib/CodeGen/QueueGraphPlan.cpp``). An empty inventory therefore
fails code generation with ``ACLOWER-PYC: family struct layout definition is
missing`` even though verification and freeze accept the same unit.

The inventory follows the resolved signature of each emitted module, so a
struct contributes its own declaration and, recursively, the declarations of
its field types. Primitive payloads declare nothing, which keeps the emitted
schema minimal.
"""

from __future__ import annotations

import re
import subprocess
import tempfile
import unittest
from pathlib import Path

from agentic_circuit._queue_frontend import lower_queue_source

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


def _module_nominals(lowered: str) -> dict[str, tuple[str, ...]]:
    """Map each emitted module symbol to its nominal declaration inventory."""

    nominals: dict[str, tuple[str, ...]] = {}
    for line in lowered.splitlines():
        match = re.match(r"\s*ac\.module @(\w+) source ", line)
        if match is None:
            continue
        inventory = re.search(r"#ac\.source_owner<[^>]*>, \[([^]]*)\]> \{", line)
        if inventory is None:
            raise AssertionError(line)
        nominals[match.group(1)] = tuple(
            item.strip().removeprefix("@")
            for item in inventory.group(1).split(",")
            if item.strip()
        )
    return nominals


FLAT_STRUCT_MODULE = """
import agentic_circuit as ac

@ac.struct
class In:
    a: ac.u8

@ac.struct
class Out:
    v: ac.u8

@ac.module_decl(source="generated/module.py")
def stage_decl(value: In) -> Out:
    ...

stage_decl_ref = stage_decl

@ac.module(declaration=stage_decl_ref)
def stage(value: In) -> Out:
    return Out(v=value.a)

@ac.system
def probe(value: In) -> Out:
    return stage(value)
"""

NESTED_STRUCT_MODULE = """
import agentic_circuit as ac

@ac.struct
class Inner:
    a: ac.u8

@ac.struct
class Outer:
    inner: Inner
    tag: ac.u8

@ac.module_decl(source="generated/module.py")
def stage_decl(value: Outer) -> Inner:
    ...

stage_decl_ref = stage_decl

@ac.module(declaration=stage_decl_ref)
def stage(value: Outer) -> Inner:
    return value.inner

@ac.system
def probe(value: Outer) -> Inner:
    return stage(value)
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

STRUCT_HIERARCHY = """
import agentic_circuit as ac

@ac.struct
class In:
    a: ac.u8

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


class ModuleNominalInventoryTest(unittest.TestCase):
    def _lower(self, source: str) -> str:
        return lower_queue_source(source, "probe", source_path="generated/module.py")

    def test_struct_payload_families_declare_their_nominals(self) -> None:
        nominals = _module_nominals(self._lower(FLAT_STRUCT_MODULE))

        self.assertEqual({"stage", "Top"}, set(nominals))
        self.assertEqual(("In", "Out"), nominals["stage"])
        self.assertEqual(("In", "Out"), nominals["Top"])

    def test_nested_struct_fields_are_declared_recursively(self) -> None:
        nominals = _module_nominals(self._lower(NESTED_STRUCT_MODULE))

        self.assertEqual(("Outer", "Inner"), nominals["stage"])
        self.assertEqual(("Outer", "Inner"), nominals["Top"])

    def test_primitive_payload_families_declare_no_nominals(self) -> None:
        nominals = _module_nominals(self._lower(PRIMITIVE_HIERARCHY))

        self.assertEqual({"child_a", "child_b", "parent", "Top"}, set(nominals))
        self.assertEqual(
            {
                "child_a": (),
                "child_b": (),
                "parent": (),
                "Top": (),
            },
            nominals,
        )

    def test_struct_module_lowers_to_pyc(self) -> None:
        self._assert_lowers_to_pyc(FLAT_STRUCT_MODULE)

    def test_struct_hierarchy_lowers_to_pyc(self) -> None:
        self._assert_lowers_to_pyc(STRUCT_HIERARCHY)

    def test_nested_struct_module_lowers_to_pyc(self) -> None:
        self._assert_lowers_to_pyc(NESTED_STRUCT_MODULE)

    def _assert_lowers_to_pyc(self, source: str) -> None:
        acir_opt = _repository_tool("acir-opt")
        pycgen = _repository_tool("acir-queue-pycgen")
        pycc = _repository_tool("pycc")
        if acir_opt is None or pycgen is None or pycc is None:
            self.skipTest("the ACIR code generation tools are not built")

        with tempfile.TemporaryDirectory() as temporary:
            module = Path(temporary) / "module.mlir"
            module.write_text(self._lower(source), encoding="utf-8")
            frozen = Path(temporary) / "frozen.mlir"
            frozen_result = subprocess.run(
                [
                    str(acir_opt),
                    f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                    str(module),
                    "-o",
                    str(frozen),
                ],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, frozen_result.returncode, frozen_result.stderr)

            generated = subprocess.run(
                [str(pycgen), str(frozen)], text=True, capture_output=True, check=False
            )
            self.assertEqual(0, generated.returncode, generated.stderr)

            pyc = Path(temporary) / "module.pyc"
            pyc.write_text(generated.stdout, encoding="utf-8")
            compiled = subprocess.run(
                [str(pycc), str(pyc), "--emit=none"],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, compiled.returncode, compiled.stderr)


if __name__ == "__main__":
    unittest.main()
