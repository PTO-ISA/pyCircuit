#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#w1>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b5 = !ac.bits<#w5>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b70 = !ac.bits<#w70>
module {
  "ac.module"() ({
  ^bb0(%a: !b8, %b: !b8):
    %sum = "ac.bits.binary"(%a, %b) {opcode = "add"} : (!b8, !b8) -> !b8
    "ac.yield"(%sum) : (!b8) -> ()
  }) {sym_name = "child", source_owner = {package = "", path = "child.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b8) -> !b8, input_names = ["a", "b"], output_names = ["sum"]} : () -> ()
  "ac.module"() ({
  ^bb0(%x: !b8):
    %q = "ac.instance"(%x, %x) {instance_name = "nested", callee = @child, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b8, !b8) -> !b8
    "ac.yield"(%q) : (!b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b8) -> !b8, input_names = ["x"], output_names = ["q"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
