// RUN: %acir_opt_public --split-input-file --verify-diagnostics %s

builtin.module {
  ac.type_scope @types {
    // expected-error @+1 {{dependent struct fields require exact {name, type_expr} records}}
    ac.struct @Batch fields [{name = "value", type = i8}] {
      parameters = #ac.static_parameters<[
        #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/types.py", 1, 1, 1, 1>>
      ]>
    }
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Batch, #ac.dependent_arguments<[#ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>]>> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
}

// -----

builtin.module {
  ac.type_scope @types {
    // expected-error @+1 {{parameterized struct layout must use a typed application key}}
    ac.struct @Batch fields [{name = "value", type_expr = #ac.type_expr<#ac.type_expr_concrete<i8>>}] {
      parameters = #ac.static_parameters<[
        #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/types.py", 1, 1, 1, 1>>
      ]>
    }
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Batch> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
}

// -----

builtin.module {
  ac.type_scope @types {
    // expected-error @+1 {{dependent struct DLTI layout disagrees with materialized physical fields}}
    ac.struct @Batch fields [
      {name = "lanes", type_expr = #ac.type_expr<#ac.type_expr_value_array<#ac.dependent_value<#ac.dependent_parameter<"width">>, #ac.type_expr<#ac.type_expr_concrete<i8>>>>}
    ] {
      parameters = #ac.static_parameters<[
        #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/types.py", 1, 1, 1, 1>>
      ]>
    }
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Batch, #ac.dependent_arguments<[#ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>>>]>> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
}
