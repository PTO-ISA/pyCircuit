#n0 = #ac.static_expr<{kind = "literal", location = {path = "unmapped.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = {path = "unmapped.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#n4 = #ac.static_expr<{kind = "literal", location = {path = "unmapped.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#n8 = #ac.static_expr<{kind = "literal", location = {path = "unmapped.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#n16 = #ac.static_expr<{kind = "literal", location = {path = "unmapped.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<16>}}>
!b1 = !ac.bits<#n1>
!b4 = !ac.bits<#n4>
!b8 = !ac.bits<#n8>
!b16 = !ac.bits<#n16>
module {
  "ac.module"() ({
  ^bb0(%a: !b8, %b: !b8):
    %eq = "ac.bits.compare"(%a, %b) {predicate = "eq"} : (!b8, !b8) -> !b1
    %wide = "ac.bits.resize"(%a) {mode = "zext"} : (!b8) -> !b16
    %cat = "ac.bits.concat"(%a, %b) : (!b8, !b8) -> !b16
    %slice = "ac.bits.extract"(%a) {low = #n0} : (!b8) -> !b4
    "ac.yield"(%eq, %wide, %cat, %slice) : (!b1, !b16, !b16, !b4) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "unmapped.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b8) -> (!b1, !b16, !b16, !b4), input_names = ["a", "b"], output_names = ["eq_out", "wide_out", "cat_out", "slice_out"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}