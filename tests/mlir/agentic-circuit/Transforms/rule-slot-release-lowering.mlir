// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s | %FileCheck %s
// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s -o %t.lowered
// RUN: sed 's/access = "release"/access = "read"/' %t.lowered | %not %acir_opt 2>&1 | %FileCheck %s --check-prefix=EXACT-TAMPER
// RUN: sed 's/fields = \[\]/fields = ["forged"]/' %t.lowered | %not %acir_opt 2>&1 | %FileCheck %s --check-prefix=FIELD-TAMPER

module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "slot_rule"} {
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
// CHECK: ac.expression_dag = [
// CHECK-SAME: endpoint = "slot_get"
// CHECK-SAME: owner = "/"
// CHECK-SAME: stable_id = "slot/mailbox"
// CHECK: ac.footprints_exact = [{access = "read", all_entries = true, endpoint = "ac.slot.get", fields = []
// CHECK-SAME: owner_stable_id = "slot/mailbox"
// CHECK-SAME: predicate = 0 : i64
// CHECK-SAME: whole_entry = true
// CHECK-SAME: {access = "release", all_entries = true, endpoint = "ac.slot.propose_release", fields = []
// CHECK-SAME: predicate = 1 : i64
// CHECK-SAME: whole_entry = true
// EXACT-TAMPER: exact rule effect summary does not match the live body
// FIELD-TAMPER: exact state footprint must choose whole entry or explicit fields
// CHECK: ac.transaction_resources = [
// CHECK-SAME: output_queue>, ordinal = 0
// CHECK-SAME: slot>, resource = @mailbox
