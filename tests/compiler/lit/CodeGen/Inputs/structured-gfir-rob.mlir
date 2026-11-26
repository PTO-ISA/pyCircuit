#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @example_rob.rob.Rob, ast_path = []}, expansion = []}, location = {path = "rob.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @example_rob.rob.Rob, ast_path = []}, expansion = []}, location = {path = "rob.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w3 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<3>}, origin = {site = {definition = @example_rob.rob.Rob, ast_path = []}, expansion = []}, location = {path = "rob.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w4 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<4>}, origin = {site = {definition = @example_rob.rob.Rob, ast_path = []}, expansion = []}, location = {path = "rob.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @example_rob.rob.Rob, ast_path = []}, expansion = []}, location = {path = "rob.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w16 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<16>}, origin = {site = {definition = @example_rob.rob.Rob, ast_path = []}, expansion = []}, location = {path = "rob.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
!b8 = !ac.bits<#w8>
!b16 = !ac.bits<#w16>
!Entry = !ac.struct<"example_rob.rob.Entry">
!RobResult = !ac.struct<"example_rob.rob.RobResult">
!RobRequest = !ac.struct<"example_rob.rob.RobRequest">
!Allocation = !ac.struct<"example_rob.rob.Allocation">
!Retirement = !ac.struct<"example_rob.rob.Retirement">
!Rows = !ac.table<[#w4], !Entry>
module attributes {gfir.contract = 1 : i64} {
  ac.struct "example_rob.rob.Entry" fields [{name = "valid", type = !b1}, {name = "done", type = !b1}, {name = "tag", type = !b8}, {name = "value", type = !b16}]
  ac.struct "example_rob.rob.RobResult" fields [{name = "allocate_accepted", type = !b1}, {name = "allocated_index", type = !b2}, {name = "complete_accepted", type = !b1}, {name = "retire_accepted", type = !b1}, {name = "retired_tag", type = !b8}, {name = "retired_value", type = !b16}, {name = "count", type = !b3}]
  ac.struct "example_rob.rob.RobRequest" fields [{name = "allocate", type = !b1}, {name = "allocate_tag", type = !b8}, {name = "complete", type = !b1}, {name = "complete_index", type = !b2}, {name = "complete_tag", type = !b8}, {name = "complete_value", type = !b16}, {name = "retire", type = !b1}, {name = "flush", type = !b1}]
  ac.struct "example_rob.rob.Allocation" fields [{name = "accepted", type = !b1}, {name = "index", type = !b2}, {name = "count", type = !b3}]
  ac.struct "example_rob.rob.Retirement" fields [{name = "accepted", type = !b1}, {name = "tag", type = !b8}, {name = "value", type = !b16}]
  gfir.module @example_rob.rob.RobStorage(%request: !RobRequest) -> (!RobResult) attributes {source_owner = {package = "example_rob", path = "rob.py"}, input_names = ["request"], output_names = ["result"], domains = ["default"]} {
    %z1 = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
    %z2 = "ac.bits.constant4"() {pattern = "00"} : () -> !b2
    %z3 = "ac.bits.constant4"() {pattern = "000"} : () -> !b3
    %z8 = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    %z16 = "ac.bits.constant4"() {pattern = "0000000000000000"} : () -> !b16
    %one1 = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
    %one2 = "ac.bits.constant4"() {pattern = "01"} : () -> !b2
    %one3 = "ac.bits.constant4"() {pattern = "001"} : () -> !b3
    %empty = ac.struct.create(%z1, %z1, %z8, %z16) : (!b1, !b1, !b8, !b16) -> !Entry
    %initial = "ac.table.splat"(%empty) {shape = [#w4]} : (!Entry) -> !Rows
    gfir.var @entries init %initial : !Rows
    gfir.var @head init %z2 : !b2
    gfir.var @tail init %z2 : !b2
    gfir.var @count init %z3 : !b3
    gfir.method @flush(%request: !RobRequest) -> () {
      %z1 = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
      %z2 = "ac.bits.constant4"() {pattern = "00"} : () -> !b2
      %z3 = "ac.bits.constant4"() {pattern = "000"} : () -> !b3
      %z8 = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
      %z16 = "ac.bits.constant4"() {pattern = "0000000000000000"} : () -> !b16
      %flush = ac.struct.get %request["flush"] : (!RobRequest) -> !b1
      gfir.if %flush : !b1 -> () {
        %empty = ac.struct.create(%z1, %z1, %z8, %z16) : (!b1, !b1, !b8, !b16) -> !Entry
        %rows = "ac.table.splat"(%empty) {shape = [#w4]} : (!Entry) -> !Rows
        gfir.write @entries, %rows : !Rows
        gfir.write @head, %z2 : !b2
        gfir.write @tail, %z2 : !b2
        gfir.write @count, %z3 : !b3
        gfir.yield
      } else { gfir.yield }
      gfir.yield
    }
    gfir.method @complete(%request: !RobRequest) -> (!b1) {
      %z1 = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
      %one1 = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
      %flush = ac.struct.get %request["flush"] : (!RobRequest) -> !b1
      %not_flush = "ac.bits.compare"(%flush, %z1) {predicate = "eq"} : (!b1, !b1) -> !b1
      %enabled = ac.struct.get %request["complete"] : (!RobRequest) -> !b1
      %enabled_unflushed = "ac.bits.binary"(%not_flush, %enabled) {opcode = "and"} : (!b1, !b1) -> !b1
      %rows = gfir.read @entries : !Rows
      %index = ac.struct.get %request["complete_index"] : (!RobRequest) -> !b2
      %entry, %range = "ac.table.get"(%rows, %index) : (!Rows, !b2) -> (!Entry, !b1)
      %valid = ac.struct.get %entry["valid"] : (!Entry) -> !b1
      %tag = ac.struct.get %entry["tag"] : (!Entry) -> !b8
      %requested_tag = ac.struct.get %request["complete_tag"] : (!RobRequest) -> !b8
      %value = ac.struct.get %request["complete_value"] : (!RobRequest) -> !b16
      %match = "ac.bits.compare"(%tag, %requested_tag) {predicate = "eq"} : (!b8, !b8) -> !b1
      %live = "ac.bits.binary"(%enabled_unflushed, %valid) {opcode = "and"} : (!b1, !b1) -> !b1
      %condition = "ac.bits.binary"(%live, %match) {opcode = "and"} : (!b1, !b1) -> !b1
      %result = gfir.if %condition : !b1 -> (!b1) {
        %updated = ac.struct.create(%one1, %one1, %tag, %value) : (!b1, !b1, !b8, !b16) -> !Entry
        gfir.write @entries[%index : !b2], %updated : !Entry
        gfir.yield %one1 : !b1
      } else {
        gfir.yield %z1 : !b1
      }
      gfir.yield %result : !b1
    }
    gfir.method @retire(%request: !RobRequest) -> (!Retirement) {
      %z1 = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
      %z3 = "ac.bits.constant4"() {pattern = "000"} : () -> !b3
      %z8 = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
      %z16 = "ac.bits.constant4"() {pattern = "0000000000000000"} : () -> !b16
      %one1 = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
      %one2 = "ac.bits.constant4"() {pattern = "01"} : () -> !b2
      %one3 = "ac.bits.constant4"() {pattern = "001"} : () -> !b3
      %flush = ac.struct.get %request["flush"] : (!RobRequest) -> !b1
      %not_flush = "ac.bits.compare"(%flush, %z1) {predicate = "eq"} : (!b1, !b1) -> !b1
      %enabled = ac.struct.get %request["retire"] : (!RobRequest) -> !b1
      %enabled_unflushed = "ac.bits.binary"(%not_flush, %enabled) {opcode = "and"} : (!b1, !b1) -> !b1
      %rows = gfir.read @entries : !Rows
      %head = gfir.read @head : !b2
      %count = gfir.read @count : !b3
      %entry, %range = "ac.table.get"(%rows, %head) : (!Rows, !b2) -> (!Entry, !b1)
      %done = ac.struct.get %entry["done"] : (!Entry) -> !b1
      %tag = ac.struct.get %entry["tag"] : (!Entry) -> !b8
      %value = ac.struct.get %entry["value"] : (!Entry) -> !b16
      %nonempty = "ac.bits.compare"(%count, %z3) {predicate = "ne"} : (!b3, !b3) -> !b1
      %active = "ac.bits.binary"(%enabled_unflushed, %nonempty) {opcode = "and"} : (!b1, !b1) -> !b1
      %condition = "ac.bits.binary"(%active, %done) {opcode = "and"} : (!b1, !b1) -> !b1
      %result = gfir.if %condition : !b1 -> (!Retirement) {
        %removed = ac.struct.create(%z1, %z1, %z8, %z16) : (!b1, !b1, !b8, !b16) -> !Entry
        gfir.write @entries[%head : !b2], %removed : !Entry
        %next_head = "ac.bits.binary"(%head, %one2) {opcode = "add"} : (!b2, !b2) -> !b2
        %next_count = "ac.bits.binary"(%count, %one3) {opcode = "sub"} : (!b3, !b3) -> !b3
        gfir.write @head, %next_head : !b2
        gfir.write @count, %next_count : !b3
        %retired = ac.struct.create(%one1, %tag, %value) : (!b1, !b8, !b16) -> !Retirement
        gfir.yield %retired : !Retirement
      } else {
        %rejected = ac.struct.create(%z1, %z8, %z16) : (!b1, !b8, !b16) -> !Retirement
        gfir.yield %rejected : !Retirement
      }
      gfir.yield %result : !Retirement
    }
    gfir.method @allocate(%request: !RobRequest) -> (!Allocation) {
      %z1 = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
      %z2 = "ac.bits.constant4"() {pattern = "00"} : () -> !b2
      %z16 = "ac.bits.constant4"() {pattern = "0000000000000000"} : () -> !b16
      %one1 = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
      %one2 = "ac.bits.constant4"() {pattern = "01"} : () -> !b2
      %one3 = "ac.bits.constant4"() {pattern = "001"} : () -> !b3
      %flush = ac.struct.get %request["flush"] : (!RobRequest) -> !b1
      %not_flush = "ac.bits.compare"(%flush, %z1) {predicate = "eq"} : (!b1, !b1) -> !b1
      %enabled = ac.struct.get %request["allocate"] : (!RobRequest) -> !b1
      %enabled_unflushed = "ac.bits.binary"(%not_flush, %enabled) {opcode = "and"} : (!b1, !b1) -> !b1
      %rows = gfir.read @entries : !Rows
      %count = gfir.read @count : !b3
      %tail = gfir.read @tail : !b2
      %tag = ac.struct.get %request["allocate_tag"] : (!RobRequest) -> !b8
      %capacity = "ac.bits.constant4"() {pattern = "100"} : () -> !b3
      %space = "ac.bits.compare"(%count, %capacity) {predicate = "ult"} : (!b3, !b3) -> !b1
      %condition = "ac.bits.binary"(%enabled_unflushed, %space) {opcode = "and"} : (!b1, !b1) -> !b1
      %result = gfir.if %condition : !b1 -> (!Allocation) {
        %allocated = ac.struct.create(%one1, %z1, %tag, %z16) : (!b1, !b1, !b8, !b16) -> !Entry
        gfir.write @entries[%tail : !b2], %allocated : !Entry
        %next_tail = "ac.bits.binary"(%tail, %one2) {opcode = "add"} : (!b2, !b2) -> !b2
        %next_count = "ac.bits.binary"(%count, %one3) {opcode = "add"} : (!b3, !b3) -> !b3
        gfir.write @tail, %next_tail : !b2
        gfir.write @count, %next_count : !b3
        %accepted = ac.struct.create(%one1, %tail, %next_count) : (!b1, !b2, !b3) -> !Allocation
        gfir.yield %accepted : !Allocation
      } else {
        %rejected = ac.struct.create(%z1, %z2, %count) : (!b1, !b2, !b3) -> !Allocation
        gfir.yield %rejected : !Allocation
      }
      gfir.yield %result : !Allocation
    }
    gfir.rule @transact(%input = %request : !RobRequest) -> (!RobResult) {
      gfir.call self["flush"](%input) : (!RobRequest) -> ()
      %complete = gfir.call self["complete"](%input) : (!RobRequest) -> !b1
      %retire = gfir.call self["retire"](%input) : (!RobRequest) -> !Retirement
      %allocate = gfir.call self["allocate"](%input) : (!RobRequest) -> !Allocation
      %aa = ac.struct.get %allocate["accepted"] : (!Allocation) -> !b1
      %ai = ac.struct.get %allocate["index"] : (!Allocation) -> !b2
      %count = ac.struct.get %allocate["count"] : (!Allocation) -> !b3
      %ra = ac.struct.get %retire["accepted"] : (!Retirement) -> !b1
      %rt = ac.struct.get %retire["tag"] : (!Retirement) -> !b8
      %rv = ac.struct.get %retire["value"] : (!Retirement) -> !b16
      %result = ac.struct.create(%aa, %ai, %complete, %ra, %rt, %rv, %count) : (!b1, !b2, !b1, !b1, !b8, !b16, !b3) -> !RobResult
      gfir.yield %result : !RobResult
    }
    %result = gfir.result @transact[0] : !RobResult
    gfir.yield %result : !RobResult
  }
  gfir.module @example_rob.rob.Rob(%allocate: !b1, %allocate_tag: !b8, %complete: !b1, %complete_index: !b2, %complete_tag: !b8, %complete_value: !b16, %retire: !b1, %flush: !b1) -> (!RobResult) attributes {source_owner = {package = "example_rob", path = "rob.py"}, input_names = ["allocate", "allocate_tag", "complete", "complete_index", "complete_tag", "complete_value", "retire", "flush"], output_names = ["result"], domains = ["default"]} {
    %request = ac.struct.create(%allocate, %allocate_tag, %complete, %complete_index, %complete_tag, %complete_value, %retire, %flush) : (!b1, !b8, !b1, !b2, !b8, !b16, !b1, !b1) -> !RobRequest
    gfir.instance @storage of @example_rob.rob.RobStorage
    %result = gfir.call @storage(%request) : (!RobRequest) -> !RobResult
    gfir.yield %result : !RobResult
  }
  gfir.root @example_rob.rob.Rob kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
