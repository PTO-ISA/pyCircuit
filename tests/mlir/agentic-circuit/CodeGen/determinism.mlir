// REQUIRES: system-darwin
// RUN: rm -rf %t.first %t.second %t.provider.o
// RUN: %cxx -std=c++20 -I%source_root/simulator/gfsim/include -c %S/Inputs/extension/extension_provider.cpp -o %t.provider.o
// RUN: %python %S/Inputs/extension/prepare_target.py %S/extension-provider.mlir %t.acsim --compiler %cxx
// RUN: for out in %t.first %t.second; do %acir_cxxgen %t.acsim --stop-after=publish --output-root=$out --frozen-acir=%S/Inputs/extension/frozen.acir --binding-lock=%S/Inputs/extension/extension.binding.json --project-name=project --project-identity=project.example --system-name=system --system-identity=system.example --profile=fast --compiler=%cxx --standard-library=libc++ --abi-mode=default --object-format=mach-o --contract-flag=-std=c++20 --include-root=%source_root/simulator/gfsim/include --include-root=%S/Inputs/extension --provider-input=ac_test --link-input=%t.provider.o --link-input=%binary_root/gfsim/libgfsim.a --link-input=%binary_root/lib/Bindings/libACIRBindings.a %llvm_linker_flags || exit 1; done
// RUN: diff -r %t.first %t.second
// RUN: %t.first/builds/*/bin/model --build-fingerprint > %t.first.fingerprint
// RUN: %t.second/builds/*/bin/model --build-fingerprint > %t.second.fingerprint
// RUN: cmp %t.first.fingerprint %t.second.fingerprint

// The complete immutable build, including model plan inputs, generated source,
// compile plan, manifest, executable, and embedded fingerprint, is identical.
