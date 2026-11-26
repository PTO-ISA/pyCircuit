#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @FifoConsumer, ast_path = []}, expansion = []}, location = {path = "fifo_consumer.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @FifoConsumer, ast_path = []}, expansion = []}, location = {path = "fifo_consumer.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!packet = !ac.struct<"QueuePacket">
module attributes {gfir.contract = 1 : i64} {
  ac.struct "QueuePacket" fields [{name = "viewed", type = !b1}, {name = "transferred", type = !b1}, {name = "moves", type = !b8}]
  gfir.primitive @BitFifo native "fifo" version 1
      {element = !b1, depth = 1 : i64, latency = 1 : i64, ready_policy = "downstream_pop"}
  gfir.module @FifoStage(%offer: !b1, %data: !b1, %early: !b1, %late: !b1, %take: !b1) -> (!packet, !b1, !b1, !b1, !b1, !b1)
      attributes {source_owner = {package = "", path = "fifo_stage.py"}, input_names = ["offer", "data", "early", "late", "take"], output_names = ["packet", "in_ready", "in_valid", "head", "out_valid", "out_data"], domains = ["default"]} {
    gfir.instance @ingress of @BitFifo
    gfir.instance @egress of @BitFifo
    %ready = gfir.call @ingress["raw_in"](%offer, %data) : (!b1, !b1) -> !b1
    %unused_ready, %present, %head = gfir.call @ingress["raw_status"]() : () -> (!b1, !b1, !b1)
    %out_valid, %out_data = gfir.call @egress["raw_out"](%take) : (!b1) -> (!b1, !b1)
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @moved init %zero : !b8
    // A view borrows the same old head without consuming it.
    gfir.rule @inspect() -> (!b1) {
      %value = gfir.call @ingress["view"]() : () -> !b1
      gfir.yield %value : !b1
    }
    gfir.rule @transfer(%before = %early : !b1, %after = %late : !b1) -> (!b1) {
      gfir.guard %before : !b1
      %value = gfir.call @ingress["out"]() : () -> !b1
      %old = gfir.read @moved : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @moved, %next : !b8
      gfir.guard %after : !b1
      gfir.call @egress["in"](%value) : (!b1) -> ()
      gfir.yield %value : !b1
    }
    %viewed = gfir.result @inspect[0] : !b1
    %transferred = gfir.result @transfer[0] : !b1
    %moves = gfir.read @moved : !b8
    %packet = ac.struct.create(%viewed, %transferred, %moves) : (!b1, !b1, !b8) -> !packet
    gfir.yield %packet, %ready, %present, %head, %out_valid, %out_data : !packet, !b1, !b1, !b1, !b1, !b1
  }
  gfir.module @FifoConsumer(%offer: !b1, %data: !b1, %early: !b1, %late: !b1, %take: !b1, %consume: !b1) -> (!b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b8, !b8)
      attributes {source_owner = {package = "", path = "fifo_consumer.py"}, input_names = ["offer", "data", "early", "late", "take", "consume"], output_names = ["viewed", "transferred", "derived", "in_ready", "in_valid", "head", "out_valid", "out_data", "moves", "seen"], domains = ["default"]} {
    gfir.instance @child of @FifoStage
    %packet, %ready, %present, %head, %out_valid, %out_data = gfir.call @child(%offer, %data, %early, %late, %take) : (!b1, !b1, !b1, !b1, !b1) -> (!packet, !b1, !b1, !b1, !b1, !b1)
    %viewed = ac.struct.get %packet["viewed"] : (!packet) -> !b1
    %transferred = ac.struct.get %packet["transferred"] : (!packet) -> !b1
    %moves = ac.struct.get %packet["moves"] : (!packet) -> !b8
    %zero1 = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
    %derived = "ac.bits.compare"(%transferred, %zero1) {predicate = "ne"} : (!b1, !b1) -> !b1
    %zero8 = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @accepted init %zero8 : !b8
    gfir.rule @consumer(%enabled = %consume : !b1, %value = %derived : !b1) -> () {
      gfir.guard %enabled : !b1
      gfir.guard %value : !b1
      %old = gfir.read @accepted : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @accepted, %next : !b8
      gfir.yield
    }
    %seen = gfir.read @accepted : !b8
    gfir.yield %viewed, %transferred, %derived, %ready, %present, %head, %out_valid, %out_data, %moves, %seen : !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b1, !b8, !b8
  }
  gfir.root @FifoConsumer kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
