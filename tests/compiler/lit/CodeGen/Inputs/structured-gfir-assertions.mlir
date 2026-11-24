#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Assertions, ast_path = []}, expansion = []}, location = {path = "assertions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Assertions, ast_path = []}, expansion = []}, location = {path = "assertions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @Assertions(%predicate: !b1, %allow: !b1) -> (!b8)
      attributes {source_owner = {package = "", path = "assertions.py"}, input_names = ["predicate", "allow"], output_names = ["count"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @ticks init %zero : !b8
    gfir.rule @advance(%condition = %predicate : !b1) -> () {
      %old = gfir.read @ticks : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @ticks, %next : !b8
      gfir.effect assert %condition {message = "cycle assertion"} : !b1 loc("assertions.py":10:1)
      gfir.yield
    }
    // This later Rule's reached guard fault precedes the earlier assertion.
    gfir.rule @late_guard(%enabled = %allow : !b1) -> () {
      gfir.guard %enabled : !b1 loc("assertions.py":20:1)
      gfir.yield
    }
    %count = gfir.read @ticks : !b8
    gfir.yield %count : !b8
  }
  gfir.root @Assertions kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
