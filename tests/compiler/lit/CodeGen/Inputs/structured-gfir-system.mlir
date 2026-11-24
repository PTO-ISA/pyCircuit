#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Scenario, ast_path = []}, expansion = []}, location = {path = "scenario.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b8 = !ac.bits<#w8>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Unused, ast_path = []}, expansion = []}, location = {path = "unused.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
module attributes {gfir.contract = 1 : i64} {
  // The C++ bundle also compiles uninstantiated definitions and their checks.
  gfir.module @Unused() -> ()
      attributes {source_owner = {package = "", path = "unused.py"},
                  input_names = [], output_names = [], domains = ["default"]} {
    gfir.rule @check() -> () {
      %permit = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
      gfir.guard %permit : !b1
      gfir.yield
    }
    gfir.yield
  }
  gfir.module @Counter() -> ()
      attributes {source_owner = {package = "", path = "counter.py"},
                  input_names = [], output_names = [], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @count init %zero : !b8
    gfir.rule @advance() -> () {
      %old = gfir.read @count : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @count, %next : !b8
      gfir.yield
    }
    gfir.yield
  }
  gfir.module @Scenario() -> ()
      attributes {source_owner = {package = "", path = "scenario.py"},
                  input_names = [], output_names = [], domains = ["default"]} {
    gfir.instance @counter of @Counter
    gfir.call @counter() : () -> ()
    gfir.yield
  }
  gfir.root @Scenario kind "system" {
    domains = [{name = "default", edge = "rising",
                clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}],
    ac.execution_policy = {reset = "if_one"},
    ac.system_samples = {
      rows = ["00", "10", "1z", "00", "1x", "00", "10"],
      origin = {site = {definition = @Scenario, ast_path = []}, expansion = []},
      location = {path = "scenario.py", line = 1 : i64, column = 1 : i64,
                  end_line = 1 : i64, end_column = 2 : i64}}
  }
}
