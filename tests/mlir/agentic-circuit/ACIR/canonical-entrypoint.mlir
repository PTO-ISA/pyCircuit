// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/generic.mlir 2>&1 | %FileCheck %s --check-prefix=GENERIC
// RUN: %acir_opt_public %t/canonical.mlir | %FileCheck %s --check-prefix=CANONICAL
// RUN: %acir_opt %t/canonical.mlir --emit-bytecode -o %t/canonical.mlirbc
// RUN: %acir_opt_public %t/canonical.mlirbc > /dev/null
// RUN: %acir_opt %t/internal-provider.mlir > /dev/null
// RUN: %not %acir_opt_public %t/internal-provider.mlir 2>&1 | %FileCheck %s --check-prefix=PROVIDER
// RUN: %not %acir_opt_public %t/escaped-ac.mlir 2>&1 | %FileCheck %s --check-prefix=ESCAPED-AC
// RUN: %not %acir_opt_public %t/mixed-escaped-ac.mlir 2>&1 | %FileCheck %s --check-prefix=ESCAPED-AC
// RUN: %not %acir_opt_public %t/escaped-non-ac.mlir 2>&1 | %FileCheck %s --check-prefix=NON-AC --implicit-check-not=internal-only
// RUN: %not %acir_opt_public %t/malformed-escape.mlir 2>&1 | %FileCheck %s --check-prefix=MALFORMED

//--- generic.mlir
builtin.module  {
  "ac.module"() <{sym_name = "Top", function_type = () -> (), static_params = {}}> ({
    "ac.return"() : () -> ()
  }) : () -> ()
}
// GENERIC: requires attribute 'schema'

//--- canonical.mlir
module {
  ac.module @Top
      source #ac.source_owner<"tests/native_family.py", "tests/native_family.py">
      schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> ()
        source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
      ac.return
    }
  } {label = "ac.fake"}
}
// CANONICAL: ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema

//--- internal-provider.mlir
module  {
  ac.module.extern @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []>
      implementation {registry = "cpp", name = "Leaf"}
}
// PROVIDER: structural provider 'cpp:Leaf' is not registered

//--- escaped-ac.mlir
module  {
  "\61c.module"() <{sym_name = "Top", function_type = () -> (), static_params = {}}> ({
    "ac.return"() : () -> ()
  }) : () -> ()
}
// ESCAPED-AC: generic ACIR operation spelling is internal-only

//--- mixed-escaped-ac.mlir
module  {
  "\61\63.module"() <{sym_name = "Top", function_type = () -> (), static_params = {}}> ({
    "ac.return"() : () -> ()
  }) : () -> ()
}

//--- escaped-non-ac.mlir
module  {
  "\62c.fake"() : () -> ()
}
// NON-AC: error:

//--- malformed-escape.mlir
module  {
  "\6Gc.module"() : () -> ()
}
// MALFORMED: malformed quoted operation name escape
