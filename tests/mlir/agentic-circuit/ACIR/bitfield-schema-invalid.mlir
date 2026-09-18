// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/schema-range.mlir 2>&1 | %FileCheck %s --check-prefix=SCHEMA-RANGE
// RUN: %not %acir_opt %t/schema-order.mlir 2>&1 | %FileCheck %s --check-prefix=SCHEMA-ORDER
// RUN: %not %acir_opt %t/extract-range.mlir 2>&1 | %FileCheck %s --check-prefix=EXTRACT-RANGE
// RUN: %not %acir_opt %t/concat-width.mlir 2>&1 | %FileCheck %s --check-prefix=CONCAT-WIDTH

// SCHEMA-RANGE: error: 'ac.bitfield' op field 'bad' range must satisfy 0 <= lsb <= msb < width
// SCHEMA-ORDER: error: 'ac.bitfield' op fields must be sorted by UTF-8 name
// EXTRACT-RANGE: error: 'ac.var.extract' op bitfield field range does not match its schema
// CONCAT-WIDTH: error: 'ac.var.concat' op bitfield concat input width does not match its field

builtin.module  {
  ac.type_scope @types {
    ac.bitfield @Instr width 8 fields [{lsb = 4 : i64, msb = 7 : i64, name = "hi"}, {lsb = 0 : i64, msb = 3 : i64, name = "low"}, {lsb = 0 : i64, msb = 5 : i64, name = "low6"}]
  }
}

//--- schema-range.mlir
builtin.module  {
  ac.type_scope @types {
    ac.bitfield @Bad width 8 fields [{lsb = 0 : i64, msb = 8 : i64, name = "bad"}]
  }
}

//--- schema-order.mlir
builtin.module  {
  ac.type_scope @types {
    ac.bitfield @Bad width 8 fields [{lsb = 0 : i64, msb = 3 : i64, name = "low"}, {lsb = 4 : i64, msb = 7 : i64, name = "hi"}]
  }
}

//--- extract-range.mlir
builtin.module  {
  ac.type_scope @types {
    ac.bitfield @Instr width 8 fields [{lsb = 4 : i64, msb = 7 : i64, name = "hi"}, {lsb = 0 : i64, msb = 3 : i64, name = "low"}, {lsb = 0 : i64, msb = 5 : i64, name = "low6"}]
  }
  %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<i8>
  %bad = ac.var.extract %value from 3 width 4 {ac.bitfield_field = "hi", ac.bitfield_schema = @types::@Instr} : !ac.var<i8> -> !ac.var<i4>
}

//--- concat-width.mlir
builtin.module  {
  ac.type_scope @types {
    ac.bitfield @Instr width 8 fields [{lsb = 4 : i64, msb = 7 : i64, name = "hi"}, {lsb = 0 : i64, msb = 3 : i64, name = "low"}, {lsb = 0 : i64, msb = 5 : i64, name = "low6"}]
  }
  %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<i4>
  %bad = ac.var.concat %value {ac.bitfield_fields = ["low6"], ac.bitfield_schema = @types::@Instr} : !ac.var<i4> -> !ac.var<i4>
}
