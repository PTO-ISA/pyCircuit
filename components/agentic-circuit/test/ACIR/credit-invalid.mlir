// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/credits.mlir 2>&1 | %FileCheck %s --check-prefix=CREDITS
// RUN: %not %acir_opt %t/cost.mlir 2>&1 | %FileCheck %s --check-prefix=COST
// RUN: %not %acir_opt %t/effect.mlir 2>&1 | %FileCheck %s --check-prefix=EFFECT

// CREDITS: error: 'ac.credit' op credits, depth, and latency must be positive
// COST: error: 'ac.credit' op cost must yield an integer Var with width at most 64
// EFFECT: error: 'ac.credit' op cost operation 'ac.assert' must be pure

//--- credits.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.credit %input credits 0 depth 1 latency 1 cost {
  ^cost(%item: !ac.var<i8>): ac.credit.yield %item : !ac.var<i8>
  } : !ac.queue<i8> -> !ac.queue<i8>
}

//--- cost.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.credit %input credits 1 depth 1 latency 1 cost {
  ^cost(%item: !ac.var<i8>):
    %wide = ac.var.constant 1 : i128 as !ac.var<i128>
    ac.credit.yield %wide : !ac.var<i128>
  } : !ac.queue<i8> -> !ac.queue<i8>
}

//--- effect.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  %bad = ac.credit %input credits 1 depth 1 latency 1 cost {
  ^cost(%item: !ac.var<i8>):
    %condition = arith.constant true
    ac.assert %condition, "illegal"
    ac.credit.yield %item : !ac.var<i8>
  } : !ac.queue<i8> -> !ac.queue<i8>
}
