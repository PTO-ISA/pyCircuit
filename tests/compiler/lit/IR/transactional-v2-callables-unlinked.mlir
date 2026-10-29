// RUN: %pycircuit_opt %s -o %t.once
// RUN: %pycircuit_opt %t.once -o %t.twice
// RUN: diff %t.once %t.twice
// RUN: %FileCheck %s < %t.twice
// RUN: %pycircuit_opt --cse %s | %FileCheck %s
// Unresolved reads may consume Queue entries; even unused reads survive CSE.
#w8 = #ac.static_expr<{kind = "literal", location = {path = "callables.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
!p8 = !ac.primitive<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "unlinked"} {
  ac.rule @copy ins(!p8) outs(!p8) captures() body {
  ^bb0(%src: !p8, %dst: !p8):
    %unused = ac.method.call %src::@read() {rule_parameter = 0 : i64} : (!p8) -> (!b8)
    %value = ac.method.call %src::@read() : (!p8) -> (!b8)
    ac.payload.use %value : !b8
    ac.method.call %dst::@write(%value) : (!p8, !b8) -> ()
    "ac.yield"() : () -> ()
  }
  "ac.module"() ({
  ^bb0(%src: !p8, %dst: !p8):
    ac.rule.call @copy ins(%src) outs(%dst) : (!p8, !p8)
    "ac.yield"(%dst) : (!p8) -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "callables.py"}, parameters = [], type_parameters = [], function_type = (!p8) -> (!p8), input_names = ["input"], output_names = ["output"], output_kinds = ["bound"]} : () -> ()
}
// CHECK: ac.rule @copy
// CHECK: ac.method.call {{.*}}::@read() {rule_parameter = 0 : i64}
// CHECK: ac.method.call {{.*}}::@read()
// CHECK: ac.payload.use
// CHECK: ac.method.call {{.*}}::@write(
// CHECK: ac.rule.call @copy
