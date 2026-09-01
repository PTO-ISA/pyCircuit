// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %s -o %t.frozen
// RUN: %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/stateful-graph.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen -o %t.first
// RUN: %acir_opt --ac-lower-to-acsim --ac-binding-registry=%S/Inputs/stateful-graph.json --ac-binding-profile=fast --ac-binding-target=arm64-apple-darwin %t.frozen -o %t.second
// RUN: diff %t.first %t.second
// RUN: %FileCheck %s < %t.first

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
  ac.module @Cell() parameters {} graph {
    ac.return
  }
  ac.module @Top() -> !ac.endpoint<@Wire, @source> parameters {} graph {
    ac.array @cells of @Cell shape [2]() static [{}, {}]
        id "cells" path "cells" : () -> ()
    %captured = ac.instance @captured of @Producer() static {}
        id "captured" path "captured" : () -> !ac.endpoint<@Wire, @source>
    %connected = ac.instance @producer of @Producer() static {}
        id "producer" path "producer" : () -> !ac.endpoint<@Wire, @source>
    ac.instance @consumer of @Consumer(%connected) static {}
        id "consumer" path "consumer" : (!ac.endpoint<@Wire, @source>) -> ()
    %exported = ac.instance @exporter of @Producer() static {}
        id "exporter" path "exporter" : () -> !ac.endpoint<@Wire, @source>
    ac.process @workload kind "workload"
        captures(%captured : !ac.endpoint<@Wire, @source>) {
    ^bb0(%capture : !ac.endpoint<@Wire, @source>):
      %ready = arith.constant true
      %value = arith.constant 7 : i32
      ac.wait_until %ready
      %used = arith.addi %value, %value : i32
      ac.yield_sim
    }
    ac.return %exported : !ac.endpoint<@Wire, @source>
  }
}

// CHECK: acsim.model @soc epoch "0.4" root @Top
// CHECK: acsim.binding @Consumer
// CHECK: acsim.binding @Producer
// CHECK: acsim.module @Cell interface {ports = [], resources = [], results = []}
// CHECK: acsim.module @Top interface {
// CHECK-SAME: ports = [{accessor = @output
// CHECK-SAME: name = "port_00000000"
// CHECK-SAME: role = @source
// CHECK-SAME: results = []}
// CHECK-SAME: exports [@port_00000000]
// CHECK: acsim.array @cells target @Cell
// CHECK: acsim.bind {{.*}} kind "port"
// CHECK: acsim.export @port_00000000 {{.*}} role @source
// CHECK: acsim.process @workload
// CHECK-SAME: captures({{.*}}!acsim.port<@Wire, @source, @cpp_i8, @wire>)
// CHECK-SAME: pcs [@entry, @pc00000001]
// CHECK-SAME: live [{{.*}}name = "live00000000"{{.*}}]
// CHECK: acsim.live.store
// CHECK: acsim.live.load
// CHECK: acsim.return {{.*}} : !acsim.port<@Wire, @source, @cpp_i8, @wire>
// CHECK-COUNT-5: acsim.dispatch
// CHECK-COUNT-7: acsim.activate
