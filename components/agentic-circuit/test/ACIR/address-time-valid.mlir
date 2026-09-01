// RUN: %acir_opt_public %s | %FileCheck %s
// RUN: %acir_opt_public %s | %acir_opt_public | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  ac.module @Bridge() parameters {} graph { ac.return }
  ac.module @Top() parameters {} graph {
    ac.instance @cdc of @Bridge() static {} id "cdc" path "cdc" : () -> ()
    ac.time_domain @global period 1 phase 0 scale 1
    ac.time_domain @core period 2 phase 1 scale 2 parent @global
        bridge {kind = "explicit", owner = @cdc}
    ac.time_domain @late period 9223372036854775807 phase 9223372036854775807 scale 2
    ac.address_space @physical width 48 unit "byte" id "physical" path "physical"
        layout #dlti.dl_spec<>
    ac.address_space @virtual width 32 unit "byte" id "virtual" path "virtual"
        parent @physical translate {numerator = 1 : i64, denominator = 1 : i64, offset = 4096 : i64}
    ac.address_space @bits width 19 unit "bit" id "bits" path "bits"
    ac.address_space @bytes width 16 unit "byte" id "bytes" path "bytes"
        parent @bits translate {numerator = 8 : i64, denominator = 1 : i64, offset = 0 : i64}
    ac.address_space @small width 8 unit "byte" id "small" path "small"
    ac.address_space @wide width 16 unit "byte" id "wide" path "wide"
        parent @small translate {numerator = 1 : i64, denominator = 256 : i64,
                                 offset = 0 : i64, alignment = 256 : i64}
    ac.address_space @half width 7 unit "byte" id "half" path "half"
    ac.address_space @aligned width 8 unit "byte" id "aligned" path "aligned"
        parent @half translate {numerator = 1 : i64, denominator = 2 : i64,
                                offset = 0 : i64, alignment = 2 : i64}
    ac.address_space @full width 64 unit "byte" id "full" path "full"
    ac.address_map @map source @virtual entries [
      {base = 0 : i64, size = 4096 : i64, target = @physical, offset = 0 : i64,
       permissions = ["read", "write"], classes = [], priority = 1 : i64},
      {base = 4096 : i64, size = 4096 : i64, target = @physical, offset = 8192 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 64 : i64, banks = 4 : i64, bank = 0 : i64}}
    ] default {kind = "unmapped"}
    ac.address_map @upper source @full entries [
      {base = -9223372036854775808 : i64, size = -9223372036854775808 : i64,
       target = @full, offset = 0 : i64, permissions = ["read"], classes = [],
       priority = -1 : i64}
    ] default {kind = "unmapped"}
    ac.address_map @permission_split source @small entries [
      {base = 0 : i64, size = 16 : i64, target = @small, offset = 0 : i64,
       permissions = ["read"], classes = []},
      {base = 0 : i64, size = 16 : i64, target = @small, offset = 0 : i64,
       permissions = ["write"], classes = []}
    ] default {kind = "unmapped"}
    ac.address_space @bank_target width 6 unit "byte" id "bank_target" path "bank_target"
    ac.address_map @bank_split source @small entries [
      {base = 0 : i64, size = 256 : i64, target = @bank_target, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 0 : i64}},
      {base = 0 : i64, size = 256 : i64, target = @bank_target, offset = 0 : i64,
       permissions = ["read"], classes = [],
       interleave = {granularity = 1 : i64, banks = 4 : i64, bank = 1 : i64}}
    ] default {kind = "unmapped"}
    ac.return
  }
}

// CHECK: ac.time_domain @core
// CHECK: ac.address_space @virtual
// CHECK: ac.address_map @map
// CHECK: ac.address_map @upper
// CHECK: ac.address_map @bank_split
