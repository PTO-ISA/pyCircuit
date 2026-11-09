#w1 = #ac.static_expr<{kind = "literal", location = {path = "dff.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#w1>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "dff.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
module {
  "ac.module.import"() {semantic_kind = "hardware", sym_name = "storage", source_owner = {package = "gfsim", path = "dff.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "d", "init"], output_names = ["q"], primitive_kind = "dff", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %d: !b8, %init: !b8):
    %q = "ac.instance"(%clk, %rst, %d, %init) {instance_name = "reg", callee = @storage, parameters = [], type_arguments = [!b8], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b1, !b1, !b8, !b8) -> !b8
    "ac.yield"(%q) : (!b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "dff.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b8, !b8) -> !b8, input_names = ["clk", "rst", "d", "init"], output_names = ["q"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
