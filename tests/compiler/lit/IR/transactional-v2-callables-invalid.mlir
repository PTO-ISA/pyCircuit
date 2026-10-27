// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics

// A readonly-looking wrapper cannot hide a consuming Method call.
#w8 = #ac.static_expr<{kind = "literal", location = {path = "callables.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "callables.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!q8 = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
  ^bb0(%queue: !q8):
    %self = ac.instance.ref [] : !ac.module_ref<@Top>
    ac.method @consume : () -> (!b8) captures(%queue : !q8) body {
    ^bb0(%queue: !q8):
      %value = ac.method.call %queue::@pop() : (!q8) -> (!b8)
      "ac.yield"(%value) : (!b8) -> ()
    }
    ac.method @wrapper : () -> (!b1) captures(%self : !ac.module_ref<@Top>) body {
    ^bb0(%self: !ac.module_ref<@Top>):
      %value = ac.method.call %self::@consume() : (!ac.module_ref<@Top>) -> (!b8)
      %ready = "ac.bits.compare"(%value, %value) {predicate = "eq"} : (!b8, !b8) -> !b1
      "ac.yield"(%ready) : (!b1) -> ()
    }
    // expected-error @+1 {{read-only}}
    ac.rule @guarded ins() outs() captures(%self : !ac.module_ref<@Top>) guard {
    ^bb0(%self: !ac.module_ref<@Top>):
      %ready = ac.method.call %self::@wrapper() : (!ac.module_ref<@Top>) -> (!b1)
      "ac.yield"(%ready) : (!b1) -> ()
    } body {
    ^bb0(%self: !ac.module_ref<@Top>):
      "ac.yield"() : () -> ()
    }
    "ac.yield"() : () -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "callables.py"}, parameters = [], type_parameters = [], function_type = (!q8) -> (), input_names = ["queue"], output_names = [], output_kinds = []} : () -> ()
}

// -----

// Public Method parameters are payload, not resource interfaces.
#w8 = #ac.static_expr<{kind = "literal", location = {path = "callables.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Top, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
!s8 = !ac.state<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    // expected-error @+1 {{payload}}
    ac.method @bad : (!s8) -> () captures() body {
    ^bb0(%state: !s8):
      "ac.yield"() : () -> ()
    }
    "ac.yield"() : () -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "callables.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// A private Rule is not a Method on an existing instance.
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    ac.rule @private ins() outs() captures() body {
      "ac.yield"() : () -> ()
    }
    "ac.yield"() : () -> ()
  }) {sym_name = "Child", source_owner = {package = "", path = "callables.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
  "ac.module"() ({
    ac.instance "child" @Child inputs() bound() {parameters = [], type_arguments = [], occurrence = {}} : () -> ()
    %child = ac.instance.ref ["child"] : !ac.module_ref<@Child>
    ac.rule @caller ins() outs() captures(%child : !ac.module_ref<@Child>) body {
    ^bb0(%child: !ac.module_ref<@Child>):
      // expected-error @+1 {{Method}}
      ac.method.call %child::@private() : (!ac.module_ref<@Child>) -> ()
      "ac.yield"() : () -> ()
    }
    "ac.yield"() : () -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "callables.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}
