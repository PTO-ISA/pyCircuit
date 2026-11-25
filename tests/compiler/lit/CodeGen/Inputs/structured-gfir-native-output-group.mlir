#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "group.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "group.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!Pair = !ac.struct<"Pair">
!Packet = !ac.struct<"Packet">
module attributes {gfir.contract = 1 : i64} {
  ac.struct "Pair" fields [{name = "left", type = !b8}, {name = "right", type = !b8}]
  ac.struct "Packet" fields [{name = "picked", type = !b8}, {name = "left", type = !b8}, {name = "right", type = !b8}, {name = "echo", type = !b8}]
  gfir.module @Leaf(%pair: !Pair, %count: !b8, %selector: !b1) -> (!Packet, !b8)
      attributes {source_owner = {package = "", path = "group.py"}, input_names = ["pair", "count", "selector"], output_names = ["packet", "mirror"]} {
    %a = ac.struct.get %pair["left"] : (!Pair) -> !b8
    %b = ac.struct.get %pair["right"] : (!Pair) -> !b8
    %c5a = "ac.bits.constant4"() {pattern = "01011010"} : () -> !b8
    %cf0 = "ac.bits.constant4"() {pattern = "11110000"} : () -> !b8
    %ccc = "ac.bits.constant4"() {pattern = "11001100"} : () -> !b8
    %x = "ac.bits.binary"(%a, %c5a) {opcode = "xor"} : (!b8, !b8) -> !b8
    %y = "ac.bits.binary"(%b, %cf0) {opcode = "and"} : (!b8, !b8) -> !b8
    %z = "ac.bits.binary"(%x, %y) {opcode = "or"} : (!b8, !b8) -> !b8
    %inverse = "ac.bits.unary"(%z) {opcode = "not"} : (!b8) -> !b8
    %left = "ac.bits.binary"(%inverse, %count) {opcode = "shl"} : (!b8, !b8) -> !b8
    %right = "ac.bits.binary"(%x, %count) {opcode = "lshr"} : (!b8, !b8) -> !b8
    %mixed = "ac.bits.binary"(%left, %right) {opcode = "xor"} : (!b8, !b8) -> !b8
    %masked = "ac.bits.binary"(%mixed, %ccc) {opcode = "and"} : (!b8, !b8) -> !b8
    %picked = "ac.bits.select"(%selector, %masked, %z) : (!b1, !b8, !b8) -> !b8
    %mirror = "ac.bits.binary"(%picked, %a) {opcode = "xor"} : (!b8, !b8) -> !b8
    %packet = ac.struct.create(%picked, %left, %right, %a) : (!b8, !b8, !b8, !b8) -> !Packet
    gfir.yield %packet, %mirror : !Packet, !b8
  }
  gfir.module @Top(%a: !b8, %b: !b8, %count: !b8, %selector: !b1) -> (!b8, !b8, !b8, !b8, !b8)
      attributes {source_owner = {package = "", path = "group.py"}, input_names = ["a", "b", "count", "selector"], output_names = ["picked", "left", "right", "echo", "mirror"]} {
    gfir.instance @leaf of @Leaf
    %pair = ac.struct.create(%a, %b) : (!b8, !b8) -> !Pair
    %packet, %mirror = gfir.call @leaf(%pair, %count, %selector) : (!Pair, !b8, !b1) -> (!Packet, !b8)
    %picked = ac.struct.get %packet["picked"] : (!Packet) -> !b8
    %left = ac.struct.get %packet["left"] : (!Packet) -> !b8
    %right = ac.struct.get %packet["right"] : (!Packet) -> !b8
    %echo = ac.struct.get %packet["echo"] : (!Packet) -> !b8
    // One requested Top output needs two Leaf outputs simultaneously.
    %joined = "ac.bits.binary"(%picked, %mirror) {opcode = "xor"} : (!b8, !b8) -> !b8
    gfir.yield %joined, %left, %right, %echo, %mirror : !b8, !b8, !b8, !b8, !b8
  }
  gfir.root @Top kind "module" {domains = []}
}
