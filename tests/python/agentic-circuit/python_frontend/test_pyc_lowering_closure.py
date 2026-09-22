"""ACIR that the frontend publishes lowers to verified PYC with no residual IR.

The cross-layer verification matrix (issue #128, item V03) requires that each
applicable lowering reaches PYC without leftover ``scf.*`` or ``index``
operations and that the PYC verifier accepts the result. The frontend publishes
frozen ACIR, ``acir-queue-pycgen`` lowers one family case at a time, and ``pycc``
verifies the emitted PYC.

Every design here is checked end to end, and each one also asserts that the PYC
actually carries ``pyc.*`` operations, so an empty or trivially rejected
lowering cannot pass the residual-operation check by producing nothing.
"""

from __future__ import annotations

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
RESIDUAL_TOKENS = ("scf.", "index.")


def _repository_tool(name: str) -> Path | None:
    for candidate in (
        REPOSITORY / ".pycircuit_out/toolchain/build/bin" / name,
        REPOSITORY / ".pycircuit_out/acir/dev-llvm22/bin" / name,
    ):
        if candidate.is_file():
            return candidate
    return None


HELPER_SYSTEM = """
import agentic_circuit as ac

@ac.struct
class Item:
    value: ac.u8

@ac.inline
def pure_inc(value: ac.u8) -> ac.u8:
    return value + 1

@ac.rule
def keep(v):
    return v.with_fields(value=pure_inc(v.value))

@ac.system
def pipeline() -> None:
    incoming = ac.source(Item, depth=2, latency=1)
    outgoing = keep(incoming)
    ac.sink(outgoing)
"""

PURE_MODULE = """
import agentic_circuit as ac

@ac.struct
class Item:
    value: ac.u8

@ac.module_decl(source="generated/module.py")
def stage_decl(value: Item) -> Item:
    ...

stage_decl_ref = stage_decl

@ac.module(declaration=stage_decl_ref)
def stage(value: Item) -> Item:
    return value.with_fields(value=value.value + 1)

@ac.system
def pipeline(incoming: Item) -> Item:
    return stage(incoming)
"""

HIERARCHY_MODULE = """
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

FAMILY_SPECIALIZATION = """
import agentic_circuit as ac

@ac.module_decl(
    source="stage.py",
    parameters=(
        ac.static_parameter("lanes", ac.static_int(width=4, signed=False)),
    ),
    finite_cases=(
        ac.case(("lanes", 2)),
        ac.case(("lanes", 4)),
    ),
)
def stage(
    value: ac.Queue[ac.u8, lanes, 2],
) -> ac.Queue[ac.u8, lanes, 2]:
    ...

stage_decl = stage

@ac.module(declaration=stage_decl)
def stage(
    value: ac.Queue[ac.u8, lanes, 2],
) -> ac.Queue[ac.u8, lanes, 2]:
    return value

@ac.system
def core(
    value: ac.Queue[ac.u8, 2, 2],
) -> ac.Queue[ac.u8, 2, 2]:
    return stage(value, static=ac.case(("lanes", 2)))
"""

NESTED_PROJECTION = """
import agentic_circuit as ac

@ac.struct
class Inner:
    left: ac.u8
    right: ac.u8

@ac.struct
class Envelope:
    inner: Inner
    tag: ac.u4

@ac.system
def pipeline() -> None:
    incoming = ac.source(Envelope, depth=2, latency=1)
    outgoing = incoming.apply(lambda item: item.with_fields(
        inner=item.inner.with_fields(left=item.inner.left + 1),
    ))
    ac.sink(outgoing)
"""


class PycLoweringClosureTest(unittest.TestCase):
    """Item V03: freeze, lower to PYC, and verify the PYC."""

    def _assert_lowers_to_verified_pyc(self, source: str, system: str) -> None:
        acir_opt = _repository_tool("acir-opt")
        pycgen = _repository_tool("acir-queue-pycgen")
        pycc = _repository_tool("pycc")
        if acir_opt is None or pycgen is None or pycc is None:
            self.skipTest("the ACIR code generation tools are not built")

        with tempfile.TemporaryDirectory() as temporary:
            raw = Path(temporary) / "module.raw.mlir"
            raw.write_text(
                lower_queue_source(source, system, source_path="generated/module.py"),
                encoding="utf-8",
            )
            frozen = Path(temporary) / "module.frozen.mlir"
            frozen_result = subprocess.run(
                [
                    str(acir_opt),
                    f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                    str(raw),
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
            self.assertIn("pyc.", generated.stdout)

            for token in RESIDUAL_TOKENS:
                self.assertNotIn(token, generated.stdout)

            pyc = Path(temporary) / "module.pyc"
            pyc.write_text(generated.stdout, encoding="utf-8")
            verified = subprocess.run(
                [str(pycc), str(pyc), "--emit=none"],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verified.returncode, verified.stderr)

    def test_inline_helper_system_lowers_to_verified_pyc(self) -> None:
        self._assert_lowers_to_verified_pyc(HELPER_SYSTEM, "pipeline")

    def test_pure_module_lowers_to_verified_pyc(self) -> None:
        self._assert_lowers_to_verified_pyc(PURE_MODULE, "pipeline")

    def test_hierarchy_module_lowers_to_verified_pyc(self) -> None:
        self._assert_lowers_to_verified_pyc(HIERARCHY_MODULE, "probe")

    def test_family_specialization_lowers_to_verified_pyc(self) -> None:
        self._assert_lowers_to_verified_pyc(FAMILY_SPECIALIZATION, "core")

    def test_nested_projection_lowers_to_verified_pyc(self) -> None:
        self._assert_lowers_to_verified_pyc(NESTED_PROJECTION, "pipeline")


if __name__ == "__main__":
    unittest.main()
