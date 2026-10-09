// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics
#wm1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<-1>}}>
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w4 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b4 = !ac.bits<#w4>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b70 = !ac.bits<#w70>
!a = !ac.table<[#w2, #w3], !b8>
!at = !ac.table<[#w3, #w2], !b8>
!as = !ac.table<[#w2, #w2], !b8>
!af = !ac.table<[#w6], !b8>
!apair = !ac.table<[#w2], !b8>
!lanes = !ac.table<[#w3], !b8>
!controls = !ac.table<[#w3], !b1>
module {
  "ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %controls: !controls, %data: !lanes):
    // expected-error @+1 {{instance wire type does not match bound callee signature}}
    %q = "ac.collection"(%clk, %controls, %controls, %data, %data) {instance_name = "family", callee = @storage, parameters = [], type_arguments = [!b8], shape = [#w3], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b1, !controls, !controls, !lanes, !lanes) -> !lanes
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !controls, !lanes) -> (), input_names = ["clk", "controls", "data"], output_names = []} : () -> ()
}
// -----
#wm1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<-1>}}>
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w4 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b4 = !ac.bits<#w4>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b70 = !ac.bits<#w70>
!a = !ac.table<[#w2, #w3], !b8>
!at = !ac.table<[#w3, #w2], !b8>
!as = !ac.table<[#w2, #w2], !b8>
!af = !ac.table<[#w6], !b8>
!apair = !ac.table<[#w2], !b8>
!lanes = !ac.table<[#w3], !b8>
!controls = !ac.table<[#w3], !b1>
module {
  "ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %controls: !controls, %data: !lanes):
    // expected-error @+1 {{unsupported collection layout or hint}}
    %q = "ac.collection"(%controls, %controls, %controls, %data, %data) {instance_name = "family", callee = @storage, parameters = [], type_arguments = [!b8], shape = [#w3], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}, constrained = true} : (!controls, !controls, !controls, !lanes, !lanes) -> !lanes
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !controls, !lanes) -> (), input_names = ["clk", "controls", "data"], output_names = []} : () -> ()
}
// -----
#wm1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<-1>}}>
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w4 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b4 = !ac.bits<#w4>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b70 = !ac.bits<#w70>
!a = !ac.table<[#w2, #w3], !b8>
!at = !ac.table<[#w3, #w2], !b8>
!as = !ac.table<[#w2, #w2], !b8>
!af = !ac.table<[#w6], !b8>
!apair = !ac.table<[#w2], !b8>
!lanes = !ac.table<[#w3], !b8>
!controls = !ac.table<[#w3], !b1>
module {
  "ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %controls: !controls, %data: !lanes):
    // expected-error @+1 {{physical layout accepts only reshape, transpose and rotate steps}}
    %q = "ac.collection"(%controls, %controls, %controls, %data, %data) {instance_name = "family", callee = @storage, parameters = [], type_arguments = [!b8], shape = [#w3], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}, layout = [{kind = "slice", parameters = {offsets = [#w0], sizes = [#w3], strides = [#w1]}}]} : (!controls, !controls, !controls, !lanes, !lanes) -> !lanes
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !controls, !lanes) -> (), input_names = ["clk", "controls", "data"], output_names = []} : () -> ()
}
// -----
#wm1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<-1>}}>
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w4 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b4 = !ac.bits<#w4>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b70 = !ac.bits<#w70>
!a = !ac.table<[#w2, #w3], !b8>
!at = !ac.table<[#w3, #w2], !b8>
!as = !ac.table<[#w2, #w2], !b8>
!af = !ac.table<[#w6], !b8>
!apair = !ac.table<[#w2], !b8>
!lanes = !ac.table<[#w3], !b8>
!controls = !ac.table<[#w3], !b1>
module {
  "ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %controls: !controls, %data: !lanes):
    // expected-error @+1 {{instance_name must be unique within parent}}
    %q = "ac.collection"(%controls, %controls, %controls, %data, %data) {instance_name = "family", callee = @storage, parameters = [], type_arguments = [!b8], shape = [#w3], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!controls, !controls, !controls, !lanes, !lanes) -> !lanes
    %q2 = "ac.collection"(%controls, %controls, %controls, %data, %data) {instance_name = "family", callee = @storage, parameters = [], type_arguments = [!b8], shape = [#w3], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!controls, !controls, !controls, !lanes, !lanes) -> !lanes
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !controls, !lanes) -> (), input_names = ["clk", "controls", "data"], output_names = []} : () -> ()
}
