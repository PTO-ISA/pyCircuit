// RUN: %not %acir_opt %s 2>&1 | %FileCheck %s

// CHECK: error: 'ac.observe' op name must be non-empty

module attributes {ac.contract_epoch = "0.4"} {
  %input = ac.source depth 2 latency 1 : !ac.queue<i64>
  ac.observe %input name "" : !ac.queue<i64>
}
