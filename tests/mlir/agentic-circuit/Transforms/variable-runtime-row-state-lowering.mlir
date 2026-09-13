// RUN: %acir_opt --pass-pipeline='builtin.module(ac-verify-value-constraints,ac-lower-variable-state)' %s | %FileCheck %s --check-prefix=STORAGE
// RUN: %acir_opt --verify-each=false --pass-pipeline='builtin.module(ac-verify-value-constraints,ac-lower-variable-state,ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)' %s -o %t.frozen.mlir
// RUN: %acir_queue_cxxgen %t.frozen.mlir > %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %t.cpp -o %t.o

module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "variable_runtime_row"} {
  ac.var.decl @entries type i8 init 0 : i8 owner "/" stable_id "var/entries" shape [8, 2]
  %rows = ac.source depth 1 latency 1 {ac.name = "rows"} : !ac.queue<i3>
  %output = ac.rule %rows depths [1] latencies [1]
      name "lookup" stable_id "lookup_0" domain "cycle" type exact {
  ^body(%row: !ac.var<i3>):
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    %mask = ac.var.match @entries row %row : !ac.var<i3> predicate {
    ^predicate(%entry: !ac.var<i8>):
      %zero = ac.var.constant 0 : i8 as !ac.var<i8>
      %valid = ac.var.cmp "ne" %entry, %zero : !ac.var<i8> -> !ac.var<i1>
      ac.var.match.yield %valid : !ac.var<i1>
    } -> !ac.var<i2>
    %index, %valid = ac.var.choose @entries %mask : !ac.var<i2>
        count 1 policy "first" key {} {ac.query = "runtime-row"}
        -> !ac.var<i4>, !ac.var<i1>
    %old = ac.var.read_element @entries[%index] : !ac.var<i4> -> !ac.var<i8>
    %one = ac.var.constant 1 : i8 as !ac.var<i8>
    ac.var.assign_element @entries[%index] = %one when %valid : !ac.var<i1>
        : !ac.var<i4>, !ac.var<i8>
    %ready = ac.marker.obligation %old state pending resolver handshake
        origin "lookup:return" path "true" : !ac.var<i8>
    ac.rule.output %old when %true ordinal 0 : !ac.var<i8>, !ac.var<i1>
    ac.rule.return %ready : !ac.var<i8>
  } {ac.name = "output"} : (!ac.queue<i3>) -> !ac.queue<i8>
  ac.sink %output {ac.name = "sink"} : !ac.queue<i8>
}

// STORAGE-NOT: ac.var.decl
// STORAGE-NOT: ac.var.match
// STORAGE: ac.table @entries entry i8 entries 16 init 0
// STORAGE-SAME: axis_widths = array<i64: 3, 1>
// STORAGE-SAME: shape = array<i64: 8, 2>
// STORAGE: %[[ZERO:.*]] = ac.var.constant false as !ac.var<i1>
// STORAGE: %[[BASE:.*]] = ac.table.index @entries[%{{.*}}, %[[ZERO]]]
// STORAGE-SAME: !ac.var<i3>, !ac.var<i1> -> !ac.var<i4>
// STORAGE: %[[MASK:.*]] = ac.table.match @entries base %[[BASE]] : !ac.var<i4> predicate
// STORAGE: } {domain_axes = array<i64: 1>
// STORAGE-SAME: domain_offset = 0 : i64
// STORAGE-SAME: domain_shape = array<i64: 2>
// STORAGE-SAME: domain_strides = array<i64: 1>
// STORAGE: ac.table.choose @entries %[[MASK]]
// STORAGE: } {{.*}} -> !ac.var<i4>, !ac.var<i1>
// STORAGE: ac.table.get @entries[%{{.*}}] : !ac.var<i4> -> !ac.var<i8>
// STORAGE: ac.table.propose @entries[%{{.*}}] = %{{.*}} when %{{.*}} : !ac.var<i1>
// STORAGE-SAME: mode "replace" write_fields ["$entry"]
