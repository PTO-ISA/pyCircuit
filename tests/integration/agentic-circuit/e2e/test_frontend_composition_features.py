from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from agentic_circuit._jit import _lower_queue_acir
from agentic_circuit._queue_frontend import QueueFrontendError, lower_queue_source
from agentic_circuit._static_eval import FrozenMap

ROOT = Path(__file__).resolve().parents[4]
ACIR_BIN = Path(os.environ.get("ACIR_BIN", ROOT / ".pycircuit_out/acir/dev-llvm22/bin"))


class FrontendCompositionFeatureTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.acir_opt = ACIR_BIN / "acir-opt"
        cls.cxxgen = ACIR_BIN / "acir-queue-cxxgen"
        cls.plan = ACIR_BIN / "acir-queue-plan"
        cls.pycgen = ACIR_BIN / "acir-queue-pycgen"
        cls.pycc = Path(
            os.environ.get("PYCC", ROOT / ".pycircuit_out/toolchain/build/bin/pycc")
        )
        cls.compiler = shutil.which("c++")
        cls.verilator = shutil.which("verilator")
        if cls.compiler is None or not all(
            path.is_file()
            for path in (cls.acir_opt, cls.cxxgen, cls.plan, cls.pycgen, cls.pycc)
        ):
            raise unittest.SkipTest("integrated ACIR/PYC toolchain is required")

    def _run(self, command: tuple[object, ...], *, cwd: Path) -> str:
        completed = subprocess.run(
            tuple(str(item) for item in command),
            cwd=cwd,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, completed.returncode, completed.stderr)
        return completed.stdout

    def test_dependent_range_metadata_tracks_live_expressions_after_dce(self) -> None:
        duplicate = """import agentic_circuit as ac
N = ac.param[int]("n")
@ac.rule
def decode(raw: ac.u8) -> tuple[ac.index[5], ac.index[5]]:
    first = ac.checked(raw, ac.index[N]).value
    second = ac.checked(raw, ac.index[N]).value
    return first, second
@ac.system
def pipeline(raw: ac.u8, *, n: ac.const[int]) -> tuple[ac.index[5], ac.index[5]]:
    first, second = decode(raw)
    return first, second
"""
        dead = """import agentic_circuit as ac
N = ac.param[int]("n")
@ac.rule
def decode(raw: ac.u8) -> ac.u8:
    unused = ac.wrap(raw, ac.index[N])
    also_unused = unused
    return raw
@ac.system
def pipeline(raw: ac.u8, *, n: ac.const[int]) -> ac.u8:
    result = decode(raw)
    return result
"""
        overwritten = """import agentic_circuit as ac
N = ac.param[int]("n")
@ac.rule
def decode(raw: ac.u8) -> ac.index[5]:
    index = ac.wrap(raw, ac.index[N])
    index = ac.saturate(raw, ac.index[N])
    return index
@ac.system
def pipeline(raw: ac.u8, *, n: ac.const[int]) -> ac.index[5]:
    result = decode(raw)
    return result
"""
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            duplicate_frozen = _lower_queue_acir(
                lower_queue_source(
                    duplicate, "pipeline", static_arguments={"n": 5}
                ),
                optimizer=self.acir_opt,
            )
            self.assertEqual(2, duplicate_frozen.count("ac.static_type_target ="))
            self.assertEqual(
                1, duplicate_frozen.count('target = "expression.decode.0:')
            )
            self.assertEqual(
                1, duplicate_frozen.count('target = "expression.decode.1:')
            )
            duplicate_path = work / "duplicate.mlir"
            duplicate_path.write_text(duplicate_frozen, encoding="utf-8")
            self._run((self.plan, duplicate_path), cwd=ROOT)

            dead_frozen = _lower_queue_acir(
                lower_queue_source(dead, "pipeline", static_arguments={"n": 5}),
                optimizer=self.acir_opt,
            )
            self.assertNotIn("ac.static_type_target", dead_frozen)
            self.assertNotIn("ac.static_type_checks", dead_frozen)
            dead_path = work / "dead.mlir"
            dead_path.write_text(dead_frozen, encoding="utf-8")
            self._run((self.plan, dead_path), cwd=ROOT)

            overwritten_frozen = _lower_queue_acir(
                lower_queue_source(
                    overwritten, "pipeline", static_arguments={"n": 5}
                ),
                optimizer=self.acir_opt,
            )
            self.assertNotIn("ac.var.range_wrap", overwritten_frozen)
            self.assertEqual(
                1, overwritten_frozen.count("ac.static_type_target =")
            )
            overwritten_path = work / "overwritten.mlir"
            overwritten_path.write_text(overwritten_frozen, encoding="utf-8")
            self._run((self.plan, overwritten_path), cwd=ROOT)

    def test_module_state_uses_bounded_storage_initializer_type(self) -> None:
        source = """import agentic_circuit as ac
@ac.module
def store(raw: ac.u8) -> ac.index[5]:
    saved: ac.index[5] = 0
    saved = ac.wrap(raw, ac.index[5])
    return saved
@ac.system
def pipeline(raw: ac.u8) -> ac.index[5]:
    result = store(raw)
    return result
"""
        raw = lower_queue_source(source, "pipeline")
        self.assertIn("init 0 : i3", raw)
        frozen = _lower_queue_acir(raw, optimizer=self.acir_opt)
        self.assertIn("entry !ac.range<0, 4>", frozen)

        unsupported = source.replace("ac.index[5]", "ac.range[4, 9]").replace(
            "saved: ac.range[4, 9] = 0", "saved: ac.range[4, 9] = 4"
        ).replace(
            "ac.wrap(raw, ac.range[4, 9])",
            "ac.saturate(raw, ac.range[4, 9])",
        )
        with self.assertRaisesRegex(
            QueueFrontendError,
            "bounded module state storage requires a zero initializer",
        ):
            lower_queue_source(unsupported, "pipeline")

    def test_nested_bounded_equality_lowers_to_range_leaves(self) -> None:
        source = """import agentic_circuit as ac
@ac.struct
class Item:
    index: ac.index[5]
@ac.rule
def compare(raw: ac.u8) -> bool:
    left = Item(index=ac.wrap(raw, ac.index[5]))
    right = Item(index=ac.literal(4, ac.index[5]))
    return left == right
@ac.system
def pipeline(raw: ac.u8) -> bool:
    result = compare(raw)
    return result
"""
        frozen = _lower_queue_acir(
            lower_queue_source(source, "pipeline"), optimizer=self.acir_opt
        )
        self.assertIn('ac.var.range_cmp "eq"', frozen)
        self.assertNotIn("ac.var.cmp \"eq\"", frozen)
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "range-equality.mlir"
            path.write_text(frozen, encoding="utf-8")
            self._run((self.plan, path), cwd=ROOT)

    def test_examples_freeze_plan_generate_and_compile(self) -> None:
        cases = (
            (
                ROOT / "examples/agentic-circuit/types/parameterized_types.py",
                "parameterized_types",
                {"rob_entries": 128, "issue_width": 4},
            ),
            (
                ROOT / "examples/agentic-circuit/types/scalar_parameterized_types.py",
                "scalar_parameterized_types",
                {"width": 17},
            ),
            (
                ROOT / "examples/agentic-circuit/types/multi_specialization_types.py",
                "multi_specialization_types",
                {},
            ),
            (
                ROOT / "examples/agentic-circuit/types/multi_config_specialization.py",
                "multi_config_specialization",
                {
                    "first": FrozenMap((("entries", 5),)),
                    "second": FrozenMap((("stage", FrozenMap((("entries", 6),))),)),
                },
            ),
            (
                ROOT / "examples/agentic-circuit/types/nested_config_types.py",
                "nested_config_types",
                {
                    "cfg": FrozenMap(
                        (
                            (
                                "cache",
                                FrozenMap(
                                    (
                                        ("line_bytes", 64),
                                        ("sets", 64),
                                        ("ways", 4),
                                    )
                                ),
                            ),
                            ("lanes", 4),
                        )
                    )
                },
            ),
            (
                ROOT / "examples/agentic-circuit/pipelines/record_spread_pipeline.py",
                "record_spread_pipeline",
                {},
            ),
            (
                ROOT / "examples/agentic-circuit/blocks/onehot_encode.py",
                "onehot_encode",
                {},
            ),
            (
                ROOT / "examples/agentic-circuit/pipelines/encoded_enum_pipeline.py",
                "encoded_enum_pipeline",
                {},
            ),
            (
                ROOT / "examples/agentic-circuit/blocks/bounded_integer_operations.py",
                "bounded_integer_operations",
                {},
            ),
        )

        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            for path, system, static_arguments in cases:
                with self.subTest(system=system):
                    raw = lower_queue_source(
                        path.read_text(encoding="utf-8"),
                        system,
                        static_arguments=static_arguments,
                        source_path=path.relative_to(ROOT).as_posix(),
                    )
                    frozen_text = _lower_queue_acir(raw, optimizer=self.acir_opt)
                    frozen = work / f"{system}.mlir"
                    frozen.write_text(frozen_text, encoding="utf-8")
                    generated = self._run((self.cxxgen, frozen), cwd=ROOT)
                    compiled = subprocess.run(
                        (
                            self.compiler,
                            "-std=c++20",
                            "-I",
                            ROOT / "simulator/gfsim/include",
                            "-x",
                            "c++",
                            "-fsyntax-only",
                            "-",
                        ),
                        cwd=ROOT,
                        input=generated,
                        text=True,
                        capture_output=True,
                        check=False,
                    )
                    self.assertEqual(0, compiled.returncode, compiled.stderr)

                    if system in {
                        "multi_specialization_types",
                        "multi_config_specialization",
                    }:
                        self.assertEqual(2, raw.count("ac.struct @Entry__p"))
                        self.assertEqual(2, raw.count("ac.module @stage__p"))
                        plan = json.loads(self._run((self.plan, frozen), cwd=ROOT))
                        self.assertEqual(2, len(plan["module_specializations"]))
                        if system == "multi_config_specialization":
                            self.assertEqual(2, len(plan["static_config_bindings"]))
                            self.assertTrue(
                                all(
                                    instance["source_provenance"]["origins"][0][
                                        "frames"
                                    ][0]["file"].endswith(
                                        "multi_config_specialization.py"
                                    )
                                    for instance in plan["module_instances"]
                                )
                            )
                            roots = [
                                binding["root"]
                                for binding in plan["static_config_bindings"]
                            ]
                            self.assertEqual(
                                sorted(roots),
                                roots,
                            )
                            self.assertTrue(
                                all(
                                    root.startswith("stage__p")
                                    and root.endswith("__cfg")
                                    for root in roots
                                )
                            )
                        # Module-preserving QueueGraph-to-PYC lowering is a
                        # separate accepted limitation; this case proves
                        # specialization identity plus frozen GFSim codegen.
                        continue

                    pyc = work / f"{system}.pyc"
                    pyc.write_text(
                        self._run((self.pycgen, frozen), cwd=ROOT),
                        encoding="utf-8",
                    )
                    self.assertIn("pyc.source_map", pyc.read_text(encoding="utf-8"))
                    cpp_output = work / f"{system}-cpp"
                    verilog_output = work / f"{system}-verilog"
                    self._run(
                        (
                            self.pycc,
                            pyc,
                            "--emit=cpp",
                            "--out-dir",
                            cpp_output,
                            "--hierarchy-policy=strict",
                            "--inline-policy=off",
                        ),
                        cwd=ROOT,
                    )
                    for source in cpp_output.glob("*.cpp"):
                        checked = subprocess.run(
                            (
                                self.compiler,
                                "-std=c++20",
                                "-I",
                                cpp_output,
                                "-I",
                                ROOT / "library",
                                "-fsyntax-only",
                                source,
                            ),
                            cwd=ROOT,
                            text=True,
                            capture_output=True,
                            check=False,
                        )
                        self.assertEqual(0, checked.returncode, checked.stderr)
                    self._run(
                        (
                            self.pycc,
                            pyc,
                            "--emit=verilog",
                            "--out-dir",
                            verilog_output,
                            "--hierarchy-policy=strict",
                            "--inline-policy=off",
                            "--include-primitives",
                        ),
                        cwd=ROOT,
                    )
                    if self.verilator is not None:
                        verilog_sources = tuple(verilog_output.glob("*.v"))
                        linted = subprocess.run(
                            (
                                self.verilator,
                                "--lint-only",
                                "--timing",
                                "-Wno-fatal",
                                *verilog_sources,
                            ),
                            cwd=ROOT,
                            text=True,
                            capture_output=True,
                            check=False,
                        )
                        self.assertEqual(0, linted.returncode, linted.stderr)

                    if system == "parameterized_types":
                        self.assertIn("gfsim::UInt<7> index", generated)
                        self.assertIn("gfsim::UInt<8> count", generated)
                        plan = json.loads(self._run((self.plan, frozen), cwd=ROOT))
                        self.assertEqual(
                            {"ISSUE_WIDTH": 4, "ROB_ENTRIES": 128},
                            plan["static_type_bindings"],
                        )
                        self.assertEqual(3, len(plan["static_type_checks"]))
                        self.assertEqual(2, len(plan["static_type_identities"]))
                    elif system == "scalar_parameterized_types":
                        self.assertIn("gfsim::UInt<17>", generated)
                        plan = json.loads(self._run((self.plan, frozen), cwd=ROOT))
                        self.assertEqual(2, len(plan["static_type_checks"]))
                        self.assertTrue(
                            all(
                                check["type"] == "i17"
                                for check in plan["static_type_checks"]
                            )
                        )
                    elif system == "nested_config_types":
                        plan = json.loads(self._run((self.plan, frozen), cwd=ROOT))
                        self.assertEqual(
                            {
                                "cfg.cache.sets": 64,
                                "cfg.cache.ways": 4,
                                "cfg.lanes": 4,
                            },
                            plan["static_type_bindings"],
                        )
                        self.assertEqual(1, len(plan["static_config_bindings"]))
                        self.assertEqual(
                            "cfg", plan["static_config_bindings"][0]["root"]
                        )
                    elif system == "bounded_integer_operations":
                        plan = json.loads(self._run((self.plan, frozen), cwd=ROOT))
                        kinds = {
                            expression["kind"]
                            for block in plan["blocks"]
                            for expression in block["expressions"]
                        }
                        self.assertTrue(
                            {
                                "range_wrap",
                                "range_saturate",
                                "range_checked_value",
                                "range_checked_valid",
                                "range_add",
                                "array_get_dynamic",
                            }
                            <= kinds
                        )
                        self.assertNotRegex(pyc.read_text(encoding="utf-8"), r"\bscf\.")
                    elif system == "record_spread_pipeline":
                        self.assertIn("struct Packet", generated)
                        self.assertIn("auto v4 = Packet{v0, v1, v2, v3};", generated)
                        plan = json.loads(self._run((self.plan, frozen), cwd=ROOT))
                        record = next(
                            expression
                            for block in plan["blocks"]
                            for expression in block["expressions"]
                            if expression["kind"] == "record_create"
                        )
                        self.assertTrue(
                            record["source_provenance"]["origins"][0]["frames"][0][
                                "file"
                            ].endswith("record_spread_pipeline.py")
                        )
                    elif system == "onehot_encode":
                        self.assertIn("gfsim::priorityEncode", generated)
                        self.assertIn("gfsim::populationCount", generated)
                    elif system == "encoded_enum_pipeline":
                        self.assertIn("WRITE = 9", generated)
                        self.assertIn("MAX = 18446744073709551615", generated)
                        plan = json.loads(self._run((self.plan, frozen), cwd=ROOT))
                        opcode = next(
                            item for item in plan["enums"] if item["name"] == "Opcode"
                        )
                        self.assertEqual(["0x0", "0x3", "0x9"], opcode["values"])
                        self.assertEqual("unsigned_hex", opcode["value_format"])
                        self.assertEqual(4, opcode["width"])
                        wide = next(
                            item
                            for item in plan["enums"]
                            if item["name"] == "WideOpcode"
                        )
                        self.assertEqual(
                            [
                                "0x7fffffffffffffff",
                                "0x8000000000000000",
                                "0xffffffffffffffff",
                            ],
                            wide["values"],
                        )
                        self.assertEqual(64, wide["width"])

    def test_same_dependent_type_crosses_system_module_boundary(self) -> None:
        source = """
from __future__ import annotations
import agentic_circuit as ac

WIDTH = ac.param[int]("width")

@ac.struct
class Entry:
    data: ac.bits[WIDTH]

@ac.rule
def keep(value: Entry) -> Entry:
    return value

@ac.module
def stage(value: Entry, *, width: ac.const[int]) -> Entry:
    result = keep(value)
    return result

@ac.system
def design(value: Entry, *, width: ac.const[int]) -> Entry:
    result = stage(value, width=width)
    return result
"""
        raw = lower_queue_source(source, "design", static_arguments={"width": 4})
        self.assertEqual(
            1,
            len(
                {
                    token
                    for token in raw.replace(">", " ").split()
                    if token.startswith("@Entry__p")
                }
            ),
        )
        with tempfile.TemporaryDirectory() as temporary:
            frozen = Path(temporary) / "same-specialization.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt), encoding="utf-8"
            )
            plan = json.loads(self._run((self.plan, frozen), cwd=ROOT))
        self.assertEqual(1, len(plan["static_type_identities"]))

    def test_same_dependent_scalar_crosses_system_module_boundary(self) -> None:
        source = """
from __future__ import annotations
import agentic_circuit as ac

WIDTH = ac.param[int]("width")

@ac.rule
def keep(value):
    return value

@ac.module
def stage(value: ac.bits[WIDTH], *, width: ac.const[int]) -> ac.bits[WIDTH]:
    result = keep(value)
    return result

@ac.system
def design(
    value: ac.bits[WIDTH], *, width: ac.const[int]
) -> ac.bits[WIDTH]:
    result = stage(value, width=width)
    return result
"""
        raw = lower_queue_source(
            source, "design", static_arguments={"width": 4}, host_results=True
        )
        with tempfile.TemporaryDirectory() as temporary:
            frozen = Path(temporary) / "same-scalar-specialization.mlir"
            frozen.write_text(
                _lower_queue_acir(raw, optimizer=self.acir_opt), encoding="utf-8"
            )
            plan = json.loads(self._run((self.plan, frozen), cwd=ROOT))
        self.assertEqual(4, len(plan["static_type_checks"]))


if __name__ == "__main__":
    unittest.main()
