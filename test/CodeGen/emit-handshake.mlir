// RUN: rm -rf %t.out %t.frozen
// RUN: %acir_opt_public --verify-each=false --pass-pipeline='builtin.module(ac-freeze-topology)' %S/../../examples/handshake/model.mlir -o %t.frozen
// RUN: %acir_opt_public --ac-lower-to-acsim --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu --acsim-output-dir=%t.out %t.frozen -o /dev/null
// RUN: %FileCheck %s --input-file=%t.out/include/generated/model.h --check-prefix=HDR
// RUN: %FileCheck %s --input-file=%t.out/src/generated/model.cpp --check-prefix=SRC

// HDR: enum class Pc : std::uint8_t {entry, s1};
// SRC: proposedPc_ = Pc::s1;
// SRC: case Pc::s1:
