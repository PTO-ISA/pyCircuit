#indexMask = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Fields, ast_path = []}, expansion = []}, location = {path = "fields.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Fields, ast_path = []}, expansion = []}, location = {path = "fields.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @Fields, ast_path = []}, expansion = []}, location = {path = "fields.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Fields, ast_path = []}, expansion = []}, location = {path = "fields.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b2 = !ac.bits<#w2>
!b8 = !ac.bits<#w8>
!Rows = !ac.table<[#w3], !b8>
!Inner = !ac.struct<"Inner">
!Outer = !ac.struct<"Outer">
module attributes {gfir.contract = 1 : i64} {
  ac.struct "Inner" fields [{name = "tag", type = !b8}, {name = "rows", type = !Rows}]
  ac.struct "Outer" fields [{name = "untouched", type = !b8}, {name = "nested", type = !Inner}]
  gfir.module @Fields(%raw: !b2, %data: !b8, %tag: !b8) -> (!Outer, !Outer, !Outer)
      attributes {source_owner = {package = "", path = "fields.py"}, input_names = ["raw", "data", "tag"], output_names = ["old", "snapshot", "candidate"], domains = ["default"]} {
    %keep = "ac.bits.constant4"() {pattern = "10100101"} : () -> !b8
    %tag0 = "ac.bits.constant4"() {pattern = "00111100"} : () -> !b8
    %row0 = "ac.bits.constant4"() {pattern = "00010001"} : () -> !b8
    %rows = "ac.table.splat"(%row0) {shape = [#w3]} : (!b8) -> !Rows
    %inner = ac.struct.create(%tag0, %rows) : (!b8, !Rows) -> !Inner
    %initial = ac.struct.create(%keep, %inner) : (!b8, !Inner) -> !Outer
    gfir.var @s init %initial : !Outer
    gfir.rule @update(%r = %raw : !b2, %d = %data : !b8, %t = %tag : !b8) -> (!Outer, !Outer) {
      %mask = "ac.bits.constant"() {value = #indexMask} : () -> !b2
      %index = "ac.bits.binary"(%r, %mask) {opcode = "and"} : (!b2, !b2) -> !b2
      gfir.write @s["nested"]["tag"], %t : !b8
      %before = gfir.read @s : !Outer
      gfir.write @s["nested"]["rows"][%index : !b2], %d : !b8
      gfir.write @s["nested"]["tag"], %d : !b8
      %after = gfir.read @s : !Outer
      gfir.yield %before, %after : !Outer, !Outer
    }
    %old = gfir.read @s : !Outer
    %snapshot = gfir.result @update[0] : !Outer
    %candidate = gfir.result @update[1] : !Outer
    gfir.yield %old, %snapshot, %candidate : !Outer, !Outer, !Outer
  }
  gfir.root @Fields kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
