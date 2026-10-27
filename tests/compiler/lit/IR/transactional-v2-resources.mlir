// RUN: %pycircuit_opt %s -o %t.once.mlir
// RUN: %pycircuit_opt %t.once.mlir -o %t.twice.mlir
// RUN: diff %t.once.mlir %t.twice.mlir
// RUN: %FileCheck %s --check-prefix=IDENTITY < %t.twice.mlir
// RUN: %pycircuit_opt --cse %s | %FileCheck %s
// Structural identity checks; this is not an execution or storage-lowering test.
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
  ^bb0(%state: !s, %first: !q, %second: !q):
    "ac.yield"(%first, %state, %second) : (!q, !s, !q) -> ()
  }) {sym_name = "Mixed", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = (!s) -> (!q, !s, !q), input_names = ["in0"], output_names = ["out0", "out1", "out2"], output_kinds = ["bound", "export", "bound"]} : () -> ()
  "ac.module"() ({
    %state = "ac.resource.alloc"() {instance_name = "state"} : () -> !s
    %first = "ac.resource.alloc"() {instance_name = "first", depth = 3 : i64} : () -> !q
    %second = "ac.resource.alloc"() {instance_name = "second", depth = 7 : i64} : () -> !q
    %mixed:3 = "ac.instance"(%state, %first, %second) {instance_name = "mixed", callee = @Mixed, parameters = [], type_arguments = [], occurrence = {}} : (!s, !q, !q) -> (!q, !s, !q)
    "ac.yield"(%mixed#0, %mixed#1, %mixed#2) : (!q, !s, !q) -> ()
  }) {sym_name = "MixedUse", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!q, !s, !q), input_names = [], output_names = ["out0", "out1", "out2"], output_kinds = ["export", "export", "export"]} : () -> ()
  "ac.module"() ({
    %init = "ac.bits.constant4"() {pattern = "10xz0011"} : () -> !b8
    %a = "ac.resource.alloc"(%init) {instance_name = "a"} : (!b8) -> !s
    %b = "ac.resource.alloc"(%init) {instance_name = "b"} : (!b8) -> !s
    %unused = "ac.resource.alloc"(%init) {instance_name = "unused"} : (!b8) -> !s
    "ac.yield"(%a, %a, %b) : (!s, !s, !s) -> ()
  }) {sym_name = "Leaf", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!s, !s, !s), input_names = [], output_names = ["out0", "out1", "out2"], output_kinds = ["export", "export", "export"]} : () -> ()
  "ac.module"() ({
  ^bb0(%destination: !q):
    "ac.yield"(%destination) : (!q) -> ()
  }) {sym_name = "Bound", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!q), input_names = [], output_names = ["out0"], output_kinds = ["bound"]} : () -> ()
  "ac.module"() ({
    %child:3 = "ac.instance"() {instance_name = "child", callee = @Leaf, parameters = [], type_arguments = [], occurrence = {}} : () -> (!s, !s, !s)
    "ac.yield"(%child#0, %child#1, %child#2) : (!s, !s, !s) -> ()
  }) {sym_name = "Forward", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!s, !s, !s), input_names = [], output_names = ["out0", "out1", "out2"], output_kinds = ["export", "export", "export"]} : () -> ()
  "ac.module"() ({
  ^bb0(%left: !ac.module_ref<@Leaf>, %right: !ac.module_ref<@Leaf>):
    "ac.yield"() : () -> ()
  }) {sym_name = "References", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = (!ac.module_ref<@Leaf>, !ac.module_ref<@Leaf>) -> (), input_names = ["in0", "in1"], output_names = [], output_kinds = []} : () -> ()
  "ac.module"() ({
    %q2 = "ac.resource.alloc"() {instance_name = "q2", depth = 2 : i64} : () -> !q
    %q5 = "ac.resource.alloc"() {instance_name = "q5", depth = 5 : i64} : () -> !q
    %bound = "ac.instance"(%q2) {instance_name = "bound", callee = @Bound, parameters = [], type_arguments = [], occurrence = {}} : (!q) -> (!q)
    %left:3 = "ac.instance"() {instance_name = "left", callee = @Forward, parameters = [], type_arguments = [], occurrence = {}} : () -> (!s, !s, !s)
    %right:3 = "ac.instance"() {instance_name = "right", callee = @Forward, parameters = [], type_arguments = [], occurrence = {}} : () -> (!s, !s, !s)
    %lr = "ac.instance.ref"() {instance_path = ["left", "child"]} : () -> !ac.module_ref<@Leaf>
    %rr = "ac.instance.ref"() {instance_path = ["right", "child"]} : () -> !ac.module_ref<@Leaf>
    "ac.instance"(%lr, %rr) {instance_name = "references", callee = @References, parameters = [], type_arguments = [], occurrence = {}} : (!ac.module_ref<@Leaf>, !ac.module_ref<@Leaf>) -> ()
    "ac.yield"(%bound, %q5, %left#0, %right#0) : (!q, !q, !s, !s) -> ()
  }) {sym_name = "Top", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!q, !q, !s, !s), input_names = [], output_names = ["out0", "out1", "out2", "out3"], output_kinds = ["export", "export", "export", "export"]} : () -> ()
}
// CHECK: ac.resource.alloc "a" initial(
// CHECK: ac.resource.alloc "b" initial(
// CHECK: ac.resource.alloc "unused" initial(
// CHECK: output_kinds = ["bound"]
// CHECK: ac.resource.alloc "q2" depth 2
// CHECK: ac.resource.alloc "q5" depth 5
// CHECK: ac.instance "left" @Forward inputs() bound()
// CHECK: ac.instance "right" @Forward inputs() bound()
// IDENTITY: %[[FIRST:[a-zA-Z0-9_]+]] = ac.resource.alloc "a"
// IDENTITY: %[[SECOND:[a-zA-Z0-9_]+]] = ac.resource.alloc "b"
// IDENTITY: "ac.yield"(%[[FIRST]], %[[FIRST]], %[[SECOND]])
// IDENTITY: ac.instance "bound" @Bound inputs() bound(%{{[a-zA-Z0-9_]+}})
// IDENTITY: ac.instance.ref ["left", "child"]
// IDENTITY: ac.instance.ref ["right", "child"]
// CHECK: ac.instance.ref ["left", "child"]
// CHECK: ac.instance.ref ["right", "child"]
