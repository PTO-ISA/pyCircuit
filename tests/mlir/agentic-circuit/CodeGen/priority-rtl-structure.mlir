// RUN: %pyc_opt %s -o %t.pyc
// RUN: %pycc %t.pyc --verilog %t.sv
// RUN: %FileCheck %s < %t.sv
// RUN: %FileCheck %s --check-prefix=PRIMITIVE < %source_root/library/verilog/pyc_priority_encode.v
// RUN: %python %source_root/flows/tools/check_generated_rtl.py %t.sv
// RUN: verilator --lint-only -Wno-fatal -I%source_root/library/verilog %t.sv

module attributes {pyc.top = @PriorityMux, pyc.frontend.contract = "pycircuit"} {
  func.func @PriorityMux(%mask: i8) -> (i3, i1) attributes {
      arg_names = ["mask"],
      result_names = ["index", "valid"],
      pyc.value_params = [],
      pyc.value_param_types = [],
      pyc.kind = "module",
      pyc.inline = "false",
      pyc.params = "{}",
      pyc.base = "PriorityMux",
      pyc.struct.metrics = "{\"ast_node_count\":1,\"collection_count\":0,\"collection_instance_count\":0,\"estimated_inline_cost\":1,\"hardware_call_count\":0,\"instance_count\":0,\"loop_count\":0,\"module_call_count\":0,\"module_family_collection_count\":0,\"repeat_pressure\":0,\"repeated_body_clusters\":[],\"source_loc\":0,\"state_alloc_count\":0,\"state_call_count\":0}",
      pyc.struct.collections = "[]"
    } {
    %index, %valid = pyc.priority_encode %mask {order = "low"} : i8 -> i3, i1
    func.return %index, %valid : i3, i1
  }
}

// CHECK: module priority_mux (
// CHECK: pyc_priority_encode #(.ORDER_LOW(1), .WIDTH(8))
// CHECK-SAME: .in_value(mask)
// CHECK-SAME: .index({{[A-Za-z0-9_]+}})
// CHECK-SAME: .valid({{[A-Za-z0-9_]+}})

// PRIMITIVE: if (ORDER_LOW != 0) begin
// PRIMITIVE-NEXT: for (bit_index = WIDTH - 1; bit_index >= 0; bit_index = bit_index - 1) begin
// PRIMITIVE: if (in_value[bit_index]) begin
// PRIMITIVE: index = INDEX_WIDTH'(bit_index);
// PRIMITIVE-NOT: assign index = {{.*}}&{{.*}}
