// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics

// The native grammar rejections of the transaction effect family. The
// staged-tier rejections of the same family live in
// transaction-surface-invalid.mlir under --ac-verify-hardware.

// Occurrence dictionaries must have the complete structural identity.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!lanes = !ac.table<[#w2], !b8>
#counter = {site = {definition = @Counter, ast_path = []}, expansion = []}
#root = {site = {definition = @Root, ast_path = []}, expansion = []}
#span = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#byte_constraint = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
module {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %data: !b8):
    %initial = "ac.bits.constant"() {value = #w1} : () -> !b8
// expected-error @+1 {{Occurrence must contain exactly 2 fields}}
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = {}} : (!b1, !b1, !b8) -> !b8
    "ac.method"(%rst) ({
    ^bb0(%value: !b8, %path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%value: !b8, %path: !b1):
      %old = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #counter} : (!b1) -> !b8
      "ac.state.write"(%path, %value) {state = "count", selectors = [], occurrence = #counter} : (!b1, !b8) -> ()
      "ac.yield"(%old) : (!b8) -> ()
    }) {sym_name = "add", function_type = (!b8) -> !b8, argument_names = ["value"], ac.parameters = [{name = "value", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #counter, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", ac.guard_location = #span, occurrence = #counter} : (!b1) -> ()
    "ac.yield"(%q) : (!b8) -> ()
  }) {sym_name = "Counter", source_owner = {package = "", path = "transactions.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b8) -> !b8, input_names = ["clk", "rst", "data"], output_names = ["out"]} : () -> ()
}


// -----

// Guard writes reject before transaction lowering.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!lanes = !ac.table<[#w2], !b8>
#counter = {site = {definition = @Counter, ast_path = []}, expansion = []}
#root = {site = {definition = @Root, ast_path = []}, expansion = []}
#span = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#byte_constraint = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
module {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %data: !b8):
    %initial = "ac.bits.constant"() {value = #w1} : () -> !b8
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #counter} : (!b1, !b1, !b8) -> !b8
    "ac.method"(%rst) ({
    ^bb0(%value: !b8, %path: !b1):
// expected-error @+1 {{state writes and method calls are forbidden in guards}}
      "ac.state.write"(%path, %value) {state = "count", selectors = [], occurrence = #counter} : (!b1, !b8) -> ()
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%value: !b8, %path: !b1):
      %old = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #counter} : (!b1) -> !b8
      "ac.state.write"(%path, %value) {state = "count", selectors = [], occurrence = #counter} : (!b1, !b8) -> ()
      "ac.yield"(%old) : (!b8) -> ()
    }) {sym_name = "add", function_type = (!b8) -> !b8, argument_names = ["value"], ac.parameters = [{name = "value", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #counter, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", ac.guard_location = #span, occurrence = #counter} : (!b1) -> ()
    "ac.yield"(%q) : (!b8) -> ()
  }) {sym_name = "Counter", source_owner = {package = "", path = "transactions.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b8) -> !b8, input_names = ["clk", "rst", "data"], output_names = ["out"]} : () -> ()
}


// -----

// Ordinary returns never coerce to the method result type.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!lanes = !ac.table<[#w2], !b8>
#counter = {site = {definition = @Counter, ast_path = []}, expansion = []}
#root = {site = {definition = @Root, ast_path = []}, expansion = []}
#span = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#byte_constraint = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
module {
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %data: !b8):
    %initial = "ac.bits.constant"() {value = #w1} : () -> !b8
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #counter} : (!b1, !b1, !b8) -> !b8
    "ac.method"(%rst) ({
    ^bb0(%value: !b8, %path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%value: !b8, %path: !b1):
      %old = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #counter} : (!b1) -> !b8
      "ac.state.write"(%path, %value) {state = "count", selectors = [], occurrence = #counter} : (!b1, !b8) -> ()
      // expected-error @+1 {{body yield types do not match results}}
      "ac.yield"(%path) : (!b1) -> ()
    }) {sym_name = "add", function_type = (!b8) -> !b8, argument_names = ["value"], ac.parameters = [{name = "value", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #counter, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", ac.guard_location = #span, occurrence = #counter} : (!b1) -> ()
    "ac.yield"(%q) : (!b8) -> ()
  }) {sym_name = "Counter", source_owner = {package = "", path = "transactions.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b8) -> !b8, input_names = ["clk", "rst", "data"], output_names = ["out"]} : () -> ()
}

