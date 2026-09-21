// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-verify-value-constraints,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %FileCheck %s --check-prefix=CLOSED < %t.frozen.mlir
// RUN: %acir_opt "-ac-build-rule-effect-graph=json-output=%t.effect.json dot-output=%t.effect.dot" %t.frozen.mlir -o /dev/null
// RUN: %FileCheck %s --check-prefix=EFFECT < %t.effect.json
// RUN: %acir_queue_plan %t.frozen.mlir > %t.plan.json
// RUN: %FileCheck %s --check-prefix=PLAN < %t.plan.json
// RUN: %acir_queue_pycgen %t.frozen.mlir > %t.pyc
// RUN: %acir_queue_pycgen %t.frozen.mlir > %t.pyc.again
// RUN: diff %t.pyc %t.pyc.again
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.gfsim.cpp
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.gfsim.again.cpp
// RUN: diff %t.gfsim.cpp %t.gfsim.again.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.gfsim.cpp
// RUN: %pycc %t.pyc --cpp %t.cpp --logic-depth=512
// RUN: %FileCheck %s --check-prefix=CPP < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -include %t.cpp %S/miniooo-stress-harness.cpp -o %t.stress
// RUN: %t.stress | %FileCheck %s --check-prefix=EXEC
// RUN: %pycc %t.pyc --verilog %t.sv --logic-depth=512
// RUN: %pycc %t.pyc --verilog %t.sv.again --logic-depth=512
// RUN: diff %t.sv %t.sv.again
// RUN: %FileCheck %s --check-prefix=SVA < %t.sv
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t.sv --json-out %t.audit.json
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t.sv
// RUN: iverilog -g2012 -DSYNTHESIS -I%source_root/library/verilog -s tb_miniooo -o %t.vvp %t.sv %S/miniooo-tb.sv
// RUN: vvp %t.vvp | %FileCheck %s --check-prefix=RTL-EXEC

// A vendor-neutral reduced MiniOOO, not a product Core: four-wide dispatch with
// one committed resource group, two execution resource classes, a small
// versioned reorder window, a ready/dependency update, age-ordered retirement,
// versioned completion, branch recovery, and one memory ordering dependency.
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "miniooo"} {
  ac.type_scope @types {
    ac.struct @Dispatch fields [
      {name = "valid", type = i4}, {name = "r0", type = i4},
      {name = "r1", type = i4}, {name = "r2", type = i4},
      {name = "free", type = i4}, {name = "release", type = i4},
      {name = "candidates", type = i4},
      {name = "age0", type = i3}, {name = "age1", type = i3},
      {name = "age2", type = i3}, {name = "age3", type = i3},
      {name = "current", type = i4}, {name = "resolve", type = i4},
      {name = "kill", type = i4}, {name = "identity", type = i4},
      {name = "effects", type = i4}, {name = "terminal", type = i4},
      {name = "alias", type = i4}, {name = "disjoint", type = i4},
      {name = "data_ready", type = i4}, {name = "executed", type = i4},
      {name = "killed", type = i4}
    ]
    ac.struct @Issue fields [
      {name = "slot", type = i2}, {name = "valid", type = i1},
      {name = "generation", type = i2}, {name = "recovery_epoch", type = i3},
      {name = "attempt", type = i2}, {name = "payload", type = i8},
      {name = "reserved", type = i4}, {name = "accepted", type = i4},
      {name = "alu_accepted", type = i4}, {name = "mem_accepted", type = i4},
      {name = "commit_mask", type = i4}, {name = "winners", type = i4},
      {name = "deps_next", type = i4}, {name = "wait", type = i4},
      {name = "bypass", type = i4}, {name = "forward", type = i4},
      {name = "replay", type = i4}, {name = "stale", type = i4},
      {name = "completed", type = i4}, {name = "ready", type = i1},
      {name = "order_mask", type = i4}
    ]
    ac.struct @Entry fields [
      {name = "slot", type = i2}, {name = "valid", type = i1},
      {name = "generation", type = i2}, {name = "recovery_epoch", type = i3},
      {name = "attempt", type = i2}, {name = "payload", type = i8}
    ]
    ac.struct @Completion fields [
      {name = "slot", type = i2}, {name = "generation", type = i2},
      {name = "recovery_epoch", type = i3}, {name = "attempt", type = i2},
      {name = "payload", type = i8}
    ]
    ac.struct @Recovery fields [
      {name = "valid", type = i1}, {name = "next_epoch", type = i3},
      {name = "checkpoint", type = i2}, {name = "boundary", type = i2},
      {name = "slot", type = i2}, {name = "generation", type = i2},
      {name = "transaction_epoch", type = i3}, {name = "attempt", type = i2}
    ]
  } {dlti.dl_spec = #dlti.dl_spec<
      !ac.struct<@types::@Dispatch> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 84 : i64},
      !ac.struct<@types::@Issue> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 75 : i64},
      !ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 18 : i64},
      !ac.struct<@types::@Completion> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 17 : i64},
      !ac.struct<@types::@Recovery> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 17 : i64}>}

  ac.recovery_domain @speculation epoch_bits 3 initial 0
  ac.typed_identity @transaction width 1
  ac.table @window entry !ac.struct<@types::@Entry> entries 4 init 0
      owner "/" stable_id "table/window" {
    recovery_domain = @speculation, identity = @transaction,
    generation_bits = 2 : i64, epoch_bits = 3 : i64,
    attempt_bits = 2 : i64, valid_field = "valid",
    generation_field = "generation", epoch_field = "recovery_epoch",
    attempt_field = "attempt", payload_field = "payload"
  }

  %dispatch = ac.source depth 4 latency 1 {ac.name = "dispatch"}
      : !ac.queue<!ac.struct<@types::@Dispatch>>
  %issue_q = ac.rule %dispatch depths [4] latencies [1]
      name "issue" stable_id "issue" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Dispatch>>):
    %valid = ac.var.get %item field "valid" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %r0 = ac.var.get %item field "r0" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %r1 = ac.var.get %item field "r1" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %r2 = ac.var.get %item field "r2" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %free = ac.var.get %item field "free" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %release = ac.var.get %item field "release" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %candidates = ac.var.get %item field "candidates" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %age0 = ac.var.get %item field "age0" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i3>
    %age1 = ac.var.get %item field "age1" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i3>
    %age2 = ac.var.get %item field "age2" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i3>
    %age3 = ac.var.get %item field "age3" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i3>
    %current = ac.var.get %item field "current" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %resolve = ac.var.get %item field "resolve" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %kill = ac.var.get %item field "kill" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %identity = ac.var.get %item field "identity" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %effects = ac.var.get %item field "effects" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %terminal = ac.var.get %item field "terminal" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %alias = ac.var.get %item field "alias" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %disjoint = ac.var.get %item field "disjoint" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %data_ready = ac.var.get %item field "data_ready" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %executed = ac.var.get %item field "executed" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %killed = ac.var.get %item field "killed" : !ac.var<!ac.struct<@types::@Dispatch>> -> !ac.var<i4>
    %reserved = "ac.reservation_set"(%r0, %r1, %r2) <{lanes = 4 : i64}>
        : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %accepted = "ac.transaction_group"(%valid, %reserved) <{
      lanes = 4 : i64, policy = #ac<transaction_group_policy valid_prefix>
    }> : (!ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %alu_alloc, %alu_accepted, %alu_next = "ac.multi_allocator"(
        %free, %accepted, %release) <{
      lanes = 4 : i64, reuse_policy = #ac<same_cycle_reuse_policy forbid>,
      generation_bits = 2 : i64, generation_policy = "increment_on_allocate"
    }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) ->
        (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
    %mem_alloc, %mem_accepted, %mem_next = "ac.multi_allocator"(
        %alu_next, %alu_accepted, %release) <{
      lanes = 4 : i64, reuse_policy = #ac<same_cycle_reuse_policy allow>,
      generation_bits = 2 : i64, generation_policy = "increment_on_allocate"
    }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) ->
        (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
    %commit_mask = "ac.reservation_set"(%mem_accepted, %mem_accepted,
        %mem_accepted) <{lanes = 4 : i64, commit = true}>
        : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %winners = "ac.age_select_k"(%candidates, %age0, %age1, %age2, %age3) <{
      lanes = 4 : i64, count = 2 : i64, ordering = "oldest_first"
    }> : (!ac.var<i4>, !ac.var<i3>, !ac.var<i3>, !ac.var<i3>, !ac.var<i3>)
        -> !ac.var<i4>
    %deps_next, %ready = "ac.dependency_set"(
        %current, %mem_accepted, %resolve, %kill, %identity) <{lanes = 4 : i64}>
        : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>)
        -> (!ac.var<i4>, !ac.var<i1>)
    %order = "ac.memory_order_edge"(%alu_accepted, %mem_accepted, %alias) <{
      lanes = 4 : i64, kind = #ac<memory_order_kind must_forward>
    }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %wait, %bypass, %forward, %replay, %stale = "ac.load_disposition"(
        %mem_accepted, %alias, %disjoint, %data_ready, %executed, %identity,
        %killed) <{lanes = 4 : i64}>
        : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
           !ac.var<i4>, !ac.var<i4>) -> (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
                                         !ac.var<i4>, !ac.var<i4>)
    %completed = "ac.terminal_transaction"(%commit_mask, %effects, %terminal) <{
      lanes = 4 : i64
    }> : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>) -> !ac.var<i4>
    %slot = ac.var.constant 0 : i2 as !ac.var<i2>
    %generation = ac.var.constant 0 : i2 as !ac.var<i2>
    %epoch = ac.var.constant 0 : i3 as !ac.var<i3>
    %attempt = ac.var.constant 0 : i2 as !ac.var<i2>
    %payload = ac.var.constant 0 : i8 as !ac.var<i8>
    %entry_valid = ac.var.constant true as !ac.var<i1>
    %issue = ac.var.record %slot, %entry_valid, %generation, %epoch, %attempt,
        %payload, %reserved, %accepted, %alu_accepted, %mem_accepted,
        %commit_mask, %winners, %deps_next, %wait, %bypass, %forward, %replay,
        %stale, %completed, %ready, %order : !ac.var<i2>, !ac.var<i1>,
        !ac.var<i2>, !ac.var<i3>, !ac.var<i2>, !ac.var<i8>, !ac.var<i4>,
        !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
        !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
        !ac.var<i4>, !ac.var<i4>, !ac.var<i1>, !ac.var<i4>
        -> !ac.var<!ac.struct<@types::@Issue>>
    %handshake = ac.marker.obligation %issue state pending resolver handshake
        origin "issue:return" path "true" : !ac.var<!ac.struct<@types::@Issue>>
    ac.rule.return %handshake : !ac.var<!ac.struct<@types::@Issue>>
  } {ac.name = "issue_q"} : (!ac.queue<!ac.struct<@types::@Dispatch>>)
      -> !ac.queue<!ac.struct<@types::@Issue>>

  ac.rule %issue_q depths [] latencies [] name "commit" stable_id "commit"
      domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Issue>>):
    %slot = ac.var.get %item field "slot" : !ac.var<!ac.struct<@types::@Issue>> -> !ac.var<i2>
    %valid = ac.var.get %item field "valid" : !ac.var<!ac.struct<@types::@Issue>> -> !ac.var<i1>
    %generation = ac.var.get %item field "generation" : !ac.var<!ac.struct<@types::@Issue>> -> !ac.var<i2>
    %epoch = ac.var.get %item field "recovery_epoch" : !ac.var<!ac.struct<@types::@Issue>> -> !ac.var<i3>
    %attempt = ac.var.get %item field "attempt" : !ac.var<!ac.struct<@types::@Issue>> -> !ac.var<i2>
    %payload = ac.var.get %item field "payload" : !ac.var<!ac.struct<@types::@Issue>> -> !ac.var<i8>
    %entry = ac.var.record %slot, %valid, %generation, %epoch, %attempt,
        %payload : !ac.var<i2>, !ac.var<i1>, !ac.var<i2>, !ac.var<i3>,
        !ac.var<i2>, !ac.var<i8> -> !ac.var<!ac.struct<@types::@Entry>>
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.versioned_table.propose @window [%slot] = %entry when %true : !ac.var<i1>
        ref %generation, %epoch : !ac.var<i2>, !ac.var<i3>
        attempt %attempt : !ac.var<i2> action "allocate" mode "replace"
        write_fields ["slot", "valid", "generation", "recovery_epoch",
                      "attempt", "payload"]
        {ac.arbitration = #ac.writer_priority<0>}
        : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Issue>>) -> ()

  %completions = ac.source depth 2 latency 1 {ac.name = "completions"}
      : !ac.queue<!ac.struct<@types::@Completion>>
  ac.rule %completions depths [] latencies [] name "retire" stable_id "retire"
      domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Completion>>):
    %slot = ac.var.get %item field "slot" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i2>
    %generation = ac.var.get %item field "generation" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i2>
    %epoch = ac.var.get %item field "recovery_epoch" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i3>
    %attempt = ac.var.get %item field "attempt" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i2>
    %payload = ac.var.get %item field "payload" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i8>
    %stored = ac.table.get @window[%slot] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %updated = ac.var.with %stored, %payload field "payload"
        : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i8> -> !ac.var<!ac.struct<@types::@Entry>>
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.versioned_table.propose @window [%slot] = %updated when %true : !ac.var<i1>
        ref %generation, %epoch : !ac.var<i2>, !ac.var<i3>
        attempt %attempt : !ac.var<i2> action "qualified_update" mode "field"
        write_fields ["payload"] {ac.arbitration = #ac.writer_priority<1>}
        : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Completion>>) -> ()

  %recoveries = ac.source depth 2 latency 1 {ac.name = "recoveries"}
      : !ac.queue<!ac.struct<@types::@Recovery>>
  ac.rule %recoveries depths [] latencies [] name "recover" stable_id "recover"
      domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Recovery>>):
    %valid = ac.var.get %item field "valid" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i1>
    %next_epoch = ac.var.get %item field "next_epoch" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i3>
    %checkpoint = ac.var.get %item field "checkpoint" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i2>
    %boundary = ac.var.get %item field "boundary" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i2>
    %slot = ac.var.get %item field "slot" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i2>
    %generation = ac.var.get %item field "generation" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i2>
    %transaction_epoch = ac.var.get %item field "transaction_epoch" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i3>
    %attempt = ac.var.get %item field "attempt" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i2>
    %event = ac.recovery.event %valid epoch %next_epoch checkpoint %checkpoint
        boundary %boundary domain @speculation cause "mispredict"
        : !ac.var<i1>, !ac.var<i3>, !ac.var<i2>, !ac.var<i2> -> !ac.var<i1>
    %kill = ac.kill_set %event transaction_epoch %transaction_epoch
        next_epoch %next_epoch transaction_slot %slot boundary %boundary
        policy "epoch_mismatch_or_younger"
        : !ac.var<i1>, !ac.var<i3>, !ac.var<i3>, !ac.var<i2>, !ac.var<i2>
        -> !ac.var<i1>
    %stored = ac.table.get @window[%slot] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %false = ac.var.constant false as !ac.var<i1>
    %cleared = ac.var.with %stored, %false field "valid"
        : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.condition %kill : !ac.var<i1>
    ac.versioned_table.propose @window [%slot] = %cleared when %kill : !ac.var<i1>
        ref %generation, %transaction_epoch : !ac.var<i2>, !ac.var<i3>
        attempt %attempt : !ac.var<i2> action "invalidate" mode "field"
        write_fields ["valid"] {ac.arbitration = #ac.writer_priority<2>}
        : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Recovery>>) -> ()

}

// CLOSED: ac.versioned_table.propose
// CLOSED: ac.recovery.event
// CLOSED: ac.kill_set
// EFFECT: "edges"
// EFFECT: "nodes"
// EFFECT: "kind": "arbitration_domain"
// EFFECT: "kind": "state_footprint"
// EFFECT: "kind": "interaction"
// EFFECT: "kind": "obligation_linkage"
// EFFECT: "kind": "recovery_domain"
// EFFECT: "kind": "rule"
// PYC: obligation_kind = "no_stale_response"
// PYC-SAME: obligation_id = "no_stale_response:issue_q:disposition0"
// PYC: obligation_kind = "no_stale_update"
// PYC: no_stale_update:window:recover:slot0
// PYC: no_stale_update:window:retire:slot0
// CPP: obligation_no_stale_response_issue_q_disposition0_coverage = 0;
// CPP: obligation_no_stale_update_window_retire_slot0_coverage = 0;
// PLAN: "kind":"reservation_set"
// PLAN: "kind":"transaction_group"
// PLAN: "kind":"multi_allocator_accepted"
// PLAN: "kind":"age_select_k"
// PLAN: "kind":"dependency_set_next"
// PLAN: "kind":"memory_order_edge"
// PLAN: "kind":"load_disposition_forward"
// PLAN: "kind":"terminal_transaction"
// PLAN: "stale_obligation_id":"no_stale_update:window:commit"
// PLAN: "stale_obligation_id":"no_stale_update:window:recover"
// SVA: assert property
// SVA: cover property
// The executed stress drives the whole lane algebra for 48 bounded cycles:
// every dispatch and every driven completion or recovery is accepted through the
// real ready/valid handshake. Coverage is the load-bearing evidence here, not
// the assertion outcome: the generated no_stale_update guard is orthogonal by
// construction, so this run asserts that the versioned window keeps committing
// qualified updates (the halfway counter is strictly smaller than the final
// one) instead of wedging after its first update, that the killing recovery is
// observed as a stale-mutation attempt, and that a stale lane is classified as
// stale rather than forwarded.
// EXEC: miniOOO stress PASS cycles=48 stale_trials=6 dispatched=48 completed=47 recovered=1 retire_coverage={{[1-9][0-9]*}}
// EXEC-SAME: retire_at_half={{[1-9][0-9]*}}
// EXEC-SAME: recover_coverage={{[1-9][0-9]*}}
// EXEC-SAME: stale_coverage={{[1-9][0-9]*}} failures=0
// The RTL lowering replays the same bounded schedule with an independently
// drawn payload stream and has to accept the identical dispatch, completion and
// recovery cadence: 48 dispatches, 47 versioned completions and the single
// killing recovery, with no source stalled past the handshake bound.
// RTL-EXEC: miniOOO rtl stress PASS cycles=48 dispatched=48 completed=47 recovered=1
