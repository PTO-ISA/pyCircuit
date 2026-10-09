// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics
// Schema/type cases must fail at native constraints; semantic cases must
// reach the named Enum owning guard.
// This fixture does not claim source, common layout, storage or backend support.

// Zero width.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum width must be positive and fit u64}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<0>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
}

// -----
// Negative width.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum width must be positive and fit u64}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<-1>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
}

// -----
// Width beyond finite packed layout.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum width must be positive and fit u64}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<18446744073709551616>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
}

// -----
// Boolean width is not MathInt.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{attribute 'width' failed to satisfy constraint}}
  "ac.enum"() {sym_name = "pkg.State", width = true, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
}

// -----
// Empty members.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum requires at least one member}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = []} : () -> ()
}

// -----
// Unknown encoding.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{unknown enum encoding}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "guessed", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
}

// -----
// Empty member name.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum members require exactly a nonempty name and mathematical integer code}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "", code = #ac.math_int<0>}]} : () -> ()
}

// -----
// Duplicate member name.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum member names must be unique}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "SAME", code = #ac.math_int<0>}, {name = "SAME", code = #ac.math_int<1>}]} : () -> ()
}

// -----
// Duplicate physical code.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum member codes must be unique}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "FIRST", code = #ac.math_int<1>}, {name = "SECOND", code = #ac.math_int<1>}]} : () -> ()
}

// -----
// Negative member code.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum member code must be nonnegative}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "NEGATIVE", code = #ac.math_int<-1>}]} : () -> ()
}

// -----
// Out-of-width member code.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum member code does not fit declared width}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "OVER", code = #ac.math_int<8>}]} : () -> ()
}

// -----
// Arbitrary-precision out-of-width code.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum member code does not fit declared width}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<130>, encoding = "explicit", members = [{name = "OVER", code = #ac.math_int<1361129467683753853853498429727072845824>}]} : () -> ()
}

// -----
// Boolean member code.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum members require exactly a nonempty name and mathematical integer code}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "FALSE", code = false}]} : () -> ()
}

// -----
// Missing member code.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum members require exactly a nonempty name and mathematical integer code}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "NONE"}]} : () -> ()
}

// -----
// Extra member field.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum members require exactly a nonempty name and mathematical integer code}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "EXTRA", code = #ac.math_int<0>, hint = true}]} : () -> ()
}

// -----
// Derived encoding codes are recomputed: binary_sequential.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum member code does not match derived encoding}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "binary_sequential", members = [{name = "A", code = #ac.math_int<0>}, {name = "B", code = #ac.math_int<2>}]} : () -> ()
}

// -----
// Derived encoding codes are recomputed: gray_sequential.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum member code does not match derived encoding}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "gray_sequential", members = [{name = "A", code = #ac.math_int<0>}, {name = "B", code = #ac.math_int<1>}, {name = "C", code = #ac.math_int<2>}]} : () -> ()
}

// -----
// Derived encoding codes are recomputed: binary_one_hot.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum member code does not match derived encoding}}
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "binary_one_hot", members = [{name = "A", code = #ac.math_int<1>}, {name = "B", code = #ac.math_int<3>}]} : () -> ()
}

// -----
// Empty nominal type name.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{enum type requires a nonempty nominal name}}
  %bad = "ac.enum.create"() {member = "IDLE"} : () -> !ac.enum<"">
}

// -----
// Unknown declaration.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{cannot resolve nominal enum declaration}}
  %state = "ac.enum.create"() {member = "IDLE"} : () -> !ac.enum<"missing.State">
}

// -----
// Unknown declared member.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  // expected-error @+1 {{unknown enum member}}
  %state = "ac.enum.create"() {member = "MISSING"} : () -> !ac.enum<"pkg.State">
}

// -----
// To-bits result width differs.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
!b5 = !ac.bits<#w5>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  %state = "ac.enum.create"() {member = "IDLE"} : () -> !ac.enum<"pkg.State">
  // expected-error @+1 {{enum.to_bits result width must equal 3}}
  %bad = "ac.enum.to_bits"(%state) : (!ac.enum<"pkg.State">) -> !b5
}

// -----
// From-bits input width differs.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
#w5 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<5>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
!b5 = !ac.bits<#w5>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  %raw = "ac.bits.constant"() {value = #w0} : () -> !b5
  // expected-error @+1 {{enum.from_bits input width must equal 3}}
  %bad, %member = "ac.enum.from_bits"(%raw) : (!b5) -> (!ac.enum<"pkg.State">, !b1)
}

// -----
// Membership must be one bit.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w2 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<2>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b2 = !ac.bits<#w2>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  %raw = "ac.bits.constant"() {value = #w0} : () -> !b3
  // expected-error @+1 {{enum.from_bits membership result width must equal 1}}
  %bad, %member = "ac.enum.from_bits"(%raw) : (!b3) -> (!ac.enum<"pkg.State">, !b2)
}

// -----
// Create cannot return bits.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  // expected-error @+1 {{result #0 must be}}
  %bad = "ac.enum.create"() {member = "IDLE"} : () -> !b3
}

// -----
// To-bits cannot accept raw bits.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  %raw = "ac.bits.constant"() {value = #w0} : () -> !b3
  // expected-error @+1 {{operand #0 must be}}
  %bad = "ac.enum.to_bits"(%raw) : (!b3) -> !b3
}

// -----
// To-bits cannot produce an enum.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  %state = "ac.enum.create"() {member = "IDLE"} : () -> !ac.enum<"pkg.State">
  // expected-error @+1 {{result #0 must be}}
  %bad = "ac.enum.to_bits"(%state) : (!ac.enum<"pkg.State">) -> !ac.enum<"pkg.State">
}

// -----
// From-bits cannot accept an enum implicitly.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  %state = "ac.enum.create"() {member = "IDLE"} : () -> !ac.enum<"pkg.State">
  // expected-error @+1 {{operand #0 must be}}
  %bad, %member = "ac.enum.from_bits"(%state) : (!ac.enum<"pkg.State">) -> (!ac.enum<"pkg.State">, !b1)
}

// -----
// From-bits cannot accept builtin integer.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  %integer = "arith.constant"() {value = 0 : i3} : () -> i3
  // expected-error @+1 {{operand #0 must be}}
  %bad, %member = "ac.enum.from_bits"(%integer) : (i3) -> (!ac.enum<"pkg.State">, !b1)
}

// -----
// Membership cannot be a nominal carrier.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  %raw = "ac.bits.constant"() {value = #w0} : () -> !b3
  // expected-error @+1 {{result #1 must be}}
  %bad, %member = "ac.enum.from_bits"(%raw) : (!b3) -> (!ac.enum<"pkg.State">, !ac.enum<"pkg.State">)
}

// -----
// Bits arithmetic cannot erase Enum nominal identity.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  %state = "ac.enum.create"() {member = "IDLE"} : () -> !ac.enum<"pkg.State">
  // expected-error @+1 {{operand #0 must be}}
  %bad = "ac.bits.binary"(%state, %state) {opcode = "add"} : (!ac.enum<"pkg.State">, !ac.enum<"pkg.State">) -> !b3
}

// -----
// Bits comparison cannot accept Enum directly.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "pkg.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  %state = "ac.enum.create"() {member = "IDLE"} : () -> !ac.enum<"pkg.State">
  // expected-error @+1 {{operand #0 must be}}
  %bad = "ac.bits.compare"(%state, %state) {predicate = "eq"} : (!ac.enum<"pkg.State">, !ac.enum<"pkg.State">) -> !b1
}

// -----
// Equal layout does not change qualified SSA type identity.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.enum"() {sym_name = "first.protocol.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  "ac.enum"() {sym_name = "second.protocol.State", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}, {name = "RUN", code = #ac.math_int<7>}]} : () -> ()
  // expected-note @+1 {{prior use here}}
  %first = "ac.enum.create"() {member = "RUN"} : () -> !ac.enum<"first.protocol.State">
  // expected-error @+1 {{use of value '%first' expects different type than prior uses}}
  %bad = "ac.enum.to_bits"(%first) : (!ac.enum<"second.protocol.State">) -> !b3
}

// -----
// Create on unbound T.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.module"() ({
  ^bb0(%value: !ac.type_param<@generic, "T">, %raw: !b3):
      // expected-error @+1 {{result #0 must be}}
      %bad = "ac.enum.create"() {member = "IDLE"} : () -> !ac.type_param<@generic, "T">
      "ac.yield"() : () -> ()
  }) {sym_name = "generic", source_owner = {package = "", path = "generic.py"}, parameters = [], type_parameters = ["T"], function_type = (!ac.type_param<@generic, "T">, !b3) -> (), input_names = ["value", "raw"], output_names = []} : () -> ()
}

// -----
// To-bits on unbound T.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.module"() ({
  ^bb0(%value: !ac.type_param<@generic, "T">, %raw: !b3):
      // expected-error @+1 {{operand #0 must be}}
      %bad = "ac.enum.to_bits"(%value) : (!ac.type_param<@generic, "T">) -> !b3
      "ac.yield"() : () -> ()
  }) {sym_name = "generic", source_owner = {package = "", path = "generic.py"}, parameters = [], type_parameters = ["T"], function_type = (!ac.type_param<@generic, "T">, !b3) -> (), input_names = ["value", "raw"], output_names = []} : () -> ()
}

// -----
// From-bits into unbound T.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.module"() ({
  ^bb0(%value: !ac.type_param<@generic, "T">, %raw: !b3):
      // expected-error @+1 {{result #0 must be}}
      %bad, %member = "ac.enum.from_bits"(%raw) : (!b3) -> (!ac.type_param<@generic, "T">, !b1)
      "ac.yield"() : () -> ()
  }) {sym_name = "generic", source_owner = {package = "", path = "generic.py"}, parameters = [], type_parameters = ["T"], function_type = (!ac.type_param<@generic, "T">, !b3) -> (), input_names = ["value", "raw"], output_names = []} : () -> ()
}

// -----
// Members must be dictionaries at the native schema boundary.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  // expected-error @+1 {{attribute 'members' failed to satisfy constraint}}
  "ac.enum"() {sym_name = "pkg.Bad", width = #ac.math_int<3>, encoding = "explicit", members = [#ac.math_int<0>]} : () -> ()
}

// -----
// A declaration nested in a hardware definition is not package-owned.
#w0 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<0>}}>
#w1 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<1>}}>
#w3 = #ac.static_expr<{kind = "literal", location = {path = "fixture.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @fixture, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<3>}}>
!b1 = !ac.bits<#w1>
!b3 = !ac.bits<#w3>
module {
  "ac.module"() ({
  ^bb0:
    // expected-error @+1 {{enum requires package placement}}
    "ac.enum"() {sym_name = "pkg.Nested", width = #ac.math_int<3>, encoding = "explicit", members = [{name = "IDLE", code = #ac.math_int<0>}]} : () -> ()
    "ac.yield"() : () -> ()
  }) {sym_name = "generic", source_owner = {package = "", path = "generic.py"}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = []} : () -> ()
}
