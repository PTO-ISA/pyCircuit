#loc = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @top, ast_path = []}, expansion = []}
#a = #ac.static_expr<{kind = "reference", location = #loc, origin = #origin, ref = {kind = "parameter", owner = @top, name = "A"}}>
#b = #ac.static_expr<{kind = "reference", location = #loc, origin = #origin, ref = {kind = "parameter", owner = @top, name = "B"}}>
#eight = #ac.static_expr<{kind = "literal", location = #loc, origin = #origin, value = {kind = "integer", value = #ac.math_int<8>}}>
#minus_seven = #ac.static_expr<{kind = "literal", location = #loc, origin = #origin, value = {kind = "integer", value = #ac.math_int<-7>}}>
#three = #ac.static_expr<{kind = "literal", location = #loc, origin = #origin, value = {kind = "integer", value = #ac.math_int<3>}}>
#quotient = #ac.static_expr<{kind = "binary", location = #loc, origin = #origin, operator = "floordiv", lhs = #a, rhs = #b}>
#remainder = #ac.static_expr<{kind = "binary", location = #loc, origin = #origin, operator = "mod", lhs = #a, rhs = #b}>
#q_width = #ac.static_expr<{kind = "binary", location = #loc, origin = #origin, operator = "add", lhs = #eight, rhs = #quotient}>
#r_width = #ac.static_expr<{kind = "binary", location = #loc, origin = #origin, operator = "add", lhs = #eight, rhs = #remainder}>
!q_bits = !ac.bits<#q_width>
!r_bits = !ac.bits<#r_width>
module {
  "ac.module"() ({
  ^bb0(%q: !q_bits, %r: !r_bits):
    "ac.yield"(%q, %r) : (!q_bits, !r_bits) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [{name = "A", type = !ac.math_int}, {name = "B", type = !ac.math_int}], type_parameters = [], function_type = (!q_bits, !r_bits) -> (!q_bits, !r_bits), input_names = ["q_in", "r_in"], output_names = ["q", "r"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [#minus_seven, #three], type_arguments = []}, domain = "default"} : () -> ()
}
