// RUN: rm -rf %t.out
// RUN: %acir_cxxgen %source_root/tests/mlir/agentic-circuit/ACSim/ops-valid.mlir --stop-after=link --output-root=%t.out --project-name=project --project-identity=project.example --system-name=system --system-identity=system.example --profile=fast --compiler=%cxx --standard-library=libc++ --abi-mode=default --object-format=mach-o --contract-flag=-std=c++20 --include-root=%source_root/simulator/gfsim/include --include-root=%S/Inputs/full --link-input=%binary_root/gfsim/libgfsim.a --link-input=%binary_root/lib/Bindings/libACIRBindings.a %llvm_linker_flags | %FileCheck %s --check-prefix=LINK
// RUN: grep -F "static_assert(gfsim::StatefulModel<gfsim::Fifo>)" %t.out/include/generated/modules/Top_s2100000000000000.h
// RUN: grep -F "bindStatic(lanes_[0].output(), lanes_[1].input())" %t.out/src/generated/modules/Top_s2100000000000000.cpp
// RUN: grep -F "switch (static_cast<Pc>(pc))" %t.out/src/generated/processes/tick_s2300000000000000.cpp
// RUN: %t.out/bin/model --build-fingerprint | %FileCheck %s --check-prefix=FINGERPRINT

// LINK: stage=link status=passed
// FINGERPRINT: sha256:
