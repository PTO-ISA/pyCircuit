// Handwritten equivalent of examples/digital_filter/digital_filter.py.
// The unchanged example drivers own stimulus and independent expectations.
#w1 = #ac.static_expr<{
  kind = "literal", value = {kind = "integer", value = #ac.math_int<1>},
  origin = {site = {definition = @example_digital_filter.digital_filter.DigitalFilter, ast_path = []}, expansion = []},
  location = {path = "digital_filter.py", line = 14 : i64, column = 1 : i64, end_line = 14 : i64, end_column = 2 : i64}}>
#w15 = #ac.static_expr<{
  kind = "literal", value = {kind = "integer", value = #ac.math_int<15>},
  origin = {site = {definition = @example_digital_filter.digital_filter.DigitalFilter, ast_path = []}, expansion = []},
  location = {path = "digital_filter.py", line = 14 : i64, column = 1 : i64, end_line = 14 : i64, end_column = 2 : i64}}>
#w16 = #ac.static_expr<{
  kind = "literal", value = {kind = "integer", value = #ac.math_int<16>},
  origin = {site = {definition = @example_digital_filter.digital_filter.DigitalFilter, ast_path = []}, expansion = []},
  location = {path = "digital_filter.py", line = 14 : i64, column = 1 : i64, end_line = 14 : i64, end_column = 2 : i64}}>
#w34 = #ac.static_expr<{
  kind = "literal", value = {kind = "integer", value = #ac.math_int<34>},
  origin = {site = {definition = @example_digital_filter.digital_filter.DigitalFilter, ast_path = []}, expansion = []},
  location = {path = "digital_filter.py", line = 14 : i64, column = 1 : i64, end_line = 14 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b16 = !ac.bits<#w16>
!b34 = !ac.bits<#w34>
!R = !ac.struct<"example_digital_filter.digital_filter.FilterResult">

module attributes {gfir.contract = 1 : i64} {
  ac.struct "example_digital_filter.digital_filter.FilterResult" fields [{name = "y_out", type = !b34}, {name = "y_valid", type = !b1}]
  gfir.module @example_digital_filter.digital_filter.DigitalFilter(%x_in: !b16, %x_valid: !b1) -> (!R)
      attributes {source_owner = {package = "example_digital_filter", path = "digital_filter.py"},
                  input_names = ["x_in", "x_valid"], output_names = ["result"],
                  domains = ["default"]} {
    %zero1 = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
    %zero16 = "ac.bits.constant4"() {pattern = "0000000000000000"} : () -> !b16
    %zero34 = "ac.bits.constant4"() {pattern = "0000000000000000000000000000000000"} : () -> !b34
    gfir.var @delay1 init %zero16 : !b16
    gfir.var @delay2 init %zero16 : !b16
    gfir.var @delay3 init %zero16 : !b16
    gfir.var @y_out init %zero34 : !b34
    gfir.var @y_valid init %zero1 : !b1

    gfir.rule @filter_sample(%input = %x_in : !b16, %valid = %x_valid : !b1) -> (!R) {
      %old_output = gfir.read @y_out : !b34
      %old_valid = gfir.read @y_valid : !b1
      %previous = ac.struct.create(%old_output, %old_valid) : (!b34, !b1) -> !R
      %delay1 = gfir.read @delay1 : !b16
      %delay2 = gfir.read @delay2 : !b16
      %delay3 = gfir.read @delay3 : !b16
      // Preserve the source's zero extension, sign-bit merge and arithmetic.
      // In particular, multiplication by one still propagates unknown input.
      %zero = "ac.bits.constant4"() {pattern = "0000000000000000000000000000000000"} : () -> !b34
      %sign_mask = "ac.bits.constant4"() {pattern = "1111111111111111110000000000000000"} : () -> !b34
      %wide0 = "ac.bits.resize"(%input) {mode = "zext"} : (!b16) -> !b34
      %sign0 = "ac.bits.extract"(%input) {low = #w15} : (!b16) -> !b1
      %extension0 = "ac.bits.select"(%sign0, %sign_mask, %zero) : (!b1, !b34, !b34) -> !b34
      %signed0 = "ac.bits.binary"(%wide0, %extension0) {opcode = "or"} : (!b34, !b34) -> !b34
      %weight0 = "ac.bits.constant4"() {pattern = "0000000000000000000000000000000001"} : () -> !b34
      %tap0 = "ac.bits.binary"(%signed0, %weight0) {opcode = "mul"} : (!b34, !b34) -> !b34
      %wide1 = "ac.bits.resize"(%delay1) {mode = "zext"} : (!b16) -> !b34
      %sign1 = "ac.bits.extract"(%delay1) {low = #w15} : (!b16) -> !b1
      %extension1 = "ac.bits.select"(%sign1, %sign_mask, %zero) : (!b1, !b34, !b34) -> !b34
      %signed1 = "ac.bits.binary"(%wide1, %extension1) {opcode = "or"} : (!b34, !b34) -> !b34
      %weight1 = "ac.bits.constant4"() {pattern = "0000000000000000000000000000000010"} : () -> !b34
      %tap1 = "ac.bits.binary"(%signed1, %weight1) {opcode = "mul"} : (!b34, !b34) -> !b34
      %wide2 = "ac.bits.resize"(%delay2) {mode = "zext"} : (!b16) -> !b34
      %sign2 = "ac.bits.extract"(%delay2) {low = #w15} : (!b16) -> !b1
      %extension2 = "ac.bits.select"(%sign2, %sign_mask, %zero) : (!b1, !b34, !b34) -> !b34
      %signed2 = "ac.bits.binary"(%wide2, %extension2) {opcode = "or"} : (!b34, !b34) -> !b34
      %weight2 = "ac.bits.constant4"() {pattern = "0000000000000000000000000000000011"} : () -> !b34
      %tap2 = "ac.bits.binary"(%signed2, %weight2) {opcode = "mul"} : (!b34, !b34) -> !b34
      %wide3 = "ac.bits.resize"(%delay3) {mode = "zext"} : (!b16) -> !b34
      %sign3 = "ac.bits.extract"(%delay3) {low = #w15} : (!b16) -> !b1
      %extension3 = "ac.bits.select"(%sign3, %sign_mask, %zero) : (!b1, !b34, !b34) -> !b34
      %signed3 = "ac.bits.binary"(%wide3, %extension3) {opcode = "or"} : (!b34, !b34) -> !b34
      %weight3 = "ac.bits.constant4"() {pattern = "0000000000000000000000000000000100"} : () -> !b34
      %tap3 = "ac.bits.binary"(%signed3, %weight3) {opcode = "mul"} : (!b34, !b34) -> !b34
      %sum01 = "ac.bits.binary"(%tap0, %tap1) {opcode = "add"} : (!b34, !b34) -> !b34
      %sum012 = "ac.bits.binary"(%sum01, %tap2) {opcode = "add"} : (!b34, !b34) -> !b34
      %total = "ac.bits.binary"(%sum012, %tap3) {opcode = "add"} : (!b34, !b34) -> !b34
      // Data selection is a four-state bit merge, not a transaction guard.
      %next3 = "ac.bits.select"(%valid, %delay2, %delay3) : (!b1, !b16, !b16) -> !b16
      gfir.write @delay3, %next3 : !b16
      %next2 = "ac.bits.select"(%valid, %delay1, %delay2) : (!b1, !b16, !b16) -> !b16
      gfir.write @delay2, %next2 : !b16
      %next1 = "ac.bits.select"(%valid, %input, %delay1) : (!b1, !b16, !b16) -> !b16
      gfir.write @delay1, %next1 : !b16
      %next_output = "ac.bits.select"(%valid, %total, %old_output) : (!b1, !b34, !b34) -> !b34
      gfir.write @y_out, %next_output : !b34
      gfir.write @y_valid, %valid : !b1
      gfir.yield %previous : !R
    }
    %result = gfir.result @filter_sample[0] : !R
    gfir.yield %result : !R
  }
  gfir.root @example_digital_filter.digital_filter.DigitalFilter kind "module" {
    domains = [{name = "default", edge = "rising",
                clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}],
    execution_policy = {reset = "strict"}
  }
}
