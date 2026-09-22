// RUN: %not %acir_queue_cxxgen %s 2>&1 | %FileCheck %s --check-prefix=ERR

module attributes {ac.frozen_owners = [{kind = "ac.system_root", owner = @Top, path = "root", stable_id = "root"}, {kind = "ac.instance", owner = @Top::@pick, path = "root.pick", stable_id = "root/pick"}], ac.frozen_system = @local_select, ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.topology_frozen = true} {
  ac.system @local_select root @Top as "root" tick 0 "cycle" seed {kind = "fixed", value = 0 : i64} instrumentation [] results {format = "json", id = "default"} selected true
  ac.module @SelectModule source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"ctrl", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i1>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"data", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"result", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<i1>, !ac.queue<i8>) -> !ac.queue<i8> source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ^bb0(%arg0: !ac.queue<i1>, %arg1: !ac.queue<i8>):
    %0 = ac.scope @body(%arg0, %arg1) {
    ^bb0(%ctrl: !ac.queue<i1>, %data: !ac.queue<i8>):
      %1:2 = ac.broadcast %data depths [1, 1] latencies [1, 1] {ac.name = "fan", ac.output_names = ["fan0", "fan1"]} : !ac.queue<i8> -> (!ac.queue<i8>, !ac.queue<i8>)
      %2 = ac.select %ctrl, %1#0, %1#1 depth 1 latency 1 key {
      ^key(%key_item: !ac.var<i1>):
        ac.select.yield %key_item : !ac.var<i1>
      } {ac.name = "pick"} : (!ac.queue<i1>, !ac.queue<i8>, !ac.queue<i8>) -> !ac.queue<i8>
      %3 = ac.transform %2 depths [1] latencies [1] {
      ^bb0(%item2: !ac.var<i8>):
        ac.transform.yield %item2 : !ac.var<i8>
      } {ac.name = "finish"} : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %3 : !ac.queue<i8>
    } : (!ac.queue<i1>, !ac.queue<i8>) -> !ac.queue<i8>
    ac.return %0 : !ac.queue<i8>

    }
  }
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    %0, %1 = ac.scope @inputs() {
      %2 = ac.source depth 1 latency 1 {ac.name = "ctrl"} : !ac.queue<i1>
      %3 = ac.source depth 1 latency 1 {ac.name = "data"} : !ac.queue<i8>
      ac.scope.yield %2, %3 : !ac.queue<i1>, !ac.queue<i8>
    } : () -> (!ac.queue<i1>, !ac.queue<i8>)
    %4 = ac.instance @pick of @SelectModule(%0, %1) static #ac.static_arguments<[]> id "pick" path "pick" : (!ac.queue<i1>, !ac.queue<i8>) -> !ac.queue<i8>
    ac.scope @outputs(%4) {
    ^bb0(%out: !ac.queue<i8>):
      ac.sink %out {ac.name = "sink_0"} : !ac.queue<i8>
      ac.scope.yield
    } : (!ac.queue<i8>) -> ()
    ac.return

    }
  }
}

// A select block in a parent's own scope must not be flattened into a
// single-pass transform: the multi-block local backend only accepts local
// transforms, fanout broadcasts, selective merges, and stateless firing blocks.
// ERR: ACLOWER-QUEUE-CXX: mixed nested module supports only local transform, fanout broadcast, selective merge, and stateless firing blocks; block 'pick' has kind 'select'
