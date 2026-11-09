#w1 = #ac.static_expr<{kind = "literal", location = {path = "aggregate.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "aggregate.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w4 = #ac.static_expr<{kind = "literal", location = {path = "aggregate.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "aggregate.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b2 = !ac.bits<#w2>
!b4 = !ac.bits<#w4>
!b8 = !ac.bits<#w8>
!packet = !ac.struct<"types.Packet">
!mode = !ac.enum<"types.Mode">
module attributes {ac.source_units = [{package = "", path = "aggregate.py"}]} {
  "ac.enum"() {sym_name = "types.Mode", width = #ac.math_int<2>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<1>}]} : () -> ()
  ac.struct "types.Packet" fields [{name = "high", type = !b8}, {name = "low", type = !b4}]
  "ac.module"() ({
  ^bb0(%high: !b8, %low: !b4):
    %packet = "ac.struct.create"(%high, %low) : (!b8, !b4) -> !packet
    %field = "ac.struct.get"(%packet) {field = "low"} : (!packet) -> !b4
    %mode = "ac.enum.create"() {member = "RUN"} : () -> !mode
    %mode_bits = "ac.enum.to_bits"(%mode) : (!mode) -> !b2
    "ac.yield"(%packet, %field, %mode, %mode_bits) : (!packet, !b4, !mode, !b2) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "aggregate.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b4) -> (!packet, !b4, !mode, !b2), input_names = ["high", "low"], output_names = ["packet", "field", "mode", "mode_bits"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
