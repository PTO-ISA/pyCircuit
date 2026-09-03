// RUN: rm -rf %t.out
// RUN: %acir_cxxgen %source_root/tests/mlir/agentic-circuit/ACSim/reusable-modules.mlir --stop-after=link --output-root=%t.out --project-name=project --project-identity=project.example --system-name=system --system-identity=system.example --profile=fast --compiler=%cxx --standard-library=libc++ --abi-mode=default --object-format=mach-o --contract-flag=-std=c++20 --include-root=%source_root/simulator/gfsim/include --include-root=%S/Inputs/nested --link-input=%binary_root/gfsim/libgfsim.a --link-input=%binary_root/lib/Bindings/libACIRBindings.a %llvm_linker_flags | %FileCheck %s
// RUN: %t.out/bin/model

// CHECK: stage=link status=passed
