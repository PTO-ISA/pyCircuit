// RUN: %acir_opt_public --split-input-file --verify-diagnostics %s

builtin.module {
  ac.type_scope @types {
    ac.struct @Batch fields [{name = "value", type_expr = #ac.type_expr<#ac.type_expr_concrete<i8>>}] {
      parameters = #ac.static_parameters<[
        #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/types.py", 1, 1, 1, 1>>
      ]>
    }
    // expected-error @+1 {{struct application arguments must exactly match declaration parameters}}
    ac.struct @Holder fields [{name = "batch", type = !ac.struct<@types::@Batch>}]
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Batch, #ac.dependent_arguments<[#ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>]>> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64},
    !ac.struct<@types::@Holder> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
}

// -----

builtin.module {
  ac.type_scope @types {
    ac.struct @Batch fields [{name = "value", type_expr = #ac.type_expr<#ac.type_expr_concrete<i8>>}] {
      parameters = #ac.static_parameters<[
        #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/types.py", 1, 1, 1, 1>>
      ]>
    }
    // expected-error @+1 {{struct application has no exact typed DLTI layout}}
    ac.struct @Holder fields [{name = "batch", type = !ac.struct<@types::@Batch, #ac.dependent_arguments<[
      #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>>>
    ]>>}]
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Batch, #ac.dependent_arguments<[#ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>]>> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64},
    !ac.struct<@types::@Holder> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
}

// -----

builtin.module {
  ac.type_scope @types {
    ac.struct @Plain fields [{name = "value", type = i8}]
    // expected-error @+1 {{empty struct application arguments must use the zero-argument form}}
    ac.struct @Holder fields [{name = "plain", type = !ac.struct<@types::@Plain, #ac.dependent_arguments<[]>>}]
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Plain> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64},
    !ac.struct<@types::@Holder> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
}

// -----

builtin.module {
  ac.type_scope @types {
    ac.struct @Batch fields [{name = "value", type_expr = #ac.type_expr<#ac.type_expr_concrete<i8>>}] {
      parameters = #ac.static_parameters<[
        #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/types.py", 1, 1, 1, 1>>
      ]>
    }
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Batch, #ac.dependent_arguments<[#ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>>>]>> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
  ac.module @stage source #ac.source_owner<"pkg/stage.py", "pkg/stage.py"> schema #ac.module_family_schema<
    #ac.static_parameters<[
      #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/stage.py", 1, 1, 1, 1>>
    ]>,
    #ac.static_cases<[
      #ac.static_arguments<[#ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>]>
    ]>,
    #ac.module_interface<[
      #ac.interface_port<"value", "input", #ac.type_expr<#ac.type_expr_queue<
        #ac.type_expr<#ac.type_expr_nominal<@types::@Batch, #ac.dependent_arguments<[
          #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_parameter<"width">>>
        ]>>>,
        #ac.dependent_value<#ac.dependent_integer<1>>,
        #ac.dependent_value<#ac.dependent_integer<1>>
      >>, #ac.source_provenance<"pkg/stage.py", 2, 1, 2, 1>>
    ]>,
    #ac.source_owner<"pkg/stage.py", "pkg/stage.py">,
    [@Batch]
  > {
    // expected-error @+1 {{dependent interface does not materialize to the case signature: materialized logical interface disagrees with concrete case signature}}
    ac.module.case arguments #ac.static_arguments<[
      #ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>
    ]> type (
      !ac.queue<!ac.struct<@types::@Batch, #ac.dependent_arguments<[
        #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>
      ]>>>
    ) -> () source #ac.source_provenance<"pkg/stage.py", 3, 1, 3, 1> graph {
    ^bb0(%value: !ac.queue<!ac.struct<@types::@Batch, #ac.dependent_arguments<[
      #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>
    ]>>>):
      ac.return
    }
  }
}

// -----

builtin.module {
  ac.type_scope @types {
    ac.struct @Batch fields [{name = "value", type_expr = #ac.type_expr<#ac.type_expr_concrete<i8>>}] {
      parameters = #ac.static_parameters<[
        #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/types.py", 1, 1, 1, 1>>,
        #ac.static_parameter<"enabled", #ac.static_type<#ac.static_bool_type>, true, [], #ac.source_provenance<"pkg/types.py", 2, 1, 2, 1>>
      ]>
    }
    // expected-error @+1 {{struct application arguments must preserve declaration order, names, and exact typed values}}
    ac.struct @Holder fields [{name = "batch", type = !ac.struct<@types::@Batch, #ac.dependent_arguments<[
      #ac.dependent_argument<"enabled", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_bool_value<true>>>>>,
      #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>
    ]>>}]
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Batch, #ac.dependent_arguments<[#ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>, #ac.dependent_argument<"enabled", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_bool_value<true>>>>>]>> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64},
    !ac.struct<@types::@Holder> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
}

// -----

builtin.module {
  ac.type_scope @types {
    ac.struct @Batch fields [{name = "value", type_expr = #ac.type_expr<#ac.type_expr_concrete<i8>>}] {
      parameters = #ac.static_parameters<[
        #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/types.py", 1, 1, 1, 1>>
      ]>
    }
    // expected-error @+1 {{struct application arguments must preserve declaration order, names, and exact typed values}}
    ac.struct @Holder fields [{name = "batch", type = !ac.struct<@types::@Batch, #ac.dependent_arguments<[
      #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_bool_value<true>>>>>
    ]>>}]
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Batch, #ac.dependent_arguments<[#ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>]>> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64},
    !ac.struct<@types::@Holder> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
}

// -----

builtin.module {
  ac.type_scope @types {
    ac.struct @Batch fields [{name = "value", type_expr = #ac.type_expr<#ac.type_expr_concrete<i8>>}] {
      parameters = #ac.static_parameters<[
        #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/types.py", 1, 1, 1, 1>>
      ]>
    }
    // expected-error @+1 {{struct application arguments must preserve declaration order, names, and exact typed values}}
    ac.struct @Holder fields [{name = "batch", type = !ac.struct<@types::@Batch, #ac.dependent_arguments<[
      #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_parameter<"width">>>
    ]>>}]
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Batch, #ac.dependent_arguments<[#ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>]>> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64},
    !ac.struct<@types::@Holder> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}
  >}
}
