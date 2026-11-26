#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Ram, ast_path = []}, expansion = []}, location = {path = "ram.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Ram, ast_path = []}, expansion = []}, location = {path = "ram.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b2 = !ac.bits<#w2>
#w16 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<16>}, origin = {site = {definition = @Ram, ast_path = []}, expansion = []}, location = {path = "ram.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b16 = !ac.bits<#w16>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @RamConsumer, ast_path = []}, expansion = []}, location = {path = "ram_consumer.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#low = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @RamConsumer, ast_path = []}, expansion = []}, location = {path = "ram_consumer.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.primitive @Dual native "memory" version 1 {kind = "sync_mem_dp", element = !b16, depth = 3 : i64, access = "transaction"}
  gfir.module @Ram(%req0: !b1, %req1: !b1, %write: !b1, %addr0: !b2, %addr1: !b2, %waddr: !b2, %data: !b16, %mask: !b2) -> (!b16, !b16)
      attributes {source_owner = {package = "", path = "ram.py"}, input_names = ["req0", "req1", "write", "addr0", "addr1", "waddr", "data", "mask"], output_names = ["a", "b"], domains = ["default"]} {
    gfir.instance @ram of @Dual
    gfir.rule @request0(%en = %req0 : !b1, %address = %addr0 : !b2) -> () {
      gfir.guard %en : !b1
      gfir.call @ram["request0"](%address) : (!b2) -> ()
      gfir.yield
    }
    gfir.rule @request1(%en = %req1 : !b1, %address = %addr1 : !b2) -> () {
      gfir.guard %en : !b1
      gfir.call @ram["request1"](%address) : (!b2) -> ()
      gfir.yield
    }
    gfir.rule @store(%en = %write : !b1, %address = %waddr : !b2, %payload = %data : !b16, %strobe = %mask : !b2) -> () {
      gfir.guard %en : !b1
      gfir.call @ram["write"](%address, %payload, %strobe) : (!b2, !b16, !b2) -> ()
      gfir.yield
    }
    gfir.rule @result0() -> (!b16) {
      %value = gfir.call @ram["result0"]() : () -> !b16
      gfir.yield %value : !b16
    }
    gfir.rule @result1() -> (!b16) {
      %value = gfir.call @ram["result1"]() : () -> !b16
      gfir.yield %value : !b16
    }
    %a = gfir.result @result0[0] : !b16
    %b = gfir.result @result1[0] : !b16
    gfir.yield %a, %b : !b16, !b16
  }
  gfir.module @RamConsumer(%req0: !b1, %req1: !b1, %write: !b1, %addr0: !b2, %addr1: !b2, %waddr: !b2, %data: !b16, %mask: !b2, %consume: !b1) -> (!b16, !b16, !b1, !b8)
      attributes {source_owner = {package = "", path = "ram_consumer.py"}, input_names = ["req0", "req1", "write", "addr0", "addr1", "waddr", "data", "mask", "consume"], output_names = ["a", "b", "response_bit", "count"], domains = ["default"]} {
    gfir.instance @child of @Ram
    %a, %b = gfir.call @child(%req0, %req1, %write, %addr0, %addr1, %waddr, %data, %mask) : (!b1, !b1, !b1, !b2, !b2, !b2, !b16, !b2) -> (!b16, !b16)
    %selected = "ac.bits.extract"(%a) {low = #low} : (!b16) -> !b1
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @accepted init %zero : !b8
    gfir.rule @consumer(%enabled = %consume : !b1, %value = %selected : !b1) -> () {
      gfir.guard %enabled : !b1
      gfir.guard %value : !b1
      %old = gfir.read @accepted : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @accepted, %next : !b8
      gfir.yield
    }
    %count = gfir.read @accepted : !b8
    gfir.yield %a, %b, %selected, %count : !b16, !b16, !b1, !b8
  }
  gfir.root @RamConsumer kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
