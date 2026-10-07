// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b5 = !ac.bits<#w5>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
module {
  "ac.module"() ({
  ^bb0(%a: !b5, %b: !b8):
    // expected-error @+1 {{operands and result must have the same symbolic width}}
    %bad = "ac.bits.binary"(%a, %b) {opcode = "xor"} : (!b5, !b8) -> !b5
    "ac.yield"(%bad) : (!b5) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b5, !b8) -> !b5, input_names = ["a", "b"], output_names = ["result"]} : () -> ()
}
// -----
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
#bad_value = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<256>}}>
module {
  // expected-error @+1 {{bits constant does not fit its result width}}
  %bad = "ac.bits.constant"() {value = #bad_value} : () -> !b8
}
