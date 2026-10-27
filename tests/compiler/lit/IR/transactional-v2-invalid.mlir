// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics

// primitive allocation
// expected-error @+9 {{primitive}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "unlinked"} {
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !ac.primitive<!b8>
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// missing queue depth
// expected-error @+9 {{depth}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %q = "ac.resource.alloc"() {instance_name = "q"} : () -> !q
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// zero queue depth
// expected-error @+9 {{depth}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %q = "ac.resource.alloc"() {instance_name = "q", depth = 0 : i64} : () -> !q
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// negative queue depth
// expected-error @+9 {{depth}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %q = "ac.resource.alloc"() {instance_name = "q", depth = -3 : i64} : () -> !q
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// state depth
// expected-error @+9 {{depth}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s", depth = 2 : i64} : () -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// queue initializer
// expected-error @+10 {{initial}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %c = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
    %q = "ac.resource.alloc"(%c) {instance_name = "q", depth = 2 : i64} : (!b8) -> !q
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// state initializer type mismatch
// expected-error @+10 {{initial}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %c = "ac.bits.constant4"() {pattern = "1"} : () -> !b1
    %s = "ac.resource.alloc"(%c) {instance_name = "s"} : (!b1) -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// open initializer
// expected-error @+10 {{closed}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
  ^bb0(%input: !b8):
    %s = "ac.resource.alloc"(%input) {instance_name = "s"} : (!b8) -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = (!b8) -> (), input_names = ["in0"], output_names = [], output_kinds = []} : () -> ()
}

// -----

// duplicate owned name
// expected-error @+9 {{name}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %a = "ac.resource.alloc"() {instance_name = "a"} : () -> !s
    %b = "ac.resource.alloc"() {instance_name = "a"} : () -> !s
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

// missing bound argument
// expected-error @+8 {{argument}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %own = "ac.resource.alloc"() {instance_name = "own", depth = 2 : i64} : () -> !q
    "ac.yield"(%own) : (!q) -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!q), input_names = [], output_names = ["out0"], output_kinds = ["bound"]} : () -> ()
}

// -----

// missing output kinds
// expected-error @+8 {{output_kinds}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %own = "ac.resource.alloc"() {instance_name = "own"} : () -> !s
    "ac.yield"(%own) : (!s) -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!s), input_names = [], output_names = ["out0"], output_kinds = []} : () -> ()
}

// -----

// invalid output kind
// expected-error @+8 {{output_kinds}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %own = "ac.resource.alloc"() {instance_name = "own"} : () -> !s
    "ac.yield"(%own) : (!s) -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!s), input_names = [], output_names = ["out0"], output_kinds = ["copy"]} : () -> ()
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

// -----

// unresolved instance
// expected-error @+9 {{definition}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    "ac.instance"() {instance_name = "absent", callee = @Missing, parameters = [], type_arguments = [], occurrence = {}} : () -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// wrong reference path
// expected-error @+9 {{path}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %r = "ac.instance.ref"() {instance_path = ["absent"]} : () -> !ac.module_ref<@Bad>
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// empty reference path
// expected-error @+9 {{path}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %r = "ac.instance.ref"() {instance_path = []} : () -> !ac.module_ref<@Bad>
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// legacy version
// expected-error @+7 {{version}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 1 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// wrong version width
// expected-error @+7 {{version}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i32, ac.stage = "linked"} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// missing version
// expected-error @+7 {{transactional}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.stage = "linked"} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// wrong stage
// expected-error @+7 {{stage}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "final"} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// unsupported Method requirement
// expected-error @+7 {{unsupported}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked", ac.method_requirements = []} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// legacy output bindings
// expected-error @+7 {{unsupported}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked", ac.output_bindings = []} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

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

// resource as storage payload
// expected-error @+9 {{payload}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !ac.state<!s>
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// resource in ordinary struct
// expected-error @+9 {{finite bits/struct/enum/table types}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.enum"() {sym_name = "Mode", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}, {name = "ON", code = #ac.math_int<1>}]} : () -> ()
  "ac.struct"() {sym_name = "Packet", fields = [{name = "mode", type = !ac.enum<"Mode">}, {name = "data", type = !s}]} : () -> ()
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// unsupported Rule
// expected-error @+9 {{not supported}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    "ac.rule"() ({}) {name = "rule", occurrence = {}} : () -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// instance cycle
// expected-error @+8 {{recurs}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    "ac.instance"() {instance_name = "self", callee = @Bad, parameters = [], type_arguments = [], occurrence = {}} : () -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// cross module direct use
// expected-error @+13 {{undeclared}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "First", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
  "ac.module"() ({
    "ac.yield"(%s) : (!s) -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!s), input_names = [], output_names = ["out0"], output_kinds = ["export"]} : () -> ()
}

// -----

// linked import
// expected-error @+8 {{import}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module.import"() {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!s) -> (!s), input_names = ["input"], output_names = ["output"], output_kinds = ["export"], semantic_kind = "resources"} : () -> ()
}

// -----

// instance input mismatch
// expected-error @+14 {{do not match the interface}}
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
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !s
    %child = "ac.instance"(%s) {instance_name = "child", callee = @Bound, parameters = [], type_arguments = [], occurrence = {}} : (!s) -> (!q)
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// malformed alloc result count
// expected-error @+9 {{result}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    "ac.resource.alloc"() {instance_name = "bad"} : () -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// State multiple initializers
// expected-error @+10 {{at most one}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %c = "ac.bits.constant4"() {pattern = "00000001"} : () -> !b8
    %s = "ac.resource.alloc"(%c, %c) {instance_name = "s"} : (!b8, !b8) -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// reference wrong declared module
// expected-error @+13 {{path}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Child", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
  "ac.module"() ({
    "ac.instance"() {instance_name = "child", callee = @Child, parameters = [], type_arguments = [], occurrence = {}} : () -> ()
    %r = "ac.instance.ref"() {instance_path = ["child"]} : () -> !ac.module_ref<@Bad>
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// missing stage
// expected-error @+7 {{stage}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// unknown stage
// expected-error @+7 {{stage}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "unknown"} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// resource without version or stage
// expected-error @+9 {{transactional}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {} {
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = []} : () -> ()
}

// -----

// legacy final resource
// expected-error @+9 {{transactional}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.stage = "final"} {
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = []} : () -> ()
}

// -----

// legacy routed resource
// expected-error @+9 {{transactional}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.stage = "routed"} {
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = []} : () -> ()
}

// -----

// legacy source resource
// expected-error @+9 {{transactional}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.stage = "source"} {
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = []} : () -> ()
}

// -----

// unsupported Method operation
// expected-error @+9 {{not supported}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    "ac.method"() ({}, {}) {sym_name = "method", function_type = () -> (), argument_names = [], occurrence = {}} : () -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// unsupported transaction operation
// expected-error @+10 {{not supported}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %c = "ac.bits.constant4"() {pattern = "0"} : () -> !b1
    "ac.transaction"(%c, %c) ({}, {}) {name = "transaction", declaration = {}, occurrence = {}} : (!b1, !b1) -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// malformed module region count
// expected-error @+8 {{region}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// allocation outside module
// expected-error @+8 {{declaration}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !s
}

// -----

// cyclic initializer is not a closed expression
// expected-error @+10 {{closed pure expression}}
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    %cycle = "ac.bits.unary"(%cycle) {opcode = "not"} : (!b8) -> !b8
    %s = "ac.resource.alloc"(%cycle) {instance_name = "s"} : (!b8) -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// Custom syntax cannot disguise a bound resource as an ordinary input.
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
  "ac.module"() ({
    %queue = "ac.resource.alloc"() {instance_name = "queue", depth = 2 : i64} : () -> !q
    // expected-error @+1 {{printed bound operand group must match the callee bound output signature}}
    %bound = ac.instance "bound" @Bound inputs(%queue) bound() {parameters = [], type_arguments = [], occurrence = {}} : (!q) -> !q
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// Unsupported bare package contract field
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
// expected-error @+1 {{unsupported transactional contract field}}
module attributes {ac.transactional = 2 : i64, ac.stage = "linked", method_requirements = []} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// Unsupported bare package contract field
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
// expected-error @+1 {{unsupported transactional contract field}}
module attributes {ac.transactional = 2 : i64, ac.stage = "linked", methods = []} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// Unsupported bare package contract field
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
// expected-error @+1 {{unsupported transactional contract field}}
module attributes {ac.transactional = 2 : i64, ac.stage = "linked", output_kinds = []} {
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// Output interface metadata cannot be attached to allocations.
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.module"() ({
    // expected-error @+1 {{unsupported transactional contract field}}
    %s = "ac.resource.alloc"() {instance_name = "s", output_kinds = []} : () -> !s
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// Unsupported bare module Method contract field
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  // expected-error @+1 {{unsupported transactional contract field}}
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", method_requirements = [], source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// Unsupported bare module Method contract field
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  // expected-error @+1 {{unsupported transactional contract field}}
  "ac.module"() ({
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", methods = [], source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}

// -----

// Malformed operand segment sizes must diagnose before table operand access.
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!table = !ac.table<[#w1], !b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "linked"} {
  "ac.enum"() {sym_name = "Mode", width = #ac.math_int<1>, encoding = "explicit", members = [{name = "OFF", code = #ac.math_int<0>}, {name = "ON", code = #ac.math_int<1>}]} : () -> ()
  "ac.struct"() {sym_name = "Packet", fields = [{name = "mode", type = !ac.enum<"Mode">}, {name = "data", type = !b8}]} : () -> ()
  "ac.module"() ({
    %mode = "ac.enum.create"() {member = "ON"} : () -> !ac.enum<"Mode">
    %data = "ac.bits.constant4"() {pattern = "01xz1100"} : () -> !b8
    %packet = "ac.struct.create"(%mode, %data) : (!ac.enum<"Mode">, !b8) -> !ac.struct<"Packet">
    %enum = "ac.resource.alloc"(%mode) {instance_name = "enum"} : (!ac.enum<"Mode">) -> !ac.state<!ac.enum<"Mode">>
    %packet_state = "ac.resource.alloc"(%packet) {instance_name = "packet_state"} : (!ac.struct<"Packet">) -> !ac.state<!ac.struct<"Packet">>
    %inverse = "ac.bits.unary"(%data) {opcode = "not"} : (!b8) -> !b8
    %computed = "ac.resource.alloc"(%inverse) {instance_name = "computed"} : (!b8) -> !s
    %table = "ac.table.splat"(%data) {shape = [#w1]} : (!b8) -> !table
    // expected-error @+1 {{operand}}
    %mapped = "ac.table.map"(%table) ({
    ^bb0(%ordinal: !b1, %entry: !b8):
      %mapped_value = "ac.bits.binary"(%entry, %entry) {opcode = "add"} : (!b8, !b8) -> !b8
      "ac.yield"(%mapped_value) : (!b8) -> ()
    }) {shape = [#w1], operandSegmentSizes = array<i32: 2, 0>} : (!table) -> !table
    %mapped_state = "ac.resource.alloc"(%mapped) {instance_name = "mapped_state"} : (!table) -> !ac.state<!table>
    %table_state = "ac.resource.alloc"(%table) {instance_name = "table_state"} : (!table) -> !ac.state<!table>
    %unspecified = "ac.resource.alloc"() {instance_name = "unspecified"} : () -> !s
    "ac.yield"(%enum, %packet_state, %unspecified) : (!ac.state<!ac.enum<"Mode">>, !ac.state<!ac.struct<"Packet">>, !s) -> ()
  }) {sym_name = "Payloads", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (!ac.state<!ac.enum<"Mode">>, !ac.state<!ac.struct<"Packet">>, !s), input_names = [], output_names = ["out0", "out1", "out2"], output_kinds = ["export", "export", "export"]} : () -> ()
}

// -----

// PR-01 verifies explicit exact bindings; specialization belongs to PR-07.
#w8 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "resources.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Leaf, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
!b8 = !ac.bits<#w8>
!b1 = !ac.bits<#w1>
!s = !ac.state<!b8>
!q = !ac.queue<!b8>
module attributes {ac.transactional = 2 : i64, ac.stage = "unlinked"} {
  "ac.module.import"() {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!ac.primitive<!b8>) -> (), input_names = ["input"], output_names = [], output_kinds = [], semantic_kind = "resources"} : () -> ()
  "ac.module"() ({
    %s = "ac.resource.alloc"() {instance_name = "s"} : () -> !s
    // expected-error @+1 {{do not match the interface}}
    "ac.instance"(%s) {instance_name = "provider", callee = @Provider, parameters = [], type_arguments = [], occurrence = {}} : (!s) -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "Bad", source_owner = {package = "", path = "resources.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = [], output_kinds = []} : () -> ()
}
