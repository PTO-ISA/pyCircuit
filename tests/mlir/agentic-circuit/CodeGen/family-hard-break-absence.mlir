// RUN: %not rg -n 'specializationKey|moduleSpecializations|"module_specializations"|"specialization"[[:space:]]*:' %source_root/compiler/acir %source_root/compiler/mlir
// RUN: %not rg -n '__ac_fanout|__fanout|__local|__ac_instance_|__capture|__pyc_(match|choice)|__p[[:xdigit:]]{8,}' %source_root/python/agentic-circuit/src/agentic_circuit %source_root/compiler/acir
// RUN: %not rg -n 'ac\.static_type_bindings|ac\.static_type_checks|ac\.static_config_bindings|interfaces/modules|types\.ac' %source_root/python/agentic-circuit/src/agentic_circuit %source_root/compiler/acir %source_root/compiler/mlir

module {
}
