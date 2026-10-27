// RUN: %pycircuit_opt %s -o %t.once
// RUN: %pycircuit_opt %t.once -o %t.twice
// RUN: diff %t.once %t.twice
// RUN: %FileCheck %s < %t.twice
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
!s = !ac.state<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.enum"() {sym_name = "Mode", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}, {name = "ON", code = #ac.math_int<1>}]} : () -> ()
  "ac.struct"() {sym_name = "Packet", fields = [{name = "mode", type = !ac.enum<"Mode">}, {name = "data", type = !b8}]} : () -> ()
  "ac.module"() ({
    %mode = "ac.enum.create"() {member = "ON"} : () -> !ac.enum<"Mode">
    %data = "ac.bits.constant4"() {pattern = "01xz1100"} : () -> !b8
    %packet = "ac.struct.create"(%mode, %data) : (!ac.enum<"Mode">, !b8) -> !ac.struct<"Packet">
    %enum = "ac.resource.alloc"(%mode) {instance_name = "enum"} : (!ac.enum<"Mode">) -> !ac.state<!ac.enum<"Mode">>
    %packet_state = "ac.resource.alloc"(%packet) {instance_name = "packet_state"} : (!ac.struct<"Packet">) -> !ac.state<!ac.struct<"Packet">>
    %unspecified = "ac.resource.alloc"() {instance_name = "unspecified"} : () -> !s
    "ac.yield"(%enum, %packet_state, %unspecified) : (!ac.state<!ac.enum<"Mode">>, !ac.state<!ac.struct<"Packet">>, !s) -> ()
  }) {sym_name = "Payloads", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!ac.state<!ac.enum<"Mode">>, !ac.state<!ac.struct<"Packet">>, !s), input_names = [], output_names = ["out0", "out1", "out2"], output_kinds = ["export", "export", "export"]} : () -> ()
}
// CHECK: pattern = "01xz1100"
// CHECK: !ac.state<!ac.enum<"Mode">>
// CHECK: !ac.state<!ac.struct<"Packet">>
// CHECK: ac.resource.alloc "unspecified" :
