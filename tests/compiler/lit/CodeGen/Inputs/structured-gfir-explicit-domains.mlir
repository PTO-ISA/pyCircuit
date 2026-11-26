#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "domains.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "domains.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @Storage() -> (!b8)
      attributes {source_owner = {package = "", path = "cell.py"}, input_names = [], output_names = ["value"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @total init %zero : !b8
    gfir.method @add(%step: !b8) -> () {
      %old = gfir.read @total : !b8
      %next = "ac.bits.binary"(%old, %step) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @total, %next : !b8
      gfir.yield
    }
    %old = gfir.read @total : !b8
    gfir.yield %old : !b8
  }
  gfir.module @Cell(%allow: !b1, %delta: !b8) -> (!b8)
      attributes {source_owner = {package = "", path = "cell.py"}, input_names = ["allow", "delta"], output_names = ["value"], domains = ["default"]} {
    gfir.instance @storage of @Storage
    %old = gfir.call @storage() : () -> !b8
    gfir.rule @advance(%enabled = %allow : !b1, %step = %delta : !b8) -> () {
      gfir.guard %enabled : !b1
      gfir.call @storage["add"](%step) : (!b8) -> ()
      gfir.yield
    }
    gfir.yield %old : !b8
  }
  gfir.module @Top(%clk_e: !b1, %rst_e: !b1, %clk_w: !b1, %rst_w: !b1, %allow_e: !b1, %allow_w: !b1) -> (!b8, !b8)
      attributes {source_owner = {package = "", path = "domains.py"}, input_names = ["clk_e", "rst_e", "clk_w", "rst_w", "allow_e", "allow_w"], output_names = ["east_value", "west_value"], domains = ["east", "west"]} {
    %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
    %ten = "ac.bits.constant4"() {pattern = "00001010"} : () -> !b8
    gfir.instance @east of @Cell {domains = {default = "east"}}
    gfir.instance @west of @Cell {domains = {default = "west"}}
    gfir.instance @idle of @Storage {domains = {default = "west"}}
    %e = gfir.call @east(%allow_e, %one) : (!b1, !b8) -> !b8
    %w = gfir.call @west(%allow_w, %ten) : (!b1, !b8) -> !b8
    %held = gfir.call @idle() : () -> !b8
    %west_value = "ac.bits.binary"(%w, %held) {opcode = "add"} : (!b8, !b8) -> !b8
    gfir.yield %e, %west_value : !b8, !b8
  }
  gfir.root @Top kind "module" {domains = [
    {name = "east", edge = "rising", clock = {input = 0 : i64}, reset = {input = 1 : i64}},
    {name = "west", edge = "rising", clock = {input = 2 : i64}, reset = {input = 3 : i64}}
  ]}
}
