// RUN: %acir_opt --canonicalize %s | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  func.func @test(%value: !ac.var<i8>) -> (!ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>) {
    %eight = ac.var.constant 8 : i8 as !ac.var<i8>
    %zero = ac.var.constant 0 : i8 as !ac.var<i8>
    %quotient = ac.var.udiv %value, %eight : !ac.var<i8>
    %remainder = ac.var.urem %value, %eight : !ac.var<i8>
    %divide_zero = ac.var.udiv %value, %zero : !ac.var<i8>
    %remainder_zero = ac.var.urem %value, %zero : !ac.var<i8>
    func.return %quotient, %remainder, %divide_zero, %remainder_zero
        : !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>
  }
}

// CHECK: %[[SHIFT:.*]] = ac.var.constant 3 : i8 as !ac.var<i8>
// CHECK: ac.var.shr %{{.*}}, %[[SHIFT]] : !ac.var<i8>
// CHECK: %[[MASK:.*]] = ac.var.constant 7 : i8 as !ac.var<i8>
// CHECK: ac.var.and %{{.*}}, %[[MASK]] : !ac.var<i8>
// CHECK: ac.var.udiv %{{.*}}, %{{.*}} : !ac.var<i8>
// CHECK: ac.var.urem %{{.*}}, %{{.*}} : !ac.var<i8>
