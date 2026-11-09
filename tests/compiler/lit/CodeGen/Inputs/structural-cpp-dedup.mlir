#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
module {
  "ac.module"() ({
  ^bb0(%x: !ac.type_param<@identity, "T">):
    %y = "ac.rule"(%x) ({
    ^bb0(%capture: !ac.type_param<@identity, "T">):
      "ac.yield"(%capture) : (!ac.type_param<@identity, "T">) -> ()
    }) {name = "forward", occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!ac.type_param<@identity, "T">) -> !ac.type_param<@identity, "T">
    "ac.yield"(%y) : (!ac.type_param<@identity, "T">) -> ()
  }) {sym_name = "identity", source_owner = {package = "", path = "identity.py"}, parameters = [], type_parameters = ["T"], function_type = (!ac.type_param<@identity, "T">) -> !ac.type_param<@identity, "T">, input_names = ["x"], output_names = ["y"]} : () -> ()
  "ac.module"() ({
  ^bb0(%a: !b8, %b: !b8):
    %x = "ac.instance"(%a) {instance_name = "first", callee = @identity, parameters = [], type_arguments = [!b8], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b8) -> !b8
    %y = "ac.instance"(%b) {instance_name = "second", callee = @identity, parameters = [], type_arguments = [!b8], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b8) -> !b8
    "ac.yield"(%x, %y) : (!b8, !b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b8) -> (!b8, !b8), input_names = ["a", "b"], output_names = ["x", "y"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}