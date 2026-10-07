// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics

// Shape/rank, ordinal width, isolation, non-pure operations, and multi-bit
// match predicates retain coverage in collection-invalid-regions.mlir.
// Missing map element argument.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
module {
  "ac.module"() ({
  ^bb0(%a: !a):
    // expected-error @+1 {{requires isolated single block with matching scalar arguments and ac.yield}}
    %mapped = "ac.table.map"(%a) ({
    ^bb0(%ordinal: !ac.bits<#w2>):
      %zero = "ac.bits.constant"() {value = #w0} : () -> !b5
      "ac.yield"(%zero) : (!b5) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!a) -> !a
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> (), input_names = ["a"], output_names = []} : () -> ()
}

// -----
// Extra map scalar argument.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
module {
  "ac.module"() ({
  ^bb0(%a: !a):
    // expected-error @+1 {{requires isolated single block with matching scalar arguments and ac.yield}}
    %mapped = "ac.table.map"(%a) ({
    ^bb0(%ordinal: !ac.bits<#w2>, %entry: !b5, %extra: !b5):
      "ac.yield"(%entry) : (!b5) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!a) -> !a
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> (), input_names = ["a"], output_names = []} : () -> ()
}

// -----
// Missing map yield.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
module {
  "ac.module"() ({
  ^bb0(%a: !a):
    // expected-error @+1 {{scalar region yield types do not match results}}
    %mapped = "ac.table.map"(%a) ({
    ^bb0(%ordinal: !ac.bits<#w2>, %entry: !b5):
      "ac.yield"() : () -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!a) -> !a
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> (), input_names = ["a"], output_names = []} : () -> ()
}

// -----
// Extra map yield.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
module {
  "ac.module"() ({
  ^bb0(%a: !a):
    // expected-error @+1 {{scalar region yield types do not match results}}
    %mapped = "ac.table.map"(%a) ({
    ^bb0(%ordinal: !ac.bits<#w2>, %entry: !b5):
      "ac.yield"(%entry, %entry) : (!b5, !b5) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!a) -> !a
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> (), input_names = ["a"], output_names = []} : () -> ()
}

// -----
// Five- and six-bit element arguments remain distinct.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
module {
  "ac.module"() ({
  ^bb0(%a: !a):
    // expected-error @+1 {{requires isolated single block with matching scalar arguments and ac.yield}}
    %mapped = "ac.table.map"(%a) ({
    ^bb0(%ordinal: !ac.bits<#w2>, %entry: !b6):
      %slice = "ac.bits.extract"(%entry) {low = #w0} : (!b6) -> !b5
      "ac.yield"(%slice) : (!b5) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!a) -> !a
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> (), input_names = ["a"], output_names = []} : () -> ()
}

// -----
// A six-bit result cannot satisfy a five-bit map result element.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
module {
  "ac.module"() ({
  ^bb0(%a: !a):
    // expected-error @+1 {{scalar region yield types do not match results}}
    %mapped = "ac.table.map"(%a) ({
    ^bb0(%ordinal: !ac.bits<#w2>, %entry: !b5):
      %wide = "ac.bits.resize"(%entry) {mode = "zext"} : (!b5) -> !b6
      "ac.yield"(%wide) : (!b6) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!a) -> !a
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> (), input_names = ["a"], output_names = []} : () -> ()
}

// -----
// Equal packed widths do not equate different nominal struct arguments.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
!records = !ac.table<[#w3], !ac.struct<"First">>
module {
  ac.struct "First" fields [{name = "value", type = !b5}]
  ac.struct "Second" fields [{name = "value", type = !b5}]
  "ac.module"() ({
  ^bb0(%a: !records):
    // expected-error @+1 {{requires isolated single block with matching scalar arguments and ac.yield}}
    %mapped = "ac.table.map"(%a) ({
    ^bb0(%ordinal: !ac.bits<#w2>, %entry: !ac.struct<"Second">):
      "ac.yield"(%entry) : (!ac.struct<"Second">) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!records) -> !records
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!records) -> (), input_names = ["a"], output_names = []} : () -> ()
}

// -----
// Equal packed widths do not equate different nominal struct yields.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
!records = !ac.table<[#w3], !ac.struct<"First">>
module {
  ac.struct "First" fields [{name = "value", type = !b5}]
  ac.struct "Second" fields [{name = "value", type = !b5}]
  "ac.module"() ({
  ^bb0(%a: !records):
    // expected-error @+1 {{scalar region yield types do not match results}}
    %mapped = "ac.table.map"(%a) ({
    ^bb0(%ordinal: !ac.bits<#w2>, %entry: !ac.struct<"First">):
      %field = "ac.struct.get"(%entry) {field = "value"} : (!ac.struct<"First">) -> !b5
      %other = "ac.struct.create"(%field) : (!b5) -> !ac.struct<"Second">
      "ac.yield"(%other) : (!ac.struct<"Second">) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!records) -> !records
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!records) -> (), input_names = ["a"], output_names = []} : () -> ()
}

// -----
// Missing match capture argument.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
module {
  "ac.module"() ({
  ^bb0(%a: !a, %data: !b5):
    // expected-error @+1 {{requires isolated single block with matching scalar arguments and ac.yield}}
    %mask = "ac.table.match"(%a, %data) ({
    ^bb0(%entry: !b5):
      %hit = "ac.bits.constant"() {value = #w0} : () -> !b1
      "ac.yield"(%hit) : (!b1) -> ()
    }) : (!a, !b5) -> !ac.bits<#w3>
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a, !b5) -> (), input_names = ["a", "data"], output_names = []} : () -> ()
}

// -----
// Extra match scalar argument.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
module {
  "ac.module"() ({
  ^bb0(%a: !a, %data: !b5):
    // expected-error @+1 {{requires isolated single block with matching scalar arguments and ac.yield}}
    %mask = "ac.table.match"(%a, %data) ({
    ^bb0(%entry: !b5, %capture: !b5, %extra: !b5):
      %hit = "ac.bits.constant"() {value = #w0} : () -> !b1
      "ac.yield"(%hit) : (!b1) -> ()
    }) : (!a, !b5) -> !ac.bits<#w3>
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a, !b5) -> (), input_names = ["a", "data"], output_names = []} : () -> ()
}

// -----
// Mismatched match capture width.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
module {
  "ac.module"() ({
  ^bb0(%a: !a, %data: !b5):
    // expected-error @+1 {{requires isolated single block with matching scalar arguments and ac.yield}}
    %mask = "ac.table.match"(%a, %data) ({
    ^bb0(%entry: !b5, %capture: !b6):
      %hit = "ac.bits.constant"() {value = #w0} : () -> !b1
      "ac.yield"(%hit) : (!b1) -> ()
    }) : (!a, !b5) -> !ac.bits<#w3>
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a, !b5) -> (), input_names = ["a", "data"], output_names = []} : () -> ()
}

// -----
// Missing match predicate yield.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
module {
  "ac.module"() ({
  ^bb0(%a: !a):
    // expected-error @+1 {{table.match predicate must yield one bit}}
    %mask = "ac.table.match"(%a) ({
    ^bb0(%entry: !b5):
      "ac.yield"() : () -> ()
    }) : (!a) -> !ac.bits<#w3>
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> (), input_names = ["a"], output_names = []} : () -> ()
}

// -----
// Unresolved width references retain their distinct formal owners.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
#top_w = #ac.static_expr<{kind = "reference", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @top, name = "W"}}>
#other_w = #ac.static_expr<{kind = "reference", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @other, name = "W"}}>
!formal_table = !ac.table<[#w3], !ac.bits<#top_w>>
module {
  "ac.module.import"() {sym_name = "other", source_owner = {package = "", path = "other.py"}, parameters = [{name = "W", type = !ac.math_int}], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], dependency_summary = []} : () -> ()
  "ac.module"() ({
  ^bb0(%a: !formal_table):
    // expected-error @+1 {{requires isolated single block with matching scalar arguments and ac.yield}}
    %mapped = "ac.table.map"(%a) ({
    ^bb0(%ordinal: !ac.bits<#w2>, %entry: !ac.bits<#other_w>):
      %zero = "ac.bits.constant"() {value = #w0} : () -> !b5
      "ac.yield"(%zero) : (!b5) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!formal_table) -> !a
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [{name = "W", type = !ac.math_int}], type_parameters = [], function_type = (!formal_table) -> (), input_names = ["a"], output_names = []} : () -> ()
}
