#span = {path = "transactional-state.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Failure, ast_path = []}, expansion = []}
#n1 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<2>}}>
#n3 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<3>}}>
#n5 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<5>}}>
!b1 = !ac.bits<#n1>
!b3 = !ac.bits<#n3>
!s3 = !ac.state<!b3>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %two = "ac.bits.constant"() {value = #n2} : () -> !b3
    %five = "ac.bits.constant"() {value = #n5} : () -> !b3
    %a = ac.resource.alloc "a" initial(%two : !b3) : !s3
    %b = ac.resource.alloc "b" initial(%five : !b3) : !s3
    ac.rule @checked ins() outs() captures(%a : !s3) body {
    ^bb0(%a: !s3):
      %old = ac.method.call %a::@read() : (!s3) -> (!b3)
      %one = "ac.bits.constant"() {value = #n1} : () -> !b3
      %three = "ac.bits.constant"() {value = #n3} : () -> !b3
      %true = "ac.bits.constant"() {value = #n1} : () -> !b1
      %okay = "ac.bits.compare"(%old, %three) {predicate = "ne"} : (!b3, !b3) -> !b1
      "ac.expect"(%okay, %true) {kind = "assert", location = #span} : (!b1, !b1) -> ()
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b3, !b3) -> !b3
      ac.method.call %a::@write(%next) : (!s3, !b3) -> ()
      "ac.yield"() : () -> ()
    }
    ac.rule @independent ins() outs() captures(%b : !s3) body {
    ^bb0(%b: !s3):
      %old = ac.method.call %b::@read() : (!s3) -> (!b3)
      %one = "ac.bits.constant"() {value = #n1} : () -> !b3
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b3, !b3) -> !b3
      ac.method.call %b::@write(%next) : (!s3, !b3) -> ()
      "ac.yield"() : () -> ()
    }
    ac.rule.call @checked ins() outs() : ()
    ac.rule.call @independent ins() outs() : ()
    "ac.yield"(%a, %b) : (!s3, !s3) -> ()
  }) {sym_name = "Failure", source_owner = {package = "", path = "transactional-state.py"}, parameters = [], type_parameters = [], function_type = () -> (!s3, !s3), input_names = [], output_names = ["a", "b"], output_kinds = ["export", "export"]} : () -> ()
}
