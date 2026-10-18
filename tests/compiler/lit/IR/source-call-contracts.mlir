// RUN: %pycircuit_opt %s --split-input-file --verify-diagnostics

// A complete contract preserves logical authority even when Boolean, fixed u1,
// and one-bit Integer have the same physical type. A structural declaration
// without the optional bundle remains valid.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#bool = #ac.source_domain<{kind = "bool"}>
#integer1 = #ac.source_domain<{kind = "integer", lower = #ac.math_int<0>, upper = #ac.math_int<2>}>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  "ac.module"() ({
  ^bb0(%flag: !b1, %clock: !b1, %reset: !b1):
    %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result">
    "ac.yield"(%result) : (!ac.struct<"Result">) -> ()
  }) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> !ac.struct<"Result">, input_names = ["flag", "pyc_clk", "pyc_rst"], output_names = ["result"], ac.return_form = "single", ac.parameters = [{name = "flag", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "boolean", domain = #bool}, default = {present = false}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}} : () -> ()
  "ac.module"() ({
  ^bb0(%flag: !b1):
    %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result">
    "ac.yield"(%result) : (!ac.struct<"Result">) -> ()
  }) {sym_name = "Structural", source_owner = {package = "", path = "structural.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !ac.struct<"Result">, input_names = ["flag"], output_names = ["result"]} : () -> ()
}

// -----

// Any partial optional bundle rejects.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call contract requires complete physical signature metadata}}
  "ac.module"() ({^bb0(%flag: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !ac.struct<"Result">, input_names = ["flag"], output_names = ["result"], ac.return_form = "single"} : () -> ()
}

// -----

// Boolean authority requires the Boolean domain, while fixed bits carry no
// domain field. These two errors distinguish equal physical u1 payloads.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#int = #ac.source_domain<{kind = "integer", lower = #ac.math_int<0>, upper = #ac.math_int<2>}>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call Boolean requires a bool domain and one-bit physical type}}
  "ac.module"() ({^bb0(%flag: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !ac.struct<"Result">, input_names = ["flag"], output_names = ["result"], ac.return_form = "single", ac.parameters = [{name = "flag", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "boolean", domain = #int}, default = {present = false}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {}} : () -> ()
}

// -----

#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#bool = #ac.source_domain<{kind = "bool"}>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call hardware constraint fields or type scope are invalid}}
  "ac.module"() ({^bb0(%flag: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !ac.struct<"Result">, input_names = ["flag"], output_names = ["result"], ac.return_form = "single", ac.parameters = [{name = "flag", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "fixed_bits", domain = #bool}, default = {present = false}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {}} : () -> ()
}

// -----

// Integer authority requires the exact half-open [0, 2**W) domain.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#wrong = #ac.source_domain<{kind = "integer", lower = #ac.math_int<0>, upper = #ac.math_int<4>}>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call Integer domain must be [0, 2**W)}}
  "ac.module"() ({^bb0(%flag: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !ac.struct<"Result">, input_names = ["flag"], output_names = ["result"], ac.return_form = "single", ac.parameters = [{name = "flag", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "integer", domain = #wrong}, default = {present = false}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {}} : () -> ()
}

// -----

// Domain indices are explicit and follow the data formals.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#bool = #ac.source_domain<{kind = "bool"}>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call domain must explicitly map clock and reset after the data inputs}}
  "ac.module"() ({^bb0(%flag: !b1, %clock: !b1, %reset: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> !ac.struct<"Result">, input_names = ["flag", "pyc_clk", "pyc_rst"], output_names = ["result"], ac.return_form = "single", ac.parameters = [{name = "flag", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "boolean", domain = #bool}, default = {present = false}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {clock = 2 : i64, reset = 1 : i64}} : () -> ()
}

// -----

// Physical domain names cannot be inferred or renamed.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#bool = #ac.source_domain<{kind = "bool"}>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call domain disagrees with physical port names or types}}
  "ac.module"() ({^bb0(%flag: !b1, %clock: !b1, %reset: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1, !b1, !b1) -> !ac.struct<"Result">, input_names = ["flag", "clock", "reset"], output_names = ["result"], ac.return_form = "single", ac.parameters = [{name = "flag", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "boolean", domain = #bool}, default = {present = false}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {clock = 1 : i64, reset = 2 : i64}} : () -> ()
}

// -----

// A present source-call default rejects independently of formal naming.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call parameters require matching names, positional-or-keyword binding and absent defaults}}
  "ac.module"() ({^bb0(%flag: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !ac.struct<"Result">, input_names = ["flag"], output_names = ["result"], ac.return_form = "single", ac.parameters = [{name = "flag", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "fixed_bits"}, default = {present = true, value = {kind = "integer", value = #ac.math_int<0>}}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {}} : () -> ()
}

// -----

// A mismatched formal name rejects with an otherwise valid absent default.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call parameters require matching names, positional-or-keyword binding and absent defaults}}
  "ac.module"() ({^bb0(%flag: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !ac.struct<"Result">, input_names = ["flag"], output_names = ["result"], ac.return_form = "single", ac.parameters = [{name = "other", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "fixed_bits"}, default = {present = false}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {}} : () -> ()
}

// -----

// Parameter provenance must identify the actual provider definition.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#foreign = {site = {definition = @Foreign, ast_path = []}, expansion = []}
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call occurrence definition does not match its canonical symbol}}
  "ac.module"() ({^bb0(%flag: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !ac.struct<"Result">, input_names = ["flag"], output_names = ["result"], ac.return_form = "single", ac.parameters = [{name = "flag", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "fixed_bits"}, default = {present = false}, origin = #foreign, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {}} : () -> ()
}

// -----

// Parameter location paths remain tied to the original provider source.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#span = {path = "consumer.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call parameter path differs from its original SourceOwner}}
  "ac.module"() ({^bb0(%flag: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> !ac.struct<"Result">, input_names = ["flag"], output_names = ["result"], ac.return_form = "single", ac.parameters = [{name = "flag", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "fixed_bits"}, default = {present = false}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {}} : () -> ()
}

// -----

// Static Integer parameters do not excuse an incomplete hardware-input contract.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
module {
  ac.struct "Result" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call input count differs from its domain-free physical signature}}
  "ac.module"() ({^bb0(%flag: !b1): %result = "ac.struct.create"(%flag) : (!b1) -> !ac.struct<"Result"> "ac.yield"(%result) : (!ac.struct<"Result">) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [{name = "N", type = !ac.math_int}], type_parameters = [], function_type = (!b1) -> !ac.struct<"Result">, input_names = ["flag"], output_names = ["result"], ac.return_form = "single", ac.parameters = [], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Result">, source_kind = "nominal"}], ac.domain_inputs = {}} : () -> ()
}

// -----

// Tuple source modules publish one positional physical result and one source
// constraint per element. Equal-layout nominal elements retain their distinct
// identities, and fixed bits retain their source kind.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Header" fields [{name = "value", type = !b1}]
  ac.struct "Payload" fields [{name = "value", type = !b1}]
  "ac.module"() ({
  ^bb0(%value: !b1):
    %header = "ac.struct.create"(%value) : (!b1) -> !ac.struct<"Header">
    %payload = "ac.struct.create"(%value) : (!b1) -> !ac.struct<"Payload">
    "ac.yield"(%header, %payload, %value) : (!ac.struct<"Header">, !ac.struct<"Payload">, !b1) -> ()
  }) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> (!ac.struct<"Header">, !ac.struct<"Payload">, !b1), input_names = ["value"], output_names = ["result_0", "result_1", "result_2"], ac.return_form = "tuple", ac.parameters = [{name = "value", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "fixed_bits"}, default = {present = false}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Header">, source_kind = "nominal"}, {kind = "hardware", type = !ac.struct<"Payload">, source_kind = "nominal"}, {kind = "hardware", type = !b1, source_kind = "fixed_bits"}], ac.domain_inputs = {}} : () -> ()
}

// -----

// Tampering result ordinal one to the equal-layout but wrong nominal type must
// be rejected. Checking only arity or packed width would accept this change.
#w1 = #ac.static_expr<{kind = "literal", value = {kind = "integer", value = #ac.math_int<1>}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, location = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}>
!b1 = !ac.bits<#w1>
#span = {path = "provider.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#origin = {site = {definition = @Provider, ast_path = []}, expansion = []}
module {
  ac.struct "Header" fields [{name = "value", type = !b1}]
  ac.struct "Payload" fields [{name = "value", type = !b1}]
  // expected-error @+1 {{source module call hardware constraint disagrees with its physical type}}
  "ac.module"() ({^bb0(%value: !b1): %header = "ac.struct.create"(%value) : (!b1) -> !ac.struct<"Header"> %payload = "ac.struct.create"(%value) : (!b1) -> !ac.struct<"Payload"> "ac.yield"(%header, %payload, %value) : (!ac.struct<"Header">, !ac.struct<"Payload">, !b1) -> ()}) {sym_name = "Provider", source_owner = {package = "", path = "provider.py"}, parameters = [], type_parameters = [], function_type = (!b1) -> (!ac.struct<"Header">, !ac.struct<"Payload">, !b1), input_names = ["value"], output_names = ["result_0", "result_1", "result_2"], ac.return_form = "tuple", ac.parameters = [{name = "value", binding = "positional_or_keyword", constraint = {kind = "hardware", type = !b1, source_kind = "fixed_bits"}, default = {present = false}, origin = #origin, location = #span}], ac.result_constraints = [{kind = "hardware", type = !ac.struct<"Header">, source_kind = "nominal"}, {kind = "hardware", type = !ac.struct<"Header">, source_kind = "nominal"}, {kind = "hardware", type = !b1, source_kind = "fixed_bits"}], ac.domain_inputs = {}} : () -> ()
}
