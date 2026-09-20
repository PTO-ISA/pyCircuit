// RUN: %acir_opt %s | %FileCheck %s

module {
  ac.type_scope @types {
    "ac.type_alias"() <{sym_name = "TxnRef", target = !ac.transaction_ref<2, 2, 3>}> : () -> ()
    "ac.type_alias"() <{sym_name = "Attempt", target = !ac.execution_attempt<!ac.transaction_ref<2, 2, 3>, 2>}> : () -> ()
    ac.struct @Entry fields [
      {name = "valid", type = i1},
      {name = "generation", type = i2},
      {name = "recovery_epoch", type = i3},
      {name = "attempt", type = i2},
      {name = "payload", type = i8}
    ]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 16 : i64}>}

  ac.recovery_domain @speculation epoch_bits 3 initial 0
  ac.typed_identity @transaction width 8
  ac.typed_identity @attempt width 2 parent @transaction
  ac.checkpoint @rename_checkpoint domain @speculation entries 4 payload i8
  ac.retained_result @engine_result domain @speculation identity @transaction
      attempt_bits 2 payload i8

  ac.table @window entry !ac.struct<@types::@Entry> entries 4 init 0
      owner "/" stable_id "table/window" {
    recovery_domain = @speculation,
    identity = @transaction,
    checkpoint = @rename_checkpoint,
    retained_result = @engine_result,
    generation_bits = 2 : i64,
    epoch_bits = 3 : i64,
    attempt_bits = 2 : i64,
    valid_field = "valid",
    generation_field = "generation",
    epoch_field = "recovery_epoch",
    attempt_field = "attempt",
    payload_field = "payload"
  }
  %slot = ac.var.constant 0 : i2 as !ac.var<i2>
  %entry = ac.table.get @window[%slot]
      : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
}

// CHECK: target = !ac.transaction_ref<2, 2, 3>
// CHECK: target = !ac.execution_attempt<!ac.transaction_ref<2, 2, 3>, 2>
// CHECK: ac.recovery_domain @speculation epoch_bits 3 initial 0
// CHECK: ac.typed_identity @transaction width 8
// CHECK: ac.typed_identity @attempt width 2 parent @transaction
// CHECK: ac.checkpoint @rename_checkpoint domain @speculation entries 4 payload i8
// CHECK: ac.retained_result @engine_result domain @speculation identity @transaction attempt_bits 2 payload i8
// CHECK: ac.table @window entry !ac.struct<@types::@Entry> entries 4 init 0 owner "/" stable_id "table/window"
// CHECK-SAME: generation_bits = 2 : i64
// CHECK-SAME: recovery_domain = @speculation
