#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#w1>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b5 = !ac.bits<#w5>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b70 = !ac.bits<#w70>
module {
  ac.struct "Packet" fields [{name = "tag", type = !b5}, {name = "data", type = !b70}]
  "ac.module"() ({
  ^bb0(%x: !ac.type_param<@identity, "T">):
    %y = "ac.rule"(%x) ({
    ^bb0(%capture: !ac.type_param<@identity, "T">):
      "ac.yield"(%capture) : (!ac.type_param<@identity, "T">) -> ()
    }) {name = "forward", occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!ac.type_param<@identity, "T">) -> !ac.type_param<@identity, "T">
    "ac.yield"(%y) : (!ac.type_param<@identity, "T">) -> ()
  }) {sym_name = "identity", source_owner = {package = "", path = "identity.py"}, parameters = [], type_parameters = ["T"], function_type = (!ac.type_param<@identity, "T">) -> !ac.type_param<@identity, "T">, input_names = ["x"], output_names = ["y"]} : () -> ()
  "ac.module"() ({
  ^bb0(%a: !b8, %p: !ac.struct<"Packet">):
    %b = "ac.instance"(%a) {instance_name = "scalar", callee = @identity, parameters = [], type_arguments = [!b8], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b8) -> !b8
    %q = "ac.instance"(%p) {instance_name = "aggregate", callee = @identity, parameters = [], type_arguments = [!ac.struct<"Packet">], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!ac.struct<"Packet">) -> !ac.struct<"Packet">
    "ac.yield"(%b, %q) : (!b8, !ac.struct<"Packet">) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b8, !ac.struct<"Packet">) -> (!b8, !ac.struct<"Packet">), input_names = ["a", "p"], output_names = ["b", "q"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
