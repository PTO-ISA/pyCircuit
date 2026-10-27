// RUN: %pycircuit_opt %s -o %t.once
// RUN: %pycircuit_opt %t.once -o %t.twice
// RUN: diff %t.once %t.twice
// RUN: %FileCheck %s < %t.twice
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
module attributes {ac.transactional = 2 : i64, ac.stage = "unlinked"} {
  "ac.module.import"() {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!ac.primitive<!b8>) -> (!ac.primitive<!b8>), input_names = ["input"], output_names = ["output"], output_kinds = ["export"], semantic_kind = "resources"} : () -> ()
  "ac.module"() ({
  ^bb0(%input: !ac.primitive<!b8>):
    %provider = "ac.instance"(%input) {instance_name = "provider", callee = @Provider, parameters = [], type_arguments = [], occurrence = {}} : (!ac.primitive<!b8>) -> (!ac.primitive<!b8>)
    "ac.yield"(%provider) : (!ac.primitive<!b8>) -> ()
  }) {sym_name = "Consumer", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = (!ac.primitive<!b8>) -> (!ac.primitive<!b8>), input_names = ["in0"], output_names = ["out0"], output_kinds = ["export"]} : () -> ()
}
// CHECK: !ac.primitive<
// CHECK: semantic_kind = "resources"
