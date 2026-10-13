// RUN: %pycircuit_opt %s | %FileCheck %s
// Native typed-effect verification only. These operations are not final hardware.
// Equal child inputs still denote two distinct state owners.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w2 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<2>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
#w8 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<8>}, origin = {site = {definition = @Counter, ast_path = []}, expansion = []}, location = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
!b8 = !ac.bits<#w8>
!lanes = !ac.table<[#w2], !b8>
#count_state = {site = {definition = @Counter, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#packet_state = {site = {definition = @Counter, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#counter = {site = {definition = @Counter, ast_path = []}, expansion = []}
#left_instance = {site = {definition = @Root, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#right_instance = {site = {definition = @Root, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#root = {site = {definition = @Root, ast_path = []}, expansion = []}
#span = {path = "transactions.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#byte_constraint = {kind = "hardware", type = !b8, source_kind = "fixed_bits"}
module {
  ac.struct "Packet" fields [{name = "lanes", type = !lanes}]
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %seed: !b8, %packet: !ac.struct<"Packet">):
    %initial = "ac.bits.constant"() {value = #w1} : () -> !b8
    %initial_lanes = "ac.table.splat"(%initial) {shape = [#w2]} : (!b8) -> !lanes
    %initial_packet = "ac.struct.create"(%initial_lanes) : (!lanes) -> !ac.struct<"Packet">
    %q = "ac.state"(%clk, %rst, %initial) {instance_name = "count", occurrence = #count_state} : (!b1, !b1, !b8) -> !b8
    %packet_q = "ac.state"(%clk, %rst, %initial_packet) {instance_name = "packet", occurrence = #packet_state} : (!b1, !b1, !ac.struct<"Packet">) -> !ac.struct<"Packet">
    "ac.method"(%rst) ({
    ^bb0(%delta: !b8, %path: !b1):
      %current = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #counter} : (!b1) -> !b8
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%delta: !b8, %path: !b1):
      %old = "ac.state.read"(%path) {state = "count", selectors = [], occurrence = #counter} : (!b1) -> !b8
      %lane = "ac.state.read"(%path, %path) {state = "packet", selectors = [{field = "lanes"}, {index = 0 : i64}], occurrence = #counter} : (!b1, !b1) -> !b8
      %next = "ac.bits.binary"(%old, %delta) {opcode = "add"} : (!b8, !b8) -> !b8
      %pair = "ac.table.splat"(%next) {shape = [#w2]} : (!b8) -> !lanes
      %mapped = "ac.table.map"(%pair) ({
      ^bb0(%ordinal: !b1, %entry: !b8):
        %twice = "ac.bits.binary"(%entry, %entry) {opcode = "add"} : (!b8, !b8) -> !b8
        "ac.yield"(%twice) : (!b8) -> ()
      }) {shape = [#w2], operandSegmentSizes = array<i32: 1, 0>} : (!lanes) -> !lanes
      "ac.state.write"(%path, %next) {state = "count", selectors = [], occurrence = #counter} : (!b1, !b8) -> ()
      "ac.state.write"(%path, %lane, %path) {state = "packet", selectors = [{field = "lanes"}, {index = 0 : i64}], occurrence = #counter} : (!b1, !b8, !b1) -> ()
      "ac.yield"(%old) : (!b8) -> ()
    }) {sym_name = "add", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #counter, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", ac.guard_location = #span, occurrence = #counter} : (!b1) -> ()
    "ac.method"(%rst) ({
    ^bb0(%delta: !b8, %path: !b1):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%delta: !b8, %path: !b1):
      %result = "ac.method.call"(%path, %delta) {callee = @Counter::@add, instance_path = [], occurrence = #counter} : (!b1, !b8) -> !b8
      "ac.yield"(%result) : (!b8) -> ()
    }) {sym_name = "relay", function_type = (!b8) -> !b8, argument_names = ["delta"], ac.parameters = [{name = "delta", binding = "positional_or_keyword", constraint = #byte_constraint, default = {present = false}, origin = #counter, location = #span}], ac.result_constraints = [#byte_constraint], ac.return_form = "single", ac.guard_location = #span, occurrence = #counter} : (!b1) -> ()
    "ac.yield"(%q) : (!b8) -> ()
  }) {sym_name = "Counter", source_owner = {package = "", path = "transactions.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b8, !ac.struct<"Packet">) -> !b8, input_names = ["clk", "rst", "seed", "packet"], output_names = ["count"]} : () -> ()
  "ac.module"() ({
  ^bb0(%clk: !b1, %rst: !b1, %seed: !b8, %packet: !ac.struct<"Packet">):
    %left = "ac.instance"(%clk, %rst, %seed, %packet) {instance_name = "left", callee = @Counter, parameters = [], type_arguments = [], occurrence = #left_instance} : (!b1, !b1, !b8, !ac.struct<"Packet">) -> !b8
    %right = "ac.instance"(%clk, %rst, %seed, %packet) {instance_name = "right", callee = @Counter, parameters = [], type_arguments = [], occurrence = #right_instance} : (!b1, !b1, !b8, !ac.struct<"Packet">) -> !b8
    %answer = "ac.transaction"(%clk, %rst, %rst, %seed) ({
    ^bb0(%path: !b1, %value: !b8):
      "ac.yield"(%path) : (!b1) -> ()
    }, {
    ^bb0(%path: !b1, %value: !b8):
      %a = "ac.method.call"(%path, %value) {callee = @Counter::@relay, instance_path = ["left"], occurrence = #root} : (!b1, !b8) -> !b8
      %again = "ac.method.call"(%path, %value) {callee = @Counter::@add, instance_path = ["left"], occurrence = #root} : (!b1, !b8) -> !b8
      %b = "ac.method.call"(%path, %value) {callee = @Counter::@add, instance_path = ["right"], occurrence = #root} : (!b1, !b8) -> !b8
      "ac.yield"(%b) : (!b8) -> ()
    }) {name = "step", declaration = #root, occurrence = #root, ac.guard_location = #span} : (!b1, !b1, !b1, !b8) -> !b8
    "ac.yield"(%answer) : (!b8) -> ()
  }) {sym_name = "Root", source_owner = {package = "", path = "transactions.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b8, !ac.struct<"Packet">) -> !b8, input_names = ["clk", "rst", "seed", "packet"], output_names = ["answer"]} : () -> ()
  "ac.system"() {entry = {callee = @Root, parameters = [], type_arguments = []}, domain = "default"} : () -> ()
}

// CHECK: "ac.state"
// CHECK-SAME: instance_name = "count"
// CHECK: "ac.method"
// CHECK: selectors = [{field = "lanes"}, {index = 0 : i64}]
// CHECK: instance_path = []
// CHECK: instance_name = "left"
// CHECK: instance_name = "right"
// CHECK: "ac.transaction"
// CHECK: instance_path = ["left"]
// CHECK: instance_path = ["right"]
