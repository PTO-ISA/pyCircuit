// RUN: %pycircuit_opt %s --verify-diagnostics
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#w1>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b5 = !ac.bits<#w5>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b70 = !ac.bits<#w70>
module {
  "ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %en: !b1, %d: !b8, %init: !b8):
    // expected-error @+1 {{instance signature arity mismatch}}
    %left = "ac.instance"(%clk, %rst, %en, %d) {instance_name = "left", callee = @storage, parameters = [], type_arguments = [!b8], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !b8) -> !b8
    %invert = "ac.rule"(%d) ({
    ^bb0(%input: !b8):
      %not = "ac.bits.unary"(%input) {opcode = "not"} : (!b8) -> !b8
      "ac.yield"(%not) : (!b8) -> ()
    }) {name = "invert", occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b8) -> !b8
    %right = "ac.instance"(%clk, %rst, %en, %invert, %init) {instance_name = "right", callee = @storage, parameters = [], type_arguments = [!b8], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !b8, !b8) -> !b8
    "ac.yield"(%left, %right) : (!b8, !b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b8, !b8) -> (!b8, !b8), input_names = ["clk", "rst", "en", "d", "init"], output_names = ["qa", "qb"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
