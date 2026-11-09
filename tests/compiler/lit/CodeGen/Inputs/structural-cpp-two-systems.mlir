#n1 = #ac.static_expr<{kind = "literal", location = {path = "two.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#n1>
module {
  "ac.module"() ({
  ^bb0(%x: !b1):
    "ac.yield"(%x) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "two.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["x"], output_names = ["x_out"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}