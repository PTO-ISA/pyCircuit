// RUN: %acir_opt %s | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  %lhs = ac.var.constant 0x8000000000000001 : i64 as !ac.var<i64>
  %rhs = ac.var.constant 3 : i64 as !ac.var<i64>
  %aux = ac.var.constant 5 : i64 as !ac.var<i64>
  %width = ac.var.constant 8 : i7 as !ac.var<i7>
  %offset = ac.var.constant 4 : i6 as !ac.var<i6>
  %pred = ac.var.constant 1 : i1 as !ac.var<i1>
  %negate = ac.var.constant 0 : i1 as !ac.var<i1>
  %addw = ac.var.addw %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %subw = ac.var.subw %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %andw = ac.var.andw %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %orw = ac.var.orw %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %xorw = ac.var.xorw %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %sll = ac.var.sll %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %srl = ac.var.srl %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %sra = ac.var.sra %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %sllw = ac.var.sllw %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %srlw = ac.var.srlw %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %sraw = ac.var.sraw %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %smin = ac.var.smin %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %umin = ac.var.umin %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %smax = ac.var.smax %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %umax = ac.var.umax %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %mulw = ac.var.mulw %lhs, %rhs : !ac.var<i64> -> !ac.var<i64>
  %madd = ac.var.madd %lhs, %rhs, %aux : !ac.var<i64> -> !ac.var<i64>
  %maddw = ac.var.maddw %lhs, %rhs, %aux : !ac.var<i64> -> !ac.var<i64>
  %msub = ac.var.msub %lhs, %rhs, %aux : !ac.var<i64> -> !ac.var<i64>
  %bxs = ac.var.bitfield_extract %lhs, %width, %offset signed_mode true : !ac.var<i64>, !ac.var<i7>, !ac.var<i6> -> !ac.var<i64>
  %bxu = ac.var.bitfield_extract %lhs, %width, %offset signed_mode false : !ac.var<i64>, !ac.var<i7>, !ac.var<i6> -> !ac.var<i64>
  %bcnt = ac.var.bitfield_popcount %lhs, %width, %offset : !ac.var<i64>, !ac.var<i7>, !ac.var<i6> -> !ac.var<i64>
  %clz = ac.var.bitfield_clz %lhs, %width, %offset : !ac.var<i64>, !ac.var<i7>, !ac.var<i6> -> !ac.var<i64>
  %ctz = ac.var.bitfield_ctz %lhs, %width, %offset : !ac.var<i64>, !ac.var<i7>, !ac.var<i6> -> !ac.var<i64>
  %bic = ac.var.bitfield_clear %lhs, %width, %offset : !ac.var<i64>, !ac.var<i7>, !ac.var<i6> -> !ac.var<i64>
  %bis = ac.var.bitfield_set %lhs, %width, %offset : !ac.var<i64>, !ac.var<i7>, !ac.var<i6> -> !ac.var<i64>
  %rev = ac.var.bitfield_reverse_bytes %lhs, %width, %offset : !ac.var<i64>, !ac.var<i7>, !ac.var<i6> -> !ac.var<i64>
  %bfi = ac.var.bitfield_insert %lhs, %rhs, %width, %offset : !ac.var<i64>, !ac.var<i64>, !ac.var<i7>, !ac.var<i6> -> !ac.var<i64>
  %sext = ac.var.sext_low %lhs, %width : !ac.var<i64>, !ac.var<i7> -> !ac.var<i64>
  %zext = ac.var.zext_low %lhs, %width : !ac.var<i64>, !ac.var<i7> -> !ac.var<i64>
  %csel = ac.var.csel %pred, %lhs, %rhs, %negate : !ac.var<i1>, !ac.var<i64>, !ac.var<i64>, !ac.var<i1> -> !ac.var<i64>
}

// CHECK: ac.var.addw
// CHECK: ac.var.subw
// CHECK: ac.var.andw
// CHECK: ac.var.orw
// CHECK: ac.var.xorw
// CHECK: ac.var.sll
// CHECK: ac.var.srl
// CHECK: ac.var.sra
// CHECK: ac.var.sllw
// CHECK: ac.var.srlw
// CHECK: ac.var.sraw
// CHECK: ac.var.smin
// CHECK: ac.var.umin
// CHECK: ac.var.smax
// CHECK: ac.var.umax
// CHECK: ac.var.mulw
// CHECK: ac.var.madd
// CHECK: ac.var.maddw
// CHECK: ac.var.msub
// CHECK: ac.var.bitfield_extract
// CHECK: ac.var.bitfield_popcount
// CHECK: ac.var.bitfield_clz
// CHECK: ac.var.bitfield_ctz
// CHECK: ac.var.bitfield_clear
// CHECK: ac.var.bitfield_set
// CHECK: ac.var.bitfield_reverse_bytes
// CHECK: ac.var.bitfield_insert
// CHECK: ac.var.sext_low
// CHECK: ac.var.zext_low
// CHECK: ac.var.csel
