#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Memories, ast_path = []}, expansion = []}, location = {path = "memories.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Memories, ast_path = []}, expansion = []}, location = {path = "memories.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b2 = !ac.bits<#w2>
#w16 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<16>}, origin = {site = {definition = @Memories, ast_path = []}, expansion = []}, location = {path = "memories.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b16 = !ac.bits<#w16>
module attributes {gfir.contract = 1 : i64} {
gfir.primitive @Single native "memory" version 1 {kind = "sync_mem", element = !b16, depth = 3 : i64, access = "raw"}
gfir.primitive @Dual native "memory" version 1 {kind = "sync_mem_dp", element = !b16, depth = 3 : i64, access = "raw"}
gfir.primitive @Bytes native "memory" version 1 {kind = "byte_mem", element = !b16, depth = 3 : i64, access = "raw"}
gfir.module @Memories(%ren: !b1, %raddr: !b2, %other_addr: !b2, %byte_addr: !b2, %write: !b1, %waddr: !b2, %data: !b16, %mask: !b2) -> (!b16, !b16, !b16, !b16)
    attributes {source_owner = {package = "", path = "memories.py"},
                input_names = ["ren", "raddr", "other_addr", "byte_addr", "write", "waddr", "data", "mask"],
                output_names = ["single", "dual0", "dual1", "bytes"], domains = ["default"]} {
  gfir.instance @single_mem of @Single
  gfir.instance @dual_mem of @Dual
  gfir.instance @byte_mem of @Bytes
  %single = gfir.call @single_mem["raw"](%ren, %raddr, %write, %waddr, %data, %mask) : (!b1, !b2, !b1, !b2, !b16, !b2) -> !b16
  %dual0, %dual1 = gfir.call @dual_mem["raw"](%ren, %raddr, %ren, %other_addr, %write, %waddr, %data, %mask) : (!b1, !b2, !b1, !b2, !b1, !b2, !b16, !b2) -> (!b16, !b16)
  %bytes = gfir.call @byte_mem["raw"](%byte_addr, %write, %waddr, %data, %mask) : (!b2, !b1, !b2, !b16, !b2) -> !b16
  gfir.yield %single, %dual0, %dual1, %bytes : !b16, !b16, !b16, !b16
}
gfir.root @Memories kind "module" {
  domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]
}
}
