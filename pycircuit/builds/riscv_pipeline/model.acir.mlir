module attributes {acir.ehr = [{aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "decode_stage.out", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "execute.out", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "fetch.out", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "retirement", same_port = "invisible", semantics = "last_effective_prior_port"}], acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, contracts = [{guards = {push = ["query.full"]}, id = "Queue", methods = ["read", "revise", "pop", "push", "query.empty", "query.full", "query.size"], relations = [{after = "revise", before = "read", origin = "Pipeline FIFO", relation = "SB", source = "default"}, {after = "pop", before = "revise", origin = "Pipeline FIFO", relation = "SB", source = "default"}], views = {full = {stages = ["pop", "query.full"], visibility = "ehr"}}}, {id = "EHR", methods = ["read", "write"], relations = []}, {id = "Signal", methods = ["set", "get"], relations = [{after = "get", before = "set", origin = "Signal current value", relation = "SB", source = "default"}]}, {id = "Cell", methods = ["read", "write"], relations = []}, {id = "Module:rf", methods = ["commit", "read", "ready", "reserve"], relations = []}, {id = "Module:memory", methods = ["load", "store"], relations = []}, {id = "Module:fetch", methods = [], relations = []}, {id = "Module:decode_stage", methods = [], relations = []}, {id = "Module:execute", methods = [], relations = []}, {id = "Module:memory_stage", methods = [], relations = []}, {id = "Module:writeback", methods = [], relations = []}, {id = "Module:top", methods = [], relations = []}], exports = {retirement = "retirement"}, format_version = 6 : i64, instances = [{contract = "Module:rf", definition = "src_model_8294833e.RegisterFile", inputs = {}, interfaces = {commit = "Module:rf:commit", read = "Module:rf:read", ready = "Module:rf:ready", reserve = "Module:rf:reserve"}, members = {pending = {resources = ["rf.pending"], type = "cell<array<bool,32>>", value}, registers = {resources = ["rf.registers"], type = "cell<array<u32,32>>", value}}, methods = ["commit", "read", "ready", "reserve"], name = "rf", outputs = {resources = ["rf.registers"], type = "cell<array<u32,32>>", value}, parent = "top"}, {contract = "Module:memory", definition = "src_model_8294833e.DataMemory", inputs = {}, interfaces = {load = "Module:memory:load", store = "Module:memory:store"}, members = {words = {resources = ["memory.words"], type = "cell<array<u32,16>>", value}}, methods = ["load", "store"], name = "memory", outputs = {resources = ["memory.words"], type = "cell<array<u32,16>>", value}, parent = "top"}, {contract = "Module:fetch", definition = "src_model_8294833e.Fetch", inputs = {}, interfaces = {}, members = {out = {resources = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>", value}, pc = {resources = ["fetch.pc"], type = "cell<u32>", value}, rom = {resources = ["fetch.rom"], type = "cell<array<u32,6>>", value}}, methods = [], name = "fetch", outputs = {resources = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>", value}, parent = "top"}, {contract = "Module:decode_stage", definition = "src_model_8294833e.DecodeStage", inputs = {retirement = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, rf = {instance = "rf"}, source = {resources = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>", value}}, interfaces = {}, members = {out = {resources = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>", value}, retirement = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, rf = {instance = "rf"}, source = {resources = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>", value}}, methods = [], name = "decode_stage", outputs = {resources = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>", value}, parent = "top"}, {contract = "Module:execute", definition = "src_model_8294833e.Execute", inputs = {source = {resources = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>", value}}, interfaces = {}, members = {out = {resources = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>", value}, source = {resources = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>", value}}, methods = [], name = "execute", outputs = {resources = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>", value}, parent = "top"}, {contract = "Module:memory_stage", definition = "src_model_8294833e.MemoryStage", inputs = {memory = {instance = "memory"}, out = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, source = {resources = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>", value}}, interfaces = {}, members = {memory = {instance = "memory"}, out = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, source = {resources = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>", value}}, methods = [], name = "memory_stage", outputs = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, parent = "top"}, {contract = "Module:writeback", definition = "src_model_8294833e.Writeback", inputs = {rf = {instance = "rf"}, source = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}}, interfaces = {}, members = {last_pc = {resources = ["writeback.last_pc"], type = "cell<u32>", value}, retired = {resources = ["writeback.retired"], type = "cell<u32>", value}, rf = {instance = "rf"}, source = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}}, methods = [], name = "writeback", outputs = [{resources = ["writeback.retired"], type = "cell<u32>", value}, {resources = ["writeback.last_pc"], type = "cell<u32>", value}], parent = "top"}, {contract = "Module:top", definition = "src_model_8294833e.Pipeline", inputs = {}, interfaces = {}, members = {decode_stage = {instance = "decode_stage"}, execute = {instance = "execute"}, fetch = {instance = "fetch"}, memory = {instance = "memory"}, memory_stage = {instance = "memory_stage"}, retirement = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, rf = {instance = "rf"}, writeback = {instance = "writeback"}}, methods = [], name = "top", outputs = [{resources = ["rf.registers"], type = "cell<array<u32,32>>", value}, {resources = ["memory.words"], type = "cell<array<u32,16>>", value}, [{resources = ["writeback.retired"], type = "cell<u32>", value}, {resources = ["writeback.last_pc"], type = "cell<u32>", value}]], parent}], modules = [{name = "fetch", rules = [{function = "fetch.fetch", name = "fetch"}], work = "fetch_Work"}, {name = "decode_stage", rules = [{function = "decode_stage.read_operands", name = "read_operands"}], work = "decode_stage_Work"}, {name = "execute", rules = [{function = "execute.execute", name = "execute"}], work = "execute_Work"}, {name = "memory_stage", rules = [{function = "memory_stage.access", name = "access"}], work = "memory_stage_Work"}, {name = "writeback", rules = [{function = "writeback.commit", name = "commit"}], work = "writeback_Work"}], top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}, {default = 0 : i64, name = "immediate", type = "u32"}, {default = false, name = "immediate_rhs", type = "bool"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "lhs", type = "u32"}, {default = 0 : i64, name = "rhs", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = false, name = "writes", type = "bool"}]}}} {
  acir.resource "rf.registers" "cell" "array<u32,32>" {contract = "Cell", initial_fn = "ac_init_rf.registers", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 12 : i64}, visibility = "old"}
  acir.resource "rf.pending" "cell" "array<bool,32>" {contract = "Cell", initial_fn = "ac_init_rf.pending", loc = {column = 14 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 16 : i64}, visibility = "old"}
  acir.resource "memory.words" "cell" "array<u32,16>" {contract = "Cell", initial_fn = "ac_init_memory.words", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 51 : i64}, visibility = "old"}
  acir.resource "retirement" "queue" "src_isa_f0424bc8.Retired" {capacity_fn = "ac_init_retirement_cap", contract = "Queue", initial_fn = "ac_init_retirement", loc = {column = 17 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 167 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "fetch.rom" "cell" "array<u32,6>" {contract = "Cell", initial_fn = "ac_init_fetch.rom", loc = {column = 10 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 72 : i64}, visibility = "old"}
  acir.resource "fetch.pc" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_fetch.pc", loc = {column = 9 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 76 : i64}, visibility = "old"}
  acir.resource "fetch.out" "queue" "src_isa_f0424bc8.Fetched" {capacity_fn = "ac_init_fetch.out_cap", contract = "Queue", initial_fn = "ac_init_fetch.out", loc = {column = 10 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 77 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "decode_stage.out" "queue" "src_isa_f0424bc8.Operands" {capacity_fn = "ac_init_decode_stage.out_cap", contract = "Queue", initial_fn = "ac_init_decode_stage.out", loc = {column = 10 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 93 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "execute.out" "queue" "src_isa_f0424bc8.Executed" {capacity_fn = "ac_init_execute.out_cap", contract = "Queue", initial_fn = "ac_init_execute.out", loc = {column = 10 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 117 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "writeback.retired" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_writeback.retired", loc = {column = 14 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 149 : i64}, visibility = "old"}
  acir.resource "writeback.last_pc" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_writeback.last_pc", loc = {column = 14 : i64, file = "/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py", line = 150 : i64}, visibility = "old"}
  func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  func.func @ac_init_rf.registers() -> !acir.array<i32, 32> attributes {acir.function = {kind = "helper", name = "ac_init_rf.registers", params = [], result = "array<u32,32>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v33", acir.source_type = "array<u32,32>"} : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 32> loc(#loc1)
    return %0 : !acir.array<i32, 32>
  }
  func.func @ac_init_rf.pending() -> !acir.array<i1, 32> attributes {acir.function = {kind = "helper", name = "ac_init_rf.pending", params = [], result = "array<bool,32>"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.aggregate(%false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false) {acir.id = "v67", acir.source_type = "array<bool,32>"} : (i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1) -> !acir.array<i1, 32> loc(#loc2)
    return %0 : !acir.array<i1, 32>
  }
  func.func @ac_init_memory.words() -> !acir.array<i32, 16> attributes {acir.function = {kind = "helper", name = "ac_init_memory.words", params = [], result = "array<u32,16>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v85", acir.source_type = "array<u32,16>"} : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 16> loc(#loc3)
    return %0 : !acir.array<i32, 16>
  }
  func.func @ac_init_retirement_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_retirement_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">> attributes {acir.function = {kind = "helper", name = "ac_init_retirement", params = [], result = "vector<src_isa_f0424bc8.Retired>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Retired>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
  }
  func.func @ac_init_fetch.rom() -> !acir.array<i32, 6> attributes {acir.function = {kind = "helper", name = "ac_init_fetch.rom", params = [], result = "array<u32,6>"}} {
    %c7340179_i32 = arith.constant {acir.source_type = "u32"} 7340179 : i32 loc(#loc4)
    %c5243155_i32 = arith.constant {acir.source_type = "u32"} 5243155 : i32 loc(#loc5)
    %c2130355_i32 = arith.constant {acir.source_type = "u32"} 2130355 : i32 loc(#loc6)
    %c3153955_i32 = arith.constant {acir.source_type = "u32"} 3153955 : i32 loc(#loc7)
    %c8707_i32 = arith.constant {acir.source_type = "u32"} 8707 : i32 loc(#loc8)
    %c1075970739_i32 = arith.constant {acir.source_type = "u32"} 1075970739 : i32 loc(#loc9)
    %0 = acir.aggregate(%c7340179_i32, %c5243155_i32, %c2130355_i32, %c3153955_i32, %c8707_i32, %c1075970739_i32) {acir.id = "v95", acir.source_type = "array<u32,6>"} : (i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 6> loc(#loc10)
    return %0 : !acir.array<i32, 6>
  }
  func.func @ac_init_fetch.pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_fetch.pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc11)
    return %c0_i32 : i32
  }
  func.func @ac_init_fetch.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_fetch.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">> attributes {acir.function = {kind = "helper", name = "ac_init_fetch.out", params = [], result = "vector<src_isa_f0424bc8.Fetched>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Fetched>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
  }
  func.func @ac_init_decode_stage.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_decode_stage.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">> attributes {acir.function = {kind = "helper", name = "ac_init_decode_stage.out", params = [], result = "vector<src_isa_f0424bc8.Operands>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Operands>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
  }
  func.func @ac_init_execute.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_execute.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">> attributes {acir.function = {kind = "helper", name = "ac_init_execute.out", params = [], result = "vector<src_isa_f0424bc8.Executed>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Executed>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
  }
  func.func @ac_init_writeback.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_writeback.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc12)
    return %c0_i32 : i32
  }
  func.func @ac_init_writeback.last_pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_writeback.last_pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc13)
    return %c0_i32 : i32
  }
  func.func @decode_stage_Work.0.call.0.2.0.call.0.35(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "decode_stage_Work.0.call.0.2.0.call.0.35", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc14)
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc15)
    return %1 : i32 loc(#loc16)
  }
  func.func @decode_stage_Work.0.call.0.2.0.call.1.85(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "decode_stage_Work.0.call.0.2.0.call.1.85", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc14)
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc15)
    return %1 : i32 loc(#loc16)
  }
  func.func @decode_stage_Work.0.call.0.2.0.call.2.108(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "decode_stage_Work.0.call.0.2.0.call.2.108", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc14)
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc15)
    return %1 : i32 loc(#loc16)
  }
  func.func @decode_stage_Work.0.call.0.2(%arg0: !acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded"> attributes {acir.function = {definition = "src_isa_f0424bc8.decode", kind = "helper", name = "decode_stage_Work.0.call.0.2", params = [{consume = false, id = "v1", name = "packet", targets = [], type = "src_isa_f0424bc8.Fetched"}], result = "src_isa_f0424bc8.Decoded"}} {
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %c35_i32 = arith.constant {acir.source_type = "u32"} 35 : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c32_i32 = arith.constant {acir.source_type = "u32"} 32 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c51_i32 = arith.constant {acir.source_type = "u32"} 51 : i32
    %c19_i32 = arith.constant {acir.source_type = "u32"} 19 : i32 loc(#loc17)
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c20_i32 = arith.constant {acir.source_type = "u32"} 20 : i32
    %c15_i32 = arith.constant {acir.source_type = "u32"} 15 : i32 loc(#loc18)
    %c31_i32 = arith.constant {acir.source_type = "u32"} 31 : i32
    %c25_i32 = arith.constant {acir.source_type = "u32"} 25 : i32
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    %c12_i32 = arith.constant {acir.source_type = "u32"} 12 : i32 loc(#loc19)
    %c127_i32 = arith.constant {acir.source_type = "u32"} 127 : i32 loc(#loc20)
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc21)
    %1 = arith.andi %0, %c127_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc22)
    %2 = call @ac_builtin_shr_u32(%0, %c12_i32) {acir.id = "v6", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc23)
    %3 = arith.andi %2, %c7_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc24)
    %4 = call @ac_builtin_shr_u32(%0, %c25_i32) {acir.id = "v10", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc25)
    %5 = call @ac_builtin_shr_u32(%0, %c7_i32) {acir.id = "v12", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc26)
    %6 = arith.andi %5, %c31_i32 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc27)
    %7 = call @ac_builtin_shr_u32(%0, %c15_i32) {acir.id = "v16", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc28)
    %8 = arith.andi %7, %c31_i32 {acir.id = "v18", acir.source_type = "u32"} : i32 loc(#loc29)
    %9 = call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v20", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc30)
    %10 = arith.andi %9, %c31_i32 {acir.id = "v22", acir.source_type = "u32"} : i32 loc(#loc31)
    %11 = arith.cmpi eq, %1, %c19_i32 {acir.id = "v28", acir.source_type = "bool"} : i32 loc(#loc32)
    cf.cond_br %11, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %12 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v31", acir.source_type = "bool"} : i32 loc(#loc33)
    acir.check %12 {acir.id = "v32", acir.source_type = "void"} loc(#loc34)
    %13 = call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v35", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc35)
    %14 = call @decode_stage_Work.0.call.0.2.0.call.0.35(%13) {acir.id = "v36", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc36)
    cf.br ^bb12(%14, %true, %c0_i32, %arg0, %6, %8, %c0_i32, %0, %true : i32, i1, i32, !acir.struct<"src_isa_f0424bc8.Fetched">, i32, i32, i32, i32, i1)
  ^bb2:  // pred: ^bb0
    %15 = arith.cmpi eq, %1, %c51_i32 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc37)
    cf.cond_br %15, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %16 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v42", acir.source_type = "bool"} : i32 loc(#loc38)
    cf.cond_br %16, ^bb5, ^bb7(%false : i1)
  ^bb4:  // pred: ^bb2
    %17 = arith.cmpi eq, %1, %c3_i32 {acir.id = "v77", acir.source_type = "bool"} : i32 loc(#loc39)
    cf.cond_br %17, ^bb8, ^bb9
  ^bb5:  // pred: ^bb3
    %18 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v45", acir.source_type = "bool"} : i32 loc(#loc40)
    cf.cond_br %18, ^bb7(%true : i1), ^bb6
  ^bb6:  // pred: ^bb5
    %19 = arith.cmpi eq, %4, %c32_i32 {acir.id = "v49", acir.source_type = "bool"} : i32 loc(#loc41)
    cf.br ^bb7(%19 : i1)
  ^bb7(%20: i1):  // 3 preds: ^bb3, ^bb5, ^bb6
    acir.check %20 {acir.id = "v57", acir.source_type = "void"} loc(#loc42)
    %21 = arith.cmpi eq, %4, %c32_i32 {acir.id = "v59", acir.source_type = "bool"} : i32 loc(#loc43)
    cf.cond_br %21, ^bb12(%c0_i32, %false, %c1_i32, %arg0, %6, %8, %10, %0, %true : i32, i1, i32, !acir.struct<"src_isa_f0424bc8.Fetched">, i32, i32, i32, i32, i1), ^bb12(%c0_i32, %false, %c0_i32, %arg0, %6, %8, %10, %0, %true : i32, i1, i32, !acir.struct<"src_isa_f0424bc8.Fetched">, i32, i32, i32, i32, i1)
  ^bb8:  // pred: ^bb4
    %22 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v80", acir.source_type = "bool"} : i32 loc(#loc44)
    acir.check %22 {acir.id = "v81", acir.source_type = "void"} loc(#loc45)
    %23 = call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v85", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc46)
    %24 = call @decode_stage_Work.0.call.0.2.0.call.1.85(%23) {acir.id = "v86", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc47)
    cf.br ^bb12(%24, %true, %c2_i32, %arg0, %6, %8, %c0_i32, %0, %true : i32, i1, i32, !acir.struct<"src_isa_f0424bc8.Fetched">, i32, i32, i32, i32, i1)
  ^bb9:  // pred: ^bb4
    %25 = arith.cmpi eq, %1, %c35_i32 {acir.id = "v89", acir.source_type = "bool"} : i32 loc(#loc48)
    cf.cond_br %25, ^bb10, ^bb11(%false : i1)
  ^bb10:  // pred: ^bb9
    %26 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v92", acir.source_type = "bool"} : i32 loc(#loc49)
    cf.br ^bb11(%26 : i1)
  ^bb11(%27: i1):  // 2 preds: ^bb9, ^bb10
    acir.check %27 {acir.id = "v97", acir.source_type = "void"} loc(#loc50)
    %28 = call @ac_builtin_shr_u32(%0, %c25_i32) {acir.id = "v101", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc51)
    %29 = call @ac_builtin_shl_u32(%28, %c5_i32) {acir.id = "v103", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc52)
    %30 = call @ac_builtin_shr_u32(%0, %c7_i32) {acir.id = "v105", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc53)
    %31 = arith.andi %30, %c31_i32 {acir.id = "v107", acir.source_type = "u32"} : i32 loc(#loc54)
    %32 = arith.ori %29, %31 {acir.id = "v108", acir.source_type = "u32"} : i32 loc(#loc55)
    %33 = call @decode_stage_Work.0.call.0.2.0.call.2.108(%32) {acir.id = "v109", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc56)
    cf.br ^bb12(%33, %true, %c3_i32, %arg0, %c0_i32, %8, %10, %0, %false : i32, i1, i32, !acir.struct<"src_isa_f0424bc8.Fetched">, i32, i32, i32, i32, i1)
  ^bb12(%34: i32, %35: i1, %36: i32, %37: !acir.struct<"src_isa_f0424bc8.Fetched">, %38: i32, %39: i32, %40: i32, %41: i32, %42: i1):  // 5 preds: ^bb1, ^bb7, ^bb7, ^bb8, ^bb11
    %43 = acir.extract %37 {acir.id = "v154", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc57)
    %44 = acir.aggregate(%43, %41, %36, %38, %39, %40, %34, %35, %42) {acir.id = "v155", acir.source_type = "src_isa_f0424bc8.Decoded"} : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc58)
    return %44 : !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc59)
  }
  func.func @decode_stage_Work.0.call.4.6(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 attributes {acir.function = {contract = "Module:rf", definition = "ready", instance = "rf", interface = "Module:rf:ready", kind = "method", method = "ready", name = "decode_stage_Work.0.call.4.6", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "bool"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rf.pending" {acir.id = "v2", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc61)
    %2 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc62)
    %3 = acir.extract %1[%2] {acir.id = "v5", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc61)
    %4 = acir.unary %3 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc63)
    cf.cond_br %4, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %5 = acir.read %0 {acir.id = "v8", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc64)
    %6 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc65)
    %7 = acir.extract %5[%6] {acir.id = "v10", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc64)
    %8 = acir.unary %7 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc66)
    cf.br ^bb2(%8 : i1)
  ^bb2(%9: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %9, ^bb3, ^bb5(%false : i1)
  ^bb3:  // pred: ^bb2
    %10 = acir.extract %arg0 {acir.id = "v17", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc67)
    %11 = acir.unary %10 "not" {acir.id = "v18", acir.source_type = "bool"} : i1 -> i1 loc(#loc68)
    cf.cond_br %11, ^bb5(%true : i1), ^bb4
  ^bb4:  // pred: ^bb3
    %12 = acir.read %0 {acir.id = "v21", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc69)
    %13 = acir.extract %arg0 {acir.id = "v22", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc70)
    %14 = acir.extract %12[%13] {acir.id = "v23", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc69)
    %15 = acir.unary %14 "not" {acir.id = "v24", acir.source_type = "bool"} : i1 -> i1 loc(#loc71)
    cf.br ^bb5(%15 : i1)
  ^bb5(%16: i1):  // 3 preds: ^bb2, ^bb3, ^bb4
    return %16 : i1 loc(#loc72)
  }
  func.func @decode_stage.read_operands.0.call.5.7(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:rf", definition = "read", instance = "rf", interface = "Module:rf:read", kind = "method", method = "read", name = "decode_stage.read_operands.0.call.5.7", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v3", acir.source_type = "bool"} : i32 loc(#loc73)
    %1 = acir.get "rf.registers" {acir.id = "v9", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>> loc(#loc74)
    cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    return %c0_i32 : i32 loc(#loc75)
  ^bb2(%2: i32):  // pred: ^bb0
    %3 = acir.read %1 {acir.id = "v10", acir.source_type = "array<u32,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc76)
    %4 = acir.extract %3[%2] {acir.id = "v11", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc76)
    return %4 : i32 loc(#loc77)
  }
  func.func @decode_stage.read_operands.0.call.6.9(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:rf", definition = "read", instance = "rf", interface = "Module:rf:read", kind = "method", method = "read", name = "decode_stage.read_operands.0.call.6.9", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v3", acir.source_type = "bool"} : i32 loc(#loc73)
    %1 = acir.get "rf.registers" {acir.id = "v9", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>> loc(#loc74)
    cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    return %c0_i32 : i32 loc(#loc75)
  ^bb2(%2: i32):  // pred: ^bb0
    %3 = acir.read %1 {acir.id = "v10", acir.source_type = "array<u32,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc76)
    %4 = acir.extract %3[%2] {acir.id = "v11", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc76)
    return %4 : i32 loc(#loc77)
  }
  func.func @decode_stage.read_operands.0.call.7.16(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">) attributes {acir.function = {contract = "Module:rf", definition = "reserve", instance = "rf", interface = "Module:rf:reserve", kind = "method", method = "reserve", name = "decode_stage.read_operands.0.call.7.16", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc78)
    %1 = acir.get "rf.pending" {acir.id = "v12", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
    cf.cond_br %0, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %2 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc79)
    %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc79)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc80)
    %6 = acir.read %1 {acir.id = "v15", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc81)
    %7 = acir.update %6, %true[%5] {acir.id = "v16", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc81)
    acir.write %1, %7 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32> loc(#loc81)
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @memory_stage.access.0.call.8.11(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:memory", definition = "load", instance = "memory", interface = "Module:memory:load", kind = "method", method = "load", name = "memory_stage.access.0.call.8.11", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc82)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc83)
    %0 = arith.andi %arg0, %c3_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc84)
    %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc85)
    %2 = acir.get "memory.words" {acir.id = "v14", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>> loc(#loc86)
    cf.cond_br %1, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc87)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    acir.check %4 {acir.id = "v13", acir.source_type = "void"} loc(#loc88)
    %5 = acir.read %2 {acir.id = "v15", acir.source_type = "array<u32,16>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc89)
    %6 = call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v17", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc90)
    %7 = acir.extract %5[%6] {acir.id = "v18", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32 loc(#loc89)
    return %7 : i32 loc(#loc91)
  }
  func.func @memory_stage.access.0.call.9.18(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:memory", definition = "store", instance = "memory", interface = "Module:memory:store", kind = "method", method = "store", name = "memory_stage.access.0.call.9.18", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc92)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc93)
    %0 = arith.andi %arg0, %c3_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc94)
    %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc95)
    %2 = acir.get "memory.words" {acir.id = "v15", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>> loc(#loc86)
    cf.cond_br %1, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc96)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    acir.check %4 {acir.id = "v14", acir.source_type = "void"} loc(#loc97)
    %5 = call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v17", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc98)
    %6 = acir.read %2 {acir.id = "v18", acir.source_type = "array<u32,16>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc99)
    %7 = acir.update %6, %arg1[%5] {acir.id = "v19", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16> loc(#loc99)
    acir.write %2, %7 {acir.id = "v20", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 16>>, !acir.array<i32, 16> loc(#loc99)
    return
  }
  func.func @writeback.commit.0.call.10.5(%arg0: !acir.struct<"src_isa_f0424bc8.Retired">) attributes {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "writeback.commit.0.call.10.5", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Retired"}], result = "void"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc100)
    %1 = acir.get "rf.registers" {acir.id = "v12", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>> loc(#loc74)
    %2 = acir.get "rf.pending" {acir.id = "v18", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
    cf.cond_br %0, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc101)
    %4 = arith.cmpi ne, %3, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc101)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %arg0 {acir.id = "v13", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc102)
    %7 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc103)
    %8 = acir.read %1 {acir.id = "v15", acir.source_type = "array<u32,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc104)
    %9 = acir.update %8, %6[%7] {acir.id = "v16", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32> loc(#loc104)
    acir.write %1, %9 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 32>>, !acir.array<i32, 32> loc(#loc104)
    %10 = acir.extract %arg0 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc105)
    %11 = acir.read %2 {acir.id = "v21", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc106)
    %12 = acir.update %11, %false[%10] {acir.id = "v22", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc106)
    acir.write %2, %12 {acir.id = "v23", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32> loc(#loc106)
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @ac_summary_rf.ready.0.call.11.1(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 attributes {acir.function = {contract = "Module:rf", definition = "ready", instance = "rf", interface = "Module:rf:ready", kind = "method", method = "ready", name = "ac_summary_rf.ready.0.call.11.1", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "bool", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rf.pending" {acir.id = "v2", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc61)
    %2 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc62)
    %3 = acir.extract %1[%2] {acir.id = "v5", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc61)
    %4 = acir.unary %3 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc63)
    cf.cond_br %4, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %5 = acir.read %0 {acir.id = "v8", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc64)
    %6 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc65)
    %7 = acir.extract %5[%6] {acir.id = "v10", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc64)
    %8 = acir.unary %7 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc66)
    cf.br ^bb2(%8 : i1)
  ^bb2(%9: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %9, ^bb3, ^bb5(%false : i1)
  ^bb3:  // pred: ^bb2
    %10 = acir.extract %arg0 {acir.id = "v17", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc67)
    %11 = acir.unary %10 "not" {acir.id = "v18", acir.source_type = "bool"} : i1 -> i1 loc(#loc68)
    cf.cond_br %11, ^bb5(%true : i1), ^bb4
  ^bb4:  // pred: ^bb3
    %12 = acir.read %0 {acir.id = "v21", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc69)
    %13 = acir.extract %arg0 {acir.id = "v22", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc70)
    %14 = acir.extract %12[%13] {acir.id = "v23", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc69)
    %15 = acir.unary %14 "not" {acir.id = "v24", acir.source_type = "bool"} : i1 -> i1 loc(#loc71)
    cf.br ^bb5(%15 : i1)
  ^bb5(%16: i1):  // 3 preds: ^bb2, ^bb3, ^bb4
    return %16 : i1 loc(#loc72)
  }
  func.func @ac_summary_rf.read.0.call.12.1(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:rf", definition = "read", instance = "rf", interface = "Module:rf:read", kind = "method", method = "read", name = "ac_summary_rf.read.0.call.12.1", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "u32", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v3", acir.source_type = "bool"} : i32 loc(#loc73)
    %1 = acir.get "rf.registers" {acir.id = "v9", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>> loc(#loc74)
    cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    return %c0_i32 : i32 loc(#loc75)
  ^bb2(%2: i32):  // pred: ^bb0
    %3 = acir.read %1 {acir.id = "v10", acir.source_type = "array<u32,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc76)
    %4 = acir.extract %3[%2] {acir.id = "v11", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc76)
    return %4 : i32 loc(#loc77)
  }
  func.func @ac_summary_rf.reserve.0.call.13.1(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">) attributes {acir.function = {contract = "Module:rf", definition = "reserve", instance = "rf", interface = "Module:rf:reserve", kind = "method", method = "reserve", name = "ac_summary_rf.reserve.0.call.13.1", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc78)
    %1 = acir.get "rf.pending" {acir.id = "v12", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
    cf.cond_br %0, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %2 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc79)
    %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc79)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc80)
    %6 = acir.read %1 {acir.id = "v15", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc81)
    %7 = acir.update %6, %true[%5] {acir.id = "v16", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc81)
    acir.write %1, %7 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32> loc(#loc81)
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @ac_summary_rf.commit.0.call.14.1(%arg0: !acir.struct<"src_isa_f0424bc8.Retired">) attributes {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "ac_summary_rf.commit.0.call.14.1", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Retired"}], result = "void", summary = true}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc100)
    %1 = acir.get "rf.registers" {acir.id = "v12", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>> loc(#loc74)
    %2 = acir.get "rf.pending" {acir.id = "v18", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
    cf.cond_br %0, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc101)
    %4 = arith.cmpi ne, %3, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc101)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %arg0 {acir.id = "v13", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc102)
    %7 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc103)
    %8 = acir.read %1 {acir.id = "v15", acir.source_type = "array<u32,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc104)
    %9 = acir.update %8, %6[%7] {acir.id = "v16", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32> loc(#loc104)
    acir.write %1, %9 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 32>>, !acir.array<i32, 32> loc(#loc104)
    %10 = acir.extract %arg0 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc105)
    %11 = acir.read %2 {acir.id = "v21", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc106)
    %12 = acir.update %11, %false[%10] {acir.id = "v22", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc106)
    acir.write %2, %12 {acir.id = "v23", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32> loc(#loc106)
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @ac_summary_memory.load.0.call.15.1(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:memory", definition = "load", instance = "memory", interface = "Module:memory:load", kind = "method", method = "load", name = "ac_summary_memory.load.0.call.15.1", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}], result = "u32", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc82)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc83)
    %0 = arith.andi %arg0, %c3_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc84)
    %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc85)
    %2 = acir.get "memory.words" {acir.id = "v14", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>> loc(#loc86)
    cf.cond_br %1, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc87)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    acir.check %4 {acir.id = "v13", acir.source_type = "void"} loc(#loc88)
    %5 = acir.read %2 {acir.id = "v15", acir.source_type = "array<u32,16>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc89)
    %6 = call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v17", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc90)
    %7 = acir.extract %5[%6] {acir.id = "v18", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32 loc(#loc89)
    return %7 : i32 loc(#loc91)
  }
  func.func @ac_summary_memory.store.0.call.16.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:memory", definition = "store", instance = "memory", interface = "Module:memory:store", kind = "method", method = "store", name = "ac_summary_memory.store.0.call.16.2", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc92)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc93)
    %0 = arith.andi %arg0, %c3_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc94)
    %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc95)
    %2 = acir.get "memory.words" {acir.id = "v15", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>> loc(#loc86)
    cf.cond_br %1, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc96)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    acir.check %4 {acir.id = "v14", acir.source_type = "void"} loc(#loc97)
    %5 = call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v17", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc98)
    %6 = acir.read %2 {acir.id = "v18", acir.source_type = "array<u32,16>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc99)
    %7 = acir.update %6, %arg1[%5] {acir.id = "v19", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16> loc(#loc99)
    acir.write %2, %7 {acir.id = "v20", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 16>>, !acir.array<i32, 16> loc(#loc99)
    return
  }
  func.func @fetch.fetch(%arg0: i32) attributes {acir.function = {kind = "rule", name = "fetch.fetch", outputs = [{kind = "queue", name = "fetch.out", targets = ["fetch.out"], type = "src_isa_f0424bc8.Fetched"}], owner = "fetch", params = [{consume = false, id = "v1", name = "current_pc", targets = [], type = "u32"}], result = "void", rule = "fetch"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc107)
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc108)
    %0 = acir.get "fetch.rom" {acir.id = "v2", acir.source_type = "cell<array<u32,6>>"} : !acir.cell<!acir.array<i32, 6>> loc(#loc109)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "array<u32,6>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 6>> -> !acir.array<i32, 6> loc(#loc110)
    %2 = call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v5", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc111)
    %3 = acir.extract %1[%2] {acir.id = "v6", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 6>, i32) -> i32 loc(#loc110)
    %4 = acir.get "fetch.pc" {acir.id = "v7", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc112)
    %5 = arith.addi %arg0, %c4_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc113)
    acir.write %4, %5 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc114)
    %6 = acir.aggregate(%arg0, %3) {acir.id = "v11", acir.source_type = "src_isa_f0424bc8.Fetched"} : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc115)
    %7 = acir.get "fetch.out" {acir.id = "v12", acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> loc(#loc116)
    %8 = acir.read %7 {acir.id = "v13", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "fetch.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> i1 loc(#loc117)
    %9 = acir.unary %8 "not" {acir.id = "v14", acir.source_type = "bool"} : i1 -> i1 loc(#loc117)
    acir.guard %9 "Queue.push capacity" {acir.id = "v15", acir.source_type = "void", implicit = true} loc(#loc117)
    acir.push %7, %6 {acir.id = "v16", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc117)
    return loc(#loc117)
  }
  func.func @fetch_Work() attributes {acir.function = {kind = "work", name = "fetch_Work", owner = "fetch", params = [], result = "void"}} {
    %c24_i32 = arith.constant {acir.source_type = "u32"} 24 : i32 loc(#loc118)
    %0 = acir.get "fetch.pc" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc112)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc119)
    %2 = arith.cmpi ult, %1, %c24_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc120)
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    acir.invoke "fetch"(%1) {acir.id = "v6", acir.source_type = "void", callee = "fetch.fetch"} : (i32) -> () loc(#loc121)
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @decode_stage.read_operands(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, %arg1: !acir.struct<"src_isa_f0424bc8.Decoded">) attributes {acir.function = {kind = "rule", name = "decode_stage.read_operands", outputs = [{kind = "queue", name = "decode_stage.out", targets = ["decode_stage.out"], type = "src_isa_f0424bc8.Operands"}], owner = "decode_stage", params = [{consume = true, id = "v1", name = "message", targets = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>"}, {consume = false, id = "v2", name = "decoded", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "void", rule = "read_operands"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc122)
    %0 = acir.read %arg0 {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Fetched", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc122)
    acir.pop %arg0 {acir.id = "v4", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> loc(#loc122)
    acir.write %arg0, %false {acir.id = "v6", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "fetch.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, i1 loc(#loc122)
    %1 = acir.extract %arg1 {acir.id = "v7", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc123)
    %2 = call @decode_stage.read_operands.0.call.5.7(%1) {acir.id = "v8", acir.source_type = "u32", contract = "Module:rf", instance = "rf", interface = "Module:rf:read", method = "read", role = "method"} : (i32) -> i32 loc(#loc124)
    %3 = acir.extract %arg1 {acir.id = "v9", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc125)
    %4 = call @decode_stage.read_operands.0.call.6.9(%3) {acir.id = "v10", acir.source_type = "u32", contract = "Module:rf", instance = "rf", interface = "Module:rf:read", method = "read", role = "method"} : (i32) -> i32 loc(#loc126)
    %5 = acir.extract %arg1 {acir.id = "v11", acir.source_type = "bool", field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc127)
    %6 = acir.get "decode_stage.out" {acir.id = "v24", acir.source_type = "queue<src_isa_f0424bc8.Operands>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> loc(#loc128)
    cf.cond_br %5, ^bb1, ^bb2(%4 : i32)
  ^bb1:  // pred: ^bb0
    %7 = acir.extract %arg1 {acir.id = "v13", acir.source_type = "u32", field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc129)
    cf.br ^bb2(%7 : i32)
  ^bb2(%8: i32):  // 2 preds: ^bb0, ^bb1
    call @decode_stage.read_operands.0.call.7.16(%arg1) {acir.id = "v17", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:reserve", method = "reserve", role = "method"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> () loc(#loc130)
    %9 = acir.extract %arg1 {acir.id = "v18", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc131)
    %10 = acir.extract %arg1 {acir.id = "v19", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc132)
    %11 = acir.extract %arg1 {acir.id = "v20", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc133)
    %12 = acir.extract %arg1 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc134)
    %13 = acir.extract %arg1 {acir.id = "v22", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc135)
    %14 = acir.aggregate(%9, %10, %11, %12, %2, %8, %4, %13) {acir.id = "v23", acir.source_type = "src_isa_f0424bc8.Operands"} : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc136)
    %15 = acir.read %6 {acir.id = "v25", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "decode_stage.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> -> i1 loc(#loc137)
    %16 = acir.unary %15 "not" {acir.id = "v26", acir.source_type = "bool"} : i1 -> i1 loc(#loc137)
    acir.guard %16 "Queue.push capacity" {acir.id = "v27", acir.source_type = "void", implicit = true} loc(#loc137)
    acir.push %6, %14 {acir.id = "v28", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>, !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc137)
    return loc(#loc137)
  }
  func.func @decode_stage_Work() attributes {acir.function = {kind = "work", name = "decode_stage_Work", owner = "decode_stage", params = [], result = "void"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "fetch.out" {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> loc(#loc138)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Fetched", role = "observe", view = "old"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc139)
    %2 = call @decode_stage_Work.0.call.0.2(%1) {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Decoded", role = "helper"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc140)
    %3 = acir.get "retirement" {acir.id = "v4", acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> loc(#loc141)
    %4 = acir.query %3 "empty" {acir.id = "v5", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> i1 loc(#loc142)
    cf.cond_br %4, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %5 = call @decode_stage_Work.0.call.4.6(%2) {acir.id = "v7", acir.source_type = "bool", contract = "Module:rf", instance = "rf", interface = "Module:rf:ready", method = "ready", role = "method"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc143)
    cf.br ^bb2(%5 : i1)
  ^bb2(%6: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %6, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    acir.invoke "read_operands"(%0, %2) {acir.id = "v13", acir.source_type = "void", callee = "decode_stage.read_operands"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Decoded">) -> () loc(#loc144)
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @execute.execute(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) attributes {acir.function = {kind = "rule", name = "execute.execute", outputs = [{kind = "queue", name = "execute.out", targets = ["execute.out"], type = "src_isa_f0424bc8.Executed"}], owner = "execute", params = [{consume = true, id = "v1", name = "message", targets = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>"}], result = "void", rule = "execute"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc145)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc146)
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Operands", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc146)
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> loc(#loc146)
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "decode_stage.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>, i1 loc(#loc146)
    %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc147)
    %2 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc147)
    %3 = acir.get "execute.out" {acir.id = "v26", acir.source_type = "queue<src_isa_f0424bc8.Executed>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> loc(#loc148)
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %4 = acir.extract %0 {acir.id = "v10", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc149)
    %5 = acir.extract %0 {acir.id = "v11", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc150)
    %6 = arith.subi %4, %5 {acir.id = "v12", acir.source_type = "u32"} : i32 loc(#loc149)
    cf.br ^bb3(%6 : i32)
  ^bb2:  // pred: ^bb0
    %7 = acir.extract %0 {acir.id = "v13", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc151)
    %8 = acir.extract %0 {acir.id = "v14", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc152)
    %9 = arith.addi %7, %8 {acir.id = "v15", acir.source_type = "u32"} : i32 loc(#loc151)
    cf.br ^bb3(%9 : i32)
  ^bb3(%10: i32):  // 2 preds: ^bb1, ^bb2
    %11 = acir.extract %0 {acir.id = "v19", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc153)
    %12 = acir.extract %0 {acir.id = "v20", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc154)
    %13 = acir.extract %0 {acir.id = "v21", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc155)
    %14 = acir.extract %0 {acir.id = "v22", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc156)
    %15 = acir.extract %0 {acir.id = "v23", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc157)
    %16 = acir.extract %0 {acir.id = "v24", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc158)
    %17 = acir.aggregate(%11, %12, %13, %14, %10, %15, %16) {acir.id = "v25", acir.source_type = "src_isa_f0424bc8.Executed"} : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc159)
    %18 = acir.read %3 {acir.id = "v27", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "execute.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> -> i1 loc(#loc160)
    %19 = acir.unary %18 "not" {acir.id = "v28", acir.source_type = "bool"} : i1 -> i1 loc(#loc160)
    acir.guard %19 "Queue.push capacity" {acir.id = "v29", acir.source_type = "void", implicit = true} loc(#loc160)
    acir.push %3, %17 {acir.id = "v30", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>, !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc160)
    return loc(#loc160)
  }
  func.func @execute_Work() attributes {acir.function = {kind = "work", name = "execute_Work", owner = "execute", params = [], result = "void"}} {
    %0 = acir.get "decode_stage.out" {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Operands>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> loc(#loc161)
    acir.invoke "execute"(%0) {acir.id = "v2", acir.source_type = "void", callee = "execute.execute"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) -> () loc(#loc162)
    return
  }
  func.func @memory_stage.access(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) attributes {acir.function = {kind = "rule", name = "memory_stage.access", outputs = [{kind = "queue", name = "retirement", targets = ["retirement"], type = "src_isa_f0424bc8.Retired"}], owner = "memory_stage", params = [{consume = true, id = "v1", name = "message", targets = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>"}], result = "void", rule = "access"}} {
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc163)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc164)
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Executed", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc164)
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> loc(#loc164)
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "execute.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>, i1 loc(#loc164)
    %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc165)
    %2 = acir.extract %0 {acir.id = "v7", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc166)
    %3 = arith.cmpi eq, %2, %c2_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc166)
    %4 = acir.get "retirement" {acir.id = "v35", acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> loc(#loc141)
    cf.cond_br %3, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %5 = acir.extract %0 {acir.id = "v11", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc167)
    %6 = call @memory_stage.access.0.call.8.11(%5) {acir.id = "v12", acir.source_type = "u32", contract = "Module:memory", instance = "memory", interface = "Module:memory:load", method = "load", role = "method"} : (i32) -> i32 loc(#loc168)
    cf.br ^bb4(%0, %6 : !acir.struct<"src_isa_f0424bc8.Executed">, i32)
  ^bb2:  // pred: ^bb0
    %7 = acir.extract %0 {acir.id = "v13", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc169)
    %8 = arith.cmpi eq, %7, %c3_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc169)
    cf.cond_br %8, ^bb3, ^bb4(%0, %1 : !acir.struct<"src_isa_f0424bc8.Executed">, i32)
  ^bb3:  // pred: ^bb2
    %9 = acir.extract %0 {acir.id = "v17", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc170)
    %10 = acir.extract %0 {acir.id = "v18", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc171)
    call @memory_stage.access.0.call.9.18(%9, %10) {acir.id = "v19", acir.source_type = "void", contract = "Module:memory", instance = "memory", interface = "Module:memory:store", method = "store", role = "method"} : (i32, i32) -> () loc(#loc172)
    cf.br ^bb4(%0, %1 : !acir.struct<"src_isa_f0424bc8.Executed">, i32)
  ^bb4(%11: !acir.struct<"src_isa_f0424bc8.Executed">, %12: i32):  // 3 preds: ^bb1, ^bb2, ^bb3
    %13 = acir.extract %11 {acir.id = "v30", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc173)
    %14 = acir.extract %11 {acir.id = "v31", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc174)
    %15 = acir.extract %11 {acir.id = "v32", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc175)
    %16 = acir.extract %11 {acir.id = "v33", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc176)
    %17 = acir.aggregate(%13, %14, %15, %12, %16) {acir.id = "v34", acir.source_type = "src_isa_f0424bc8.Retired"} : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc177)
    %18 = acir.read %4 {acir.id = "v36", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "retirement"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> i1 loc(#loc178)
    %19 = acir.unary %18 "not" {acir.id = "v37", acir.source_type = "bool"} : i1 -> i1 loc(#loc178)
    acir.guard %19 "Queue.push capacity" {acir.id = "v38", acir.source_type = "void", implicit = true} loc(#loc178)
    acir.push %4, %17 {acir.id = "v39", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>, !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc178)
    return loc(#loc178)
  }
  func.func @memory_stage_Work() attributes {acir.function = {kind = "work", name = "memory_stage_Work", owner = "memory_stage", params = [], result = "void"}} {
    %0 = acir.get "execute.out" {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Executed>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> loc(#loc179)
    acir.invoke "access"(%0) {acir.id = "v2", acir.source_type = "void", callee = "memory_stage.access"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) -> () loc(#loc180)
    return
  }
  func.func @writeback.commit(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) attributes {acir.function = {kind = "rule", name = "writeback.commit", outputs = [], owner = "writeback", params = [{consume = true, id = "v1", name = "message", targets = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>"}], result = "void", rule = "commit"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc181)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc182)
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Retired", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc182)
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> loc(#loc182)
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "retirement"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>, i1 loc(#loc182)
    call @writeback.commit.0.call.10.5(%0) {acir.id = "v6", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:commit", method = "commit", role = "method"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> () loc(#loc183)
    %1 = acir.get "writeback.retired" {acir.id = "v7", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc184)
    %2 = acir.read %1 {acir.id = "v8", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc185)
    %3 = arith.addi %2, %c1_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc185)
    acir.write %1, %3 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc186)
    %4 = acir.get "writeback.last_pc" {acir.id = "v12", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc187)
    %5 = acir.extract %0 {acir.id = "v13", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc188)
    acir.write %4, %5 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc189)
    return
  }
  func.func @writeback_Work() attributes {acir.function = {kind = "work", name = "writeback_Work", owner = "writeback", params = [], result = "void"}} {
    %0 = acir.get "retirement" {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> loc(#loc141)
    acir.invoke "commit"(%0) {acir.id = "v2", acir.source_type = "void", callee = "writeback.commit"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) -> () loc(#loc190)
    return
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":12:55)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":16:51)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:51)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":73:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":73:21)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":73:33)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":74:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":74:21)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":74:33)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":72:48)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":76:34)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":149:39)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":150:39)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:13)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:12)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:5)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":72:18)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":66:20)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":63:23)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":62:21)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":61:12)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":62:14)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":63:15)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":63:14)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:14)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:11)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:10)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":66:12)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":66:11)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:12)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:11)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":72:8)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:16)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:9)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":75:28)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":75:21)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:10)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":78:16)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":81:10)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":78:33)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":78:48)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":78:9)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":79:12)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":82:16)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":82:9)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":85:28)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":85:21)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":88:16)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":88:35)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":88:9)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:30)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:29)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:52)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:51)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:28)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:21)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":94:20)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":94:12)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":94:5)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":16:15)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":25:21)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":25:35)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":25:17)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":26:25)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":26:39)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":26:21)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":27:26)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":27:22)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":27:45)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":27:59)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":27:41)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":25:9)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":31:12)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":12:17)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":32:13)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:16)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:9)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":37:12)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":37:28)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":38:27)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":38:13)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:33)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:27)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:17)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:16)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:13)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:39)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:9)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":58:16)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":58:28)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":58:9)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:33)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:27)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:17)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:16)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:39)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:9)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:21)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:9)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":42:12)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":42:28)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":43:40)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":43:29)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":43:13)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":44:27)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":44:13)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:33)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:40)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":72:11)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:16)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:26)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":76:10)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:20)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:9)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:16)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":77:11)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:9)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:21)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":85:18)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:8)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":87:15)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":97:20)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":98:23)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":98:15)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":99:31)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":99:23)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":100:36)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":93:11)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":100:15)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:9)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:25)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:37)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:51)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:65)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":103:48)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:16)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:9)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:12)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":107:19)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":107:12)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":167:18)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":109:15)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":109:38)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":111:15)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:54)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:16)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:41)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":117:11)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:18)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:29)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:63)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:74)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:25)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:34)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:45)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:56)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:33)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:51)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:16)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:9)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":112:12)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":126:11)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:25)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":135:16)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":136:18)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:12)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:34)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:22)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":139:14)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":140:26)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":140:38)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":140:13)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:24)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:33)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:44)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:61)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:16)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:9)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":127:12)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":143:11)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":156:41)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":154:16)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":155:9)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":149:15)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":156:25)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":156:9)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":150:15)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":157:25)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":157:9)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":159:5)

