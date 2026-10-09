#w0 = #ac.static_expr<{kind = "literal", location = {path = "transport.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "transport.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "transport.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "transport.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w4 = #ac.static_expr<{kind = "literal", location = {path = "transport.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "transport.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "transport.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w17 = #ac.static_expr<{kind = "literal", location = {path = "transport.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<17>}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b4 = !ac.bits<#w4>
!b5 = !ac.bits<#w5>
!b8 = !ac.bits<#w8>
!nested_rows = !ac.table<[#w2], !ac.table<[#w3], !b8>>
!samples = !ac.table<[#w5], !b5>
!inner = !ac.struct<"SampleGroup">
!groups = !ac.table<[#w3], !inner>
!packet = !ac.struct<"SamplePacket">
!packets = !ac.table<[#w3], !packet>
!projected = !ac.table<[#w3, #w3], !inner>
!sample_rows = !ac.table<[#w3, #w5], !b5>
module {
  // 5 + 3*(4 + 5*5) = 92 packed bits. First field/element is MSB.
  ac.struct "SampleGroup" fields [{name = "tag", type = !b4}, {name = "samples", type = !samples}]
  ac.struct "SamplePacket" fields [{name = "marker", type = !b5}, {name = "groups", type = !groups}]
  "ac.module"() ({
  ^bb0(%source: !packet, %replacement: !groups):
    %marker = ac.struct.get %source["marker"] : (!packet) -> !b5
    %groups = ac.struct.get %source["groups"] : (!packet) -> !groups
    %i0 = "ac.bits.constant"() {value = #w0} : () -> !b2
    %i1 = "ac.bits.constant"() {value = #w1} : () -> !b2
    %i2 = "ac.bits.constant"() {value = #w2} : () -> !b2
    %g0, %v0 = "ac.table.get"(%groups, %i0) : (!groups, !b2) -> (!inner, !b1)
    %g1, %v1 = "ac.table.get"(%groups, %i1) : (!groups, !b2) -> (!inner, !b1)
    %g2, %v2 = "ac.table.get"(%groups, %i2) : (!groups, !b2) -> (!inner, !b1)
    %tag = ac.struct.get %g1["tag"] : (!inner) -> !b4
    %samples = ac.struct.get %g1["samples"] : (!inner) -> !samples
    %s0 = "ac.bits.constant"() {value = #w0} : () -> !b3
    %s1 = "ac.bits.constant"() {value = #w1} : () -> !b3
    %s2 = "ac.bits.constant"() {value = #w2} : () -> !b3
    %s3 = "ac.bits.constant"() {value = #w3} : () -> !b3
    %s4 = "ac.bits.constant"() {value = #w4} : () -> !b3
    %a, %va = "ac.table.get"(%samples, %s0) : (!samples, !b3) -> (!b5, !b1)
    %b, %vb = "ac.table.get"(%samples, %s1) : (!samples, !b3) -> (!b5, !b1)
    %c, %vc = "ac.table.get"(%samples, %s2) : (!samples, !b3) -> (!b5, !b1)
    %d, %vd = "ac.table.get"(%samples, %s3) : (!samples, !b3) -> (!b5, !b1)
    %e, %ve = "ac.table.get"(%samples, %s4) : (!samples, !b3) -> (!b5, !b1)
    %rotated = "ac.table.create"(%e, %a, %b, %c, %d) {shape = [#w5]} : (!b5, !b5, !b5, !b5, !b5) -> !samples
    %middle = ac.struct.create(%tag, %rotated) : (!b4, !samples) -> !inner
    %rebuilt_groups = "ac.table.create"(%g0, %middle, %g2) {shape = [#w3]} : (!inner, !inner, !inner) -> !groups
    %rebuilt = ac.struct.create(%marker, %rebuilt_groups) : (!b5, !groups) -> !packet
    %true = "ac.bits.constant"() {value = #w1} : () -> !b1
    %changed, %changed_en = "ac.value.merge"(%source, %true, %replacement) {paths = [["groups"]], operandSegmentSizes = array<i32: 1, 1, 1>} : (!packet, !b1, !groups) -> (!packet, !b1)
    %new_marker = "ac.bits.constant"() {value = #w17} : () -> !b5
    %retagged, %retagged_en = "ac.value.merge"(%source, %true, %new_marker) {paths = [["marker"]], operandSegmentSizes = array<i32: 1, 1, 1>} : (!packet, !b1, !b5) -> (!packet, !b1)
    "ac.yield"(%source, %rebuilt, %changed, %groups, %samples, %retagged) : (!packet, !packet, !packet, !groups, !samples, !packet) -> ()
  }) {sym_name = "transport", source_owner = {package = "", path = "transport.py"}, parameters = [], type_parameters = [], function_type = (!packet, !groups) -> (!packet, !packet, !packet, !groups, !samples, !packet), input_names = ["source", "replacement"], output_names = ["saved", "rebuilt", "changed", "groups", "samples", "retagged"]} : () -> ()
  "ac.module"() ({
  ^bb0(%source: !packets, %replacement: !projected, %nested_input: !nested_rows):
    %saved, %rebuilt, %changed, %groups, %samples, %retagged = "ac.collection"(%source, %replacement) {instance_name = "transports", callee = @transport, parameters = [], type_arguments = [], shape = [#w3], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!packets, !projected) -> (!packets, !packets, !packets, !projected, !sample_rows, !packets)
    // Root Table-of-Table axes are contiguous runtime lanes, in first-axis order.
    "ac.yield"(%saved, %rebuilt, %changed, %groups, %samples, %nested_input, %retagged) : (!packets, !packets, !packets, !projected, !sample_rows, !nested_rows, !packets) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!packets, !projected, !nested_rows) -> (!packets, !packets, !packets, !projected, !sample_rows, !nested_rows, !packets), input_names = ["source", "replacement", "nested_input"], output_names = ["saved", "rebuilt", "changed", "groups", "samples", "nested_saved", "retagged"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
