// RUN: %split_file %s %t
// RUN: %not %acir_opt --verify-each=false --ac-inline-pure-helpers %t/external.mlir 2>&1 | %FileCheck %s --check-prefix=EXTERNAL
// RUN: %not %acir_opt --verify-each=false --ac-inline-pure-helpers %t/recursive.mlir 2>&1 | %FileCheck %s --check-prefix=RECURSIVE
// RUN: %not %acir_opt --verify-each=false --ac-inline-pure-helpers %t/loop.mlir 2>&1 | %FileCheck %s --check-prefix=LOOP
// RUN: %not %acir_opt --verify-each=false --ac-inline-pure-helpers %t/state.mlir 2>&1 | %FileCheck %s --check-prefix=STATE

//--- external.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "external"} {
  func.func private @external(%arg0: !ac.var<i8>) -> !ac.var<i8>
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^bb0(%item: !ac.var<i8>):
    %result = func.call @external(%item) : (!ac.var<i8>) -> !ac.var<i8>
    ac.transform.yield %result : !ac.var<i8>
  } {ac.output_names = ["output"]} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}
// EXTERNAL: Queue/rule func.call callee '@external' has no body and cannot be proven effect-free

//--- recursive.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "recursive"} {
  func.func private @recursive(%arg0: !ac.var<i8>) -> !ac.var<i8>
      attributes {ac.inline = true} {
    %result = func.call @recursive(%arg0) : (!ac.var<i8>) -> !ac.var<i8>
    return %result : !ac.var<i8>
  }
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^bb0(%item: !ac.var<i8>):
    %result = func.call @recursive(%item) : (!ac.var<i8>) -> !ac.var<i8>
    ac.transform.yield %result : !ac.var<i8>
  } {ac.output_names = ["output"]} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}
// RECURSIVE: recursive func.call purity cycle: @recursive -> @recursive

//--- loop.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "loop"} {
  func.func private @loop(%arg0: !ac.var<i8>) -> !ac.var<i8>
      attributes {ac.inline = true} {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    scf.for %i = %c0 to %c1 step %c1 {
    }
    return %arg0 : !ac.var<i8>
  }
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^bb0(%item: !ac.var<i8>):
    %result = func.call @loop(%item) : (!ac.var<i8>) -> !ac.var<i8>
    ac.transform.yield %result : !ac.var<i8>
  } {ac.output_names = ["output"]} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}
// LOOP: is not legal in a pure Queue/rule helper

//--- state.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "state"} {
  ac.var.decl @state type i8 init 0 : i8 owner "/" stable_id "var/state"
  func.func private @read_state(%arg0: !ac.var<i8>) -> !ac.var<i8>
      attributes {ac.inline = true} {
    %value = ac.var.read @state : !ac.var<i8>
    return %value : !ac.var<i8>
  }
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  %output = ac.transform %input depths [1] latencies [1] {
  ^bb0(%item: !ac.var<i8>):
    %result = func.call @read_state(%item) : (!ac.var<i8>) -> !ac.var<i8>
    ac.transform.yield %result : !ac.var<i8>
  } {ac.output_names = ["output"]} : (!ac.queue<i8>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}
// STATE: 'ac.var.read' op is not legal in a pure Queue/rule helper
