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
!outer = !ac.struct<"Outer">
!records = !ac.table<[#w3], !outer>
!xs = !ac.table<[#w3], !b8>
module {
  ac.struct "Inner" fields [{name = "x", type = !b8}, {name = "y", type = !b8}]
  ac.struct "Outer" fields [{name = "tag", type = !b4}, {name = "inner", type = !ac.struct<"Inner">}, {name = "tail", type = !b8}]
  "ac.module"() ({
  ^bb0(%base: !outer, %guard_tag: !b1, %guard_y: !b1, %tag_candidate: !b4, %y_candidate: !b8, %records: !records, %query_tag: !b4, %index: !b70):
    %merged, %merge_en = "ac.value.merge"(%base, %guard_tag, %guard_y, %tag_candidate, %y_candidate) {paths = [["tag"], ["inner", "y"]], operandSegmentSizes = array<i32: 1, 2, 2>} : (!outer, !b1, !b1, !b4, !b8) -> (!outer, !b1)
    %unchanged, %zero_en = "ac.value.merge"(%base) {paths = [], operandSegmentSizes = array<i32: 1, 0, 0>} : (!outer) -> (!outer, !b1)
    %xs = "ac.table.map"(%records) ({
    ^bb0(%ordinal: !b2, %entry: !outer):
      %inner = ac.struct.get %entry["inner"] : (!outer) -> !ac.struct<"Inner">
      %x = ac.struct.get %inner["x"] : (!ac.struct<"Inner">) -> !b8
      "ac.yield"(%x) : (!b8) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!records) -> !xs
    %mask = "ac.table.match"(%records, %query_tag) ({
    ^bb0(%entry: !outer, %query: !b4):
      %tag = ac.struct.get %entry["tag"] : (!outer) -> !b4
      %hit = "ac.bits.compare"(%tag, %query) {predicate = "eq"} : (!b4, !b4) -> !b1
      "ac.yield"(%hit) : (!b1) -> ()
    }) : (!records, !b4) -> !b3
    %choice0, %choice1, %valid0, %valid1 = "ac.table.choose"(%records, %mask) {count = 2 : i64, policy = "first", order = "low"} : (!records, !b3) -> (!b2, !b2, !b1, !b1)
    %selected, %in_range = "ac.table.get"(%records, %index) : (!records, !b70) -> (!outer, !b1)
    "ac.yield"(%merged, %merge_en, %unchanged, %zero_en, %xs, %mask, %choice0, %choice1, %valid0, %valid1, %selected, %in_range) : (!outer, !b1, !outer, !b1, !xs, !b3, !b2, !b2, !b1, !b1, !outer, !b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!outer, !b1, !b1, !b4, !b8, !records, !b4, !b70) -> (!outer, !b1, !outer, !b1, !xs, !b3, !b2, !b2, !b1, !b1, !outer, !b1), input_names = ["base", "guard_tag", "guard_y", "tag_candidate", "y_candidate", "records", "query_tag", "index"], output_names = ["merged", "merge_en", "unchanged", "zero_en", "xs", "mask", "choice0", "choice1", "valid0", "valid1", "selected", "in_range"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
