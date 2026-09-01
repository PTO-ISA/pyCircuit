// RUN: %acir_queue_cxxgen %s > %t.cpp
// RUN: %FileCheck %s --check-prefix=GFSIM < %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/include -c %t.cpp -o %t.o
// RUN: %acir_queue_pycgen %s | %FileCheck %s --check-prefix=PYC

module attributes {ac.contract_epoch = "0.4", ac.system = "atomic_sum"} {
  %left = ac.source depth 2 latency 1 {ac.name = "left"} : !ac.queue<i64>
  %right = ac.source depth 2 latency 1 {ac.name = "right"} : !ac.queue<i64>
  %sum = ac.transform %left, %right depths [2] latencies [1] {
  ^transform(%item0: !ac.var<i64>, %item1: !ac.var<i64>):
    %result = ac.var.add %item0, %item1 : !ac.var<i64>
    ac.transform.yield %result : !ac.var<i64>
  } {ac.output_names = ["sum"]} : (!ac.queue<i64>, !ac.queue<i64>) -> !ac.queue<i64>
  ac.sink %sum {ac.name = "sink_0"} : !ac.queue<i64>
}

// GFSIM: std::tuple<std::int64_t> operator()(const std::int64_t &item, const std::int64_t &item1)
// GFSIM: gfsim::QueueAtomicTransform<block_0_policy, std::tuple<std::int64_t, std::int64_t>, std::tuple<std::int64_t>> block_0_;

// PYC: = pyc.wire : i1
// PYC: = pyc.add
// PYC: pyc.assign
