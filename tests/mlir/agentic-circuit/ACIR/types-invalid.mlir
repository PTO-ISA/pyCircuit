// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/vector-zero.mlir 2>&1 | %FileCheck %s --check-prefix=VECTOR-ZERO
// RUN: %not %acir_opt %t/vector-negative.mlir 2>&1 | %FileCheck %s --check-prefix=VECTOR-NEGATIVE
// RUN: %not %acir_opt %t/vector-minimum.mlir 2>&1 | %FileCheck %s --check-prefix=VECTOR-MINIMUM
// RUN: %not %acir_opt %t/vector-overflow.mlir 2>&1 | %FileCheck %s --check-prefix=VECTOR-OVERFLOW
// RUN: %not %acir_opt %t/vector-underflow.mlir 2>&1 | %FileCheck %s --check-prefix=VECTOR-UNDERFLOW
// RUN: %not %acir_opt %t/channel-nested.mlir 2>&1 | %FileCheck %s --check-prefix=CHANNEL-NESTED
// RUN: %not %acir_opt %t/channel-standalone.mlir 2>&1 | %FileCheck %s --check-prefix=CHANNEL-STANDALONE
// RUN: %not %acir_opt %t/channel-tuple.mlir 2>&1 | %FileCheck %s --check-prefix=CHANNEL-TUPLE
// RUN: %not %acir_opt %t/channel-function.mlir 2>&1 | %FileCheck %s --check-prefix=CHANNEL-FUNCTION
// RUN: %not %acir_opt %t/channel-type-attr.mlir 2>&1 | %FileCheck %s --check-prefix=CHANNEL-TYPE-ATTR
// RUN: %not %acir_opt %t/channel-composite-attr.mlir 2>&1 | %FileCheck %s --check-prefix=CHANNEL-COMPOSITE-ATTR
// RUN: %not %acir_opt %t/rate-numerator.mlir 2>&1 | %FileCheck %s --check-prefix=RATE-NUMERATOR
// RUN: %not %acir_opt %t/rate-denominator.mlir 2>&1 | %FileCheck %s --check-prefix=RATE-DENOMINATOR
// RUN: %not %acir_opt %t/duration-data-unit.mlir 2>&1 | %FileCheck %s --check-prefix=DURATION-DATA
// RUN: %not %acir_opt %t/unknown-unit.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN-UNIT
// RUN: %not %acir_opt %t/malformed-named.mlir 2>&1 | %FileCheck %s --check-prefix=MALFORMED-NAMED
// RUN: %not %acir_opt %t/malformed-aggregate.mlir 2>&1 | %FileCheck %s --check-prefix=MALFORMED-AGGREGATE
// RUN: %not %acir_opt %t/malformed-topology.mlir 2>&1 | %FileCheck %s --check-prefix=MALFORMED-TOPOLOGY
// RUN: %not %acir_opt %t/union-non-symbol.mlir 2>&1 | %FileCheck %s --check-prefix=UNION-PARAM
// RUN: %not %acir_opt %t/address-non-symbol.mlir 2>&1 | %FileCheck %s --check-prefix=ADDRESS-PARAM

// VECTOR-ZERO: error: vector length must be positive
// VECTOR-NEGATIVE: error: vector length must be positive
// VECTOR-MINIMUM: error: vector length must be positive
// VECTOR-OVERFLOW: error: failed to parse ACIR_VectorType parameter 'length'
// VECTOR-UNDERFLOW: error: failed to parse ACIR_VectorType parameter 'length'
// CHANNEL-NESTED: error: channel types cannot be nested inside value types
// CHANNEL-STANDALONE: error: channel type is only permitted in an ac.interface channel declaration
// CHANNEL-TUPLE: topology type '!ac.channel<i8, @ready_valid>' cannot be nested inside 'tuple<i8, tuple<!ac.channel<i8, @ready_valid>>>'
// CHANNEL-FUNCTION: channel type is only permitted in an ac.interface channel declaration
// CHANNEL-TYPE-ATTR: error: channel type is only permitted in an ac.interface channel declaration
// CHANNEL-COMPOSITE-ATTR: topology type '!ac.channel<i8, @ready_valid>' cannot be nested inside 'tuple<i8, !ac.channel<i8, @ready_valid>>'
// RATE-NUMERATOR: error: rate numerator must be a data unit
// RATE-DENOMINATOR: error: rate denominator must be a time unit
// DURATION-DATA: error: duration requires a time unit
// UNKNOWN-UNIT: error: failed to parse ACIR_DurationType parameter 'unit'
// MALFORMED-NAMED: error: failed to parse ACIR_StructType parameter 'name'
// MALFORMED-AGGREGATE: error: failed to parse ACIR_OptionalType parameter 'elementType'
// MALFORMED-TOPOLOGY: error: expected ','

//--- vector-zero.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.vector<0 x i8>
}

//--- vector-negative.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.vector<-2 x i8>
}

//--- vector-overflow.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.vector<9223372036854775808 x i8>
}

//--- vector-minimum.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.vector<-9223372036854775808 x i8>
}

//--- vector-underflow.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.vector<-9223372036854775809 x i8>
}

//--- channel-nested.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.optional<!ac.channel<i8, @ready_valid>>
}

//--- channel-standalone.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.channel<i8, @ready_valid>
}

//--- channel-tuple.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> tuple<i8, tuple<!ac.channel<i8, @ready_valid>>>
}

//--- channel-function.mlir
builtin.module attributes {
  ac.contract_epoch = "0.4",
  test.signature = (i8) -> !ac.channel<i8, @ready_valid>
} {
}

//--- channel-type-attr.mlir
builtin.module attributes {
  ac.contract_epoch = "0.4",
  test.type = !ac.channel<i8, @ready_valid>
} {
}

//--- channel-composite-attr.mlir
builtin.module attributes {
  ac.contract_epoch = "0.4",
  test.types = [tuple<i8, !ac.channel<i8, @ready_valid>>]
} {
}

//--- rate-numerator.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.rate<cycles, cycles>
}

//--- rate-denominator.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.rate<bytes, packets>
}

//--- duration-data-unit.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.duration<bytes>
}

//--- unknown-unit.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.duration<femtoseconds>
}

//--- malformed-named.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.struct<i8>
}

//--- malformed-aggregate.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.optional<>
}

//--- malformed-topology.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.flow<i8>
}

//--- union-non-symbol.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.union<i32>
}
// UNION-PARAM: error: invalid kind of attribute specified

//--- address-non-symbol.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !ac.address<i32>
}
// ADDRESS-PARAM: error: invalid kind of attribute specified
