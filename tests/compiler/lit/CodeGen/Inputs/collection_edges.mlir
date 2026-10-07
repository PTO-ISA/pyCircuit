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
#w65 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
!t1 = !ac.table<[#w1], !b8>
!t3 = !ac.table<[#w3], !b8>
!t4 = !ac.table<[#w4], !b8>
!t5 = !ac.table<[#w5], !b8>
!t65 = !ac.table<[#w65], !b8>
module {
  "ac.module"() ({
  ^bb0(%t1: !t1, %t3: !t3, %t4: !t4, %t5: !t5, %t65: !t65, %coordinate: !b70):
    %f1_add = "ac.table.fold"(%t1) {kind = "add"} : (!t1) -> !b8
    %f1_and = "ac.table.fold"(%t1) {kind = "and"} : (!t1) -> !b8
    %f1_min = "ac.table.fold"(%t1) {kind = "min"} : (!t1) -> !b8
    %f3_add = "ac.table.fold"(%t3) {kind = "add"} : (!t3) -> !b8
    %f3_mul = "ac.table.fold"(%t3) {kind = "mul"} : (!t3) -> !b8
    %f5_add = "ac.table.fold"(%t5) {kind = "add"} : (!t5) -> !b8
    %f5_mul = "ac.table.fold"(%t5) {kind = "mul"} : (!t5) -> !b8
    %f5_and = "ac.table.fold"(%t5) {kind = "and"} : (!t5) -> !b8
    %f5_or = "ac.table.fold"(%t5) {kind = "or"} : (!t5) -> !b8
    %f5_xor = "ac.table.fold"(%t5) {kind = "xor"} : (!t5) -> !b8
    %f5_min = "ac.table.fold"(%t5) {kind = "min"} : (!t5) -> !b8
    %f5_max = "ac.table.fold"(%t5) {kind = "max"} : (!t5) -> !b8
    %f65_add = "ac.table.fold"(%t65) {kind = "add"} : (!t65) -> !b8
    %f65_mul = "ac.table.fold"(%t65) {kind = "mul"} : (!t65) -> !b8
    %index1 = "ac.table.index"(%coordinate) {shape = [#w1]} : (!b70) -> !b1
    %value1, %range1 = "ac.table.get"(%t1, %coordinate) : (!t1, !b70) -> (!b8, !b1)
    %index3 = "ac.table.index"(%coordinate) {shape = [#w3]} : (!b70) -> !b2
    %value3, %range3 = "ac.table.get"(%t3, %coordinate) : (!t3, !b70) -> (!b8, !b1)
    %index4 = "ac.table.index"(%coordinate) {shape = [#w4]} : (!b70) -> !b3
    %value4, %range4 = "ac.table.get"(%t4, %coordinate) : (!t4, !b70) -> (!b8, !b1)
    "ac.yield"(%f1_add, %f1_and, %f1_min, %f3_add, %f3_mul, %f5_add, %f5_mul, %f5_and, %f5_or, %f5_xor, %f5_min, %f5_max, %f65_add, %f65_mul, %index1, %value1, %range1, %index3, %value3, %range3, %index4, %value4, %range4) : (!b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b1, !b8, !b1, !b2, !b8, !b1, !b3, !b8, !b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!t1, !t3, !t4, !t5, !t65, !b70) -> (!b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b1, !b8, !b1, !b2, !b8, !b1, !b3, !b8, !b1), input_names = ["t1", "t3", "t4", "t5", "t65", "coordinate"], output_names = ["f1_add", "f1_and", "f1_min", "f3_add", "f3_mul", "f5_add", "f5_mul", "f5_and", "f5_or", "f5_xor", "f5_min", "f5_max", "f65_add", "f65_mul", "index1", "value1", "range1", "index3", "value3", "range3", "index4", "value4", "range4"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
