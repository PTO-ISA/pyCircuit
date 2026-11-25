#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @OwnCredit, ast_path = []}, expansion = []}, location = {path = "own_credit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w16 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<16>}, origin = {site = {definition = @OwnCredit, ast_path = []}, expansion = []}, location = {path = "own_credit.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!word = !ac.bits<#w16>
module attributes {gfir.contract = 1 : i64} {
  gfir.primitive @WordFifo native "fifo" version 1 {element = !word, depth = 1 : i64, latency = 1 : i64, ready_policy = "downstream_pop"}
  gfir.module @OwnCredit(%take: !b1, %data: !word, %late: !b1) -> (!b1, !b1, !word)
      attributes {source_owner = {package = "", path = "own_credit.py"}, input_names = ["take", "data", "late"], output_names = ["ready", "present", "head"], domains = ["default"]} {
    gfir.instance @queue of @WordFifo
    %ready, %present, %head = gfir.call @queue["raw_status"]() : () -> (!b1, !b1, !word)
    gfir.rule @replace(%pop = %take : !b1, %payload = %data : !word, %finish = %late : !b1) -> () {
      gfir.if %pop : !b1 -> () {
        %old = gfir.call @queue["out"]() : () -> !word
        gfir.yield
      } else {
        gfir.yield
      }
      gfir.call @queue["in"](%payload) : (!word) -> ()
      gfir.guard %finish : !b1
      gfir.yield
    }
    gfir.yield %ready, %present, %head : !b1, !b1, !word
  }
  gfir.root @OwnCredit kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
