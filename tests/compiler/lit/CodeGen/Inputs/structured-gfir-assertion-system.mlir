#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Assertions, ast_path = []}, expansion = []}, location = {path = "assertions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Assertions, ast_path = []}, expansion = []}, location = {path = "assertions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @Assertions() -> ()
      attributes {source_owner = {package = "", path = "assertions.py"}, input_names = [], output_names = [], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @ticks init %zero : !b8
    gfir.rule @advance() -> () {
      %old = gfir.read @ticks : !b8
      %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @ticks, %next : !b8
      %initial = "ac.bits.compare"(%old, %zero) {predicate = "eq"} : (!b8, !b8) -> !b1
      gfir.effect assert %initial {message = "system count assertion"} : !b1 loc("assertions.py":30:1)
      gfir.yield
    }
    gfir.yield
  }
  gfir.root @Assertions kind "system" {
    domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}],
    ac.system_samples = {
      rows = ["00", "10", "00", "10"],
      origin = {site = {definition = @Assertions, ast_path = []}, expansion = []},
      location = {path = "assertions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}
  }
}
