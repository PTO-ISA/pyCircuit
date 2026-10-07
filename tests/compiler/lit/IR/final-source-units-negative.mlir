// RUN: %pycircuit_opt %s --split-input-file --ac-verify-hardware --verify-diagnostics

// Present inventory wrong type.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
// expected-error @+1 {{ac.source_units must be an ArrayAttr}}
module attributes {ac.source_units = "wrong"} {
  "ac.enum"() {sym_name = "native.State", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "native.Record" fields [{name = "value", type = !b1}]
  "ac.module"() ({
  ^bb0(%value: !b1):
    "ac.yield"(%value) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Inventory entry wrong type.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
// expected-error @+1 {{must be a valid SourceOwner dictionary}}
module attributes {ac.source_units = ["not-owner"]} {
  "ac.enum"() {sym_name = "native.State", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "native.Record" fields [{name = "value", type = !b1}]
  "ac.module"() ({
  ^bb0(%value: !b1):
    "ac.yield"(%value) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Duplicate owner inventory.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
// expected-error @+1 {{contains a duplicate SourceOwner}}
module attributes {ac.source_units = [{package = "", path = "top.py"}, {package = "", path = "top.py"}]} {
  "ac.enum"() {sym_name = "native.State", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "native.Record" fields [{name = "value", type = !b1}]
  "ac.module"() ({
  ^bb0(%value: !b1):
    "ac.yield"(%value) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Noncanonical UTF-8 owner order.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
// expected-error @+1 {{must be sorted by UTF-8}}
module attributes {ac.source_units = [{package = "demo", path = "types.py"}, {package = "", path = "top.py"}]} {
  "ac.enum"() {sym_name = "native.State", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "native.Record" fields [{name = "value", type = !b1}]
  "ac.module"() ({
  ^bb0(%value: !b1):
    "ac.yield"(%value) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Explicit empty inventory cannot drop real owner.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
// expected-error @+1 {{SourceOwner is absent from ac.source_units}}
module attributes {ac.source_units = []} {
  "ac.enum"() {sym_name = "native.State", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "native.Record" fields [{name = "value", type = !b1}]
  "ac.module"() ({
  ^bb0(%value: !b1):
    "ac.yield"(%value) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Actual nominal owner omitted.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
// expected-error @+1 {{SourceOwner is absent from ac.source_units}}
module attributes {ac.source_units = [{package = "", path = "top.py"}]} {
  "ac.enum"() {sym_name = "native.State", width = #ac.math_int<1>, ac.source_owner = {package = "demo", path = "types.py"}, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "native.Record" fields [{name = "value", type = !b1}]
  "ac.module"() ({
  ^bb0(%value: !b1):
    "ac.yield"(%value) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Wrong-typed Enum owner remains present.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
// expected-error @+1 {{requires a valid SourceOwner dictionary in ac.source_owner}}
module {
  "ac.enum"() {sym_name = "native.State", width = #ac.math_int<1>, ac.source_owner = "wrong", encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "native.Record" fields [{name = "value", type = !b1}]
  "ac.module"() ({
  ^bb0(%value: !b1):
    "ac.yield"(%value) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Wrong-typed Struct owner remains present.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
// expected-error @+1 {{requires a valid SourceOwner dictionary in ac.source_owner}}
module {
  "ac.enum"() {sym_name = "native.State", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}]} : () -> ()
  ac.struct "native.Record" fields [{name = "value", type = !b1}] {ac.source_owner = 42 : i64}
  "ac.module"() ({
  ^bb0(%value: !b1):
    "ac.yield"(%value) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !b1, input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
