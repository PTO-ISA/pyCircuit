// RUN: %split_file %s %t
// RUN: %acir_opt --verify-each=false --acir-test-pass-trace --acir-test-raw-depth=512 %t/shallow.mlir -o /dev/null 2>&1 | %FileCheck %s --check-prefix=DEFAULT
// RUN: %acir_opt --verify-each=false --acir-test-pass-trace --acir-test-raw-depth=512 --normalize-ac-file %t/shallow.mlir -o /dev/null 2>&1 | %FileCheck %s --check-prefix=REGISTERED
// RUN: %not %acir_opt --verify-each=false --acir-test-pass-trace --acir-test-raw-depth=513 %t/shallow.mlir -o /dev/null > %t/depth513-default.raw 2>&1
// RUN: /usr/bin/sed -E 's#^.*shallow.mlir:1:1: error:#LOC: error:#' %t/depth513-default.raw > %t/depth513-default.actual
// RUN: /usr/bin/diff -u %t/expected-depth.txt %t/depth513-default.actual
// RUN: %not %acir_opt --verify-each=false --acir-test-pass-trace --acir-test-raw-depth=513 --normalize-ac-file %t/shallow.mlir -o /dev/null > %t/depth513-registered.raw 2>&1
// RUN: /usr/bin/sed -E 's#^.*shallow.mlir:1:1: error:#LOC: error:#' %t/depth513-registered.raw > %t/depth513-registered.actual
// RUN: /usr/bin/diff -u %t/expected-depth.txt %t/depth513-registered.actual
// RUN: %not %acir_opt --verify-each=false --acir-test-pass-trace --acir-test-raw-depth=10000 --acir-test-raw-malformed %t/shallow.mlir -o /dev/null > %t/depth10000-default.raw 2>&1
// RUN: /usr/bin/sed -E 's#^.*shallow.mlir:1:1: error:#LOC: error:#' %t/depth10000-default.raw > %t/depth10000-default.actual
// RUN: /usr/bin/diff -u %t/expected-depth.txt %t/depth10000-default.actual
// RUN: %not %acir_opt --verify-each=false --acir-test-pass-trace --acir-test-raw-depth=10000 --acir-test-raw-malformed --normalize-ac-file %t/shallow.mlir -o /dev/null > %t/depth10000-registered.raw 2>&1
// RUN: /usr/bin/sed -E 's#^.*shallow.mlir:1:1: error:#LOC: error:#' %t/depth10000-registered.raw > %t/depth10000-registered.actual
// RUN: /usr/bin/diff -u %t/expected-depth.txt %t/depth10000-registered.actual

// DEFAULT: enter:acir-test-materialize-raw-depth
// DEFAULT-NEXT: complete:acir-test-materialize-raw-depth
// DEFAULT-NEXT: enter:normalize-ac-file
// DEFAULT-NEXT: complete:normalize-ac-file
// DEFAULT-NEXT: enter:verify-ac-file
// DEFAULT-NEXT: complete:verify-ac-file
// DEFAULT-NEXT: enter:ac-verify-model
// DEFAULT-NEXT: complete:ac-verify-model

// REGISTERED: enter:acir-test-materialize-raw-depth
// REGISTERED-NEXT: complete:acir-test-materialize-raw-depth
// REGISTERED-NEXT: enter:normalize-ac-file
// REGISTERED-NEXT: complete:normalize-ac-file
// REGISTERED-NEXT: enter:verify-ac-file
// REGISTERED-NEXT: complete:verify-ac-file
// REGISTERED-NEXT: enter:normalize-ac-file
// REGISTERED-NEXT: complete:normalize-ac-file
// REGISTERED-NEXT: enter:ac-verify-model
// REGISTERED-NEXT: complete:ac-verify-model

//--- shallow.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {}

//--- expected-depth.txt
enter:acir-test-materialize-raw-depth
complete:acir-test-materialize-raw-depth
enter:normalize-ac-file
LOC: error: whole-model region nesting exceeds ACIR capability limit 512
builtin.module attributes {ac.contract_epoch = "0.5"} {}
^
fail:normalize-ac-file
