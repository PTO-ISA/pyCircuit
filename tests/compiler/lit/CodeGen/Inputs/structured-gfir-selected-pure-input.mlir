#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "selected_input.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "selected_input.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w16 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<16>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "selected_input.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b16 = !ac.bits<#w16>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @Sink(%value: !b8, %run: !b1) -> (!b8, !b8)
      attributes {source_owner = {package = "", path = "selected_input.py"}, input_names = ["value", "run"], output_names = ["result", "old"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @state init %zero : !b8
    gfir.rule @consume(%v = %value : !b8, %r = %run : !b1) -> (!b8) {
      // Availability of value is required only after this reached guard.
      gfir.guard %r : !b1
      %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
      %packed = "ac.bits.concat"(%v, %zero) : (!b8, !b8) -> !b16
      %next = "ac.bits.extract"(%packed) {low = #w8} : (!b16) -> !b8
      gfir.write @state, %next : !b8
      %candidate = gfir.read @state : !b8
      gfir.yield %candidate : !b8
    }
    %result = gfir.result @consume[0] : !b8
    %old = gfir.read @state : !b8
    gfir.yield %result, %old : !b8, !b8
  }
  gfir.module @Relay(%value: !b8, %run: !b1) -> (!b8, !b8)
      attributes {source_owner = {package = "", path = "selected_input.py"}, input_names = ["value", "run"], output_names = ["result", "old"], domains = ["default"]} {
    gfir.instance @first of @Sink
    gfir.instance @second of @Sink
    %first, %unused_old = gfir.call @first(%value, %run) : (!b8, !b1) -> (!b8, !b8)
    %result, %old = gfir.call @second(%first, %run) : (!b8, !b1) -> (!b8, !b8)
    gfir.yield %result, %old : !b8, !b8
  }
  gfir.module @Identity(%value: !b8) -> (!b8)
      attributes {source_owner = {package = "", path = "selected_input.py"}, input_names = ["value"], output_names = ["result"]} {
    gfir.yield %value : !b8
  }
  gfir.module @Fallback(%value: !b8) -> (!b8)
      attributes {source_owner = {package = "", path = "selected_input.py"}, input_names = ["value"], output_names = ["result"]} {
    gfir.yield %value : !b8
  }
  gfir.module @Pick(%selector: !b1, %yes: !b8, %no: !b8) -> (!b8)
      attributes {source_owner = {package = "", path = "selected_input.py"}, input_names = ["selector", "yes", "no"], output_names = ["result"]} {
    %selected = "ac.bits.select"(%selector, %yes, %no) : (!b1, !b8, !b8) -> !b8
    gfir.instance @identity of @Identity
    %result = gfir.call @identity(%selected) : (!b8) -> !b8
    gfir.yield %result : !b8
  }
  gfir.module @Top(%offer: !b1, %data: !b8, %select: !b1, %fallback: !b8, %run: !b1) -> (!b8, !b8, !b8)
      attributes {source_owner = {package = "", path = "selected_input.py"}, input_names = ["offer", "data", "select", "fallback", "run"], output_names = ["result", "old", "producer_old"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @issued init %zero : !b8
    gfir.rule @produce(%enabled = %offer : !b1, %value = %data : !b8) -> (!b8) {
      gfir.guard %enabled : !b1
      gfir.write @issued, %value : !b8
      gfir.yield %value : !b8
    }
    %produced = gfir.result @produce[0] : !b8
    %choose_fallback = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
    gfir.instance @fallback_source of @Fallback
    %total_input = gfir.call @fallback_source(%fallback) : (!b8) -> !b8
    gfir.instance @total of @Pick
    %total = gfir.call @total(%choose_fallback, %zero, %total_input) : (!b1, !b8, !b8) -> !b8
    gfir.instance @pick of @Pick
    %chosen = gfir.call @pick(%select, %produced, %total) : (!b1, !b8, !b8) -> !b8
    gfir.instance @relay of @Relay
    %result, %old = gfir.call @relay(%chosen, %run) : (!b8, !b1) -> (!b8, !b8)
    %producer_old = gfir.read @issued : !b8
    gfir.yield %result, %old, %producer_old : !b8, !b8, !b8
  }
  gfir.root @Top kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
