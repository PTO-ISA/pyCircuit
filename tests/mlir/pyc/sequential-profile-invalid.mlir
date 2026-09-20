// RUN: %not %pyc_opt --pyc-check-frontend-contract %s 2>&1 | %FileCheck %s

module attributes {pyc.top = @redundant_hold, pyc.frontend.contract = "pycircuit"} {
  func.func @redundant_hold(%clk: !pyc.clock, %rst: !pyc.reset, %enable: i1,
      %value: i8) -> i8 attributes {
      arg_names = ["clk", "rst", "enable", "value"], result_names = ["q"],
      pyc.value_params = [], pyc.value_param_types = [], pyc.kind = "module",
      pyc.inline = "false", pyc.params = "{}", pyc.base = "redundant_hold",
      pyc.struct.metrics = "{\"ast_node_count\":3,\"collection_count\":0,\"collection_instance_count\":0,\"estimated_inline_cost\":3,\"hardware_call_count\":0,\"instance_count\":0,\"loop_count\":0,\"module_call_count\":0,\"module_family_collection_count\":0,\"repeat_pressure\":0,\"repeated_body_clusters\":[],\"source_loc\":0,\"state_alloc_count\":1,\"state_call_count\":0}",
      pyc.struct.collections = "[]"} {
    %zero = pyc.constant 0 : i8
    %next = pyc.wire : i8
    %q = pyc.reg %clk, %rst, %enable, %next, %zero : i8
    %recirculate = pyc.select %enable, %value, %q : i1, i8, i8 -> i8
    pyc.assign %next, %recirculate : i8
    func.return %q : i8
  }
}

// CHECK: stall recirculation is redundant; drive the update value and hold state with register enable=0
