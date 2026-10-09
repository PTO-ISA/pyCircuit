// RUN: %pycircuit_opt %s --split-input-file --ac-verify-hardware --verify-diagnostics
// A forward generic definition and two complete token shapes use real queue owners.
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
#forward_depth = #ac.static_expr<{kind = "reference", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Forward, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Forward, name = "N"}}>
!forward = !ac.type_param<@Forward, "U">
module {
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b130, %table: !table130, %take: !b1, %clk: !b1, %rst: !b1):
    %ready, %available, %head = "ac.instance"(%valid, %data, %take, %clk, %rst) {instance_name = "scalar", callee = @Forward, parameters = [#n3], type_arguments = [!b130], occurrence = {site = {definition = @Root, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b130, !b1, !b1, !b1) -> (!b1, !b1, !b130)
    %table_ready, %table_valid, %table_head = "ac.instance"(%valid, %table, %take, %clk, %rst) {instance_name = "table", callee = @Token3, parameters = [#n5], type_arguments = [!b130], occurrence = {site = {definition = @Root, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b1, !table130, !b1, !b1, !b1) -> (!b1, !b1, !table130)
    "ac.yield"(%ready, %available, %head, %table_ready, %table_valid, %table_head) : (!b1, !b1, !b130, !b1, !b1, !table130) -> ()
  }) {sym_name = "Root", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b130, !table130, !b1, !b1, !b1) -> (!b1, !b1, !b130, !b1, !b1, !table130), input_names = ["valid", "data", "table", "take", "clk", "rst"], output_names = ["ready", "available", "head", "table_ready", "table_valid", "table_head"]} : () -> ()
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !forward, %take: !b1, %clk: !b1, %rst: !b1):
    %ready, %available, %head = "ac.instance"(%valid, %data, %take, %clk, %rst) {instance_name = "forwarded", callee = @Generic, parameters = [#forward_depth], type_arguments = [!forward], occurrence = {site = {definition = @Forward, ast_path = []}, expansion = []}} : (!b1, !forward, !b1, !b1, !b1) -> (!b1, !b1, !forward)
    "ac.yield"(%ready, %available, %head) : (!b1, !b1, !forward) -> ()
  }) {sym_name = "Forward", source_owner = {package = "", path = "__init__.py"}, parameters = [{name = "N", type = !ac.math_int}], type_parameters = ["U"], function_type = (!b1, !forward, !b1, !b1, !b1) -> (!b1, !b1, !forward), input_names = ["valid", "data", "take", "clk", "rst"], output_names = ["ready", "available", "head"]} : () -> ()
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !generic, %take: !b1, %clk: !b1, %rst: !b1):
    %ready, %valid_out, %head = "ac.queue"(%clk, %rst, %valid, %data, %take) {instance_name = "fifo", depth = #depth, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Generic, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !generic, !b1) -> (!b1, !b1, !generic)
    "ac.yield"(%ready, %valid_out, %head) : (!b1, !b1, !generic) -> ()
  }) {sym_name = "Generic", source_owner = {package = "", path = "__init__.py"}, parameters = [{name = "D", type = !ac.math_int}], type_parameters = ["T"], function_type = (!b1, !generic, !b1, !b1, !b1) -> (!b1, !b1, !generic), input_names = ["in_valid", "in_data", "out_ready", "clk", "rst"], output_names = ["in_ready", "out_valid", "out_data"]} : () -> ()
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !tableToken, %take: !b1, %clk: !b1, %rst: !b1):
    %ready, %valid_out, %head = "ac.queue"(%clk, %rst, %valid, %data, %take) {instance_name = "fifo", depth = #token_depth, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Token3, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !tableToken, !b1) -> (!b1, !b1, !tableToken)
    "ac.yield"(%ready, %valid_out, %head) : (!b1, !b1, !tableToken) -> ()
  }) {sym_name = "Token3", source_owner = {package = "", path = "__init__.py"}, parameters = [{name = "D", type = !ac.math_int}], type_parameters = ["T"], function_type = (!b1, !tableToken, !b1, !b1, !b1) -> (!b1, !b1, !tableToken), input_names = ["in_valid", "in_data", "out_ready", "clk", "rst"], output_names = ["in_ready", "out_valid", "out_data"]} : () -> ()
  "ac.system"() {entry = {callee = @Root, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
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
  ^bb0(%valid: !b1, %data: !b13, %take: !b1, %clk: !b1, %rst: !b1):
    // expected-error @+1 {{queue depth must be positive and fit u64}}
    %ready, %valid_out, %head = "ac.queue"(%clk, %rst, %valid, %data, %take) {instance_name = "fifo", depth = #n0, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Provider, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    "ac.yield"(%ready, %valid_out, %head) : (!b1, !b1, !b13) -> ()
  }) {sym_name = "Provider", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1, !b1) -> (!b1, !b1, !b13), input_names = ["in_valid", "in_data", "out_ready", "clk", "rst"], output_names = ["in_ready", "out_valid", "out_data"]} : () -> ()
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
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b13, %take: !b1, %clk: !b1, %rst: !b1):
    // expected-error @+1 {{queue depth must be positive and fit u64}}
    %ready, %valid_out, %head = "ac.queue"(%clk, %rst, %valid, %data, %take) {instance_name = "fifo", depth = #ac.static_expr<{kind = "literal", location = {path = "__init__.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<-1>}}>, ready_policy = "local_occupancy", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Provider, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    "ac.yield"(%ready, %valid_out, %head) : (!b1, !b1, !b13) -> ()
  }) {sym_name = "Provider", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1, !b1) -> (!b1, !b1, !b13), input_names = ["in_valid", "in_data", "out_ready", "clk", "rst"], output_names = ["in_ready", "out_valid", "out_data"]} : () -> ()
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
  "ac.module"() ({
  ^bb0(%valid: !b1, %data: !b13, %take: !b1, %clk: !b1, %rst: !b1):
    // expected-error @+1 {{queue ready_policy must be local_occupancy or downstream_pop}}
    %ready, %valid_out, %head = "ac.queue"(%clk, %rst, %valid, %data, %take) {instance_name = "fifo", depth = #n3, ready_policy = "other", availability_latency = #n1, head_read_latency = #n0, empty_flow = false, read_during_write = "old", reset_policy = "sync_high_empty", empty_data = "zero", occurrence = {site = {definition = @Provider, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !b13, !b1) -> (!b1, !b1, !b13)
    "ac.yield"(%ready, %valid_out, %head) : (!b1, !b1, !b13) -> ()
  }) {sym_name = "Provider", source_owner = {package = "", path = "__init__.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b13, !b1, !b1, !b1) -> (!b1, !b1, !b13), input_names = ["in_valid", "in_data", "out_ready", "clk", "rst"], output_names = ["in_ready", "out_valid", "out_data"]} : () -> ()
  "ac.system"() {entry = {callee = @Provider, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
