// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics

// bound result self cycle
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
  ^bb0(%destination: !q):
    "ac.yield"(%destination) : (!q) -> ()
  }) {sym_name = "Bound", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!q), input_names = [], output_names = ["out0"], output_kinds = ["bound"]} : () -> ()
  // expected-error @+1 {{resource alias cycle has no existing allocation or formal owner}}
  "ac.module"() ({
    %cycle = "ac.instance"(%cycle) {instance_name = "cycle", callee = @Bound, parameters = [], type_arguments = [], occurrence = {}} : (!q) -> (!q)
    "ac.yield"() : () -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// cross instance export cycle
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
  ^bb0(%input: !q):
    "ac.yield"(%input) : (!q) -> ()
  }) {sym_name = "Pass", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = (!q) -> (!q), input_names = ["in0"], output_names = ["out0"], output_kinds = ["export"]} : () -> ()
  // expected-error @+1 {{resource alias cycle has no existing allocation or formal owner}}
  "ac.module"() ({
    %left = "ac.instance"(%right) {instance_name = "left", callee = @Pass, parameters = [], type_arguments = [], occurrence = {}} : (!q) -> (!q)
    %right = "ac.instance"(%left) {instance_name = "right", callee = @Pass, parameters = [], type_arguments = [], occurrence = {}} : (!q) -> (!q)
    "ac.yield"() : () -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}
