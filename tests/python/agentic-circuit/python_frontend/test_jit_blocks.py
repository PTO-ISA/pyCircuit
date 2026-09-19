from __future__ import annotations

import importlib.util
import os
import subprocess
import sys
import tempfile
import unittest
from dataclasses import FrozenInstanceError
from pathlib import Path
from unittest import mock

JIT_SOURCE = """
import agentic_circuit as ac

@ac.config
class Config:
    depth: int
    bias: int

@ac.system
def pipeline(cfg: ac.const[Config]) -> None:
    incoming = ac.source(int, depth=cfg.depth, latency=1)
    outgoing = ac.compute(
        incoming,
        lambda item: item + cfg.bias,
        depth=cfg.depth,
        latency=1,
    )
    ac.sink(outgoing)
"""


HIGH_LEVEL_SOURCE = """
import agentic_circuit as ac

@ac.config
class Config:
    engines: int
    entries: int

@ac.struct
class Token:
    sequence: ac.u8
    waits_for: ac.u8
    engine: ac.u2
    cycles: ac.u16
    value: ac.u32

@ac.system
def core(cfg: ac.const[Config]) -> None:
    incoming = ac.source(Token)
    decoded = ac.compute(
        incoming,
        lambda item: item.with_fields(value=item.value + 1),
    )
    scalar, vector, cube, tma = ac.route(
        decoded,
        by=Token.engine,
        outputs=cfg.engines,
        depth=2,
    )
    dispatched = ac.merge(
        scalar,
        vector,
        cube,
        tma,
        policy=ac.round_robin,
        depth=4,
    )
    completed = ac.schedule(
        dispatched,
        by=Token.sequence,
        waits_for=Token.waits_for,
        resource=Token.engine,
        cost=Token.cycles,
        entries=cfg.entries,
        resources=cfg.engines,
        no_dependency=255,
        depth=8,
    )
    engine_input = ac.source(Token)
    engine_done = ac.engine(
        engine_input,
        cost=Token.cycles,
        lanes=cfg.engines,
        depth=4,
    )
    engine_pipe = ac.pipeline(engine_done, stages=2, depth=2)
    retired = ac.reorder(
        completed,
        by=Token.sequence,
        entries=cfg.entries,
        start=0,
        depth=4,
    )
    ac.sink(retired)
    ac.sink(engine_pipe)
"""


STRUCTURAL_BLOCK_SOURCE = """
import agentic_circuit as ac

@ac.config
class Config:
    outputs: int
    entries: int

@ac.struct
class Request:
    address: ac.u8
    write: bool
    data: ac.u16

@ac.system
def blocks(cfg: ac.const[Config]) -> None:
    incoming = ac.source(Request)
    left, right = ac.fork(incoming, outputs=cfg.outputs, depth=2)
    left_ready, right_ready = ac.barrier(left, right, depth=2)
    storage = ac.memory(ac.u16, entries=cfg.entries, init=0, latency=1)
    response = storage.request(
        left_ready,
        address=lambda request: request.address,
        write=lambda request: request.write,
        data=lambda request: request.data,
        result_field="data",
        depth=4,
    )
    ac.sink(response)
    ac.sink(right_ready)
"""


MULTIRATE_SOURCE = """
import agentic_circuit as ac

@ac.config
class Config:
    rate: int

@ac.system
def multirate(cfg: ac.const[Config]) -> None:
    incoming = ac.source(int, depth=8, lanes=cfg.rate, rate=cfg.rate)
    computed = ac.compute(
        incoming,
        lambda item: item + 1,
        depth=8,
        rate=cfg.rate,
    )
    pipelined = ac.pipeline(
        computed, stages=2, depth=8, rate=cfg.rate
    )
    ac.sink(pipelined)
"""


REPOSITORY = Path(__file__).resolve().parents[4]
NATIVE_BUILD = REPOSITORY / ".pycircuit_out/acir/dev-llvm22"


class ConfigAndJitTest(unittest.TestCase):
    def test_capture_materializes_typed_config_from_closed_json(self) -> None:
        import agentic_circuit as ac
        from agentic_circuit._capture_worker import _jit_static_arguments
        from agentic_circuit._static_eval import FrozenMap

        @ac.config
        class Geometry:
            entries: int

        @ac.config
        class Config:
            geometry: Geometry
            lanes: int

        @ac.system
        def core(*, cfg: ac.const[Config]) -> None:
            pass

        closed = FrozenMap(
            (
                (
                    "geometry",
                    FrozenMap((("entries", 16),)),
                ),
                ("lanes", 4),
            )
        )
        restored = _jit_static_arguments(core, {"cfg": closed})["cfg"]

        self.assertIs(type(restored), Config)
        self.assertIs(type(restored.geometry), Geometry)
        self.assertEqual(16, restored.geometry.entries)
        self.assertEqual(4, restored.lanes)

    def test_config_is_an_immutable_closed_record(self) -> None:
        import agentic_circuit as ac

        @ac.config
        class Config:
            lanes: int
            enabled: bool = True

        value = Config(lanes=4)

        self.assertEqual(4, value.lanes)
        self.assertTrue(value.enabled)
        with self.assertRaises(FrozenInstanceError):
            value.lanes = 8

    def test_jit_canonicalizes_const_arguments(self) -> None:
        import agentic_circuit as ac

        @ac.config
        class Config:
            lanes: int
            entries: int

        @ac.system
        def core(cfg: ac.const[Config]) -> None:
            raise AssertionError("JIT must not execute the system body")

        left = ac.jit(core, cfg=Config(lanes=4, entries=16))
        right = ac.jit(core, cfg=Config(entries=16, lanes=4))

        self.assertEqual(left.canonical_arguments, right.canonical_arguments)
        self.assertEqual((), left.diagnostics)
        self.assertEqual(
            (("cfg", (("entries", 16), ("lanes", 4))),),
            left.canonical_arguments,
        )
        self.assertIn("core", repr(left))

    def test_module_jit_resolves_imported_typed_config(self) -> None:
        import agentic_circuit as ac

        @ac.config
        class Config:
            entries: int

        @ac.module
        def stage(*, cfg: ac.const[Config]) -> None:
            pass

        specialized = ac.jit(stage, cfg=Config(entries=8))

        self.assertEqual((("cfg", (("entries", 8),)),), specialized.canonical_arguments)

    def test_jit_rejects_wrong_nominal_or_nested_config_types(self) -> None:
        import agentic_circuit as ac

        @ac.config
        class Geometry:
            entries: int

        @ac.config
        class OtherGeometry:
            entries: int

        @ac.config
        class Config:
            geometry: Geometry

        @ac.config
        class OtherConfig:
            geometry: Geometry

        @ac.system
        def core(*, cfg: ac.const[Config]) -> None:
            pass

        ac.jit(core, cfg=Config(geometry=Geometry(entries=8)))
        with self.assertRaisesRegex(TypeError, "requires config type 'Config'"):
            ac.jit(core, cfg=OtherConfig(geometry=Geometry(entries=8)))
        with self.assertRaisesRegex(TypeError, "Config.geometry requires Geometry"):
            ac.jit(core, cfg=Config(geometry=OtherGeometry(entries=8)))
        with self.assertRaisesRegex(TypeError, "requires config type 'Config'"):
            ac.jit(core, cfg={"geometry": {"entries": 8}})

    def test_jit_binds_deferred_config_annotation_at_definition_time(self) -> None:
        import agentic_circuit as ac

        def factory():
            @ac.config
            class Config:
                entries: int

            @ac.system
            def core(*, cfg: ac.const[Config]) -> None:
                pass

            return Config, core

        first_config, first = factory()
        second_config, second = factory()

        ac.jit(first, cfg=first_config(entries=4))
        ac.jit(second, cfg=second_config(entries=4))
        with self.assertRaisesRegex(TypeError, "requires config type 'Config'"):
            ac.jit(first, cfg=second_config(entries=4))
        with self.assertRaisesRegex(TypeError, "requires config type 'Config'"):
            ac.jit(second, cfg=first_config(entries=4))

    def test_jit_normalizes_forward_config_refs_and_rejects_unresolved_types(
        self,
    ) -> None:
        import agentic_circuit as ac

        @ac.config
        class Config:
            entries: int

        @ac.system
        def quoted(*, cfg: ac.const["Config"]) -> None:
            pass

        @ac.system
        def missing(*, cfg: ac.const[MissingConfig]) -> None:
            pass

        ac.jit(quoted, cfg=Config(entries=4))
        with self.assertRaisesRegex(TypeError, "unresolved annotation 'MissingConfig'"):
            ac.jit(missing, cfg={"entries": 4})
        with self.assertRaisesRegex(TypeError, "requires config type 'Config'"):
            ac.jit(quoted, cfg={"entries": 4})

        namespace: dict[str, object] = {}
        exec(
            compile(
                "import agentic_circuit as ac\n"
                "@ac.config\n"
                "class RuntimeConfig:\n"
                "    entries: int\n"
                "@ac.system\n"
                "def runtime_forward(*, cfg: ac.const['RuntimeConfig']) -> None:\n"
                "    pass\n",
                "<runtime-forward-config>",
                "exec",
                dont_inherit=True,
            ),
            namespace,
        )
        runtime_config = namespace["RuntimeConfig"]
        runtime_forward = namespace["runtime_forward"]
        ac.jit(runtime_forward, cfg=runtime_config(entries=4))
        with self.assertRaisesRegex(TypeError, "requires config type 'RuntimeConfig'"):
            ac.jit(runtime_forward, cfg={"entries": 4})

    def test_jit_leaves_typed_runtime_parameters_unbound(self) -> None:
        import agentic_circuit as ac

        @ac.system
        def runtime(value: int, cfg: ac.const[int] = 7) -> None:
            pass

        @ac.system
        def templated(value: ac.const[int]) -> None:
            pass

        specialization = ac.jit(runtime)
        self.assertEqual((("cfg", 7),), specialization.canonical_arguments)
        with self.assertRaisesRegex(
            TypeError, "runtime system parameter 'value' cannot be specialized"
        ):
            ac.jit(runtime, value=4)
        with self.assertRaisesRegex(TypeError, "ACPY-JIT-002"):
            ac.jit(templated, value={"mutable"})

    def test_jit_uses_canonical_ijson_validation_for_const_values(self) -> None:
        import agentic_circuit as ac

        @ac.system
        def templated(value: ac.const[object]) -> None:
            pass

        for value in (-0.0, float("inf"), "\ud800", 1 << 53):
            with self.subTest(value=repr(value)):
                with self.assertRaisesRegex(TypeError, "ACPY-JIT-002"):
                    ac.jit(templated, value=value)

    def test_typed_runtime_jit_selects_static_structure(self) -> None:
        import agentic_circuit as ac

        path = (
            REPOSITORY
            / "examples/agentic-circuit"
            / "pipelines"
            / "inferred_jit_boundary_pipeline.py"
        )
        spec = importlib.util.spec_from_file_location("ac_typed_runtime_jit", path)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load typed runtime JIT example")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        increment = module.specialization
        decrement = ac.jit(
            module.inferred_jit_boundary_pipeline,
            cfg=module.BoundaryConfig(increment=False),
        )

        self.assertNotEqual(
            increment.canonical_arguments, decrement.canonical_arguments
        )
        self.assertEqual(
            (("cfg", (("increment", True),)),), increment.canonical_arguments
        )
        increment_acir = increment.lower_acir()
        decrement_acir = decrement.lower_acir()
        self.assertIn('ac.name = "value"', increment_acir)
        self.assertNotIn('ac.name = "cfg"', increment_acir)
        self.assertIn("of @add_one", increment_acir)
        self.assertNotIn("of @subtract_one(%inputs)", increment_acir)
        self.assertIn("of @subtract_one", decrement_acir)

    def test_workspace_jit_lowers_typed_state_leaf_with_runtime_boundaries(
        self,
    ) -> None:
        import agentic_circuit as ac

        fixture = (
            REPOSITORY / "tests/integration/agentic-circuit/e2e/fixtures/typed_system"
        )
        top = fixture / "top.py"
        sys.path.insert(0, str(fixture))
        self.addCleanup(sys.path.remove, str(fixture))
        for name in ("contracts", "leaf"):
            sys.modules.pop(name, None)
            self.addCleanup(sys.modules.pop, name, None)
        spec = importlib.util.spec_from_file_location("ac_typed_state_top", top)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load typed state JIT fixture")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        generation_9 = ac.jit(
            module.typed_system, workspace=fixture, owner_generation=9
        )
        generation_10 = ac.jit(
            module.typed_system, workspace=fixture, owner_generation=10
        )
        self.assertNotEqual(
            generation_9.canonical_arguments, generation_10.canonical_arguments
        )
        self.assertEqual(3, len(generation_9.sources))
        with self.assertRaisesRegex(
            TypeError, "runtime system parameter 'read_request'"
        ):
            ac.jit(
                module.typed_system,
                workspace=fixture,
                owner_generation=9,
                read_request=0,
            )

        raw = generation_9.lower_acir()
        self.assertIn("shape [128]", raw)
        self.assertIn("ac.var.record", raw)
        self.assertIn('ac.name = "read_request"', raw)
        self.assertIn('ac.name = "write_request"', raw)
        self.assertIn("parameters {owner_generation = 9 : i64}", raw)

    def test_workspace_jit_rejects_external_code_and_detects_source_changes(
        self,
    ) -> None:
        import agentic_circuit as ac

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            invalid = root / "invalid.py"
            invalid.write_text(
                "import os\n"
                "import agentic_circuit as ac\n"
                "@ac.system\n"
                "def invalid(value: int) -> int:\n"
                "    return value\n",
                encoding="utf-8",
            )
            spec = importlib.util.spec_from_file_location("ac_invalid_closure", invalid)
            if spec is None or spec.loader is None:
                raise RuntimeError("cannot load invalid closure fixture")
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            self.addCleanup(sys.modules.pop, spec.name, None)
            spec.loader.exec_module(module)
            with self.assertRaisesRegex(ValueError, "ACPY-JIT-006"):
                ac.jit(module.invalid, workspace=root)

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "stable.py"
            source.write_text(
                "import agentic_circuit as ac\n"
                "@ac.system\n"
                "def stable(value: int) -> int:\n"
                "    return value\n",
                encoding="utf-8",
            )
            spec = importlib.util.spec_from_file_location("ac_stable_closure", source)
            if spec is None or spec.loader is None:
                raise RuntimeError("cannot load stable closure fixture")
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            self.addCleanup(sys.modules.pop, spec.name, None)
            spec.loader.exec_module(module)
            specialization = ac.jit(module.stable, workspace=root)
            source.write_text(
                source.read_text(encoding="utf-8") + "# changed\n",
                encoding="utf-8",
            )
            self.assertIn("ac.system", specialization.lower_acir())


class JitQueueLoweringTest(unittest.TestCase):
    def test_jit_rule_locations_use_workspace_relative_paths(self) -> None:
        import agentic_circuit as ac

        source_text = """\
import agentic_circuit as ac

@ac.struct
class Entry:
    index: ac.u1
    value: ac.u8

@ac.rule
def replace(entries, incoming):
    old = entries[incoming.index]
    entries[incoming.index] = incoming
    return old

@ac.system
def readable(incoming: Entry) -> Entry:
    entries = ac.table[2, Entry](init=0)
    result = replace(entries, incoming)
    return result
"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src" / "readable.py"
            source.parent.mkdir()
            source.write_text(source_text, encoding="utf-8")
            spec = importlib.util.spec_from_file_location("ac_readable_source", source)
            if spec is None or spec.loader is None:
                raise RuntimeError("cannot load readable source fixture")
            module = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = module
            self.addCleanup(sys.modules.pop, spec.name, None)
            spec.loader.exec_module(module)

            lowered = ac.jit(module.readable, workspace=root).lower_acir()

        self.assertIn('loc(callsite("src/readable.py":9:1 at ', lowered)
        self.assertNotIn(str(root), lowered)

    def test_const_config_and_compute_lower_to_frozen_acir(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source
        from agentic_circuit._static_eval import FrozenMap

        constants = {
            "cfg": FrozenMap((("bias", 3), ("depth", 4))),
        }
        lowered = lower_queue_source(
            JIT_SOURCE,
            "pipeline",
            static_arguments=constants,
        )

        self.assertIn("%incoming = ac.source depth 4 latency 1", lowered)
        self.assertIn(
            "%outgoing = ac.transform %incoming depths [4] latencies [1]",
            lowered,
        )
        self.assertIn("ac.var.constant 3 : i64", lowered)
        self.assertNotIn("cfg", lowered)

    def test_missing_or_non_const_system_parameter_is_rejected(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        with self.assertRaisesRegex(QueueFrontendError, "requires static argument"):
            lower_queue_source(JIT_SOURCE, "pipeline")
        with self.assertRaisesRegex(QueueFrontendError, "must use ac.const"):
            lower_queue_source(
                JIT_SOURCE.replace("cfg: ac.const[Config]", "cfg: Config"),
                "pipeline",
                static_arguments={"cfg": 4},
            )

    def test_only_compute_accepts_function_style_lambda(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source
        from agentic_circuit._static_eval import FrozenMap

        lowered = lower_queue_source(
            JIT_SOURCE,
            "pipeline",
            static_arguments={
                "cfg": FrozenMap((("bias", 3), ("depth", 4))),
            },
        )

        self.assertEqual(1, lowered.count(" = ac.transform "))

    def test_simple_high_level_blocks_lower_to_existing_acir(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source
        from agentic_circuit._static_eval import FrozenMap

        lowered = lower_queue_source(
            HIGH_LEVEL_SOURCE,
            "core",
            static_arguments={
                "cfg": FrozenMap((("engines", 4), ("entries", 16))),
            },
        )

        self.assertIn("%decoded = ac.transform %incoming", lowered)
        self.assertIn("%scalar, %vector, %cube, %tma = ac.route %decoded", lowered)
        self.assertIn('field "engine"', lowered)
        self.assertIn(
            '%dispatched = ac.merge %scalar, %vector, %cube, %tma policy "round_robin"',
            lowered,
        )
        self.assertIn(
            "%completed = ac.dependency %dispatched capacity 16 resources 4",
            lowered,
        )
        self.assertIn("%engine_done = ac.credit %engine_input credits 4", lowered)
        self.assertIn(
            "%engine_pipe = ac.transform %engine_done depths [2] latencies [2]",
            lowered,
        )
        self.assertIn("%retired = ac.reorder %completed capacity 16", lowered)
        self.assertNotIn("lambda", lowered)

    def test_high_level_non_compute_lambda_is_rejected(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )
        from agentic_circuit._static_eval import FrozenMap

        with self.assertRaisesRegex(QueueFrontendError, "field descriptor"):
            lower_queue_source(
                HIGH_LEVEL_SOURCE.replace(
                    "by=Token.engine,", "by=lambda item: item.engine,", 1
                ),
                "core",
                static_arguments={
                    "cfg": FrozenMap((("engines", 4), ("entries", 16))),
                },
            )

    def test_schedule_requires_bounded_all_ones_dependency_sentinel(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )
        from agentic_circuit._static_eval import FrozenMap

        with self.assertRaisesRegex(
            QueueFrontendError, "requires one 'no_dependency' parameter"
        ):
            lower_queue_source(
                HIGH_LEVEL_SOURCE.replace("        no_dependency=255,\n", ""),
                "core",
                static_arguments={
                    "cfg": FrozenMap((("engines", 4), ("entries", 16))),
                },
            )

        with self.assertRaisesRegex(
            QueueFrontendError, "all-ones no_dependency sentinel"
        ):
            lower_queue_source(
                HIGH_LEVEL_SOURCE.replace("no_dependency=255", "no_dependency=127"),
                "core",
                static_arguments={
                    "cfg": FrozenMap((("engines", 4), ("entries", 16))),
                },
            )

        boolean_key = (
            HIGH_LEVEL_SOURCE.replace("sequence: ac.u8", "sequence: bool")
            .replace("waits_for: ac.u8", "waits_for: bool")
            .replace("no_dependency=255", "no_dependency=1")
        )
        with self.assertRaisesRegex(QueueFrontendError, "exact matching unsigned"):
            lower_queue_source(
                boolean_key,
                "core",
                static_arguments={
                    "cfg": FrozenMap((("engines", 4), ("entries", 16))),
                },
            )

        boolean_u1_mismatch = (
            HIGH_LEVEL_SOURCE.replace("sequence: ac.u8", "sequence: bool")
            .replace("waits_for: ac.u8", "waits_for: ac.u1")
            .replace("no_dependency=255", "no_dependency=1")
        )
        with self.assertRaisesRegex(QueueFrontendError, "exact matching unsigned"):
            lower_queue_source(
                boolean_u1_mismatch,
                "core",
                static_arguments={
                    "cfg": FrozenMap((("engines", 4), ("entries", 16))),
                },
            )

    def test_simple_structural_and_memory_blocks_reuse_existing_acir(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source
        from agentic_circuit._static_eval import FrozenMap

        lowered = lower_queue_source(
            STRUCTURAL_BLOCK_SOURCE,
            "blocks",
            static_arguments={
                "cfg": FrozenMap((("entries", 32), ("outputs", 2))),
            },
        )

        self.assertIn("%left, %right = ac.fork %incoming", lowered)
        self.assertIn("%left_ready, %right_ready = ac.barrier %left, %right", lowered)
        self.assertIn("ac.memory.instance @storage data i16 entries 32", lowered)
        self.assertIn("%response = ac.memory.request @storage, %left_ready", lowered)
        self.assertIn('result_field "data"', lowered)

    def test_parent_lowers_against_module_declaration_without_child_body(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(
            """
import agentic_circuit as ac

@ac.struct
class Token:
    value: ac.u8

@ac.module_decl(source="pkg/child.py")
def child(value: Token, *, width: ac.const[int] = 8) -> Token:
    ...

@ac.system
def parent(value: Token) -> Token:
    return child(value, width=8)
""",
            "parent",
        )

        self.assertIn(
            "ac.module.import @child__width_8", lowered
        )
        self.assertIn('from {source = "pkg/child.py"}', lowered)
        self.assertIn("ac.instance @", lowered)
        self.assertNotIn("ac.module @child__width_8", lowered)

    def test_module_body_may_instance_imported_module_declaration(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(
            """
import agentic_circuit as ac

@ac.module_decl(source="pkg/child.py")
def child(value: ac.u8) -> ac.u8:
    ...

@ac.module
def parent(value: ac.u8) -> ac.u8:
    return child(value)

@ac.system
def top(value: ac.u8) -> ac.u8:
    return parent(value)
""",
            "top",
        )

        self.assertIn("ac.module.import @child", lowered)
        self.assertIn("ac.module @parent", lowered)
        self.assertIn("ac.instance @child_0 of @child", lowered)

    def test_zero_port_parent_may_compose_imported_declarations(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(
            """
import agentic_circuit as ac

@ac.module_decl(source="pkg/ifu.py")
def ifu() -> None:
    ...

@ac.module_decl(source="pkg/ooo.py")
def ooo() -> None:
    ...

@ac.module
def spe() -> None:
    ifu()
    ooo()

@ac.system
def core() -> None:
    spe()
""",
            "core",
        )

        self.assertIn("ac.module.import @ifu : () -> ()", lowered)
        self.assertIn("ac.module.import @ooo : () -> ()", lowered)
        self.assertIn("ac.module @spe()", lowered)
        self.assertIn("ac.instance @ifu_0 of @ifu()", lowered)
        self.assertIn("ac.instance @ooo_1 of @ooo()", lowered)
        self.assertIn("ac.instance @spe_0 of @spe()", lowered)

    def test_parent_module_composes_imported_children_through_internal_queue(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(
            """
import agentic_circuit as ac

@ac.module_decl(source="pkg/decode.py")
def decode(value: ac.u8) -> ac.u16:
    ...

@ac.module_decl(source="pkg/execute.py")
def execute(value: ac.u16) -> ac.u32:
    ...

@ac.module
def pipeline(value: ac.u8) -> ac.u32:
    decoded = decode(value)
    result = execute(decoded)
    return result

@ac.system
def top(value: ac.u8) -> ac.u32:
    return pipeline(value)
""",
            "top",
        )

        self.assertIn("ac.module @pipeline", lowered)
        self.assertIn("ac.instance @decode_0 of @decode", lowered)
        self.assertIn("ac.instance @execute_1 of @execute", lowered)
        self.assertIn("ac.return %result : !ac.queue<i32>", lowered)

    def test_parent_module_supports_heterogeneous_ports_and_fanout(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(
            """
import agentic_circuit as ac

@ac.module_decl(source="pkg/widen.py")
def widen(value: ac.u8) -> ac.u16:
    ...

@ac.module_decl(source="pkg/join.py")
def join(left: ac.u16, right: ac.u16) -> tuple[ac.u32, ac.u1]:
    ...

@ac.module
def assembly(value: ac.u8) -> tuple[ac.u32, ac.u1]:
    left = widen(value)
    right = widen(value)
    result, accepted = join(left, right)
    return result, accepted

@ac.system
def top(value: ac.u8) -> tuple[ac.u32, ac.u1]:
    return assembly(value)
""",
            "top",
        )

        self.assertIn("ac.module @assembly", lowered)
        self.assertIn("ac.broadcast", lowered)
        self.assertEqual(2, lowered.count("of @widen"))
        self.assertIn("of @join", lowered)
        self.assertIn("ac.return %result, %accepted", lowered)

    def test_composite_module_forwards_typed_static_arguments(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(
            """
import agentic_circuit as ac

@ac.module_decl(source="pkg/stage.py")
def stage(value: ac.u8, *, width: ac.const[int]) -> ac.u8:
    ...

@ac.module
def pipeline(
    value: ac.u8,
    *,
    width: ac.const[int],
) -> ac.u8:
    result = stage(value, width=width)
    return result

@ac.system
def top(value: ac.u8, *, width: ac.const[int] = 8) -> ac.u8:
    return pipeline(value, width=width)
""",
            "top",
        )

        self.assertIn("ac.module @pipeline__width_8", lowered)
        self.assertIn("ac.module.import @stage__width_8", lowered)
        self.assertIn("of @stage__width_8", lowered)
        self.assertNotIn("sha", lowered.lower())

    def test_composite_module_rejects_forward_reference_as_implicit_cycle(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        source = """
import agentic_circuit as ac

@ac.module_decl(source="pkg/requester.py")
def requester(command: ac.u8, response: ac.u8) -> ac.u8:
    ...

@ac.module_decl(source="pkg/responder.py")
def responder(request: ac.u8) -> ac.u8:
    ...

@ac.module
def assembly(command: ac.u8) -> ac.u8:
    request = requester(command, response)
    response = responder(request)
    return response

@ac.system
def top(command: ac.u8) -> ac.u8:
    return assembly(command)
"""
        with self.assertRaisesRegex(
            QueueFrontendError,
            "parent or prior-child Queue values",
        ):
            lower_queue_source(source, "top")

    def test_composite_module_rejects_unconsumed_child_output(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        source = """
import agentic_circuit as ac

@ac.module_decl(source="pkg/copy.py")
def copy_value(value: ac.u8) -> ac.u8:
    ...

@ac.module
def assembly(value: ac.u8) -> ac.u8:
    result = copy_value(value)
    unused = copy_value(value)
    return result

@ac.system
def top(value: ac.u8) -> ac.u8:
    return assembly(value)
"""
        with self.assertRaisesRegex(
            QueueFrontendError,
            "every composite Queue value requires a consumer",
        ):
            lower_queue_source(source, "top")

    def test_composite_instance_names_are_injective(self) -> None:
        from agentic_circuit._queue_frontend import lower_queue_source

        lowered = lower_queue_source(
            """
import agentic_circuit as ac

@ac.module_decl(source="pkg/one.py")
def one(value: ac.u8) -> ac.u8:
    ...

@ac.module_decl(source="pkg/pair.py")
def pair(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    ...

@ac.module_decl(source="pkg/join.py")
def join(left: ac.u8, middle: ac.u8, right: ac.u8) -> ac.u8:
    ...

@ac.module
def assembly(value: ac.u8) -> ac.u8:
    a__b = one(value)
    a, b = pair(value)
    result = join(a__b, a, b)
    return result

@ac.system
def top(value: ac.u8) -> ac.u8:
    return assembly(value)
""",
            "top",
        )

        self.assertIn("ac.instance @one_0 of @one", lowered)
        self.assertIn("ac.instance @pair_1 of @pair", lowered)
        self.assertEqual(1, lowered.count('id "one_0"'))
        self.assertEqual(1, lowered.count('id "pair_1"'))

    def test_composite_rejects_duplicate_destructuring_targets(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        source = """
import agentic_circuit as ac

@ac.module_decl(source="pkg/pair.py")
def pair(value: ac.u8) -> tuple[ac.u8, ac.u8]:
    ...

@ac.module
def assembly(value: ac.u8) -> ac.u8:
    result, result = pair(value)
    return result

@ac.system
def top(value: ac.u8) -> ac.u8:
    return assembly(value)
"""
        with self.assertRaisesRegex(
            QueueFrontendError,
            "composite results require unique names",
        ):
            lower_queue_source(source, "top")

    def test_composite_rejects_compiler_reserved_local_name(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )

        source = """
import agentic_circuit as ac

@ac.module_decl(source="pkg/copy.py")
def copy_value(value: ac.u8) -> ac.u8:
    ...

@ac.module
def assembly(value: ac.u8) -> ac.u8:
    __ac_instance_copy = copy_value(value)
    return __ac_instance_copy

@ac.system
def top(value: ac.u8) -> ac.u8:
    return assembly(value)
"""
        with self.assertRaisesRegex(
            QueueFrontendError,
            "names beginning with '__ac_' are compiler-owned",
        ):
            lower_queue_source(source, "top")

    def test_multirate_queue_metadata_is_frozen(self) -> None:
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
        )
        from agentic_circuit._static_eval import FrozenMap

        arguments = {"cfg": FrozenMap((("rate", 4),))}
        lowered = lower_queue_source(
            MULTIRATE_SOURCE,
            "multirate",
            static_arguments=arguments,
        )
        self.assertEqual(3, lowered.count("ac.output_rates = array<i64: 4>"))
        with self.assertRaisesRegex(QueueFrontendError, "rate must not exceed depth"):
            lower_queue_source(
                MULTIRATE_SOURCE.replace(
                    "depth=8, rate=cfg.rate", "depth=2, rate=cfg.rate", 1
                ),
                "multirate",
                static_arguments=arguments,
            )


if __name__ == "__main__":
    unittest.main()
