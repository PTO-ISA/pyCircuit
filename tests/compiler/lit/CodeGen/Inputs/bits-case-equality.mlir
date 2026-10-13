#w1 = #ac.static_expr<{kind = "literal", location = {path = "case_eq.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w65 = #ac.static_expr<{kind = "literal", location = {path = "case_eq.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
!b1 = !ac.bits<#w1>
!b65 = !ac.bits<#w65>
module {
  "ac.module"() ({
  ^bb0(%lhs1: !b1, %rhs1: !b1, %lhs65: !b65, %rhs65: !b65):
    %case1 = "ac.bits.compare"(%lhs1, %rhs1) {predicate = "case_eq"} : (!b1, !b1) -> !b1
    %eq1 = "ac.bits.compare"(%lhs1, %rhs1) {predicate = "eq"} : (!b1, !b1) -> !b1
    %case65 = "ac.bits.compare"(%lhs65, %rhs65) {predicate = "case_eq"} : (!b65, !b65) -> !b1
    %eq65 = "ac.bits.compare"(%lhs65, %rhs65) {predicate = "eq"} : (!b65, !b65) -> !b1
    "ac.yield"(%case1, %eq1, %case65, %eq65) : (!b1, !b1, !b1, !b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "case_eq.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b65, !b65) -> (!b1, !b1, !b1, !b1), input_names = ["lhs1", "rhs1", "lhs65", "rhs65"], output_names = ["case1", "eq1", "case65", "eq65"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
