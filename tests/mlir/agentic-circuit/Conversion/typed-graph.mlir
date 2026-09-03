// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/stateful-graph.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.protocol @wire {
    ac.role @sender dual @receiver cardinality "exclusive"
    ac.role @receiver dual @sender cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.event @send from @sender to @receiver payload i8 action "offer"
    ac.transition from @idle to @idle on @send transfer true retain false guard {}
  }
  ac.interface @Wire {
    ac.role @source dual @sink cardinality "exclusive"
    ac.role @sink dual @source cardinality "exclusive"
    ac.port @data : !ac.channel<i8, @wire> from @source to @sink
        protocol_roles @sender to @receiver
  }
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module.extern @Consumer : (!ac.endpoint<@Wire, @source>) -> () parameters {}
      implementation {registry = "cpp", name = "Consumer"}
  ac.module.extern @Producer : () -> !ac.endpoint<@Wire, @source> parameters {}
      implementation {registry = "cpp", name = "Producer"}
  ac.module @Top() parameters {} graph {
    %out = ac.instance @producer of @Producer() static {}
        id "producer" path "producer" : () -> !ac.endpoint<@Wire, @source>
    ac.instance @consumer of @Consumer(%out) static {}
        id "consumer" path "consumer" : (!ac.endpoint<@Wire, @source>) -> ()
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
}

// CHECK: %[[CONSUMER:.+]] = acsim.instance @consumer target @Consumer
// CHECK: %[[PRODUCER:.+]] = acsim.instance @producer target @Producer
// CHECK: %[[CONSUMER_PORT:.+]] = acsim.port %[[CONSUMER]] accessor @input
// CHECK: %[[PRODUCER_PORT:.+]] = acsim.port %[[PRODUCER]] accessor @output
// CHECK: acsim.bind %[[PRODUCER_PORT]] to %[[CONSUMER_PORT]] kind "port"
// CHECK: acsim.dispatch @Top::@consumer path "root.consumer"
// CHECK: acsim.dispatch @Top::@producer path "root.producer"
// CHECK-COUNT-4: acsim.activate
