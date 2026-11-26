#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Parent, ast_path = []}, expansion = []}, location = {path = "parent.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Parent, ast_path = []}, expansion = []}, location = {path = "parent.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @Accumulator() -> (!b8)
      attributes {source_owner = {package = "", path = "accumulator.py"}, input_names = [], output_names = ["total"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @total init %zero : !b8
    gfir.method @add(%delta: !b8) -> (!b8) {
      %old = gfir.read @total : !b8
      %next = "ac.bits.binary"(%old, %delta) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @total, %next : !b8
      gfir.yield %next : !b8
    }
    gfir.method @snapshot() -> (!b8) {
      %old = gfir.read @total : !b8
      gfir.yield %old : !b8
    }
    gfir.method @add_and_read(%delta: !b8) -> (!b8) {
      %sum = gfir.call self["add"](%delta) : (!b8) -> !b8
      %candidate = gfir.call self["snapshot"]() : () -> !b8
      gfir.yield %candidate : !b8
    }
    %old = gfir.read @total : !b8
    gfir.yield %old : !b8
  }
  gfir.module @Parent(%allow: !b1) -> (!b8, !b8, !b8, !b8, !b8, !b8, !b8)
      attributes {source_owner = {package = "", path = "parent.py"}, input_names = ["allow"], output_names = ["q", "observed", "first", "second", "q_b", "observed_b", "untouched"], domains = ["default"]} {
    gfir.instance @child of @Accumulator {domains = {default = "default"}}
    gfir.instance @other of @Accumulator {domains = {default = "default"}}
    gfir.instance @idle of @Accumulator {domains = {default = "default"}}
    %q_b = gfir.call @other() : () -> !b8
    %untouched = gfir.call @idle() : () -> !b8
    %q = gfir.call @child() : () -> !b8
    gfir.rule @writer(%enabled = %allow : !b1) -> (!b8, !b8) {
      %three = "ac.bits.constant4"() {pattern = "00000011"} : () -> !b8
      %five = "ac.bits.constant4"() {pattern = "00000101"} : () -> !b8
      %first = gfir.call @child["add_and_read"](%three) : (!b8) -> !b8
      %second = gfir.call @child["add"](%five) : (!b8) -> !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %other = gfir.call @other["add"](%one) : (!b8) -> !b8
      gfir.guard %enabled : !b1 loc("parent.py":20:1)
      gfir.yield %first, %second : !b8, !b8
    }
    gfir.rule @inspect() -> (!b8) {
      %old = gfir.call @child["snapshot"]() : () -> !b8
      gfir.yield %old : !b8
    }
    gfir.rule @inspect_b() -> (!b8) {
      %old = gfir.call @other["snapshot"]() : () -> !b8
      gfir.yield %old : !b8
    }
    %observed_b = gfir.result @inspect_b[0] : !b8
    %observed = gfir.result @inspect[0] : !b8
    %first = gfir.result @writer[0] : !b8
    %second = gfir.result @writer[1] : !b8
    gfir.yield %q, %observed, %first, %second, %q_b, %observed_b, %untouched : !b8, !b8, !b8, !b8, !b8, !b8, !b8
  }
  gfir.root @Parent kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
