#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "row_write.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "row_write.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "row_write.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!Table = !ac.table<[#w2], !b8>
!State = !ac.struct<"State">

module attributes {gfir.contract = 1 : i64} {
  ac.struct "State" fields [{name = "rows", type = !Table}, {name = "tag", type = !b8}]
  gfir.module @Update(%old: !State, %outer: !b1, %inner: !b1, %index: !b1, %data: !b8, %tag: !b8) -> (!State)
      attributes {source_owner = {package = "", path = "row_write.py"}, input_names = ["old", "outer", "inner", "index", "data", "tag"], output_names = ["next"]} {
    %rows = ac.struct.get %old["rows"] : (!State) -> !Table
    %updated = "ac.table.map"(%rows, %index, %data) ({
    ^bb0(%ordinal: !b1, %previous: !b8, %key: !b1, %replacement: !b8):
      %hit = "ac.bits.compare"(%ordinal, %key) {predicate = "eq"} : (!b1, !b1) -> !b1
      %value = "ac.bits.select"(%hit, %replacement, %previous) : (!b1, !b8, !b8) -> !b8
      "ac.yield"(%value) : (!b8) -> ()
    }) {shape = [#w2], operandSegmentSizes = array<i32: 1, 2>} : (!Table, !b1, !b8) -> !Table
    %inner_rows = "ac.table.map"(%updated, %rows, %inner) ({
    ^bb0(%ordinal: !b1, %yes: !b8, %no: !b8, %condition: !b1):
      %value = "ac.bits.select"(%condition, %yes, %no) : (!b1, !b8, !b8) -> !b8
      "ac.yield"(%value) : (!b8) -> ()
    }) {shape = [#w2], operandSegmentSizes = array<i32: 2, 1>} : (!Table, !Table, !b1) -> !Table
    %next_rows = "ac.table.map"(%inner_rows, %rows, %outer) ({
    ^bb0(%ordinal: !b1, %yes: !b8, %no: !b8, %condition: !b1):
      %value = "ac.bits.select"(%condition, %yes, %no) : (!b1, !b8, !b8) -> !b8
      "ac.yield"(%value) : (!b8) -> ()
    }) {shape = [#w2], operandSegmentSizes = array<i32: 2, 1>} : (!Table, !Table, !b1) -> !Table
    %next = ac.struct.create(%next_rows, %tag) : (!Table, !b8) -> !State
    gfir.yield %next : !State
  }
  gfir.module @Top(%outer: !b1, %inner: !b1, %index: !b1, %data: !b8, %tag: !b8) -> (!State)
      attributes {source_owner = {package = "", path = "row_write.py"}, input_names = ["outer", "inner", "index", "data", "tag"], output_names = ["old"], domains = ["default"]} {
    %initial = "ac.bits.constant4"() {pattern = "10100000"} : () -> !b8
    %rows = "ac.table.splat"(%initial) {shape = [#w2]} : (!b8) -> !Table
    %state = ac.struct.create(%rows, %initial) : (!Table, !b8) -> !State
    gfir.var @state init %state : !State
    %old = gfir.read @state : !State
    gfir.instance @update of @Update
    %next = gfir.call @update(%old, %outer, %inner, %index, %data, %tag) : (!State, !b1, !b1, !b1, !b8, !b8) -> !State
    gfir.rule @step(%candidate = %next : !State) -> () {
      gfir.write @state, %candidate : !State
      gfir.yield
    }
    gfir.yield %old : !State
  }
  gfir.root @Top kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
