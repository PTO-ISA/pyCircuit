module attributes {pyc.top = @count_zeros_top, pyc.frontend.contract = "pycircuit"} {
  func.func @count_zeros_top(%value: i13) -> (i4, i4) attributes {
      arg_names = ["value"],
      result_names = ["leading", "trailing"],
      pyc.value_params = [],
      pyc.value_param_types = [],
      pyc.kind = "module",
      pyc.inline = "false",
      pyc.params = "{}",
      pyc.base = "count_zeros_top",
      pyc.struct.metrics = "{\"ast_node_count\":1,\"collection_count\":0,\"collection_instance_count\":0,\"estimated_inline_cost\":1,\"hardware_call_count\":0,\"instance_count\":0,\"loop_count\":0,\"module_call_count\":0,\"module_family_collection_count\":0,\"repeat_pressure\":0,\"repeated_body_clusters\":[],\"source_loc\":0,\"state_alloc_count\":0,\"state_call_count\":0}",
      pyc.struct.collections = "[]"
    } {
    %leading = pyc.count_zeros %value {direction = "leading"} : i13 -> i4
    %trailing = pyc.count_zeros %value {direction = "trailing"} : i13 -> i4
    func.return %leading, %trailing : i4, i4
  }
}
