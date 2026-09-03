// RUN: rm -rf %t.out
// RUN: %acir_cxxgen %S/driver-stages.mlir --stop-after=acsim-emit-cxx --output-root=%t.out
// RUN: %not grep -R -E "Python(\.h)?|libpython|pybind|importlib|libMLIR|mlir/|dl(open|sym)|co_await|std::function|dynamic_cast|runtime_factory|descriptor_interpreter|schema_(walker|catalog)|catalog_(walker|lookup)|topology_(mutation|builder)|plugin_(loader|registry)" %t.out
// RUN: %not grep -R -E "if.*(Counter|Scheduler|Compute|Link|Sink)" %source_root/compiler/acir/lib/CodeGen

// Generated C++ remains statically typed and dependency-closed.
