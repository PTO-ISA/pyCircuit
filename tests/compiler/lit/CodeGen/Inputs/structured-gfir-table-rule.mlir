// The original TableRule driver supplies the stimulus and independent oracle.
#w1 = #ac.static_expr<{
  kind = "literal", value = {kind = "integer", value = #ac.math_int<1>},
  origin = {site = {definition = @example_table_rule.table_rule.TableRule, ast_path = []}, expansion = []},
  location = {path = "table_rule.py", line = 8 : i64, column = 12 : i64, end_line = 8 : i64, end_column = 17 : i64}}>
#w2 = #ac.static_expr<{
  kind = "literal", value = {kind = "integer", value = #ac.math_int<2>},
  origin = {site = {definition = @example_table_rule.table_rule.TableRule, ast_path = []}, expansion = []},
  location = {path = "table_rule.py", line = 21 : i64, column = 24 : i64, end_line = 21 : i64, end_column = 25 : i64}}>
#w7 = #ac.static_expr<{
  kind = "literal", value = {kind = "integer", value = #ac.math_int<7>},
  origin = {site = {definition = @example_table_rule.table_rule.TableRule, ast_path = []}, expansion = []},
  location = {path = "table_rule.py", line = 9 : i64, column = 12 : i64, end_line = 9 : i64, end_column = 17 : i64}}>
!b1 = !ac.bits<#w1>
!b7 = !ac.bits<#w7>
!E = !ac.struct<"example_table_rule.table_rule.Entry">
!R = !ac.struct<"example_table_rule.table_rule.Result">
!Rows = !ac.table<[#w2], !E>

module attributes {gfir.contract = 1 : i64} {
  ac.struct "example_table_rule.table_rule.Entry" fields [{name = "index", type = !b1}, {name = "value", type = !b7}]
  ac.struct "example_table_rule.table_rule.Result" fields [{name = "ready", type = !b1}, {name = "valid", type = !b1},
                             {name = "data", type = !E}]
  gfir.primitive @EntryFifo2 native "fifo" version 1
      {element = !E, depth = 2 : i64, latency = 1 : i64, ready_policy = "downstream_pop",
       ac.payload_constraint = {kind = "hardware", type = !E, source_kind = "nominal"}}
  gfir.primitive @EntryFifo1 native "fifo" version 1
      {element = !E, depth = 1 : i64, latency = 1 : i64, ready_policy = "downstream_pop"}

  gfir.module @example_table_rule.table_rule.TableRule(%valid: !b1, %data: !E, %take: !b1) -> (!R)
      attributes {source_owner = {package = "example_table_rule", path = "table_rule.py"},
                  input_names = ["valid", "data", "take"], output_names = ["result"],
                  domains = ["default"]} {
    %zero1 = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
    %zero7 = "ac.bits.constant4"() {pattern = "0000000"} : () -> !b7
    %empty = ac.struct.create(%zero1, %zero7) : (!b1, !b7) -> !E
    %initial = "ac.table.splat"(%empty) {shape = [#w2]} : (!E) -> !Rows
    gfir.var @entries init %initial
        {ac.payload_constraint = {kind = "hardware", type = !Rows, source_kind = "table"}} : !Rows
    gfir.instance @ingress of @EntryFifo2
    gfir.instance @egress of @EntryFifo1
        {ac.payload_constraint = {kind = "hardware", type = !E, source_kind = "nominal"}}
    %ready = gfir.call @ingress["raw_in"](%valid, %data) : (!b1, !E) -> !b1
    %out_valid, %head = gfir.call @egress["raw_out"](%take) : (!b1) -> (!b1, !E)

    gfir.method @install(%entry: !E) -> (!E) {
      %index = ac.struct.get %entry["index"] : (!E) -> !b1
      %rows = gfir.read @entries : !Rows
      %old, %in_range = "ac.table.get"(%rows, %index) : (!Rows, !b1) -> (!E, !b1)
      gfir.write @entries[%index : !b1], %entry : !E
      gfir.yield %old : !E
    }

    gfir.rule @transfer() -> () {
      %incoming = gfir.call @ingress["out"]() : () -> !E
      %old = gfir.call self["install"](%incoming) : (!E) -> !E
      gfir.call @egress["in"](%old) : (!E) -> ()
      gfir.yield
    }

    %result = ac.struct.create(%ready, %out_valid, %head) : (!b1, !b1, !E) -> !R
    gfir.yield %result : !R
  }

  gfir.root @example_table_rule.table_rule.TableRule kind "module" {
    domains = [{name = "default", edge = "rising",
                clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}],
    execution_policy = {reset = "strict"}
  }
}
