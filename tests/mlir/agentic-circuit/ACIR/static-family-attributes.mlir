// RUN: %acir_opt %s | %FileCheck %s

builtin.module attributes {
  ac.family_schema = #ac.module_family_schema<
    #ac.static_parameters<[
      #ac.static_parameter<"enabled", #ac.static_type<#ac.static_bool_type>, true, [], #ac.source_provenance<"pkg/stage.py", 1, 1, 1, 1>>,
      #ac.static_parameter<"lanes", #ac.static_type<#ac.static_int_type<4, false>>, false default #ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 2 : i4>>, [#ac.static_constraint<#ac.one_of<[#ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 2 : i4>>, #ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 4 : i4>>]>>], #ac.source_provenance<"pkg/stage.py", 2, 1, 2, 1>>
    ]>,
    #ac.static_cases<[
      #ac.static_arguments<[
        #ac.static_argument<"enabled", #ac.static_value<#ac.static_bool_value<true>>>,
        #ac.static_argument<"lanes", #ac.static_value<#ac.static_int_value<#ac.static_int_type<4, false>, 2 : i4>>>
      ]>
    ]>,
    #ac.module_interface<[]>,
    #ac.source_owner<"pkg/stage.py", "pkg/stage.py">,
    []>
} {
}

// CHECK: #ac.module_family_schema<
// CHECK-SAME: #ac.static_parameter<"enabled", <#ac.static_bool_type>, true, [], <"pkg/stage.py", 1, 1, 1, 1>>
// CHECK-SAME: #ac.static_parameter<"lanes", <#ac.static_int_type<4, false>>, false default
// CHECK-SAME: #ac.static_arguments<[
// CHECK-SAME: <"pkg/stage.py", "pkg/stage.py">
