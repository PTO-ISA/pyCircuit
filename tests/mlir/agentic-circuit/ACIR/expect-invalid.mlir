// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/message.mlir 2>&1 | %FileCheck %s --check-prefix=MESSAGE
// RUN: %not %acir_opt %t/predicate.mlir 2>&1 | %FileCheck %s --check-prefix=PREDICATE

// MESSAGE: error: 'ac.expect' op message must be non-empty
// PREDICATE: error: 'ac.expect' op predicate must terminate with an i1 ac.expect.yield

//--- message.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  ac.expect %input message "" {
  ^predicate(%item: !ac.var<i8>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.expect.yield %true : !ac.var<i1>
  } : !ac.queue<i8>
}

//--- predicate.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8>
  ac.expect %input message "bad" {
  ^predicate(%item: !ac.var<i8>):
    ac.expect.yield %item : !ac.var<i8>
  } : !ac.queue<i8>
}
