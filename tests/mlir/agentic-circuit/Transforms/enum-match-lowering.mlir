// RUN: %acir_opt %s -ac-lower-value-contracts | %FileCheck %s

builtin.module  {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run", "wait"] values [1 : i64, 3 : i64, 7 : i64] width 3
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  %selector = ac.var.enum @types::@Mode "run" : !ac.var<!ac.enum<@types::@Mode>>
  %idle = ac.var.constant 10 : i8 as !ac.var<i8>
  %run = ac.var.constant 11 : i8 as !ac.var<i8>
  %wait = ac.var.constant 12 : i8 as !ac.var<i8>
  %invalid = ac.var.constant 255 : i8 as !ac.var<i8>
  %result = ac.var.enum_match %selector, %idle, %run, %wait, %invalid cases ["idle", "run", "wait"] : !ac.var<!ac.enum<@types::@Mode>>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8> -> !ac.var<i8>
}

// The tournament is built through named locals on purpose: passing the two
// creations as sibling call arguments left their order in the block up to
// the host compiler's argument evaluation order, so this expected
// select/or interleaving differed between toolchains.
// CHECK-NOT: ac.var.enum_match
// CHECK-COUNT-3: ac.var.cmp "eq"
// CHECK: ac.var.select
// CHECK: ac.var.or
// CHECK: ac.var.select
// CHECK: ac.var.or
// CHECK: ac.var.select
