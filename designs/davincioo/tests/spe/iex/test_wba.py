from __future__ import annotations

import ast
import os
import re
import subprocess
import tempfile
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.spe.iex.wba import wba_system

ROOT = Path(__file__).resolve().parents[5]
DEFAULT_ACIR = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"


def _assert_wba_match_fusion(generated: str) -> None:
    assert (
        generated.count(
            "for (std::size_t index = 0; index < table_entries->size(); ++index)"
        )
        == 13
    )
    assert generated.count("fused_match_") == 4
    fused_begin = generated.index("auto [fused_match_")
    fused_end = generated.index("}();", fused_begin)
    fused_predicates = generated[fused_begin:fused_end]
    assert fused_predicates.count("entry.valid") == 1
    assert fused_predicates.count("entry.completed") == 1
    assert fused_predicates.count("item.key") == 1


def test_wba_uses_nominal_attempt_equality_without_expanded_leaf_chains() -> None:
    source_path = ROOT / "designs/davincioo/spe/iex/wba.py"
    source = source_path.read_text(encoding="utf-8")
    tree = ast.parse(source)
    comparisons = [node for node in ast.walk(tree) if isinstance(node, ast.Compare)]
    aggregate_attempt_comparisons = [
        node
        for node in comparisons
        if "attempt" in ast.unparse(node) and "==" in ast.unparse(node)
    ]
    assert len(aggregate_attempt_comparisons) >= 6
    for legacy_field in (
        ".core_id ==",
        ".pe_id ==",
        ".instruction_sequence ==",
        ".original_pc ==",
        ".attempt_generation ==",
    ):
        assert legacy_field not in source


def test_wba_source_closure_reaches_frozen_queuegraph_and_rejects_table_pyc() -> None:
    from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

    optimizer = Path(os.environ.get("ACIR_OPT", DEFAULT_ACIR))
    cxxgen = optimizer.with_name("acir-queue-cxxgen")
    pycgen = optimizer.with_name("acir-queue-pycgen")
    compiler = os.environ.get("CXX", "c++")
    if not optimizer.is_file():
        pytest.skip(f"current-checkout acir-opt is unavailable: {optimizer}")
    if not cxxgen.is_file() or not pycgen.is_file():
        pytest.skip("current-checkout QueueGraph generators are unavailable")
    specialization = ac.jit(wba_system, workspace=ROOT)
    raw = specialization.lower_acir()
    assert raw.count('ac.var.cmp "eq"') >= 6
    assert "ac.var.decl @entries" in raw
    assert "ac.var.decl @cancel_tombstones" in raw
    with tempfile.TemporaryDirectory(prefix="davincioo-wba-") as directory:
        raw_path = Path(directory) / "raw.mlir"
        frozen_path = Path(directory) / "frozen.mlir"
        raw_path.write_text(raw, encoding="utf-8")
        lowered = subprocess.run(
            (
                str(optimizer),
                f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                str(raw_path),
                "-o",
                str(frozen_path),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert lowered.returncode == 0, lowered.stderr
        frozen = frozen_path.read_text(encoding="utf-8")
        assert "ac.var.invariant" not in frozen
        assert "ac.firing" in frozen
        assert "ac.table" in frozen

        generated = subprocess.run(
            (str(cxxgen), str(frozen_path)),
            text=True,
            capture_output=True,
            check=False,
        )
        assert generated.returncode == 0, generated.stderr
        _assert_wba_match_fusion(generated.stdout)
        aggregate_table_reads = re.findall(
            r"^\s+const auto &[A-Za-z0-9_]+ = "
            r"table_[A-Za-z0-9_]+->at\(static_cast<size_t>",
            generated.stdout,
            re.MULTILINE,
        )
        expected_aggregate_reads = frozen.count("ac.table.get @entries") + frozen.count(
            "ac.table.get @cancel_tombstones"
        )
        assert len(aggregate_table_reads) == expected_aggregate_reads
        assert not re.search(
            r"^\s+auto [A-Za-z0-9_]+ = "
            r"table_(?:entries|cancel_tombstones)->at\(static_cast<size_t>",
            generated.stdout,
            re.MULTILINE,
        )
        model_path = Path(directory) / "model.cpp"
        model_path.write_text(
            generated.stdout
            + """
int main() {
  ac_generated::WbaSystem model;
  auto rows = model.dispatch_rows();
  const gfsim::Epoch epoch{1, 0};
  for (auto &row : rows) row.work(row.object, epoch);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  return model.sink_0_values().empty() && model.sink_1_values().empty() &&
                 model.sink_2_values().empty() && model.sink_3_values().empty()
             ? 0
             : 1;
}
""",
            encoding="utf-8",
        )
        executable = Path(directory) / "model"
        compiled = subprocess.run(
            (
                compiler,
                "-std=c++20",
                "-I",
                str(ROOT / "simulator/gfsim/include"),
                str(model_path),
                "-o",
                str(executable),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert compiled.returncode == 0, compiled.stderr
        executed = subprocess.run(
            (str(executable),), text=True, capture_output=True, check=False
        )
        assert executed.returncode == 0, executed.stderr

        pyc = subprocess.run(
            (str(pycgen), str(frozen_path)),
            text=True,
            capture_output=True,
            check=False,
        )
        assert pyc.returncode != 0
        assert (
            "module-preserving QueueGraph PYC lowering is not implemented" in pyc.stderr
            or "unsupported provisional Table" in pyc.stderr
        )


def test_wba_executes_terminal_apply_cancel_drain_and_backpressure_matrix() -> None:
    from agentic_circuit._jit import _lower_acir_to_cpp

    optimizer = Path(os.environ.get("ACIR_OPT", DEFAULT_ACIR))
    cxxgen = optimizer.with_name("acir-queue-cxxgen")
    compiler = os.environ.get("CXX", "c++")
    if not optimizer.is_file() or not cxxgen.is_file():
        pytest.skip("current-checkout ACIR/gfsim toolchain is unavailable")

    generated = _lower_acir_to_cpp(ac.jit(wba_system, workspace=ROOT).lower_acir())
    _assert_wba_match_fusion(generated)
    assert re.search(
        r"^\s+const auto &[A-Za-z0-9_]+ = "
        r"table_[A-Za-z0-9_]+->at\(static_cast<size_t>",
        generated,
        re.MULTILINE,
    )
    assert not re.search(
        r"^\s+auto [A-Za-z0-9_]+ = "
        r"table_(?:entries|cancel_tombstones)->at\(static_cast<size_t>",
        generated,
        re.MULTILINE,
    )
    with tempfile.TemporaryDirectory(prefix="davincioo-wba-behavior-") as directory:
        source = Path(directory) / "model.cpp"
        executable = Path(directory) / "model"
        source.write_text(
            generated
            + r"""
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
using namespace ac_generated;

template <typename T, size_t N>
T &objectByName(std::array<gfsim::DispatchRow, N> &rows,
                std::string_view name, gfsim::ObjectKind kind) {
  T *matched = nullptr;
  for (auto &row : rows) {
    auto *object = static_cast<gfsim::SimObject *>(row.object);
    if (row.kind != kind || object->name() != name)
      continue;
    if (matched != nullptr)
      throw std::runtime_error("duplicate generated object name");
    matched = dynamic_cast<T *>(object);
    if (matched == nullptr)
      throw std::runtime_error("generated object type mismatch");
  }
  if (matched == nullptr)
    throw std::runtime_error("missing generated object name");
  return *matched;
}

template <size_t N>
void cycle(std::array<gfsim::DispatchRow, N> &rows, std::uint64_t &tick,
           std::array<bool, 4> drain = {true, true, true, true}) {
  const gfsim::Epoch epoch{tick++, 0};
  for (auto &row : rows) {
    bool run = true;
    if (row.kind == gfsim::ObjectKind::Sink) {
      const auto &name = static_cast<gfsim::SimObject *>(row.object)->name();
      if (name == "sink_sink_0") run = drain[0];
      else if (name == "sink_sink_1") run = drain[1];
      else if (name == "sink_sink_2") run = drain[2];
      else if (name == "sink_sink_3") run = drain[3];
    }
    if (run)
      row.work(row.object, epoch);
  }
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
}

template <typename T>
bool offer(gfsim::SimQueue<T> &queue, const T &value, std::uint64_t &tick) {
  if (!queue.proposePush(value))
    return false;
  queue.doXfer({tick++, 0});
  return true;
}

IssueAttemptKey makeKey(unsigned id, ExecutionClass executionClass) {
  IssueAttemptKey key{};
  key.identity.epoch.flow.core_id = gfsim::UInt<4>{id};
  key.identity.inst.flow = key.identity.epoch.flow;
  key.identity.block.flow = key.identity.epoch.flow;
  key.identity.rob.flow = key.identity.epoch.flow;
  key.identity.dispatch.flow = key.identity.epoch.flow;
  key.identity.dispatch.execution_class = executionClass;
  key.identity.dispatch.valid = gfsim::UInt<1>{1};
  key.attempt_generation = gfsim::UInt<16>{id};
  return key;
}

TerminalResult makeBase(unsigned id, unsigned age, TerminalSource source,
                        TerminalStatus status, ExecutionClass executionClass,
                        unsigned effects) {
  TerminalResult result{};
  result.source = source;
  result.status = status;
  result.attempt = makeKey(id, executionClass);
  result.age_order = gfsim::UInt<48>{age};
  result.required_effect_mask = gfsim::UInt<7>{effects};
  result.valid = gfsim::UInt<1>{1};
  return result;
}

TerminalResult makeValue(unsigned id, unsigned age) {
  auto result = makeBase(id, age, TerminalSource::ALU, TerminalStatus::VALUE,
                         ExecutionClass::ALU, 0b0000111);
  result.destination = PhysRef{gfsim::UInt<7>{id + 1}, gfsim::UInt<16>{1},
                               gfsim::UInt<1>{1}};
  result.destination_arch_index = gfsim::UInt<5>{id + 1};
  result.result = gfsim::UInt<64>{100 + id};
  result.result_valid = gfsim::UInt<1>{1};
  return result;
}

TerminalResult makeFault(unsigned id, unsigned age) {
  auto result = makeBase(id, age, TerminalSource::ALU, TerminalStatus::FAULT,
                         ExecutionClass::ALU, 0b0011100);
  result.fault_code = gfsim::UInt<32>{17};
  result.fault_valid = gfsim::UInt<1>{1};
  return result;
}

TerminalResult makeStore(unsigned id, unsigned age) {
  auto result = makeBase(id, age, TerminalSource::LSU, TerminalStatus::STORE,
                         ExecutionClass::AGU, 0b0101100);
  result.store.address_ready = gfsim::UInt<1>{1};
  result.store.data_ready = gfsim::UInt<1>{1};
  result.store.valid = gfsim::UInt<1>{1};
  return result;
}

TerminalResult makeBranch(unsigned id, unsigned age) {
  auto result = makeBase(id, age, TerminalSource::BRU, TerminalStatus::BRANCH,
                         ExecutionClass::BRU, 0b1001100);
  result.branch.target_valid = gfsim::UInt<1>{1};
  result.branch.valid = gfsim::UInt<1>{1};
  return result;
}

TerminalResult makeNoDestination(unsigned id, unsigned age) {
  return makeBase(id, age, TerminalSource::FSU,
                  TerminalStatus::NO_DESTINATION, ExecutionClass::FSU,
                  0b0001100);
}

bool offerLane(WbaSystem &model, unsigned lane, const TerminalResult &result,
               std::uint64_t &tick) {
  switch (lane) {
  case 0: return offer(model.alu_result(), result, tick);
  case 1: return offer(model.bru_result(), result, tick);
  case 2: return offer(model.lsu_result(), result, tick);
  case 3: return offer(model.other_result(), result, tick);
  default: return false;
  }
}

template <typename T>
size_t validCount(const gfsim::SimTable<T> &table) {
  size_t count = 0;
  for (size_t index = 0; index < table.size(); ++index)
    count += static_cast<bool>(table.at(index).valid);
  return count;
}

bool runCommitCase(TerminalResult result, unsigned lane,
                   TerminalStatus expected) {
  WbaSystem model;
  auto rows = model.dispatch_rows();
  std::uint64_t tick = 0;
  if (!offerLane(model, lane, result, tick)) return false;
  for (int i = 0; i < 4; ++i) cycle(rows, tick);
  return model.sink_0_values().size() == 1 &&
         model.sink_0_values().front().entry.result.status == expected &&
         static_cast<bool>(model.sink_0_values().front().valid);
}

int main() {
  if (!runCommitCase(makeFault(3, 3), 0, TerminalStatus::FAULT) ||
      !runCommitCase(makeStore(4, 4), 2, TerminalStatus::STORE) ||
      !runCommitCase(makeBranch(5, 5), 1, TerminalStatus::BRANCH) ||
      !runCommitCase(makeNoDestination(6, 6), 3,
                     TerminalStatus::NO_DESTINATION))
    return 1;

  WbaSystem model;
  WbaSystem isolated;
  auto rows = model.dispatch_rows();
  auto isolatedRows = isolated.dispatch_rows();
  std::uint64_t tick = 0;
  auto &entries = objectByName<gfsim::SimTable<WbaEntry>>(
      rows, "entries", gfsim::ObjectKind::Memory);
  auto &tombstones = objectByName<gfsim::SimTable<AttemptTombstone>>(
      rows, "cancel_tombstones", gfsim::ObjectKind::Memory);
  auto &isolatedEntries = objectByName<gfsim::SimTable<WbaEntry>>(
      isolatedRows, "entries", gfsim::ObjectKind::Memory);

  // Keep one commit queued to block publication. Concurrent producers then
  // contend for one Table row; the losing input is retained and consumed on
  // the following cycle. Once the sink is released, the two unpublished rows
  // are selected by age.
  auto value = makeValue(1, 30);
  auto branch = makeBranch(2, 20);
  auto store = makeStore(3, 10);
  if (!offerLane(model, 0, value, tick))
    return 2;
  cycle(rows, tick, {false, true, true, true});
  cycle(rows, tick, {false, true, true, true});
  auto &commitQueue = objectByName<gfsim::SimQueue<WritebackCommit>>(
      rows, "commit__ack_result__cancel_ack__drain_ack_0",
      gfsim::ObjectKind::Queue);
  if (commitQueue.committedSize() != 1 ||
      commitQueue.committedValues().front().entry.result.attempt != value.attempt)
    return 3;
  if (!offerLane(model, 1, branch, tick) || !offerLane(model, 2, store, tick))
    return 4;
  cycle(rows, tick, {false, true, true, true});
  if (model.bru_result().committedSize() +
          model.lsu_result().committedSize() != 1 ||
      validCount(entries) != 2)
    return 5;
  cycle(rows, tick, {false, true, true, true});
  if (!model.bru_result().isEmpty() || !model.lsu_result().isEmpty() ||
      validCount(entries) != 3 || validCount(isolatedEntries) != 0 ||
      commitQueue.committedSize() != 1 || !model.sink_0_values().empty())
    return 6;
  cycle(rows, tick);
  cycle(rows, tick);
  cycle(rows, tick);
  cycle(rows, tick);
  cycle(rows, tick);
  if (model.sink_0_values().size() != 3 ||
      model.sink_0_values()[0].entry.result.attempt != value.attempt ||
      model.sink_0_values()[1].entry.result.attempt != store.attempt ||
      model.sink_0_values()[2].entry.result.attempt != branch.attempt)
    return 7;

  const auto branchCommit = model.sink_0_values()[2];
  const auto valueCommit = model.sink_0_values()[0];
  WritebackApplyAck retry{};
  retry.attempt = branch.attempt;
  retry.slot = branchCommit.entry.slot;
  retry.generation = branchCommit.entry.generation;
  retry.effect_mask = branchCommit.effect_mask;
  retry.retry = gfsim::UInt<1>{1};
  retry.valid = gfsim::UInt<1>{1};
  if (!offer(model.apply_ack(), retry, tick)) return 6;
  cycle(rows, tick, {true, false, true, true});
  cycle(rows, tick, {true, false, true, true});
  auto &applyAckQueue = objectByName<gfsim::SimQueue<WritebackAckResult>>(
      rows, "commit__ack_result__cancel_ack__drain_ack_1",
      gfsim::ObjectKind::Queue);
  if (!model.sink_1_values().empty() || applyAckQueue.committedSize() != 1 ||
      !static_cast<bool>(applyAckQueue.committedValues().front().matched))
    return 31;
  cycle(rows, tick);
  cycle(rows, tick);
  if (model.sink_1_values().empty() ||
      !static_cast<bool>(model.sink_1_values().back().matched) ||
      static_cast<bool>(model.sink_1_values().back().cancel_completed) ||
      model.sink_0_values().size() != 4)
    return 7;

  WritebackApplyAck applied{};
  applied.attempt = value.attempt;
  applied.slot = valueCommit.entry.slot;
  applied.generation = valueCommit.entry.generation;
  applied.effect_mask = valueCommit.effect_mask;
  applied.applied = gfsim::UInt<1>{1};
  applied.valid = gfsim::UInt<1>{1};
  if (!offer(model.apply_ack(), applied, tick)) return 8;
  cycle(rows, tick);
  cycle(rows, tick);
  if (!static_cast<bool>(model.sink_1_values().back().matched)) return 9;

  WbaDrainRequest drain{};
  drain.attempt = value.attempt;
  drain.wba_identity_valid = gfsim::UInt<1>{1};
  drain.wba_slot = valueCommit.entry.slot;
  drain.wba_generation = valueCommit.entry.generation;
  drain.effect_mask = valueCommit.effect_mask;
  drain.producer_drained = gfsim::UInt<1>{1};
  drain.valid = gfsim::UInt<1>{1};
  if (!offer(model.drain_request(), drain, tick)) return 10;
  cycle(rows, tick, {true, true, true, false});
  cycle(rows, tick, {true, true, true, false});
  auto &drainAckQueue = objectByName<gfsim::SimQueue<WbaDrainAck>>(
      rows, "commit__ack_result__cancel_ack__drain_ack_3",
      gfsim::ObjectKind::Queue);
  if (!model.sink_3_values().empty() || drainAckQueue.committedSize() != 1 ||
      validCount(entries) != 2)
    return 32;
  cycle(rows, tick);
  const auto &drainAck = model.sink_3_values().back();
  if (!static_cast<bool>(drainAck.accepted) ||
      !static_cast<bool>(drainAck.entry_reclaimed) || validCount(entries) != 2)
    return 11;

  // Reset clears retained entries, queues, and sink history while another
  // instance remains independently empty.
  model.reset();
  if (validCount(entries) != 0 || validCount(tombstones) != 0 ||
      !model.sink_0_values().empty() || !model.sink_1_values().empty() ||
      !model.apply_ack().isEmpty() || validCount(isolatedEntries) != 0)
    return 12;

  // The fused target/completed scan keeps independent masks. A completed row
  // blocks cancellation even when an unpublished row has the same attempt.
  WbaSystem mixedMatch;
  auto mixedRows = mixedMatch.dispatch_rows();
  auto &mixedEntries = objectByName<gfsim::SimTable<WbaEntry>>(
      mixedRows, "entries", gfsim::ObjectKind::Memory);
  auto &mixedTombstones = objectByName<gfsim::SimTable<AttemptTombstone>>(
      mixedRows, "cancel_tombstones", gfsim::ObjectKind::Memory);
  auto mixedResult = makeValue(60, 60);
  WbaEntry pending{};
  pending.result = mixedResult;
  pending.canceled = gfsim::UInt<1>{1};
  pending.valid = gfsim::UInt<1>{1};
  WbaEntry completed = pending;
  completed.completed = gfsim::UInt<1>{1};
  if (!mixedEntries.initializeEntry(1, pending) ||
      !mixedEntries.initializeEntry(6, completed))
    return 50;
  IssueCancel mixedCancel{mixedResult.attempt, IssueCancelReason::RECOVERY,
                          gfsim::UInt<1>{1}};
  std::uint64_t mixedTick = 0;
  if (!offer(mixedMatch.cancel(), mixedCancel, mixedTick)) return 51;
  for (int i = 0; i < 3; ++i)
    cycle(mixedRows, mixedTick, {false, true, true, true});
  if (mixedMatch.sink_2_values().size() != 1 ||
      static_cast<bool>(mixedMatch.sink_2_values()[0].accepted) ||
      !static_cast<bool>(mixedMatch.sink_2_values()[0].already_completed) ||
      validCount(mixedEntries) != 2 || validCount(mixedTombstones) != 0)
    return 52;

  // Duplicate unpublished rows preserve low-index first selection and leave
  // the second candidate untouched.
  WbaSystem duplicateMatch;
  auto duplicateRows = duplicateMatch.dispatch_rows();
  auto &duplicateEntries = objectByName<gfsim::SimTable<WbaEntry>>(
      duplicateRows, "entries", gfsim::ObjectKind::Memory);
  auto &duplicateTombstones = objectByName<gfsim::SimTable<AttemptTombstone>>(
      duplicateRows, "cancel_tombstones", gfsim::ObjectKind::Memory);
  WbaEntry publishedPending = pending;
  publishedPending.published = gfsim::UInt<1>{1};
  if (!duplicateEntries.initializeEntry(1, pending) ||
      !duplicateEntries.initializeEntry(6, publishedPending))
    return 53;
  std::uint64_t duplicateTick = 0;
  if (!offer(duplicateMatch.cancel(), mixedCancel, duplicateTick)) return 54;
  for (int i = 0; i < 3; ++i)
    cycle(duplicateRows, duplicateTick, {false, true, true, true});
  if (duplicateMatch.sink_2_values().size() != 1) return 55;
  if (!static_cast<bool>(duplicateMatch.sink_2_values()[0].accepted)) return 56;
  if (!static_cast<bool>(
          duplicateMatch.sink_2_values()[0].unpublished_cleared) ||
      static_cast<bool>(duplicateMatch.sink_2_values()[0].apply_owned))
    return 57;
  if (!static_cast<bool>(duplicateEntries.at(6).valid)) return 58;
  if (validCount(duplicateTombstones) != 1) return 59;

  // Early cancel installs a tombstone. A later matching terminal result stays
  // at the source boundary until a producer-drained request reclaims it.
  IssueCancel cancel{};
  cancel.key = makeKey(7, ExecutionClass::ALU);
  cancel.reason = IssueCancelReason::RECOVERY;
  cancel.valid = gfsim::UInt<1>{1};
  if (!offer(model.cancel(), cancel, tick)) return 13;
  cycle(rows, tick, {true, true, false, true});
  cycle(rows, tick, {true, true, false, true});
  auto &cancelAckQueue = objectByName<gfsim::SimQueue<WritebackCancelAck>>(
      rows, "commit__ack_result__cancel_ack__drain_ack_2",
      gfsim::ObjectKind::Queue);
  if (!model.sink_2_values().empty() || cancelAckQueue.committedSize() != 1 ||
      validCount(tombstones) != 1)
    return 33;
  cycle(rows, tick);
  if (model.sink_2_values().empty() ||
      !static_cast<bool>(model.sink_2_values().back().accepted) ||
      !static_cast<bool>(model.sink_2_values().back().tombstoned) ||
      validCount(tombstones) != 1)
    return 14;
  auto late = makeValue(7, 7);
  if (!offerLane(model, 0, late, tick)) return 15;
  for (int i = 0; i < 3; ++i) cycle(rows, tick);
  if (model.alu_result().committedSize() != 1 || validCount(entries) != 0)
    return 16;

  WbaDrainRequest cancelDrain{};
  cancelDrain.attempt = cancel.key;
  cancelDrain.producer_drained = gfsim::UInt<1>{1};
  cancelDrain.valid = gfsim::UInt<1>{1};
  if (!offer(model.drain_request(), cancelDrain, tick)) return 17;
  cycle(rows, tick, {true, true, true, false});
  cycle(rows, tick, {true, true, true, false});
  if (!model.sink_3_values().empty() ||
      drainAckQueue.committedSize() != 1 || validCount(tombstones) != 0)
    return 34;
  cycle(rows, tick);
  if (model.sink_3_values().size() != 1 ||
      !static_cast<bool>(model.sink_3_values().back().accepted) ||
      !static_cast<bool>(
          model.sink_3_values().back().cancel_tombstone_reclaimed) ||
      validCount(tombstones) != 0)
    return 18;
  for (int i = 0; i < 4; ++i) cycle(rows, tick);
  if (!model.alu_result().isEmpty() || model.sink_0_values().empty())
    return 19;

  // A queued commit keeps a later entry unpublished so cancellation can clear
  // it without publishing. A published cancellation transfers ownership to
  // apply/retry, and a later cancel classifies the completed attempt without
  // creating another tombstone.
  WbaSystem cancelPaths;
  auto cancelRows = cancelPaths.dispatch_rows();
  std::uint64_t cancelTick = 0;
  auto blocker = makeValue(10, 10);
  auto unpublished = makeValue(11, 11);
  if (!offerLane(cancelPaths, 0, blocker, cancelTick)) return 35;
  cycle(cancelRows, cancelTick, {false, true, true, true});
  cycle(cancelRows, cancelTick, {false, true, true, true});
  if (!offerLane(cancelPaths, 0, unpublished, cancelTick)) return 36;
  cycle(cancelRows, cancelTick, {false, true, true, true});
  IssueCancel unpublishedCancel{unpublished.attempt,
                                IssueCancelReason::RECOVERY,
                                gfsim::UInt<1>{1}};
  if (!offer(cancelPaths.cancel(), unpublishedCancel, cancelTick)) return 37;
  cycle(cancelRows, cancelTick, {false, true, true, true});
  cycle(cancelRows, cancelTick, {false, true, true, true});
  if (cancelPaths.sink_2_values().size() != 1 ||
      !static_cast<bool>(cancelPaths.sink_2_values()[0].accepted) ||
      !static_cast<bool>(cancelPaths.sink_2_values()[0].unpublished_cleared) ||
      static_cast<bool>(cancelPaths.sink_2_values()[0].apply_owned) ||
      static_cast<bool>(cancelPaths.sink_2_values()[0].already_completed))
    return 38;
  for (int i = 0; i < 5; ++i) cycle(cancelRows, cancelTick);
  if (cancelPaths.sink_0_values().size() != 1 ||
      cancelPaths.sink_0_values()[0].entry.result.attempt != blocker.attempt)
    return 39;

  WbaSystem applyOwned;
  auto applyRows = applyOwned.dispatch_rows();
  std::uint64_t applyTick = 0;
  auto published = makeValue(12, 12);
  if (!offerLane(applyOwned, 0, published, applyTick)) return 40;
  for (int i = 0; i < 4; ++i) cycle(applyRows, applyTick);
  const auto publishedCommit = applyOwned.sink_0_values().front();
  IssueCancel publishedCancel{published.attempt, IssueCancelReason::RECOVERY,
                              gfsim::UInt<1>{1}};
  if (!offer(applyOwned.cancel(), publishedCancel, applyTick)) return 41;
  cycle(applyRows, applyTick);
  cycle(applyRows, applyTick);
  if (applyOwned.sink_2_values().size() != 1 ||
      !static_cast<bool>(applyOwned.sink_2_values()[0].accepted) ||
      !static_cast<bool>(applyOwned.sink_2_values()[0].apply_owned) ||
      static_cast<bool>(applyOwned.sink_2_values()[0].unpublished_cleared))
    return 42;
  WritebackApplyAck cancelRetry{};
  cancelRetry.attempt = published.attempt;
  cancelRetry.slot = publishedCommit.entry.slot;
  cancelRetry.generation = publishedCommit.entry.generation;
  cancelRetry.effect_mask = publishedCommit.effect_mask;
  cancelRetry.retry = gfsim::UInt<1>{1};
  cancelRetry.valid = gfsim::UInt<1>{1};
  if (!offer(applyOwned.apply_ack(), cancelRetry, applyTick)) return 43;
  cycle(applyRows, applyTick);
  cycle(applyRows, applyTick);
  if (applyOwned.sink_1_values().size() != 1 ||
      !static_cast<bool>(applyOwned.sink_1_values()[0].cancel_completed))
    return 44;
  if (!offer(applyOwned.cancel(), publishedCancel, applyTick)) return 45;
  cycle(applyRows, applyTick);
  cycle(applyRows, applyTick);
  if (applyOwned.sink_2_values().size() != 2 ||
      static_cast<bool>(applyOwned.sink_2_values()[1].accepted) ||
      !static_cast<bool>(applyOwned.sink_2_values()[1].already_completed))
    return 46;

  // Invalid VALUE effect shape is rejected without consuming the source.
  WbaSystem invalid;
  auto invalidRows = invalid.dispatch_rows();
  std::uint64_t invalidTick = 0;
  auto malformed = makeValue(9, 9);
  malformed.required_effect_mask = gfsim::UInt<7>{0b0000100};
  if (!offerLane(invalid, 0, malformed, invalidTick)) return 20;
  for (int i = 0; i < 3; ++i) cycle(invalidRows, invalidTick);
  if (invalid.alu_result().committedSize() != 1 ||
      !invalid.sink_0_values().empty())
    return 21;
  return 0;
}

""",
            encoding="utf-8",
        )
        compiled = subprocess.run(
            (
                compiler,
                "-std=c++20",
                "-I",
                str(ROOT / "simulator/gfsim/include"),
                str(source),
                "-o",
                str(executable),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert compiled.returncode == 0, compiled.stderr
        executed = subprocess.run(
            (str(executable),), text=True, capture_output=True, check=False
        )
        assert (
            executed.returncode == 0
        ), f"exit={executed.returncode}\nstdout:\n{executed.stdout}\nstderr:\n{executed.stderr}"
