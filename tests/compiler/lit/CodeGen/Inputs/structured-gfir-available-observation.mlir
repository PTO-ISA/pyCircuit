#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @AvailableObservation, ast_path = []}, expansion = []}, location = {path = "available_observation.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @AvailableObservation, ast_path = []}, expansion = []}, location = {path = "available_observation.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
module attributes {gfir.contract = 1 : i64} {
  gfir.module @AvailableObservation() -> () attributes {source_owner = {package = "", path = "available_observation.py"}, input_names = [], output_names = [], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    %two = "ac.bits.constant4"() {pattern = "00000010"} : () -> !b8
    %three = "ac.bits.constant4"() {pattern = "00000011"} : () -> !b8
    %four = "ac.bits.constant4"() {pattern = "00000100"} : () -> !b8
    %x_word = "ac.bits.constant4"() {pattern = "xxxxxxxx"} : () -> !b8
    %z_word = "ac.bits.constant4"() {pattern = "zzzzzzzz"} : () -> !b8
    gfir.var @ticks init %zero : !b8
    %old = gfir.read @ticks : !b8
    %not_zero = "ac.bits.compare"(%old, %zero) {predicate = "ne"} : (!b8, !b8) -> !b1
    %not_four = "ac.bits.compare"(%old, %four) {predicate = "ne"} : (!b8, !b8) -> !b1
    %offer = "ac.bits.binary"(%not_zero, %not_four) {opcode = "and"} : (!b1, !b1) -> !b1
    %is_two = "ac.bits.compare"(%old, %two) {predicate = "eq"} : (!b8, !b8) -> !b1
    %is_three = "ac.bits.compare"(%old, %three) {predicate = "eq"} : (!b8, !b8) -> !b1
    %z_or_count = "ac.bits.select"(%is_three, %z_word, %old) : (!b1, !b8, !b8) -> !b8
    %chosen = "ac.bits.select"(%is_two, %x_word, %z_or_count) : (!b1, !b8, !b8) -> !b8
    gfir.rule @advance() -> () {
      %count = gfir.read @ticks : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%count, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @ticks, %next : !b8
      gfir.yield
    }
    gfir.rule @produce(%data = %chosen : !b8, %admit = %offer : !b1) -> (!b8) {
      gfir.guard %admit : !b1
      gfir.yield %data : !b8
    }
    %produced = gfir.result @produce[0] : !b8
    gfir.rule @observe(%payload = %produced : !b8) -> () {
      %count = gfir.read @ticks : !b8
      gfir.effect log(%count, %payload) {spec = {event = "available", level = "info", items = [{kind = "value", ordinal = 0 : i64}, {kind = "value", ordinal = 1 : i64}]}} : (!b8, !b8)
      %same = "ac.bits.compare"(%payload, %payload) {predicate = "case_eq"} : (!b8, !b8) -> !b1
      gfir.effect report(%same) {spec = {name = "sample"}} : (!b1)
      %five = "ac.bits.constant4"() {pattern = "00000101"} : () -> !b8
      %local_limit = "ac.bits.constant4"() {pattern = "00000110"} : () -> !b8
      %bad = "ac.bits.constant4"() {pattern = "x"} : () -> !b1
      %not_five = "ac.bits.compare"(%count, %five) {predicate = "ne"} : (!b8, !b8) -> !b1
      %at_limit = "ac.bits.compare"(%count, %local_limit) {predicate = "eq"} : (!b8, !b8) -> !b1
      %late = "ac.bits.select"(%at_limit, %bad, %not_five) : (!b1, !b1, !b1) -> !b1
      gfir.guard %late : !b1
      gfir.yield
    }
    gfir.rule @independent_failure() -> () {
      %count = gfir.read @ticks : !b8
      %seven = "ac.bits.constant4"() {pattern = "00000111"} : () -> !b8
      %one = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
      %bad = "ac.bits.constant4"() {pattern = "x"} : () -> !b1
      %at_seven = "ac.bits.compare"(%count, %seven) {predicate = "eq"} : (!b8, !b8) -> !b1
      %late = "ac.bits.select"(%at_seven, %bad, %one) : (!b1, !b1, !b1) -> !b1
      gfir.guard %late : !b1
      gfir.yield
    }
    gfir.yield
  }
  gfir.root @AvailableObservation kind "system" {
    domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}],
    ac.system_samples = {rows = ["01", "11", "10", "00", "10", "10", "00", "10", "00", "10", "10", "00", "10", "00", "10", "10", "00", "10", "10", "00", "10", "10", "00", "10"], origin = {site = {definition = @AvailableObservation, ast_path = []}, expansion = []}, location = {path = "available_observation.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 1 : i64}}
  }
}
