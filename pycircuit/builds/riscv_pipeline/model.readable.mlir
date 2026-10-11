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
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v33", acir.source_type = "array<u32,32>"} : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 32>
    return %0 : !acir.array<i32, 32>
  }
  func.func @ac_init_rf.pending() -> !acir.array<i1, 32> attributes {acir.function = {kind = "helper", name = "ac_init_rf.pending", params = [], result = "array<bool,32>"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.aggregate(%false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false) {acir.id = "v67", acir.source_type = "array<bool,32>"} : (i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1) -> !acir.array<i1, 32>
    return %0 : !acir.array<i1, 32>
  }
  func.func @ac_init_memory.words() -> !acir.array<i32, 16> attributes {acir.function = {kind = "helper", name = "ac_init_memory.words", params = [], result = "array<u32,16>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v85", acir.source_type = "array<u32,16>"} : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 16>
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
    %c7340179_i32 = arith.constant {acir.source_type = "u32"} 7340179 : i32
    %c5243155_i32 = arith.constant {acir.source_type = "u32"} 5243155 : i32
    %c2130355_i32 = arith.constant {acir.source_type = "u32"} 2130355 : i32
    %c3153955_i32 = arith.constant {acir.source_type = "u32"} 3153955 : i32
    %c8707_i32 = arith.constant {acir.source_type = "u32"} 8707 : i32
    %c1075970739_i32 = arith.constant {acir.source_type = "u32"} 1075970739 : i32
    %0 = acir.aggregate(%c7340179_i32, %c5243155_i32, %c2130355_i32, %c3153955_i32, %c8707_i32, %c1075970739_i32) {acir.id = "v95", acir.source_type = "array<u32,6>"} : (i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 6>
    return %0 : !acir.array<i32, 6>
  }
  func.func @ac_init_fetch.pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_fetch.pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
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
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_writeback.last_pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_writeback.last_pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @decode_stage_Work.0.call.0.2.0.call.0.35(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "decode_stage_Work.0.call.0.2.0.call.0.35", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
    return %1 : i32
  }
  func.func @decode_stage_Work.0.call.0.2.0.call.1.85(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "decode_stage_Work.0.call.0.2.0.call.1.85", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
    return %1 : i32
  }
  func.func @decode_stage_Work.0.call.0.2.0.call.2.108(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "decode_stage_Work.0.call.0.2.0.call.2.108", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
    return %1 : i32
  }
  func.func @decode_stage_Work.0.call.0.2(%arg0: !acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded"> attributes {acir.function = {definition = "src_isa_f0424bc8.decode", kind = "helper", name = "decode_stage_Work.0.call.0.2", params = [{consume = false, id = "v1", name = "packet", targets = [], type = "src_isa_f0424bc8.Fetched"}], result = "src_isa_f0424bc8.Decoded"}} {
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %c35_i32 = arith.constant {acir.source_type = "u32"} 35 : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c32_i32 = arith.constant {acir.source_type = "u32"} 32 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c51_i32 = arith.constant {acir.source_type = "u32"} 51 : i32
    %c19_i32 = arith.constant {acir.source_type = "u32"} 19 : i32
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c20_i32 = arith.constant {acir.source_type = "u32"} 20 : i32
    %c15_i32 = arith.constant {acir.source_type = "u32"} 15 : i32
    %c31_i32 = arith.constant {acir.source_type = "u32"} 31 : i32
    %c25_i32 = arith.constant {acir.source_type = "u32"} 25 : i32
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    %c12_i32 = arith.constant {acir.source_type = "u32"} 12 : i32
    %c127_i32 = arith.constant {acir.source_type = "u32"} 127 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32
    %1 = arith.andi %0, %c127_i32 {acir.id = "v4", acir.source_type = "u32"} : i32
    %2 = call @ac_builtin_shr_u32(%0, %c12_i32) {acir.id = "v6", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %3 = arith.andi %2, %c7_i32 {acir.id = "v8", acir.source_type = "u32"} : i32
    %4 = call @ac_builtin_shr_u32(%0, %c25_i32) {acir.id = "v10", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %5 = call @ac_builtin_shr_u32(%0, %c7_i32) {acir.id = "v12", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %6 = arith.andi %5, %c31_i32 {acir.id = "v14", acir.source_type = "u32"} : i32
    %7 = call @ac_builtin_shr_u32(%0, %c15_i32) {acir.id = "v16", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %8 = arith.andi %7, %c31_i32 {acir.id = "v18", acir.source_type = "u32"} : i32
    %9 = call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v20", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %10 = arith.andi %9, %c31_i32 {acir.id = "v22", acir.source_type = "u32"} : i32
    %11 = arith.cmpi eq, %1, %c19_i32 {acir.id = "v28", acir.source_type = "bool"} : i32
    cf.cond_br %11, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %12 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v31", acir.source_type = "bool"} : i32
    acir.check %12 {acir.id = "v32", acir.source_type = "void"}
    %13 = call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v35", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %14 = call @decode_stage_Work.0.call.0.2.0.call.0.35(%13) {acir.id = "v36", acir.source_type = "u32", role = "helper"} : (i32) -> i32
    cf.br ^bb12(%14, %true, %c0_i32, %arg0, %6, %8, %c0_i32, %0, %true : i32, i1, i32, !acir.struct<"src_isa_f0424bc8.Fetched">, i32, i32, i32, i32, i1)
  ^bb2:  // pred: ^bb0
    %15 = arith.cmpi eq, %1, %c51_i32 {acir.id = "v39", acir.source_type = "bool"} : i32
    cf.cond_br %15, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %16 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v42", acir.source_type = "bool"} : i32
    cf.cond_br %16, ^bb5, ^bb7(%false : i1)
  ^bb4:  // pred: ^bb2
    %17 = arith.cmpi eq, %1, %c3_i32 {acir.id = "v77", acir.source_type = "bool"} : i32
    cf.cond_br %17, ^bb8, ^bb9
  ^bb5:  // pred: ^bb3
    %18 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v45", acir.source_type = "bool"} : i32
    cf.cond_br %18, ^bb7(%true : i1), ^bb6
  ^bb6:  // pred: ^bb5
    %19 = arith.cmpi eq, %4, %c32_i32 {acir.id = "v49", acir.source_type = "bool"} : i32
    cf.br ^bb7(%19 : i1)
  ^bb7(%20: i1):  // 3 preds: ^bb3, ^bb5, ^bb6
    acir.check %20 {acir.id = "v57", acir.source_type = "void"}
    %21 = arith.cmpi eq, %4, %c32_i32 {acir.id = "v59", acir.source_type = "bool"} : i32
    cf.cond_br %21, ^bb12(%c0_i32, %false, %c1_i32, %arg0, %6, %8, %10, %0, %true : i32, i1, i32, !acir.struct<"src_isa_f0424bc8.Fetched">, i32, i32, i32, i32, i1), ^bb12(%c0_i32, %false, %c0_i32, %arg0, %6, %8, %10, %0, %true : i32, i1, i32, !acir.struct<"src_isa_f0424bc8.Fetched">, i32, i32, i32, i32, i1)
  ^bb8:  // pred: ^bb4
    %22 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v80", acir.source_type = "bool"} : i32
    acir.check %22 {acir.id = "v81", acir.source_type = "void"}
    %23 = call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v85", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %24 = call @decode_stage_Work.0.call.0.2.0.call.1.85(%23) {acir.id = "v86", acir.source_type = "u32", role = "helper"} : (i32) -> i32
    cf.br ^bb12(%24, %true, %c2_i32, %arg0, %6, %8, %c0_i32, %0, %true : i32, i1, i32, !acir.struct<"src_isa_f0424bc8.Fetched">, i32, i32, i32, i32, i1)
  ^bb9:  // pred: ^bb4
    %25 = arith.cmpi eq, %1, %c35_i32 {acir.id = "v89", acir.source_type = "bool"} : i32
    cf.cond_br %25, ^bb10, ^bb11(%false : i1)
  ^bb10:  // pred: ^bb9
    %26 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v92", acir.source_type = "bool"} : i32
    cf.br ^bb11(%26 : i1)
  ^bb11(%27: i1):  // 2 preds: ^bb9, ^bb10
    acir.check %27 {acir.id = "v97", acir.source_type = "void"}
    %28 = call @ac_builtin_shr_u32(%0, %c25_i32) {acir.id = "v101", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %29 = call @ac_builtin_shl_u32(%28, %c5_i32) {acir.id = "v103", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32
    %30 = call @ac_builtin_shr_u32(%0, %c7_i32) {acir.id = "v105", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %31 = arith.andi %30, %c31_i32 {acir.id = "v107", acir.source_type = "u32"} : i32
    %32 = arith.ori %29, %31 {acir.id = "v108", acir.source_type = "u32"} : i32
    %33 = call @decode_stage_Work.0.call.0.2.0.call.2.108(%32) {acir.id = "v109", acir.source_type = "u32", role = "helper"} : (i32) -> i32
    cf.br ^bb12(%33, %true, %c3_i32, %arg0, %c0_i32, %8, %10, %0, %false : i32, i1, i32, !acir.struct<"src_isa_f0424bc8.Fetched">, i32, i32, i32, i32, i1)
  ^bb12(%34: i32, %35: i1, %36: i32, %37: !acir.struct<"src_isa_f0424bc8.Fetched">, %38: i32, %39: i32, %40: i32, %41: i32, %42: i1):  // 5 preds: ^bb1, ^bb7, ^bb7, ^bb8, ^bb11
    %43 = acir.extract %37 {acir.id = "v154", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32
    %44 = acir.aggregate(%43, %41, %36, %38, %39, %40, %34, %35, %42) {acir.id = "v155", acir.source_type = "src_isa_f0424bc8.Decoded"} : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    return %44 : !acir.struct<"src_isa_f0424bc8.Decoded">
  }
  func.func @decode_stage_Work.0.call.4.6(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 attributes {acir.function = {contract = "Module:rf", definition = "ready", instance = "rf", interface = "Module:rf:ready", kind = "method", method = "ready", name = "decode_stage_Work.0.call.4.6", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "bool"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rf.pending" {acir.id = "v2", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32>
    %2 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %3 = acir.extract %1[%2] {acir.id = "v5", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %4, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %5 = acir.read %0 {acir.id = "v8", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32>
    %6 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %7 = acir.extract %5[%6] {acir.id = "v10", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1
    %8 = acir.unary %7 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%8 : i1)
  ^bb2(%9: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %9, ^bb3, ^bb5(%false : i1)
  ^bb3:  // pred: ^bb2
    %10 = acir.extract %arg0 {acir.id = "v17", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1
    %11 = acir.unary %10 "not" {acir.id = "v18", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %11, ^bb5(%true : i1), ^bb4
  ^bb4:  // pred: ^bb3
    %12 = acir.read %0 {acir.id = "v21", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32>
    %13 = acir.extract %arg0 {acir.id = "v22", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %14 = acir.extract %12[%13] {acir.id = "v23", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1
    %15 = acir.unary %14 "not" {acir.id = "v24", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb5(%15 : i1)
  ^bb5(%16: i1):  // 3 preds: ^bb2, ^bb3, ^bb4
    return %16 : i1
  }
  func.func @decode_stage.read_operands.0.call.5.7(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:rf", definition = "read", instance = "rf", interface = "Module:rf:read", kind = "method", method = "read", name = "decode_stage.read_operands.0.call.5.7", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v3", acir.source_type = "bool"} : i32
    %1 = acir.get "rf.registers" {acir.id = "v9", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>>
    cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    return %c0_i32 : i32
  ^bb2(%2: i32):  // pred: ^bb0
    %3 = acir.read %1 {acir.id = "v10", acir.source_type = "array<u32,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32>
    %4 = acir.extract %3[%2] {acir.id = "v11", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32
    return %4 : i32
  }
  func.func @decode_stage.read_operands.0.call.6.9(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:rf", definition = "read", instance = "rf", interface = "Module:rf:read", kind = "method", method = "read", name = "decode_stage.read_operands.0.call.6.9", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v3", acir.source_type = "bool"} : i32
    %1 = acir.get "rf.registers" {acir.id = "v9", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>>
    cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    return %c0_i32 : i32
  ^bb2(%2: i32):  // pred: ^bb0
    %3 = acir.read %1 {acir.id = "v10", acir.source_type = "array<u32,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32>
    %4 = acir.extract %3[%2] {acir.id = "v11", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32
    return %4 : i32
  }
  func.func @decode_stage.read_operands.0.call.7.16(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">) attributes {acir.function = {contract = "Module:rf", definition = "reserve", instance = "rf", interface = "Module:rf:reserve", kind = "method", method = "reserve", name = "decode_stage.read_operands.0.call.7.16", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1
    %1 = acir.get "rf.pending" {acir.id = "v12", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>>
    cf.cond_br %0, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %2 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %6 = acir.read %1 {acir.id = "v15", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32>
    %7 = acir.update %6, %true[%5] {acir.id = "v16", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32>
    acir.write %1, %7 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32>
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @memory_stage.access.0.call.8.11(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:memory", definition = "load", instance = "memory", interface = "Module:memory:load", kind = "method", method = "load", name = "memory_stage.access.0.call.8.11", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %0 = arith.andi %arg0, %c3_i32 {acir.id = "v3", acir.source_type = "u32"} : i32
    %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32
    %2 = acir.get "memory.words" {acir.id = "v14", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>>
    cf.cond_br %1, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    acir.check %4 {acir.id = "v13", acir.source_type = "void"}
    %5 = acir.read %2 {acir.id = "v15", acir.source_type = "array<u32,16>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16>
    %6 = call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v17", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %7 = acir.extract %5[%6] {acir.id = "v18", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32
    return %7 : i32
  }
  func.func @memory_stage.access.0.call.9.18(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:memory", definition = "store", instance = "memory", interface = "Module:memory:store", kind = "method", method = "store", name = "memory_stage.access.0.call.9.18", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %0 = arith.andi %arg0, %c3_i32 {acir.id = "v4", acir.source_type = "u32"} : i32
    %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    %2 = acir.get "memory.words" {acir.id = "v15", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>>
    cf.cond_br %1, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v9", acir.source_type = "bool"} : i32
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    acir.check %4 {acir.id = "v14", acir.source_type = "void"}
    %5 = call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v17", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %6 = acir.read %2 {acir.id = "v18", acir.source_type = "array<u32,16>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16>
    %7 = acir.update %6, %arg1[%5] {acir.id = "v19", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16>
    acir.write %2, %7 {acir.id = "v20", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 16>>, !acir.array<i32, 16>
    return
  }
  func.func @writeback.commit.0.call.10.5(%arg0: !acir.struct<"src_isa_f0424bc8.Retired">) attributes {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "writeback.commit.0.call.10.5", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Retired"}], result = "void"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1
    %1 = acir.get "rf.registers" {acir.id = "v12", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>>
    %2 = acir.get "rf.pending" {acir.id = "v18", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>>
    cf.cond_br %0, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32
    %4 = arith.cmpi ne, %3, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %arg0 {acir.id = "v13", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32
    %7 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32
    %8 = acir.read %1 {acir.id = "v15", acir.source_type = "array<u32,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32>
    %9 = acir.update %8, %6[%7] {acir.id = "v16", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32>
    acir.write %1, %9 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 32>>, !acir.array<i32, 32>
    %10 = acir.extract %arg0 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32
    %11 = acir.read %2 {acir.id = "v21", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32>
    %12 = acir.update %11, %false[%10] {acir.id = "v22", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32>
    acir.write %2, %12 {acir.id = "v23", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32>
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @ac_summary_rf.ready.0.call.11.1(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 attributes {acir.function = {contract = "Module:rf", definition = "ready", instance = "rf", interface = "Module:rf:ready", kind = "method", method = "ready", name = "ac_summary_rf.ready.0.call.11.1", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "bool", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rf.pending" {acir.id = "v2", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32>
    %2 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %3 = acir.extract %1[%2] {acir.id = "v5", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %4, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %5 = acir.read %0 {acir.id = "v8", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32>
    %6 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %7 = acir.extract %5[%6] {acir.id = "v10", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1
    %8 = acir.unary %7 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%8 : i1)
  ^bb2(%9: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %9, ^bb3, ^bb5(%false : i1)
  ^bb3:  // pred: ^bb2
    %10 = acir.extract %arg0 {acir.id = "v17", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1
    %11 = acir.unary %10 "not" {acir.id = "v18", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %11, ^bb5(%true : i1), ^bb4
  ^bb4:  // pred: ^bb3
    %12 = acir.read %0 {acir.id = "v21", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32>
    %13 = acir.extract %arg0 {acir.id = "v22", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %14 = acir.extract %12[%13] {acir.id = "v23", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1
    %15 = acir.unary %14 "not" {acir.id = "v24", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb5(%15 : i1)
  ^bb5(%16: i1):  // 3 preds: ^bb2, ^bb3, ^bb4
    return %16 : i1
  }
  func.func @ac_summary_rf.read.0.call.12.1(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:rf", definition = "read", instance = "rf", interface = "Module:rf:read", kind = "method", method = "read", name = "ac_summary_rf.read.0.call.12.1", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "u32", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v3", acir.source_type = "bool"} : i32
    %1 = acir.get "rf.registers" {acir.id = "v9", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>>
    cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    return %c0_i32 : i32
  ^bb2(%2: i32):  // pred: ^bb0
    %3 = acir.read %1 {acir.id = "v10", acir.source_type = "array<u32,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32>
    %4 = acir.extract %3[%2] {acir.id = "v11", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32
    return %4 : i32
  }
  func.func @ac_summary_rf.reserve.0.call.13.1(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">) attributes {acir.function = {contract = "Module:rf", definition = "reserve", instance = "rf", interface = "Module:rf:reserve", kind = "method", method = "reserve", name = "ac_summary_rf.reserve.0.call.13.1", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1
    %1 = acir.get "rf.pending" {acir.id = "v12", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>>
    cf.cond_br %0, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %2 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %6 = acir.read %1 {acir.id = "v15", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32>
    %7 = acir.update %6, %true[%5] {acir.id = "v16", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32>
    acir.write %1, %7 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32>
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @ac_summary_rf.commit.0.call.14.1(%arg0: !acir.struct<"src_isa_f0424bc8.Retired">) attributes {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "ac_summary_rf.commit.0.call.14.1", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Retired"}], result = "void", summary = true}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1
    %1 = acir.get "rf.registers" {acir.id = "v12", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>>
    %2 = acir.get "rf.pending" {acir.id = "v18", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>>
    cf.cond_br %0, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32
    %4 = arith.cmpi ne, %3, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %arg0 {acir.id = "v13", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32
    %7 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32
    %8 = acir.read %1 {acir.id = "v15", acir.source_type = "array<u32,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32>
    %9 = acir.update %8, %6[%7] {acir.id = "v16", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32>
    acir.write %1, %9 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 32>>, !acir.array<i32, 32>
    %10 = acir.extract %arg0 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32
    %11 = acir.read %2 {acir.id = "v21", acir.source_type = "array<bool,32>", role = "observe", view = "old"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32>
    %12 = acir.update %11, %false[%10] {acir.id = "v22", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32>
    acir.write %2, %12 {acir.id = "v23", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32>
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @ac_summary_memory.load.0.call.15.1(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:memory", definition = "load", instance = "memory", interface = "Module:memory:load", kind = "method", method = "load", name = "ac_summary_memory.load.0.call.15.1", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}], result = "u32", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %0 = arith.andi %arg0, %c3_i32 {acir.id = "v3", acir.source_type = "u32"} : i32
    %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32
    %2 = acir.get "memory.words" {acir.id = "v14", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>>
    cf.cond_br %1, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    acir.check %4 {acir.id = "v13", acir.source_type = "void"}
    %5 = acir.read %2 {acir.id = "v15", acir.source_type = "array<u32,16>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16>
    %6 = call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v17", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %7 = acir.extract %5[%6] {acir.id = "v18", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32
    return %7 : i32
  }
  func.func @ac_summary_memory.store.0.call.16.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:memory", definition = "store", instance = "memory", interface = "Module:memory:store", kind = "method", method = "store", name = "ac_summary_memory.store.0.call.16.2", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %0 = arith.andi %arg0, %c3_i32 {acir.id = "v4", acir.source_type = "u32"} : i32
    %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    %2 = acir.get "memory.words" {acir.id = "v15", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>>
    cf.cond_br %1, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v9", acir.source_type = "bool"} : i32
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    acir.check %4 {acir.id = "v14", acir.source_type = "void"}
    %5 = call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v17", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %6 = acir.read %2 {acir.id = "v18", acir.source_type = "array<u32,16>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16>
    %7 = acir.update %6, %arg1[%5] {acir.id = "v19", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16>
    acir.write %2, %7 {acir.id = "v20", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 16>>, !acir.array<i32, 16>
    return
  }
  func.func @fetch.fetch(%arg0: i32) attributes {acir.function = {kind = "rule", name = "fetch.fetch", outputs = [{kind = "queue", name = "fetch.out", targets = ["fetch.out"], type = "src_isa_f0424bc8.Fetched"}], owner = "fetch", params = [{consume = false, id = "v1", name = "current_pc", targets = [], type = "u32"}], result = "void", rule = "fetch"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %0 = acir.get "fetch.rom" {acir.id = "v2", acir.source_type = "cell<array<u32,6>>"} : !acir.cell<!acir.array<i32, 6>>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "array<u32,6>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 6>> -> !acir.array<i32, 6>
    %2 = call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v5", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32
    %3 = acir.extract %1[%2] {acir.id = "v6", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 6>, i32) -> i32
    %4 = acir.get "fetch.pc" {acir.id = "v7", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %5 = arith.addi %arg0, %c4_i32 {acir.id = "v9", acir.source_type = "u32"} : i32
    acir.write %4, %5 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32
    %6 = acir.aggregate(%arg0, %3) {acir.id = "v11", acir.source_type = "src_isa_f0424bc8.Fetched"} : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched">
    %7 = acir.get "fetch.out" {acir.id = "v12", acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
    %8 = acir.read %7 {acir.id = "v13", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "fetch.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> i1
    %9 = acir.unary %8 "not" {acir.id = "v14", acir.source_type = "bool"} : i1 -> i1
    acir.guard %9 "Queue.push capacity" {acir.id = "v15", acir.source_type = "void", implicit = true}
    acir.push %7, %6 {acir.id = "v16", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Fetched">
    return
  }
  func.func @fetch_Work() attributes {acir.function = {kind = "work", name = "fetch_Work", owner = "fetch", params = [], result = "void"}} {
    %c24_i32 = arith.constant {acir.source_type = "u32"} 24 : i32
    %0 = acir.get "fetch.pc" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %2 = arith.cmpi ult, %1, %c24_i32 {acir.id = "v4", acir.source_type = "bool"} : i32
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    acir.invoke "fetch"(%1) {acir.id = "v6", acir.source_type = "void", callee = "fetch.fetch"} : (i32) -> ()
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @decode_stage.read_operands(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, %arg1: !acir.struct<"src_isa_f0424bc8.Decoded">) attributes {acir.function = {kind = "rule", name = "decode_stage.read_operands", outputs = [{kind = "queue", name = "decode_stage.out", targets = ["decode_stage.out"], type = "src_isa_f0424bc8.Operands"}], owner = "decode_stage", params = [{consume = true, id = "v1", name = "message", targets = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>"}, {consume = false, id = "v2", name = "decoded", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "void", rule = "read_operands"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.read %arg0 {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Fetched", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched">
    acir.pop %arg0 {acir.id = "v4", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
    acir.write %arg0, %false {acir.id = "v6", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "fetch.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, i1
    %1 = acir.extract %arg1 {acir.id = "v7", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %2 = call @decode_stage.read_operands.0.call.5.7(%1) {acir.id = "v8", acir.source_type = "u32", contract = "Module:rf", instance = "rf", interface = "Module:rf:read", method = "read", role = "method"} : (i32) -> i32
    %3 = acir.extract %arg1 {acir.id = "v9", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %4 = call @decode_stage.read_operands.0.call.6.9(%3) {acir.id = "v10", acir.source_type = "u32", contract = "Module:rf", instance = "rf", interface = "Module:rf:read", method = "read", role = "method"} : (i32) -> i32
    %5 = acir.extract %arg1 {acir.id = "v11", acir.source_type = "bool", field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1
    %6 = acir.get "decode_stage.out" {acir.id = "v24", acir.source_type = "queue<src_isa_f0424bc8.Operands>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
    cf.cond_br %5, ^bb1, ^bb2(%4 : i32)
  ^bb1:  // pred: ^bb0
    %7 = acir.extract %arg1 {acir.id = "v13", acir.source_type = "u32", field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    cf.br ^bb2(%7 : i32)
  ^bb2(%8: i32):  // 2 preds: ^bb0, ^bb1
    call @decode_stage.read_operands.0.call.7.16(%arg1) {acir.id = "v17", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:reserve", method = "reserve", role = "method"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> ()
    %9 = acir.extract %arg1 {acir.id = "v18", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %10 = acir.extract %arg1 {acir.id = "v19", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %11 = acir.extract %arg1 {acir.id = "v20", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %12 = acir.extract %arg1 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %13 = acir.extract %arg1 {acir.id = "v22", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1
    %14 = acir.aggregate(%9, %10, %11, %12, %2, %8, %4, %13) {acir.id = "v23", acir.source_type = "src_isa_f0424bc8.Operands"} : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands">
    %15 = acir.read %6 {acir.id = "v25", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "decode_stage.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> -> i1
    %16 = acir.unary %15 "not" {acir.id = "v26", acir.source_type = "bool"} : i1 -> i1
    acir.guard %16 "Queue.push capacity" {acir.id = "v27", acir.source_type = "void", implicit = true}
    acir.push %6, %14 {acir.id = "v28", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>, !acir.struct<"src_isa_f0424bc8.Operands">
    return
  }
  func.func @decode_stage_Work() attributes {acir.function = {kind = "work", name = "decode_stage_Work", owner = "decode_stage", params = [], result = "void"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "fetch.out" {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Fetched", role = "observe", view = "old"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched">
    %2 = call @decode_stage_Work.0.call.0.2(%1) {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Decoded", role = "helper"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    %3 = acir.get "retirement" {acir.id = "v4", acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
    %4 = acir.query %3 "empty" {acir.id = "v5", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> i1
    cf.cond_br %4, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %5 = call @decode_stage_Work.0.call.4.6(%2) {acir.id = "v7", acir.source_type = "bool", contract = "Module:rf", instance = "rf", interface = "Module:rf:ready", method = "ready", role = "method"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1
    cf.br ^bb2(%5 : i1)
  ^bb2(%6: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %6, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    acir.invoke "read_operands"(%0, %2) {acir.id = "v13", acir.source_type = "void", callee = "decode_stage.read_operands"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Decoded">) -> ()
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @execute.execute(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) attributes {acir.function = {kind = "rule", name = "execute.execute", outputs = [{kind = "queue", name = "execute.out", targets = ["execute.out"], type = "src_isa_f0424bc8.Executed"}], owner = "execute", params = [{consume = true, id = "v1", name = "message", targets = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>"}], result = "void", rule = "execute"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Operands", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> -> !acir.struct<"src_isa_f0424bc8.Operands">
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "decode_stage.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>, i1
    %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32
    %2 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
    %3 = acir.get "execute.out" {acir.id = "v26", acir.source_type = "queue<src_isa_f0424bc8.Executed>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %4 = acir.extract %0 {acir.id = "v10", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32
    %5 = acir.extract %0 {acir.id = "v11", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32
    %6 = arith.subi %4, %5 {acir.id = "v12", acir.source_type = "u32"} : i32
    cf.br ^bb3(%6 : i32)
  ^bb2:  // pred: ^bb0
    %7 = acir.extract %0 {acir.id = "v13", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32
    %8 = acir.extract %0 {acir.id = "v14", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32
    %9 = arith.addi %7, %8 {acir.id = "v15", acir.source_type = "u32"} : i32
    cf.br ^bb3(%9 : i32)
  ^bb3(%10: i32):  // 2 preds: ^bb1, ^bb2
    %11 = acir.extract %0 {acir.id = "v19", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32
    %12 = acir.extract %0 {acir.id = "v20", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32
    %13 = acir.extract %0 {acir.id = "v21", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32
    %14 = acir.extract %0 {acir.id = "v22", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32
    %15 = acir.extract %0 {acir.id = "v23", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32
    %16 = acir.extract %0 {acir.id = "v24", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1
    %17 = acir.aggregate(%11, %12, %13, %14, %10, %15, %16) {acir.id = "v25", acir.source_type = "src_isa_f0424bc8.Executed"} : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed">
    %18 = acir.read %3 {acir.id = "v27", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "execute.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> -> i1
    %19 = acir.unary %18 "not" {acir.id = "v28", acir.source_type = "bool"} : i1 -> i1
    acir.guard %19 "Queue.push capacity" {acir.id = "v29", acir.source_type = "void", implicit = true}
    acir.push %3, %17 {acir.id = "v30", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>, !acir.struct<"src_isa_f0424bc8.Executed">
    return
  }
  func.func @execute_Work() attributes {acir.function = {kind = "work", name = "execute_Work", owner = "execute", params = [], result = "void"}} {
    %0 = acir.get "decode_stage.out" {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Operands>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
    acir.invoke "execute"(%0) {acir.id = "v2", acir.source_type = "void", callee = "execute.execute"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) -> ()
    return
  }
  func.func @memory_stage.access(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) attributes {acir.function = {kind = "rule", name = "memory_stage.access", outputs = [{kind = "queue", name = "retirement", targets = ["retirement"], type = "src_isa_f0424bc8.Retired"}], owner = "memory_stage", params = [{consume = true, id = "v1", name = "message", targets = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>"}], result = "void", rule = "access"}} {
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Executed", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> -> !acir.struct<"src_isa_f0424bc8.Executed">
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "execute.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>, i1
    %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32
    %2 = acir.extract %0 {acir.id = "v7", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32
    %3 = arith.cmpi eq, %2, %c2_i32 {acir.id = "v9", acir.source_type = "bool"} : i32
    %4 = acir.get "retirement" {acir.id = "v35", acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
    cf.cond_br %3, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %5 = acir.extract %0 {acir.id = "v11", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32
    %6 = call @memory_stage.access.0.call.8.11(%5) {acir.id = "v12", acir.source_type = "u32", contract = "Module:memory", instance = "memory", interface = "Module:memory:load", method = "load", role = "method"} : (i32) -> i32
    cf.br ^bb4(%0, %6 : !acir.struct<"src_isa_f0424bc8.Executed">, i32)
  ^bb2:  // pred: ^bb0
    %7 = acir.extract %0 {acir.id = "v13", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32
    %8 = arith.cmpi eq, %7, %c3_i32 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.cond_br %8, ^bb3, ^bb4(%0, %1 : !acir.struct<"src_isa_f0424bc8.Executed">, i32)
  ^bb3:  // pred: ^bb2
    %9 = acir.extract %0 {acir.id = "v17", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32
    %10 = acir.extract %0 {acir.id = "v18", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32
    call @memory_stage.access.0.call.9.18(%9, %10) {acir.id = "v19", acir.source_type = "void", contract = "Module:memory", instance = "memory", interface = "Module:memory:store", method = "store", role = "method"} : (i32, i32) -> ()
    cf.br ^bb4(%0, %1 : !acir.struct<"src_isa_f0424bc8.Executed">, i32)
  ^bb4(%11: !acir.struct<"src_isa_f0424bc8.Executed">, %12: i32):  // 3 preds: ^bb1, ^bb2, ^bb3
    %13 = acir.extract %11 {acir.id = "v30", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32
    %14 = acir.extract %11 {acir.id = "v31", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32
    %15 = acir.extract %11 {acir.id = "v32", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32
    %16 = acir.extract %11 {acir.id = "v33", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1
    %17 = acir.aggregate(%13, %14, %15, %12, %16) {acir.id = "v34", acir.source_type = "src_isa_f0424bc8.Retired"} : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired">
    %18 = acir.read %4 {acir.id = "v36", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "retirement"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> i1
    %19 = acir.unary %18 "not" {acir.id = "v37", acir.source_type = "bool"} : i1 -> i1
    acir.guard %19 "Queue.push capacity" {acir.id = "v38", acir.source_type = "void", implicit = true}
    acir.push %4, %17 {acir.id = "v39", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>, !acir.struct<"src_isa_f0424bc8.Retired">
    return
  }
  func.func @memory_stage_Work() attributes {acir.function = {kind = "work", name = "memory_stage_Work", owner = "memory_stage", params = [], result = "void"}} {
    %0 = acir.get "execute.out" {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Executed>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
    acir.invoke "access"(%0) {acir.id = "v2", acir.source_type = "void", callee = "memory_stage.access"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) -> ()
    return
  }
  func.func @writeback.commit(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) attributes {acir.function = {kind = "rule", name = "writeback.commit", outputs = [], owner = "writeback", params = [{consume = true, id = "v1", name = "message", targets = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>"}], result = "void", rule = "commit"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Retired", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> !acir.struct<"src_isa_f0424bc8.Retired">
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "retirement"}], view = "ehr"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>, i1
    call @writeback.commit.0.call.10.5(%0) {acir.id = "v6", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:commit", method = "commit", role = "method"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> ()
    %1 = acir.get "writeback.retired" {acir.id = "v7", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %2 = acir.read %1 {acir.id = "v8", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %3 = arith.addi %2, %c1_i32 {acir.id = "v10", acir.source_type = "u32"} : i32
    acir.write %1, %3 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<i32>, i32
    %4 = acir.get "writeback.last_pc" {acir.id = "v12", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %5 = acir.extract %0 {acir.id = "v13", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32
    acir.write %4, %5 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @writeback_Work() attributes {acir.function = {kind = "work", name = "writeback_Work", owner = "writeback", params = [], result = "void"}} {
    %0 = acir.get "retirement" {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
    acir.invoke "commit"(%0) {acir.id = "v2", acir.source_type = "void", callee = "writeback.commit"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) -> ()
    return
  }
}

