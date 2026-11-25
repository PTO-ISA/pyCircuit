#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @SelectedRam, ast_path = []}, expansion = []}, location = {path = "ram_composition.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @SelectedRam, ast_path = []}, expansion = []}, location = {path = "ram_composition.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.primitive @Memory native "memory" version 1 {kind = "sync_mem", element = !b8, depth = 2 : i64, access = "transaction"}
  gfir.module @SelectedRam(%active: !b1, %addr: !b1, %data: !b8, %mask: !b1, %late: !b1, %response: !b1) -> (!b8, !b8)
      attributes {source_owner = {package = "", path = "ram_composition.py"}, input_names = ["active", "addr", "data", "mask", "late", "response"], output_names = ["value", "count"], domains = ["default"]} {
    gfir.instance @ram of @Memory
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @committed init %zero : !b8
    gfir.rule @operate(%enabled = %active : !b1, %address = %addr : !b1, %payload = %data : !b8, %strobe = %mask : !b1, %admit = %late : !b1) -> () {
      gfir.if %enabled : !b1 -> () {
        gfir.call @ram["request0"](%address) : (!b1) -> ()
        gfir.call @ram["write"](%address, %payload, %strobe) : (!b1, !b8, !b1) -> ()
        %old = gfir.read @committed : !b8
        %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
        %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
        gfir.write @committed, %next : !b8
        gfir.yield
      } else { gfir.yield }
      gfir.guard %admit : !b1
      gfir.yield
    }
    gfir.rule @observe(%use = %response : !b1) -> (!b8) {
      %value = gfir.if %use : !b1 -> (!b8) {
        %old = gfir.call @ram["result0"]() : () -> !b8
        gfir.yield %old : !b8
      } else {
        %fallback = "ac.bits.constant4"() {pattern = "01111110"} : () -> !b8
        gfir.yield %fallback : !b8
      }
      gfir.yield %value : !b8
    }
    %value = gfir.result @observe[0] : !b8
    %count = gfir.read @committed : !b8
    gfir.yield %value, %count : !b8, !b8
  }
  gfir.root @SelectedRam kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
