// RUN: %pycircuit_opt %s --split-input-file --ac-verify-hardware --verify-diagnostics
// Each native/common-IR rejection must reach its owning guard; no source or
// runtime behavior is claimed. Diagnostic text remains provisional until B.
// Nominal struct field cannot receive another equal-width Enum.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  ac.struct "types.Tag" fields [{name = "tag", type = !a}]
  "ac.module"() ({
  ^bb0(%value: !b):
    // expected-error @+1 {{struct.create field type mismatch}}
    %record = "ac.struct.create"(%value) : (!b) -> !ac.struct<"types.Tag">
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b) -> (), input_names = ["value"], output_names = []} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Generic binding preserves nominal identity.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  "ac.module"() ({
  ^bb0(%value: !ac.type_param<@forward, "T">):
    %copy = "ac.rule"(%value) ({
    ^bb0(%capture: !ac.type_param<@forward, "T">):
      "ac.yield"(%capture) : (!ac.type_param<@forward, "T">) -> ()
    }) {name = "forward", occurrence = {site = {definition = @forward, ast_path = []}, expansion = []}} : (!ac.type_param<@forward, "T">) -> !ac.type_param<@forward, "T">
    "ac.yield"(%copy) : (!ac.type_param<@forward, "T">) -> ()
  }) {sym_name = "forward", source_owner = {package = "", path = "forward.py"}, parameters = [], type_parameters = ["T"], function_type = (!ac.type_param<@forward, "T">) -> (!ac.type_param<@forward, "T">), input_names = ["value"], output_names = ["copy"]} : () -> ()
  "ac.module"() ({
  ^bb0(%value: !b):
    // expected-error @+1 {{instance wire type does not match bound callee signature}}
    %bad = "ac.instance"(%value) {instance_name = "bad", callee = @forward, parameters = [], type_arguments = [!a], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b) -> !a
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b) -> (), input_names = ["value"], output_names = []} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Table scalar argument identity.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  "ac.module"() ({
  ^bb0(%value: !pair):
    // expected-error @+1 {{requires isolated single block with matching scalar arguments and ac.yield}}
    %bad = "ac.table.map"(%value) ({
    ^bb0(%ordinal: !b1, %entry: !b):
      "ac.yield"(%entry) : (!b) -> ()
    }) {shape = [#w2], operandSegmentSizes = array<i32: 1, 0>} : (!pair) -> !pair
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!pair) -> (), input_names = ["value"], output_names = []} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Table scalar result identity.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  "ac.module"() ({
  ^bb0(%value: !pair):
    // expected-error @+1 {{scalar region yield types do not match results}}
    %bad = "ac.table.map"(%value) ({
    ^bb0(%ordinal: !b1, %entry: !a):
      %other = "ac.enum.create"() {member = "ONE"} : () -> !b
      "ac.yield"(%other) : (!b) -> ()
    }) {shape = [#w2], operandSegmentSizes = array<i32: 1, 0>} : (!pair) -> !pair
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!pair) -> (), input_names = ["value"], output_names = []} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Enum table values do not become unsigned arithmetic.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  "ac.module"() ({
  ^bb0(%value: !pair):
    // expected-error @+1 {{table.fold requires unsigned bits elements}}
    %bad = "ac.table.fold"(%value) {kind = "add"} : (!pair) -> !b65
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!pair) -> (), input_names = ["value"], output_names = []} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Closed width obligation rejects a different actual width.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
#width = #ac.static_expr<{kind = "reference", ref = {kind = "parameter", owner = @convert, name = "W"}, origin = {site = {definition = @convert, ast_path = []}, expansion = []}, location = {path = "convert.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!bound = !ac.bits<#width>
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  "ac.module"() ({
  ^bb0(%raw: !bound):
    // expected-error @+1 {{enum.from_bits input width must equal 65}}
    %carrier, %member = "ac.enum.from_bits"(%raw) : (!bound) -> (!a, !b1)
    %roundtrip = "ac.enum.to_bits"(%carrier) : (!a) -> !bound
    "ac.yield"(%carrier, %roundtrip, %member) : (!a, !bound, !b1) -> ()
  }) {sym_name = "convert", source_owner = {package = "", path = "convert.py"}, parameters = [{name = "W", type = !ac.math_int}], type_parameters = [], function_type = (!bound) -> (!a, !bound, !b1), input_names = ["raw"], output_names = ["carrier", "roundtrip", "member"]} : () -> ()
  "ac.module"() ({
  ^bb0(%raw: !b130):
    %carrier, %roundtrip, %member = "ac.instance"(%raw) {instance_name = "convert", callee = @convert, parameters = [#w130], type_arguments = [], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b130) -> (!a, !b130, !b1)
    "ac.yield"(%carrier, %roundtrip, %member) : (!a, !b130, !b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b130) -> (!a, !b130, !b1), input_names = ["raw"], output_names = ["carrier", "roundtrip", "member"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Unused conversion cycle remains invalid.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  // expected-error @+1 {{combinational cycle in hardware field dependencies}}
  "ac.module"() ({
  ^bb0(%value: !a):
    %bits = "ac.enum.to_bits"(%cycle) : (!a) -> !b65
    %cycle, %member = "ac.enum.from_bits"(%bits) : (!b65) -> (!a, !b1)
    "ac.yield"(%value) : (!a) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> (!a), input_names = ["value"], output_names = ["original"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Observed conversion cycle remains invalid.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  // expected-error @+1 {{combinational cycle in hardware field dependencies}}
  "ac.module"() ({
  ^bb0():
    %bits = "ac.enum.to_bits"(%cycle) : (!a) -> !b65
    %cycle, %member = "ac.enum.from_bits"(%bits) : (!b65) -> (!a, !b1)
    "ac.yield"(%cycle) : (!a) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = () -> (!a), input_names = [], output_names = ["cycle"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Table packed width retains overflow guard.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
#huge_extent = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<9223372036854775807>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!huge = !ac.table<[#huge_extent], !wide>
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  // expected-error @+1 {{table packed width overflow}}
  "ac.module"() ({
  ^bb0(%value: !huge):
    "ac.yield"(%value) : (!huge) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!huge) -> (!huge), input_names = ["value"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Single-Work memory feedback through conversions retains owning guard.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
#address = #ac.static_expr<{kind = "reference", ref = {kind = "parameter", owner = @memory, name = "ADDR_WIDTH"}, origin = {site = {definition = @memory, ast_path = []}, expansion = []}, location = {path = "byte_mem.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!address = !ac.bits<#address>
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  "ac.module.import"() {sym_name = "memory", source_owner = {package = "gfsim", path = "byte_mem.py"}, parameters = [{name = "ADDR_WIDTH", type = !ac.math_int}, {name = "DEPTH", type = !ac.math_int}], type_parameters = ["T"], function_type = (!b1, !b1, !address, !b1, !address, !ac.type_param<@memory, "T">, !b1) -> !ac.type_param<@memory, "T">, input_names = ["clk", "rst", "raddr", "wvalid", "waddr", "wdata", "wstrb"], output_names = ["rdata"], primitive_kind = "byte_mem", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = [{port = 2 : i64, path = []}]}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %address: !b1, %write: !b1):
    // expected-error @+1 {{byte_mem feedback requires multiple Work samples}}
    %q = "ac.instance"(%clk, %rst, %address, %write, %address, %feedback, %write) {instance_name = "memory", callee = @memory, parameters = [#w1, #w2], type_arguments = [!byte], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !b1, !b1, !byte, !b1) -> !byte
    %raw = "ac.enum.to_bits"(%q) : (!byte) -> !b8
    %feedback, %member = "ac.enum.from_bits"(%raw) : (!b8) -> (!byte, !b1)
    "ac.yield"(%q) : (!byte) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b1) -> (!byte), input_names = ["clk", "rst", "address", "write"], output_names = ["q"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Closed to-bits result width is independently checked.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
#width = #ac.static_expr<{kind = "reference", ref = {kind = "parameter", owner = @pack, name = "W"}, origin = {site = {definition = @pack, ast_path = []}, expansion = []}, location = {path = "pack.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!bound = !ac.bits<#width>
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  "ac.module"() ({
  ^bb0(%state: !a):
    // expected-error @+1 {{enum.to_bits result width must equal 65}}
    %raw = "ac.enum.to_bits"(%state) : (!a) -> !bound
    "ac.yield"(%raw) : (!bound) -> ()
  }) {sym_name = "pack", source_owner = {package = "", path = "pack.py"}, parameters = [{name = "W", type = !ac.math_int}], type_parameters = [], function_type = (!a) -> !bound, input_names = ["state"], output_names = ["raw"]} : () -> ()
  "ac.module"() ({
  ^bb0(%state: !a):
    %raw = "ac.instance"(%state) {instance_name = "pack", callee = @pack, parameters = [#w130], type_arguments = [], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!a) -> !b130
    "ac.yield"(%raw) : (!b130) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> !b130, input_names = ["state"], output_names = ["raw"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Unknown Enum ports must resolve even in a pure transport module.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w6 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<6>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w65 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<65>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w130 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<130>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b6 = !ac.bits<#w6>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b130 = !ac.bits<#w130>
!a = !ac.enum<"alpha.State">
!b = !ac.enum<"beta.State">
!wide = !ac.enum<"types.Wide">
!byte = !ac.enum<"types.Byte">
!pair = !ac.table<[#w2], !a>
!lanes = !ac.table<[#w2, #w3], !a>
!controls = !ac.table<[#w2], !b1>
!inner = !ac.struct<"types.Inner">
!outer = !ac.struct<"types.Outer">
module {
  "ac.enum"() {sym_name = "alpha.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "beta.State", width = #ac.math_int<65>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Wide", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  "ac.enum"() {sym_name = "types.Byte", width = #ac.math_int<8>, encoding = "explicit", members = [{name = "ZERO", code = #ac.math_int<0>}, {name = "ONE", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Inner" fields [{name = "big", type = !wide}, {name = "flag", type = !b1}]
  ac.struct "types.Outer" fields [{name = "tag", type = !a}, {name = "inner", type = !inner}, {name = "tail", type = !b3}]
  // expected-error @+1 {{cannot resolve nominal enum declaration}}
  "ac.module"() ({
  ^bb0(%state: !ac.enum<"missing.State">):
    "ac.yield"(%state) : (!ac.enum<"missing.State">) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!ac.enum<"missing.State">) -> !ac.enum<"missing.State">, input_names = ["state"], output_names = ["same"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
