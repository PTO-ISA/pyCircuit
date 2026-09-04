// RUN: %split_file %s %t
// RUN: %acir_opt --show-dialects | %FileCheck %s --check-prefix=DIALECTS
// RUN: %acir_opt %t/canonical.mlir | %FileCheck %s --check-prefix=CANONICAL
// RUN: %not %acir_opt %t/unknown-acsim-op.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN-ACSIM
// RUN: %not %acir_opt %t/unregistered-dialect.mlir 2>&1 | %FileCheck %s --check-prefix=UNREGISTERED

// DIALECTS: Available Dialects: ac,acsim,arith,builtin,cf,dlti,func,index,scf
// CANONICAL: module attributes {ac.contract_epoch = "0.5"}
// UNKNOWN-ACSIM: error: unregistered operation 'acsim.unknown'
// UNREGISTERED: error: operation being parsed with an unregistered dialect

//--- canonical.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
}

//--- unknown-acsim-op.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "acsim.unknown"() : () -> ()
}

//--- unregistered-dialect.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "gpu.unknown"() : () -> ()
}
