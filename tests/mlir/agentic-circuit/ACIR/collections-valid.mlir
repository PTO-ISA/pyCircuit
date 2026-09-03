// RUN: %acir_opt %s | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.system"() <{sym_name = "collections", root = @Top, root_name = "root", tick_epoch = 0 : i64, tick_unit = "cycle", seed_policy = {kind = "fixed", value = 0 : i64}, instrumentation = [], result_schema = {id = "default", format = "json"}, selected = true}> : () -> ()
  "ac.module"() <{sym_name = "Leaf", function_type = (i32) -> i32, static_params = {}}> ({
  ^bb0(%arg0 : i32):
    "ac.return"(%arg0) : (i32) -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "Leaf2", function_type = (i32) -> i32, static_params = {}}> ({
  ^bb0(%arg0 : i32):
    "ac.return"(%arg0) : (i32) -> ()
  }) : () -> ()
  "ac.module"() <{sym_name = "Top", function_type = (i32, i32) -> (i32, i32), static_params = {}}> ({
  ^bb0(%a : i32, %b : i32):
    %x:2 = "ac.array"(%a, %b) <{definition = @Leaf, sym_name = "lanes", stable_id = "lanes", path = "lanes", shape = array<i64: 2>, static_args = [{}, {}]}> : (i32, i32) -> (i32, i32)
    %y:2 = "ac.instances"(%x#0, %x#1) <{sym_name = "collection", stable_id = "collection", path = "collection", definitions = [@Leaf, @Leaf2], names = ["a", "b"], stable_ids = ["a", "b"], paths = ["mix_a", "mix_b"], interface = (i32) -> i32, static_args = [{}, {}]}> : (i32, i32) -> (i32, i32)
    %z:2 = "ac.view"(%y#0, %y#1) <{sym_name = "z", kind = "permutation", source_producers = [@collection], source_shapes = [array<i64: 2, 1>], indices = array<i64: 1, 0>, shape = array<i64: 2, 1>}> : (i32, i32) -> (i32, i32)
    %selected = "ac.view"(%y#0, %y#1) <{sym_name = "selected", kind = "select", source_producers = [@collection], source_shapes = [array<i64: 2, 1>], indices = array<i64: 0, 0>, shape = array<i64>}> : (i32, i32) -> i32
    %slice:2 = "ac.view"(%y#0, %y#1) <{sym_name = "slice", kind = "slice", source_producers = [@collection], source_shapes = [array<i64: 2, 1>], indices = array<i64: 0, 2, 0, 1>, shape = array<i64: 2, 1>}> : (i32, i32) -> (i32, i32)
    %concat:4 = "ac.view"(%x#0, %x#1, %y#0, %y#1) <{sym_name = "concat", kind = "concat", source_producers = [@lanes, @collection], source_shapes = [array<i64: 2, 1>, array<i64: 2, 1>], axis = 0 : i64, indices = array<i64>, shape = array<i64: 4, 1>}> : (i32, i32, i32, i32) -> (i32, i32, i32, i32)
    %zip:4 = "ac.view"(%x#0, %x#1, %y#0, %y#1) <{sym_name = "zip", kind = "zip", source_producers = [@lanes, @collection], source_shapes = [array<i64: 2, 1>, array<i64: 2, 1>], indices = array<i64>, shape = array<i64: 2, 1, 2>}> : (i32, i32, i32, i32) -> (i32, i32, i32, i32)
    %bound:4 = "ac.view"(%x#0, %x#1, %y#0, %y#1) <{sym_name = "bound", kind = "elementwise", source_producers = [@lanes, @collection], source_shapes = [array<i64: 2, 1>, array<i64: 2, 1>], indices = array<i64>, shape = array<i64: 2, 1, 2>}> : (i32, i32, i32, i32) -> (i32, i32, i32, i32)
    "ac.return"(%z#0, %z#1) : (i32, i32) -> ()
  }) : () -> ()
}

// CHECK: ac.array
// CHECK: ac.instances
// CHECK: ac.view
