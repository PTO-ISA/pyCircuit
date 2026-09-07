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
    incoming = ac.source(int, depth=8, rate=cfg.rate)
    computed = ac.compute(
        incoming, lambda item: item + 1, depth=8, rate=cfg.rate
    )
    pipelined = ac.pipeline(computed, stages=2, depth=8, rate=cfg.rate)
    ac.sink(pipelined)
"""


REPOSITORY = Path(__file__).resolve().parents[4]


class ConfigAndJitTest(unittest.TestCase):
    def test_zero_count_family_jit_uses_typed_gfsim_semantics(self) -> None:
        import agentic_circuit as ac

        path = REPOSITORY / "examples/agentic-circuit/blocks/count_leading_zeros.py"
        spec = importlib.util.spec_from_file_location("ac_count_leading_zeros", path)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load count_leading_zeros example")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        cpp = ac.jit(module.count_zeros_pipeline).lower_cpp()
        self.assertIn('#include "gfsim/count_zeros.h"', cpp)
        self.assertIn("gfsim::countLeadingZeros(item.value)", cpp)
        self.assertIn("gfsim::countTrailingZeros(item.value)", cpp)
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "count_leading_zeros.cpp"
            source.write_text(cpp, encoding="utf-8")
            harness = Path(directory) / "count_leading_zeros_harness.cpp"
            executable = Path(directory) / "count_leading_zeros_pipeline"
            harness.write_text(
                f'''#include "{source.name}"
#include <cstddef>

int main() {{
  ac_generated::CountZerosPipeline model;
  if (!model.incoming().proposePush(ac_generated::Item{{0x0120, 0, 0}}))
    return 1;
  auto rows = model.dispatch_rows();
  for (std::size_t tick = 0; tick < 5; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }}
  const auto &values = model.sink_0_values();
  return values.size() == 1 && values[0].value == 0x0120 &&
                 values[0].leading == 4 && values[0].trailing == 5
             ? 0
             : 2;
}}
''',
                encoding="utf-8",
            )
            completed = subprocess.run(
                (
                    "c++",
                    "-std=c++20",
                    "-I",
                    str(REPOSITORY / "simulator/gfsim/include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            ran = subprocess.run(
                (str(executable),), text=True, capture_output=True, check=False
            )
            self.assertEqual(0, ran.returncode, ran.stderr)

    def test_popcount_jit_uses_typed_gfsim_semantics(self) -> None:
        import agentic_circuit as ac

        path = REPOSITORY / "examples/agentic-circuit/blocks/popcount.py"
        spec = importlib.util.spec_from_file_location("ac_popcount", path)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load popcount example")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        cpp = ac.jit(module.popcount_pipeline).lower_cpp()
        self.assertIn('#include "gfsim/popcount.h"', cpp)
        self.assertIn("gfsim::populationCount(item.value)", cpp)
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "popcount.cpp"
            source.write_text(cpp, encoding="utf-8")
            harness = Path(directory) / "popcount_harness.cpp"
            executable = Path(directory) / "popcount_pipeline"
            harness.write_text(
                f'''#include "{source.name}"
#include <cstddef>

int main() {{
  ac_generated::PopcountPipeline model;
  if (!model.incoming().proposePush(ac_generated::Item{{0x1123, 0}}))
    return 1;
  auto rows = model.dispatch_rows();
  for (std::size_t tick = 0; tick < 5; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }}
  const auto &values = model.sink_0_values();
  return values.size() == 1 && values[0].value == 0x1123 &&
                 values[0].count == 5
             ? 0
             : 2;
}}
''',
                encoding="utf-8",
            )
            completed = subprocess.run(
                (
                    "c++",
                    "-std=c++20",
                    "-I",
                    str(REPOSITORY / "simulator/gfsim/include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            ran = subprocess.run(
                (str(executable),), text=True, capture_output=True, check=False
            )
            self.assertEqual(0, ran.returncode, ran.stderr)

    def test_stateful_rule_jit_uses_native_mlir_and_grouped_gfsim(self) -> None:
        import agentic_circuit as ac

        path = REPOSITORY / "examples/agentic-circuit/state/table_rule.py"
        optimizer = REPOSITORY / ".pycircuit_out/toolchain/build/bin/acir-opt-internal"
        generator = REPOSITORY / ".pycircuit_out/toolchain/build/bin/acir-queue-cxxgen"
        if not optimizer.is_file() or not generator.is_file():
            self.skipTest("native stateful-rule JIT tools are unavailable")
        spec = importlib.util.spec_from_file_location("ac_stateful_rule", path)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load stateful rule example")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        with mock.patch.dict(
            os.environ,
            {
                "ACIR_OPT": str(optimizer),
                "ACIR_QUEUE_CXXGEN": str(generator),
            },
        ):
            cpp = ac.jit(module.table_rule).lower_cpp()
        self.assertIn("gfsim::QueueTableTransition<", cpp)
        self.assertIn("table_rob()", cpp)
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "table_rule.cpp"
            source.write_text(cpp, encoding="utf-8")
            completed = subprocess.run(
                (
                    "c++",
                    "-std=c++20",
                    "-I",
                    str(REPOSITORY / "simulator/gfsim/include"),
                    "-fsyntax-only",
                    str(source),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
        self.assertEqual(0, completed.returncode, completed.stderr)

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

    def test_jit_canonicalizes_const_arguments_and_identity(self) -> None:
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

        self.assertEqual(left.fingerprint, right.fingerprint)
        self.assertEqual(
            (("cfg", (("entries", 16), ("lanes", 4))),),
            left.canonical_arguments,
        )
        self.assertIn("core", repr(left))

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

        self.assertNotEqual(increment.fingerprint, decrement.fingerprint)
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
        self.assertIn(increment.fingerprint, increment_acir)
        self.assertIn(decrement.fingerprint, decrement_acir)

    @unittest.skipIf(
        os.environ.get("AC_PYTHON_ONLY") == "1",
        "native MLIR tools are release/targeted-test dependencies",
    )
    def test_typed_runtime_jit_lowers_native_cpp(self) -> None:
        import agentic_circuit as ac

        path = (
            REPOSITORY
            / "examples/agentic-circuit"
            / "pipelines"
            / "inferred_jit_boundary_pipeline.py"
        )
        optimizer = Path(
            os.environ.get(
                "ACIR_OPT",
                REPOSITORY / ".pycircuit_out/layout-root/bin/acir-opt",
            )
        )
        generator = Path(
            os.environ.get(
                "ACIR_QUEUE_CXXGEN",
                REPOSITORY / ".pycircuit_out/layout-root/bin/acir-queue-cxxgen",
            )
        )
        if not optimizer.is_file() or not generator.is_file():
            self.skipTest("native typed-JIT tools are unavailable")
        spec = importlib.util.spec_from_file_location(
            "ac_typed_runtime_jit_native", path
        )
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load typed runtime JIT example")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        with mock.patch.dict(
            os.environ,
            {
                "ACIR_OPT": str(optimizer),
                "ACIR_QUEUE_CXXGEN": str(generator),
            },
        ):
            cpp = ac.jit(
                module.inferred_jit_boundary_pipeline,
                cfg=module.BoundaryConfig(increment=True),
            ).lower_cpp()
        self.assertIn("gfsim::SimQueue", cpp)

    def test_workspace_jit_lowers_typed_state_leaf_with_runtime_boundaries(
        self,
    ) -> None:
        import agentic_circuit as ac

        fixture = (
            REPOSITORY
            / "tests/integration/agentic-circuit/e2e/fixtures/typed_system"
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
        self.assertNotEqual(generation_9.fingerprint, generation_10.fingerprint)
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

    @unittest.skipIf(
        os.environ.get("AC_PYTHON_ONLY") == "1",
        "native MLIR tools are release/targeted-test dependencies",
    )
    def test_workspace_typed_state_jit_lowers_and_runs_native_cpp(self) -> None:
        import agentic_circuit as ac

        fixture = (
            REPOSITORY
            / "tests/integration/agentic-circuit/e2e/fixtures/typed_system"
        )
        top = fixture / "top.py"
        optimizer = Path(
            os.environ.get(
                "ACIR_OPT",
                REPOSITORY / ".pycircuit_out/layout-root/bin/acir-opt",
            )
        )
        generator = Path(
            os.environ.get(
                "ACIR_QUEUE_CXXGEN",
                REPOSITORY / ".pycircuit_out/layout-root/bin/acir-queue-cxxgen",
            )
        )
        runtime = Path(
            os.environ.get(
                "GFSIM_LIBRARY",
                REPOSITORY
                / ".pycircuit_out/layout-root/compiler/acir/gfsim/libgfsim.a",
            )
        )
        if not optimizer.is_file() or not generator.is_file() or not runtime.is_file():
            self.skipTest("native typed-JIT tools or gfsim runtime are unavailable")
        sys.path.insert(0, str(fixture))
        self.addCleanup(sys.path.remove, str(fixture))
        for name in ("contracts", "leaf"):
            sys.modules.pop(name, None)
            self.addCleanup(sys.modules.pop, name, None)
        spec = importlib.util.spec_from_file_location("ac_typed_state_native", top)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load typed state JIT fixture")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        with mock.patch.dict(
            os.environ,
            {
                "ACIR_OPT": str(optimizer),
                "ACIR_QUEUE_CXXGEN": str(generator),
            },
        ):
            cpp = ac.jit(
                module.typed_system, workspace=fixture, owner_generation=9
            ).lower_cpp()
        self.assertIn("gfsim::SimTable<Entry>", cpp)
        self.assertIn("gfsim::StateReservation::forFieldsAt", cpp)
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "typed_state.cpp"
            source.write_text(cpp, encoding="utf-8")
            harness = Path(directory) / "harness.cpp"
            executable = Path(directory) / "typed_state"
            harness.write_text(
                f'''#include "{source.name}"

int main() {{
  ac_generated::TypedSystem model;
  gfsim::SimSystem system("typed_state");
  auto rows = model.dispatch_rows();
  constexpr auto offsets = ac_generated::TypedSystem::activation_offsets();
  constexpr auto targets = ac_generated::TypedSystem::activation_targets();
  constexpr auto closure_offsets =
      ac_generated::TypedSystem::work_closure_offsets();
  constexpr auto closure_targets =
      ac_generated::TypedSystem::work_closure_targets();
  if (!system.setDispatchTable(rows) ||
      !system.setActivationPlan(offsets, targets) ||
      !system.setWorkClosurePlan(closure_offsets, closure_targets) ||
      !ac_generated::TypedSystem::schedule_initial_work(system) ||
      !model.offer_write_request(
          system,
          ac_generated::WriteRequest{{
              gfsim::UInt<7>{{127}}, gfsim::UInt<16>{{9}},
              gfsim::UInt<32>{{0x1234}}, ac_generated::AccessKind::WRITE,
              gfsim::UInt<1>{{1}}}}))
    return 1;
  if (system.run().classification != gfsim::TerminationClass::Completed)
    return 2;
  if (model.sink_1_values().size() != 1 ||
      model.sink_1_values()[0].accepted != gfsim::UInt<1>{{1}})
    return 3;
  gfsim::SimSystem read_system("typed_state_read");
  if (!read_system.setDispatchTable(rows) ||
      !read_system.setActivationPlan(offsets, targets) ||
      !read_system.setWorkClosurePlan(closure_offsets, closure_targets) ||
      !ac_generated::TypedSystem::schedule_initial_work(read_system))
    return 4;
  if (!model.offer_read_request(
          read_system,
          ac_generated::ReadRequest{{gfsim::UInt<7>{{127}},
                                     gfsim::UInt<16>{{9}}}}))
    return 5;
  if (read_system.run().classification != gfsim::TerminationClass::Completed)
    return 6;
  if (model.sink_0_values().size() != 1 ||
      model.sink_0_values()[0].found != gfsim::UInt<1>{{1}} ||
      model.sink_0_values()[0].value != gfsim::UInt<32>{{0x1234}})
    return 7;
  return 0;
}}
''',
                encoding="utf-8",
            )
            completed = subprocess.run(
                (
                    "c++",
                    "-std=c++20",
                    "-I",
                    str(REPOSITORY / "simulator/gfsim/include"),
                    str(harness),
                    str(runtime),
                    "-o",
                    str(executable),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            ran = subprocess.run(
                (str(executable),), text=True, capture_output=True, check=False
            )
        self.assertEqual(0, ran.returncode, ran.stderr)

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
            with self.assertRaisesRegex(RuntimeError, "source closure changed"):
                specialization.lower_acir()

    def test_checked_in_specialization_materializes_acir_and_cpp(self) -> None:
        path = (
            REPOSITORY
            / "examples/agentic-circuit"
            / "architecture"
            / "davincioo_jit.py"
        )
        spec = importlib.util.spec_from_file_location("ac_davincioo", path)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load specialization example")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        acir = module.specialization.lower_acir()
        cpp = module.specialization.lower_cpp()

        self.assertIn(module.specialization.fingerprint, acir)
        self.assertIn(module.specialization.fingerprint, cpp)
        self.assertIn("ac.dependency", acir)
        self.assertIn("ac.reorder", acir)
        self.assertIn("gfsim::Schedule<PTOInst, 16, 4, 255", cpp)
        self.assertIn("gfsim::Pipeline<PTOInst, 2, 1>", cpp)
        self.assertIn("gfsim::Reorder<PTOInst, 64, 0", cpp)
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "model.cpp"
            source.write_text(cpp, encoding="utf-8")
            completed = subprocess.run(
                (
                    "c++",
                    "-std=c++20",
                    "-I",
                    str(REPOSITORY / "simulator/gfsim/include"),
                    "-fsyntax-only",
                    str(source),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
        self.assertEqual(0, completed.returncode, completed.stderr)

    def test_cpp_specialization_cache_misses_then_hits(self) -> None:
        path = (
            REPOSITORY
            / "examples/agentic-circuit"
            / "architecture"
            / "davincioo_jit.py"
        )
        spec = importlib.util.spec_from_file_location("ac_jit_cache", path)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load specialization example")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        with tempfile.TemporaryDirectory() as directory:
            first = module.specialization.materialize_cpp(directory)
            second = module.specialization.materialize_cpp(directory)

            self.assertFalse(first.cache_hit)
            self.assertTrue(second.cache_hit)
            self.assertEqual(first.fingerprint, second.fingerprint)
            self.assertTrue(second.source.is_file())
            self.assertTrue(second.artifact.is_file())
            self.assertTrue(second.manifest.is_file())
            self.assertEqual(first.artifact.read_bytes(), second.artifact.read_bytes())

    def test_pyc_cpp_verilog_specialization_cache_when_toolchain_is_available(
        self,
    ) -> None:
        root_value = os.environ.get("PYC_TOOLCHAIN_ROOT")
        pycgen_value = os.environ.get("ACIR_QUEUE_PYCGEN")
        if not root_value or not pycgen_value:
            self.skipTest("pinned PYC toolchain integration paths are not configured")
        root = Path(root_value)
        path = (
            REPOSITORY
            / "examples/agentic-circuit"
            / "architecture"
            / "davincioo_jit.py"
        )
        spec = importlib.util.spec_from_file_location("ac_pyc_cache", path)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load specialization example")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        with tempfile.TemporaryDirectory() as directory:
            kwargs = {
                "pycgen_tool": pycgen_value,
                "pycc": root / "bin" / "pycc",
                "toolchain_metadata": root
                / "share"
                / "pycircuit"
                / "toolchain-metadata.json",
            }
            first = module.specialization.materialize_pyc(directory, **kwargs)
            second = module.specialization.materialize_pyc(directory, **kwargs)

            self.assertFalse(first.cache_hit)
            self.assertTrue(second.cache_hit)
            self.assertTrue(first.pyc.is_file())
            self.assertTrue((first.verilog / "davincioo.v").is_file())
            self.assertTrue(first.bundle_manifest.is_file())


class JitQueueLoweringTest(unittest.TestCase):
    @unittest.skipIf(
        os.environ.get("AC_PYTHON_ONLY") == "1",
        "native MLIR tools are release/targeted-test dependencies",
    )
    def test_rule_specialization_uses_native_mlir_pipeline(self) -> None:
        path = REPOSITORY / "examples/agentic-circuit/state/rob.py"
        spec = importlib.util.spec_from_file_location("ac_rule_rob", path)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load rule retirement example")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        self.addCleanup(sys.modules.pop, spec.name, None)
        spec.loader.exec_module(module)

        cpp = module.specialization.lower_cpp()
        self.assertIn("gfsim::QueueTransform<Entry, Entry", cpp)
        self.assertIn("gfsim::QueueReorder<Entry", cpp)

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
        with self.assertRaisesRegex(QueueFrontendError, "fingerprint is invalid"):
            lower_queue_source(
                JIT_SOURCE,
                "pipeline",
                static_arguments={"cfg": 4},
                specialization_fingerprint="sha256:bad",
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
        from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
        from agentic_circuit._queue_frontend import parse_queue_program

        cpp = lower_queue_program_to_cpp(
            parse_queue_program(
                HIGH_LEVEL_SOURCE,
                "core",
                static_arguments={
                    "cfg": FrozenMap((("engines", 4), ("entries", 16))),
                },
            )
        )
        self.assertIn("gfsim::Compute<Token, Token", cpp)
        self.assertIn("gfsim::Pipeline<Token, 2, 1>", cpp)

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

    def test_simple_structural_and_memory_blocks_reuse_existing_acir(self) -> None:
        from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
        from agentic_circuit._queue_frontend import (
            lower_queue_source,
            parse_queue_program,
        )
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
        program = parse_queue_program(
            STRUCTURAL_BLOCK_SOURCE,
            "blocks",
            static_arguments={
                "cfg": FrozenMap((("entries", 32), ("outputs", 2))),
            },
        )
        cpp = lower_queue_program_to_cpp(program)
        self.assertIn("gfsim::QueueFork", cpp)
        self.assertIn("gfsim::QueueBarrier", cpp)
        self.assertIn("gfsim::QueueMemoryArbiter<Request, gfsim::UInt<16>, 1", cpp)

    def test_multirate_queue_metadata_and_cpp_templates_are_frozen(self) -> None:
        from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
        from agentic_circuit._queue_frontend import (
            QueueFrontendError,
            lower_queue_source,
            parse_queue_program,
        )
        from agentic_circuit._static_eval import FrozenMap

        arguments = {"cfg": FrozenMap((("rate", 4),))}
        lowered = lower_queue_source(
            MULTIRATE_SOURCE,
            "multirate",
            static_arguments=arguments,
        )
        self.assertEqual(3, lowered.count("ac.output_rates = array<i64: 4>"))
        cpp = lower_queue_program_to_cpp(
            parse_queue_program(
                MULTIRATE_SOURCE,
                "multirate",
                static_arguments=arguments,
            )
        )
        self.assertIn("gfsim::Compute<gfsim::UInt<64>, gfsim::UInt<64>, 4", cpp)
        self.assertIn("gfsim::Pipeline<gfsim::UInt<64>, 2, 4>", cpp)
        self.assertGreaterEqual(cpp.count(", nullptr, 1, 4)"), 2)
        self.assertIn(", nullptr, 2, 4)", cpp)
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
