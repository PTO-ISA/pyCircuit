#span = {path = "transactional-state.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @StateCycles, ast_path = []}, expansion = []}
#n0 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<0>}}>
#n1 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<1>}}>
#n2 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<2>}}>
#n3 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<3>}}>
#n4 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<4>}}>
#n5 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<5>}}>
#n6 = #ac.static_expr<{kind = "literal", location = #span, origin = #origin, value = {kind = "integer", value = #ac.math_int<6>}}>
!b1 = !ac.bits<#n1>
!s1 = !ac.state<!b1>
!b3 = !ac.bits<#n3>
!s3 = !ac.state<!b3>
!b4 = !ac.bits<#n4>
!s4 = !ac.state<!b4>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  ac.rule @advance ins(!s3) outs() captures() body {
  ^bb0(%counter: !s3):
    %old = ac.method.call %counter::@read() : (!s3) -> (!b3)
    %one = "ac.bits.constant"() {value = #n1} : () -> !b3
    %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b3, !b3) -> !b3
    ac.method.call %counter::@write(%next) : (!s3, !b3) -> ()
    "ac.yield"() : () -> ()
  }
  "ac.module"() ({
    %one = "ac.bits.constant"() {value = #n1} : () -> !b1
    %three = "ac.bits.constant"() {value = #n3} : () -> !b3
    %five = "ac.bits.constant"() {value = #n5} : () -> !b3
    %six = "ac.bits.constant"() {value = #n6} : () -> !b3
    %xz = "ac.bits.constant4"() {pattern = "10xz"} : () -> !b4
    %enabled = ac.resource.alloc "enabled" initial(%one : !b1) : !s1
    %a = ac.resource.alloc "a" initial(%six : !b3) : !s3
    %b = ac.resource.alloc "b" initial(%three : !b3) : !s3
    %counter = ac.resource.alloc "counter" initial(%five : !b3) : !s3
    %shared = ac.resource.alloc "shared" initial(%three : !b3) : !s3
    %payload = ac.resource.alloc "payload" initial(%xz : !b4) : !s4
    %zero4 = "ac.bits.constant"() {value = #n0} : () -> !b4
    %copy = ac.resource.alloc "copy" initial(%zero4 : !b4) : !s4
    %hidden = ac.resource.alloc "hidden" initial(%three : !b3) : !s3
    %comb_enabled = ac.method.call %enabled::@read() : (!s1) -> (!b1)
    ac.rule @toggle ins() outs() captures(%enabled : !s1) body {
    ^bb0(%enabled: !s1):
      %old = ac.method.call %enabled::@read() : (!s1) -> (!b1)
      %next = "ac.bits.unary"(%old) {opcode = "not"} : (!b1) -> !b1
      ac.method.call %enabled::@write(%next) : (!s1, !b1) -> ()
      "ac.yield"() : () -> ()
    }
    ac.rule @pair ins() outs() captures(%comb_enabled : !b1, %a : !s3, %b : !s3, %payload : !s4, %copy : !s4)
      guard {
      ^bb0(%comb_enabled: !b1, %a: !s3, %b: !s3, %payload: !s4, %copy: !s4):
        "ac.yield"(%comb_enabled) : (!b1) -> ()
      } body {
      ^bb0(%comb_enabled: !b1, %a: !s3, %b: !s3, %payload: !s4, %copy: !s4):
        %old = ac.method.call %a::@read() : (!s3) -> (!b3)
        %one = "ac.bits.constant"() {value = #n1} : () -> !b3
        %next = "ac.bits.binary"(%old, %one) {opcode = "add"} : (!b3, !b3) -> !b3
        ac.method.call %a::@write(%next) : (!s3, !b3) -> ()
        %read_after_write = ac.method.call %a::@read() : (!s3) -> (!b3)
        ac.method.call %b::@write(%read_after_write) : (!s3, !b3) -> ()
        %value = ac.method.call %payload::@read() : (!s4) -> (!b4)
        ac.method.call %copy::@write(%value) : (!s4, !b4) -> ()
        "ac.yield"() : () -> ()
      }
    ac.rule @yes ins() outs() captures(%enabled : !s1, %shared : !s3) guard {
    ^bb0(%enabled: !s1, %shared: !s3):
      %go = ac.method.call %enabled::@read() : (!s1) -> (!b1)
      "ac.yield"(%go) : (!b1) -> ()
    } body {
    ^bb0(%enabled: !s1, %shared: !s3):
      %value = "ac.bits.constant"() {value = #n1} : () -> !b3
      ac.method.call %shared::@write(%value) : (!s3, !b3) -> ()
      "ac.yield"() : () -> ()
    }
    ac.rule @no ins() outs() captures(%enabled : !s1, %shared : !s3) guard {
    ^bb0(%enabled: !s1, %shared: !s3):
      %go = ac.method.call %enabled::@read() : (!s1) -> (!b1)
      %stop = "ac.bits.unary"(%go) {opcode = "not"} : (!b1) -> !b1
      "ac.yield"(%stop) : (!b1) -> ()
    } body {
    ^bb0(%enabled: !s1, %shared: !s3):
      %value = "ac.bits.constant"() {value = #n2} : () -> !b3
      ac.method.call %shared::@write(%value) : (!s3, !b3) -> ()
      "ac.yield"() : () -> ()
    }
    ac.rule.call @toggle ins() outs() : ()
    ac.rule.call @pair ins() outs() : ()
    ac.rule.call @yes ins() outs() : ()
    ac.rule.call @no ins() outs() : ()
    ac.rule.call @advance ins(%counter) outs() : (!s3)
    "ac.yield"(%a, %b, %counter, %shared, %payload, %copy, %a) : (!s3, !s3, !s3, !s3, !s4, !s4, !s3) -> ()
  }) {sym_name = "StateCycles", source_owner = {package = "", path = "transactional-state.py"}, parameters = [], type_parameters = [], function_type = () -> (!s3, !s3, !s3, !s3, !s4, !s4, !s3), input_names = [], output_names = ["a", "b", "counter", "shared", "payload", "copy", "alias_a"], output_kinds = ["export", "export", "export", "export", "export", "export", "export"]} : () -> ()
}
