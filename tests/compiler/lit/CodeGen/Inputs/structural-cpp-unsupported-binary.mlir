#n8 = #ac.static_expr<{kind = "literal", location = {path = "mul.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#n8>
module attributes {ac.source_units = [{package = "", path = "mul.py"}]} {
  "ac.module"() ({
  ^bb0(%a: !b8, %b: !b8):
    %mul = "ac.bits.binary"(%a, %b) {opcode = "mul"} : (!b8, !b8) -> !b8
    "ac.yield"(%mul) : (!b8) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "mul.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b8) -> (!b8), input_names = ["a", "b"], output_names = ["mul_out"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}