#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @GuardConsumer, ast_path = []}, expansion = []}, location = {path = "guard_consumer.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @GuardConsumer, ast_path = []}, expansion = []}, location = {path = "guard_consumer.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @GuardConsumer(%offer: !b1, %payload: !b1, %enable: !b1) -> (!b8)
      attributes {source_owner = {package = "", path = "guard_consumer.py"}, input_names = ["offer", "payload", "enable"], output_names = ["count"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @count init %zero : !b8
    // The producer's ordinary result is consumed locally, never exported.
    gfir.rule @producer(%enabled = %offer : !b1, %value = %payload : !b1) -> (!b1) {
      gfir.guard %enabled : !b1 loc("guard_consumer.py":10:1)
      gfir.yield %value : !b1
    }
    %produced = gfir.result @producer[0] : !b1
    gfir.rule @consumer(%enabled = %enable : !b1, %value = %produced : !b1) -> () {
      gfir.guard %enabled : !b1 loc("guard_consumer.py":20:1)
      gfir.guard %value : !b1 loc("guard_consumer.py":21:1)
      %old = gfir.read @count : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @count, %next : !b8
      gfir.yield
    }
    %count = gfir.read @count : !b8
    gfir.yield %count : !b8
  }
  gfir.root @GuardConsumer kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
