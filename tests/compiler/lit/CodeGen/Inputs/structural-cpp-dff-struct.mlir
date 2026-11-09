#w1 = #ac.static_expr<{kind = "literal", location = {path = "dff_struct.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#w1>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "dff_struct.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
module {
  ac.struct "Pair" fields [{name = "lo", type = !b8}, {name = "hi", type = !b8}]
  "ac.module.import"() {semantic_kind = "hardware", sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %en: !b1, %d: !ac.struct<"Pair">, %init: !ac.struct<"Pair">):
    %q = "ac.instance"(%clk, %rst, %en, %d, %init) {instance_name = "reg", callee = @storage, parameters = [], type_arguments = [!ac.struct<"Pair">], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !ac.struct<"Pair">, !ac.struct<"Pair">) -> !ac.struct<"Pair">
    "ac.yield"(%q) : (!ac.struct<"Pair">) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "dff_struct.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !ac.struct<"Pair">, !ac.struct<"Pair">) -> !ac.struct<"Pair">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}