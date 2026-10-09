// RUN: %pycircuit_opt %s --split-input-file --ac-verify-hardware | %FileCheck %s

// Map arguments and yields compare hardware meaning across source provenance.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w5capture = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 7 : i64, column = 1 : i64, end_line = 7 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 6 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w5arg = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 8 : i64, column = 1 : i64, end_line = 8 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 7 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w5capturearg = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 9 : i64, column = 1 : i64, end_line = 9 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 8 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w5yield = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 10 : i64, column = 1 : i64, end_line = 10 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 9 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w5out = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 11 : i64, column = 1 : i64, end_line = 11 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 10 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
!capture = !ac.bits<#w5capture>
!entry_arg = !ac.bits<#w5arg>
!capture_arg = !ac.bits<#w5capturearg>
!yield = !ac.bits<#w5yield>
!out = !ac.table<[#w3], !ac.bits<#w5out>>
module {
  "ac.module"() ({
  ^bb0(%a: !a, %data: !capture):
    %mapped = "ac.table.map"(%a, %data) ({
    ^bb0(%ordinal: !ac.bits<#w2>, %entry: !entry_arg, %capture: !capture_arg):
      %sum = "ac.bits.binary"(%entry, %capture) {opcode = "xor"} : (!entry_arg, !capture_arg) -> !entry_arg
      %slice = "ac.bits.extract"(%sum) {low = #w0} : (!entry_arg) -> !yield
      "ac.yield"(%slice) : (!yield) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 1>} : (!a, !capture) -> !out
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a, !capture) -> (), input_names = ["a", "data"], output_names = []} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Map yield comparison independently accepts an actual extract result.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w5yield = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 7 : i64, column = 1 : i64, end_line = 7 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 6 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
!yield = !ac.bits<#w5yield>
module {
  "ac.module"() ({
  ^bb0(%a: !a):
    %mapped = "ac.table.map"(%a) ({
    ^bb0(%ordinal: !ac.bits<#w2>, %entry: !b5):
      %slice = "ac.bits.extract"(%entry) {low = #w0} : (!b5) -> !yield
      "ac.yield"(%slice) : (!yield) -> ()
    }) {shape = [#w3], operandSegmentSizes = array<i32: 1, 0>} : (!a) -> !a
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a) -> (), input_names = ["a"], output_names = []} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// -----
// Match accepts equivalent scalar arguments and a one-bit predicate.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 2 : i64, column = 1 : i64, end_line = 2 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 3 : i64, column = 1 : i64, end_line = 3 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 4 : i64, column = 1 : i64, end_line = 4 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w6 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 5 : i64, column = 1 : i64, end_line = 5 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<6>}}>
#w5capture = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 6 : i64, column = 1 : i64, end_line = 6 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w5arg = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 7 : i64, column = 1 : i64, end_line = 7 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 6 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w5capturearg = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 8 : i64, column = 1 : i64, end_line = 8 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 7 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w1predicate = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 9 : i64, column = 1 : i64, end_line = 9 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = [{kind = "index", value = 8 : i64}]}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#w1>
!b5 = !ac.bits<#w5>
!b6 = !ac.bits<#w6>
!a = !ac.table<[#w3], !b5>
!capture = !ac.bits<#w5capture>
!entry_arg = !ac.bits<#w5arg>
!capture_arg = !ac.bits<#w5capturearg>
!predicate = !ac.bits<#w1predicate>
!mask = !ac.bits<#w3>
module {
  "ac.module"() ({
  ^bb0(%a: !a, %data: !capture):
    %mask = "ac.table.match"(%a, %data) ({
    ^bb0(%entry: !entry_arg, %capture: !capture_arg):
      %hit = "ac.bits.compare"(%entry, %capture) {predicate = "eq"} : (!entry_arg, !capture_arg) -> !predicate
      "ac.yield"(%hit) : (!predicate) -> ()
    }) : (!a, !capture) -> !mask
    "ac.yield"() : () -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!a, !capture) -> (), input_names = ["a", "data"], output_names = []} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// CHECK: "ac.table.map"
// CHECK: "ac.bits.extract"
// CHECK: "ac.table.map"
// CHECK: "ac.bits.extract"
// CHECK: "ac.table.match"
// CHECK: "ac.bits.compare"
