#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.primitive @ByteFifo native "fifo" version 1
      {element = !b8, depth = 1 : i64, latency = 1 : i64, ready_policy = "downstream_pop"}
  gfir.module @cell(%peer: !b8, %increment: !b8, %valid: !b1) -> (!b8)
      attributes {source_owner = {package = "", path = "cell.py"},
                  input_names = ["peer", "increment", "valid"], output_names = ["value"],
                  domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    %one = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
    gfir.var @value init %zero : !b8
    gfir.instance @queue of @ByteFifo
    %ready = gfir.call @queue["raw_in"](%valid, %peer) : (!b1, !b8) -> !b1
    %live, %head = gfir.call @queue["raw_out"](%one) : (!b1) -> (!b1, !b8)
    gfir.rule @advance(%input = %peer : !b8, %delta = %increment : !b8) -> () {
      %next = "ac.bits.binary"(%input, %delta) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @value, %next : !b8
      gfir.yield
    }
    // Committed state cuts the feedback dependency, even though updates use peer.
    %old = gfir.read @value : !b8
    gfir.yield %old : !b8
  }
  gfir.module @top(%late_valid: !b1) -> (!b8, !b8)
      attributes {source_owner = {package = "", path = "top.py"},
                  input_names = ["late_valid"], output_names = ["left", "right"],
                  domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
    %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
    %three = "ac.bits.constant4"() {pattern = "00000011"} : () -> !b8
    gfir.instance @first of @cell
    gfir.instance @second of @cell {domains = {default = "default"}}
    %left = gfir.call @first(%right, %one, %zero) : (!b8, !b8, !b1) -> !b8
    %right = gfir.call @second(%left, %three, %late_valid) : (!b8, !b8, !b1) -> !b8
    gfir.yield %left, %right : !b8, !b8
  }
  gfir.root @top kind "module" {
    domains = [{name = "default", edge = "rising",
                clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]
  }
}
