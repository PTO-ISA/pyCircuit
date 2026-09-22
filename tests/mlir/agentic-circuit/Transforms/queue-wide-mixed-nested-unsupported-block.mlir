// RUN: %acir_opt --ac-freeze-topology %s -o %t.frozen.mlir
// RUN: %not %acir_queue_cxxgen %t.frozen.mlir 2>&1 | %FileCheck %s --check-prefix=ERR


builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle"
} {
  "ac.system"() <{
    sym_name = "wide_mixed_nested_reuse",
    root = @Top,
    root_name = "root",
    tick_epoch = 0 : i64,
    tick_unit = "cycle",
    seed_policy = {kind = "fixed", value = 0 : i64},
    instrumentation = [],
    result_schema = {id = "default", format = "json"},
    selected = true
  }> : () -> ()
ac.module @Increment source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<i8>) -> (!ac.queue<i8>) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ^bb0(%input: !ac.queue<i8>):
    %output = ac.scope @logic(%input) {
    ^bb0(%borrowed: !ac.queue<i8>):
      %incremented = ac.transform %borrowed depths [2] latencies [1] {
      ^bb0(%item: !ac.var<i8>):
        %one = ac.var.constant 1 : i8 as !ac.var<i8>
        %next = ac.var.add %item, %one : !ac.var<i8>
        ac.transform.yield %next : !ac.var<i8>
      } {ac.name = "incremented"} : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %incremented : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %output : !ac.queue<i8>

    }
  }
  ac.module @PrepareChildFinalize source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<i8>) -> (!ac.queue<i8>) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ^bb0(%input: !ac.queue<i8>):
    %prepared = ac.scope @prepare(%input) {
    ^bb0(%borrowed: !ac.queue<i8>):
      %local = ac.feedback %borrowed depth 2 latency 1 max_iterations 8 {
      ^bb0(%item: !ac.var<i8>):
        %one = ac.var.constant 1 : i8 as !ac.var<i8>
        %next = ac.var.add %item, %one : !ac.var<i8>
        %cont = ac.var.constant false as !ac.var<i1>
        ac.feedback.yield %next continue %cont : !ac.var<i8>, !ac.var<i1>
      } {ac.name = "prepared"} : !ac.queue<i8> -> !ac.queue<i8>
      ac.scope.yield %local : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    %child_output = ac.instance @child of @Increment(%prepared) static #ac.static_arguments<[]>
        id "child" path "child" : (!ac.queue<i8>) -> !ac.queue<i8>
    %finalized = ac.scope @finalize(%child_output) {
    ^bb0(%borrowed_final: !ac.queue<i8>):
      %local_final = ac.transform %borrowed_final depths [2] latencies [1] {
      ^bb0(%item_final: !ac.var<i8>):
        %one_final = ac.var.constant 1 : i8 as !ac.var<i8>
        %next_final = ac.var.add %item_final, %one_final : !ac.var<i8>
        ac.transform.yield %next_final : !ac.var<i8>
      } {ac.name = "finalized"} : (!ac.queue<i8>) -> !ac.queue<i8>
      ac.scope.yield %local_final : !ac.queue<i8>
    } : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.return %finalized : !ac.queue<i8>

    }
  }
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    %left_input, %right_input = ac.scope @inputs() {
      %left = ac.source depth 2 latency 1 {ac.name = "left_input"}
          : !ac.queue<i8>
      %right = ac.source depth 2 latency 1 {ac.name = "right_input"}
          : !ac.queue<i8>
      ac.scope.yield %left, %right : !ac.queue<i8>, !ac.queue<i8>
    } : () -> (!ac.queue<i8>, !ac.queue<i8>)
    %left_output = ac.instance @left of @PrepareChildFinalize(%left_input) static #ac.static_arguments<[]>
        id "left" path "left" : (!ac.queue<i8>) -> !ac.queue<i8>
    %right_output = ac.instance @right of @PrepareChildFinalize(%right_input) static #ac.static_arguments<[]>
        id "right" path "right" : (!ac.queue<i8>) -> !ac.queue<i8>
    ac.scope @outputs(%left_output, %right_output) {
    ^bb0(%left: !ac.queue<i8>, %right: !ac.queue<i8>):
      ac.sink %left {ac.name = "left_sink"} : !ac.queue<i8>
      ac.sink %right {ac.name = "right_sink"} : !ac.queue<i8>
      ac.scope.yield
    } : (!ac.queue<i8>, !ac.queue<i8>) -> ()
    ac.return

    }
  }
}



// A feedback block next to child instances must not be flattened into a
// single-pass transform: the mixed nested backend only accepts local transforms
// and fanout broadcasts.
// ERR: ACLOWER-QUEUE-CXX: mixed nested module supports only local transform and broadcast blocks; block 'prepared' has kind 'feedback'
