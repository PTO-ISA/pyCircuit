#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "guard.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "guard.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @Child(%a: !b1, %b: !b1, %select: !b1) -> (!b8)
      attributes {source_owner = {package = "", path = "guard.py"}, input_names = ["a", "b", "select"], output_names = ["value"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @value init %zero : !b8
    gfir.method @inspect() -> (!b8) {
      %old = gfir.read @value : !b8
      gfir.yield %old : !b8
    }
    gfir.rule @advance(%first = %a : !b1, %second = %b : !b1, %selector = %select : !b1) -> () {
      gfir.guard %first : !b1
      gfir.guard %second : !b1
      gfir.if %selector : !b1 -> () {
        %unused = gfir.call self["inspect"]() : () -> !b8
        gfir.yield
      } else { gfir.yield }
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %old = gfir.read @value : !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @value, %next : !b8
      %candidate = gfir.call self["inspect"]() : () -> !b8
      %twice = "ac.bits.binary"(%candidate, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @value, %twice : !b8
      gfir.yield
    }
    %value = gfir.read @value : !b8
    gfir.yield %value : !b8
  }
  gfir.module @Top(%a: !b1, %b: !b1, %select: !b1) -> (!b8, !b8)
      attributes {source_owner = {package = "", path = "guard.py"}, input_names = ["a", "b", "select"], output_names = ["value", "sibling"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @sibling init %zero : !b8
    gfir.rule @independent() -> () {
      %old = gfir.read @sibling : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @sibling, %next : !b8
      gfir.yield
    }
    gfir.instance @child of @Child
    %value = gfir.call @child(%a, %b, %select) : (!b1, !b1, !b1) -> !b8
    %sibling = gfir.read @sibling : !b8
    gfir.yield %value, %sibling : !b8, !b8
  }
  gfir.root @Top kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
