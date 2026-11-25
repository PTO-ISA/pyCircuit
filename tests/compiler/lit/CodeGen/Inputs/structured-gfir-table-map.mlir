#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "table_map.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "table_map.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "table_map.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w4 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<4>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "table_map.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "table_map.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b4 = !ac.bits<#w4>
!b8 = !ac.bits<#w8>
!Rows = !ac.table<[#w3], !b8>
!Flags = !ac.table<[#w3], !b1>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @Update(%rows: !Rows, %index: !b4, %data: !b8) -> (!Rows)
      attributes {source_owner = {package = "", path = "table_map.py"}, input_names = ["rows", "index", "data"], output_names = ["updated"]} {
    %updated = "ac.table.map"(%rows, %index, %data) ({
    ^bb0(%ordinal: !b2, %old: !b8, %selected: !b4, %replacement: !b8):
      %wide = "ac.bits.resize"(%ordinal) {mode = "zext"} : (!b2) -> !b4
      %hit = "ac.bits.compare"(%wide, %selected) {predicate = "eq"} : (!b4, !b4) -> !b1
      %next = "ac.bits.select"(%hit, %replacement, %old) : (!b1, !b8, !b8) -> !b8
      "ac.yield"(%next) : (!b8) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 2>} : (!Rows, !b4, !b8) -> !Rows
    gfir.yield %updated : !Rows
  }
  gfir.module @Read(%rows: !Rows, %index: !b4) -> (!b8, !b8, !b8, !b8, !b1)
      attributes {source_owner = {package = "", path = "table_map.py"}, input_names = ["rows", "index"], output_names = ["row0", "row1", "row2", "selected", "in_range"]} {
    %zero = "ac.bits.constant4"() {pattern = "00"} : () -> !b2
    %one = "ac.bits.constant4"() {pattern = "01"} : () -> !b2
    %two = "ac.bits.constant4"() {pattern = "10"} : () -> !b2
    %row0, %r0 = "ac.table.get"(%rows, %zero) : (!Rows, !b2) -> (!b8, !b1)
    %row1, %r1 = "ac.table.get"(%rows, %one) : (!Rows, !b2) -> (!b8, !b1)
    %row2, %r2 = "ac.table.get"(%rows, %two) : (!Rows, !b2) -> (!b8, !b1)
    %selected, %in_range = "ac.table.get"(%rows, %index) : (!Rows, !b4) -> (!b8, !b1)
    gfir.yield %row0, %row1, %row2, %selected, %in_range : !b8, !b8, !b8, !b8, !b1
  }
  gfir.module @Top(%index: !b4, %base: !b8, %data: !b8, %enable: !b1) -> (!b8, !b8, !b8, !b8, !b1)
      attributes {source_owner = {package = "", path = "table_map.py"}, input_names = ["index", "base", "data", "enable"], output_names = ["row0", "row1", "row2", "selected", "in_range"]} {
    %rows = "ac.table.splat"(%base) {shape = [#w3]} : (!b8) -> !Rows
    gfir.instance @update of @Update
    gfir.instance @read of @Read
    %updated = gfir.call @update(%rows, %index, %data) : (!Rows, !b4, !b8) -> !Rows
    %flags = "ac.table.splat"(%enable) {shape = [#w3]} : (!b1) -> !Flags
    %chosen = "ac.table.map"(%flags, %updated, %rows) ({
    ^bb0(%ordinal: !b2, %keep: !b1, %new: !b8, %old: !b8):
      %row = "ac.bits.select"(%keep, %new, %old) : (!b1, !b8, !b8) -> !b8
      "ac.yield"(%row) : (!b8) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 3, 0>} : (!Flags, !Rows, !Rows) -> !Rows
    %r0, %r1, %r2, %selected, %range = gfir.call @read(%chosen, %index) : (!Rows, !b4) -> (!b8, !b8, !b8, !b8, !b1)
    gfir.yield %r0, %r1, %r2, %selected, %range : !b8, !b8, !b8, !b8, !b1
  }
  gfir.root @Top kind "module" {domains = []}
}
