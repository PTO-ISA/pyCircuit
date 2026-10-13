module attributes {acir.model = {config = [], constants = {}, exports = {}, format_version = 8 : i64, top = "CellFeedback", type_order = [], types = {}}} {
  func.func @ac_init_top.a() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.a", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc1)
    return %c1_i32 : i32
  }
  func.func @ac_init_top.b() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.b", params = [], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc2)
    return %c2_i32 : i32
  }
  acir.module @top attributes {acir.instance = {definition = "src_dependency_cycles_63d6eeaf.CellFeedback", inputs = {}, members = {}, name = "top", parent, rules = ["update_a", "update_b"]}} {
    acir.contract {relations = [], rules = ["update_a", "update_b"]}
    acir.cell @top.a : i32 {initial_fn = "ac_init_top.a", name = "top.a", type = "u32"}
    acir.cell @top.b : i32 {initial_fn = "ac_init_top.b", name = "top.b", type = "u32"}
    func.func private @ac_init_top.a() -> i32
    func.func private @ac_init_top.b() -> i32
    acir.rule @top_body.0.call.0.0 : () -> () {acir.function = {contract = "Module:top", definition = "update_a", instance = "top", interface = "Module:top:update_a", kind = "rule", name = "top_body.0.call.0.0", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "update_a"}} body {
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc3)
      %0 = acir.ref @top::@top.b {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc4)
      %2 = arith.addi %1, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc4)
      %3 = acir.ref @top::@top.a {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %3, %2 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc5)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.1.1 : () -> () {acir.function = {contract = "Module:top", definition = "update_b", instance = "top", interface = "Module:top:update_b", kind = "rule", name = "top_body.0.call.1.1", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "update_b"}} body {
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc6)
      %0 = acir.ref @top::@top.a {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc7)
      %2 = arith.addi %1, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc7)
      %3 = acir.ref @top::@top.b {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %3, %2 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc8)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      acir.rule.call @top::@top_body.0.call.0.0() {acir.id = "v1", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:update_a", role = "rule", rule = "update_a"} : () -> () loc(#loc9)
      acir.rule.call @top::@top_body.0.call.1.1() {acir.id = "v2", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:update_b", role = "rule", rule = "update_b"} : () -> () loc(#loc10)
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":8:37)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":9:37)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:17)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:13)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":13:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:17)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:13)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":17:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":20:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/dependency_cycles.py":21:9)
