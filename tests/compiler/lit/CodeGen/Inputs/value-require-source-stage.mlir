#n1 = #ac.static_expr<{kind = "literal", location = {path = "source.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#n1>
#occ = {site = {definition = @top, ast_path = []}, expansion = []}
#span = {path = "source.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
module attributes {ac.stage = "source"} {
  "ac.module"() ({
  ^bb0(%payload: !b1, %available: !b1, %path: !b1):
    %bad = "ac.value.require"(%payload, %available, %path) {occurrence = #occ, source_location = #span} : (!b1, !b1, !b1) -> !b1
    "ac.yield"(%bad) : (!b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "source.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> !b1, input_names = ["payload", "available", "path"], output_names = ["bad"]} : () -> ()
}
