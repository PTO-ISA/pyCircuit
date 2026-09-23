// RUN: %split_file %s %t
// RUN: %acir_opt %t/distinct-cases.mlir | %FileCheck %s --check-prefix=DISTINCT
// RUN: %not %acir_opt %t/duplicate-in-case.mlir 2>&1 | %FileCheck %s --check-prefix=DUPLICATE

//--- distinct-cases.mlir
#owner = #ac.source_owner<"tests/table_family.py", "tests/table_family.py">
#prov = #ac.source_provenance<"tests/table_family.py", 1, 1, 1, 1>
#enabled = #ac.static_value<#ac.static_bool_value<true>>
#disabled = #ac.static_value<#ac.static_bool_value<false>>
#enabled_args = #ac.static_arguments<[
  #ac.static_argument<"enabled", #enabled>
]>
#disabled_args = #ac.static_arguments<[
  #ac.static_argument<"enabled", #disabled>
]>
#schema = #ac.module_family_schema<
  #ac.static_parameters<[
    #ac.static_parameter<"enabled", #ac.static_type<#ac.static_bool_type>, true, [], #prov>
  ]>,
  #ac.static_cases<[#enabled_args, #disabled_args]>,
  #ac.module_interface<[]>, #owner, []>

builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle"
} {
  ac.module @Family source #owner schema #schema {
    ac.module.case arguments #enabled_args type () -> () source #prov graph {
      ac.scope @body() {
        ac.table @entries entry i8 entries 1 init 0 owner "/" stable_id "table/entries"
        %index = ac.var.constant false as !ac.var<i1>
        %entry = ac.table.get @entries[%index] : !ac.var<i1> -> !ac.var<i8>
        ac.scope.yield
      } : () -> ()
      ac.return
    }
    ac.module.case arguments #disabled_args type () -> () source #prov graph {
      ac.scope @body() {
        ac.table @entries entry i8 entries 1 init 0 owner "/" stable_id "table/entries"
        %index = ac.var.constant false as !ac.var<i1>
        %entry = ac.table.get @entries[%index] : !ac.var<i1> -> !ac.var<i8>
        ac.scope.yield
      } : () -> ()
      ac.return
    }
  }
}

// DISTINCT: ac.module @Family
// DISTINCT-COUNT-2: ac.table @entries entry i8 entries 1 init 0 owner "/" stable_id "table/entries"

//--- duplicate-in-case.mlir
#owner = #ac.source_owner<"tests/table_family.py", "tests/table_family.py">
#prov = #ac.source_provenance<"tests/table_family.py", 1, 1, 1, 1>

builtin.module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle"
} {
  ac.module @Family source #owner schema #ac.module_family_schema<
      #ac.static_parameters<[]>,
      #ac.static_cases<[#ac.static_arguments<[]>]>,
      #ac.module_interface<[]>, #owner, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () source #prov graph {
      ac.scope @left() {
        ac.table @entries entry i8 entries 1 init 0 owner "/" stable_id "table/entries"
        %index = ac.var.constant false as !ac.var<i1>
        %entry = ac.table.get @entries[%index] : !ac.var<i1> -> !ac.var<i8>
        ac.scope.yield
      } : () -> ()
      ac.scope @right() {
        ac.table @entries entry i8 entries 1 init 0 owner "/" stable_id "table/entries"
        %index = ac.var.constant false as !ac.var<i1>
        %entry = ac.table.get @entries[%index] : !ac.var<i1> -> !ac.var<i8>
        ac.scope.yield
      } : () -> ()
      ac.return
    }
  }
}

// DUPLICATE: error: 'ac.table' op stable_id must be unique
