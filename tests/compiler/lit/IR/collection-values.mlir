// RUN: %pycircuit_opt %s --ac-verify-hardware | %FileCheck %s
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
  ^bb0(%a: !a, %threshold: !b8, %row: !b70, %col: !b70, %get_index: !b70, %guard: !b1, %base: !b8, %candidate: !b8):
    %mapped = "ac.table.map"(%a, %threshold) ({
    ^bb0(%ordinal_arg: !b3, %entry: !b8, %capture: !b8):
      %result = "ac.bits.binary"(%entry, %capture) {opcode = "add"} : (!b8, !b8) -> !b8
      "ac.yield"(%result) : (!b8) -> ()
    }) {shape = [#w2, #w3], operandSegmentSizes = array<i32: 1, 1>} : (!a, !b8) -> !a
    %transposed = "ac.table.view"(%a) {kind = "transpose", parameters = {axes = [#w1, #w0]}} : (!a) -> !at
    %sliced = "ac.table.view"(%a) {kind = "slice", parameters = {offsets = [#w0, #w0], sizes = [#w2, #w2], strides = [#w1, #w2]}} : (!a) -> !as
    %rotated = "ac.table.view"(%a) {kind = "rotate", parameters = {axis = #w1, offset = #wm1}} : (!a) -> !a
    %reshaped = "ac.table.view"(%a) {kind = "reshape", parameters = {shape = [#w6]}} : (!a) -> !af
    %broadcast = "ac.table.splat"(%threshold) {shape = [#w2, #w3]} : (!b8) -> !a
    %created = "ac.table.create"(%base, %candidate) : (!b8, !b8) -> !apair
    %ordinal = "ac.table.index"(%row, %col) {shape = [#w2, #w3]} : (!b70, !b70) -> !b3
    %selected, %in_range = "ac.table.get"(%a, %get_index) : (!a, !b70) -> (!b8, !b1)
    %mask = "ac.table.match"(%a, %threshold) ({
    ^bb0(%entry: !b8, %capture: !b8):
      %hit = "ac.bits.compare"(%entry, %capture) {predicate = "uge"} : (!b8, !b8) -> !b1
      "ac.yield"(%hit) : (!b1) -> ()
    }) : (!a, !b8) -> !b6
    %low0, %low1, %lowvalid0, %lowvalid1 = "ac.table.choose"(%a, %mask) {count = 2 : i64, policy = "first", order = "low"} : (!a, !b6) -> (!b3, !b3, !b1, !b1)
    %high0, %high1, %highvalid0, %highvalid1 = "ac.table.choose"(%a, %mask) {count = 2 : i64, policy = "first", order = "high"} : (!a, !b6) -> (!b3, !b3, !b1, !b1)
    %add = "ac.table.fold"(%a) {kind = "add"} : (!a) -> !b8
    %mul = "ac.table.fold"(%a) {kind = "mul"} : (!a) -> !b8
    %and = "ac.table.fold"(%a) {kind = "and"} : (!a) -> !b8
    %or = "ac.table.fold"(%a) {kind = "or"} : (!a) -> !b8
    %xor = "ac.table.fold"(%a) {kind = "xor"} : (!a) -> !b8
    %min = "ac.table.fold"(%a) {kind = "min"} : (!a) -> !b8
    %max = "ac.table.fold"(%a) {kind = "max"} : (!a) -> !b8
    %merged, %merge_en = "ac.value.merge"(%base, %guard, %candidate) {paths = [[]], operandSegmentSizes = array<i32: 1, 1, 1>} : (!b8, !b1, !b8) -> (!b8, !b1)
    "ac.yield"(%mapped, %transposed, %sliced, %rotated, %reshaped, %broadcast, %created, %ordinal, %selected, %in_range, %mask, %low0, %low1, %lowvalid0, %lowvalid1, %high0, %high1, %highvalid0, %highvalid1, %add, %mul, %and, %or, %xor, %min, %max, %merged, %merge_en) : (!a, !at, !as, !a, !af, !a, !apair, !b3, !b8, !b1, !b6, !b3, !b3, !b1, !b1, !b3, !b3, !b1, !b1, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a, !b8, !b70, !b70, !b70, !b1, !b8, !b8) -> (!a, !at, !as, !a, !af, !a, !apair, !b3, !b8, !b1, !b6, !b3, !b3, !b1, !b1, !b3, !b3, !b1, !b1, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b8, !b1), input_names = ["a", "threshold", "row", "col", "get_index", "guard", "base", "candidate"], output_names = ["mapped", "transposed", "sliced", "rotated", "reshaped", "broadcast", "created", "ordinal", "selected", "in_range", "mask", "low0", "low1", "lowvalid0", "lowvalid1", "high0", "high1", "highvalid0", "highvalid1", "fold_add", "fold_mul", "fold_and", "fold_or", "fold_xor", "fold_min", "fold_max", "merged", "merge_en"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// CHECK: "ac.table.map"
// CHECK: "ac.table.view"
// CHECK: "ac.table.index"
// CHECK: "ac.table.get"
// CHECK: "ac.table.match"
// CHECK: "ac.table.choose"
// CHECK: "ac.table.fold"
// CHECK: "ac.value.merge"
