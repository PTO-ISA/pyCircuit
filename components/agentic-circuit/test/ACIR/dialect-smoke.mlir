// RUN: %split_file %s %t
// RUN: %acir_opt --show-dialects | %FileCheck %s --check-prefix=DIALECTS
// RUN: %acir_opt %t/canonical.mlir | %FileCheck %s --check-prefix=CANONICAL
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/canonical.mlir | %FileCheck %s --check-prefix=CANONICAL
// RUN: %not %acir_opt %t/missing-epoch.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING
// RUN: %not %acir_opt %t/wrong-epoch.mlir 2>&1 | %FileCheck %s --check-prefix=WRONG
// RUN: %not %acir_opt %t/unknown-ac-op.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN-AC

// DIALECTS: Available Dialects: ac,acsim,arith,builtin,cf,dlti,func,index,scf
// CANONICAL: module attributes {ac.contract_epoch = "0.4"}
// MISSING: error: expected top-level 'ac.contract_epoch' string attribute equal to "0.4"
// WRONG: error: expected top-level 'ac.contract_epoch' string attribute equal to "0.4"
// UNKNOWN-AC: error: unregistered operation 'ac.unknown'

//--- canonical.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
}

//--- missing-epoch.mlir
builtin.module {
}

//--- wrong-epoch.mlir
builtin.module attributes {ac.contract_epoch = "0.1"} {
}

//--- unknown-ac-op.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.unknown"() : () -> ()
}
