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
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#n = #ac.static_expr<{kind = "reference", location = {path = "family.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @family, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @family, name = "N"}}>
#i = #ac.static_expr<{kind = "reference", location = {path = "family.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @family, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @family, name = "I"}}>
#m = #ac.static_expr<{kind = "reference", location = {path = "child.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @child, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @child, name = "M"}}>
#shift = #ac.static_expr<{kind = "binary", location = {path = "family.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @family, ast_path = []}, expansion = []}, operator = "sub", lhs = #n, rhs = #w1}>
!parent_type = !ac.type_param<@family, "T">
!child_type = !ac.type_param<@child, "U">
!ordinal = !ac.bits<#i>
!parent_input = !ac.table<[#n], !parent_type>
!parent_guard = !ac.table<[#n], !b1>
!parent_output = !ac.table<[#n, #n], !parent_type>
!child_output = !ac.table<[#m], !child_type>
!record = !ac.struct<"Record">
!r3 = !ac.table<[#w3], !record>
!g3 = !ac.table<[#w3], !b1>
!o3 = !ac.table<[#w3, #w3], !record>
!r5 = !ac.table<[#w5], !record>
!g5 = !ac.table<[#w5], !b1>
!o5 = !ac.table<[#w5, #w5], !record>
!bits_input = !ac.table<[#w3], !b8>
!bits_output = !ac.table<[#w3, #w3], !b8>
module {
  ac.struct "Record" fields [{name = "tag", type = !b4}, {name = "data", type = !b8}]
  "ac.module"() ({
  ^bb0(%base: !child_type, %guard: !b1, %candidate: !child_type):
    %next, %enable = "ac.value.merge"(%base, %guard, %candidate) {paths = [[]], operandSegmentSizes = array<i32: 1, 1, 1>} : (!child_type, !b1, !child_type) -> (!child_type, !b1)
    %unchanged, %zero = "ac.value.merge"(%next) {paths = [], operandSegmentSizes = array<i32: 1, 0, 0>} : (!child_type) -> (!child_type, !b1)
    %out = "ac.table.splat"(%unchanged) {shape = [#m]} : (!child_type) -> !child_output
    "ac.yield"(%out) : (!child_output) -> ()
  }) {sym_name = "child", source_owner = {package = "", path = "child.py"}, parameters = [{name = "M", type = !ac.math_int}], type_parameters = ["U"], function_type = (!child_type, !b1, !child_type) -> !child_output, input_names = ["base", "guard", "candidate"], output_names = ["out"]} : () -> ()
  "ac.module"() ({
  ^bb0(%input: !parent_input, %guard: !parent_guard, %candidate: !parent_input):
    %mapped = "ac.table.map"(%input) ({
    ^bb0(%ordinal: !ordinal, %entry: !parent_type):
      "ac.yield"(%entry) : (!parent_type) -> ()
    }) {shape = [#n], operandSegmentSizes = array<i32: 1, 0>} : (!parent_input) -> !parent_input
    %rotated = "ac.table.view"(%mapped) {kind = "rotate", parameters = {axis = #w0, offset = #shift}} : (!parent_input) -> !parent_input
    %out = "ac.collection"(%rotated, %guard, %candidate) {instance_name = "children", callee = @child, parameters = [#n], type_arguments = [!parent_type], shape = [#n], occurrence = {site = {definition = @family, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}]}, expansion = []}} : (!parent_input, !parent_guard, !parent_input) -> !parent_output
    "ac.yield"(%out) : (!parent_output) -> ()
  }) {sym_name = "family", source_owner = {package = "", path = "family.py"}, parameters = [{name = "N", type = !ac.math_int}, {name = "I", type = !ac.math_int}], type_parameters = ["T"], function_type = (!parent_input, !parent_guard, !parent_input) -> !parent_output, input_names = ["input", "guard", "candidate"], output_names = ["out"]} : () -> ()
  "ac.module"() ({
  ^bb0(%r3: !r3, %g3: !g3, %c3: !r3, %r5: !r5, %g5: !g5, %c5: !r5, %bits: !bits_input, %bits_guard: !g3, %bits_candidate: !bits_input):
    %o3 = "ac.instance"(%r3, %g3, %c3) {instance_name = "three", callee = @family, parameters = [#w3, #w2], type_arguments = [!record], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 0 : i64}]}, expansion = []}} : (!r3, !g3, !r3) -> !o3
    %o5 = "ac.instance"(%r5, %g5, %c5) {instance_name = "five", callee = @family, parameters = [#w5, #w3], type_arguments = [!record], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 1 : i64}]}, expansion = []}} : (!r5, !g5, !r5) -> !o5
    %bo = "ac.instance"(%bits, %bits_guard, %bits_candidate) {instance_name = "bits", callee = @family, parameters = [#w3, #w2], type_arguments = [!b8], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}]}, expansion = []}} : (!bits_input, !g3, !bits_input) -> !bits_output
    "ac.yield"(%o3, %o5, %bo) : (!o3, !o5, !bits_output) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!r3, !g3, !r3, !r5, !g5, !r5, !bits_input, !g3, !bits_input) -> (!o3, !o5, !bits_output), input_names = ["r3", "g3", "c3", "r5", "g5", "c5", "bits", "bits_guard", "bits_candidate"], output_names = ["o3", "o5", "bo"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
