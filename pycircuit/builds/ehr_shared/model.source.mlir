module attributes {acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "EHRShared", type_order = [], types = {}}} {
  func.func @ac_init_top.storage.state() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.storage.state", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc1)
    return %c1_i32 : i32
  }
  func.func @ac_init_top.first_seen() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.first_seen", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc2)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.second_seen() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.second_seen", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc3)
    return %c0_i32 : i32
  }
  acir.module @top attributes {acir.instance = {definition = "src_ehr_shared_32e4aeac.EHRShared", name = "top", parent, rules = ["first", "second"]}} {
    acir.contract {relations = [], rules = ["first", "second"]}
    acir.cell @top.first_seen : i32 {initial_fn = "ac_init_top.first_seen", name = "top.first_seen", type = "u32"}
    acir.cell @top.second_seen : i32 {initial_fn = "ac_init_top.second_seen", name = "top.second_seen", type = "u32"}
    func.func private @ac_init_top.storage.state() -> i32
    func.func private @ac_init_top.first_seen() -> i32
    func.func private @ac_init_top.second_seen() -> i32
    acir.rule @top_body.0.call.2.3 : (i32) -> () {acir.function = {contract = "Module:top", definition = "first", instance = "top", interface = "Module:top:first", kind = "rule", name = "top_body.0.call.2.3", output_type = "void", owner = "top", params = [{id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "first"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.first_seen {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc4)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.3.4 : (i32) -> () {acir.function = {contract = "Module:top", definition = "second", instance = "top", interface = "Module:top:second", kind = "rule", name = "top_body.0.call.3.4", output_type = "void", owner = "top", params = [{id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "second"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc5)
      %0 = arith.addi %arg0, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc6)
      %1 = acir.ref @top::@top.second_seen {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %1, %0 {acir.id = "v8", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc7)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      acir.rule.call @top.storage::@top_body.0.call.0.1() {acir.id = "v2", acir.source_type = "void", contract = "Module:top.storage", instance = "top.storage", interface = "Module:top.storage:late", role = "rule", rule = "late"} : () -> () loc(#loc8)
      %0 = acir.rule.call @top.storage::@top_body.0.call.1.2() {acir.id = "v3", acir.source_type = "u32", contract = "Module:top.storage", instance = "top.storage", interface = "Module:top.storage:read", role = "rule", rule = "read"} : () -> i32 loc(#loc9)
      acir.rule.call @top::@top_body.0.call.2.3(%0) {acir.id = "v4", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:first", role = "rule", rule = "first"} : (i32) -> () loc(#loc10)
      acir.rule.call @top::@top_body.0.call.3.4(%0) {acir.id = "v5", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:second", role = "rule", rule = "second"} : (i32) -> () loc(#loc11)
      acir.rule.call @top.storage::@top_body.0.call.4.5() {acir.id = "v6", acir.source_type = "void", contract = "Module:top.storage", instance = "top.storage", interface = "Module:top.storage:early", role = "rule", rule = "early"} : () -> () loc(#loc12)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.storage attributes {acir.instance = {definition = "src_ehr_shared_32e4aeac.Storage", name = "top.storage", parent = "top", rules = ["early", "late", "read"]}} {
    acir.contract {relations = [{after = "read", before = "early", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_shared.py", line = 10 : i64}, relation = "SB", source = "user"}, {after = "late", before = "early", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_shared.py", line = 10 : i64}, relation = "SB", source = "user"}, {after = "late", before = "read", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/ehr_shared.py", line = 14 : i64}, relation = "SB", source = "user"}], rules = ["early", "late", "read"]}
    acir.ehr @top.storage.state : i32 {initial_fn = "ac_init_top.storage.state", name = "top.storage.state", type = "u32"}
    func.func private @ac_init_top.storage.state() -> i32
    func.func private @ac_init_top.first_seen() -> i32
    func.func private @ac_init_top.second_seen() -> i32
    acir.rule @top_body.0.call.0.1 : () -> () {acir.function = {contract = "Module:top.storage", definition = "late", instance = "top.storage", interface = "Module:top.storage:late", kind = "rule", name = "top_body.0.call.0.1", output_type = "void", owner = "top.storage", params = [], public = true, result = "void", rule = "late"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %c9_i32 = arith.constant {acir.source_type = "u32"} 9 : i32 loc(#loc13)
      %0 = acir.ref @top.storage::@top.storage.state {acir.source_type = "ehr<u32>"} : !acir.ehr<i32>
      acir.ehr.write %0, %c9_i32 {acir.id = "v6", acir.source_type = "void"} : !acir.ehr<i32>, i32 loc(#loc14)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.1.2 : () -> i32 {acir.function = {contract = "Module:top.storage", definition = "read", instance = "top.storage", interface = "Module:top.storage:read", kind = "rule", name = "top_body.0.call.1.2", output_type = "u32", owner = "top.storage", params = [], public = true, result = "u32", rule = "read"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.storage::@top.storage.state {acir.source_type = "ehr<u32>"} : !acir.ehr<i32>
      %2 = acir.ehr.read %1 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.ehr<i32> -> i32 loc(#loc15)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %3 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %3 {acir.source_type = "void"}
      acir.return %2 : i32 {acir.source_type = "void"}
    }
    acir.rule @top_body.0.call.4.5 : () -> () {acir.function = {contract = "Module:top.storage", definition = "early", instance = "top.storage", interface = "Module:top.storage:early", kind = "rule", name = "top_body.0.call.4.5", output_type = "void", owner = "top.storage", params = [], public = true, result = "void", rule = "early"}} body {
      %true = arith.constant {acir.source_type = "bool"} true
      %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc16)
      %0 = acir.ref @top.storage::@top.storage.state {acir.source_type = "ehr<u32>"} : !acir.ehr<i32>
      acir.ehr.write %0, %c7_i32 {acir.id = "v6", acir.source_type = "void"} : !acir.ehr<i32>, i32 loc(#loc17)
      acir.return {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":8:40)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":27:46)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":28:47)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":32:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":36:31)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":36:23)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":36:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":39:9)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":40:18)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":41:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":42:9)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":43:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":20:17)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":20:9)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":16:16)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":12:17)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ehr_shared.py":12:9)
