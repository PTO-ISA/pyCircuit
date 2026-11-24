#width = #ac.static_expr<{
  kind = "literal", value = {kind = "integer", value = #ac.math_int<8>},
  origin = {site = {definition = @Candidates, ast_path = []}, expansion = []},
  location = {path = "candidates.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!byte = !ac.bits<#width>

module attributes {gfir.contract = 1 : i64} {
  gfir.module @Candidates() -> (!byte, !byte)
      attributes {source_owner = {package = "", path = "candidates.py"},
                  input_names = [], output_names = ["total", "snapshot"],
                  domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !byte
    gfir.var @accumulator init %zero : !byte
    gfir.var @remembered init %zero : !byte

    gfir.method @bump() -> () {
      %old = gfir.read @accumulator : !byte
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !byte
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!byte, !byte) -> !byte
      gfir.write @accumulator, %next : !byte
      gfir.yield
    }

    gfir.rule @advance() -> () {
      gfir.call self["bump"]() : () -> ()
      gfir.call self["bump"]() : () -> ()
      gfir.yield
    }

    gfir.rule @remember() -> () {
      %sample = gfir.read @accumulator : !byte
      gfir.write @remembered, %sample : !byte
      gfir.yield
    }

    %total = gfir.read @accumulator : !byte
    %snapshot = gfir.read @remembered : !byte
    gfir.yield %total, %snapshot : !byte, !byte
  }

  gfir.root @Candidates kind "module" {
    domains = [{name = "default", edge = "rising",
                clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]
  }
}
