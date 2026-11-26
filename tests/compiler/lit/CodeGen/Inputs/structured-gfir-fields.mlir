#indexMask = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Fields, ast_path = []}, expansion = []}, location = {path = "fields.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Fields, ast_path = []}, expansion = []}, location = {path = "fields.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Fields, ast_path = []}, expansion = []}, location = {path = "fields.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @Fields, ast_path = []}, expansion = []}, location = {path = "fields.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w23 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<23>}, origin = {site = {definition = @Fields, ast_path = []}, expansion = []}, location = {path = "fields.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b23 = !ac.bits<#w23>
!Rows = !ac.table<[#w3], !b23>
!Inner = !ac.struct<"Inner">
!Outer = !ac.struct<"Outer">
module attributes {gfir.contract = 1 : i64} {
  ac.struct "Inner" fields [{name = "tag", type = !b23}, {name = "rows", type = !Rows}]
  ac.struct "Outer" fields [{name = "untouched", type = !b23}, {name = "nested", type = !Inner}]
  gfir.module @Fields(%raw: !b2, %data: !b23, %tag: !b23) -> (!Outer, !Outer, !Outer, !b23, !b1)
      attributes {source_owner = {package = "", path = "fields.py"}, input_names = ["raw", "data", "tag"], output_names = ["old", "snapshot", "candidate", "selected", "in_range"], domains = ["default"]} {
    %keep = "ac.bits.constant4"() {pattern = "10100101000000010100101"} : () -> !b23
    %tag0 = "ac.bits.constant4"() {pattern = "00111100000000000111100"} : () -> !b23
    %row0 = "ac.bits.constant4"() {pattern = "00010001000000000010001"} : () -> !b23
    %rows = "ac.table.splat"(%row0) {shape = [#w3]} : (!b23) -> !Rows
    %inner = ac.struct.create(%tag0, %rows) : (!b23, !Rows) -> !Inner
    %initial = ac.struct.create(%keep, %inner) : (!b23, !Inner) -> !Outer
    gfir.var @s init %initial : !Outer
    gfir.rule @update(%r = %raw : !b2, %d = %data : !b23, %t = %tag : !b23) -> (!Outer, !Outer) {
      %mask = "ac.bits.constant"() {value = #indexMask} : () -> !b2
      %index = "ac.bits.binary"(%r, %mask) {opcode = "and"} : (!b2, !b2) -> !b2
      gfir.write @s["nested"]["tag"], %t : !b23
      %before = gfir.read @s : !Outer
      gfir.write @s["nested"]["rows"][%index : !b2], %d : !b23
      gfir.write @s["nested"]["tag"], %d : !b23
      %after = gfir.read @s : !Outer
      gfir.yield %before, %after : !Outer, !Outer
    }
    %old = gfir.read @s : !Outer
    %snapshot = gfir.result @update[0] : !Outer
    %candidate = gfir.result @update[1] : !Outer
    %prior = ac.struct.get %old["nested"] : (!Outer) -> !Inner
    %prior_rows = ac.struct.get %prior["rows"] : (!Inner) -> !Rows
    %selected, %in_range = "ac.table.get"(%prior_rows, %raw) : (!Rows, !b2) -> (!b23, !b1)
    gfir.yield %old, %snapshot, %candidate, %selected, %in_range : !Outer, !Outer, !Outer, !b23, !b1
  }
  gfir.root @Fields kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
