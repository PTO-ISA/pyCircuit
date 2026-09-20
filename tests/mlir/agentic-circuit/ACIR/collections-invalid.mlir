// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/negative-shape.mlir 2>&1 | %FileCheck %s --check-prefix=NEGATIVE
// RUN: %not %acir_opt %t/wrong-cardinality.mlir 2>&1 | %FileCheck %s --check-prefix=CARDINALITY
// RUN: %not %acir_opt %t/heterogeneous-shape.mlir 2>&1 | %FileCheck %s --check-prefix=HETERO
// RUN: %not %acir_opt %t/bad-permutation.mlir 2>&1 | %FileCheck %s --check-prefix=PERMUTE
// RUN: %not %acir_opt %t/duplicate-owned-path.mlir 2>&1 | %FileCheck %s --check-prefix=DUP-OWNED
// RUN: %not %acir_opt %t/too-large.mlir 2>&1 | %FileCheck %s --check-prefix=TOO-LARGE
// RUN: %not %acir_opt %t/view-overflow.mlir 2>&1 | %FileCheck %s --check-prefix=VIEW-OVERFLOW
// RUN: %not %acir_opt %t/view-provenance.mlir 2>&1 | %FileCheck %s --check-prefix=PROVENANCE
// RUN: %not %acir_opt %t/bad-select.mlir 2>&1 | %FileCheck %s --check-prefix=SELECT
// RUN: %not %acir_opt %t/bad-slice.mlir 2>&1 | %FileCheck %s --check-prefix=SLICE
// RUN: %not %acir_opt %t/bad-concat.mlir 2>&1 | %FileCheck %s --check-prefix=CONCAT
// RUN: %not %acir_opt %t/bad-zip.mlir 2>&1 | %FileCheck %s --check-prefix=ZIP
// RUN: %not %acir_opt %t/bad-elementwise.mlir 2>&1 | %FileCheck %s --check-prefix=ELEMENTWISE

//--- negative-shape.mlir
builtin.module  {
  ac.module.extern @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.array"() <{definition = @Leaf, sym_name = "a", stable_id = "a", path = "a", shape = array<i64: -1>, static_args = []}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// NEGATIVE: array shape dimensions must be non-negative

//--- wrong-cardinality.mlir
builtin.module  {
  ac.module.extern @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.array"() <{definition = @Leaf, sym_name = "a", stable_id = "a", path = "a", shape = array<i64: 2>, static_args = [#ac.static_arguments<[]>]}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// CARDINALITY: one concrete static argument set per lexicographically ordered element

//--- heterogeneous-shape.mlir
builtin.module  {
  ac.module @A source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (i32) -> i32 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ^bb0(%value: i32):
      ac.return %value : i32
    }
  }
  ac.module @B source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i64>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i64>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type (i64) -> i64 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    ^bb0(%value: i64):
      ac.return %value : i64
    }
  }
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_1", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> (i32, i32) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%a : i32, %b : i32):
    %x:2 = "ac.instances"(%a, %b) <{sym_name = "collection", stable_id = "collection", path = "collection", definitions = [@A, @B], names = ["a", "b"], stable_ids = ["a", "b"], paths = ["a", "b"], interface = (i32) -> i32, static_args = [#ac.static_arguments<[]>, #ac.static_arguments<[]>]}> : (i32, i32) -> (i32, i32)
    "ac.return"(%x#0, %x#1) : (i32, i32) -> ()

  }
}
}
// HETERO: element case does not implement the exact common interface

//--- bad-permutation.mlir
builtin.module  {
  ac.module @Pair source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_1", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> (i32, i32) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%a : i32, %b : i32):
    "ac.return"(%a, %b) : (i32, i32) -> ()

  }
}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_1", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> (i32, i32) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%a : i32, %b : i32):
    %source:2 = "ac.instance"(%a, %b) <{definition = @Pair, sym_name = "pair", stable_id = "pair", path = "pair", static_args = #ac.static_arguments<[]>}> : (i32, i32) -> (i32, i32)
    %x:2 = "ac.view"(%source#0, %source#1) <{sym_name = "view", kind = "permutation", source_producers = [@pair], source_shapes = [array<i64: 2>], indices = array<i64: 0, 0>, shape = array<i64: 2>}> : (i32, i32) -> (i32, i32)
    "ac.return"(%x#0, %x#1) : (i32, i32) -> ()

  }
}
}
// PERMUTE: permutation indices must be an in-bounds bijection

//--- duplicate-owned-path.mlir
builtin.module  {
  "ac.system"() <{sym_name = "s", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "default", format = "json"}, selected = true}> : () -> ()
  ac.module.extern @A source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "A"}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.instances"() <{sym_name = "collection", stable_id = "collection", path = "collection", definitions = [@A, @A], names = ["a", "b"], stable_ids = ["a", "b"], paths = ["same", "same"], interface = () -> (), static_args = [#ac.static_arguments<[]>, #ac.static_arguments<[]>]}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// DUP-OWNED: collection paths must be stable, unique parent-relative segments

//--- too-large.mlir
builtin.module  {
  ac.module.extern @Leaf source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> implementation {registry = "cpp", name = "Leaf"}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.array"() <{definition = @Leaf, sym_name = "huge", stable_id = "huge", path = "huge", shape = array<i64: 1048577>, static_args = []}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// TOO-LARGE: array cardinality exceeds static elaboration bound 1048576

//--- view-overflow.mlir
builtin.module  {
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type () -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
    "ac.view"() <{sym_name = "view", kind = "concat", source_producers = [@missing], source_shapes = [array<i64: 9223372036854775807, 3>], axis = 0 : i64, indices = array<i64>, shape = array<i64: 9223372036854775807, 3>}> : () -> ()
    "ac.return"() : () -> ()

  }
}
}
// VIEW-OVERFLOW: view cardinality overflows 64 bits

//--- view-provenance.mlir
builtin.module  {
  ac.module @Identity source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32) -> i32 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { ^bb0(%x : i32): "ac.return"(%x) : (i32) -> ()
  }
}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32) -> i32 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%arg : i32):
    %source = "ac.instance"(%arg) <{definition = @Identity, sym_name = "source", stable_id = "source", path = "source", static_args = #ac.static_arguments<[]>}> : (i32) -> i32
    %x = "ac.view"(%arg) <{sym_name = "view", kind = "permutation", source_producers = [@source], source_shapes = [array<i64: 1>], indices = array<i64: 0>, shape = array<i64: 1>}> : (i32) -> i32
    "ac.return"(%x) : (i32) -> ()

  }
}
}
// PROVENANCE: each source must be the complete result group of its declared structural producer

//--- bad-select.mlir
builtin.module  {
  ac.module @Pair source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_1", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> (i32, i32) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { ^bb0(%a : i32, %b : i32): "ac.return"(%a, %b) : (i32, i32) -> ()
  }
}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> i32 source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%a : i32, %b : i32):
    %source:2 = "ac.instance"(%a, %b) <{definition = @Pair, sym_name = "pair", stable_id = "pair", path = "pair", static_args = #ac.static_arguments<[]>}> : (i32, i32) -> (i32, i32)
    %x = "ac.view"(%source#0, %source#1) <{sym_name = "view", kind = "select", source_producers = [@pair], source_shapes = [array<i64: 2>], indices = array<i64: 2>, shape = array<i64>}> : (i32, i32) -> i32
    "ac.return"(%x) : (i32) -> ()

  }
}
}
// SELECT: select coordinate is out of bounds

//--- bad-slice.mlir
builtin.module  {
  ac.module @Pair source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_1", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> (i32, i32) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { ^bb0(%a : i32, %b : i32): "ac.return"(%a, %b) : (i32, i32) -> ()
  }
}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_1", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> (i32, i32) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%a : i32, %b : i32):
    %source:2 = "ac.instance"(%a, %b) <{definition = @Pair, sym_name = "pair", stable_id = "pair", path = "pair", static_args = #ac.static_arguments<[]>}> : (i32, i32) -> (i32, i32)
    %x:2 = "ac.view"(%source#0, %source#1) <{sym_name = "view", kind = "slice", source_producers = [@pair], source_shapes = [array<i64: 2>], indices = array<i64: 0, 3>, shape = array<i64: 3>}> : (i32, i32) -> (i32, i32)
    "ac.return"(%x#0, %x#1) : (i32, i32) -> ()

  }
}
}
// SLICE: slice bounds are invalid

//--- bad-concat.mlir
builtin.module  {
  ac.module @Pair source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_1", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> (i32, i32) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { ^bb0(%a : i32, %b : i32): "ac.return"(%a, %b) : (i32, i32) -> ()
  }
}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%a : i32, %b : i32):
    %left:2 = "ac.instance"(%a, %b) <{definition = @Pair, sym_name = "left", stable_id = "left", path = "left", static_args = #ac.static_arguments<[]>}> : (i32, i32) -> (i32, i32)
    %right:2 = "ac.instance"(%a, %b) <{definition = @Pair, sym_name = "right", stable_id = "right", path = "right", static_args = #ac.static_arguments<[]>}> : (i32, i32) -> (i32, i32)
    %x:4 = "ac.view"(%left#0, %left#1, %right#0, %right#1) <{sym_name = "view", kind = "concat", source_producers = [@left, @right], source_shapes = [array<i64: 2>, array<i64: 2>], axis = 1 : i64, indices = array<i64>, shape = array<i64: 4>}> : (i32, i32, i32, i32) -> (i32, i32, i32, i32)
    "ac.return"() : () -> ()

  }
}
}
// CONCAT: concat axis is out of bounds

//--- bad-zip.mlir
builtin.module  {
  ac.module @Pair source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_1", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> (i32, i32) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { ^bb0(%a : i32, %b : i32): "ac.return"(%a, %b) : (i32, i32) -> ()
  }
}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%a : i32, %b : i32):
    %left:2 = "ac.instance"(%a, %b) <{definition = @Pair, sym_name = "left", stable_id = "left", path = "left", static_args = #ac.static_arguments<[]>}> : (i32, i32) -> (i32, i32)
    %right:2 = "ac.instance"(%a, %b) <{definition = @Pair, sym_name = "right", stable_id = "right", path = "right", static_args = #ac.static_arguments<[]>}> : (i32, i32) -> (i32, i32)
    %x:4 = "ac.view"(%left#0, %left#1, %right#0, %right#1) <{sym_name = "view", kind = "zip", source_producers = [@left, @right], source_shapes = [array<i64: 2>, array<i64: 2>], indices = array<i64>, shape = array<i64: 4>}> : (i32, i32, i32, i32) -> (i32, i32, i32, i32)
    "ac.return"() : () -> ()

  }
}
}
// ZIP: zip result shape must append source count

//--- bad-elementwise.mlir
builtin.module  {
  ac.module @Pair source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_0", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"output_1", "output", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> (i32, i32) source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph { ^bb0(%a : i32, %b : i32): "ac.return"(%a, %b) : (i32, i32) -> ()
  }
}
  ac.module @Top source #ac.source_owner<"tests/native_family.py", "tests/native_family.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[#ac.interface_port<"input_0", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>, #ac.interface_port<"input_1", "input", #ac.type_expr<#ac.type_expr_concrete<i32>>, #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1>>]>, #ac.source_owner<"tests/native_family.py", "tests/native_family.py">, []> {
  ac.module.case arguments #ac.static_arguments<[]> type (i32, i32) -> () source #ac.source_provenance<"tests/native_family.py", 1, 1, 1, 1> graph {
  ^bb0(%a : i32, %b : i32):
    %source:2 = "ac.instance"(%a, %b) <{definition = @Pair, sym_name = "pair", stable_id = "pair", path = "pair", static_args = #ac.static_arguments<[]>}> : (i32, i32) -> (i32, i32)
    %x:2 = "ac.view"(%source#0, %source#1) <{sym_name = "view", kind = "elementwise", source_producers = [@pair], source_shapes = [array<i64: 2>], indices = array<i64>, shape = array<i64: 2>}> : (i32, i32) -> (i32, i32)
    "ac.return"() : () -> ()

  }
}
}
// ELEMENTWISE: elementwise requires at least two sources
