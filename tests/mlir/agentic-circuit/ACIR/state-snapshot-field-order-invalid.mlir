// RUN: %not %acir_opt %s 2>&1 | %FileCheck %s --check-prefix=ORDER

// Field lists in ACIR denote sets, and the canonical order is Table Entry
// declaration order. Here the Entry declares `right` before `left`, so a
// snapshot that reads `left` before `right` is not normalized and must be
// rejected. The write endpoint in the same rule already follows the canonical
// order, so the snapshot is the only violation.

module  {
  ac.type_scope @types {
    ac.struct @Entry fields [{name = "right", type = i8}, {name = "left", type = i8}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Entry> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 2 : i64}>}
  ac.table @state entry !ac.struct<@types::@Entry> entries 4 init 0 owner "/"
      stable_id "table/state"
  %input = "builtin.unrealized_conversion_cast"() : () -> !ac.queue<i2>
  ac.rule %input depths [] latencies [] name "read" stable_id "read"
      domain "cycle" type exact {
  ^body(%item: !ac.var<i2>):
    %old = ac.table.get @state[%item] : !ac.var<i2> -> !ac.var<!ac.struct<@types::@Entry>>
    %true = ac.var.constant true as !ac.var<i1>
    ac.rule.condition %true : !ac.var<i1>
    ac.table.propose @state[%item] = %old mode "replace"
        write_fields ["right", "left"]
        : !ac.var<i2>, !ac.var<!ac.struct<@types::@Entry>>
    ac.state.snapshot @state[%item : !ac.var<i2>] for %true : !ac.var<i1>
        kind #ac<rule_index_kind dynamic> read_fields ["left", "right"]
    ac.rule.return
  } : (!ac.queue<i2>) -> ()
}

// ORDER: read_fields must follow Table Entry declaration order
