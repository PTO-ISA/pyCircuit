#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "lookup.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "lookup.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, location = {path = "lookup.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!Rows = !ac.table<[#w2], !b8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @Leaf(%rows: !Rows, %index: !b1) -> (!b8, !b8, !b1)
      attributes {source_owner = {package = "", path = "lookup.py"}, input_names = ["rows", "index"], output_names = ["selected", "encoded", "in_range"]} {
    %selected, %range = "ac.table.get"(%rows, %index) : (!Rows, !b1) -> (!b8, !b1)
    %c5a = "ac.bits.constant4"() {pattern = "01011010"} : () -> !b8
    %ccc = "ac.bits.constant4"() {pattern = "11001100"} : () -> !b8
    %c33 = "ac.bits.constant4"() {pattern = "00110011"} : () -> !b8
    %ca5 = "ac.bits.constant4"() {pattern = "10100101"} : () -> !b8
    %cf0 = "ac.bits.constant4"() {pattern = "11110000"} : () -> !b8
    %c0f = "ac.bits.constant4"() {pattern = "00001111"} : () -> !b8
    %x1 = "ac.bits.binary"(%selected, %c5a) {opcode = "xor"} : (!b8, !b8) -> !b8
    %x2 = "ac.bits.unary"(%x1) {opcode = "not"} : (!b8) -> !b8
    %x3 = "ac.bits.binary"(%x2, %ccc) {opcode = "and"} : (!b8, !b8) -> !b8
    %x4 = "ac.bits.binary"(%x3, %c33) {opcode = "or"} : (!b8, !b8) -> !b8
    %x5 = "ac.bits.binary"(%x4, %ca5) {opcode = "xor"} : (!b8, !b8) -> !b8
    %x6 = "ac.bits.unary"(%x5) {opcode = "not"} : (!b8) -> !b8
    %x7 = "ac.bits.binary"(%x6, %cf0) {opcode = "and"} : (!b8, !b8) -> !b8
    %encoded = "ac.bits.binary"(%x7, %c0f) {opcode = "xor"} : (!b8, !b8) -> !b8
    gfir.yield %selected, %encoded, %range : !b8, !b8, !b1
  }
  gfir.module @Top(%rows: !Rows, %index: !b1) -> (!b8, !b8, !b1)
      attributes {source_owner = {package = "", path = "lookup.py"}, input_names = ["rows", "index"], output_names = ["joined", "selected", "in_range"]} {
    gfir.instance @leaf of @Leaf
    %selected, %encoded, %range = gfir.call @leaf(%rows, %index) : (!Rows, !b1) -> (!b8, !b8, !b1)
    // This requested closure consumes both outputs of the same actual call.
    %joined = "ac.bits.binary"(%selected, %encoded) {opcode = "xor"} : (!b8, !b8) -> !b8
    gfir.yield %joined, %selected, %range : !b8, !b8, !b1
  }
  gfir.root @Top kind "module" {domains = []}
}
