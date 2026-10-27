// RUN: %pycircuit_opt %s -o %t.once.mlir
// RUN: %pycircuit_opt %t.once.mlir -o %t.twice.mlir
// RUN: diff %t.once.mlir %t.twice.mlir
// RUN: %pycircuit_opt --cse %s | %FileCheck %s
// Structural identity checks; this is not an execution or storage-lowering test.
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
!b8 = !ac.bits<#w8>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %init = "ac.bits.constant4"() {pattern = "10xz0011"} : () -> !b8
    %a = "ac.resource.alloc"(%init) {instance_name = "a"} : (!b8) -> !s
    %b = "ac.resource.alloc"(%init) {instance_name = "b"} : (!b8) -> !s
    "ac.yield"(%a, %a) : (!s, !s) -> ()
  }) {sym_name = "Leaf", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!s, !s), input_names = [], output_names = ["out0", "out1"], output_kinds = ["export", "export"]} : () -> ()
  "ac.module"() ({
  ^bb0(%destination: !q):
    "ac.yield"(%destination) : (!q) -> ()
  }) {sym_name = "Bound", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!q), input_names = [], output_names = ["out0"], output_kinds = ["bound"]} : () -> ()
  "ac.module"() ({
    %q2 = "ac.resource.alloc"() {instance_name = "q2", depth = 2 : i64} : () -> !q
    %q5 = "ac.resource.alloc"() {instance_name = "q5", depth = 5 : i64} : () -> !q
    %bound = "ac.instance"(%q2) {instance_name = "bound", callee = @Bound, parameters = [], type_arguments = [], occurrence = {}} : (!q) -> (!q)
    %left:2 = "ac.instance"() {instance_name = "left", callee = @Leaf, parameters = [], type_arguments = [], occurrence = {}} : () -> (!s, !s)
    %right:2 = "ac.instance"() {instance_name = "right", callee = @Leaf, parameters = [], type_arguments = [], occurrence = {}} : () -> (!s, !s)
    %lr = "ac.instance.ref"() {instance_path = ["left"]} : () -> !ac.module_ref<@Leaf>
    "ac.yield"(%bound, %q5, %left#0, %right#0) : (!q, !q, !s, !s) -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!q, !q, !s, !s), input_names = [], output_names = ["out0", "out1", "out2", "out3"], output_kinds = ["export", "export", "export", "export"]} : () -> ()
}
// CHECK: %[[FIRST:[a-zA-Z0-9_]+]] = ac.resource.alloc "a"
// CHECK: ac.resource.alloc "b"
// CHECK: "ac.yield"(%[[FIRST]], %[[FIRST]])
// CHECK: ac.resource.alloc "q2" depth 2
// CHECK: ac.resource.alloc "q5" depth 5
// CHECK: ac.instance "bound" @Bound inputs() bound(%{{[a-zA-Z0-9_]+}})
