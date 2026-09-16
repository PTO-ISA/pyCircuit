// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "slot_rule"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<i8>
  ac.slot @mailbox, %input owner "/" stable_id "slot/mailbox" : !ac.queue<i8>
  %output = ac.rule depths [1] latencies [1] name "consume" stable_id "consume" domain "cycle" type exact {
    %valid, %value = ac.slot.get @mailbox : !ac.var<i1>, !ac.var<i8>
    ac.rule.condition %valid : !ac.var<i1>
    ac.slot.propose_release @mailbox when %valid : !ac.var<i1>
    %ready = ac.marker.obligation %value state pending resolver handshake origin "consume:return" path "true" : !ac.var<i8>
    ac.rule.output %value when %valid ordinal 0 : !ac.var<i8>, !ac.var<i1>
    ac.rule.return %ready : !ac.var<i8>
  } : () -> !ac.queue<i8>
  ac.sink %output {ac.name = "output"} : !ac.queue<i8>
}

// CHECK: ac.slot @mailbox
// CHECK: %[[OUTPUT:.*]] = ac.firing depths [1] latencies [1] stable_id "consume" domain "cycle"
// CHECK: %[[VALID:.*]], %[[VALUE:.*]] = ac.slot.get @mailbox
// CHECK: ac.firing.condition %[[VALID]]
// CHECK: ac.slot.propose_release @mailbox when %[[VALID]]
// CHECK: ac.firing.output %[[VALUE]] when %[[VALID]] ordinal 0
// CHECK: ac.activation_sources = [
// CHECK-SAME: output_queue>, ordinal = 0
// CHECK-SAME: slot>, resource = @mailbox
// CHECK: ac.transaction_resources = [
// CHECK-SAME: output_queue>, ordinal = 0
// CHECK-SAME: slot>, resource = @mailbox
