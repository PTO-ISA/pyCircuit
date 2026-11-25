#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Response, ast_path = []}, expansion = []}, location = {path = "response.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w16 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<16>}, origin = {site = {definition = @Response, ast_path = []}, expansion = []}, location = {path = "response.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#low = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @Response, ast_path = []}, expansion = []}, location = {path = "response.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!word = !ac.bits<#w16>
module attributes {gfir.contract = 1 : i64} {
  gfir.primitive @WordFifo native "fifo" version 1
      {element = !word, depth = 1 : i64, latency = 1 : i64, ready_policy = "downstream_pop"}
  gfir.module @Response(%left: !word, %right: !word) -> (!word)
      attributes {source_owner = {package = "", path = "response.py"}, input_names = ["left", "right"], output_names = ["result"], domains = ["default"]} {
    gfir.instance @ingress of @WordFifo
    gfir.instance @egress of @WordFifo
    %allow = "ac.bits.extract"(%right) {low = #low} : (!word) -> !b1
    gfir.rule @send(%value = %left : !word) -> () {
      gfir.call @ingress["in"](%value) : (!word) -> ()
      gfir.yield
    }
    gfir.rule @transfer() -> () {
      %value = gfir.call @ingress["out"]() : () -> !word
      gfir.call @egress["in"](%value) : (!word) -> ()
      gfir.yield
    }
    gfir.rule @receive(%late = %allow : !b1) -> (!word) {
      %value = gfir.call @egress["out"]() : () -> !word
      // Reaching out does not return credit until this whole Rule admits.
      gfir.guard %late : !b1
      gfir.yield %value : !word
    }
    %result = gfir.result @receive[0] : !word
    gfir.yield %result : !word
  }
  gfir.root @Response kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
