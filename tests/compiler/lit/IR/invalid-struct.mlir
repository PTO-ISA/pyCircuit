// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b5 = !ac.bits<#w5>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
module {
  ac.struct "Packet" fields [{name = "tag", type = !b5}, {name = "data", type = !b8}]
  "ac.module"() ({
  ^bb0(%data: !b8, %tag: !b5):
    // expected-error @+1 {{struct.create field type mismatch}}
    %bad = ac.struct.create(%data, %tag) : (!b8, !b5) -> !ac.struct<"Packet">
    "ac.yield"(%bad) : (!ac.struct<"Packet">) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b5) -> !ac.struct<"Packet">, input_names = ["data", "tag"], output_names = ["packet"]} : () -> ()
}
