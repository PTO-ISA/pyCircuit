#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @ChildAvailability, ast_path = []}, expansion = []}, location = {path = "child_availability.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @ChildAvailability, ast_path = []}, expansion = []}, location = {path = "child_availability.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!packet = !ac.struct<"GuardPacket">
module attributes {gfir.contract = 1 : i64} {
  ac.struct "GuardPacket" fields [{name = "gate", type = !b1}, {name = "committed", type = !b8}]
  // These are independent result ports: the committed-state output is total.
  gfir.module @GuardedChild(%allow: !b1, %payload: !b1) -> (!b1, !b8)
      attributes {source_owner = {package = "", path = "guarded_child.py"}, input_names = ["allow", "payload"], output_names = ["gate", "committed"], domains = ["default"]} {
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @issued init %zero : !b8
    gfir.rule @produce(%enabled = %allow : !b1, %value = %payload : !b1) -> (!b1) {
      gfir.guard %enabled : !b1 loc("guarded_child.py":10:1)
      %old = gfir.read @issued : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @issued, %next : !b8
      gfir.yield %value : !b1
    }
    %gate = gfir.result @produce[0] : !b1
    %committed = gfir.read @issued : !b8
    gfir.yield %gate, %committed : !b1, !b8
  }
  gfir.module @ChildAvailability(%left_allow: !b1, %left_payload: !b1, %right_allow: !b1, %right_payload: !b1, %left_enable: !b1, %right_enable: !b1) -> (!packet, !b1, !b8, !b8, !b8, !b8)
      attributes {source_owner = {package = "", path = "child_availability.py"}, input_names = ["left_allow", "left_payload", "right_allow", "right_payload", "left_enable", "right_enable"], output_names = ["packet", "right_gate", "left_committed", "right_committed", "left_seen", "right_seen"], domains = ["default"]} {
    gfir.instance @left of @GuardedChild
    gfir.instance @right of @GuardedChild
    %left_gate, %left_q = gfir.call @left(%left_allow, %left_payload) : (!b1, !b1) -> (!b1, !b8)
    %right_gate, %right_q = gfir.call @right(%right_allow, %right_payload) : (!b1, !b1) -> (!b1, !b8)
    %packet = ac.struct.create(%left_gate, %left_q) : (!b1, !b8) -> !packet
    %other = ac.struct.create(%right_gate, %right_q) : (!b1, !b8) -> !packet
    %selected_left = ac.struct.get %packet["gate"] : (!packet) -> !b1
    %selected_right = ac.struct.get %other["gate"] : (!packet) -> !b1
    %ordinary_left = ac.struct.get %packet["committed"] : (!packet) -> !b8
    %ordinary_right = ac.struct.get %other["committed"] : (!packet) -> !b8
    %zero = "ac.bits.constant4"() {pattern = "00000000"} : () -> !b8
    gfir.var @seen_left init %zero : !b8
    gfir.var @seen_right init %zero : !b8
    gfir.rule @consume_left(%enabled = %left_enable : !b1, %value = %selected_left : !b1) -> () {
      gfir.guard %enabled : !b1 loc("child_availability.py":40:1)
      gfir.guard %value : !b1 loc("child_availability.py":41:1)
      %old = gfir.read @seen_left : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @seen_left, %next : !b8
      gfir.yield
    }
    gfir.rule @consume_right(%enabled = %right_enable : !b1, %value = %selected_right : !b1) -> () {
      gfir.guard %enabled : !b1 loc("child_availability.py":50:1)
      gfir.guard %value : !b1 loc("child_availability.py":51:1)
      %old = gfir.read @seen_right : !b8
      %one = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
      %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b8, !b8) -> !b8
      gfir.write @seen_right, %next : !b8
      gfir.yield
    }
    %left_seen = gfir.read @seen_left : !b8
    %right_seen = gfir.read @seen_right : !b8
    gfir.yield %packet, %selected_right, %ordinary_left, %ordinary_right, %left_seen, %right_seen : !packet, !b1, !b8, !b8, !b8, !b8
  }
  gfir.root @ChildAvailability kind "module" {domains = [{name = "default", edge = "rising", clock = {generated = "pyc_clk"}, reset = {generated = "pyc_rst"}}]}
}
