#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#w1>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b5 = !ac.bits<#w5>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b70 = !ac.bits<#w70>
module {
  ac.struct "Body" fields [{name = "kind", type = !ac.bits<#w1>}, {name = "value", type = !b70}]
  ac.struct "Packet" fields [{name = "tag", type = !b8}, {name = "data", type = !ac.struct<"Body">}]
  "ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dff.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "d", "init"], output_names = ["q"], primitive_kind = "dff", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %d: !ac.struct<"Packet">, %init: !ac.struct<"Packet">):
    %q = "ac.instance"(%clk, %rst, %d, %init) {instance_name = "state", callee = @storage, parameters = [], type_arguments = [!ac.struct<"Packet">], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b1, !b1, !ac.struct<"Packet">, !ac.struct<"Packet">) -> !ac.struct<"Packet">
    %tag = "ac.rule"(%q) ({
    ^bb0(%packet: !ac.struct<"Packet">):
      %field = ac.struct.get %packet["tag"] : (!ac.struct<"Packet">) -> !b8
      "ac.yield"(%field) : (!b8) -> ()
    }) {name = "project", occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!ac.struct<"Packet">) -> !b8
    "ac.yield"(%q, %tag) : (!ac.struct<"Packet">, !b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !ac.struct<"Packet">, !ac.struct<"Packet">) -> (!ac.struct<"Packet">, !b8), input_names = ["clk", "rst", "d", "init"], output_names = ["q", "tag"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
