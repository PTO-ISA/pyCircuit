#n1 = #ac.static_expr<{kind = "literal", location = {path = "bad.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#n1>
module {
  "ac.module"() ({
    %bad = "ac.bits.constant4"() {pattern = "X"} : () -> !b1
    "ac.yield"(%bad) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "bad.py"}, parameters = [], type_parameters = [], function_type = () -> !b1, input_names = [], output_names = ["bad"]} : () -> ()
}
