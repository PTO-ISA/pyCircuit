// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/negative-array.mlir 2>&1 | %FileCheck %s --check-prefix=NEGATIVE
// RUN: %not %acir_opt %t/rank-zero-array.mlir 2>&1 | %FileCheck %s --check-prefix=RANK-ZERO
// RUN: %not %acir_opt %t/excessive-array.mlir 2>&1 | %FileCheck %s --check-prefix=EXCESSIVE
// RUN: %not %acir_opt %t/malformed-port.mlir 2>&1 | %FileCheck %s --check-prefix=PORT
// RUN: %not %acir_opt %t/malformed-pc.mlir 2>&1 | %FileCheck %s --check-prefix=PC
// RUN: %not %acir_opt %t/malformed-ref.mlir 2>&1 | %FileCheck %s --check-prefix=REF
// RUN: %not %acir_opt %t/malformed-resource.mlir 2>&1 | %FileCheck %s --check-prefix=RESOURCE

// NEGATIVE: error: array extents must be non-negative
// RANK-ZERO: error: array shape must have at least one extent
// EXCESSIVE: error: array volume exceeds ACSim capability 1048576
// PORT: error: expected ','
// PC: error: invalid kind of attribute specified

//--- negative-array.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !acsim.array<[2, -1], !acsim.owner<@b>>
}

//--- excessive-array.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !acsim.array<[1048576, 1048576], !acsim.owner<@b>>
}

//--- rank-zero-array.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !acsim.array<[], !acsim.owner<@b>>
}

//--- malformed-port.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !acsim.port<@stream, @producer>
}

//--- malformed-pc.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !acsim.pc<i32>
}

//--- malformed-ref.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !acsim.ref<i32>
}
// REF: error: invalid kind of attribute specified

//--- malformed-resource.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "builtin.unrealized_conversion_cast"() : () -> !acsim.resource<@rk>
}
// RESOURCE: error: expected ','
