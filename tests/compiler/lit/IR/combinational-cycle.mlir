// RUN: %pycircuit_opt %s --ac-verify-hardware --verify-diagnostics
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#w1>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b5 = !ac.bits<#w5>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b70 = !ac.bits<#w70>
module {
  ac.struct "Pair" fields [{name = "a", type = !b8}, {name = "b", type = !b8}]
  "ac.module"() ({
  ^bb0(%x: !ac.struct<"Pair">):
    %a = ac.struct.get %x["a"] : (!ac.struct<"Pair">) -> !b8
    %b = ac.struct.get %x["b"] : (!ac.struct<"Pair">) -> !b8
    %out = ac.struct.create(%a, %b) : (!b8, !b8) -> !ac.struct<"Pair">
    "ac.yield"(%out) : (!ac.struct<"Pair">) -> ()
  }) {sym_name = "child", source_owner = {package = "", path = "child.py"}, parameters = [], type_parameters = [], function_type = (!ac.struct<"Pair">) -> !ac.struct<"Pair">, input_names = ["x"], output_names = ["y"]} : () -> ()
  // expected-error @+1 {{combinational cycle in hardware field dependencies}}
  "ac.module"() ({
  ^bb0(%a: !b8):
    %x = ac.struct.create(%a, %feedback) : (!b8, !b8) -> !ac.struct<"Pair">
    %y = "ac.instance"(%x) {instance_name = "nested", callee = @child, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!ac.struct<"Pair">) -> !ac.struct<"Pair">
    %feedback = "ac.rule"(%y) ({
    ^bb0(%pair: !ac.struct<"Pair">):
      %first = ac.struct.get %pair["b"] : (!ac.struct<"Pair">) -> !b8
      %not = "ac.bits.unary"(%first) {opcode = "not"} : (!b8) -> !b8
      "ac.yield"(%not) : (!b8) -> ()
    }) {name = "invert_a", occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!ac.struct<"Pair">) -> !b8
    %second = ac.struct.get %y["b"] : (!ac.struct<"Pair">) -> !b8
    "ac.yield"(%second) : (!b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b8) -> !b8, input_names = ["a"], output_names = ["q"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
