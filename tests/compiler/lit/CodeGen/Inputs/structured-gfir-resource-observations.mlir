#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @ResourceObservations, ast_path = []}, expansion = []}, location = {path = "resource_observations.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @ResourceObservations, ast_path = []}, expansion = []}, location = {path = "resource_observations.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.primitive @Queue native "fifo" version 1 {element = !b8, depth = 1 : i64, latency = 1 : i64, ready_policy = "downstream_pop"}
  gfir.module @ResourceObservations() -> () attributes {source_owner = {package = "", path = "resource_observations.py"}, input_names = [], output_names = [], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @ticks init %zero : !b8
    gfir.instance @queue of @Queue
    gfir.rule @advance() -> () {
      %old = gfir.read @ticks : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @ticks, %next : !b8
      gfir.yield
    }
    gfir.rule @produce() -> () {
      %old = gfir.read @ticks : !b8
      gfir.call @queue["in"](%old) : (!b8) -> ()
      gfir.yield
    }
    gfir.rule @consume() -> () {
      %old = gfir.read @ticks : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %three = "ac.bits.constant4"() {pattern = "00000011"} : () -> !b8
      %selected = "ac.bits.compare"(%old, %three) {predicate = "ne"} : (!b8, !b8) -> !b1
      %allowed = "ac.bits.compare"(%old, %one) {predicate = "ne"} : (!b8, !b8) -> !b1
      gfir.if %selected : !b1 -> () {
        %head = gfir.call @queue["out"]() : () -> !b8
        gfir.effect log(%head) {spec = {event = "pop", level = "info", items = [{kind = "value", ordinal = 0 : i64}]}} : (!b8)
        gfir.effect report(%head) {spec = {name = "consumed"}} : (!b8)
        gfir.yield
      } else {
        gfir.yield
      }
      gfir.guard %allowed : !b1
      gfir.yield
    }
    gfir.rule @late_failure() -> () {
      %old = gfir.read @ticks : !b8
      %five = "ac.bits.constant4"() {pattern = "00000101"} : () -> !b8
      %early = "ac.bits.compare"(%old, %five) {predicate = "ult"} : (!b8, !b8) -> !b1
      %one = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
      %bad = "ac.bits.constant4"() {pattern = "x"} : () -> !b1
      %selected = "ac.bits.select"(%early, %one, %bad) : (!b1, !b1, !b1) -> !b1
      gfir.if %selected : !b1 -> () {
        gfir.effect assert %one : !b1
        gfir.yield
      } else {
        gfir.effect assert %one : !b1
        gfir.yield
      }
      gfir.yield
    }
    gfir.yield
  }
  gfir.root @ResourceObservations kind "system" {
    domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}],
    ac.system_samples = {rows = ["01", "11", "10", "00", "10", "10", "00", "10", "00", "10", "10", "00", "10", "00", "10", "10", "00", "10"], origin = {site = {definition = @ResourceObservations, ast_path = []}, expansion = []}, location = {path = "resource_observations.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}
  }
}
