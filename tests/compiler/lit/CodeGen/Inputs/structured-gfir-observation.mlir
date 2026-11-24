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
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @ticks, %next : !b8
      gfir.yield
    }
    gfir.rule @observe() -> () {
      %old = gfir.read @ticks : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.effect log(%old, %next) {spec = {event = "transition", level = "info", items = [{kind = "value", ordinal = 0 : i64}, {kind = "value", ordinal = 1 : i64}]}} : (!b8, !b8)
      gfir.effect report(%next) {spec = {name = "total"}} : (!b8)
      %allowed = "ac.bits.compare"(%old, %one) {predicate = "ne"} : (!b8, !b8) -> !b1
      gfir.guard %allowed : !b1
      gfir.yield
    }
    gfir.rule @late_check() -> () {
      %old = gfir.read @ticks : !b8
      %two = "ac.bits.constant4"() {pattern = "00000010"} : () -> !b8
      %safe = "ac.bits.compare"(%old, %two) {predicate = "ult"} : (!b8, !b8) -> !b1
      gfir.effect assert %safe {message = "late observation failure"} : !b1
      gfir.yield
    }
    gfir.yield
  }
  gfir.root @Assertions kind "system" {
    domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}],
    ac.system_samples = {
      rows = ["01", "11", "10", "00", "10", "00", "10", "00"],
      origin = {site = {definition = @Assertions, ast_path = []}, expansion = []},
      location = {path = "assertions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}
  }
}
