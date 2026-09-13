from __future__ import annotations

import importlib.util
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class JitSourceClosureTest(unittest.TestCase):
    def test_module_qualified_local_import_is_rejected_during_capture(self) -> None:
        from agentic_circuit._source_closure import (
            SourceClosureError,
            capture_source_closure,
        )

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "contracts.py").write_text("VALUE = 1\n", encoding="utf-8")
            entry = root / "top.py"
            entry.write_text("import contracts\n", encoding="utf-8")

            with self.assertRaisesRegex(
                SourceClosureError, "module-qualified local import"
            ):
                capture_source_closure(entry, root)

    def test_renamed_local_import_is_rejected_during_capture(self) -> None:
        from agentic_circuit._source_closure import (
            SourceClosureError,
            capture_source_closure,
        )

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "contracts.py").write_text("VALUE = 1\n", encoding="utf-8")
            entry = root / "top.py"
            entry.write_text(
                "from contracts import VALUE as RENAMED\n", encoding="utf-8"
            )

            with self.assertRaisesRegex(SourceClosureError, "renamed local import"):
                capture_source_closure(entry, root)

    def test_distinct_flattened_definitions_with_one_name_are_rejected(self) -> None:
        from agentic_circuit._source_closure import (
            SourceClosureError,
            capture_source_closure,
        )

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "left.py").write_text(
                "def helper():\n    return 1\n\nLEFT = helper()\n", encoding="utf-8"
            )
            (root / "right.py").write_text(
                "def helper():\n    return 2\n\nRIGHT = helper()\n", encoding="utf-8"
            )
            entry = root / "top.py"
            entry.write_text(
                "from left import LEFT\nfrom right import RIGHT\n", encoding="utf-8"
            )

            with self.assertRaisesRegex(
                SourceClosureError, "symbol 'helper'"
            ) as caught:
                capture_source_closure(entry, root)
            self.assertEqual("ACPY-JIT-006", caught.exception.code)
            self.assertIn("left.py", str(caught.exception))
            self.assertIn("right.py", str(caught.exception))

    def test_one_imported_definition_can_be_reused_by_multiple_modules(self) -> None:
        from agentic_circuit._source_closure import capture_source_closure

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "common.py").write_text("COMMON = 4\n", encoding="utf-8")
            (root / "left.py").write_text(
                "from common import COMMON\nLEFT = COMMON\n", encoding="utf-8"
            )
            (root / "right.py").write_text(
                "from common import COMMON\nRIGHT = COMMON\n", encoding="utf-8"
            )
            entry = root / "top.py"
            entry.write_text(
                "from left import LEFT\nfrom right import RIGHT\n", encoding="utf-8"
            )

            closure = capture_source_closure(entry, root)

        self.assertEqual(
            ("common.py", "left.py", "right.py", "top.py"),
            tuple(item.path for item in closure.entries),
        )

    def test_explicit_symbol_import_captures_payload_invariant_definition(self) -> None:
        from agentic_circuit._source_closure import capture_source_closure

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "contracts.py").write_text(
                "import agentic_circuit as ac\n\n"
                "@ac.struct\n"
                "class Payload:\n"
                "    value: ac.u8\n\n"
                "@ac.invariant\n"
                "def valid_payload(value: Payload) -> bool:\n"
                "    return value.value != 0\n",
                encoding="utf-8",
            )
            entry = root / "top.py"
            entry.write_text(
                "from contracts import Payload, valid_payload\n",
                encoding="utf-8",
            )

            closure = capture_source_closure(entry, root)

        self.assertEqual(
            ("contracts.py", "top.py"),
            tuple(item.path for item in closure.entries),
        )

    def test_imported_invariant_caller_and_callee_lower_from_source_closure(
        self,
    ) -> None:
        import agentic_circuit as ac

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "opt01_contracts.py").write_text(
                "import agentic_circuit as ac\n\n"
                "@ac.struct\n"
                "class Inner:\n"
                "    tag: ac.u8\n"
                "    valid: bool\n\n"
                "@ac.struct\n"
                "class Payload:\n"
                "    inner: Inner\n"
                "    valid: bool\n",
                encoding="utf-8",
            )
            (root / "opt01_z_callee.py").write_text(
                "import agentic_circuit as ac\n"
                "from opt01_contracts import Inner\n\n"
                "@ac.invariant\n"
                "def valid_inner(value: Inner) -> bool:\n"
                "    return value.valid and value.tag != 0\n",
                encoding="utf-8",
            )
            (root / "opt01_a_caller.py").write_text(
                "import agentic_circuit as ac\n"
                "from opt01_contracts import Payload\n"
                "from opt01_z_callee import valid_inner\n\n"
                "@ac.invariant\n"
                "def valid_payload(value: Payload) -> bool:\n"
                "    return valid_inner(value.inner) and value.valid\n",
                encoding="utf-8",
            )
            top = root / "opt01_top.py"
            top.write_text(
                "import agentic_circuit as ac\n"
                "from opt01_contracts import Payload\n"
                "from opt01_a_caller import valid_payload\n\n"
                "@ac.module\n"
                "def validate(value: Payload) -> Payload:\n"
                "    return value.with_fields(valid=valid_payload(value))\n\n"
                "@ac.system\n"
                "def composed(value: Payload) -> Payload:\n"
                "    return validate(value)\n",
                encoding="utf-8",
            )

            sys.path.insert(0, str(root))
            self.addCleanup(sys.path.remove, str(root))
            module_names = (
                "opt01_contracts",
                "opt01_z_callee",
                "opt01_a_caller",
                "opt01_top",
            )
            for name in module_names:
                sys.modules.pop(name, None)
                self.addCleanup(sys.modules.pop, name, None)
            spec = importlib.util.spec_from_file_location("opt01_top", top)
            if spec is None or spec.loader is None:
                raise RuntimeError("cannot load composed invariant fixture")
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            spec.loader.exec_module(module)

            specialization = ac.jit(module.composed, workspace=root)
            lowered = specialization.lower_acir()

        self.assertEqual(
            (
                "opt01_a_caller.py",
                "opt01_contracts.py",
                "opt01_top.py",
                "opt01_z_callee.py",
            ),
            tuple(item.path for item in specialization.sources),
        )
        self.assertIn('name "Payload.valid_payload"', lowered)
        self.assertIn('name "Inner.valid_inner"', lowered)
        self.assertIn("%invariant0_invariant1_value", lowered)

    def test_imported_dependent_struct_types_resolve_from_jit_constants(self) -> None:
        import agentic_circuit as ac

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "param_contracts.py").write_text(
                "from __future__ import annotations\n"
                "import agentic_circuit as ac\n\n"
                'ENTRIES = ac.param[int]("entries")\n'
                'LANES = ac.param[int]("lanes")\n\n'
                "@ac.struct\n"
                "class Entry:\n"
                "    index: ac.bits[ac.index_width(ENTRIES)]\n"
                "    valid: bool\n\n"
                "@ac.struct\n"
                "class Group:\n"
                "    entries: ac.array[LANES, Entry]\n"
                "    count: ac.bits[ac.count_width(ENTRIES)]\n",
                encoding="utf-8",
            )
            top = root / "param_top.py"
            top.write_text(
                "from __future__ import annotations\n"
                "import agentic_circuit as ac\n"
                "from param_contracts import Group\n\n"
                "@ac.rule\n"
                "def keep(value: Group) -> Group:\n"
                "    return value\n\n"
                "@ac.system\n"
                "def parameterized(value: Group, *, entries: ac.const[int], "
                "lanes: ac.const[int]) -> Group:\n"
                "    result = keep(value)\n"
                "    return result\n",
                encoding="utf-8",
            )

            sys.path.insert(0, str(root))
            self.addCleanup(sys.path.remove, str(root))
            for name in ("param_contracts", "param_top"):
                sys.modules.pop(name, None)
                self.addCleanup(sys.modules.pop, name, None)
            spec = importlib.util.spec_from_file_location("param_top", top)
            if spec is None or spec.loader is None:
                raise RuntimeError("cannot load dependent-type fixture")
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            spec.loader.exec_module(module)

            specialization = ac.jit(
                module.parameterized,
                workspace=root,
                entries=128,
                lanes=4,
            )
            lowered = specialization.lower_acir()

        self.assertIn('{name = "index", type = i7}', lowered)
        self.assertIn('{name = "count", type = i8}', lowered)
        self.assertRegex(
            lowered,
            r"!ac\.value_array<4 x !ac\.struct<@types::@Entry__p[0-9a-f]{12}>>",
        )

    def test_imported_rule_preserves_its_original_source_location(self) -> None:
        import agentic_circuit as ac

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "location_contracts.py").write_text(
                "import agentic_circuit as ac\n\n"
                "@ac.struct\n"
                "class Payload:\n"
                "    value: ac.u8\n",
                encoding="utf-8",
            )
            (root / "location_rules.py").write_text(
                "import agentic_circuit as ac\n"
                "from location_contracts import Payload\n\n"
                "# Keep padding so the definition line is observable.\n"
                "# second padding line\n"
                "@ac.rule\n"
                "def increment(value: Payload) -> Payload:\n"
                "    return value.with_fields(value=value.value + 1)\n",
                encoding="utf-8",
            )
            top = root / "location_top.py"
            top.write_text(
                "import agentic_circuit as ac\n"
                "from location_contracts import Payload\n"
                "from location_rules import increment\n\n"
                "@ac.module\n"
                "def stage(value: Payload) -> Payload:\n"
                "    result = increment(value)\n"
                "    return result\n\n"
                "@ac.system\n"
                "def located(value: Payload) -> Payload:\n"
                "    result = stage(value)\n"
                "    return result\n",
                encoding="utf-8",
            )

            sys.path.insert(0, str(root))
            self.addCleanup(sys.path.remove, str(root))
            for name in ("location_contracts", "location_rules", "location_top"):
                sys.modules.pop(name, None)
                self.addCleanup(sys.modules.pop, name, None)
            spec = importlib.util.spec_from_file_location("location_top", top)
            if spec is None or spec.loader is None:
                raise RuntimeError("cannot load source-location fixture")
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            spec.loader.exec_module(module)

            lowered = ac.jit(module.located, workspace=root).lower_acir()

            self.assertIn(
                'loc(callsite("location_rules.py":7:1 at "location_top.py":7:14))',
                lowered,
            )
        self.assertNotIn('loc("location_top.py":7:1)', lowered)

    def test_imported_module_static_assert_preserves_its_original_location(
        self,
    ) -> None:
        import agentic_circuit as ac

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "assert_contracts.py").write_text(
                "import agentic_circuit as ac\n\n"
                "@ac.struct\n"
                "class Payload:\n"
                "    value: ac.u8\n",
                encoding="utf-8",
            )
            (root / "assert_stage.py").write_text(
                "import agentic_circuit as ac\n"
                "from assert_contracts import Payload\n\n"
                "@ac.rule\n"
                "def keep(value: Payload) -> Payload:\n"
                "    return value\n\n"
                "@ac.module\n"
                "def checked(\n"
                "    value: Payload,\n"
                "    *,\n"
                "    entries: ac.const[int],\n"
                ") -> Payload:\n"
                "    # This comment and blank line disappear under ast.unparse.\n"
                "\n"
                '    ac.static_assert(entries > 0, message="entries must be positive")\n'
                "    result = keep(value)\n"
                "    return result\n",
                encoding="utf-8",
            )
            top = root / "assert_top.py"
            top.write_text(
                "import agentic_circuit as ac\n"
                "from assert_contracts import Payload\n"
                "from assert_stage import checked\n\n"
                "@ac.system\n"
                "def configured(value: Payload, *, entries: ac.const[int]) -> Payload:\n"
                "    result = checked(value, entries=entries)\n"
                "    return result\n",
                encoding="utf-8",
            )

            sys.path.insert(0, str(root))
            self.addCleanup(sys.path.remove, str(root))
            for name in ("assert_contracts", "assert_stage", "assert_top"):
                sys.modules.pop(name, None)
                self.addCleanup(sys.modules.pop, name, None)
            spec = importlib.util.spec_from_file_location("assert_top", top)
            if spec is None or spec.loader is None:
                raise RuntimeError("cannot load static-assert fixture")
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            spec.loader.exec_module(module)

            with self.assertRaisesRegex(
                Exception,
                r"ACPY-STATIC-003: assert_stage.py:16:5: entries must be positive",
            ):
                ac.jit(module.configured, workspace=root, entries=0).lower_acir()

    def test_imported_nested_config_keeps_exact_nominal_binding(self) -> None:
        import agentic_circuit as ac

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "config_contracts.py").write_text(
                "from __future__ import annotations\n"
                "import agentic_circuit as ac\n\n"
                "@ac.config\n"
                "class Geometry:\n"
                "    entries: int\n\n"
                "@ac.config\n"
                "class Config:\n"
                "    geometry: Geometry\n\n"
                'CFG = ac.param[Config]("cfg")\n\n'
                "@ac.struct\n"
                "class Entry:\n"
                "    index: ac.bits[ac.index_width(CFG.geometry.entries)]\n",
                encoding="utf-8",
            )
            (root / "other_config.py").write_text(
                "from __future__ import annotations\n"
                "import agentic_circuit as ac\n\n"
                "@ac.config\n"
                "class Geometry:\n"
                "    entries: int\n\n"
                "@ac.config\n"
                "class Config:\n"
                "    geometry: Geometry\n",
                encoding="utf-8",
            )
            top = root / "config_top.py"
            top.write_text(
                "from __future__ import annotations\n"
                "import agentic_circuit as ac\n"
                "from config_contracts import Config, Entry, Geometry\n\n"
                "@ac.system\n"
                "def design(value: Entry, *, cfg: ac.const[Config]) -> Entry:\n"
                "    return value\n",
                encoding="utf-8",
            )

            sys.path.insert(0, str(root))
            self.addCleanup(sys.path.remove, str(root))
            for name in ("config_contracts", "config_top", "other_config"):
                sys.modules.pop(name, None)
                self.addCleanup(sys.modules.pop, name, None)
            top_spec = importlib.util.spec_from_file_location("config_top", top)
            other_spec = importlib.util.spec_from_file_location(
                "other_config", root / "other_config.py"
            )
            if (
                top_spec is None
                or top_spec.loader is None
                or other_spec is None
                or other_spec.loader is None
            ):
                raise RuntimeError("cannot load imported config fixture")
            top_module = importlib.util.module_from_spec(top_spec)
            other_module = importlib.util.module_from_spec(other_spec)
            sys.modules[top_spec.name] = top_module
            sys.modules[other_spec.name] = other_module
            top_spec.loader.exec_module(top_module)
            other_spec.loader.exec_module(other_module)

            raw = ac.jit(
                top_module.design,
                workspace=root,
                cfg=top_module.Config(geometry=top_module.Geometry(entries=8)),
            ).lower_acir()
            self.assertIn("ac.static_config_bindings", raw)
            self.assertIn("cfg.geometry.entries = 8 : i64", raw)
            with self.assertRaisesRegex(TypeError, "requires config type 'Config'"):
                ac.jit(
                    top_module.design,
                    workspace=root,
                    cfg=other_module.Config(geometry=other_module.Geometry(entries=8)),
                )

    def test_multifile_helper_and_instance_nodes_keep_original_locations(self) -> None:
        import agentic_circuit as ac

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "source_helpers.py").write_text(
                "import agentic_circuit as ac\n\n"
                "@ac.inline\n"
                "def add_one(value: ac.u8) -> ac.u8:\n"
                "    return value + 1\n",
                encoding="utf-8",
            )
            top = root / "source_top.py"
            top.write_text(
                "import agentic_circuit as ac\n"
                "from source_helpers import add_one\n\n"
                "@ac.module\n"
                "def stage(value: ac.u8) -> ac.u8:\n"
                "    return add_one(value)\n\n"
                "@ac.system\n"
                "def design(value: ac.u8) -> ac.u8:\n"
                "    first = stage(value)\n"
                "    second = stage(first)\n"
                "    return second\n",
                encoding="utf-8",
            )

            sys.path.insert(0, str(root))
            self.addCleanup(sys.path.remove, str(root))
            for name in ("source_helpers", "source_top"):
                sys.modules.pop(name, None)
                self.addCleanup(sys.modules.pop, name, None)
            spec = importlib.util.spec_from_file_location("source_top", top)
            if spec is None or spec.loader is None:
                raise RuntimeError("cannot load source-stack fixture")
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            spec.loader.exec_module(module)

            raw = ac.jit(module.design, workspace=root).lower_acir()

        self.assertIn("func.func private @add_one", raw)
        self.assertIn('loc("source_helpers.py":4:1)', raw)
        self.assertIn('loc("source_helpers.py":5:12)', raw)
        self.assertIn('loc("source_top.py":10:13)', raw)
        self.assertIn('loc("source_top.py":11:14)', raw)

    def test_inline_helper_source_stack_reaches_queue_graph(self) -> None:
        import agentic_circuit as ac
        from agentic_circuit._jit import _lower_queue_acir

        repository = Path(__file__).resolve().parents[4]
        optimizer = Path(
            os.environ.get(
                "ACIR_OPT",
                repository / ".pycircuit_out/toolchain/build/bin/acir-opt-internal",
            )
        )
        planner = Path(
            os.environ.get(
                "ACIR_QUEUE_PLAN",
                repository / ".pycircuit_out/toolchain/build/bin/acir-queue-plan",
            )
        )
        if not optimizer.is_file() or not planner.is_file():
            self.skipTest("native source-stack tools are unavailable")

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "inline_helpers.py").write_text(
                "import agentic_circuit as ac\n\n"
                "@ac.inline\n"
                "def add_one(value: ac.u8) -> ac.u8:\n"
                "    return value + 1\n",
                encoding="utf-8",
            )
            top = root / "inline_top.py"
            top.write_text(
                "import agentic_circuit as ac\n"
                "from inline_helpers import add_one\n\n"
                "@ac.system\n"
                "def pipeline() -> None:\n"
                "    incoming = ac.source(ac.u8)\n"
                "    outgoing = incoming.apply(lambda item: add_one(item))\n"
                "    ac.sink(outgoing)\n",
                encoding="utf-8",
            )

            sys.path.insert(0, str(root))
            self.addCleanup(sys.path.remove, str(root))
            for name in ("inline_helpers", "inline_top"):
                sys.modules.pop(name, None)
                self.addCleanup(sys.modules.pop, name, None)
            spec = importlib.util.spec_from_file_location("inline_top", top)
            if spec is None or spec.loader is None:
                raise RuntimeError("cannot load inline source-stack fixture")
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            spec.loader.exec_module(module)

            frozen = root / "inline.frozen.mlir"
            frozen.write_text(
                _lower_queue_acir(
                    ac.jit(module.pipeline, workspace=root).lower_acir(),
                    optimizer=optimizer,
                ),
                encoding="utf-8",
            )
            completed = subprocess.run(
                (str(planner), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            plan = json.loads(completed.stdout)

        transform = next(
            block for block in plan["blocks"] if block["kind"] == "transform"
        )
        add = next(
            expression
            for expression in transform["expressions"]
            if expression["kind"] == "add"
        )
        frames = add["source_provenance"]["origins"][0]["frames"]
        self.assertEqual(
            [
                ("statement", "inline_helpers.py", 5),
                ("inline_callsite", "inline_top.py", 7),
            ],
            [(frame["kind"], frame["file"], frame["line"]) for frame in frames],
        )


if __name__ == "__main__":
    unittest.main()
