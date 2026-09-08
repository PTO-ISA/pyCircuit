from __future__ import annotations

import ast
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[5]
DEFAULT_ACIR = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"
CONTRACTS = ROOT / "designs/davincioo/contracts/spe.py"
I1_SOURCE = ROOT / "designs/davincioo/spe/iex/i1.py"


def _lower_i1() -> str:
    from agentic_circuit._queue_frontend import lower_queue_source

    statements: list[ast.stmt] = []
    for path in (CONTRACTS, I1_SOURCE):
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        statements.extend(
            statement
            for statement in tree.body
            if not isinstance(statement, ast.Import | ast.ImportFrom)
        )
    statements.extend(
        ast.parse(
            """
@ac.system
def i1_system(
    selected: IssueAttempt,
    cancel: IssueCancel,
    read_decision: OperandReadDecision,
) -> tuple[OperandReadRequest, OperandReadDecisionAck, IssueCancelAck]:
    read_request, read_decision_ack, cancel_ack = i1(
        selected, cancel, read_decision
    )
    return read_request, read_decision_ack, cancel_ack
"""
        ).body
    )
    source = ast.unparse(ast.fix_missing_locations(ast.Module(statements, [])))
    return lower_queue_source(source, "i1_system")


def test_i1_uses_canonical_identity_and_operand_invariant() -> None:
    source = I1_SOURCE.read_text(encoding="utf-8")
    tree = ast.parse(source)
    comparisons = [node for node in ast.walk(tree) if isinstance(node, ast.Compare)]

    assert len(comparisons) == 9
    assert "selected.key.identity == selected.entry.identity" in source
    assert "pending.key == request.key" in source
    assert source.count("valid_operand_source(") == 2
    assert ".core_id ==" not in source
    assert ".attempt_generation ==" not in source
    assert "OperandReadResponse" not in source
    assert "isq_release" not in source


def test_i1_source_closure_owns_only_contract_and_leaf_sources() -> None:
    from agentic_circuit._source_closure import capture_source_closure

    closure = capture_source_closure(I1_SOURCE, ROOT)
    assert tuple(entry.path for entry in closure.entries) == (
        "designs/davincioo/contracts/spe.py",
        "designs/davincioo/spe/iex/i1.py",
    )


def test_i1_lowers_value_contracts_and_atomic_state_rules() -> None:
    from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

    optimizer = Path(os.environ.get("ACIR_OPT", DEFAULT_ACIR))
    if not optimizer.is_file():
        pytest.skip(f"current-checkout acir-opt is unavailable: {optimizer}")
    raw = _lower_i1()
    assert 'name "OperandSourceDescriptor.valid_operand_source"' in raw
    assert 'ac.var.cmp "eq"' in raw
    assert "!ac.var<!ac.struct<@types::@IssueIdentity>>" in raw
    assert "!ac.var<!ac.struct<@types::@IssueAttemptKey>>" in raw
    assert raw.count("ac.rule ") == 4
    assert "ac.var.assign @active" in raw
    assert "ac.var.assign @pending" in raw
    assert "ac.var.assign @rf_read_mask" in raw
    assert "ac.var.assign @forward_mask" in raw
    assert "ac.var.assign @read_request_outstanding" in raw
    assert raw.index('name "accept_read_decision"') < raw.index('name "cancel_attempt"')

    with tempfile.TemporaryDirectory(prefix="davincioo-i1-") as directory:
        raw_path = Path(directory) / "raw.mlir"
        raw_path.write_text(raw, encoding="utf-8")
        lowered = subprocess.run(
            (
                str(optimizer),
                f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                str(raw_path),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
    assert lowered.returncode == 0, lowered.stderr
    assert "ac.var.invariant" not in lowered.stdout
    assert lowered.stdout.count("ac.firing") >= 4
    assert "ac.output_presence" in lowered.stdout


def test_i1_generates_current_checkout_gfsim_model() -> None:
    from agentic_circuit._jit import _lower_acir_to_cpp

    optimizer = Path(os.environ.get("ACIR_OPT", DEFAULT_ACIR))
    cxx = shutil.which("c++")
    if (
        cxx is None
        or not optimizer.is_file()
        or not optimizer.with_name("acir-queue-cxxgen").is_file()
    ):
        pytest.skip("current-checkout ACIR/gfsim toolchain is unavailable")
    generated = _lower_acir_to_cpp(_lower_i1())

    assert "class I1" in generated or "class i1" in generated
    assert "read_request_outstanding" in generated
    assert "OperandReadDecisionAck" in generated
    assert "IssueCancelAck" in generated
    with tempfile.TemporaryDirectory(prefix="davincioo-i1-gfsim-") as directory:
        source = Path(directory) / "model.cpp"
        executable = Path(directory) / "model"
        source.write_text(
            generated
            + r"""
using namespace ac_generated;

IssueAttempt make_attempt(unsigned sequence, unsigned attempt_generation) {
  FlowKey flow;
  flow.core_id = 1;
  flow.pe_id = 2;
  flow.stid = 3;
  flow.launch_generation = 4;

  IssueIdentity identity;
  identity.epoch = EpochKey{flow, gfsim::UInt<16>{5}};
  identity.inst = InstKey{
      flow, gfsim::UInt<32>{sequence}, gfsim::UInt<64>{0x1000 + sequence}};
  identity.block = BlockKey{
      flow, gfsim::UInt<32>{sequence / 2}, gfsim::UInt<8>{1},
      gfsim::UInt<16>{6}};
  identity.rob = RobKey{flow, gfsim::UInt<4>{2}, gfsim::UInt<16>{7}};
  identity.dispatch = DispatchReservation{
      flow, ExecutionClass::ALU, gfsim::UInt<4>{0}, gfsim::UInt<6>{3},
      gfsim::UInt<16>{8}, gfsim::UInt<1>{1}};
  identity.isq_index = 4;

  OperandSourceDescriptor src0;
  src0.architectural_index = 3;
  src0.tag = 9;
  src0.generation = 10;
  src0.valid = 1;

  OperandSourceDescriptor src1;
  src1.constant_zero = 1;

  IssueEntry entry;
  entry.identity = identity;
  entry.age = 11;
  entry.operation = IntAluOperation::ADD;
  entry.src0 = src0;
  entry.src0_ready = 0;
  entry.src1 = src1;
  entry.src1_ready = 1;
  entry.destination = PhysRef{
      gfsim::UInt<7>{12}, gfsim::UInt<16>{13}, gfsim::UInt<1>{1}};
  entry.valid = 1;

  return IssueAttempt{
      IssueAttemptKey{identity, gfsim::UInt<16>{attempt_generation}}, entry,
      gfsim::UInt<1>{1}};
}

template <size_t N>
void drive(std::array<gfsim::DispatchRow, N> &rows, unsigned &tick,
           unsigned count, const char *stalled_sink = nullptr) {
  while (count-- != 0) {
    const gfsim::Epoch epoch{++tick, 0};
    for (auto &row : rows) {
      auto *object = static_cast<gfsim::SimObject *>(row.object);
      if (stalled_sink != nullptr && object->name() == stalled_sink) continue;
      row.work(row.object, epoch);
    }
    for (auto phase : {gfsim::XferPhase::Arbitrate,
                       gfsim::XferPhase::Probe,
                       gfsim::XferPhase::Commit})
      for (auto &row : rows) row.xfer(row.object, epoch, phase);
  }
}

template <typename T>
bool inject(gfsim::SimQueue<T> &queue, T value, unsigned tick) {
  if (!queue.proposePush(std::move(value))) return false;
  queue.doXfer({tick, 0});
  return true;
}

int main() {
  I1System model;
  I1System isolated;
  auto rows = model.dispatch_rows();
  unsigned tick = 0;
  auto cycle = [&](const char *stalled_sink = nullptr) {
    const gfsim::Epoch epoch{++tick, 0};
    for (auto &row : rows) {
      auto *object = static_cast<gfsim::SimObject *>(row.object);
      if (stalled_sink != nullptr && object->name() == stalled_sink) continue;
      row.work(row.object, epoch);
    }
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
    for (auto &row : rows)
      row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  };
  auto offer = [&](auto &queue, auto value) {
    if (!queue.proposePush(std::move(value))) return false;
    queue.doXfer({tick, 0});
    return true;
  };
  auto run = [&](unsigned count, const char *stalled_sink = nullptr) {
    while (count-- != 0) cycle(stalled_sink);
  };

  const IssueAttempt attempt_a = make_attempt(20, 30);
  if (!offer(model.selected(), attempt_a)) return 1;
  run(5);
  if (!model.selected().isEmpty()) return 2;
  if (model.selected().totalPops() != 1) return 30;
  if (model.sink_0_values().size() != 1) return 31;
  const auto &request_a = model.sink_0_values().at(0);
  if (!(request_a.attempt == attempt_a) || !(request_a.src0 == attempt_a.entry.src0) ||
      !(request_a.src1 == attempt_a.entry.src1) || request_a.rf_read_mask != 1 ||
      request_a.forward_mask != 0 || !request_a.valid) return 3;
  run(4);
  if (model.sink_0_values().size() != 1) return 4;

  // Fill the decision-ack output with a rejected stale response, then prove
  // the exact denial remains at the input until the sink is released.
  auto stale_key = attempt_a.key;
  stale_key.attempt_generation = 31;
  const OperandReadDecision stale{
      stale_key, gfsim::UInt<1>{1}, gfsim::UInt<2>{1},
      gfsim::UInt<2>{0}, gfsim::UInt<1>{1}};
  if (!offer(model.read_decision(), stale)) return 5;
  run(4, "sink_sink_1");
  if (!model.read_decision().isEmpty() ||
      model.read_decision().totalPops() != 1 ||
      !model.sink_1_values().empty()) return 6;

  const OperandReadDecision denied{
      attempt_a.key, gfsim::UInt<1>{0}, gfsim::UInt<2>{0},
      gfsim::UInt<2>{0}, gfsim::UInt<1>{1}};
  if (!offer(model.read_decision(), denied)) return 7;
  run(4, "sink_sink_1");
  if (model.read_decision().isEmpty() ||
      model.read_decision().totalPops() != 1 ||
      !model.sink_1_values().empty() ||
      model.sink_0_values().size() != 1) return 8;

  run(7);
  if (!model.read_decision().isEmpty() ||
      model.read_decision().totalPops() != 2 ||
      model.sink_1_values().size() != 2 ||
      model.sink_0_values().size() != 2) return 9;
  const auto &stale_ack = model.sink_1_values().at(0);
  const auto &denied_ack = model.sink_1_values().at(1);
  if (!(stale_ack.request == stale) || stale_ack.accepted ||
      stale_ack.transferred || stale_ack.retry || !stale_ack.valid) return 10;
  if (!(denied_ack.request == denied) || !denied_ack.accepted ||
      denied_ack.transferred || !denied_ack.retry || !denied_ack.valid) return 11;
  if (!(model.sink_0_values().at(1).attempt == attempt_a)) return 12;

  const OperandReadDecision granted{
      attempt_a.key, gfsim::UInt<1>{1}, gfsim::UInt<2>{1},
      gfsim::UInt<2>{0}, gfsim::UInt<1>{1}};
  if (!offer(model.read_decision(), granted)) return 13;
  run(6);
  if (model.read_decision().totalPops() != 3 ||
      model.sink_1_values().size() != 3) return 14;
  const auto &granted_ack = model.sink_1_values().at(2);
  if (!(granted_ack.request == granted) || !granted_ack.accepted ||
      !granted_ack.transferred || granted_ack.retry || !granted_ack.valid)
    return 15;
  run(4);
  if (model.sink_0_values().size() != 2 ||
      model.sink_1_values().size() != 3) return 16;

  // A cancel for the transferred attempt is rejected, which observes that
  // the successful grant cleared the resident state.
  const IssueCancel stale_cancel{
      attempt_a.key, IssueCancelReason::STALE_ATTEMPT, gfsim::UInt<1>{1}};
  if (!offer(model.cancel(), stale_cancel)) return 17;
  run(5);
  if (model.cancel().totalPops() != 1 || model.sink_2_values().size() != 1 ||
      model.sink_2_values().at(0).accepted ||
      !(model.sink_2_values().at(0).request == stale_cancel)) return 18;

  const IssueAttempt attempt_b = make_attempt(21, 40);
  if (!offer(model.selected(), attempt_b)) return 19;
  run(5);
  if (model.selected().totalPops() != 2 ||
      model.sink_0_values().size() != 3 ||
      !(model.sink_0_values().at(2).attempt == attempt_b)) return 20;
  const IssueCancel exact_cancel{
      attempt_b.key, IssueCancelReason::RECOVERY, gfsim::UInt<1>{1}};
  if (!offer(model.cancel(), exact_cancel)) return 21;
  run(5);
  if (model.cancel().totalPops() != 2 || model.sink_2_values().size() != 2)
    return 22;
  const auto &cancel_ack = model.sink_2_values().at(1);
  if (!(cancel_ack.request == exact_cancel) || !cancel_ack.accepted ||
      !cancel_ack.valid) return 23;
  run(4);
  if (model.sink_0_values().size() != 3 ||
      model.sink_2_values().size() != 2) return 24;

  // A separately constructed model has no shared queues, state, or history.
  if (!isolated.selected().isEmpty() || !isolated.cancel().isEmpty() ||
      !isolated.read_decision().isEmpty() ||
      !isolated.sink_0_values().empty() || !isolated.sink_1_values().empty() ||
      !isolated.sink_2_values().empty()) return 25;

  const IssueAttempt reset_attempt = make_attempt(22, 50);
  if (!offer(model.selected(), reset_attempt)) return 26;
  run(5);
  if (model.sink_0_values().size() != 4) return 27;
  for (auto &row : rows) row.reset(row.object);
  if (!model.selected().isEmpty() || !model.cancel().isEmpty() ||
      !model.read_decision().isEmpty() ||
      !model.sink_0_values().empty() || !model.sink_1_values().empty() ||
      !model.sink_2_values().empty()) return 28;
  run(5);
  if (!model.sink_0_values().empty() || !model.sink_1_values().empty() ||
      !model.sink_2_values().empty()) return 29;

  // A stalled read-request sink retains the first request token. A second
  // resident attempt cannot publish until that exact token is drained.
  {
    I1System read_blocked;
    auto blocked_rows = read_blocked.dispatch_rows();
    unsigned blocked_tick = 0;

    const auto first = make_attempt(40, 60);
    const auto second = make_attempt(41, 61);
    if (!inject(read_blocked.selected(), first, blocked_tick)) return 32;
    drive(blocked_rows, blocked_tick, 5, "sink_sink_0");
    if (!read_blocked.selected().isEmpty() ||
        read_blocked.selected().totalPops() != 1 ||
        !read_blocked.sink_0_values().empty()) return 33;
    const OperandReadDecision first_grant{
        first.key, gfsim::UInt<1>{1}, gfsim::UInt<2>{1},
        gfsim::UInt<2>{0}, gfsim::UInt<1>{1}};
    if (!inject(read_blocked.read_decision(), first_grant, blocked_tick)) return 34;
    drive(blocked_rows, blocked_tick, 5, "sink_sink_0");
    if (read_blocked.read_decision().totalPops() != 1 ||
        read_blocked.sink_1_values().size() != 1 ||
        !read_blocked.sink_1_values()[0].transferred ||
        !read_blocked.sink_0_values().empty()) return 35;
    if (!inject(read_blocked.selected(), second, blocked_tick)) return 36;
    drive(blocked_rows, blocked_tick, 5, "sink_sink_0");
    if (!read_blocked.selected().isEmpty() ||
        read_blocked.selected().totalPops() != 2 ||
        !read_blocked.sink_0_values().empty()) return 37;
    drive(blocked_rows, blocked_tick, 8);
    if (read_blocked.sink_0_values().size() != 2 ||
        !(read_blocked.sink_0_values()[0].attempt == first) ||
        !(read_blocked.sink_0_values()[1].attempt == second)) return 38;
    drive(blocked_rows, blocked_tick, 5);
    if (read_blocked.sink_0_values().size() != 2 ||
        read_blocked.selected().totalPops() != 2 ||
        read_blocked.read_decision().totalPops() != 1) return 39;
  }

  // A full cancel-ack output retains the second exact cancel input and its
  // resident attempt. Releasing the sink commits that cancellation once.
  {
    I1System cancel_blocked;
    auto blocked_rows = cancel_blocked.dispatch_rows();
    unsigned blocked_tick = 0;

    const auto first = make_attempt(50, 70);
    const auto second = make_attempt(51, 71);
    if (!inject(cancel_blocked.selected(), first, blocked_tick)) return 40;
    drive(blocked_rows, blocked_tick, 5);
    const IssueCancel first_cancel{
        first.key, IssueCancelReason::RECOVERY, gfsim::UInt<1>{1}};
    if (!inject(cancel_blocked.cancel(), first_cancel, blocked_tick)) return 41;
    drive(blocked_rows, blocked_tick, 5, "sink_sink_2");
    if (cancel_blocked.cancel().totalPops() != 1 ||
        !cancel_blocked.cancel().isEmpty() ||
        !cancel_blocked.sink_2_values().empty()) return 42;
    if (!inject(cancel_blocked.selected(), second, blocked_tick)) return 43;
    drive(blocked_rows, blocked_tick, 5, "sink_sink_2");
    const IssueCancel second_cancel{
        second.key, IssueCancelReason::RECOVERY, gfsim::UInt<1>{1}};
    if (!inject(cancel_blocked.cancel(), second_cancel, blocked_tick)) return 44;
    drive(blocked_rows, blocked_tick, 5, "sink_sink_2");
    if (cancel_blocked.cancel().isEmpty() ||
        cancel_blocked.cancel().totalPops() != 1 ||
        !cancel_blocked.sink_2_values().empty()) return 45;
    drive(blocked_rows, blocked_tick, 10);
    if (!cancel_blocked.cancel().isEmpty() ||
        cancel_blocked.cancel().totalPops() != 2 ||
        cancel_blocked.sink_2_values().size() != 2 ||
        !(cancel_blocked.sink_2_values()[0].request == first_cancel) ||
        !(cancel_blocked.sink_2_values()[1].request == second_cancel) ||
        !cancel_blocked.sink_2_values()[0].accepted ||
        !cancel_blocked.sink_2_values()[1].accepted) return 46;
    drive(blocked_rows, blocked_tick, 5);
    if (cancel_blocked.sink_2_values().size() != 2) return 47;
  }

  // Exact grant and cancel offered at one epoch contend for the same resident
  // state. Source-order priority transfers first; cancel drains once as stale.
  {
    I1System competing;
    auto competing_rows = competing.dispatch_rows();
    unsigned competing_tick = 0;

    const auto attempt = make_attempt(60, 80);
    if (!inject(competing.selected(), attempt, competing_tick)) return 48;
    drive(competing_rows, competing_tick, 5);
    const OperandReadDecision grant{
        attempt.key, gfsim::UInt<1>{1}, gfsim::UInt<2>{1},
        gfsim::UInt<2>{0}, gfsim::UInt<1>{1}};
    const IssueCancel cancel{
        attempt.key, IssueCancelReason::RECOVERY, gfsim::UInt<1>{1}};
    if (!inject(competing.read_decision(), grant, competing_tick) ||
        !inject(competing.cancel(), cancel, competing_tick)) return 49;
    drive(competing_rows, competing_tick, 8);
    if (!competing.read_decision().isEmpty() || !competing.cancel().isEmpty() ||
        competing.read_decision().totalPops() != 1 ||
        competing.cancel().totalPops() != 1 ||
        competing.sink_1_values().size() != 1 ||
        competing.sink_2_values().size() != 1) return 50;
    const auto &grant_ack = competing.sink_1_values()[0];
    const auto &cancel_ack = competing.sink_2_values()[0];
    if (!grant_ack.accepted || !grant_ack.transferred || grant_ack.retry ||
        cancel_ack.accepted || !(grant_ack.request == grant) ||
        !(cancel_ack.request == cancel)) return 51;
    drive(competing_rows, competing_tick, 5);
    if (competing.sink_0_values().size() != 1 ||
        competing.sink_1_values().size() != 1 ||
        competing.sink_2_values().size() != 1) return 52;
  }
  return 0;
}
""",
            encoding="utf-8",
        )
        compiled = subprocess.run(
            (
                cxx,
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
        assert executed.returncode == 0, (
            f"exit={executed.returncode}\nstdout:\n{executed.stdout}\n"
            f"stderr:\n{executed.stderr}"
        )
