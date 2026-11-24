#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Bytes, ast_path = []}, expansion = []}, location = {path = "bytes.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Bytes, ast_path = []}, expansion = []}, location = {path = "bytes.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b2 = !ac.bits<#w2>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Bytes, ast_path = []}, expansion = []}, location = {path = "bytes.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
#w16 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<16>}, origin = {site = {definition = @Bytes, ast_path = []}, expansion = []}, location = {path = "bytes.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b16 = !ac.bits<#w16>
module attributes {gfir.contract = 1 : i64} {
  gfir.primitive @Memory native "memory" version 1 {kind = "byte_mem", element = !b16, depth = 3 : i64, access = "transaction"}
  gfir.module @Bytes(%before: !b1, %after: !b1, %address: !b2, %write: !b1, %waddr: !b2, %data: !b16, %mask: !b2) -> (!b16, !b8)
      attributes {source_owner = {package = "", path = "bytes.py"}, input_names = ["earlier", "later", "address", "write", "waddr", "data", "mask"], output_names = ["value", "count"], domains = ["default"]} {
    gfir.instance @ram of @Memory
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @ticks init %zero : !b8
    gfir.rule @counter() -> () {
      %old = gfir.read @ticks : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @ticks, %next : !b8
      gfir.yield
    }
    gfir.rule @store(%enabled = %write : !b1, %target = %waddr : !b2, %payload = %data : !b16, %strobe = %mask : !b2) -> () {
      gfir.guard %enabled : !b1
      gfir.call @ram["write"](%target, %payload, %strobe) : (!b2, !b16, !b2) -> ()
      gfir.yield
    }
    gfir.rule @load(%first = %before : !b1, %last = %after : !b1, %target = %address : !b2) -> (!b16) {
      gfir.guard %first : !b1
      %value = gfir.call @ram["read"](%target) : (!b2) -> !b16
      gfir.guard %last : !b1
      %sentinel = "ac.bits.constant4"() {pattern = "1111111111111111"} : () -> !b16
      %accepted = "ac.bits.compare"(%value, %sentinel) {predicate = "ne"} : (!b16, !b16) -> !b1
      gfir.guard %accepted : !b1
      gfir.yield %value : !b16
    }
    %value = gfir.result @load[0] : !b16
    %count = gfir.read @ticks : !b8
    gfir.yield %value, %count : !b16, !b8
  }
  gfir.root @Bytes kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
