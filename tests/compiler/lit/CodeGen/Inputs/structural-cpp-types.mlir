#w1 = #ac.static_expr<{kind = "literal", location = {path = "types.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "types.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "types.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "types.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
!grid = !ac.table<[#w2, #w3], !b8>
!packet = !ac.struct<"types.Packet">
!mode = !ac.enum<"types.Mode">

module {
  "ac.enum"() {sym_name = "types.Mode", width = #ac.math_int<2>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Packet" fields [{name = "payload", type = !b8}]
  "ac.module"() ({
  ^bb0(%bits: !b8, %grid: !grid, %packet: !packet, %mode: !mode):
    "ac.yield"(%bits, %grid, %packet, %mode) : (!b8, !grid, !packet, !mode) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "types.py"}, parameters = [], type_parameters = [], function_type = (!b8, !grid, !packet, !mode) -> (!b8, !grid, !packet, !mode), input_names = ["bits", "grid", "packet", "mode"], output_names = ["bits_out", "grid_out", "packet_out", "mode_out"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
