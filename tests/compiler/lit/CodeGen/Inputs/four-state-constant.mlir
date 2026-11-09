#n1 = #ac.static_expr<{kind = "literal", location = {path = "four.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n65 = #ac.static_expr<{kind = "literal", location = {path = "four.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<65>}}>
#n130 = #ac.static_expr<{kind = "literal", location = {path = "four.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<130>}}>
!b1 = !ac.bits<#n1>
!b65 = !ac.bits<#n65>
!b130 = !ac.bits<#n130>
#occ = {site = {definition = @top, ast_path = []}, expansion = []}
#span = {path = "four.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
module attributes {ac.source_units = [{package = "", path = "four.py"}]} {
  "ac.module"() ({
  ^bb0(%payload: !b65, %available: !b1, %path: !b1):
    %c1 = "ac.bits.constant4"() {pattern = "z"} : () -> !b1
    %c65 = "ac.bits.constant4"() {pattern = "1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxz"} : () -> !b65
    %c130 = "ac.bits.constant4"() {pattern = "10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xz10xzxz"} : () -> !b130
    %required = "ac.value.require"(%payload, %available, %path) {occurrence = #occ, source_location = #span} : (!b65, !b1, !b1) -> !b1
    %unused = "ac.value.require"(%payload, %available, %path) {occurrence = #occ, source_location = #span} : (!b65, !b1, !b1) -> !b1
    "ac.yield"(%c1, %c65, %c130, %required) : (!b1, !b65, !b130, !b1) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "four.py"}, parameters = [], type_parameters = [], function_type = (!b65, !b1, !b1) -> (!b1, !b65, !b130, !b1), input_names = ["payload", "available", "path"], output_names = ["c1", "c65", "c130", "required"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
