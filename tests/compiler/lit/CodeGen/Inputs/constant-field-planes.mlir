#w0 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w65 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
#w130 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
#n0 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#n5 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<680564733841876926926749214863536422917>}}>
#n9 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<36893488147419103241>}}>
#n17 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1267650600228229401496703205393>}}>
#i3 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<18446744073709551619>}}>
#i5 = #ac.static_expr<{kind = "literal", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @ConstantChild, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b8 = !ac.bits<#w8>
!b65 = !ac.bits<#w65>
!b70 = !ac.bits<#w70>
!b130 = !ac.bits<#w130>
!mode = !ac.enum<"ConstantMode">
!row = !ac.struct<"ConstantRow">
!rows = !ac.table<[#w3], !row>
!wide = !ac.table<[#w3], !b130>
!mixed = !ac.table<[#w3], !b8>
!inner = !ac.table<[#w2], !b65>
!two_b1 = !ac.table<[#w2], !b1>
!two_b8 = !ac.table<[#w2], !b8>
!two_b70 = !ac.table<[#w2], !b70>
!two_b130 = !ac.table<[#w2], !b130>
!two_rows = !ac.table<[#w2, #w3], !row>
!two_wide = !ac.table<[#w2, #w3], !b130>
!two_mixed = !ac.table<[#w2, #w3], !b8>
!modes = !ac.table<[#w3], !mode>
!mode_bits = !ac.table<[#w3], !b3>
!two_modes = !ac.table<[#w2, #w3], !mode>
!two_mode_bits = !ac.table<[#w2, #w3], !b3>
#n = #ac.static_expr<{kind = "reference", location = {path = "constant.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @GenericNegative, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @GenericNegative, name = "N"}}>
!generic_result = !ac.table<[#n], !b8>
!generic_inputs = !ac.table<[#w2], !b8>
!generic_outputs = !ac.table<[#w2, #w3], !b8>
module {
  "ac.enum"() {sym_name = "ConstantMode", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<1>}, {name = "RUN", code = #ac.math_int<4>}, {name = "DONE", code = #ac.math_int<7>}]} : () -> ()
  ac.struct "ConstantRow" fields [{name = "flag", type = !b1}, {name = "wide", type = !b130}, {name = "nested", type = !inner}, {name = "dynamic", type = !b8}]
  "ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dffe.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "en", "d", "init"], output_names = ["q"], primitive_kind = "dffe", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %en: !b1, %dynamic: !b8, %index: !b70):
    %zero = "ac.bits.constant"() {value = #n0} : () -> !b1
    %one = "ac.bits.constant"() {value = #n1} : () -> !b1
    %zero8 = "ac.bits.constant"() {value = #n0} : () -> !b8
    %one8 = "ac.bits.constant"() {value = #n1} : () -> !b8
    %q = "ac.instance"(%clk, %rst, %en, %dynamic, %zero8) {instance_name = "state", callee = @storage, parameters = [], type_arguments = [!b8], occurrence = {site = {definition = @ConstantChild, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b8, !b8) -> !b8
    %w0 = "ac.bits.constant"() {value = #n5} : () -> !b130
    %w1 = "ac.bits.constant"() {value = #n9} : () -> !b130
    %w2 = "ac.bits.constant"() {value = #n17} : () -> !b130
    %m0 = "ac.enum.create"() {member = "IDLE"} : () -> !mode
    %m1 = "ac.enum.create"() {member = "RUN"} : () -> !mode
    %m2 = "ac.enum.create"() {member = "DONE"} : () -> !mode
    %mb0 = "ac.enum.to_bits"(%m0) : (!mode) -> !b3
    %mb1 = "ac.enum.to_bits"(%m1) : (!mode) -> !b3
    %mb2 = "ac.enum.to_bits"(%m2) : (!mode) -> !b3
    %modes = "ac.table.create"(%m0, %m1, %m2) : (!mode, !mode, !mode) -> !modes
    %mode_bits = "ac.table.create"(%mb0, %mb1, %mb2) : (!b3, !b3, !b3) -> !mode_bits
    %i0 = "ac.bits.constant"() {value = #i3} : () -> !b65
    %i1 = "ac.bits.constant"() {value = #i5} : () -> !b65
    %inner0 = "ac.table.create"(%i0, %i1) : (!b65, !b65) -> !inner
    %inner1 = "ac.table.create"(%i1, %i0) : (!b65, !b65) -> !inner
    %r0 = ac.struct.create(%zero, %w0, %inner0, %dynamic) : (!b1, !b130, !inner, !b8) -> !row
    %r1 = ac.struct.create(%one, %w1, %inner1, %dynamic) : (!b1, !b130, !inner, !b8) -> !row
    %r2 = ac.struct.create(%zero, %w2, %inner0, %dynamic) : (!b1, !b130, !inner, !b8) -> !row
    %records = "ac.table.create"(%r0, %r1, %r2) : (!row, !row, !row) -> !rows
    %wide_table = "ac.table.create"(%w0, %w1, %w2) : (!b130, !b130, !b130) -> !wide
    %mixed = "ac.table.create"(%zero8, %dynamic, %one8) : (!b8, !b8, !b8) -> !mixed
    %selected, %in_range = "ac.table.get"(%records, %index) : (!rows, !b70) -> (!row, !b1)
    %flag = ac.struct.get %selected["flag"] : (!row) -> !b1
    %selected_wide = ac.struct.get %selected["wide"] : (!row) -> !b130
    %selected_dynamic = ac.struct.get %selected["dynamic"] : (!row) -> !b8
    "ac.yield"(%records, %wide_table, %mixed, %modes, %mode_bits, %flag, %selected_wide, %selected_dynamic, %in_range, %q) : (!rows, !wide, !mixed, !modes, !mode_bits, !b1, !b130, !b8, !b1, !b8) -> ()
  }) {sym_name = "ConstantChild", source_owner = {package = "", path = "constant.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b8, !b70) -> (!rows, !wide, !mixed, !modes, !mode_bits, !b1, !b130, !b8, !b1, !b8), input_names = ["clk", "rst", "en", "dynamic", "index"], output_names = ["records", "wide", "mixed", "modes", "mode_bits", "flag", "selected_wide", "selected_dynamic", "in_range", "q"]} : () -> ()
  "ac.module"() ({
  ^bb0(%x: !b8):
    %c0 = "ac.bits.constant"() {value = #n0} : () -> !b8
    %c1 = "ac.bits.constant"() {value = #n1} : () -> !b8
    %c2 = "ac.bits.constant"() {value = #n2} : () -> !b8
    %created = "ac.table.create"(%c0, %c1, %c2) : (!b8, !b8, !b8) -> !generic_result
    "ac.yield"(%created) : (!generic_result) -> ()
  }) {sym_name = "GenericNegative", source_owner = {package = "", path = "constant.py"}, parameters = [{name = "N", type = !ac.math_int}], type_parameters = [], function_type = (!b8) -> !generic_result, input_names = ["x"], output_names = ["out"]} : () -> ()
  "ac.module"() ({
    %c1 = "ac.bits.constant"() {value = #n1} : () -> !b8
    %c2 = "ac.bits.constant"() {value = #n2} : () -> !b8
    %sum = "ac.bits.binary"(%c1, %c2) {opcode = "add"} : (!b8, !b8) -> !b8
    %computed = "ac.table.create"(%sum, %sum, %sum) : (!b8, !b8, !b8) -> !mixed
    %literal = "ac.table.create"(%c1, %c2, %c1) : (!b8, !b8, !b8) -> !mixed
    %mapped = "ac.table.map"(%literal) ({
    ^bb0(%ordinal: !b2, %entry: !b8):
      "ac.yield"(%entry) : (!b8) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!mixed) -> !mixed
    %forwarded = "ac.rule"(%c1) ({
    ^bb0(%arg: !b8):
      "ac.yield"(%arg) : (!b8) -> ()
    }) {name = "forward", occurrence = {site = {definition = @ExcludedNegative, ast_path = []}, expansion = []}} : (!b8) -> !b8
    %rule_table = "ac.table.create"(%forwarded, %forwarded, %forwarded) : (!b8, !b8, !b8) -> !mixed
    "ac.yield"(%computed, %mapped, %rule_table) : (!mixed, !mixed, !mixed) -> ()
  }) {sym_name = "ExcludedNegative", source_owner = {package = "", path = "constant.py"}, parameters = [], type_parameters = [], function_type = () -> (!mixed, !mixed, !mixed), input_names = [], output_names = ["computed", "mapped", "forwarded"]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %en: !b1, %dynamic0: !b8, %index0: !b70, %dynamic1: !b8, %index1: !b70):
    %right_en = "ac.bits.constant"() {value = #n1} : () -> !b1
    %clocks = "ac.table.splat"(%clk) {shape = [#w2]} : (!b1) -> !two_b1
    %resets = "ac.table.splat"(%rst) {shape = [#w2]} : (!b1) -> !two_b1
    %enables = "ac.table.splat"(%right_en) {shape = [#w2]} : (!b1) -> !two_b1
    %dynamic_rows = "ac.table.splat"(%dynamic0) {shape = [#w2]} : (!b8) -> !two_b8
    %index_rows = "ac.table.splat"(%index0) {shape = [#w2]} : (!b70) -> !two_b70
    %collection:10 = "ac.collection"(%clocks, %resets, %enables, %dynamic_rows, %index_rows) {instance_name = "pair", callee = @ConstantChild, parameters = [], type_arguments = [], shape = [#w2], occurrence = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}} : (!two_b1, !two_b1, !two_b1, !two_b8, !two_b70) -> (!two_rows, !two_wide, !two_mixed, !two_modes, !two_mode_bits, !two_b1, !two_b130, !two_b8, !two_b1, !two_b8)
    %generic_inputs = "ac.table.splat"(%dynamic0) {shape = [#w2]} : (!b8) -> !generic_inputs
    %generic_outputs = "ac.collection"(%generic_inputs) {instance_name = "generic_pair", callee = @GenericNegative, parameters = [#w3], type_arguments = [], shape = [#w2], occurrence = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}} : (!generic_inputs) -> !generic_outputs
    %excluded:3 = "ac.instance"() {instance_name = "excluded", callee = @ExcludedNegative, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}} : () -> (!mixed, !mixed, !mixed)
    %a:10 = "ac.instance"(%clk, %rst, %en, %dynamic0, %index0) {instance_name = "left", callee = @ConstantChild, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b8, !b70) -> (!rows, !wide, !mixed, !modes, !mode_bits, !b1, !b130, !b8, !b1, !b8)
    %b:10 = "ac.instance"(%clk, %rst, %right_en, %dynamic1, %index1) {instance_name = "right", callee = @ConstantChild, parameters = [], type_arguments = [], occurrence = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}} : (!b1, !b1, !b1, !b8, !b70) -> (!rows, !wide, !mixed, !modes, !mode_bits, !b1, !b130, !b8, !b1, !b8)
    "ac.yield"(%collection#0, %collection#1, %a#2, %a#3, %a#4, %a#5, %a#6, %a#7, %a#8, %a#9, %collection#9, %generic_outputs, %excluded#0, %excluded#1, %excluded#2, %b#0, %b#1, %b#2, %b#3, %b#4, %b#5, %b#6, %b#7, %b#8, %b#9) : (!two_rows, !two_wide, !mixed, !modes, !mode_bits, !b1, !b130, !b8, !b1, !b8, !two_b8, !generic_outputs, !mixed, !mixed, !mixed, !rows, !wide, !mixed, !modes, !mode_bits, !b1, !b130, !b8, !b1, !b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "constant.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b8, !b70, !b8, !b70) -> (!two_rows, !two_wide, !mixed, !modes, !mode_bits, !b1, !b130, !b8, !b1, !b8, !two_b8, !generic_outputs, !mixed, !mixed, !mixed, !rows, !wide, !mixed, !modes, !mode_bits, !b1, !b130, !b8, !b1, !b8), input_names = ["clk", "rst", "en", "dynamic0", "index0", "dynamic1", "index1"], output_names = ["records0", "wide0", "mixed0", "modes0", "mode_bits0", "flag0", "selected_wide0", "selected_dynamic0", "in_range0", "q0", "pair_q", "generic_observed", "computed_observed", "mapped_observed", "forwarded_observed", "records1", "wide1", "mixed1", "modes1", "mode_bits1", "flag1", "selected_wide1", "selected_dynamic1", "in_range1", "q1"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
