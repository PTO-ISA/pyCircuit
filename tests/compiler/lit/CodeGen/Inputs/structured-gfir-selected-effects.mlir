#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Effects, ast_path = []}, expansion = []}, location = {path = "effects.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Effects, ast_path = []}, expansion = []}, location = {path = "effects.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @Effects() -> () attributes {source_owner = {package = "", path = "effects.py"}, input_names = [], output_names = [], domains = ["default"]} {
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
      %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %two = "ac.bits.constant4"() {pattern = "00000010"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      %early = "ac.bits.compare"(%old, %two) {predicate = "ult"} : (!b8, !b8) -> !b1
      %is_zero = "ac.bits.compare"(%old, %zero) {predicate = "eq"} : (!b8, !b8) -> !b1
      %is_one = "ac.bits.compare"(%old, %one) {predicate = "eq"} : (!b8, !b8) -> !b1
      %many = "ac.bits.unary"(%early) {opcode = "not"} : (!b1) -> !b1
      gfir.if %early : !b1 -> () {
        gfir.if %is_zero : !b1 -> () {
          gfir.effect log(%old, %next) {spec = {event = "zero", level = "info", items = [{kind = "value", ordinal = 0 : i64}, {kind = "value", ordinal = 1 : i64}]}} : (!b8, !b8)
          gfir.effect report(%next) {spec = {name = "zero_snapshot"}} : (!b8)
          gfir.effect assert %is_zero {message = "selected zero"} : !b1
          gfir.yield
        } else {
          gfir.effect log(%next, %old) {spec = {event = "one", level = "info", items = [{kind = "value", ordinal = 0 : i64}, {kind = "value", ordinal = 1 : i64}]}} : (!b8, !b8)
          gfir.effect report(%old) {spec = {name = "one_snapshot"}} : (!b8)
          gfir.effect assert %is_one {message = "selected one"} : !b1
          gfir.yield
        }
        gfir.yield
      } else {
        gfir.effect log(%old, %next) {spec = {event = "many", level = "info", items = [{kind = "value", ordinal = 0 : i64}, {kind = "value", ordinal = 1 : i64}]}} : (!b8, !b8)
        gfir.effect report(%next) {spec = {name = "many_snapshot"}} : (!b8)
        gfir.effect assert %many {message = "selected many"} : !b1
        gfir.yield
      }
      %allowed = "ac.bits.compare"(%old, %two) {predicate = "ne"} : (!b8, !b8) -> !b1
      gfir.guard %allowed : !b1
      gfir.yield
    }
    gfir.rule @late_failure() -> () {
      %old = gfir.read @ticks : !b8
      %four = "ac.bits.constant4"() {pattern = "00000100"} : () -> !b8
      %safe = "ac.bits.compare"(%old, %four) {predicate = "ult"} : (!b8, !b8) -> !b1
      %one = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
      %bad = "ac.bits.constant4"() {pattern = "x"} : () -> !b1
      %late = "ac.bits.select"(%safe, %one, %bad) : (!b1, !b1, !b1) -> !b1
      gfir.if %late : !b1 -> () {
        gfir.effect assert %one {message = "selected late true"} : !b1
        gfir.yield
      } else {
        gfir.effect assert %one {message = "selected late false"} : !b1
        gfir.yield
      }
      gfir.yield
    }
    gfir.yield
  }
  gfir.root @Effects kind "system" {
    domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}],
    ac.system_samples = {rows = ["01", "11", "10", "00", "10", "10", "00", "10", "00", "10", "10", "00", "10", "00", "10"], origin = {site = {definition = @Effects, ast_path = []}, expansion = []}, location = {path = "effects.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}
  }
}
