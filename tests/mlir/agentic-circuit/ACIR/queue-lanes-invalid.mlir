// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/zero-lanes.mlir 2>&1 | %FileCheck %s --check-prefix=LANES
// RUN: %not %acir_opt %t/zero-rate.mlir 2>&1 | %FileCheck %s --check-prefix=RATE
// RUN: %not %acir_opt %t/rate-lanes.mlir 2>&1 | %FileCheck %s --check-prefix=RATE
// RUN: %not %acir_opt %t/rate-depth.mlir 2>&1 | %FileCheck %s --check-prefix=DEPTH

// LANES: queue lane count must be positive
// RATE: queue rate must be positive and not exceed lanes
// DEPTH: Queue rate must not exceed its independently declared depth

//--- zero-lanes.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8, lanes=0>
  ac.sink %input : !ac.queue<i8, lanes=0>
}

//--- zero-rate.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8, lanes=1, rate=0>
  ac.sink %input : !ac.queue<i8, lanes=1, rate=0>
}

//--- rate-lanes.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 4 latency 1 : !ac.queue<i8, lanes=2, rate=3>
  ac.sink %input : !ac.queue<i8, lanes=2, rate=3>
}

//--- rate-depth.mlir
module attributes {ac.contract_epoch = "0.5"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i8, lanes=4, rate=2>
  ac.sink %input : !ac.queue<i8, lanes=4, rate=2>
}
