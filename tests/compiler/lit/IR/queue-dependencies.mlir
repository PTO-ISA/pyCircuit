// RUN: %pycircuit_opt %s --split-input-file --ac-verify-hardware --verify-diagnostics
// Actual observed/dead policy feedback and unobserved pure input cycles.
#n0 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#n3 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#n5 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#n13 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<13>}}>
#n65 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#n130 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#n1>
!b13 = !ac.bits<#n13>
!b65 = !ac.bits<#n65>
!b130 = !ac.bits<#n130>
!table130 = !ac.table<[#n3], !b130>
#depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Generic, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Generic, name = "D"}}>
!generic = !ac.type_param<@Generic, "T">
#token_depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Token3, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Token3, name = "D"}}>
!token = !ac.type_param<@Token3, "T">
!tableToken = !ac.table<[#n3], !token>
module {
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b13, %clk: !b1, %rst: !b1):
    %a_ready, %a_valid, %a_head = "ac.queue"(%clk, %rst, %valid, %data, %b_ready) {instance_name = "a", depth = #n3, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %b_ready, %b_valid, %b_head = "ac.queue"(%clk, %rst, %valid, %data, %a_ready) {instance_name = "b", depth = #n3, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    "ac.yield"(%a_ready, %b_ready) : (!b1, !b1) -> ()
  }) {sym_name = "Pair", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1) -> (!b1, !b1), input_names = ["valid", "data", "clk", "rst"], output_names = ["a_ready", "b_ready"]} : () -> ()
  "ac.system"() {entry = {callee = @Pair, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
#n0 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#n3 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#n5 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#n13 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<13>}}>
#n65 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#n130 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#n1>
!b13 = !ac.bits<#n13>
!b65 = !ac.bits<#n65>
!b130 = !ac.bits<#n130>
!table130 = !ac.table<[#n3], !b130>
#depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Generic, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Generic, name = "D"}}>
!generic = !ac.type_param<@Generic, "T">
#token_depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Token3, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Token3, name = "D"}}>
!token = !ac.type_param<@Token3, "T">
!tableToken = !ac.table<[#n3], !token>
module {
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b13, %clk: !b1, %rst: !b1):
    %a_ready, %a_valid, %a_head = "ac.queue"(%clk, %rst, %valid, %data, %b_ready) {instance_name = "a", depth = #n3, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %b_ready, %b_valid, %b_head = "ac.queue"(%clk, %rst, %valid, %data, %a_ready) {instance_name = "b", depth = #n3, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %constant = "ac.bits.constant"() {value = #n0} : () -> !b1
    "ac.yield"(%constant) : (!b1) -> ()
  }) {sym_name = "Pair", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1) -> !b1, input_names = ["valid", "data", "clk", "rst"], output_names = ["constant"]} : () -> ()
  "ac.system"() {entry = {callee = @Pair, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
#n0 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#n3 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#n5 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#n13 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<13>}}>
#n65 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#n130 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#n1>
!b13 = !ac.bits<#n13>
!b65 = !ac.bits<#n65>
!b130 = !ac.bits<#n130>
!table130 = !ac.table<[#n3], !b130>
#depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Generic, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Generic, name = "D"}}>
!generic = !ac.type_param<@Generic, "T">
#token_depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Token3, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Token3, name = "D"}}>
!token = !ac.type_param<@Token3, "T">
!tableToken = !ac.table<[#n3], !token>
module {
  // expected-error @+1 {{combinational cycle in hardware field dependencies}}
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b13, %clk: !b1, %rst: !b1):
    %a_ready, %a_valid, %a_head = "ac.queue"(%clk, %rst, %valid, %data, %b_ready) {instance_name = "a", depth = #n3, ready_policy = "downstream_pop", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %b_ready, %b_valid, %b_head = "ac.queue"(%clk, %rst, %valid, %data, %a_ready) {instance_name = "b", depth = #n3, ready_policy = "downstream_pop", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    "ac.yield"(%a_ready, %b_ready) : (!b1, !b1) -> ()
  }) {sym_name = "Pair", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1) -> (!b1, !b1), input_names = ["valid", "data", "clk", "rst"], output_names = ["a_ready", "b_ready"]} : () -> ()
  "ac.system"() {entry = {callee = @Pair, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
#n0 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#n3 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#n5 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#n13 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<13>}}>
#n65 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#n130 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#n1>
!b13 = !ac.bits<#n13>
!b65 = !ac.bits<#n65>
!b130 = !ac.bits<#n130>
!table130 = !ac.table<[#n3], !b130>
#depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Generic, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Generic, name = "D"}}>
!generic = !ac.type_param<@Generic, "T">
#token_depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Token3, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Token3, name = "D"}}>
!token = !ac.type_param<@Token3, "T">
!tableToken = !ac.table<[#n3], !token>
module {
  // expected-error @+1 {{combinational cycle in hardware field dependencies}}
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b13, %clk: !b1, %rst: !b1):
    %a_ready, %a_valid, %a_head = "ac.queue"(%clk, %rst, %valid, %data, %b_ready) {instance_name = "a", depth = #n3, ready_policy = "downstream_pop", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %b_ready, %b_valid, %b_head = "ac.queue"(%clk, %rst, %valid, %data, %a_ready) {instance_name = "b", depth = #n3, ready_policy = "downstream_pop", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %constant = "ac.bits.constant"() {value = #n0} : () -> !b1
    "ac.yield"(%constant) : (!b1) -> ()
  }) {sym_name = "Pair", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1) -> !b1, input_names = ["valid", "data", "clk", "rst"], output_names = ["constant"]} : () -> ()
  "ac.system"() {entry = {callee = @Pair, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
#n0 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#n3 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#n5 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#n13 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<13>}}>
#n65 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#n130 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#n1>
!b13 = !ac.bits<#n13>
!b65 = !ac.bits<#n65>
!b130 = !ac.bits<#n130>
!table130 = !ac.table<[#n3], !b130>
#depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Generic, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Generic, name = "D"}}>
!generic = !ac.type_param<@Generic, "T">
#token_depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Token3, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Token3, name = "D"}}>
!token = !ac.type_param<@Token3, "T">
!tableToken = !ac.table<[#n3], !token>
module {
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b13, %clk: !b1, %rst: !b1):
    %a_ready, %a_valid, %a_head = "ac.queue"(%clk, %rst, %valid, %data, %b_ready) {instance_name = "a", depth = #n3, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %b_ready, %b_valid, %b_head = "ac.queue"(%clk, %rst, %valid, %data, %a_ready) {instance_name = "b", depth = #n3, ready_policy = "downstream_pop", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    "ac.yield"(%a_ready, %b_ready) : (!b1, !b1) -> ()
  }) {sym_name = "Pair", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1) -> (!b1, !b1), input_names = ["valid", "data", "clk", "rst"], output_names = ["a_ready", "b_ready"]} : () -> ()
  "ac.system"() {entry = {callee = @Pair, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
#n0 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#n3 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#n5 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#n13 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<13>}}>
#n65 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#n130 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#n1>
!b13 = !ac.bits<#n13>
!b65 = !ac.bits<#n65>
!b130 = !ac.bits<#n130>
!table130 = !ac.table<[#n3], !b130>
#depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Generic, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Generic, name = "D"}}>
!generic = !ac.type_param<@Generic, "T">
#token_depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Token3, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Token3, name = "D"}}>
!token = !ac.type_param<@Token3, "T">
!tableToken = !ac.table<[#n3], !token>
module {
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b13, %clk: !b1, %rst: !b1):
    %a_ready, %a_valid, %a_head = "ac.queue"(%clk, %rst, %valid, %data, %b_ready) {instance_name = "a", depth = #n3, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %b_ready, %b_valid, %b_head = "ac.queue"(%clk, %rst, %valid, %data, %a_ready) {instance_name = "b", depth = #n3, ready_policy = "downstream_pop", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Pair, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %constant = "ac.bits.constant"() {value = #n0} : () -> !b1
    "ac.yield"(%constant) : (!b1) -> ()
  }) {sym_name = "Pair", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1) -> !b1, input_names = ["valid", "data", "clk", "rst"], output_names = ["constant"]} : () -> ()
  "ac.system"() {entry = {callee = @Pair, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
#n0 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#n3 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#n5 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#n13 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<13>}}>
#n65 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#n130 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#n1>
!b13 = !ac.bits<#n13>
!b65 = !ac.bits<#n65>
!b130 = !ac.bits<#n130>
!table130 = !ac.table<[#n3], !b130>
#depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Generic, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Generic, name = "D"}}>
!generic = !ac.type_param<@Generic, "T">
#token_depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Token3, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Token3, name = "D"}}>
!token = !ac.type_param<@Token3, "T">
!tableToken = !ac.table<[#n3], !token>
module {
  // expected-error @+1 {{combinational cycle in hardware field dependencies}}
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b13, %take: !b1, %clk: !b1, %rst: !b1):
    %cycle = "ac.bits.binary"(%cycle, %valid) {opcode = "xor"} : (!b1, !b1) -> !b1
    %ready, %valid_out, %head = "ac.queue"(%clk, %rst, %cycle, %data, %take) {instance_name = "fifo", depth = #n3, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Provider, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %constant = "ac.bits.constant"() {value = #n0} : () -> !b1
    "ac.yield"(%constant) : (!b1) -> ()
  }) {sym_name = "Provider", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1, !b1) -> !b1, input_names = ["in_valid", "in_data", "out_ready", "clk", "rst"], output_names = ["constant"]} : () -> ()
  "ac.system"() {entry = {callee = @Provider, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
#n0 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#n3 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#n5 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#n13 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<13>}}>
#n65 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#n130 = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#n1>
!b13 = !ac.bits<#n13>
!b65 = !ac.bits<#n65>
!b130 = !ac.bits<#n130>
!table130 = !ac.table<[#n3], !b130>
#depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Generic, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Generic, name = "D"}}>
!generic = !ac.type_param<@Generic, "T">
#token_depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Token3, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Token3, name = "D"}}>
!token = !ac.type_param<@Token3, "T">
!tableToken = !ac.table<[#n3], !token>
module {
  // expected-error @+1 {{combinational cycle in hardware field dependencies}}
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b13, %take: !b1, %clk: !b1, %rst: !b1):
    %cycle = "ac.bits.binary"(%cycle, %data) {opcode = "xor"} : (!b13, !b13) -> !b13
    %ready, %valid_out, %head = "ac.queue"(%clk, %rst, %valid, %cycle, %take) {instance_name = "fifo", depth = #n3, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Provider, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    %constant = "ac.bits.constant"() {value = #n0} : () -> !b1
    "ac.yield"(%constant) : (!b1) -> ()
  }) {sym_name = "Provider", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1, !b1) -> !b1, input_names = ["in_valid", "in_data", "out_ready", "clk", "rst"], output_names = ["constant"]} : () -> ()
  "ac.system"() {entry = {callee = @Provider, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
