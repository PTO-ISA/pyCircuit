// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_plan %t.frozen.mlir | %FileCheck %s --check-prefix=PLAN
// RUN: %acir_queue_pycgen %t.frozen.mlir > %t.pyc
// RUN: %FileCheck %s --check-prefix=PYC < %t.pyc
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.gfsim.cpp
// RUN: %FileCheck %s --check-prefix=CPP < %t.gfsim.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -fsyntax-only %t.gfsim.cpp
// RUN: %pycc %t.pyc --cpp %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -include %t.cpp %S/versioned-table-recovery-harness.cpp -o %t.run
// RUN: %t.run | %FileCheck %s --check-prefix=EXEC
// RUN: %pycc %t.pyc --verilog %t.sv
// RUN: %FileCheck %s --check-prefix=SVA < %t.sv
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t.sv --json-out %t.audit.json
// RUN: %pycc %t.pyc --verilog %t.second.sv
// RUN: cmp %t.sv %t.second.sv
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t.sv
// RUN: iverilog -g2012 -DSYNTHESIS -I%source_root/library/verilog -s tb_versioned_recovery -o %t.vvp %t.sv %S/versioned-table-recovery-tb.sv
// RUN: vvp %t.vvp | %FileCheck %s --check-prefix=RTL-EXEC

module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "versioned_recovery"} {
  ac.type_scope @types {
    ac.struct @Entry fields [
      {name = "slot", type = i1},
      {name = "valid", type = i1},
      {name = "generation", type = i2},
      {name = "recovery_epoch", type = i3},
      {name = "attempt", type = i2},
      {name = "payload", type = i8}
    ]
    ac.struct @Completion fields [
      {name = "slot", type = i1},
      {name = "generation", type = i2},
      {name = "recovery_epoch", type = i3},
      {name = "attempt", type = i2},
      {name = "payload", type = i8}
    ]
    ac.struct @ReadRef fields [
      {name = "retained", type = i1},
      {name = "slot", type = i1},
      {name = "generation", type = i2},
      {name = "recovery_epoch", type = i3},
      {name = "attempt", type = i2}
    ]
    ac.struct @Recovery fields [
      {name = "valid", type = i1},
      {name = "next_epoch", type = i3},
      {name = "checkpoint", type = i2},
      {name = "boundary", type = i1},
      {name = "slot", type = i1},
      {name = "generation", type = i2},
      {name = "transaction_epoch", type = i3},
      {name = "attempt", type = i2}
    ]
  } {dlti.dl_spec = #dlti.dl_spec<
      !ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 17 : i64},
      !ac.struct<@types::@Completion> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 16 : i64},
      !ac.struct<@types::@ReadRef> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 9 : i64},
      !ac.struct<@types::@Recovery> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 15 : i64}>}

  ac.recovery_domain @speculation epoch_bits 3 initial 0
  ac.typed_identity @transaction width 1
  ac.checkpoint @checkpoint domain @speculation entries 2 payload i8
  ac.retained_result @result domain @speculation identity @transaction
      attempt_bits 2 payload i8
  ac.table @window entry !ac.struct<@types::@Entry> entries 2 init 0
      owner "/" stable_id "table/window" {
    recovery_domain = @speculation, identity = @transaction,
    checkpoint = @checkpoint,
    generation_bits = 2 : i64, epoch_bits = 3 : i64,
    attempt_bits = 2 : i64, valid_field = "valid",
    generation_field = "generation", epoch_field = "recovery_epoch",
    attempt_field = "attempt", payload_field = "payload"
  }
  ac.table @retained entry !ac.struct<@types::@Entry> entries 1 init 0
      owner "/" stable_id "table/retained" {
    recovery_domain = @speculation, identity = @transaction,
    retained_result = @result,
    generation_bits = 2 : i64, epoch_bits = 3 : i64,
    attempt_bits = 2 : i64, valid_field = "valid",
    generation_field = "generation", epoch_field = "recovery_epoch",
    attempt_field = "attempt", payload_field = "payload"
  }

  %alloc_in = ac.source depth 2 latency 1 {ac.name = "alloc_in"} : !ac.queue<!ac.struct<@types::@Entry>>
  %complete_in = ac.source depth 2 latency 1 {ac.name = "complete_in"} : !ac.queue<!ac.struct<@types::@Completion>>
  %read_in = ac.source depth 2 latency 1 {ac.name = "read_in"} : !ac.queue<!ac.struct<@types::@ReadRef>>
  %recovery_in = ac.source depth 2 latency 1 {ac.name = "recovery_in"} : !ac.queue<!ac.struct<@types::@Recovery>>
  %retain_in = ac.source depth 2 latency 1 {ac.name = "retain_in"} : !ac.queue<!ac.struct<@types::@Entry>>
  %consume_in = ac.source depth 2 latency 1 {ac.name = "consume_in"} : !ac.queue<!ac.struct<@types::@Completion>>

  ac.rule %alloc_in depths [] latencies [] name "allocate" stable_id "allocate" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Entry>>):
    %slot = ac.var.get %item field "slot" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i1>
    %generation = ac.var.get %item field "generation" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i2>
    %epoch = ac.var.get %item field "recovery_epoch" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i3>
    %attempt = ac.var.get %item field "attempt" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i2>
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.versioned_table.propose @window [%slot] = %item when %true : !ac.var<i1>
        ref %generation, %epoch : !ac.var<i2>, !ac.var<i3>
        attempt %attempt : !ac.var<i2> action "allocate" mode "replace"
        write_fields ["slot", "valid", "generation", "recovery_epoch", "attempt", "payload"]
        {ac.arbitration = #ac.writer_priority<0>}
        : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Entry>>) -> ()

  ac.rule %complete_in depths [] latencies [] name "complete" stable_id "complete" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Completion>>):
    %slot = ac.var.get %item field "slot" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i1>
    %generation = ac.var.get %item field "generation" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i2>
    %epoch = ac.var.get %item field "recovery_epoch" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i3>
    %attempt = ac.var.get %item field "attempt" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i2>
    %payload = ac.var.get %item field "payload" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i8>
    %stored = ac.table.get @window[%slot] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    %updated = ac.var.with %stored, %payload field "payload" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i8> -> !ac.var<!ac.struct<@types::@Entry>>
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.versioned_table.propose @window [%slot] = %updated when %true : !ac.var<i1>
        ref %generation, %epoch : !ac.var<i2>, !ac.var<i3>
        attempt %attempt : !ac.var<i2> action "qualified_update" mode "field"
        write_fields ["payload"] {ac.arbitration = #ac.writer_priority<1>}
        : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Completion>>) -> ()

  ac.rule %recovery_in depths [] latencies [] name "recover" stable_id "recover" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Recovery>>):
    %valid = ac.var.get %item field "valid" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i1>
    %next_epoch = ac.var.get %item field "next_epoch" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i3>
    %checkpoint = ac.var.get %item field "checkpoint" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i2>
    %boundary = ac.var.get %item field "boundary" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i1>
    %slot = ac.var.get %item field "slot" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i1>
    %generation = ac.var.get %item field "generation" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i2>
    %transaction_epoch = ac.var.get %item field "transaction_epoch" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i3>
    %attempt = ac.var.get %item field "attempt" : !ac.var<!ac.struct<@types::@Recovery>> -> !ac.var<i2>
    %event = ac.recovery.event %valid epoch %next_epoch checkpoint %checkpoint boundary %boundary domain @speculation cause "mispredict" : !ac.var<i1>, !ac.var<i3>, !ac.var<i2>, !ac.var<i1> -> !ac.var<i1>
    %kill = ac.kill_set %event transaction_epoch %transaction_epoch next_epoch %next_epoch transaction_slot %slot boundary %boundary policy "epoch_mismatch_or_younger" : !ac.var<i1>, !ac.var<i3>, !ac.var<i3>, !ac.var<i1>, !ac.var<i1> -> !ac.var<i1>
    %stored = ac.table.get @window[%slot] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    %false = ac.var.constant false as !ac.var<i1>
    %cleared = ac.var.with %stored, %false field "valid" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.versioned_table.propose @window [%slot] = %cleared when %kill : !ac.var<i1>
        ref %generation, %transaction_epoch : !ac.var<i2>, !ac.var<i3>
        attempt %attempt : !ac.var<i2> action "invalidate" mode "field"
        write_fields ["valid"] {ac.arbitration = #ac.writer_priority<2>}
        : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Recovery>>) -> ()

  ac.rule %retain_in depths [] latencies [] name "retain" stable_id "retain" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Entry>>):
    %slot = ac.var.constant 0 : i1 as !ac.var<i1>
    %generation = ac.var.get %item field "generation" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i2>
    %epoch = ac.var.get %item field "recovery_epoch" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i3>
    %attempt = ac.var.get %item field "attempt" : !ac.var<!ac.struct<@types::@Entry>> -> !ac.var<i2>
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.versioned_table.propose @retained [%slot] = %item when %true : !ac.var<i1>
        ref %generation, %epoch : !ac.var<i2>, !ac.var<i3>
        attempt %attempt : !ac.var<i2> action "retain" mode "replace"
        write_fields ["slot", "valid", "generation", "recovery_epoch", "attempt", "payload"]
        {ac.arbitration = #ac.writer_priority<0>}
        : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Entry>>) -> ()

  ac.rule %consume_in depths [] latencies [] name "consume" stable_id "consume" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Completion>>):
    %slot = ac.var.constant 0 : i1 as !ac.var<i1>
    %generation = ac.var.get %item field "generation" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i2>
    %epoch = ac.var.get %item field "recovery_epoch" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i3>
    %attempt = ac.var.get %item field "attempt" : !ac.var<!ac.struct<@types::@Completion>> -> !ac.var<i2>
    %stored = ac.table.get @retained[%slot] : !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    %false = ac.var.constant false as !ac.var<i1>
    %cleared = ac.var.with %stored, %false field "valid" : !ac.var<!ac.struct<@types::@Entry>>, !ac.var<i1> -> !ac.var<!ac.struct<@types::@Entry>>
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.versioned_table.propose @retained [%slot] = %cleared when %true : !ac.var<i1>
        ref %generation, %epoch : !ac.var<i2>, !ac.var<i3>
        attempt %attempt : !ac.var<i2> action "consume" mode "field"
        write_fields ["valid"] {ac.arbitration = #ac.writer_priority<1>}
        : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return
  } : (!ac.queue<!ac.struct<@types::@Completion>>) -> ()

  %read_out = ac.rule %read_in depths [2] latencies [1] name "read" stable_id "read" domain "cycle" type exact {
  ^body(%ref: !ac.var<!ac.struct<@types::@ReadRef>>):
    %use_retained = ac.var.get %ref field "retained" : !ac.var<!ac.struct<@types::@ReadRef>> -> !ac.var<i1>
    %slot = ac.var.get %ref field "slot" : !ac.var<!ac.struct<@types::@ReadRef>> -> !ac.var<i1>
    %generation = ac.var.get %ref field "generation" : !ac.var<!ac.struct<@types::@ReadRef>> -> !ac.var<i2>
    %epoch = ac.var.get %ref field "recovery_epoch" : !ac.var<!ac.struct<@types::@ReadRef>> -> !ac.var<i3>
    %attempt = ac.var.get %ref field "attempt" : !ac.var<!ac.struct<@types::@ReadRef>> -> !ac.var<i2>
    %payload_window, %valid_window = ac.versioned_table.lookup @window[%slot] ref %generation, %epoch : !ac.var<i2>, !ac.var<i3> attempt %attempt : !ac.var<i2> : !ac.var<i1> -> !ac.var<i8>, !ac.var<i1>
    %zero = ac.var.constant 0 : i1 as !ac.var<i1>
    %payload_retained, %valid_retained = ac.versioned_table.lookup @retained[%zero] ref %generation, %epoch : !ac.var<i2>, !ac.var<i3> attempt %attempt : !ac.var<i2> : !ac.var<i1> -> !ac.var<i8>, !ac.var<i1>
    %payload = ac.var.select %use_retained, %payload_retained, %payload_window : !ac.var<i1>, !ac.var<i8> -> !ac.var<i8>
    %qualified = ac.var.select %use_retained, %valid_retained, %valid_window : !ac.var<i1>, !ac.var<i1> -> !ac.var<i1>
    ac.rule.condition %qualified : !ac.var<i1>
    %ready = ac.marker.obligation %payload state pending resolver handshake
        origin "read:return" path "true" : !ac.var<i8>
    ac.rule.return %ready : !ac.var<i8>
  } {ac.name = "read_out"} : (!ac.queue<!ac.struct<@types::@ReadRef>>) -> !ac.queue<i8>
  ac.sink %read_out {ac.name = "read_sink"} : !ac.queue<i8>
}

// PLAN: "stale_obligation_id":"no_stale_update:window:complete"
// PLAN: "versioned_action":"qualified_update"
// PLAN: "recovery_domain":"speculation"
// PLAN: "versioned":true
// PYC: obligation_id = "no_stale_update:window:complete:slot0"
// PYC: obligation_kind = "no_stale_update"
// PYC: obligation_id = "no_stale_update:window:complete:slot1"
// PYC: pyc.cmp
// SVA: obligation_no_stale_update_window_complete_slot0: assert property
// SVA: obligation_no_stale_update_window_complete_slot0_coverage: cover property
// SVA: obligation_no_stale_update_window_complete_slot1: assert property
// SVA: obligation_no_stale_update_window_complete_slot1_coverage: cover property
// CPP: const bool state_window_identity_match = static_cast<bool>(state_window_committed.valid)
// CPP-SAME: state_window_committed.generation == state_window_ref_generation
// CPP-SAME: state_window_committed.recovery_epoch == state_window_ref_epoch
// CPP-SAME: state_window_committed.attempt == state_window_ref_attempt
// CPP: state_window_write_present = state_window_requested && state_window_identity_match
// CPP: auto v9 = static_cast<bool>(v8) && ((v6 != v1) || (v3 < v4));
// EXEC: versioned recovery PASS
// RTL-EXEC: versioned recovery RTL PASS
