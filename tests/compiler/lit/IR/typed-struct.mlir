// RUN: %pycircuit_opt %s --ac-verify-hardware | %FileCheck %s
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#w1>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b5 = !ac.bits<#w5>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b70 = !ac.bits<#w70>
module {
  ac.struct "Packet" fields [{name = "tag", type = !b5}, {name = "data", type = !b70}]
  "ac.module"() ({
  ^bb0(%tag: !b5, %data: !b70):
    %whole = ac.struct.create(%tag, %data) : (!b5, !b70) -> !ac.struct<"Packet">
    %field = ac.struct.get %whole["data"] : (!ac.struct<"Packet">) -> !b70
    "ac.yield"(%whole, %field) : (!ac.struct<"Packet">, !b70) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b5, !b70) -> (!ac.struct<"Packet">, !b70), input_names = ["tag", "data"], output_names = ["packet", "projected"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
// CHECK: ac.struct "Packet" fields
// CHECK-SAME: name = "tag", type = !ac.bits
// CHECK-SAME: name = "data", type = !ac.bits
// CHECK: %[[PACK:[a-zA-Z0-9_]+]] = ac.struct.create
// CHECK: ac.struct.get %[[PACK]]["data"]
// CHECK: "ac.yield"
