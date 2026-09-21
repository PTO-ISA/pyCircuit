// RUN: %not %pyc_opt -split-input-file %s 2>&1 | %FileCheck %s

#prov = #ac.source_provenance<"tests/family.py", 1, 1, 1, 8>
#args = #ac.dependent_arguments<[]>
#iface = #ac.module_interface<[]>
#clock_origin = #pyc.implicit_control_origin<"clock", "implicit", #prov>
#reset_origin = #pyc.implicit_control_origin<"reset", "implicit", #prov>
#clock = #pyc.control_port_mapping<"clock", 0, !pyc.clock, #clock_origin, "clk">
#reset = #pyc.control_port_mapping<"reset", 1, !pyc.reset, #reset_origin, "rst">
#mapping = #pyc.module_port_mapping<[#reset, #clock], [], [], []>

module attributes {
  pyc.bad_signature = #pyc.module_case_signature<
      #args, #iface, (!pyc.clock, !pyc.reset) -> (), #mapping>
} {
}

// CHECK: controls must be ordered clock then reset

// -----

#prov = #ac.source_provenance<"tests/family.py", 1, 1, 1, 8>
#args = #ac.dependent_arguments<[]>
#logical_type = #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>
#iface = #ac.module_interface<[
  #ac.interface_port<"input", "input", <#ac.type_expr_concrete<!ac.queue<i8>>>, <"tests/family.py", 1, 1, 1, 8>>
]>
#clock_origin = #pyc.implicit_control_origin<"clock", "implicit", #prov>
#reset_origin = #pyc.implicit_control_origin<"reset", "implicit", #prov>
#clock = #pyc.control_port_mapping<"clock", 0, !pyc.clock, #clock_origin, "clk">
#reset = #pyc.control_port_mapping<"reset", 1, !pyc.reset, #reset_origin, "rst">
#layout = #pyc.layout<8, [#pyc.packed_leaf<#pyc.projection_path<[]>, #ac.type_expr<#ac.type_expr_concrete<i8>>, 0, 8>]>
#valid = #pyc.physical_port<"input", 3, i1, "queue_valid" lane 0 : i64>
#data = #pyc.physical_port<"input", 4, i8, "queue_data" lane 0 : i64 layout #layout>
#ready = #pyc.physical_port<"result", 0, i1, "queue_ready">
#logical = #pyc.logical_port_mapping<"input", 0, "input", #logical_type, [#valid, #data, #ready], #prov>
#mapping = #pyc.module_port_mapping<[#clock, #reset], [#logical], [#valid, #data], [#ready]>

module attributes {
  pyc.bad_signature = #pyc.module_case_signature<
      #args, #iface, (!pyc.clock, !pyc.reset, i1, i8) -> (i1), #mapping>
} {
}

// CHECK: physical input carriers must be contiguous after controls

// -----

#prov = #ac.source_provenance<"tests/family.py", 1, 1, 1, 8>
#args = #ac.dependent_arguments<[]>
#logical_type = #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8, lanes=2, rate=1>>>
#iface = #ac.module_interface<[
  #ac.interface_port<"input", "input", <#ac.type_expr_concrete<!ac.queue<i8, lanes=2, rate=1>>>, <"tests/family.py", 1, 1, 1, 8>>
]>
#clock_origin = #pyc.implicit_control_origin<"clock", "implicit", #prov>
#reset_origin = #pyc.implicit_control_origin<"reset", "implicit", #prov>
#clock = #pyc.control_port_mapping<"clock", 0, !pyc.clock, #clock_origin, "clk">
#reset = #pyc.control_port_mapping<"reset", 1, !pyc.reset, #reset_origin, "rst">
#layout = #pyc.layout<8, [#pyc.packed_leaf<#pyc.projection_path<[]>, #ac.type_expr<#ac.type_expr_concrete<i8>>, 0, 8>]>
#valid1 = #pyc.physical_port<"input", 2, i1, "queue_valid" lane 1 : i64>
#data1 = #pyc.physical_port<"input", 3, i8, "queue_data" lane 1 : i64 layout #layout>
#valid0 = #pyc.physical_port<"input", 4, i1, "queue_valid" lane 0 : i64>
#data0 = #pyc.physical_port<"input", 5, i8, "queue_data" lane 0 : i64 layout #layout>
#ready = #pyc.physical_port<"result", 0, i1, "queue_ready">
#logical = #pyc.logical_port_mapping<"input", 0, "input", #logical_type, [#valid1, #data1, #valid0, #data0, #ready], #prov>
#mapping = #pyc.module_port_mapping<[#clock, #reset], [#logical], [#valid1, #data1, #valid0, #data0], [#ready]>

module attributes {
  pyc.bad_signature = #pyc.module_case_signature<
      #args, #iface, (!pyc.clock, !pyc.reset, i1, i8, i1, i8) -> (i1), #mapping>
} {
}

// CHECK: Queue lane carriers have wrong direction or order

// -----

#prov = #ac.source_provenance<"tests/family.py", 1, 1, 1, 8>
#args = #ac.dependent_arguments<[]>
#logical_type = #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>
#iface = #ac.module_interface<[
  #ac.interface_port<"input", "input", <#ac.type_expr_concrete<!ac.queue<i8>>>, <"tests/family.py", 1, 1, 1, 8>>
]>
#clock_origin = #pyc.implicit_control_origin<"clock", "implicit", #prov>
#reset_origin = #pyc.implicit_control_origin<"reset", "implicit", #prov>
#clock = #pyc.control_port_mapping<"clock", 0, !pyc.clock, #clock_origin, "clk">
#reset = #pyc.control_port_mapping<"reset", 1, !pyc.reset, #reset_origin, "rst">
#layout = #pyc.layout<8, [#pyc.packed_leaf<#pyc.projection_path<[]>, #ac.type_expr<#ac.type_expr_concrete<i8>>, 0, 8>]>
#valid = #pyc.physical_port<"input", 2, i1, "queue_valid" lane 0 : i64>
#data = #pyc.physical_port<"input", 3, i8, "queue_data" lane 0 : i64 layout #layout>
#ready = #pyc.physical_port<"result", 0, i1, "queue_ready">
#logical = #pyc.logical_port_mapping<"input", 0, "input", #logical_type, [#ready, #valid, #data], #prov>
#mapping = #pyc.module_port_mapping<[#clock, #reset], [#logical], [#valid, #data], [#ready]>

module attributes {
  pyc.bad_signature = #pyc.module_case_signature<
      #args, #iface, (!pyc.clock, !pyc.reset, i1, i8) -> (i1), #mapping>
} {
}

// CHECK: Queue requires one trailing shared opposite-direction ready carrier

// -----

#prov = #ac.source_provenance<"tests/family.py", 1, 1, 1, 8>
#args = #ac.dependent_arguments<[]>
#logical_type = #ac.type_expr<#ac.type_expr_concrete<!ac.queue<i8>>>
#iface = #ac.module_interface<[
  #ac.interface_port<"input", "input", <#ac.type_expr_concrete<!ac.queue<i8>>>, <"tests/family.py", 1, 1, 1, 8>>
]>
#clock_origin = #pyc.implicit_control_origin<"clock", "implicit", #prov>
#reset_origin = #pyc.implicit_control_origin<"reset", "implicit", #prov>
#clock = #pyc.control_port_mapping<"clock", 0, !pyc.clock, #clock_origin, "clk">
#reset = #pyc.control_port_mapping<"reset", 1, !pyc.reset, #reset_origin, "rst">
#layout = #pyc.layout<8, [#pyc.packed_leaf<#pyc.projection_path<[]>, #ac.type_expr<#ac.type_expr_concrete<i8>>, 0, 8>]>
#valid = #pyc.physical_port<"input", 2, i1, "queue_valid" lane 0 : i64>
#data = #pyc.physical_port<"input", 3, i8, "queue_data" lane 0 : i64 layout #layout>
#unused = #pyc.physical_port<"input", 4, i1, "queue_ready">
#ready = #pyc.physical_port<"result", 0, i1, "queue_ready">
#logical = #pyc.logical_port_mapping<"input", 0, "input", #logical_type, [#valid, #data, #ready], #prov>
#mapping = #pyc.module_port_mapping<[#clock, #reset], [#logical], [#valid, #data, #unused], [#ready]>

module attributes {
  pyc.bad_signature = #pyc.module_case_signature<
      #args, #iface, (!pyc.clock, !pyc.reset, i1, i8, i1) -> (i1), #mapping>
} {
}

// CHECK: mapping must cover every logical and physical port exactly once

// -----

#prov = #ac.source_provenance<"tests/family.py", 1, 1, 1, 8>
#owner = #ac.source_owner<"tests/family.py", "tests/family.py">
#args = #ac.static_arguments<[]>
#dependent_args = #ac.dependent_arguments<[]>
#logical_type = #ac.type_expr<#ac.type_expr_concrete<i8>>
#iface = #ac.module_interface<[
  #ac.interface_port<"value", "input", #logical_type, #prov>
]>
#schema = #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#args]>, #iface, #owner, []>
#clock_origin = #pyc.implicit_control_origin<"clock", "implicit", #prov>
#reset_origin = #pyc.implicit_control_origin<"reset", "implicit", #prov>
#clock = #pyc.control_port_mapping<"clock", 0, !pyc.clock, #clock_origin, "clk">
#reset = #pyc.control_port_mapping<"reset", 1, !pyc.reset, #reset_origin, "rst">
#bad_layout = #pyc.layout<8, [#pyc.packed_leaf<#pyc.projection_path<[]>, #ac.type_expr<#ac.type_expr_concrete<i4>>, 0, 8>]>
#value = #pyc.physical_port<"input", 2, i8, "value" layout #bad_layout>
#logical = #pyc.logical_port_mapping<"input", 0, "value", #logical_type, [#value], #prov>
#mapping = #pyc.module_port_mapping<[#clock, #reset], [#logical], [#value], []>
#signature = #pyc.module_case_signature<#dependent_args, #iface, (!pyc.clock, !pyc.reset, i8) -> (), #mapping>

module {
  pyc.module @family source #owner schema #schema {
    pyc.module.case signature #signature source #prov {
    ^bb0(%clock_arg: !pyc.clock, %reset_arg: !pyc.reset, %value_arg: i8):
      pyc.return
    }
  }
}

// CHECK: packed layout does not recursively match the logical type
