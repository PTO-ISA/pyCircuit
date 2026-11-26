#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @GuardConsumer, ast_path = []}, expansion = []}, location = {path = "guard_consumer.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @GuardConsumer, ast_path = []}, expansion = []}, location = {path = "guard_consumer.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @GuardConsumer(%offer: !b1, %payload: !b1, %enable: !b1, %late: !b1) -> (!b8, !b8)
      attributes {source_owner = {package = "", path = "guard_consumer.py"}, input_names = ["offer", "payload", "enable", "late"], output_names = ["count", "return_count"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @count init %zero : !b8
    gfir.var @return_count init %zero : !b8
    // The producer's ordinary result is consumed locally, never exported.
    gfir.rule @producer(%enabled = %offer : !b1, %value = %payload : !b1) -> (!b1) {
      gfir.guard %enabled : !b1 loc("guard_consumer.py":10:1)
      gfir.yield %value : !b1
    }
    %produced = gfir.result @producer[0] : !b1
    gfir.rule @consumer(%enabled = %enable : !b1, %value = %produced : !b1, %later = %late : !b1) -> (!b8) {
      gfir.guard %enabled : !b1 loc("guard_consumer.py":20:1)
      %old = gfir.read @count : !b8
      %delta = "ac.bits.resize"(%value) {mode = "zext"} : (!b1) -> !b8
      // This proposal demands availability before the later guard even when
      // a successful invocation overwrites it.
      gfir.write @count, %delta : !b8 loc("guard_consumer.py":21:1)
      gfir.guard %later : !b1 loc("guard_consumer.py":22:1)
      gfir.guard %value : !b1 loc("guard_consumer.py":23:1)
      gfir.effect assert %enabled {message = "enabled consumer"} : !b1 loc("guard_consumer.py":24:1)
      %next = "ac.bits.binary"(%old, %delta) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @count, %next : !b8
      gfir.yield %next : !b8
    }
    %unused = gfir.result @consumer[0] : !b8
    // This Rule needs the value only at its reached return. Its caller never
    // projects that result, but an unavailable return still prevents commit.
    gfir.rule @return_value(%enabled = %enable : !b1, %value = %produced : !b1) -> (!b1) {
      gfir.guard %enabled : !b1
      %old = gfir.read @return_count : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @return_count, %next : !b8
      %one_bit = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
      %matches = "ac.bits.compare"(%value, %one_bit) {predicate = "case_eq"} : (!b1, !b1) -> !b1
      gfir.yield %matches : !b1
    }
    %count = gfir.read @count : !b8
    %return_count = gfir.read @return_count : !b8
    gfir.yield %count, %return_count : !b8, !b8
  }
  gfir.root @GuardConsumer kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
