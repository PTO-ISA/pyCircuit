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
  ac.struct "First" fields [{name = "bits", type = !b8}]
  ac.struct "Second" fields [{name = "bits", type = !b8}]
  "ac.module"() ({
  ^bb0(%x: !ac.struct<"First">):
    "ac.yield"(%x) : (!ac.struct<"First">) -> ()
  }) {sym_name = "child", source_owner = {package = "", path = "child.py"}, parameters = [], type_parameters = [], function_type = (!ac.struct<"First">) -> !ac.struct<"First">, input_names = ["x"], output_names = ["y"]} : () -> ()
  "ac.module"() ({
  ^bb0(%x: !ac.struct<"Second">):
    // Equal layouts do not make two nominal payload types interchangeable.
    // expected-error @+1 {{instance wire type does not match bound callee signature}}
    %y = "ac.instance"(%x) {instance_name = "nested", callee = @child, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!ac.struct<"Second">) -> !ac.struct<"First">
    "ac.yield"(%y) : (!ac.struct<"First">) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!ac.struct<"Second">) -> !ac.struct<"First">, input_names = ["x"], output_names = ["y"]} : () -> ()
}
