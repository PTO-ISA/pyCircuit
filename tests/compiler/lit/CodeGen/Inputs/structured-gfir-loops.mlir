#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Loops, ast_path = []}, expansion = []}, location = {path = "loops.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Loops, ast_path = []}, expansion = []}, location = {path = "loops.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
!Result = !ac.struct<"Result">
module attributes {gfir.contract = 1 : i64} {
  ac.struct "Result" fields [{name = "old", type = !b8}, {name = "candidate", type = !b8}, {name = "a", type = !b8}, {name = "b", type = !b8}, {name = "zero_a", type = !b8}, {name = "zero_b", type = !b8}, {name = "natural", type = !b1}, {name = "zero_natural", type = !b1}]
  gfir.module @Loops(%x: !b8, %y: !b8) -> (!Result)
      attributes {source_owner = {package = "", path = "loops.py"}, input_names = ["x", "y"], output_names = ["result"], domains = ["default"]} {
    %initial = "ac.bits.constant4"() {pattern = "00000010"} : () -> !b8
    gfir.var @s init %initial : !b8
    gfir.rule @update(%x0 = %x : !b8, %y0 = %y : !b8) -> (!Result) {
      %before = gfir.read @s : !b8
      %a, %b, %natural = gfir.loop range 3 init(%x0, %y0) : (!b8, !b8) -> (!b8, !b8, !b1) {
      ^bb0(%ordinal: i64, %left: !b8, %right: !b8):
        %i = gfir.ordinal_bits %ordinal : i64 -> !b8
        %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
        %offset = "ac.bits.binary"(%i, %one) {opcode = "add"} : (!b8, !b8) -> !b8
        %next = "ac.bits.binary"(%right, %offset) {opcode = "add"} : (!b8, !b8) -> !b8
        %current = gfir.read @s : !b8
        %sum = "ac.bits.binary"(%current, %next) {opcode = "add"} : (!b8, !b8) -> !b8
        gfir.write @s, %sum : !b8
        gfir.step (%next, %left) : (!b8, !b8)
      }
      %za, %zb, %zero_natural = gfir.loop range 0 init(%a, %b) : (!b8, !b8) -> (!b8, !b8, !b1) {
      ^bb0(%unused: i64, %left: !b8, %right: !b8):
        %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
        gfir.write @s, %zero : !b8
        gfir.step (%zero, %zero) : (!b8, !b8)
      }
      %after = gfir.read @s : !b8
      %result = ac.struct.create(%before, %after, %a, %b, %za, %zb, %natural, %zero_natural) : (!b8, !b8, !b8, !b8, !b8, !b8, !b1, !b1) -> !Result
      gfir.yield %result : !Result
    }
    %result = gfir.result @update[0] : !Result
    gfir.yield %result : !Result
  }
  gfir.root @Loops kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
