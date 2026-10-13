// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics

#w1 = #ac.static_expr<{kind = "literal", location = {path = "case_eq.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "case_eq.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
module {
  "ac.module"() ({
  ^bb0(%lhs: !b1, %rhs: !b8):
    // expected-error @+1 {{comparison operands must have the same width}}
    %bad = "ac.bits.compare"(%lhs, %rhs) {predicate = "case_eq"} : (!b1, !b8) -> !b1
    "ac.yield"(%bad) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "case_eq.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b8) -> !b1, input_names = ["lhs", "rhs"], output_names = ["result"]} : () -> ()
}

// -----

#w8 = #ac.static_expr<{kind = "literal", location = {path = "case_eq.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
module {
  "ac.module"() ({
  ^bb0(%lhs: !b8, %rhs: !b8):
    // expected-error @+1 {{comparison result requires canonical literal width one}}
    %bad = "ac.bits.compare"(%lhs, %rhs) {predicate = "case_eq"} : (!b8, !b8) -> !b8
    "ac.yield"(%bad) : (!b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "case_eq.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b8) -> !b8, input_names = ["lhs", "rhs"], output_names = ["result"]} : () -> ()
}

// -----

#w1 = #ac.static_expr<{kind = "literal", location = {path = "case_eq.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "case_eq.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
module {
  "ac.module"() ({
  ^bb0(%lhs: !b8, %rhs: !b8):
    // expected-error @+1 {{unknown closed bit-vector comparison predicate}}
    %bad = "ac.bits.compare"(%lhs, %rhs) {predicate = "case_equal"} : (!b8, !b8) -> !b1
    "ac.yield"(%bad) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "case_eq.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b8) -> !b1, input_names = ["lhs", "rhs"], output_names = ["result"]} : () -> ()
}
