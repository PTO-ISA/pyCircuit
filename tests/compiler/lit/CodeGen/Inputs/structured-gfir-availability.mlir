// Sampled availability crosses Struct construction and field projection.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Availability, ast_path = []}, expansion = []}, location = {path = "availability.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Availability, ast_path = []}, expansion = []}, location = {path = "availability.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!Packet = !ac.struct<"Packet">
module attributes {gfir.contract = 1 : i64} {
  ac.struct "Packet" fields [{name = "data", type = !b8}, {name = "stable", type = !b8}]
  gfir.module @Availability(%allow: !b1, %data: !b8) -> (!Packet, !b8, !b8, !b8)
      attributes {source_owner = {package = "", path = "availability.py"}, input_names = ["allow", "data"], output_names = ["packet", "projected", "independent", "accepted"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    %off = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
    %staged = "ac.bits.binary"(%allow, %off) {opcode = "or"} : (!b1, !b1) -> !b1
    %permit = "ac.bits.binary"(%staged, %off) {opcode = "or"} : (!b1, !b1) -> !b1
    gfir.var @ticks init %zero : !b8
    gfir.var @accepted init %zero : !b8
    gfir.rule @independent() -> () {
      %old = gfir.read @ticks : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @ticks, %next : !b8
      gfir.yield
    }
    gfir.rule @offer(%enabled = %permit : !b1, %payload = %data : !b8) -> (!Packet) {
      gfir.guard %enabled : !b1 loc("availability.py":41:1)
      %old = gfir.read @accepted : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @accepted, %next : !b8
      %out = ac.struct.create(%payload, %old) : (!b8, !b8) -> !Packet
      gfir.yield %out : !Packet
    }
    // This Rule can be scheduled earlier, but its check registers second.
    gfir.rule @monitor(%enabled = %allow : !b1) -> () {
      gfir.guard %enabled : !b1 loc("availability.py":82:1)
      gfir.yield
    }
    %offered = gfir.result @offer[0] : !Packet
    %payload = ac.struct.get %offered["data"] : (!Packet) -> !b8
    %ticks = gfir.read @ticks : !b8
    %mixed = ac.struct.create(%payload, %ticks) : (!b8, !b8) -> !Packet
    %independent = ac.struct.get %mixed["stable"] : (!Packet) -> !b8
    %accepted = gfir.read @accepted : !b8
    gfir.yield %mixed, %payload, %independent, %accepted : !Packet, !b8, !b8, !b8
  }
  gfir.root @Availability kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
