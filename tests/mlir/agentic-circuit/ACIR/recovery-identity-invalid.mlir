// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/epoch-width.mlir 2>&1 | %FileCheck %s --check-prefix=EPOCH
// RUN: %not %acir_opt %t/missing-field.mlir 2>&1 | %FileCheck %s --check-prefix=FIELD
// RUN: %not %acir_opt %t/unqualified-write.mlir 2>&1 | %FileCheck %s --check-prefix=WRITE
// RUN: %not %acir_opt %t/plain-table-ref.mlir 2>&1 | %FileCheck %s --check-prefix=PLAIN
// RUN: %not %acir_opt %t/ref-width.mlir 2>&1 | %FileCheck %s --check-prefix=REF-WIDTH
// RUN: %not %acir_opt %t/attempt-width.mlir 2>&1 | %FileCheck %s --check-prefix=ATTEMPT-WIDTH
// RUN: %not %acir_opt %t/checkpoint.mlir 2>&1 | %FileCheck %s --check-prefix=CHECKPOINT
// RUN: %not %acir_opt %t/retained.mlir 2>&1 | %FileCheck %s --check-prefix=RETAINED
// RUN: %not %acir_opt %t/recovery-event.mlir 2>&1 | %FileCheck %s --check-prefix=RECOVERY-EVENT
// RUN: %not %acir_opt %t/kill-set.mlir 2>&1 | %FileCheck %s --check-prefix=KILL-SET
// RUN: %not %acir_opt %t/lookup.mlir 2>&1 | %FileCheck %s --check-prefix=LOOKUP

//--- epoch-width.mlir
module {
  ac.recovery_domain @speculation epoch_bits 0 initial 0
}
// EPOCH: 'ac.recovery_domain' op epoch_bits must be in [1, 64]

//--- missing-field.mlir
module {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "valid", type = i1}, {name = "generation", type = i2}, {name = "payload", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 11 : i64}>}
  ac.recovery_domain @speculation epoch_bits 3 initial 0
  ac.typed_identity @transaction width 8
  ac.table @window entry !ac.struct<@types::@Entry> entries 4 init 0
      owner "/" stable_id "table/window" {
    recovery_domain = @speculation, identity = @transaction,
    generation_bits = 2 : i64, epoch_bits = 3 : i64,
    valid_field = "valid", generation_field = "generation",
    epoch_field = "recovery_epoch", payload_field = "payload"
  }
}
// FIELD: 'ac.table' op epoch field must exist with exact i3 type

//--- unqualified-write.mlir
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "unqualified_write"} {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "valid", type = i1}, {name = "generation", type = i2}, {name = "recovery_epoch", type = i3}, {name = "payload", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 14 : i64}>}
  ac.recovery_domain @speculation epoch_bits 3 initial 0
  ac.typed_identity @transaction width 8
  ac.table @window entry !ac.struct<@types::@Entry> entries 2 init 0 owner "/" stable_id "table/window" {
    recovery_domain = @speculation, identity = @transaction,
    generation_bits = 2 : i64, epoch_bits = 3 : i64,
    valid_field = "valid", generation_field = "generation",
    epoch_field = "recovery_epoch", payload_field = "payload"
  }
  %input = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@types::@Entry>>
  %output = ac.rule %input depths [1] latencies [1] name "write" stable_id "write" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@types::@Entry>>):
    %slot = ac.var.constant 0 : i1 as !ac.var<i1>
    ac.table.propose @window [%slot] = %item mode "replace" write_fields ["valid", "generation", "recovery_epoch", "payload"] : !ac.var<i1>, !ac.var<!ac.struct<@types::@Entry>>
    ac.rule.return %item : !ac.var<!ac.struct<@types::@Entry>>
  } : (!ac.queue<!ac.struct<@types::@Entry>>) -> !ac.queue<!ac.struct<@types::@Entry>>
  ac.sink %output : !ac.queue<!ac.struct<@types::@Entry>>
}
// WRITE: 'ac.table.propose' op versioned Table writes require ac.versioned_table.propose

//--- plain-table-ref.mlir
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "plain_ref"} {
  ac.table @plain entry i8 entries 1 init 0 owner "/" stable_id "table/plain"
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %output = ac.rule %input depths [1] latencies [1] name "write" stable_id "write" domain "cycle" type exact {
  ^body(%item: !ac.var<i8>):
    %slot = ac.var.constant 0 : i1 as !ac.var<i1>
    %gen = ac.var.constant 0 : i2 as !ac.var<i2>
    %epoch = ac.var.constant 0 : i3 as !ac.var<i3>
    %when = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %when : !ac.var<i1>
    ac.versioned_table.propose @plain [%slot] = %item when %when : !ac.var<i1>
        ref %gen, %epoch : !ac.var<i2>, !ac.var<i3>
        action "qualified_update" mode "replace" write_fields ["$entry"]
        : !ac.var<i1>, !ac.var<i8>
    ac.rule.output %item when %when ordinal 0 : !ac.var<i8>, !ac.var<i1>
    ac.rule.return %item : !ac.var<i8>
  } : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output : !ac.queue<i8>
}
// PLAIN: 'ac.versioned_table.propose' op ac.versioned_table.propose requires a versioned Table

//--- ref-width.mlir
module {
  ac.type_scope @types {
    "ac.type_alias"() <{sym_name = "BadRef", target = !ac.transaction_ref<0, 2, 3>}> : () -> ()
  }
}
// REF-WIDTH: transaction_ref slot/generation/epoch widths must be positive

//--- attempt-width.mlir
module {
  ac.type_scope @types {
    "ac.type_alias"() <{sym_name = "BadAttempt", target = !ac.execution_attempt<!ac.transaction_ref<1, 2, 3>, 0>}> : () -> ()
  }
}
// ATTEMPT-WIDTH: execution_attempt width must be in [1, 64]

//--- checkpoint.mlir
module {
  ac.checkpoint @bad domain @missing entries 1 payload i8
}
// CHECKPOINT: 'ac.checkpoint' op recovery_domain must resolve to ac.recovery_domain

//--- retained.mlir
module {
  ac.recovery_domain @speculation epoch_bits 3 initial 0
  ac.retained_result @bad domain @speculation identity @missing attempt_bits 2 payload i8
}
// RETAINED: 'ac.retained_result' op identity must resolve to ac.typed_identity

//--- recovery-event.mlir
module {
  ac.recovery_domain @speculation epoch_bits 3 initial 0
  %valid = ac.var.constant true as !ac.var<i1>
  %epoch = ac.var.constant 0 : i2 as !ac.var<i2>
  %checkpoint = ac.var.constant 0 : i2 as !ac.var<i2>
  %boundary = ac.var.constant 0 : i1 as !ac.var<i1>
  %event = ac.recovery.event %valid epoch %epoch checkpoint %checkpoint boundary %boundary domain @speculation cause "bad" : !ac.var<i1>, !ac.var<i2>, !ac.var<i2>, !ac.var<i1> -> !ac.var<i1>
}
// RECOVERY-EVENT: 'ac.recovery.event' op next epoch must match the recovery domain width

//--- kill-set.mlir
module {
  %valid = ac.var.constant true as !ac.var<i1>
  %epoch = ac.var.constant 0 : i3 as !ac.var<i3>
  %slot = ac.var.constant 0 : i1 as !ac.var<i1>
  %killed = ac.kill_set %valid transaction_epoch %epoch next_epoch %epoch transaction_slot %slot boundary %slot policy "manual" : !ac.var<i1>, !ac.var<i3>, !ac.var<i3>, !ac.var<i1>, !ac.var<i1> -> !ac.var<i1>
}
// KILL-SET: 'ac.kill_set' op kill-set policy must be epoch_mismatch_or_younger

//--- lookup.mlir
module {
  ac.table @plain entry i8 entries 1 init 0 owner "/" stable_id "table/plain"
  %slot = ac.var.constant 0 : i1 as !ac.var<i1>
  %generation = ac.var.constant 0 : i2 as !ac.var<i2>
  %epoch = ac.var.constant 0 : i3 as !ac.var<i3>
  %payload, %valid = ac.versioned_table.lookup @plain[%slot] ref %generation, %epoch : !ac.var<i2>, !ac.var<i3> : !ac.var<i1> -> !ac.var<i8>, !ac.var<i1>
}
// LOOKUP: 'ac.versioned_table.lookup' op lookup requires a versioned Table
