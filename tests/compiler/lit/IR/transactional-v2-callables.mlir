// RUN: %pycircuit_opt %s -o %t.once
// RUN: %pycircuit_opt %t.once -o %t.twice
// RUN: diff %t.once %t.twice
// RUN: %FileCheck %s < %t.twice
// RUN: %pycircuit_opt --cse %s | %FileCheck %s
// Definition, registration and receiver identity survive serialization.
#w8 = #ac.static_expr<{kind = "literal", location = {path = "callables.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "callables.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s8 = !ac.state<!b8>
!s1 = !ac.state<!b1>
!q8 = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    ac.method @echo : (!b8) -> (!b8) captures() body {
    ^bb0(%value: !b8):
      "ac.yield"(%value) : (!b8) -> ()
    }
    "ac.yield"() : () -> ()
  }) {sym_name = "Child", source_owner = {package = "", path = "callables.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
  "ac.module"() ({
  ^bb0(%input: !q8, %output: !q8):
    %enabled = ac.resource.alloc "enabled" : !s1
    %state = ac.resource.alloc "state" : !s8
    ac.instance "child" @Child inputs() bound() {parameters = [], type_arguments = [], occurrence = {}} : () -> ()
    %child = ac.instance.ref ["child"] : !ac.module_ref<@Child>
    %self = ac.instance.ref [] : !ac.module_ref<@Top>
    ac.method @ready : () -> (!b1) captures(%enabled : !s1) body {
    ^bb0(%enabled: !s1):
      %value = ac.method.call %enabled::@read() : (!s1) -> (!b1)
      "ac.yield"(%value) : (!b1) -> ()
    }
    ac.rule @transfer ins(!q8) outs(!q8) captures(%self : !ac.module_ref<@Top>, %child : !ac.module_ref<@Child>, %state : !s8)
      guard {
      ^bb0(%src: !q8, %dst: !q8, %self: !ac.module_ref<@Top>, %child: !ac.module_ref<@Child>, %state: !s8):
        %ready = ac.method.call %self::@ready() : (!ac.module_ref<@Top>) -> (!b1)
        "ac.yield"(%ready) : (!b1) -> ()
      } body {
      ^bb0(%src: !q8, %dst: !q8, %self: !ac.module_ref<@Top>, %child: !ac.module_ref<@Child>, %state: !s8):
        %input = ac.method.call %src::@pop() : (!q8) -> (!b8)
        %old = ac.method.call %state::@read() : (!s8) -> (!b8)
        %sum = "ac.bits.binary"(%old, %input) {opcode = "add"} : (!b8, !b8) -> !b8
        %result = ac.method.call %child::@echo(%sum) : (!ac.module_ref<@Child>, !b8) -> (!b8)
        ac.method.call %state::@write(%result) : (!s8, !b8) -> ()
        ac.method.call %dst::@push(%result) : (!q8, !b8) -> ()
        "ac.yield"() : () -> ()
      }
    ac.rule @dormant ins() outs() captures() body {
      "ac.yield"() : () -> ()
    }
    ac.rule.call @transfer ins(%input) outs(%output) : (!q8, !q8)
    "ac.yield"(%output) : (!q8) -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "callables.py"}, parameters = [], type_parameters = [], function_type = (!q8) -> (!q8), input_names = ["input"], output_names = ["output"], output_kinds = ["bound"]} : () -> ()
}
// CHECK: ac.method @echo
// CHECK: ac.instance "child" @Child
// CHECK: ac.instance.ref ["child"]
// CHECK: ac.instance.ref []
// CHECK: ac.method @ready
// CHECK: ac.rule @transfer
// CHECK: guard
// CHECK: ac.method.call {{.*}}::@ready()
// CHECK: ac.method.call {{.*}}::@pop()
// CHECK: ac.method.call {{.*}}::@read()
// CHECK: ac.method.call {{.*}}::@echo(
// CHECK: ac.method.call {{.*}}::@write(
// CHECK: ac.method.call {{.*}}::@push(
// CHECK: ac.rule @dormant
// CHECK-NOT: ac.rule.call @dormant
// CHECK: ac.rule.call @transfer ins({{.*}}) outs({{.*}})
// CHECK-NOT: ac.rule.call
