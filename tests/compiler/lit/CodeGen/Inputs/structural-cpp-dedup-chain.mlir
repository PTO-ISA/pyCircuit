#w8 = #ac.static_expr<{kind = "literal", location = {path = "chain.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#one = #ac.static_expr<{kind = "literal", location = {path = "chain.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @increment, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
module {
  "ac.module"() ({
  ^bb0(%x: !b8):
    %one = "ac.bits.constant"() {value = #one} : () -> !b8
    %y = "ac.bits.binary"(%x, %one) {opcode = "add"} : (!b8, !b8) -> !b8
    "ac.yield"(%y) : (!b8) -> ()
  }) {sym_name = "increment", source_owner = {package = "", path = "chain.py"}, parameters = [], type_parameters = [], function_type = (!b8) -> !b8, input_names = ["x"], output_names = ["y"]} : () -> ()
  "ac.module"() ({
  ^bb0(%x: !b8):
    %first = "ac.instance"(%x) {instance_name = "first", callee = @increment, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}} : (!b8) -> !b8
    %second = "ac.instance"(%first) {instance_name = "second", callee = @increment, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}} : (!b8) -> !b8
    %third = "ac.instance"(%second) {instance_name = "third", callee = @increment, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}} : (!b8) -> !b8
    "ac.yield"(%third) : (!b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "chain.py"}, parameters = [], type_parameters = [], function_type = (!b8) -> !b8, input_names = ["x"], output_names = ["y"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
