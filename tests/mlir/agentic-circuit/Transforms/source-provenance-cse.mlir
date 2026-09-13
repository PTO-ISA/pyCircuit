// RUN: %acir_opt --pass-pipeline='builtin.module(ac-inline-pure-helpers,cse,ac-freeze-topology)' %s | %FileCheck %s

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "source_cse"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^body(%item: !ac.var<i8>):
    %one = ac.var.constant 1 : i8 as !ac.var<i8> loc("src/model.py":5:11)
    %left = ac.var.add %item, %one : !ac.var<i8> loc("src/model.py":6:12)
    %also_one = ac.var.constant 1 : i8 as !ac.var<i8> loc("src/model.py":5:21)
    %right = ac.var.add %item, %also_one : !ac.var<i8> loc("src/model.py":7:13)
    %sum = ac.var.add %left, %right : !ac.var<i8> loc("src/model.py":8:12)
    ac.transform.yield %sum : !ac.var<i8>
  } {ac.name = "output"} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}

// CHECK-COUNT-1: ac.var.add %arg0, %{{[0-9]+}}
// CHECK-SAME: ac.source_provenance = [{frames = [{column = 12 : i64, file = "src/model.py", kind = "statement", line = 6 : i64}]}, {frames = [{column = 13 : i64, file = "src/model.py", kind = "statement", line = 7 : i64}]}]
