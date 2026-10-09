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
  ^bb0(%clk: !b1, %rst: !b1, %en: !b1, %d: !b8, %init: !b8):
    %q0 = "ac.instance"(%clk, %rst, %en, %d, %init) {instance_name = "first", callee = @storage, parameters = [], type_arguments = [!b8], occurrence = {site = {definition = @pipe, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b8, !b8) -> !b8
    %q1 = "ac.instance"(%clk, %rst, %en, %q0, %init) {instance_name = "second", callee = @storage, parameters = [], type_arguments = [!b8], occurrence = {site = {definition = @pipe, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b8, !b8) -> !b8
    "ac.yield"(%q0, %q1) : (!b8, !b8) -> ()
  }) {sym_name = "pipe", source_owner = {package = "", path = "pipe.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b8, !b8) -> (!b8, !b8), input_names = ["clk", "rst", "en", "d", "init"], output_names = ["first_q", "second_q"]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %en: !controls, %d: !lanes, %init: !lanes):
    %clock = "ac.table.splat"(%clk) {shape = [#w3]} : (!b1) -> !controls
    %reset = "ac.table.splat"(%rst) {shape = [#w3]} : (!b1) -> !controls
    %first_q, %second_q = "ac.collection"(%clock, %reset, %en, %d, %init) {instance_name = "pipes", callee = @pipe, parameters = [], type_arguments = [], shape = [#w3], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!controls, !controls, !controls, !lanes, !lanes) -> (!lanes, !lanes)
    "ac.yield"(%first_q, %second_q) : (!lanes, !lanes) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !controls, !lanes, !lanes) -> (!lanes, !lanes), input_names = ["clk", "rst", "en", "d", "init"], output_names = ["first_q", "second_q"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
// CHECK-COUNT-2: "ac.instance"
// CHECK-COUNT-1: "ac.collection"
