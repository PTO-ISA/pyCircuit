#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @Break, ast_path = []}, expansion = []}, location = {path = "break.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Break, ast_path = []}, expansion = []}, location = {path = "break.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @Break, ast_path = []}, expansion = []}, location = {path = "break.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Break, ast_path = []}, expansion = []}, location = {path = "break.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
!b8 = !ac.bits<#w8>
!Result = !ac.struct<"BreakResult">
module attributes {gfir.contract = 1 : i64} {
  ac.struct "BreakResult" fields [{name = "candidate", type = !b8}, {name = "a", type = !b8}, {name = "b", type = !b8}, {name = "natural", type = !b1}, {name = "zero_natural", type = !b1}]
  gfir.module @Break(%stops: !b3) -> (!b8, !Result)
      attributes {source_owner = {package = "", path = "break.py"}, input_names = ["stops"], output_names = ["q", "result"], domains = ["default"]} {
    %initial = "ac.bits.constant4"() {pattern = "00000010"} : () -> !b8
    gfir.var @state init %initial : !b8
    gfir.rule @update(%mask = %stops : !b3) -> (!Result) {
      %x = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %y = "ac.bits.constant4"() {pattern = "00000111"} : () -> !b8
      %a, %b, %remaining, %natural = gfir.loop range 3 init(%x, %y, %mask) : (!b8, !b8, !b3) -> (!b8, !b8, !b3, !b1) {
      ^bb0(%ordinal: i64, %left: !b8, %right: !b8, %flags: !b3):
        %i = gfir.ordinal_bits %ordinal : i64 -> !b8
        %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
        %offset = "ac.bits.binary"(%i, %one) {opcode = "add"} : (!b8, !b8) -> !b8
        %next = "ac.bits.binary"(%right, %offset) {opcode = "add"} : (!b8, !b8) -> !b8
        %old = gfir.read @state : !b8
        %sum = "ac.bits.binary"(%old, %next) {opcode = "add"} : (!b8, !b8) -> !b8
        gfir.write @state, %sum : !b8
        %shift = "ac.bits.constant4"() {pattern = "001"} : () -> !b3
        %next_flags = "ac.bits.binary"(%flags, %shift) {opcode = "lshr"} : (!b3, !b3) -> !b3
        %stop = "ac.bits.extract"(%flags) {low = #w0} : (!b3) -> !b1
        gfir.step (%next, %left, %next_flags) : (!b8, !b8, !b3) break_if %stop : !b1
      }
      %unused, %zero_natural = gfir.loop range 0 init(%a) : (!b8) -> (!b8, !b1) {
      ^bb0(%ordinal: i64, %value: !b8):
        %bad = "ac.bits.constant4"() {pattern = "x"} : () -> !b1
        %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
        gfir.write @state, %zero : !b8
        gfir.step (%zero) : (!b8) break_if %bad : !b1
      }
      %candidate = gfir.read @state : !b8
      %result = ac.struct.create(%candidate, %a, %b, %natural, %zero_natural) : (!b8, !b8, !b8, !b1, !b1) -> !Result
      gfir.yield %result : !Result
    }
    %q = gfir.read @state : !b8
    %result = gfir.result @update[0] : !Result
    gfir.yield %q, %result : !b8, !Result
  }
  gfir.root @Break kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
