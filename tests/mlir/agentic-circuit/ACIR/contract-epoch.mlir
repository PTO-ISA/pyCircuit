// RUN: %split_file %s %t
// RUN: %acir_opt %t/valid.mlir -o /dev/null
// RUN: %not %acir_opt %t/legacy.mlir 2>&1 | %FileCheck %s --check-prefix=LEGACY
// RUN: %not %acir_opt %t/missing.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING

//--- valid.mlir
module attributes {ac.contract_epoch = "0.5"} {
}

//--- legacy.mlir
module attributes {ac.contract_epoch = "0.4"} {
}

// LEGACY: expected top-level 'ac.contract_epoch' string attribute equal to "0.5"

//--- missing.mlir
module {
}

// MISSING: expected top-level 'ac.contract_epoch' string attribute equal to "0.5"
