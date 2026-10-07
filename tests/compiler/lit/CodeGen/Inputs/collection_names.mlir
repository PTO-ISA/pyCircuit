#wm1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<-1>}}>
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w4 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b4 = !ac.bits<#w4>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b70 = !ac.bits<#w70>
!a = !ac.table<[#w2, #w3], !b8>
!at = !ac.table<[#w3, #w2], !b8>
!as = !ac.table<[#w2, #w2], !b8>
!af = !ac.table<[#w6], !b8>
!apair = !ac.table<[#w2], !b8>
module {
  "ac.module"() ({
  ^bb0(%p: !b8, %impl: !b8, %number: !b8):
    "ac.yield"(%p, %impl, %number) : (!b8, !b8, !b8) -> ()
  }) {sym_name = "Foo", source_owner = {package = "", path = "Foo.py"}, parameters = [{name = "K", type = !ac.math_int}], type_parameters = [], function_type = (!b8, !b8, !b8) -> (!b8, !b8, !b8), input_names = ["PYC_COUNT", "implementation_", "n0"], output_names = ["a", "b", "c"]} : () -> ()
  "ac.module"() ({
  ^bb0(%p: !b8, %impl: !b8, %number: !b8):
    %x = "ac.bits.binary"(%p, %impl) {opcode = "xor"} : (!b8, !b8) -> !b8
    %y = "ac.bits.binary"(%x, %number) {opcode = "xor"} : (!b8, !b8) -> !b8
    "ac.yield"(%y) : (!b8) -> ()
  }) {sym_name = "Foo_family", source_owner = {package = "", path = "Foo_family.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b8, !b8) -> !b8, input_names = ["PYC_COUNT", "implementation_", "n0"], output_names = ["q"]} : () -> ()
  "ac.module"() ({
  ^bb0(%p: !b8, %impl: !b8, %number: !b8):
    %a, %b, %c = "ac.instance"(%p, %impl, %number) {instance_name = "first", callee = @Foo, parameters = [#w1], type_arguments = [], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!b8, !b8, !b8) -> (!b8, !b8, !b8)
    %q = "ac.instance"(%p, %impl, %number) {instance_name = "second", callee = @Foo_family, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!b8, !b8, !b8) -> !b8
    "ac.yield"(%a, %b, %c, %q) : (!b8, !b8, !b8, !b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b8, !b8) -> (!b8, !b8, !b8, !b8), input_names = ["PYC_COUNT", "implementation_", "n0"], output_names = ["a", "b", "c", "q"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
