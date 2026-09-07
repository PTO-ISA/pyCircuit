from __future__ import annotations

import ast
import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.spe.iex.i2 import i2_system

ROOT = Path(__file__).resolve().parents[5]
DEFAULT_ACIR = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"
DEFAULT_PLAN = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan"
DEFAULT_CXXGEN = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen"
I2_SOURCE = ROOT / "designs/davincioo/spe/iex/i2.py"


def test_i2_uses_shared_value_contracts_and_stays_below_comparison_budget() -> None:
    tree = ast.parse(I2_SOURCE.read_text(encoding="utf-8"))
    comparisons = [node for node in ast.walk(tree) if isinstance(node, ast.Compare)]
    assert len(comparisons) <= 80

    source = I2_SOURCE.read_text(encoding="utf-8")
    assert "row.key == key" in source
    assert "pending.key == request.consumer" in source
    assert "source == request.producer" in source
    assert "valid_operand_source(entry.src0)" in source
    assert "was_inactive = not active" in source
    assert "if was_inactive:" in source
    assert "pending_key.flow.core_id" not in source
    assert "load_attempt_generation ==" not in source


def test_i2_lowers_value_contracts_before_frozen_acir() -> None:
    from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

    optimizer = Path(os.environ.get("ACIR_OPT", DEFAULT_ACIR))
    planner = Path(os.environ.get("ACIR_QUEUE_PLAN", DEFAULT_PLAN))
    cxxgen = Path(os.environ.get("ACIR_QUEUE_CXXGEN", DEFAULT_CXXGEN))
    cxx = shutil.which("c++")
    if any(not path.is_file() for path in (optimizer, planner, cxxgen)) or cxx is None:
        pytest.skip("current-checkout ACIR/gfsim toolchain is unavailable")
    specialization = ac.jit(i2_system, workspace=ROOT)
    raw = specialization.lower_acir()
    assert "ac.var.invariant" in raw
    assert 'name "OperandSourceDescriptor.valid_operand_source"' in raw
    assert 'ac.var.cmp "eq"' in raw
    assert raw.count("ac.rule ") == 7
    with tempfile.TemporaryDirectory(prefix="davincioo-i2-") as directory:
        source = Path(directory) / "raw.mlir"
        frozen = Path(directory) / "frozen.mlir"
        generated = Path(directory) / "i2.cpp"
        executable = Path(directory) / "i2"
        source.write_text(raw, encoding="utf-8")
        lowered = subprocess.run(
            (
                str(optimizer),
                f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                str(source),
                "-o",
                str(frozen),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert lowered.returncode == 0, lowered.stderr
        frozen_text = frozen.read_text(encoding="utf-8")
        assert "ac.var.invariant" not in frozen_text
        assert "unresolved aggregate comparison" not in lowered.stderr
        planned = subprocess.run(
            (str(planner), str(frozen)),
            text=True,
            capture_output=True,
            check=False,
        )
        assert planned.returncode == 0, planned.stderr
        plan = json.loads(planned.stdout)
        assert len(plan["module_specializations"]) == 1
        assert len(plan["module_specializations"][0]["blocks"]) == 7
        emitted = subprocess.run(
            (str(cxxgen), str(frozen)),
            text=True,
            capture_output=True,
            check=False,
        )
        assert emitted.returncode == 0, emitted.stderr
        generated.write_text(
            emitted.stdout
            + """
int main() {
  using namespace ac_generated;
  auto u = [](auto value) { return static_cast<unsigned long long>(value); };
  auto flow = [](unsigned core, unsigned pe = 0) {
    return FlowKey{gfsim::UInt<4>{core}, gfsim::UInt<2>{pe},
                   gfsim::UInt<4>{1}, gfsim::UInt<16>{1}};
  };
  auto key = [&](unsigned core, unsigned generation = 1, unsigned pe = 0) {
    auto f = flow(core, pe);
    auto epoch = EpochKey{f, gfsim::UInt<16>{2}};
    auto inst = InstKey{f, gfsim::UInt<32>{3}, gfsim::UInt<64>{0x100}};
    auto block = BlockKey{f, gfsim::UInt<32>{4}, gfsim::UInt<8>{5},
                          gfsim::UInt<16>{6}};
    auto rob = RobKey{f, gfsim::UInt<4>{7}, gfsim::UInt<16>{8}};
    auto dispatch = DispatchReservation{f, ExecutionClass::ALU,
                                        gfsim::UInt<4>{1}, gfsim::UInt<6>{2},
                                        gfsim::UInt<16>{9}, gfsim::UInt<1>{1}};
    return IssueAttemptKey{IssueIdentity{epoch, inst, block, rob, dispatch,
                                         gfsim::UInt<4>{3}},
                           gfsim::UInt<16>{generation}};
  };
  auto producer = [&](const IssueAttemptKey &k, unsigned tag = 12,
                      unsigned generation = 4) {
    const auto &i = k.identity;
    return LoadProducerToken{i.epoch, i.inst, i.block, i.rob,
                             gfsim::UInt<7>{tag},
                             gfsim::UInt<16>{generation},
                             gfsim::UInt<16>{5}, gfsim::UInt<1>{1}};
  };
  auto source = [&](const IssueAttemptKey &k, unsigned arch, bool speculative) {
    if (speculative)
      return OperandSourceDescriptor{
          gfsim::UInt<5>{arch}, gfsim::UInt<1>{0}, gfsim::UInt<7>{12},
          gfsim::UInt<16>{4}, gfsim::UInt<1>{1}, gfsim::UInt<1>{1},
          gfsim::UInt<4>{1}, producer(k)};
    return OperandSourceDescriptor{
        gfsim::UInt<5>{arch}, gfsim::UInt<1>{0},
        gfsim::UInt<7>{10 + arch}, gfsim::UInt<16>{4},
        gfsim::UInt<1>{1}, gfsim::UInt<1>{0}, gfsim::UInt<4>{0},
        LoadProducerToken{}};
  };
  auto response = [&](const IssueAttemptKey &k, unsigned speculative_mask = 0) {
    auto s0 = source(k, 1, (speculative_mask & 1) != 0);
    auto s1 = source(k, 2, (speculative_mask & 2) != 0);
    auto entry = IssueEntry{k.identity, gfsim::UInt<16>{10},
                            IntAluOperation::ADD, s0, gfsim::UInt<1>{1},
                            gfsim::UInt<64>{0}, s1, gfsim::UInt<1>{1},
                            gfsim::UInt<64>{0},
                            PhysRef{gfsim::UInt<7>{20},
                                    gfsim::UInt<16>{2}, gfsim::UInt<1>{1}},
                            gfsim::UInt<1>{1}};
    auto attempt = IssueAttempt{k, entry, gfsim::UInt<1>{1}};
    auto request = OperandReadRequest{attempt, s0, s1,
                                      gfsim::UInt<2>{3 & ~speculative_mask},
                                      gfsim::UInt<2>{speculative_mask},
                                      gfsim::UInt<1>{1}};
    return OperandReadResponse{request, gfsim::UInt<64>{11},
                               gfsim::UInt<64>{22},
                               (speculative_mask & 1)
                                   ? OperandSourceKind::LOAD_FORWARD
                                   : OperandSourceKind::REGISTER_FILE,
                               (speculative_mask & 2)
                                   ? OperandSourceKind::LOAD_FORWARD
                                   : OperandSourceKind::BYPASS,
                               gfsim::UInt<2>{3}, gfsim::UInt<2>{0},
                               gfsim::UInt<1>{1}};
  };
  auto inject = [](auto &queue, auto value, unsigned tick) {
    if (!queue.proposePush(std::move(value))) return false;
    queue.doXfer(gfsim::Epoch{tick, 0});
    return true;
  };
  auto cycle = [](I2System &model, unsigned &tick,
                  const char *stalled_sink = nullptr) {
    auto rows = model.dispatch_rows();
    const gfsim::Epoch epoch{++tick, 0};
    for (auto &row : rows) {
      auto *object = static_cast<gfsim::SimObject *>(row.object);
      if (stalled_sink == nullptr || object->name() != stalled_sink)
        row.work(row.object, epoch);
    }
    for (auto phase : {gfsim::XferPhase::Arbitrate, gfsim::XferPhase::Probe,
                       gfsim::XferPhase::Commit})
      for (size_t index = 0; index < rows.size(); ++index)
        rows[index].xfer(rows[index].object, epoch, phase);
  };
  auto run = [&](I2System &model, unsigned &tick, unsigned count = 8,
                 const char *stalled_sink = nullptr) {
    for (unsigned n = 0; n < count; ++n) cycle(model, tick, stalled_sink);
  };

  // Ordinary operands: exact request, sink retry, identical reissue, accept,
  // and release exactly once.
  {
    I2System model;
    unsigned tick = 0;
    auto k = key(1);
    if (!inject(model.operand_response(), response(k), tick++)) return 10;
    run(model, tick);
    if (model.sink_1_values().size() != 1) return 101;
    if (!u(model.sink_1_values()[0].accepted)) return 102;
    if (model.sink_0_values().size() != 1) return 103;
    if (u(model.sink_0_values()[0].lhs) != 11) return 104;
    if (u(model.sink_0_values()[0].rhs) != 22) return 105;
    if (!inject(model.sink_decision(),
                ExecutionSinkDecision{k, gfsim::UInt<1>{0},
                                      gfsim::UInt<1>{1}, gfsim::UInt<1>{1}},
                tick++)) return 12;
    run(model, tick);
    if (model.sink_3_values().size() != 1 ||
        !u(model.sink_3_values()[0].matched) ||
        !u(model.sink_3_values()[0].retry) ||
        model.sink_0_values().size() != 2) return 13;
    if (!inject(model.sink_decision(),
                ExecutionSinkDecision{k, gfsim::UInt<1>{1},
                                      gfsim::UInt<1>{0}, gfsim::UInt<1>{1}},
                tick++)) return 14;
    run(model, tick);
    if (model.sink_3_values().size() != 2 ||
        !u(model.sink_3_values()[1].transferred) ||
        model.sink_4_values().size() != 1 ||
        !(model.sink_4_values()[0].key == k) ||
        model.sink_0_values().size() != 2) return 15;
  }

  // A speculative source cannot execute until an exact producer hit arrives.
  {
    I2System model;
    unsigned tick = 0;
    auto k = key(2);
    auto rsp = response(k, true);
    if (!inject(model.operand_response(), rsp, tick++)) return 20;
    run(model, tick);
    if (!model.sink_0_values().empty()) return 21;
    auto hit = LoadDependencyResolution{k, gfsim::UInt<1>{0}, producer(k),
                                        gfsim::UInt<64>{77},
                                        gfsim::UInt<1>{1}, gfsim::UInt<1>{1},
                                        gfsim::UInt<1>{0}, gfsim::UInt<1>{0},
                                        gfsim::UInt<1>{1}};
    if (!inject(model.load_dependency(), hit, tick++)) return 22;
    run(model, tick);
    if (model.sink_2_values().size() != 1 ||
        !u(model.sink_2_values()[0].resolved) ||
        model.sink_0_values().size() != 1 ||
        u(model.sink_0_values()[0].lhs) != 77) return 23;
  }

  // Two speculative operands resolve independently. Resolving src1 first
  // must not execute until src0 also resolves, and each value keeps its lane.
  {
    I2System model;
    unsigned tick = 0;
    auto k = key(12);
    if (!inject(model.operand_response(), response(k, 3), tick++)) return 24;
    run(model, tick);
    auto hit1 = LoadDependencyResolution{k, gfsim::UInt<1>{1}, producer(k),
                                         gfsim::UInt<64>{66},
                                         gfsim::UInt<1>{1}, gfsim::UInt<1>{1},
                                         gfsim::UInt<1>{0}, gfsim::UInt<1>{0},
                                         gfsim::UInt<1>{1}};
    if (!inject(model.load_dependency(), hit1, tick++)) return 25;
    run(model, tick);
    if (model.sink_2_values().size() != 1 ||
        !u(model.sink_2_values()[0].resolved) ||
        !model.sink_0_values().empty()) return 26;
    auto hit0 = LoadDependencyResolution{k, gfsim::UInt<1>{0}, producer(k),
                                         gfsim::UInt<64>{77},
                                         gfsim::UInt<1>{1}, gfsim::UInt<1>{1},
                                         gfsim::UInt<1>{0}, gfsim::UInt<1>{0},
                                         gfsim::UInt<1>{1}};
    if (!inject(model.load_dependency(), hit0, tick++)) return 27;
    run(model, tick);
    if (model.sink_2_values().size() != 2 ||
        !u(model.sink_2_values()[1].resolved) ||
        model.sink_0_values().size() != 1 ||
        u(model.sink_0_values()[0].lhs) != 77 ||
        u(model.sink_0_values()[0].rhs) != 66) return 28;
  }

  // Same PTag with an old load-attempt generation is rejected without
  // resolving the operand; the exact producer can still complete afterward.
  {
    I2System model;
    unsigned tick = 0;
    auto k = key(13);
    if (!inject(model.operand_response(), response(k, 1), tick++)) return 29;
    run(model, tick);
    auto stale_producer = producer(k);
    stale_producer.load_attempt_generation = gfsim::UInt<16>{6};
    auto stale = LoadDependencyResolution{k, gfsim::UInt<1>{0}, stale_producer,
                                          gfsim::UInt<64>{55},
                                          gfsim::UInt<1>{1}, gfsim::UInt<1>{1},
                                          gfsim::UInt<1>{0}, gfsim::UInt<1>{0},
                                          gfsim::UInt<1>{1}};
    if (!inject(model.load_dependency(), stale, tick++)) return 33;
    run(model, tick);
    if (model.sink_2_values().size() != 1 ||
        u(model.sink_2_values()[0].accepted) ||
        !model.sink_0_values().empty()) return 34;
    auto exact = LoadDependencyResolution{k, gfsim::UInt<1>{0}, producer(k),
                                          gfsim::UInt<64>{77},
                                          gfsim::UInt<1>{1}, gfsim::UInt<1>{1},
                                          gfsim::UInt<1>{0}, gfsim::UInt<1>{0},
                                          gfsim::UInt<1>{1}};
    if (!inject(model.load_dependency(), exact, tick++)) return 35;
    run(model, tick);
    if (model.sink_2_values().size() != 2 ||
        !u(model.sink_2_values()[1].accepted) ||
        model.sink_0_values().size() != 1 ||
        u(model.sink_0_values()[0].lhs) != 77) return 36;
  }

  // Miss and replay retain distinct reasons and each generate one cancel.
  for (unsigned replay = 0; replay < 2; ++replay) {
    I2System model;
    unsigned tick = 0;
    auto k = key(3 + replay);
    if (!inject(model.operand_response(), response(k, true), tick++)) return 30;
    run(model, tick);
    auto failed = LoadDependencyResolution{
        k, gfsim::UInt<1>{0}, producer(k), gfsim::UInt<64>{0},
        gfsim::UInt<1>{0}, gfsim::UInt<1>{0},
        gfsim::UInt<1>{replay ? 0 : 1}, gfsim::UInt<1>{replay},
        gfsim::UInt<1>{1}};
    if (!inject(model.load_dependency(), failed, tick++)) return 31;
    run(model, tick);
    if (model.sink_2_values().size() != 1 ||
        !u(model.sink_2_values()[0].canceled) ||
        model.sink_5_values().size() != 1 ||
        model.sink_5_values()[0].reason !=
            (replay ? IssueCancelReason::LOAD_REPLAY
                    : IssueCancelReason::LOAD_MISS) ||
        !model.sink_0_values().empty()) return 32;
  }

  // Generated-cancel sink backpressure retains the second cancel transaction
  // after its dependency input/ack commit. Releasing capacity publishes each
  // retained cancellation exactly once and in order.
  {
    I2System model;
    unsigned tick = 0;
    auto first = key(14);
    auto second = key(15);
    auto miss = [&](const IssueAttemptKey &k) {
      return LoadDependencyResolution{
          k, gfsim::UInt<1>{0}, producer(k), gfsim::UInt<64>{0},
          gfsim::UInt<1>{0}, gfsim::UInt<1>{0}, gfsim::UInt<1>{1},
          gfsim::UInt<1>{0}, gfsim::UInt<1>{1}};
    };
    if (!inject(model.operand_response(), response(first, 1), tick++)) return 37;
    run(model, tick);
    if (!inject(model.load_dependency(), miss(first), tick++)) return 38;
    run(model, tick, 8, "sink_sink_5");
    if (!model.sink_5_values().empty()) return 39;
    if (!inject(model.operand_response(), response(second, 1), tick++)) return 46;
    run(model, tick, 8, "sink_sink_5");
    if (!inject(model.load_dependency(), miss(second), tick++)) return 47;
    run(model, tick, 8, "sink_sink_5");
    if (!model.sink_5_values().empty() ||
        model.sink_2_values().size() != 2 ||
        model.load_dependency().committedSize() != 0 ||
        !model.sink_0_values().empty()) return 48;
    run(model, tick, 20);
    if (model.sink_5_values().size() != 2 ||
        !(model.sink_5_values()[0].key == first) ||
        !(model.sink_5_values()[1].key == second)) return 49;
    run(model, tick, 8);
    if (model.sink_5_values().size() != 2) return 54;
  }

  // Exercise the conservative existing-tombstone generated-cancel path. The
  // first cancel fills the output Queue; the second attempt is active while a
  // matching tombstone is seeded, so its same-value write and state clear must
  // wait for output capacity and then commit atomically exactly once.
  {
    I2System model;
    unsigned tick = 0;
    auto first = key(6, 69);
    auto second = key(7, 70);
    auto miss = [&](const IssueAttemptKey &k) {
      return LoadDependencyResolution{
          k, gfsim::UInt<1>{0}, producer(k), gfsim::UInt<64>{0},
          gfsim::UInt<1>{0}, gfsim::UInt<1>{0}, gfsim::UInt<1>{1},
          gfsim::UInt<1>{0}, gfsim::UInt<1>{1}};
    };
    if (!inject(model.operand_response(), response(first, 1), tick++)) return 124;
    run(model, tick);
    if (!inject(model.load_dependency(), miss(first), tick++)) return 125;
    run(model, tick, 8, "sink_sink_5");
    if (!inject(model.operand_response(), response(second, 1), tick++)) return 126;
    run(model, tick, 8, "sink_sink_5");

    gfsim::SimTable<AttemptTombstone> *tombstones = nullptr;
    auto rows = model.dispatch_rows();
    for (auto &row : rows) {
      auto *object = static_cast<gfsim::SimObject *>(row.object);
      if (row.kind == gfsim::ObjectKind::Memory &&
          object->name() == "cancel_tombstones") {
        if (tombstones != nullptr) return 127;
        tombstones = dynamic_cast<gfsim::SimTable<AttemptTombstone> *>(object);
      }
    }
    if (tombstones == nullptr) return 127;
    std::array<size_t, 2> fields{0, 1};
    if (!tombstones->proposeWrite(
            gfsim::ObjectId{1000}, 7,
            AttemptTombstone{second, gfsim::UInt<1>{1}}, fields,
            [](AttemptTombstone &target, const AttemptTombstone &value) {
              target = value;
            },
            gfsim::TableWriteMode::Replace)) return 128;
    const gfsim::Epoch seed_epoch{++tick, 0};
    tombstones->doArbitrate(seed_epoch);
    tombstones->doXfer(seed_epoch);
    if (!u(tombstones->at(7).valid) ||
        !(tombstones->at(7).key == second)) return 129;

    if (!inject(model.load_dependency(), miss(second), tick++)) return 130;
    run(model, tick, 8, "sink_sink_5");
    if (!model.sink_5_values().empty() ||
        model.sink_2_values().size() != 2 ||
        model.load_dependency().committedSize() != 0 ||
        !u(tombstones->at(7).valid) ||
        !(tombstones->at(7).key == second)) return 131;
    run(model, tick, 20);
    if (model.sink_5_values().size() != 2 ||
        !(model.sink_5_values()[0].key == first) ||
        !(model.sink_5_values()[1].key == second) ||
        !u(tombstones->at(7).valid) ||
        !(tombstones->at(7).key == second)) return 132;
    run(model, tick, 8);
    if (model.sink_5_values().size() != 2) return 133;
  }

  // Early cancel installs a full-key tombstone. Same-key response is rejected,
  // while equal slot/generation in another Flow remains isolated and executes.
  {
    I2System model;
    unsigned tick = 0;
    auto canceled = key(6, 1, 0);
    auto isolated = key(6, 1, 1);
    if (!inject(model.cancel(),
                IssueCancel{canceled, IssueCancelReason::RECOVERY,
                            gfsim::UInt<1>{1}}, tick++)) return 40;
    run(model, tick);
    if (model.sink_6_values().size() != 1 ||
        !u(model.sink_6_values()[0].accepted)) return 41;
    if (!inject(model.operand_response(), response(canceled), tick++)) return 42;
    run(model, tick);
    if (model.sink_1_values().size() != 1 ||
        u(model.sink_1_values()[0].accepted) ||
        !model.sink_0_values().empty()) return 43;
    if (!inject(model.operand_response(), response(isolated), tick++)) return 44;
    run(model, tick);
    if (model.sink_1_values().size() != 2 ||
        !u(model.sink_1_values()[1].accepted) ||
        model.sink_0_values().size() != 1) return 45;
  }

  // Operand-ack backpressure retains the second input until capacity is
  // released, then commits both negative acknowledgements exactly once.
  {
    I2System model;
    unsigned tick = 0;
    auto malformed = response(key(7));
    malformed.denied_mask = gfsim::UInt<2>{1};
    if (!inject(model.operand_response(), malformed, tick++)) return 50;
    run(model, tick, 5, "sink_sink_1");
    if (!inject(model.operand_response(), malformed, tick++)) return 51;
    run(model, tick, 5, "sink_sink_1");
    if (model.operand_response().committedSize() != 1 ||
        !model.sink_1_values().empty()) return 52;
    run(model, tick, 12);
    if (model.operand_response().committedSize() != 0 ||
        model.sink_1_values().size() != 2 ||
        u(model.sink_1_values()[0].accepted) ||
        u(model.sink_1_values()[1].accepted)) return 53;
  }

  // Execute-request backpressure holds the first publication. A retry clears
  // outstanding state, but the reissue cannot commit until output capacity is
  // released; afterward exactly two identical requests are observed.
  {
    I2System model;
    unsigned tick = 0;
    auto k = key(3, 66);
    if (!inject(model.operand_response(), response(k), tick++)) return 106;
    run(model, tick, 8, "sink_sink_0");
    if (!model.sink_0_values().empty()) return 107;
    if (!inject(model.sink_decision(),
                ExecutionSinkDecision{k, gfsim::UInt<1>{0},
                                      gfsim::UInt<1>{1}, gfsim::UInt<1>{1}},
                tick++)) return 108;
    run(model, tick, 8, "sink_sink_0");
    if (model.sink_3_values().size() != 1 ||
        !u(model.sink_3_values()[0].retry) ||
        !model.sink_0_values().empty()) return 109;
    run(model, tick, 16);
    if (model.sink_0_values().size() != 2 ||
        !(model.sink_0_values()[0].attempt.key == k) ||
        !(model.sink_0_values()[1].attempt.key == k)) return 110;
    run(model, tick, 8);
    if (model.sink_0_values().size() != 2) return 111;
  }

  // Each always-acknowledged input retains its second token independently
  // while the corresponding output Queue is full, then publishes exactly once.
  {
    I2System model;
    unsigned tick = 0;
    auto invalid_dependency = LoadDependencyResolution{};
    if (!inject(model.load_dependency(), invalid_dependency, tick++)) return 112;
    run(model, tick, 6, "sink_sink_2");
    if (!inject(model.load_dependency(), invalid_dependency, tick++)) return 113;
    run(model, tick, 6, "sink_sink_2");
    if (model.load_dependency().committedSize() != 1 ||
        !model.sink_2_values().empty()) return 114;
    run(model, tick, 12);
    if (model.load_dependency().committedSize() != 0 ||
        model.sink_2_values().size() != 2 ||
        u(model.sink_2_values()[0].accepted) ||
        u(model.sink_2_values()[1].accepted)) return 115;
  }
  {
    I2System model;
    unsigned tick = 0;
    auto invalid_decision = ExecutionSinkDecision{};
    if (!inject(model.sink_decision(), invalid_decision, tick++)) return 116;
    run(model, tick, 6, "sink_sink_3");
    if (!inject(model.sink_decision(), invalid_decision, tick++)) return 117;
    run(model, tick, 6, "sink_sink_3");
    if (model.sink_decision().committedSize() != 1 ||
        !model.sink_3_values().empty()) return 118;
    run(model, tick, 12);
    if (model.sink_decision().committedSize() != 0 ||
        model.sink_3_values().size() != 2 ||
        u(model.sink_3_values()[0].matched) ||
        u(model.sink_3_values()[1].matched)) return 119;
  }
  {
    I2System model;
    unsigned tick = 0;
    auto first = IssueCancel{key(4, 67), IssueCancelReason::RECOVERY,
                             gfsim::UInt<1>{1}};
    auto second = IssueCancel{key(5, 68), IssueCancelReason::RECOVERY,
                              gfsim::UInt<1>{1}};
    if (!inject(model.cancel(), first, tick++)) return 120;
    run(model, tick, 6, "sink_sink_6");
    if (!inject(model.cancel(), second, tick++)) return 121;
    run(model, tick, 6, "sink_sink_6");
    if (model.cancel().committedSize() != 1 ||
        !model.sink_6_values().empty()) return 122;
    run(model, tick, 12);
    if (model.cancel().committedSize() != 0 ||
        model.sink_6_values().size() != 2 ||
        !u(model.sink_6_values()[0].accepted) ||
        !u(model.sink_6_values()[1].accepted) ||
        !(model.sink_6_values()[0].request.key == first.key) ||
        !(model.sink_6_values()[1].request.key == second.key)) return 123;
  }

  // Sixteen early cancels fill the bounded tombstone table. A seventeenth is
  // consumed with a negative acknowledgement and no generated cancellation.
  {
    I2System model;
    unsigned tick = 0;
    for (unsigned n = 0; n < 17; ++n) {
      auto k = key(n & 15, n + 1);
      if (!inject(model.cancel(),
                  IssueCancel{k, IssueCancelReason::RECOVERY,
                              gfsim::UInt<1>{1}}, tick++)) return 60;
      run(model, tick);
    }
    if (model.sink_6_values().size() != 17) return 61;
    for (unsigned n = 0; n < 16; ++n)
      if (!u(model.sink_6_values()[n].accepted)) return 62;
    if (u(model.sink_6_values()[16].accepted) ||
        !model.sink_5_values().empty()) return 63;
  }

  // Once the sink accepts, release backpressure retains ownership. An exact
  // post-accept cancel is rejected and cannot remove the pending release.
  {
    I2System model;
    unsigned tick = 0;
    auto first = key(9);
    auto k = key(10);
    if (!inject(model.operand_response(), response(first), tick++)) return 65;
    run(model, tick);
    if (model.sink_0_values().size() != 1) return 66;
    if (!inject(model.sink_decision(),
                ExecutionSinkDecision{first, gfsim::UInt<1>{1},
                                      gfsim::UInt<1>{0}, gfsim::UInt<1>{1}},
                tick++)) return 67;
    run(model, tick, 8, "sink_sink_4");
    if (!model.sink_4_values().empty()) return 68;
    if (!inject(model.operand_response(), response(k), tick++)) return 69;
    run(model, tick, 8, "sink_sink_4");
    if (model.sink_0_values().size() != 2) return 73;
    if (!inject(model.sink_decision(),
                ExecutionSinkDecision{k, gfsim::UInt<1>{1},
                                      gfsim::UInt<1>{0}, gfsim::UInt<1>{1}},
                tick++)) return 74;
    run(model, tick, 8, "sink_sink_4");
    if (model.sink_3_values().size() != 2 ||
        !u(model.sink_3_values()[1].transferred)) return 76;
    if (!inject(model.cancel(),
                IssueCancel{k, IssueCancelReason::RECOVERY,
                            gfsim::UInt<1>{1}}, tick++)) return 77;
    run(model, tick, 8, "sink_sink_4");
    if (model.sink_6_values().size() != 1) return 78;
    if (u(model.sink_6_values()[0].accepted)) return 79;
    run(model, tick, 12);
    if (model.sink_4_values().size() != 2 ||
        !(model.sink_4_values()[0].key == first) ||
        !(model.sink_4_values()[1].key == k)) return 80;
  }

  // An exact external cancel clears active cancellable work. A later producer
  // response for that attempt is consumed as stale and cannot execute.
  {
    I2System model;
    unsigned tick = 0;
    auto k = key(11);
    if (!inject(model.operand_response(), response(k, true), tick++)) return 81;
    run(model, tick);
    if (!inject(model.cancel(),
                IssueCancel{k, IssueCancelReason::RECOVERY,
                            gfsim::UInt<1>{1}}, tick++)) return 82;
    run(model, tick);
    if (model.sink_6_values().size() != 1 ||
        !u(model.sink_6_values()[0].accepted)) return 83;
    auto hit = LoadDependencyResolution{k, gfsim::UInt<1>{0}, producer(k),
                                        gfsim::UInt<64>{88},
                                        gfsim::UInt<1>{1}, gfsim::UInt<1>{1},
                                        gfsim::UInt<1>{0}, gfsim::UInt<1>{0},
                                        gfsim::UInt<1>{1}};
    if (!inject(model.load_dependency(), hit, tick++)) return 84;
    run(model, tick);
    if (model.sink_2_values().size() != 1 ||
        u(model.sink_2_values()[0].accepted) ||
        !model.sink_0_values().empty()) return 85;
  }

  // Reset clears an already populated tombstone table. The exact same key is
  // admitted afterward rather than being suppressed by pre-reset history.
  {
    I2System model;
    unsigned tick = 0;
    auto k = key(1, 44);
    if (!inject(model.cancel(),
                IssueCancel{k, IssueCancelReason::RECOVERY,
                            gfsim::UInt<1>{1}}, tick++)) return 86;
    run(model, tick);
    if (model.sink_6_values().size() != 1 ||
        !u(model.sink_6_values()[0].accepted)) return 87;
    auto rows = model.dispatch_rows();
    for (auto &row : rows) row.reset(row.object);
    if (!inject(model.operand_response(), response(k), ++tick)) return 88;
    run(model, tick);
    if (model.sink_1_values().size() != 1 ||
        !u(model.sink_1_values()[0].accepted) ||
        model.sink_0_values().size() != 1) return 89;
  }

  // Reset clears retained speculative ownership: a formerly matching hit is
  // consumed as stale and cannot execute.
  {
    I2System model;
    unsigned tick = 0;
    auto k = key(8);
    if (!inject(model.operand_response(), response(k, true), tick++)) return 70;
    run(model, tick);
    auto rows = model.dispatch_rows();
    for (auto &row : rows) row.reset(row.object);
    auto hit = LoadDependencyResolution{k, gfsim::UInt<1>{0}, producer(k),
                                        gfsim::UInt<64>{99},
                                        gfsim::UInt<1>{1}, gfsim::UInt<1>{1},
                                        gfsim::UInt<1>{0}, gfsim::UInt<1>{0},
                                        gfsim::UInt<1>{1}};
    if (!inject(model.load_dependency(), hit, ++tick)) return 71;
    run(model, tick);
    if (model.sink_2_values().size() != 1 ||
        u(model.sink_2_values()[0].accepted) ||
        !model.sink_0_values().empty()) return 72;
  }


  // Two generated module instances retain independent state and output logs
  // even for an identical full key.
  {
    I2System left;
    I2System right;
    unsigned left_tick = 0;
    unsigned right_tick = 0;
    auto k = key(2, 55);
    if (!inject(left.cancel(),
                IssueCancel{k, IssueCancelReason::RECOVERY,
                            gfsim::UInt<1>{1}}, left_tick++)) return 90;
    run(left, left_tick);
    if (!inject(right.operand_response(), response(k), right_tick++)) return 91;
    run(right, right_tick);
    if (left.sink_6_values().size() != 1 ||
        !u(left.sink_6_values()[0].accepted) ||
        !left.sink_0_values().empty() ||
        !left.sink_1_values().empty()) return 92;
    if (!right.sink_6_values().empty() ||
        right.sink_1_values().size() != 1 ||
        !u(right.sink_1_values()[0].accepted) ||
        right.sink_0_values().size() != 1) return 93;
  }
  return 0;
}
""",
            encoding="utf-8",
        )
        built = subprocess.run(
            (
                cxx,
                "-std=c++20",
                f"-I{ROOT / 'simulator/gfsim/include'}",
                str(generated),
                "-o",
                str(executable),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert built.returncode == 0, built.stderr
        executed = subprocess.run(
            (str(executable),), text=True, capture_output=True, check=False
        )
        assert executed.returncode == 0, (
            f"exit={executed.returncode}\nstdout:\n{executed.stdout}\n"
            f"stderr:\n{executed.stderr}"
        )
