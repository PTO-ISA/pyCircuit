#n1 = #ac.static_expr<{kind = "literal", location = {path = "comb.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n5 = #ac.static_expr<{kind = "literal", location = {path = "comb.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#n8 = #ac.static_expr<{kind = "literal", location = {path = "comb.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#v18 = #ac.static_expr<{kind = "literal", location = {path = "comb.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<18>}}>
#v165 = #ac.static_expr<{kind = "literal", location = {path = "comb.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<165>}}>
!b1 = !ac.bits<#n1>
!b8 = !ac.bits<#n8>
module attributes {ac.source_units = [{package = "", path = "comb.py"}]} {
  "ac.module"() ({
  ^bb0(%a: !b8, %b: !b8, %sel: !b1):
    %k = "ac.bits.constant"() {value = #n5} : () -> !b8
    %and = "ac.bits.binary"(%a, %b) {opcode = "and"} : (!b8, !b8) -> !b8
    %or = "ac.bits.binary"(%a, %b) {opcode = "or"} : (!b8, !b8) -> !b8
    %xor = "ac.bits.binary"(%a, %b) {opcode = "xor"} : (!b8, !b8) -> !b8
    %mux = "ac.bits.select"(%sel, %a, %b) : (!b1, !b8, !b8) -> !b8
    %true_constant = "ac.bits.constant"() {value = #v18} : () -> !b8
    %false_constant = "ac.bits.constant"() {value = #v165} : () -> !b8
    %constant_mux = "ac.bits.select"(%sel, %true_constant, %false_constant) : (!b1, !b8, !b8) -> !b8
    "ac.yield"(%k, %and, %or, %xor, %mux, %constant_mux) : (!b8, !b8, !b8, !b8, !b8, !b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "comb.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b8, !b1) -> (!b8, !b8, !b8, !b8, !b8, !b8), input_names = ["a", "b", "sel"], output_names = ["k", "and_out", "or_out", "xor_out", "mux_out", "constant_mux_out"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
