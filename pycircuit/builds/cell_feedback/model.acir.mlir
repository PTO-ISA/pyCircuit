module attributes {acir.model = {config = [], constants = {}, exports = {a = "a", b = "b"}, format_version = 7 : i64, top = "CellFeedback", type_order = [], types = {}}} {
  func.func @ac_init_a() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_a", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc1)
    return %c1_i32 : i32
  }
  func.func @ac_init_b() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_b", params = [], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc2)
    return %c2_i32 : i32
  }
  acir.module @top attributes {acir.instance = {contract = "Module:top", definition = "src_dependency_cycles_63d6eeaf.CellFeedback", inputs = {}, interfaces = {}, members = {a = {resources = ["a"], type = "cell<u32>", value}, b = {resources = ["b"], type = "cell<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["a"], type = "cell<u32>", value}, {resources = ["b"], type = "cell<u32>", value}], parent}} {
    acir.cell @a : i32 {initial_fn = "ac_init_a", name = "a", type = "u32"}
    acir.cell @b : i32 {initial_fn = "ac_init_b", name = "b", type = "u32"}
    func.func private @ac_init_a() -> i32
    func.func private @ac_init_b() -> i32
    acir.rule @top.update_a : () -> () {acir.function = {kind = "rule", name = "top.update_a", owner = "top", params = [], result = "void", rule = "update_a"}, output_types = []} body {
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc3)
      %0 = acir.ref @top::@a {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc4)
      %1 = acir.ref @top::@b {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc5)
      %2 = acir.cell.read %1 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc6)
      %3 = arith.addi %2, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc6)
      acir.cell.write %0, %3 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc7)
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.rule @top.update_b : () -> () {acir.function = {kind = "rule", name = "top.update_b", owner = "top", params = [], result = "void", rule = "update_b"}, output_types = []} body {
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc8)
      %0 = acir.ref @top::@b {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc5)
      %1 = acir.ref @top::@a {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc4)
      %2 = acir.cell.read %1 {acir.id = "v5", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc9)
      %3 = arith.addi %2, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc9)
      acir.cell.write %0, %3 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc10)
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
      acir.rule.call @top::@top.update_a() {acir.id = "v1", acir.source_type = "void", output_bindings = []} : () -> () loc(#loc11)
      acir.rule.call @top::@top.update_b() {acir.id = "v2", acir.source_type = "void", output_bindings = []} : () -> () loc(#loc12)
      acir.work.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":7:33)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":8:33)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":12:29)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":7:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":8:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":12:19)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":12:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":16:29)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":16:19)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":16:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":18:5)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":19:5)

