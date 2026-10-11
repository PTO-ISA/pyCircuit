module attributes {acir.ehr = [], acir.model = {config = [], constants = {}, contracts = [{guards = {push = ["query.full"]}, id = "Queue", methods = ["read", "revise", "pop", "push", "query.empty", "query.full", "query.size"], relations = [{after = "revise", before = "read", origin = "Pipeline FIFO", relation = "SB", source = "default"}, {after = "pop", before = "revise", origin = "Pipeline FIFO", relation = "SB", source = "default"}], views = {full = {stages = ["pop", "query.full"], visibility = "ehr"}}}, {id = "EHR", methods = ["read", "write"], relations = []}, {id = "Signal", methods = ["set", "get"], relations = [{after = "get", before = "set", origin = "Signal current value", relation = "SB", source = "default"}]}, {id = "Cell", methods = ["read", "write"], relations = []}, {id = "Module:top", methods = [], relations = []}], exports = {a = "a", b = "b"}, format_version = 6 : i64, instances = [{contract = "Module:top", definition = "src_dependency_cycles_63d6eeaf.CellFeedback", inputs = {}, interfaces = {}, members = {a = {resources = ["a"], type = "cell<u32>", value}, b = {resources = ["b"], type = "cell<u32>", value}}, methods = [], name = "top", outputs = [{resources = ["a"], type = "cell<u32>", value}, {resources = ["b"], type = "cell<u32>", value}], parent}], modules = [{name = "top", rules = [{function = "top.update_a", name = "update_a"}, {function = "top.update_b", name = "update_b"}], work = "top_Work"}], top = "CellFeedback", type_order = [], types = {}}} {
  acir.resource "a" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_a", loc = {column = 8 : i64, file = "/home/lc/tmp/pycircuit/examples/dependency_cycles.py", line = 7 : i64}, visibility = "old"}
  acir.resource "b" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_b", loc = {column = 8 : i64, file = "/home/lc/tmp/pycircuit/examples/dependency_cycles.py", line = 8 : i64}, visibility = "old"}
  func.func @ac_init_a() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_a", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc1)
    return %c1_i32 : i32
  }
  func.func @ac_init_b() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_b", params = [], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc2)
    return %c2_i32 : i32
  }
  func.func @top.update_a() attributes {acir.function = {kind = "rule", name = "top.update_a", outputs = [], owner = "top", params = [], result = "void", rule = "update_a"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc3)
    %0 = acir.get "a" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc4)
    %1 = acir.get "b" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc5)
    %2 = acir.read %1 {acir.id = "v3", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc6)
    %3 = arith.addi %2, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc6)
    acir.write %0, %3 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc7)
    return
  }
  func.func @top.update_b() attributes {acir.function = {kind = "rule", name = "top.update_b", outputs = [], owner = "top", params = [], result = "void", rule = "update_b"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc8)
    %0 = acir.get "b" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc5)
    %1 = acir.get "a" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc4)
    %2 = acir.read %1 {acir.id = "v3", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc9)
    %3 = arith.addi %2, %c1_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc9)
    acir.write %0, %3 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc10)
    return
  }
  func.func @top_Work() attributes {acir.function = {kind = "work", name = "top_Work", owner = "top", params = [], result = "void"}} {
    acir.invoke "update_a"() {acir.id = "v1", acir.source_type = "void", callee = "top.update_a"} : () -> () loc(#loc11)
    acir.invoke "update_b"() {acir.id = "v2", acir.source_type = "void", callee = "top.update_b"} : () -> () loc(#loc12)
    return
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

