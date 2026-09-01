// RUN: %split_file %s %t
// RUN: %acir_opt_public %t/a.mlir > %t/a.out
// RUN: %acir_opt_public %t/b.mlir > %t/b.out
// RUN: diff %t/a.out %t/b.out
// RUN: %acir_opt_public %t/a.out > %t/a.roundtrip
// RUN: diff %t/a.out %t/a.roundtrip

//--- a.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.address_space @source width 8 unit "byte" id "source" path "source"
    ac.address_space @target_a width 8 unit "byte" id "target_a" path "target_a"
    ac.address_space @target_b width 8 unit "byte" id "target_b" path "target_b"
    ac.address_map @map source @source entries [
      {base = 0 : i64, size = 16 : i64, target = @target_b, offset = 8 : i64,
       permissions = ["execute", "write"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 1 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @target_a, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}}
    ] default {kind = "unmapped"}
    ac.return
  }
}

//--- b.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @M() parameters {} graph {
    ac.address_space @source width 8 unit "byte" id "source" path "source"
    ac.address_space @target_a width 8 unit "byte" id "target_a" path "target_a"
    ac.address_space @target_b width 8 unit "byte" id "target_b" path "target_b"
    ac.address_map @map source @source entries [
      {base = 0 : i64, size = 16 : i64, target = @target_a, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 16 : i64, target = @target_b, offset = 8 : i64,
       permissions = ["write", "execute"], classes = [],
       interleave = {granularity = 1 : i64, banks = 2 : i64, bank = 1 : i64}}
    ] default {kind = "unmapped"}
    ac.return
  }
}
