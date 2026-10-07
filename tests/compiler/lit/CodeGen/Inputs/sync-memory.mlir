#w1 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b1 = !ac.bits<#w1>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b5 = !ac.bits<#w5>
#w8 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
#w70 = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<70>}}>
!b70 = !ac.bits<#w70>

#addr_width = #ac.static_expr<{kind = "reference", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @memory, name = "ADDR_WIDTH"}}>
#payload_width = #ac.static_expr<{kind = "type_width", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, type = !ac.type_param<@memory, "T">}>
#seven = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<7>}}>
#eight = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#four = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<4>}}>
#thirteen = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<13>}}>
#rounded = #ac.static_expr<{kind = "binary", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, operator = "add", lhs = #payload_width, rhs = #seven}>
#lanes = #ac.static_expr<{kind = "binary", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, operator = "floordiv", lhs = #rounded, rhs = #eight}>
#two = #ac.static_expr<{kind = "literal", location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
!b4 = !ac.bits<#four>
!b13 = !ac.bits<#thirteen>
!b2 = !ac.bits<#two>
!addr = !ac.bits<#addr_width>
!strb = !ac.bits<#lanes>
module {
  "ac.module.import"() {sym_name = "memory", source_owner = {package = "gfsim", path = "sync_mem.py"}, parameters = [{name = "ADDR_WIDTH", type = !ac.math_int}, {name = "DEPTH", type = !ac.math_int}], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !addr, !b1, !addr, !ac.type_param<@memory, "T">, !strb) -> !ac.type_param<@memory, "T">, input_names = ["clk", "rst", "ren", "raddr", "wvalid", "waddr", "wdata", "wstrb"], output_names = ["rdata"], primitive_kind = "sync_mem", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %ren: !b1, %raddr: !b4, %wvalid: !b1, %waddr: !b4, %wdata: !b13, %wstrb: !b2):
    %q = "ac.instance"(%clk, %rst, %ren, %raddr, %wvalid, %waddr, %wdata, %wstrb) {instance_name = "ram", callee = @memory, parameters = [#four, #seven], type_arguments = [!b13], occurrence = {site = {definition = @top, ast_path = []}, expansion = []}} : (!b1, !b1, !b1, !b4, !b1, !b4, !b13, !b2) -> !b13
    "ac.yield"(%q) : (!b13) -> ()
  }) {sym_name = "top", source_owner = {package = "", path = "top.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1, !b4, !b1, !b4, !b13, !b2) -> !b13, input_names = ["clk", "rst", "ren", "raddr", "wvalid", "waddr", "wdata", "wstrb"], output_names = ["rdata"]} : () -> ()
  "ac.system"() {entry = {callee = @top, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}
