#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Parent, ast_path = []}, expansion = []}, location = {path = "nested.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Parent, ast_path = []}, expansion = []}, location = {path = "nested.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Parent, ast_path = []}, expansion = []}, location = {path = "nested.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w32 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<32>}, origin = {site = {definition = @Parent, ast_path = []}, expansion = []}, location = {path = "nested.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!b32 = !ac.bits<#w32>
!Record = !ac.struct<"Record">
!Rows = !ac.table<[#w2], !b8>
module attributes {gfir.contract = 1 : i64} {
  ac.struct "Record" fields [{name = "sum", type = !b8}, {name = "marker", type = !b8}]
  gfir.module @Leaf() -> (!Record, !Rows)
      attributes {source_owner = {package = "", path = "nested.py"}, input_names = [], output_names = ["q", "rows"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    %marker = "ac.bits.constant4"() {pattern = "01011010"} : () -> !b8
    %initial = ac.struct.create(%zero, %marker) : (!b8, !b8) -> !Record
    %rows = "ac.table.splat"(%zero) {shape = [#w2]} : (!b8) -> !Rows
    gfir.var @record init %initial : !Record
    gfir.var @table init %rows : !Rows
    gfir.method @bump_row(%delta: !b8, %index: !b1) -> (!b8) {
      %rows = gfir.read @table : !Rows
      %row, %range = "ac.table.get"(%rows, %index) : (!Rows, !b1) -> (!b8, !b1)
      %next_row = "ac.bits.binary"(%row, %delta) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @table[%index : !b1], %next_row : !b8
      gfir.yield %next_row : !b8
    }
    gfir.method @apply(%delta: !b8, %index: !b1) -> (!b8, !b8) {
      %record = gfir.read @record : !Record
      %sum = ac.struct.get %record["sum"] : (!Record) -> !b8
      %next_sum = "ac.bits.binary"(%sum, %delta) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @record["sum"], %next_sum : !b8
      %next_row = gfir.call self["bump_row"](%delta, %index) : (!b8, !b1) -> !b8
      gfir.yield %next_sum, %next_row : !b8, !b8
    }
    gfir.method @bump_record(%entry: !Record, %delta: !b8) -> (!b8)
        attributes {writable = {destination = 0 : i64}} {
      %record = gfir.read @destination : !Record
      %sum = ac.struct.get %record["sum"] : (!Record) -> !b8
      %marker = ac.struct.get %entry["marker"] : (!Record) -> !b8
      %next_sum = "ac.bits.binary"(%sum, %delta) {opcode = "add"} : (!b8, !b8) -> !b8
      %next = ac.struct.create(%next_sum, %marker) : (!b8, !b8) -> !Record
      gfir.write @destination, %next : !Record
      gfir.yield %next_sum : !b8
    }
    gfir.method @apply_bound(%entry: !Record, %delta: !b8, %index: !b1) -> (!b8, !b8)
        attributes {writable = {accumulator = 0 : i64}} {
      %next_sum = gfir.call self["bump_record"](%delta)
          bindings(0 = @accumulator) : (!Record, !b8) -> !b8
      %next_row = gfir.call self["bump_row"](%delta, %index) : (!b8, !b1) -> !b8
      gfir.yield %next_sum, %next_row : !b8, !b8
    }
    gfir.method @snapshot(%index: !b1) -> (!b8, !b8) {
      %record = gfir.read @record : !Record
      %sum = ac.struct.get %record["sum"] : (!Record) -> !b8
      %rows = gfir.read @table : !Rows
      %row, %range = "ac.table.get"(%rows, %index) : (!Rows, !b1) -> (!b8, !b1)
      gfir.yield %sum, %row : !b8, !b8
    }
    %q = gfir.read @record : !Record
    %committed_rows = gfir.read @table : !Rows
    gfir.yield %q, %committed_rows : !Record, !Rows
  }
  gfir.module @Relay() -> (!Record, !Rows)
      attributes {source_owner = {package = "", path = "nested.py"}, input_names = [], output_names = ["q", "rows"], domains = ["default"]} {
    gfir.instance @leaf of @Leaf {domains = {default = "default"}}
    %q, %rows = gfir.call @leaf() : () -> (!Record, !Rows)
    gfir.yield %q, %rows : !Record, !Rows
  }
  gfir.module @Parent(%allow: !b1, %index: !b1) -> (!Record, !Rows, !b8, !b8, !b32, !Record, !Rows)
      attributes {source_owner = {package = "", path = "nested.py"}, input_names = ["allow", "index"], output_names = ["q", "rows", "observed", "observed_row", "candidate", "untouched", "untouched_rows"], domains = ["default"]} {
    gfir.instance @relay of @Relay {domains = {default = "default"}}
    gfir.instance @unused of @Relay {domains = {default = "default"}}
    %q, %rows = gfir.call @relay() : () -> (!Record, !Rows)
    %untouched, %untouched_rows = gfir.call @unused() : () -> (!Record, !Rows)
    gfir.rule @writer(%enabled = %allow : !b1, %key = %index : !b1) -> (!b32) {
      %three = "ac.bits.constant4"() {pattern = "00000011"} : () -> !b8
      %five = "ac.bits.constant4"() {pattern = "00000101"} : () -> !b8
      %first_sum, %first_row = gfir.call @relay::@leaf["apply"](%three, %key) : (!b8, !b1) -> (!b8, !b8)
      %saved_sum, %saved_row = gfir.call @relay::@leaf["snapshot"](%key) : (!b1) -> (!b8, !b8)
      %second_sum, %second_row = gfir.call @relay::@leaf["apply_bound"](%five, %key)
          bindings(0 = @relay::@leaf::@record) : (!Record, !b8, !b1) -> (!b8, !b8)
      gfir.guard %enabled : !b1 loc("nested.py":60:1)
      %saved = "ac.bits.concat"(%saved_sum, %saved_row, %second_sum, %second_row) : (!b8, !b8, !b8, !b8) -> !b32
      gfir.yield %saved : !b32
    }
    gfir.rule @inspect(%key = %index : !b1) -> (!b8, !b8) {
      %sum, %row = gfir.call @relay::@leaf["snapshot"](%key) : (!b1) -> (!b8, !b8)
      gfir.yield %sum, %row : !b8, !b8
    }
    // OVERLAP
    %observed = gfir.result @inspect[0] : !b8
    %observed_row = gfir.result @inspect[1] : !b8
    %candidate = gfir.result @writer[0] : !b32
    gfir.yield %q, %rows, %observed, %observed_row, %candidate, %untouched, %untouched_rows : !Record, !Rows, !b8, !b8, !b32, !Record, !Rows
  }
  gfir.root @Parent kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
