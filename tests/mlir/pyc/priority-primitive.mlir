module attributes {pyc.top = @priority_top, pyc.frontend.contract = "pycircuit"} {
  func.func @priority_top(%mask: i13) -> (i4, i1, i4, i1) attributes {
      arg_names = ["mask"],
      result_names = ["low_index", "low_valid", "high_index", "high_valid"],
      pyc.value_params = [],
      pyc.value_param_types = [],
      pyc.kind = "module",
      pyc.inline = "false",
      pyc.params = "{}",
      pyc.base = "priority_top",
      pyc.struct.metrics = "{\"ast_node_count\":1,\"collection_count\":0,\"collection_instance_count\":0,\"estimated_inline_cost\":1,\"hardware_call_count\":0,\"instance_count\":0,\"loop_count\":0,\"module_call_count\":0,\"module_family_collection_count\":0,\"repeat_pressure\":0,\"repeated_body_clusters\":[],\"source_loc\":0,\"state_alloc_count\":0,\"state_call_count\":0}",
      pyc.struct.collections = "[]"
    } {
    %low_index, %low_valid = pyc.priority_encode %mask {order = "low"} : i13 -> i4, i1
    %high_index, %high_valid = pyc.priority_encode %mask {order = "high"} : i13 -> i4, i1
    func.return %low_index, %low_valid, %high_index, %high_valid : i4, i1, i4, i1
  }
}
