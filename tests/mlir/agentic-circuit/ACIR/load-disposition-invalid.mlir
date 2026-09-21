// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/pending-width.mlir 2>&1 | %FileCheck %s --check-prefix=PENDING
// RUN: %not %acir_opt %t/disjoint-width.mlir 2>&1 | %FileCheck %s --check-prefix=DISJOINT
// RUN: %not %acir_opt %t/wait-width.mlir 2>&1 | %FileCheck %s --check-prefix=WAIT
// RUN: %not %acir_opt %t/killed-width.mlir 2>&1 | %FileCheck %s --check-prefix=KILLED

//--- pending-width.mlir
module {
  %pending = ac.var.constant 0 : i3 as !ac.var<i3>
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %wait, %bypass, %forward, %replay, %stale = "ac.load_disposition"(
      %pending, %mask, %mask, %mask, %mask, %mask, %mask) <{lanes = 4 : i64}>
      : (!ac.var<i3>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
         !ac.var<i4>, !ac.var<i4>) -> (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
                                       !ac.var<i4>, !ac.var<i4>)
}
// PENDING: 'ac.load_disposition' op pending mask must be an exact !ac.var<i4>

//--- disjoint-width.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %disjoint = ac.var.constant 0 : i2 as !ac.var<i2>
  %wait, %bypass, %forward, %replay, %stale = "ac.load_disposition"(
      %mask, %mask, %disjoint, %mask, %mask, %mask, %mask) <{lanes = 4 : i64}>
      : (!ac.var<i4>, !ac.var<i4>, !ac.var<i2>, !ac.var<i4>, !ac.var<i4>,
         !ac.var<i4>, !ac.var<i4>) -> (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
                                       !ac.var<i4>, !ac.var<i4>)
}
// DISJOINT: 'ac.load_disposition' op disjoint proof mask must be an exact !ac.var<i4>

//--- wait-width.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %wait, %bypass, %forward, %replay, %stale = "ac.load_disposition"(
      %mask, %mask, %mask, %mask, %mask, %mask, %mask) <{lanes = 4 : i64}>
      : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
         !ac.var<i4>, !ac.var<i4>) -> (!ac.var<i1>, !ac.var<i4>, !ac.var<i4>,
                                       !ac.var<i4>, !ac.var<i4>)
}
// WAIT: 'ac.load_disposition' op wait mask must be an exact !ac.var<i4>

//--- killed-width.mlir
module {
  %mask = ac.var.constant 0 : i4 as !ac.var<i4>
  %killed = ac.var.constant 0 : i2 as !ac.var<i2>
  %wait, %bypass, %forward, %replay, %stale = "ac.load_disposition"(
      %mask, %mask, %mask, %mask, %mask, %mask, %killed) <{lanes = 4 : i64}>
      : (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
         !ac.var<i4>, !ac.var<i2>) -> (!ac.var<i4>, !ac.var<i4>, !ac.var<i4>,
                                       !ac.var<i4>, !ac.var<i4>)
}
// KILLED: 'ac.load_disposition' op killed mask must be an exact !ac.var<i4>
