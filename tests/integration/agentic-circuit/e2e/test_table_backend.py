from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
FIXTURE_ROOT = ROOT / "tests/integration/agentic-circuit/e2e/fixtures/table_examples"
SOURCE = FIXTURE_ROOT / "table_scoreboard.py"
MASKED_SOURCE = FIXTURE_ROOT / "table_masked_update.py"
WAKEUP_SOURCE = FIXTURE_ROOT / "table_batch_wakeup.py"
MULTI_WRITER_SOURCE = FIXTURE_ROOT / "table_multi_writer_issue.py"
ALLOCATION_SOURCES = (
    (ROOT / "examples/agentic-circuit/state/issue.py", "issue"),
    (FIXTURE_ROOT / "table_allocation_rob.py", "table_allocation_rob"),
)


class TableBackendTest(unittest.TestCase):
    def test_allocation_examples_execute_replace_contract_in_both_cpp_paths(
        self,
    ) -> None:
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")

        from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
        from agentic_circuit._queue_frontend import (
            lower_queue_source,
            parse_queue_program,
        )

        cxxgen = Path(
            os.environ.get(
                "ACIR_QUEUE_CXXGEN",
                ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen",
            )
        )
        plan_tool = Path(
            os.environ.get(
                "ACIR_QUEUE_PLAN",
                ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan",
            )
        )
        pycgen = Path(
            os.environ.get(
                "ACIR_QUEUE_PYCGEN",
                ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-pycgen",
            )
        )
        for source_path, system in ALLOCATION_SOURCES:
            source_text = source_path.read_text(encoding="utf-8")
            program = parse_queue_program(source_text, system)
            acir = lower_queue_source(source_text, system)
            self.assertEqual(1, acir.count('mode "replace"'))
            self.assertIn('mode "field"', acir)
            generated = [("direct", lower_queue_program_to_cpp(program))]
            if plan_tool.is_file():
                with tempfile.TemporaryDirectory() as directory:
                    source = Path(directory) / f"{system}.mlir"
                    source.write_text(acir, encoding="utf-8")
                    planned = subprocess.run(
                        (str(plan_tool), str(source)),
                        text=True,
                        capture_output=True,
                        check=False,
                    )
                    self.assertEqual(0, planned.returncode, planned.stderr)
                    document = json.loads(planned.stdout)
                    modes = [write["mode"] for write in document["table_writes"]]
                    self.assertEqual(1, modes.count("replace"))
                    self.assertGreaterEqual(modes.count("field"), 1)
                    replace_block = next(
                        block
                        for block in document["blocks"]
                        if block["kind"] == "table_write"
                        and block["write_mode"] == "replace"
                    )
                    self.assertTrue(replace_block["write_fields"])
            if cxxgen.is_file():
                with tempfile.TemporaryDirectory() as directory:
                    source = Path(directory) / f"{system}.mlir"
                    source.write_text(acir, encoding="utf-8")
                    native = subprocess.run(
                        (str(cxxgen), str(source)),
                        text=True,
                        capture_output=True,
                        check=False,
                    )
                    self.assertEqual(0, native.returncode, native.stderr)
                    generated.append(("native", native.stdout))
            if pycgen.is_file():
                with tempfile.TemporaryDirectory() as directory:
                    source = Path(directory) / f"{system}.mlir"
                    source.write_text(acir, encoding="utf-8")
                    rejected = subprocess.run(
                        (str(pycgen), str(source)),
                        text=True,
                        capture_output=True,
                        check=False,
                    )
                    self.assertNotEqual(0, rejected.returncode)
                    self.assertIn("unsupported provisional Table", rejected.stderr)
            for variant, model_text in generated:
                self.assertEqual(1, model_text.count("gfsim::TableWriteMode::Replace"))
                with tempfile.TemporaryDirectory() as directory:
                    root = Path(directory)
                    model = root / f"{system}_{variant}.cpp"
                    harness = root / "harness.cpp"
                    executable = root / variant
                    model.write_text(model_text, encoding="utf-8")
                    if system == "table_allocation_rob":
                        harness_text = f"""#include "{model.name}"
#include <cstddef>

using Model = ac_generated::TableAllocationRob;
using Entry = ac_generated::Entry;
using Completion = ac_generated::Completion;

static int run_disabled_allocation() {{
  Model model;
  auto *rob = dynamic_cast<gfsim::SimTable<Entry> *>(model.findChild("rob"));
  if (!rob || !rob->initializeEntry(0, Entry{{true, false, 0x1111}}))
    return 1;
  auto rows = model.dispatch_rows();
  for (std::size_t tick = 0; tick < 4; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }}
  const auto &entry = rob->at(0);
  return !entry.valid && !entry.done && entry.result == 0x1111 ? 0 : 2;
}}

static int run_different_entry_updates() {{
  Model model;
  auto *rob = dynamic_cast<gfsim::SimTable<Entry> *>(model.findChild("rob"));
  if (!rob || !rob->initializeEntry(1, Entry{{true, false, 0x1111}}))
    return 3;
  if (!model.completions().proposePush(Completion{{1, 0x2222}}) ||
      !model.allocations().proposePush(Entry{{true, false, 0x3333}}))
    return 4;
  auto rows = model.dispatch_rows();
  bool saw_independent_commit = false;
  for (std::size_t tick = 0; tick < 6; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
    const auto &allocated = rob->at(0);
    const auto &completed = rob->at(1);
    if (allocated.valid && !allocated.done && allocated.result == 0x3333 &&
        completed.valid && completed.done && completed.result == 0x2222)
      saw_independent_commit = true;
  }}
  return saw_independent_commit ? 0 : 5;
}}

static int run_same_entry_replace_priority() {{
  Model model;
  auto *rob = dynamic_cast<gfsim::SimTable<Entry> *>(model.findChild("rob"));
  if (!rob || !rob->initializeEntry(0, Entry{{true, false, 0x1111}}))
    return 7;
  if (!model.completions().proposePush(Completion{{0, 0x2222}}) ||
      !model.allocations().proposePush(Entry{{true, false, 0x4444}}))
    return 8;
  auto rows = model.dispatch_rows();
  bool saw_replace_win = false;
  for (std::size_t tick = 0; tick < 6; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
    const auto &entry = rob->at(0);
    if (entry.valid && !entry.done && entry.result == 0x4444)
      saw_replace_win = true;
  }}
  return saw_replace_win ? 0 : 9;
}}

int main() {{
  if (const int status = run_disabled_allocation())
    return status;
  if (const int status = run_different_entry_updates())
    return status;
  return run_same_entry_replace_priority();
}}
"""
                    else:
                        harness_text = f"""#include "{model.name}"
#include <cstddef>

int main() {{
  ac_generated::Issue model;
  auto *issue = dynamic_cast<gfsim::SimTable<ac_generated::Entry> *>(
      model.findChild("issue"));
  if (!issue)
    return 1;
  for (std::size_t index = 0; index < issue->size(); ++index) {{
    const auto tag = static_cast<std::uint8_t>(index == 0 ? 7 : index + 10);
    if (!issue->initializeEntry(
            index, ac_generated::Entry{{true, static_cast<std::uint8_t>(index),
                                       tag, false, tag, false}}))
      return 2;
  }}
  if (!model.allocations().proposePush(
          ac_generated::Entry{{true, 99, 20, true, 21, true}}))
    return 3;
  auto rows = model.dispatch_rows();
  auto tick_once = [&](std::size_t tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }};

  for (std::size_t tick = 0; tick < 6; ++tick)
    tick_once(tick);
  if (model.allocations().totalPops() != 1)
    return 4;
  for (std::size_t index = 0; index < issue->size(); ++index)
    if (!issue->at(index).valid || issue->at(index).age != index)
      return 5;

  if (!model.wakeups().proposePush(ac_generated::Wakeup{{7, true}}))
    return 6;
  for (std::size_t tick = 6; tick < 30; ++tick)
    tick_once(tick);

  std::size_t original_grants = 0;
  std::size_t allocation_grants = 0;
  for (const auto &entry : model.sink_0_values()) {{
    if (entry.age == 0)
      ++original_grants;
    if (entry.age == 99)
      ++allocation_grants;
  }}
  if (original_grants != 1 || allocation_grants != 1)
    return 7;
  if (model.allocations().totalPops() != 1)
    return 8;
  for (std::size_t index = 0; index < issue->size(); ++index)
    if (issue->at(index).valid && issue->at(index).age == 99)
      return 9;
  return 0;
}}
"""
                    harness.write_text(harness_text, encoding="utf-8")
                    compiled = subprocess.run(
                        (
                            compiler,
                            "-std=c++20",
                            "-I",
                            str(ROOT / "simulator/gfsim/include"),
                            str(harness),
                            "-o",
                            str(executable),
                        ),
                        cwd=root,
                        text=True,
                        capture_output=True,
                        check=False,
                    )
                    self.assertEqual(0, compiled.returncode, compiled.stderr)
                    executed = subprocess.run(
                        (str(executable),),
                        cwd=root,
                        text=True,
                        capture_output=True,
                        check=False,
                    )
                    self.assertEqual(
                        0,
                        executed.returncode,
                        f"{system} {variant} exited {executed.returncode}: "
                        f"{executed.stderr}",
                    )

    def test_disjoint_writers_merge_same_entry_in_both_cpp_paths(self) -> None:
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")

        from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
        from agentic_circuit._queue_frontend import (
            lower_queue_source,
            parse_queue_program,
        )

        text = MULTI_WRITER_SOURCE.read_text(encoding="utf-8")
        acir = lower_queue_source(text, "table_multi_writer_issue")
        self.assertIn('write_fields ["src0_ready"]', acir)
        self.assertIn('write_fields ["src1_ready"]', acir)
        self.assertIn('write_fields ["valid"]', acir)
        generated = [
            (
                "direct",
                lower_queue_program_to_cpp(
                    parse_queue_program(text, "table_multi_writer_issue")
                ),
            )
        ]
        cxxgen = Path(
            os.environ.get(
                "ACIR_QUEUE_CXXGEN",
                ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen",
            )
        )
        plan_tool = Path(
            os.environ.get(
                "ACIR_QUEUE_PLAN",
                ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan",
            )
        )
        if plan_tool.is_file():
            with tempfile.TemporaryDirectory() as directory:
                source = Path(directory) / "multi_writer.mlir"
                source.write_text(acir, encoding="utf-8")
                planned = subprocess.run(
                    (str(plan_tool), str(source)),
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, planned.returncode, planned.stderr)
                document = json.loads(planned.stdout)
                self.assertEqual(3, len(document["table_matches"]))
                self.assertEqual(1, len(document["table_selections"]))
                expression_kinds = [
                    expression["kind"]
                    for block in document["blocks"]
                    for expression in block["expressions"]
                ]
                self.assertIn("table_match_ref", expression_kinds)
                self.assertIn("table_selection_index_ref", expression_kinds)
                self.assertNotIn("table_match", expression_kinds)
                self.assertNotIn("table_choose_index", expression_kinds)
        if cxxgen.is_file():
            with tempfile.TemporaryDirectory() as directory:
                source = Path(directory) / "multi_writer.mlir"
                source.write_text(acir, encoding="utf-8")
                native = subprocess.run(
                    (str(cxxgen), str(source)),
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, native.returncode, native.stderr)
                generated.append(("native", native.stdout))

        for variant, model_text in generated:
            self.assertIn("target.valid = value.valid", model_text)
            self.assertIn("target.src0_ready = value.src0_ready", model_text)
            self.assertIn("target.src1_ready = value.src1_ready", model_text)
            self.assertEqual(1, model_text.count("using table_selection_0_cache"))
            self.assertIn("table_selection_0->get(epoch).index", model_text)
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                model = root / "table_multi_writer.cpp"
                harness = root / "harness.cpp"
                executable = root / variant
                model.write_text(model_text, encoding="utf-8")
                harness.write_text(
                    f"""#include "{model.name}"
#include <cstddef>

int main() {{
  ac_generated::TableMultiWriterIssue model;
  auto *issue = dynamic_cast<gfsim::SimTable<ac_generated::Entry> *>(
      model.findChild("issue"));
  if (!issue || !issue->initializeEntry(
                    0, ac_generated::Entry{{true, 0, 0, false, 0, false}}))
    return 1;
  if (!model.wakeups().proposePush(ac_generated::Wakeup{{0, true}}))
    return 2;
  auto rows = model.dispatch_rows();
  bool saw_ready_before_select = false;
  for (std::size_t tick = 0; tick < 16; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
    const auto &entry = issue->at(0);
    if (entry.valid && entry.src0_ready && entry.src1_ready)
      saw_ready_before_select = true;
  }}
  const auto &selected = issue->at(0);
  if (!saw_ready_before_select || selected.valid || !selected.src0_ready ||
      !selected.src1_ready)
    return 3;
  for (std::size_t index = 1; index < issue->size(); ++index) {{
    const auto &entry = issue->at(index);
    if (entry.valid || entry.src0_ready || entry.src1_ready)
      return 4;
  }}
  bool saw_selected_output = false;
  for (const auto &entry : model.sink_0_values())
    if (entry.valid && entry.src0_ready && entry.src1_ready)
      saw_selected_output = true;
  if (!saw_selected_output)
    return 5;
  return 0;
}}
""",
                    encoding="utf-8",
                )
                compiled = subprocess.run(
                    (
                        compiler,
                        "-std=c++20",
                        "-I",
                        str(ROOT / "simulator/gfsim/include"),
                        str(harness),
                        "-o",
                        str(executable),
                    ),
                    cwd=root,
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, compiled.returncode, compiled.stderr)
                executed = subprocess.run(
                    (str(executable),), cwd=root, capture_output=True, check=False
                )
                self.assertEqual(0, executed.returncode, executed.stderr)

    def test_batch_wakeup_example_generates_both_cpp_paths(self) -> None:
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")

        from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
        from agentic_circuit._queue_frontend import (
            lower_queue_source,
            parse_queue_program,
        )

        text = WAKEUP_SOURCE.read_text(encoding="utf-8")
        acir = lower_queue_source(text, "table_batch_wakeup")
        self.assertIn("ac.table.match @issue", acir)
        self.assertIn("ac.table.masked_write @issue", acir)

        generated = [
            (
                "direct",
                lower_queue_program_to_cpp(
                    parse_queue_program(text, "table_batch_wakeup")
                ),
            )
        ]
        cxxgen = Path(
            os.environ.get(
                "ACIR_QUEUE_CXXGEN",
                ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen",
            )
        )
        if cxxgen.is_file():
            with tempfile.TemporaryDirectory() as directory:
                source = Path(directory) / "wakeup.mlir"
                source.write_text(acir, encoding="utf-8")
                native = subprocess.run(
                    (str(cxxgen), str(source)),
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, native.returncode, native.stderr)
                generated.append(("native", native.stdout))

        for variant, model_text in generated:
            self.assertIn("gfsim::TableMaskedWriteSource<Entry", model_text)
            with tempfile.TemporaryDirectory() as directory:
                source = Path(directory) / f"{variant}.cpp"
                source.write_text(model_text, encoding="utf-8")
                compiled = subprocess.run(
                    (
                        compiler,
                        "-std=c++20",
                        "-fsyntax-only",
                        "-I",
                        str(ROOT / "simulator/gfsim/include"),
                        str(source),
                    ),
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, compiled.returncode, compiled.stderr)

    def test_masked_update_direct_and_native_generators_compile_and_run(self) -> None:
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")

        from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
        from agentic_circuit._queue_frontend import (
            lower_queue_source,
            parse_queue_program,
        )

        text = MASKED_SOURCE.read_text(encoding="utf-8")
        direct = lower_queue_program_to_cpp(
            parse_queue_program(text, "table_masked_update")
        )
        generated = [("direct", direct)]
        cxxgen = Path(
            os.environ.get(
                "ACIR_QUEUE_CXXGEN",
                ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen",
            )
        )
        pycgen = Path(
            os.environ.get(
                "ACIR_QUEUE_PYCGEN",
                ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-pycgen",
            )
        )
        if cxxgen.is_file():
            with tempfile.TemporaryDirectory() as directory:
                acir_file = Path(directory) / "masked.mlir"
                acir_file.write_text(
                    lower_queue_source(text, "table_masked_update"), encoding="utf-8"
                )
                native = subprocess.run(
                    (str(cxxgen), str(acir_file)),
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, native.returncode, native.stderr)
                generated.append(("native", native.stdout))
                if pycgen.is_file():
                    rejected = subprocess.run(
                        (str(pycgen), str(acir_file)),
                        text=True,
                        capture_output=True,
                        check=False,
                    )
                    self.assertNotEqual(0, rejected.returncode)
                    self.assertIn(
                        "unsupported provisional Table",
                        rejected.stdout + rejected.stderr,
                    )

        for variant, model_text in generated:
            self.assertIn("gfsim::TableMaskedWriteSource<Entry", model_text)
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                model = root / "table_masked_update.cpp"
                harness = root / "harness.cpp"
                executable = root / variant
                model.write_text(model_text, encoding="utf-8")
                harness.write_text(
                    f"""#include "{model.name}"
#include <cstddef>

int main() {{
  ac_generated::TableMaskedUpdate model;
  if (!model.updates().proposePush(ac_generated::Update{{7}}))
    return 1;
  auto rows = model.dispatch_rows();
  for (std::size_t tick = 0; tick < 12; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }}
  for (const auto &entry : model.sink_0_values())
    if (entry.valid && entry.tag == 7 && entry.age == 1)
      return 0;
  return 2;
}}
""",
                    encoding="utf-8",
                )
                compiled = subprocess.run(
                    (
                        compiler,
                        "-std=c++20",
                        "-I",
                        str(ROOT / "simulator/gfsim/include"),
                        str(harness),
                        "-o",
                        str(executable),
                    ),
                    cwd=root,
                    env=os.environ,
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, compiled.returncode, compiled.stderr)
                executed = subprocess.run(
                    (str(executable),),
                    cwd=root,
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, executed.returncode, executed.stderr)

    def test_python_codegen_checks_queue_driven_entry_expressions(self) -> None:
        from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
        from agentic_circuit._queue_frontend import parse_queue_program

        source = SOURCE.read_text(encoding="utf-8").replace(
            "    responses = scoreboard.view(lambda request: request.index).read(\n"
            "        requests,\n"
            "        when=lambda request: request.enable,",
            "    requested_entry = scoreboard.view(lambda request: request.index)\n"
            "    responses = requested_entry.read(\n"
            "        requests,\n"
            "        when=lambda request: request.enable and requested_entry.valid,",
        )
        generated = lower_queue_program_to_cpp(
            parse_queue_program(source, "table_scoreboard")
        )
        self.assertIn(
            "item.enable && table->checkedAt(static_cast<size_t>(item.index)).valid",
            generated,
        )

    def test_scoreboard_generates_compiles_and_runs_old_data_contract(self) -> None:
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")

        from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
        from agentic_circuit._queue_frontend import parse_queue_program

        generated = lower_queue_program_to_cpp(
            parse_queue_program(SOURCE.read_text(encoding="utf-8"), "table_scoreboard")
        )
        self.assertIn("gfsim::SimTable<Entry>", generated)
        self.assertIn("gfsim::QueueTableWrite<Update, Entry", generated)
        self.assertIn("gfsim::QueueTableRead<Request, Entry", generated)
        self.assertIn("gfsim::TableReadSource<Entry", generated)
        self.assertIn("table->checkedAt(static_cast<size_t>", generated)

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            model = root / "table_scoreboard.cpp"
            harness = root / "harness.cpp"
            executable = root / "table_scoreboard"
            model.write_text(generated, encoding="utf-8")
            harness.write_text(
                f"""#include "{model.name}"
#include <cstddef>

int main() {{
  ac_generated::TableScoreboard model;
  if (!model.updates().proposePush(ac_generated::Update{{0, true, true, 0x1234}}))
    return 1;
  if (!model.requests().proposePush(ac_generated::Request{{0, true}}))
    return 2;
  auto rows = model.dispatch_rows();
  for (std::size_t tick = 0; tick < 16; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
    if (tick == 1 &&
        !model.requests().proposePush(ac_generated::Request{{0, true}}))
      return 7;
  }}
  const auto &responses = model.sink_0_values();
  if (responses.size() != 2)
    return 3;
  if (responses[0].valid || responses[0].done || responses[0].result != 0)
    return 4;
  if (!responses[1].valid || !responses[1].done || responses[1].result != 0x1234)
    return 5;
  const auto &snapshots = model.sink_1_values();
  if (snapshots.empty() || !snapshots.front().valid ||
      snapshots.front().result != 0x1234)
    return 6;
  return 0;
}}
""",
                encoding="utf-8",
            )
            compiled = subprocess.run(
                (
                    compiler,
                    "-std=c++20",
                    "-I",
                    str(ROOT / "simulator/gfsim/include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                cwd=root,
                env=os.environ,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, compiled.returncode, compiled.stderr)
            executed = subprocess.run(
                (str(executable),),
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, executed.returncode, executed.stderr)

    def test_native_queuegraph_and_pyc_boundary_when_tools_are_available(self) -> None:
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("C++ compiler is unavailable")
        tools = {
            "opt": Path(
                os.environ.get(
                    "ACIR_OPT", ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"
                )
            ),
            "plan": Path(
                os.environ.get(
                    "ACIR_QUEUE_PLAN",
                    ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan",
                )
            ),
            "cxxgen": Path(
                os.environ.get(
                    "ACIR_QUEUE_CXXGEN",
                    ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen",
                )
            ),
            "pycgen": Path(
                os.environ.get(
                    "ACIR_QUEUE_PYCGEN",
                    ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-pycgen",
                )
            ),
        }
        missing = [name for name, path in tools.items() if not path.is_file()]
        if missing:
            self.skipTest(
                "native QueueGraph tools are unavailable: " + ", ".join(missing)
            )

        from agentic_circuit._queue_frontend import lower_queue_source

        acir = lower_queue_source(
            SOURCE.read_text(encoding="utf-8"), "table_scoreboard"
        )
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "table.ac.mlir"
            frozen = root / "table.frozen.mlir"
            source.write_text(acir, encoding="utf-8")
            verified = subprocess.run(
                (str(tools["opt"]), "--verify-ac-file", str(source), "-o", str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verified.returncode, verified.stderr)
            planned = subprocess.run(
                (str(tools["plan"]), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, planned.returncode, planned.stderr)
            document = json.loads(planned.stdout)
            self.assertEqual("0.5", document["contract_epoch"])
            self.assertEqual(1, len(document["tables"]))
            self.assertEqual(2, len(document["table_reads"]))
            self.assertEqual(1, len(document["table_writes"]))
            self.assertEqual(
                ["valid", "done", "result"],
                document["table_writes"][0]["write_fields"],
            )
            table_write_block = next(
                block for block in document["blocks"] if block["kind"] == "table_write"
            )
            self.assertEqual(
                ["valid", "done", "result"], table_write_block["write_fields"]
            )

            generated = subprocess.run(
                (str(tools["cxxgen"]), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            self.assertIn("gfsim::SimTable<Entry>", generated.stdout)
            self.assertIn("table->checkedAt(static_cast<size_t>", generated.stdout)

            model = root / "table_scoreboard.cpp"
            harness = root / "harness.cpp"
            executable = root / "table_scoreboard"
            model.write_text(generated.stdout, encoding="utf-8")
            harness.write_text(
                f"""#include "{model.name}"
#include <cstddef>

int main() {{
  ac_generated::TableScoreboard model;
  if (!model.updates().proposePush(ac_generated::Update{{0, true, true, 0x1234}}))
    return 1;
  if (!model.requests().proposePush(ac_generated::Request{{0, true}}))
    return 2;
  auto rows = model.dispatch_rows();
  for (std::size_t tick = 0; tick < 16; ++tick) {{
    const gfsim::Epoch epoch{{tick, 0}};
    for (auto &row : rows)
      row.work(row.object, epoch);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
    if (tick == 1 &&
        !model.requests().proposePush(ac_generated::Request{{0, true}}))
      return 7;
  }}
  const auto &responses = model.sink_0_values();
  if (responses.size() != 2)
    return 3;
  if (responses[0].valid || responses[0].done || responses[0].result != 0)
    return 4;
  if (!responses[1].valid || !responses[1].done || responses[1].result != 0x1234)
    return 5;
  const auto &snapshots = model.sink_1_values();
  if (snapshots.empty() || !snapshots.front().valid ||
      snapshots.front().result != 0x1234)
    return 6;
  return 0;
}}
""",
                encoding="utf-8",
            )
            compiled = subprocess.run(
                (
                    compiler,
                    "-std=c++20",
                    "-I",
                    str(ROOT / "simulator/gfsim/include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                cwd=root,
                env=os.environ,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, compiled.returncode, compiled.stderr)
            executed = subprocess.run(
                (str(executable),),
                cwd=root,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, executed.returncode, executed.stderr)

            rejected = subprocess.run(
                (str(tools["pycgen"]), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(0, rejected.returncode)
            self.assertIn("unsupported provisional Table", rejected.stderr)


if __name__ == "__main__":
    unittest.main()
