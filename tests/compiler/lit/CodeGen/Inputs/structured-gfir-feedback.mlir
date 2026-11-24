#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#bit7 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<7>}, origin = {site = {definition = @top, ast_path = []}, expansion = []}, location = {path = "top.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!offer = !ac.struct<"Offer">

module attributes {gfir.contract = 1 : i64} {
  ac.struct "Offer" fields [{name = "payload", type = !b8}, {name = "mirror", type = !b8}, {name = "sent", type = !b1}]

  gfir.module @producer(%value: !b8, %ready: !b1) -> (!offer)
      attributes {source_owner = {package = "", path = "producer.py"},
                  input_names = ["value", "ready"], output_names = ["offer"]} {
    %aa = "ac.bits.constant4"() {pattern = "10101010"} : () -> !b8
    %mask = "ac.bits.constant4"() {pattern = "01010101"} : () -> !b8
    %common = "ac.bits.binary"(%value, %aa) {opcode = "xor"} : (!b8, !b8) -> !b8
    %mirror = "ac.bits.binary"(%common, %mask) {opcode = "xor"} : (!b8, !b8) -> !b8
    %offer = ac.struct.create(%common, %mirror, %ready) : (!b8, !b8, !b1) -> !offer
    gfir.yield %offer : !offer
  }

  gfir.module @consumer(%offer: !offer, %allow: !b1) -> (!b1, !b8)
      attributes {source_owner = {package = "", path = "consumer.py"},
                  input_names = ["offer", "allow"], output_names = ["ready", "accepted"]} {
    %payload = ac.struct.get %offer["payload"] : (!offer) -> !b8
    %mirror = ac.struct.get %offer["mirror"] : (!offer) -> !b8
    %high = "ac.bits.extract"(%payload) {low = #bit7} : (!b8) -> !b1
    %ready = "ac.bits.binary"(%allow, %high) {opcode = "and"} : (!b1, !b1) -> !b1
    %delivered = ac.struct.get %offer["sent"] : (!offer) -> !b1
    %empty = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    %combined = "ac.bits.binary"(%payload, %mirror) {opcode = "xor"} : (!b8, !b8) -> !b8
    %accepted = "ac.bits.select"(%delivered, %combined, %empty) : (!b1, !b8, !b8) -> !b8
    gfir.yield %ready, %accepted : !b1, !b8
  }

  gfir.module @top(%stimulus: !b8, %permit: !b1) -> (!b8, !b8, !b1, !b8)
      attributes {source_owner = {package = "", path = "top.py"},
                  input_names = ["stimulus", "permit"],
                  output_names = ["forwarded", "mirror", "did_send", "accepted"]} {
    gfir.instance @source of @producer
    gfir.instance @sink of @consumer
    %offer = gfir.call @source(%stimulus, %ready) : (!b8, !b1) -> !offer
    %ready, %accepted = gfir.call @sink(%offer, %permit) : (!offer, !b1) -> (!b1, !b8)
    %forwarded = ac.struct.get %offer["payload"] : (!offer) -> !b8
    %mirror = ac.struct.get %offer["mirror"] : (!offer) -> !b8
    %did_send = ac.struct.get %offer["sent"] : (!offer) -> !b1
    gfir.yield %forwarded, %mirror, %did_send, %accepted : !b8, !b8, !b1, !b8
  }

  gfir.root @top kind "module" {domains = []}
}
