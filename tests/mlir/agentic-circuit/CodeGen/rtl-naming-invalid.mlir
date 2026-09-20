// RUN: %pyc_opt %s -o %t.pyc
// RUN: %not %pycc %t.pyc --verilog %t.sv 2>&1 | %FileCheck %s
// RUN: test ! -s %t.sv

module attributes {pyc.top = @FooBar, pyc.frontend.contract = "pycircuit"} {
  func.func @FooBar() attributes {
      arg_names = [], result_names = [], pyc.value_params = [],
      pyc.value_param_types = [], pyc.kind = "module", pyc.inline = "false",
      pyc.params = "{}", pyc.base = "FooBar",
      pyc.struct.metrics = "{\"ast_node_count\":0,\"collection_count\":0,\"collection_instance_count\":0,\"estimated_inline_cost\":0,\"hardware_call_count\":0,\"instance_count\":0,\"loop_count\":0,\"module_call_count\":0,\"module_family_collection_count\":0,\"repeat_pressure\":0,\"repeated_body_clusters\":[],\"source_loc\":0,\"state_alloc_count\":0,\"state_call_count\":0}",
      pyc.struct.collections = "[]"} {
    func.return
  }
  func.func @foo_bar() attributes {
      arg_names = [], result_names = [], pyc.value_params = [],
      pyc.value_param_types = [], pyc.kind = "module", pyc.inline = "false",
      pyc.params = "{}", pyc.base = "foo_bar",
      pyc.struct.metrics = "{\"ast_node_count\":0,\"collection_count\":0,\"collection_instance_count\":0,\"estimated_inline_cost\":0,\"hardware_call_count\":0,\"instance_count\":0,\"loop_count\":0,\"module_call_count\":0,\"module_family_collection_count\":0,\"repeat_pressure\":0,\"repeated_body_clusters\":[],\"source_loc\":0,\"state_alloc_count\":0,\"state_call_count\":0}",
      pyc.struct.collections = "[]"} {
    func.return
  }
}

// CHECK: RTL module name 'foo_bar' collides between 'FooBar' and 'foo_bar'
