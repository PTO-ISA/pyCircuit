// RUN: %pycircuit_opt %s -o %t.before.mlir
// RUN: %pycircuit_opt %s --ac-analyze-transaction-state-ssa -o %t.after.mlir
// RUN: diff %t.before.mlir %t.after.mlir
// RUN: sed '/EFFECT$/s/#w1/#w0/' %s > %t.conditional.mlir
// RUN: not %pycircuit_opt %t.conditional.mlir --ac-analyze-transaction-state-ssa 2>&1 | %FileCheck %s --check-prefix=REJECT
// REJECT: error:
// REJECT-SAME: path
// Native provenance analysis only; the pass preserves source transaction IR.
#w0 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<0>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "state.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "state.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "state.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
#state = {site = {definition = @Counter, ast_path = [{kind = "index", value = 0 : i64}]}, expansion = []}
#method = {site = {definition = @Counter, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#read = {site = {definition = @Counter, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#write = {site = {definition = @Counter, ast_path = [{kind = "index", value = 3 : i64}]}, expansion = []}
#rule = {site = {definition = @Counter, ast_path = [{kind = "index", value = 4 : i64}]}, expansion = []}
#call1 = {site = {definition = @Counter, ast_path = [{kind = "index", value = 5 : i64}]}, expansion = []}
#call2 = {site = {definition = @Counter, ast_path = [{kind = "index", value = 6 : i64}]}, expansion = []}
#byte = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
#span = {path = "state.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
module {
  "ac.module"() ({
  ^bb0(%delta: !b8, %clk: !b1, %rst: !b1):
    %yes = "ac.bits.constant"() {value = #w1} : () -> !b1
    %effect = "ac.bits.constant"() {value = #w1} : () -> !b1 // EFFECT
    %initial = "ac.bits.constant"() {value = #w0} : () -> !b8
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #state} : (!b1, !b1, !b8) -> !b8
    "ac.method"(%yes) ({
    ^bb0(%amount: !b8, %path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%amount: !b8, %path: !b1):
      %old = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #read} : (!b1) -> !b8
      %next = "ac.bits.binary"(%old, %amount) {opcode = "add"} : (!b8, !b8) -> !b8
      "ac.state.write"(%path, %next) {state = "count", selectors = [], occurrence = #write} : (!b1, !b8) -> ()
      "ac.yield"(%next) : (!b8) -> ()
    }) {sym_name = "add", function_type = (!b8) -> !b8, argument_names = ["amount"], ac.parameters = [{name = "amount", binding = "positional_or_keyword", constraint = #byte, default = {present = false}, origin = #method, location = #span}], ac.result_constraints = [#byte], ac.return_form = "single", occurrence = #method} : (!b1) -> ()
    "ac.transaction"(%clk, %rst, %yes, %delta, %effect) ({
    ^bb0(%path: !b1, %amount: !b8, %effect_path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %amount: !b8, %effect_path: !b1):
      %first = "ac.method.call"(%effect_path, %amount) {callee = @Counter::@add, instance_path = [], occurrence = #call1} : (!b1, !b8) -> !b8
      %second = "ac.method.call"(%effect_path, %amount) {callee = @Counter::@add, instance_path = [], occurrence = #call2} : (!b1, !b8) -> !b8
      "ac.yield"() : () -> ()
    }) {name = "step", declaration = #rule, occurrence = #rule} : (!b1, !b1, !b1, !b8, !b1) -> ()
    "ac.yield"(%q) : (!b8) -> ()
  }) {sym_name = "Counter", source_owner = {package = "", path = "state.py"}, parameters = [], type_parameters = [], function_type = (!b8, !b1, !b1) -> !b8, input_names = ["delta", "pyc_clk", "pyc_rst"], output_names = ["result"], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}} : () -> ()
}
