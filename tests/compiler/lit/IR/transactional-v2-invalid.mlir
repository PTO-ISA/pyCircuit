// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics

// resource arithmetic
// expected-error @+10 {{bits}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !s
    %sum = "ac.bits.binary"(%s, %s) {opcode = "add"} : (!s, !s) -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// bound result replaced
// expected-error @+11 {{bound}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
  ^bb0(%destination: !q):
    %own = "ac.resource.alloc"() {instance_name = "own", depth = 2 : i64} : () -> !q
    "ac.yield"(%own) : (!q) -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!q), input_names = [], output_names = ["out0"], output_kinds = ["bound"]} : () -> ()
}

// -----

// unresolved primitive linked
// expected-error @+8 {{primitive}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
  ^bb0(%p: !ac.primitive<!b8>):
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = (!ac.primitive<!b8>) -> (), input_names = ["in0"], output_names = [], output_kinds = []} : () -> ()
}
