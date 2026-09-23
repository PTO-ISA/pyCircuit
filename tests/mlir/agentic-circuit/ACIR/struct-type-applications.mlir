// RUN: %acir_opt_public %s | %FileCheck %s

builtin.module {
  ac.type_scope @types {
    ac.struct @Batch fields [
      {name = "lanes", type_expr = #ac.type_expr<#ac.type_expr_value_array<#ac.dependent_value<#ac.dependent_parameter<"width">>, #ac.type_expr<#ac.type_expr_concrete<i8>>>>},
      {name = "index", type_expr = #ac.type_expr<#ac.type_expr_range<#ac.dependent_value<#ac.dependent_integer<0>>, #ac.dependent_value<#ac.dependent_parameter<"width">>>>}
    ] {
      parameters = #ac.static_parameters<[
        #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/types.py", 1, 1, 1, 1>>
      ]>
    }
  } {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Batch, #ac.dependent_arguments<[#ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>]>> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64},
    !ac.struct<@types::@Batch, #ac.dependent_arguments<[#ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>>>]>> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 9 : i64}
  >}
  ac.module @stage source #ac.source_owner<"pkg/stage.py", "pkg/stage.py"> schema #ac.module_family_schema<
    #ac.static_parameters<[
      #ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<4, false>>, true, [], #ac.source_provenance<"pkg/stage.py", 1, 1, 1, 1>>
    ]>,
    #ac.static_cases<[
      #ac.static_arguments<[#ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>]>,
      #ac.static_arguments<[#ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>]>
    ]>,
    #ac.module_interface<[
      #ac.interface_port<"value", "input", #ac.type_expr<#ac.type_expr_queue<
        #ac.type_expr<#ac.type_expr_nominal<@types::@Batch, #ac.dependent_arguments<[
          #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_parameter<"width">>>
        ]>>>,
        #ac.dependent_value<#ac.dependent_integer<1>>,
        #ac.dependent_value<#ac.dependent_integer<1>>
      >>, #ac.source_provenance<"pkg/stage.py", 2, 1, 2, 1>>,
      #ac.interface_port<"result", "output", #ac.type_expr<#ac.type_expr_queue<
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
    ac.module.case arguments #ac.static_arguments<[
      #ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>
    ]> type (
      !ac.queue<!ac.struct<@types::@Batch, #ac.dependent_arguments<[
        #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>
      ]>>>
    ) -> !ac.queue<!ac.struct<@types::@Batch, #ac.dependent_arguments<[
      #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>
    ]>>> source #ac.source_provenance<"pkg/stage.py", 3, 1, 3, 1> graph {
    ^bb0(%value: !ac.queue<!ac.struct<@types::@Batch, #ac.dependent_arguments<[
      #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>
    ]>>>):
      ac.return %value : !ac.queue<!ac.struct<@types::@Batch, #ac.dependent_arguments<[
        #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 1 : i4>>>>>
      ]>>>
    }
    ac.module.case arguments #ac.static_arguments<[
      #ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>
    ]> type (
      !ac.queue<!ac.struct<@types::@Batch, #ac.dependent_arguments<[
        #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>>>
      ]>>>
    ) -> !ac.queue<!ac.struct<@types::@Batch, #ac.dependent_arguments<[
      #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>>>
    ]>>> source #ac.source_provenance<"pkg/stage.py", 4, 1, 4, 1> graph {
    ^bb0(%value: !ac.queue<!ac.struct<@types::@Batch, #ac.dependent_arguments<[
      #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>>>
    ]>>>):
      ac.return %value : !ac.queue<!ac.struct<@types::@Batch, #ac.dependent_arguments<[
        #ac.dependent_argument<"width", #ac.dependent_value<#ac.dependent_static<#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 8 : i4>>>>>
      ]>>>
    }
  }
}

// CHECK: ac.module.case arguments {{.*}}1 : i4{{.*}} type (!ac.queue<!ac.struct<@types::@Batch, {{.*}}1 : i4
// CHECK: ac.module.case arguments {{.*}}8 : i4{{.*}} type (!ac.queue<!ac.struct<@types::@Batch, {{.*}}8 : i4
