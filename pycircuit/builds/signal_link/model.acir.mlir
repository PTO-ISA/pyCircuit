module attributes {acir.config = {enabled = true}, acir.model = {config = [{name = "enabled", type = "bool"}], constants = {}, exports = {}, format_version = 8 : i64, top = "SignalLink", type_order = [], types = {}}} {
  func.func @ac_init_top.producer.offset(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.producer.offset", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.producer.count(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.producer.count", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc2)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.reader.last(%arg0: i1) -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.reader.last", params = [{id = "cfg0", name = "enabled", targets = [], type = "bool"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc3)
    return %c0_i32 : i32
  }
  acir.module @top attributes {acir.instance = {definition = "src_signal_link_6cc8a7a9.SignalLink", inputs = {}, members = {}, name = "top", parent, rules = ["configure"]}} {
    acir.contract {relations = [], rules = ["configure"]}
    acir.signal @top.result : i32 {name = "top.result", type = "u32"}
    func.func private @ac_init_top.producer.offset(i1) -> i32
    func.func private @ac_init_top.producer.count(i1) -> i32
    func.func private @ac_init_top.reader.last(i1) -> i32
    acir.rule @top_body.0.call.1.2 : () -> () {acir.function = {contract = "Module:top", definition = "configure", instance = "top", interface = "Module:top:configure", kind = "rule", name = "top_body.0.call.1.2", output_type = "void", owner = "top", params = [], public = true, result = "void", rule = "configure"}} body {
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc4)
      acir.rule.call @top.producer::@top_body.0.call.1.2.0.call.1.3(%c4_i32) {acir.id = "v4", acir.source_type = "void", contract = "Module:top.producer", instance = "top.producer", interface = "Module:top.producer:configure", role = "rule", rule = "configure"} : (i32) -> () loc(#loc5)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %0 = acir.config "enabled" {acir.id = "v1", acir.source_type = "bool"} : i1 loc(#loc6)
      %1 = acir.export %0 {acir.id = "v2", acir.source_type = "bool"} : i1 -> i1
      acir.rule.call @top::@top_body.0.call.1.2() {acir.id = "v3", acir.source_type = "void", contract = "Module:top", instance = "top", interface = "Module:top:configure", role = "rule", rule = "configure"} : () -> () loc(#loc7)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.producer attributes {acir.instance = {definition = "src_signal_link_6cc8a7a9.Producer", inputs = {}, members = {}, name = "top.producer", parent = "top", rules = ["advance", "configure", "peek"]}} {
    acir.contract {relations = [], rules = ["advance", "configure", "peek"]}
    acir.cell @top.producer.offset : i32 {initial_fn = "ac_init_top.producer.offset", name = "top.producer.offset", type = "u32"}
    acir.cell @top.producer.count : i32 {initial_fn = "ac_init_top.producer.count", name = "top.producer.count", type = "u32"}
    func.func private @ac_init_top.producer.offset(i1) -> i32
    func.func private @ac_init_top.producer.count(i1) -> i32
    func.func private @ac_init_top.reader.last(i1) -> i32
    acir.rule @top.producer_body.0.call.0.11 : () -> i32 {acir.function = {contract = "Module:top.producer", definition = "advance", instance = "top.producer", interface = "Module:top.producer:advance", kind = "rule", name = "top.producer_body.0.call.0.11", output_type = "signal<u32>", owner = "top.producer", params = [], public = true, result = "u32", rule = "advance"}} body {
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc8)
      %0 = acir.ref @top.producer::@top.producer.count {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc9)
      %2 = acir.ref @top.producer::@top.producer.offset {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %3 = acir.cell.read %2 {acir.id = "v6", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc10)
      %4 = arith.addi %1, %3 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc9)
      %5 = acir.cell.read %0 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc11)
      %6 = arith.addi %5, %c1_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc11)
      acir.cell.write %0, %6 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc12)
      acir.return %4 : i32 {acir.id = "v14", acir.source_type = "void"} loc(#loc13)
    }
    acir.rule @top_body.0.call.1.2.0.call.1.3 : (i32) -> () {acir.function = {contract = "Module:top.producer", definition = "configure", instance = "top.producer", interface = "Module:top.producer:configure", kind = "rule", name = "top_body.0.call.1.2.0.call.1.3", output_type = "void", owner = "top.producer", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "configure"}} body {
    ^bb0(%arg0: i32):
      %0 = acir.ref @top.producer::@top.producer.offset {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc14)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.producer_body", owner = "top.producer", params = [], result = "void"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %c16_i32 = arith.constant {acir.source_type = "u32"} 16 : i32
      %0 = acir.external "top_body" "v2" {acir.source_type = "bool"} : i1
      cf.cond_br %0, ^bb1, ^bb2(%false : i1)
    ^bb1:  // pred: ^bb0
      %1 = acir.ref @top.producer::@top.producer.count {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v4", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc15)
      %3 = arith.cmpi ult, %2, %c16_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc15)
      cf.br ^bb2(%3 : i1)
    ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
      cf.cond_br %4, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %5 = acir.rule.call @top.producer::@top.producer_body.0.call.0.11() {acir.id = "v12", acir.source_type = "u32", contract = "Module:top.producer", instance = "top.producer", interface = "Module:top.producer:advance", output_binding = @top::@top.result, role = "rule", rule = "advance"} : () -> i32 loc(#loc16)
      acir.body.end {acir.id = "v14", acir.source_type = "void"} loc(#loc17)
    ^bb4:  // pred: ^bb2
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.reader attributes {acir.instance = {definition = "src_signal_link_6cc8a7a9.Reader", inputs = {}, members = {}, name = "top.reader", parent = "top", rules = ["observe"]}} {
    acir.contract {relations = [], rules = ["observe"]}
    acir.cell @top.reader.last : i32 {initial_fn = "ac_init_top.reader.last", name = "top.reader.last", type = "u32"}
    func.func private @ac_init_top.producer.offset(i1) -> i32
    func.func private @ac_init_top.producer.count(i1) -> i32
    func.func private @ac_init_top.reader.last(i1) -> i32
    acir.rule @top.reader_body.0.call.3.7 : (i32) -> () {acir.function = {contract = "Module:top.reader", definition = "observe", instance = "top.reader", interface = "Module:top.reader:observe", kind = "rule", name = "top.reader_body.0.call.3.7", output_type = "void", owner = "top.reader", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "observe"}} body {
    ^bb0(%arg0: i32):
      %0 = acir.ref @top.reader::@top.reader.last {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %0, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc18)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.reader_body", owner = "top.reader", params = [], result = "void"}} {
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc19)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc20)
      %0 = acir.ref @top::@top.result {acir.source_type = "signal<u32>"} : !acir.signal<i32>
      %1 = acir.signal.get %0 {acir.id = "v2", acir.source_type = "u32", role = "observe"} : !acir.signal<i32> -> i32 loc(#loc21)
      %2 = arith.addi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc22)
      %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc23)
      scf.if %3 {
        acir.rule.call @top.reader::@top.reader_body.0.call.3.7(%2) {acir.id = "v8", acir.source_type = "void", contract = "Module:top.reader", instance = "top.reader", interface = "Module:top.reader:observe", role = "rule", rule = "observe"} : (i32) -> () loc(#loc24)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":8:42)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":9:41)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":33:40)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":56:28)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":56:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":49:18)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":60:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:25)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:17)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":21:25)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:17)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":22:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":23:9)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":13:9)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":26:24)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:20)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":27:13)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":37:9)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:29)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:28)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":40:17)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":41:20)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":42:17)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/signal_link.py":44:13)
