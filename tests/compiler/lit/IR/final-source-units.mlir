// RUN: %pycircuit_opt %s --split-input-file --ac-verify-hardware | %FileCheck %s
// Native structural provenance is distinct from public source-unit authority.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
module {
  "ac.enum"() {sym_name = "native.State", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "native.Record" fields [{name = "value", type = !b1}]
  "ac.module"() ({
  ^bb0(%value: !b1):
    "ac.yield"(%value) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Explicit inventory retains an empty facade and owned nominal declarations.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
module attributes {ac.source_units = [{package = "", path = "top.py"}, {package = "demo", path = "facade.py"}, {package = "demo", path = "types.py"}]} {
  "ac.enum"() {sym_name = "native.State", width = #ac.math_int<1>, ac.source_owner = {package = "demo", path = "types.py"}, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "native.Record" fields [{name = "value", type = !b1}] {ac.source_owner = {package = "demo", path = "types.py"}}
  "ac.module"() ({
  ^bb0(%value: !b1):
    "ac.yield"(%value) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
// CHECK: sym_name = "native.State"
// CHECK: sym_name = "top"
// CHECK: ac.source_units
// CHECK-SAME: path = "facade.py"
// CHECK-SAME: path = "types.py"
// CHECK: ac.source_owner = {package = "demo", path = "types.py"}
// CHECK: sym_name = "top"
