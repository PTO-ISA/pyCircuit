// RUN: %pycircuit_opt %s --split-input-file --ac-verify-hardware | %FileCheck %s
// Common executable analysis only; no emitted/runtime lifecycle claim.
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
  "ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%value: !ac.type_param<@forward, "T">):
    %copy = "ac.rule"(%value) ({
    ^bb0(%capture: !ac.type_param<@forward, "T">):
      "ac.yield"(%capture) : (!ac.type_param<@forward, "T">) -> ()
    }) {name = "forward", occurrence = {site = {definition = @forward, ast_path = []}, expansion = []}} : (!ac.type_param<@forward, "T">) -> !ac.type_param<@forward, "T">
    "ac.yield"(%copy) : (!ac.type_param<@forward, "T">) -> ()
  }) {sym_name = "forward", source_owner = {package = "", path = "forward.py"}, parameters = [], type_parameters = ["T"], function_type = (!ac.type_param<@forward, "T">) -> (!ac.type_param<@forward, "T">), input_names = ["value"], output_names = ["copy"]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %en: !b1, %data: !a, %candidate: !a, %other: !b, %wide: !wide):
    %init = "ac.enum.create"() {member = "ZERO"} : () -> !a
    %merged, %merge_en = "ac.value.merge"(%data, %en, %candidate) {paths = [[]], operandSegmentSizes = array<i32: 1, 1, 1>} : (!a, !b1, !a) -> (!a, !b1)
    %q = "ac.instance"(%clk, %rst, %merge_en, %merged, %init) {instance_name = "state", callee = @storage, parameters = [], type_arguments = [!a], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 2 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !a, !a) -> !a
    %rule_value = "ac.rule"(%q) ({
    ^bb0(%entry: !a):
      %literal = "ac.enum.create"() {member = "ZERO"} : () -> !a
      %raw = "ac.enum.to_bits"(%entry) : (!a) -> !b65
      %carrier, %member = "ac.enum.from_bits"(%raw) : (!b65) -> (!a, !b1)
      "ac.yield"(%carrier) : (!a) -> ()
    }) {name = "enum_transport", occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!a) -> !a
    %inner = "ac.struct.create"(%wide, %en) : (!wide, !b1) -> !inner
    %zero = "ac.bits.constant"() {value = #w0} : () -> !b3
    %packet = "ac.struct.create"(%rule_value, %inner, %zero) : (!a, !inner, !b3) -> !outer
    %updated_packet, %updated_en = "ac.value.merge"(%packet, %en, %candidate) {paths = [["tag"]], operandSegmentSizes = array<i32: 1, 1, 1>} : (!outer, !b1, !a) -> (!outer, !b1)
    %tag = "ac.struct.get"(%updated_packet) {field = "tag"} : (!outer) -> !a
    %lanes = "ac.table.splat"(%tag) {shape = [#w2, #w3]} : (!a) -> !lanes
    %mapped = "ac.table.map"(%lanes, %candidate) ({
    ^bb0(%ordinal: !b3, %entry: !a, %capture: !a):
      %literal = "ac.enum.create"() {member = "ZERO"} : () -> !a
      %entry_bits = "ac.enum.to_bits"(%entry) : (!a) -> !b65
      %capture_bits = "ac.enum.to_bits"(%capture) : (!a) -> !b65
      %hit = "ac.bits.compare"(%entry_bits, %capture_bits) {predicate = "eq"} : (!b65, !b65) -> !b1
      %carrier, %member = "ac.enum.from_bits"(%entry_bits) : (!b65) -> (!a, !b1)
      %next, %enabled = "ac.value.merge"(%entry, %hit, %carrier) {paths = [[]], operandSegmentSizes = array<i32: 1, 1, 1>} : (!a, !b1, !a) -> (!a, !b1)
      "ac.yield"(%next) : (!a) -> ()
    }) {shape = [#w2, #w3], operandSegmentSizes = array<i32: 1, 1>} : (!lanes, !a) -> !lanes
    %mask = "ac.table.match"(%mapped, %candidate) ({
    ^bb0(%entry: !a, %capture: !a):
      %entry_bits = "ac.enum.to_bits"(%entry) : (!a) -> !b65
      %capture_bits = "ac.enum.to_bits"(%capture) : (!a) -> !b65
      %hit = "ac.bits.compare"(%entry_bits, %capture_bits) {predicate = "eq"} : (!b65, !b65) -> !b1
      "ac.yield"(%hit) : (!b1) -> ()
    }) : (!lanes, !a) -> !b6
    %selected, %in_range = "ac.table.get"(%mapped, %zero) : (!lanes, !b3) -> (!a, !b1)
    %pair = "ac.table.create"(%init, %candidate) : (!a, !a) -> !pair
    %clock = "ac.table.splat"(%clk) {shape = [#w2]} : (!b1) -> !controls
    %reset = "ac.table.splat"(%rst) {shape = [#w2]} : (!b1) -> !controls
    %enable = "ac.table.splat"(%en) {shape = [#w2]} : (!b1) -> !controls
    %array_q = "ac.collection"(%clock, %reset, %enable, %pair, %pair) {instance_name = "array", callee = @storage, parameters = [], type_arguments = [!a], shape = [#w2], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 17 : i64}]}, expansion = []}} : (!controls, !controls, !controls, !pair, !pair) -> !pair
    %other_copy = "ac.instance"(%other) {instance_name = "other", callee = @forward, parameters = [], type_arguments = [!b], occurrence = {site = {definition = @top, ast_path = [{kind = "field", name = "body"}, {kind = "index", value = 18 : i64}]}, expansion = []}} : (!b) -> !b
    "ac.yield"(%q, %updated_packet, %mapped, %mask, %selected, %array_q, %other_copy) : (!a, !outer, !lanes, !b6, !a, !pair, !b) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !a, !a, !b, !wide) -> (!a, !outer, !lanes, !b6, !a, !pair, !b), input_names = ["clk", "rst", "en", "data", "candidate", "other", "wide"], output_names = ["q", "packet", "mapped", "mask", "selected", "array_q", "other_copy"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Open width obligation is checked at a matching closed occurrence.
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
    %carrier, %member = "ac.enum.from_bits"(%raw) : (!bound) -> (!a, !b1)
    %roundtrip = "ac.enum.to_bits"(%carrier) : (!a) -> !bound
    "ac.yield"(%carrier, %roundtrip, %member) : (!a, !bound, !b1) -> ()
  }) {sym_name = "convert", source_owner = {package = "", path = "convert.py"}, parameters = [{name = "W", type = !ac.math_int}], type_parameters = [], function_type = (!bound) -> (!a, !bound, !b1), input_names = ["raw"], output_names = ["carrier", "roundtrip", "member"]} : () -> ()
  "ac.module"() ({
  ^bb0(%raw: !b65):
    %carrier, %roundtrip, %member = "ac.instance"(%raw) {instance_name = "convert", callee = @convert, parameters = [#w65], type_arguments = [], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b65) -> (!a, !b65, !b1)
    "ac.yield"(%carrier, %roundtrip, %member) : (!a, !b65, !b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b65) -> (!a, !b65, !b1), input_names = ["raw"], output_names = ["carrier", "roundtrip", "member"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}


// -----
// To-bits has its own open/closed width obligation.
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
    %raw = "ac.enum.to_bits"(%state) : (!a) -> !bound
    "ac.yield"(%raw) : (!bound) -> ()
  }) {sym_name = "pack", source_owner = {package = "", path = "pack.py"}, parameters = [{name = "W", type = !ac.math_int}], type_parameters = [], function_type = (!a) -> !bound, input_names = ["state"], output_names = ["raw"]} : () -> ()
  "ac.module"() ({
  ^bb0(%state: !a):
    %raw = "ac.instance"(%state) {instance_name = "pack", callee = @pack, parameters = [#w65], type_arguments = [], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!a) -> !b65
    "ac.yield"(%raw) : (!b65) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> !b65, input_names = ["state"], output_names = ["raw"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// CHECK: primitive_kind = "dffe"
// CHECK: "ac.enum.create"
// CHECK: "ac.value.merge"
// CHECK: ac.struct.create
// CHECK: "ac.table.map"
// CHECK: "ac.enum.from_bits"
// CHECK: "ac.table.match"
// CHECK: "ac.table.get"
// CHECK: "ac.collection"
// CHECK: callee = @forward
// CHECK: sym_name = "convert"
// CHECK: callee = @convert
// CHECK: sym_name = "pack"
// CHECK: callee = @pack
