// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %t/latency.mlir -o %t.latency.frozen.mlir
// RUN: %not %acir_queue_pycgen %t.latency.frozen.mlir 2>&1 | %FileCheck %s --check-prefix=LATENCY
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-freeze-topology)' %t/topology.mlir -o %t.topology.frozen.mlir
// RUN: %not %acir_queue_pycgen %t.topology.frozen.mlir 2>&1 | %FileCheck %s --check-prefix=TOPOLOGY

// LATENCY: multi-lane transform chain requires uniform payload/lanes/rate, latency=1, and depth>=rate
// TOPOLOGY: multi-lane PYC requires one source and one sink boundary

//--- latency.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "bad_latency"} {
  %bundle = ac.source depth 4 latency 2 {ac.name = "bundle"}
      : !ac.queue<i8, lanes=3, rate=2>
  ac.sink %bundle {ac.name = "sink"} : !ac.queue<i8, lanes=3, rate=2>
}

//--- topology.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "bad_topology"} {
  %input = ac.source depth 4 latency 1 {ac.name = "input"}
      : !ac.queue<i8, lanes=3, rate=2>
  %left, %right = ac.fork %input depths [4, 4] latencies [1, 1]
      {ac.output_names = ["left", "right"]}
      : !ac.queue<i8, lanes=3, rate=2>
      -> (!ac.queue<i8, lanes=3, rate=2>, !ac.queue<i8, lanes=3, rate=2>)
  ac.sink %left {ac.name = "left_sink"} : !ac.queue<i8, lanes=3, rate=2>
  ac.sink %right {ac.name = "right_sink"} : !ac.queue<i8, lanes=3, rate=2>
}
