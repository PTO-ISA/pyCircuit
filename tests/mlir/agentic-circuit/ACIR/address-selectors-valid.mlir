// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "A", fields = [{name = "tag", type = i8}]}> : () -> ()
    "ac.transaction"() <{sym_name = "B", fields = [{name = "tag", type = i8}]}> : () -> ()
  }) : () -> ()
  ac.module @M() parameters {} graph {
    ac.address_space @space width 8 unit "byte" id "space" path "space"
    ac.address_map @class_split source @space entries [
      {base = 0 : i64, size = 32 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [@types::@A]},
      {base = 0 : i64, size = 32 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [@types::@B]}
    ] default {kind = "unmapped"}
    ac.address_map @mixed_disjoint source @space entries [
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 2 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 2 : i64}}
    ] default {kind = "unmapped"}
    ac.address_map @mixed_priority source @space entries [
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [], priority = 2 : i64,
       interleave = {granularity = 2 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [], priority = 1 : i64,
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 1 : i64}}
    ] default {kind = "unmapped"}
    ac.address_map @general_fast_forward source @space entries [
      {base = 24 : i64, size = 62 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 2 : i64, banks = 7 : i64, bank = 0 : i64}},
      {base = 48 : i64, size = 98 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 8 : i64, banks = 8 : i64, bank = 5 : i64}}
    ] default {kind = "unmapped"}
    ac.address_map @general_fast_reverse source @space entries [
      {base = 48 : i64, size = 98 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 8 : i64, banks = 8 : i64, bank = 5 : i64}},
      {base = 24 : i64, size = 62 : i64, target = @space, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 2 : i64, banks = 7 : i64, bank = 0 : i64}}
    ] default {kind = "unmapped"}
    ac.return
  }
}

// CHECK: ac.address_map @class_split
// CHECK: ac.address_map @mixed_disjoint
// CHECK: ac.address_map @mixed_priority
// CHECK: ac.address_map @general_fast_forward
// CHECK: ac.address_map @general_fast_reverse
