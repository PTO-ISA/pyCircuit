// RUN: %pycircuit_opt %s --ac-verify-hardware | %FileCheck %s
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
  ^bb0(%clk: !b1, %rst: !b1, %en_left: !controls, %en_right: !controls, %init_left: !lanes, %init_right: !lanes, %data: !lanes):
    %clock = "ac.table.splat"(%clk) {shape = [#w3]} : (!b1) -> !controls
    %reset = "ac.table.splat"(%rst) {shape = [#w3]} : (!b1) -> !controls
    %next = "ac.table.view"(%qa) {kind = "rotate", parameters = {axis = #w0, offset = #w1}} : (!lanes) -> !lanes
    %qa = "ac.collection"(%clock, %reset, %en_left, %next, %init_left) {instance_name = "left", layout = [{kind = "rotate", parameters = {axis = #w0, offset = #w1}}], callee = @storage, parameters = [], type_arguments = [!b8], shape = [#w3], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 3 : i64}]}, expansion = []}} : (!controls, !controls, !controls, !lanes, !lanes) -> !lanes
    %qb = "ac.collection"(%clock, %reset, %en_right, %data, %init_right) {instance_name = "right", callee = @storage, parameters = [], type_arguments = [!b8], shape = [#w3], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 4 : i64}]}, expansion = []}} : (!controls, !controls, !controls, !lanes, !lanes) -> !lanes
    "ac.yield"(%qa, %qb) : (!lanes, !lanes) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !controls, !controls, !lanes, !lanes, !lanes) -> (!lanes, !lanes), input_names = ["clk", "rst", "en_left", "en_right", "init_left", "init_right", "data"], output_names = ["qa", "qb"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
// CHECK: "ac.table.view"
// CHECK: "ac.collection"
// CHECK: "ac.collection"
