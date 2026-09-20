// RUN: %acir_opt --ac-freeze-topology %s -o %t.frozen.mlir
// RUN: %acir_queue_pycgen %t.frozen.mlir > %t.pyc
// RUN: %FileCheck %s --check-prefix=PYC < %t.pyc
// RUN: %pycc %t.pyc --cpp %t.cpp
// RUN: %cxx -std=c++20 -I%source_root/library -fsyntax-only %t.cpp
// RUN: %pycc %t.pyc --verilog %t.sv
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t.sv

#owner = #ac.source_owner<"tests/config_stage.py", "tests/config_stage.py">
#prov = #ac.source_provenance<"tests/config_stage.py", 1, 1, 1, 1>
#config_type = #ac.static_config_type<@Config, #ac.static_config_fields<[
  #ac.static_config_field<"enabled", #ac.static_bool_type>,
  #ac.static_config_field<"bias", #ac.static_int_type<8, false>>
]>>
#fast = #ac.static_value<#ac.static_config_value<@Config, #ac.static_config_field_values<[
  #ac.static_config_field_value<"enabled", #ac.static_bool_value<true>>,
  #ac.static_config_field_value<"bias", #ac.static_int_value<#ac.static_int_type<8, false>, 1 : i8>>
]>>>
#safe = #ac.static_value<#ac.static_config_value<@Config, #ac.static_config_field_values<[
  #ac.static_config_field_value<"enabled", #ac.static_bool_value<false>>,
  #ac.static_config_field_value<"bias", #ac.static_int_value<#ac.static_int_type<8, false>, 2 : i8>>
]>>>
#fast_args = #ac.static_arguments<[#ac.static_argument<"cfg", #fast>]>
#safe_args = #ac.static_arguments<[#ac.static_argument<"cfg", #safe>]>
#queue_expr = #ac.type_expr<#ac.type_expr_queue<#ac.type_expr<#ac.type_expr_concrete<i8>>, #ac.dependent_value<#ac.dependent_integer<2>>, #ac.dependent_value<#ac.dependent_integer<2>>>>
#interface = #ac.module_interface<[
  #ac.interface_port<"value", "input", #queue_expr, #prov>,
  #ac.interface_port<"result", "output", #queue_expr, #prov>
]>
#core_owner = #ac.source_owner<"tests/core.py", "tests/core.py">
#core_prov = #ac.source_provenance<"tests/core.py", 1, 1, 1, 1>
#core_interface = #ac.module_interface<[
  #ac.interface_port<"value", "input", #queue_expr, #core_prov>,
  #ac.interface_port<"result", "output", #queue_expr, #core_prov>
]>
#schema = #ac.module_family_schema<
  #ac.static_parameters<[
    #ac.static_parameter<"cfg", #ac.static_type<#config_type>, true, [], #prov>
  ]>,
  #ac.static_cases<[#fast_args, #safe_args]>, #interface, #owner, []>

module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.system @config_family root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true

  ac.module @ConfigStage source #owner schema #schema {
    ac.module.case arguments #fast_args type (!ac.queue<i8, lanes=2, rate=2>) -> !ac.queue<i8, lanes=2, rate=2>
        source #prov graph {
    ^bb0(%input: !ac.queue<i8, lanes=2, rate=2>):
      %output = ac.scope @body(%input) {
      ^bb0(%borrowed: !ac.queue<i8, lanes=2, rate=2>):
        %next = ac.transform %borrowed depths [2] latencies [1] {
        ^bb0(%value: !ac.var<i8>):
          %one = ac.var.constant 1 : i8 as !ac.var<i8>
          %sum = ac.var.add %value, %one : !ac.var<i8>
          ac.transform.yield %sum : !ac.var<i8>
        } {ac.name = "result"} : (!ac.queue<i8, lanes=2, rate=2>) -> !ac.queue<i8, lanes=2, rate=2>
        ac.scope.yield %next : !ac.queue<i8, lanes=2, rate=2>
      } : (!ac.queue<i8, lanes=2, rate=2>) -> !ac.queue<i8, lanes=2, rate=2>
      ac.return %output : !ac.queue<i8, lanes=2, rate=2>
    }
    ac.module.case arguments #safe_args type (!ac.queue<i8, lanes=2, rate=2>) -> !ac.queue<i8, lanes=2, rate=2>
        source #prov graph {
    ^bb0(%input: !ac.queue<i8, lanes=2, rate=2>):
      %output = ac.scope @body(%input) {
      ^bb0(%borrowed: !ac.queue<i8, lanes=2, rate=2>):
        %next = ac.transform %borrowed depths [2] latencies [1] {
        ^bb0(%value: !ac.var<i8>):
          %two = ac.var.constant 2 : i8 as !ac.var<i8>
          %sum = ac.var.add %value, %two : !ac.var<i8>
          ac.transform.yield %sum : !ac.var<i8>
        } {ac.name = "result"} : (!ac.queue<i8, lanes=2, rate=2>) -> !ac.queue<i8, lanes=2, rate=2>
        ac.scope.yield %next : !ac.queue<i8, lanes=2, rate=2>
      } : (!ac.queue<i8, lanes=2, rate=2>) -> !ac.queue<i8, lanes=2, rate=2>
      ac.return %output : !ac.queue<i8, lanes=2, rate=2>
    }
  }

  ac.module @Top source #core_owner
      schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #core_interface, #core_owner, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (!ac.queue<i8, lanes=2, rate=2>) -> !ac.queue<i8, lanes=2, rate=2>
        source #core_prov graph {
    ^bb0(%input: !ac.queue<i8, lanes=2, rate=2>):
      %output = ac.instance @stage of @ConfigStage(%input) static #fast_args
          id "stage" path "stage" : (!ac.queue<i8, lanes=2, rate=2>) -> !ac.queue<i8, lanes=2, rate=2>
      ac.return %output : !ac.queue<i8, lanes=2, rate=2>
    }
  }
}

// PYC-LABEL: pyc.module @ConfigStage
// PYC: pyc.module.case signature
// PYC-SAME: #ac.static_bool_value<true>
// PYC: pyc.module.case signature
// PYC-SAME: #ac.static_bool_value<false>
// PYC-LABEL: pyc.module @Top
// PYC: #pyc.logical_port_mapping<"input", 0, "value"
// PYC-SAME: #pyc.physical_port<"input", {{[0-9]+}}, i1, "queue_valid" lane 0 : i64>
// PYC-SAME: #pyc.physical_port<"input", {{[0-9]+}}, i8, "queue_data" lane 0 : i64
// PYC-SAME: #pyc.physical_port<"input", {{[0-9]+}}, i1, "queue_valid" lane 1 : i64>
// PYC-SAME: #pyc.physical_port<"input", {{[0-9]+}}, i8, "queue_data" lane 1 : i64
// PYC-SAME: #pyc.physical_port<"result", {{[0-9]+}}, i1, "queue_ready">
