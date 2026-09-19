// RUN: %acir_opt --pass-pipeline='builtin.module(ac-lower-rules)' %s -o %t.lowered
// RUN: %FileCheck %s < %t.lowered
// RUN: %acir_opt "-ac-build-rule-effect-graph=json-output=%t.json dot-output=%t.dot" %t.lowered -o /dev/null
// RUN: %FileCheck %s --check-prefix=GRAPH < %t.json

module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "owner_scope"} {
  ac.type_scope @left_types {
    ac.struct @Entry fields [{name = "valid", type = i1}, {name = "tag", type = i7}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@left_types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.type_scope @right_types {
    ac.struct @Entry fields [{name = "tag", type = i7}, {name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@right_types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.module @Left() parameters {} graph {
    ac.scope @left() {
      %input = ac.source depth 1 latency 1 : !ac.queue<i8>
      ac.table @state entry !ac.struct<@left_types::@Entry> entries 1 init 0 owner "/left" stable_id "table/left/state"
      ac.slot @mailbox, %input owner "/left" stable_id "slot/left/mailbox" : !ac.queue<i8>
      ac.rule depths [] latencies [] name "left" stable_id "left" domain "cycle" type exact {
        %index = ac.var.constant false as !ac.var<i1>
        %value = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<!ac.struct<@left_types::@Entry>>
        %valid, %payload = ac.slot.get @mailbox : !ac.var<i1>, !ac.var<i8>
        ac.rule.condition %valid : !ac.var<i1>
        ac.table.propose @state[%index] = %value when %valid : !ac.var<i1> mode "replace" write_fields ["valid", "tag"] : !ac.var<i1>, !ac.var<!ac.struct<@left_types::@Entry>>
        ac.slot.propose_release @mailbox when %valid : !ac.var<i1>
        ac.rule.return
      } : () -> ()
      ac.scope.yield
    } : () -> ()
    ac.return
  }
  ac.module @Right() parameters {} graph {
    ac.scope @right() {
      %input = ac.source depth 1 latency 1 : !ac.queue<i8>
      ac.table @state entry !ac.struct<@right_types::@Entry> entries 1 init 0 owner "/right" stable_id "table/right/state"
      ac.slot @mailbox, %input owner "/right" stable_id "slot/right/mailbox" : !ac.queue<i8>
      ac.rule depths [] latencies [] name "right" stable_id "right" domain "cycle" type exact {
        %mask = ac.table.match @state predicate {
        ^bb0(%entry: !ac.var<!ac.struct<@right_types::@Entry>>):
          %lane_valid = ac.var.get %entry field "valid" : !ac.var<!ac.struct<@right_types::@Entry>> -> !ac.var<i1>
          ac.table.match.yield %lane_valid : !ac.var<i1>
        } -> !ac.var<i1>
        %index, %present = ac.table.choose @state %mask : !ac.var<i1> count 1 policy #ac<table_selection_policy first> stable_id "right/first" key {
        } -> !ac.var<i1>, !ac.var<i1>
        %value = ac.table.get @state[%index] : !ac.var<i1> -> !ac.var<!ac.struct<@right_types::@Entry>>
        %valid, %payload = ac.slot.get @mailbox : !ac.var<i1>, !ac.var<i8>
        %enabled = ac.var.and %valid, %present : !ac.var<i1>
        ac.rule.condition %enabled : !ac.var<i1>
        ac.table.propose @state[%index] = %value when %enabled : !ac.var<i1> mode "replace" write_fields ["tag", "valid"] : !ac.var<i1>, !ac.var<!ac.struct<@right_types::@Entry>>
        ac.slot.propose_release @mailbox when %enabled : !ac.var<i1>
        ac.rule.return
      } : () -> ()
      ac.scope.yield
    } : () -> ()
    ac.return
  }
  ac.type_scope @capture_types {
    ac.struct @Entry fields [{name = "valid", type = i1}]
    ac.struct @Request fields [{name = "valid", type = i1}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@capture_types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}, !ac.struct<@capture_types::@Request> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.table @capture_state entry !ac.struct<@capture_types::@Entry> entries 2 init 0 owner "/" stable_id "table/capture_state"
  %request = ac.source depth 1 latency 1 : !ac.queue<!ac.struct<@capture_types::@Request>>
  ac.rule %request depths [1] latencies [1] name "captured" stable_id "captured" domain "cycle" type exact {
  ^body(%item: !ac.var<!ac.struct<@capture_types::@Request>>):
    %mask = ac.table.match @capture_state predicate {
    ^bb0(%entry: !ac.var<!ac.struct<@capture_types::@Entry>>):
      %captured_valid = ac.var.get %item field "valid" : !ac.var<!ac.struct<@capture_types::@Request>> -> !ac.var<i1>
      ac.table.match.yield %captured_valid : !ac.var<i1>
    } -> !ac.var<i2>
    %ready = ac.marker.obligation %mask state pending resolver handshake origin "captured:return" path "true" : !ac.var<i2>
    ac.rule.return %ready : !ac.var<i2>
  } : (!ac.queue<!ac.struct<@capture_types::@Request>>) -> !ac.queue<i2>
}

// CHECK-LABEL: ac.firing depths [] latencies [] stable_id "left"
// CHECK: owner = "/left"
// CHECK-SAME: owner_stable_id = "table/left/state"
// CHECK: owner = "/left"
// CHECK-SAME: owner_stable_id = "slot/left/mailbox"
// CHECK-LABEL: ac.firing depths [] latencies [] stable_id "right"
// CHECK: ac.footprints_exact = [{access = "read", all_entries = true, endpoint = "ac.table.match", fields = ["valid"]
// CHECK-SAME: owner = "/right"
// CHECK-SAME: owner_stable_id = "table/right/state"
// CHECK-SAME: whole_entry = false
// CHECK-SAME: endpoint = "ac.slot.get", fields = []
// CHECK-SAME: owner = "/right"
// CHECK-SAME: owner_stable_id = "slot/right/mailbox"
// CHECK-LABEL: ac.firing {{.*}} stable_id "captured"
// CHECK: ac.footprints_exact = [
// CHECK-SAME: endpoint = "ac.table.match", fields = []
// CHECK-SAME: owner_stable_id = "table/capture_state"
// CHECK-SAME: whole_entry = true
// GRAPH-DAG: "id": "state_owner:table/left/state"
// GRAPH-DAG: "id": "state_owner:table/right/state"
// GRAPH-DAG: "id": "resource:slot:slot/left/mailbox"
// GRAPH-DAG: "id": "resource:slot:slot/right/mailbox"
// GRAPH-DAG: "owner_path=/left;owner_stable_id=table/left/state
// GRAPH-DAG: "owner_path=/right;owner_stable_id=table/right/state
