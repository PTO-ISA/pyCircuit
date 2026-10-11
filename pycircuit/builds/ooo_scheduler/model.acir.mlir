module attributes {acir.ehr = [{aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "adder.out", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "multiplier.out", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "scheduler.add_requests", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "scheduler.mul_requests", same_port = "invisible", semantics = "last_effective_prior_port"}, {aspect = "full", bindings = [{method = "Queue:pop", port = 0 : i64}, {method = "Queue:query.full", port = 1 : i64}], commit = "queue.effects", fallback = "old", port_count = 2 : i64, resource = "source", same_port = "invisible", semantics = "last_effective_prior_port"}], acir.model = {config = [], constants = {src_payloads_25ebcaa7.ADD = 0 : i64, src_payloads_25ebcaa7.MUL = 1 : i64}, contracts = [{guards = {push = ["query.full"]}, id = "Queue", methods = ["read", "revise", "pop", "push", "query.empty", "query.full", "query.size"], relations = [{after = "revise", before = "read", origin = "Pipeline FIFO", relation = "SB", source = "default"}, {after = "pop", before = "revise", origin = "Pipeline FIFO", relation = "SB", source = "default"}], views = {full = {stages = ["pop", "query.full"], visibility = "ehr"}}}, {id = "EHR", methods = ["read", "write"], relations = []}, {id = "Signal", methods = ["set", "get"], relations = [{after = "get", before = "set", origin = "Signal current value", relation = "SB", source = "default"}]}, {id = "Cell", methods = ["read", "write"], relations = []}, {id = "Module:rob.slot0", methods = ["allocate", "complete", "peek", "retire"], relations = []}, {id = "Module:rob.slot1", methods = ["allocate", "complete", "peek", "retire"], relations = []}, {id = "Module:rob.slot2", methods = ["allocate", "complete", "peek", "retire"], relations = []}, {id = "Module:rob.slot3", methods = ["allocate", "complete", "peek", "retire"], relations = []}, {id = "Module:rob", methods = ["allocate", "complete", "has_room", "lookup", "retire"], relations = []}, {id = "Module:rf", methods = ["commit", "operand", "rename"], relations = []}, {id = "Module:left", methods = ["place", "take", "wakeup"], relations = []}, {id = "Module:right", methods = ["place", "take", "wakeup"], relations = []}, {id = "Module:scheduler", methods = [], relations = []}, {id = "Module:adder", methods = [], relations = []}, {id = "Module:multiplier", methods = [], relations = []}, {id = "Module:cdb", methods = [], relations = []}, {id = "Module:dispatch", methods = [], relations = []}, {id = "Module:commit", methods = [], relations = []}, {id = "Module:audit", methods = [], relations = []}, {id = "Module:top", methods = [], relations = []}], exports = {source = "source"}, format_version = 6 : i64, instances = [{contract = "Module:rob.slot0", definition = "src_state_7e811fd9.RobSlot", inputs = {}, interfaces = {allocate = "Module:rob.slot0:allocate", complete = "Module:rob.slot0:complete", peek = "Module:rob.slot0:peek", retire = "Module:rob.slot0:retire"}, members = {state = {resources = ["rob.slot0.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}}, methods = ["allocate", "complete", "peek", "retire"], name = "rob.slot0", outputs = {resources = ["rob.slot0.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}, parent = "rob"}, {contract = "Module:rob.slot1", definition = "src_state_7e811fd9.RobSlot", inputs = {}, interfaces = {allocate = "Module:rob.slot1:allocate", complete = "Module:rob.slot1:complete", peek = "Module:rob.slot1:peek", retire = "Module:rob.slot1:retire"}, members = {state = {resources = ["rob.slot1.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}}, methods = ["allocate", "complete", "peek", "retire"], name = "rob.slot1", outputs = {resources = ["rob.slot1.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}, parent = "rob"}, {contract = "Module:rob.slot2", definition = "src_state_7e811fd9.RobSlot", inputs = {}, interfaces = {allocate = "Module:rob.slot2:allocate", complete = "Module:rob.slot2:complete", peek = "Module:rob.slot2:peek", retire = "Module:rob.slot2:retire"}, members = {state = {resources = ["rob.slot2.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}}, methods = ["allocate", "complete", "peek", "retire"], name = "rob.slot2", outputs = {resources = ["rob.slot2.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}, parent = "rob"}, {contract = "Module:rob.slot3", definition = "src_state_7e811fd9.RobSlot", inputs = {}, interfaces = {allocate = "Module:rob.slot3:allocate", complete = "Module:rob.slot3:complete", peek = "Module:rob.slot3:peek", retire = "Module:rob.slot3:retire"}, members = {state = {resources = ["rob.slot3.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}}, methods = ["allocate", "complete", "peek", "retire"], name = "rob.slot3", outputs = {resources = ["rob.slot3.state"], type = "cell<src_payloads_25ebcaa7.RobEntry>", value}, parent = "rob"}, {contract = "Module:rob", definition = "src_state_7e811fd9.ReorderBuffer", inputs = {}, interfaces = {allocate = "Module:rob:allocate", complete = "Module:rob:complete", has_room = "Module:rob:has_room", lookup = "Module:rob:lookup", retire = "Module:rob:retire"}, members = {head = {resources = ["rob.head"], type = "cell<u32>", value}, slot0 = {instance = "rob.slot0"}, slot1 = {instance = "rob.slot1"}, slot2 = {instance = "rob.slot2"}, slot3 = {instance = "rob.slot3"}, tail = {resources = ["rob.tail"], type = "cell<u32>", value}}, methods = ["allocate", "complete", "has_room", "lookup", "retire"], name = "rob", outputs = [{resources = ["rob.head"], type = "cell<u32>", value}, {resources = ["rob.tail"], type = "cell<u32>", value}], parent = "top"}, {contract = "Module:rf", definition = "src_state_7e811fd9.RegisterFile", inputs = {rob = {instance = "rob"}}, interfaces = {commit = "Module:rf:commit", operand = "Module:rf:operand", rename = "Module:rf:rename"}, members = {committed = {resources = ["rf.committed"], type = "cell<array<u32,8>>", value}, registers = {resources = ["rf.registers"], type = "cell<array<u32,8>>", value}, renamed = {resources = ["rf.renamed"], type = "cell<array<u32,8>>", value}, rob = {instance = "rob"}}, methods = ["commit", "operand", "rename"], name = "rf", outputs = [{resources = ["rf.registers"], type = "cell<array<u32,8>>", value}, {resources = ["rf.renamed"], type = "cell<array<u32,8>>", value}, {resources = ["rf.committed"], type = "cell<array<u32,8>>", value}], parent = "top"}, {contract = "Module:left", definition = "src_state_7e811fd9.ReservationStation", inputs = {}, interfaces = {place = "Module:left:place", take = "Module:left:take", wakeup = "Module:left:wakeup"}, members = {slot = {resources = ["left.slot"], type = "cell<src_payloads_25ebcaa7.Entry>", value}}, methods = ["place", "take", "wakeup"], name = "left", outputs = {resources = ["left.slot"], type = "cell<src_payloads_25ebcaa7.Entry>", value}, parent = "top"}, {contract = "Module:right", definition = "src_state_7e811fd9.ReservationStation", inputs = {}, interfaces = {place = "Module:right:place", take = "Module:right:take", wakeup = "Module:right:wakeup"}, members = {slot = {resources = ["right.slot"], type = "cell<src_payloads_25ebcaa7.Entry>", value}}, methods = ["place", "take", "wakeup"], name = "right", outputs = {resources = ["right.slot"], type = "cell<src_payloads_25ebcaa7.Entry>", value}, parent = "top"}, {contract = "Module:scheduler", definition = "src_execution_885f3671.Scheduler", inputs = {left = {instance = "left"}, right = {instance = "right"}}, interfaces = {}, members = {add_requests = {resources = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}, left = {instance = "left"}, mul_requests = {resources = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}, right = {instance = "right"}}, methods = [], name = "scheduler", outputs = [{resources = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}, {resources = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}], parent = "top"}, {contract = "Module:adder", definition = "src_execution_885f3671.Adder", inputs = {source = {resources = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}}, interfaces = {}, members = {out = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, source = {resources = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}}, methods = [], name = "adder", outputs = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, parent = "top"}, {contract = "Module:multiplier", definition = "src_execution_885f3671.Multiplier", inputs = {source = {resources = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}}, interfaces = {}, members = {active = {resources = ["multiplier.active"], type = "cell<bool>", value}, out = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, remaining = {resources = ["multiplier.remaining"], type = "cell<u32>", value}, request = {resources = ["multiplier.request"], type = "cell<src_payloads_25ebcaa7.Entry>", value}, source = {resources = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>", value}}, methods = [], name = "multiplier", outputs = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, parent = "top"}, {contract = "Module:cdb", definition = "src_model_3cf90884.Broadcast", inputs = {add_done = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, left = {instance = "left"}, mul_done = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, right = {instance = "right"}, rob = {instance = "rob"}}, interfaces = {}, members = {add_done = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, event = {resources = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>", value}, left = {instance = "left"}, mul_done = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, right = {instance = "right"}, rob = {instance = "rob"}}, methods = [], name = "cdb", outputs = {resources = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>", value}, parent = "top"}, {contract = "Module:dispatch", definition = "src_model_3cf90884.Dispatch", inputs = {add_done = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, left = {instance = "left"}, mul_done = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, rf = {instance = "rf"}, right = {instance = "right"}, rob = {instance = "rob"}, source = {resources = ["source"], type = "queue<src_payloads_25ebcaa7.MicroOp>", value}}, interfaces = {}, members = {add_done = {resources = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, left = {instance = "left"}, mul_done = {resources = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>", value}, rf = {instance = "rf"}, right = {instance = "right"}, rob = {instance = "rob"}, source = {resources = ["source"], type = "queue<src_payloads_25ebcaa7.MicroOp>", value}}, methods = [], name = "dispatch", outputs, parent = "top"}, {contract = "Module:commit", definition = "src_model_3cf90884.Commit", inputs = {rf = {instance = "rf"}, rob = {instance = "rob"}}, interfaces = {}, members = {last_tag = {resources = ["commit.last_tag"], type = "cell<u32>", value}, retired = {resources = ["commit.retired"], type = "cell<u32>", value}, rf = {instance = "rf"}, rob = {instance = "rob"}}, methods = [], name = "commit", outputs = [{resources = ["commit.retired"], type = "cell<u32>", value}, {resources = ["commit.last_tag"], type = "cell<u32>", value}], parent = "top"}, {contract = "Module:audit", definition = "src_model_3cf90884.CompletionAudit", inputs = {event = {resources = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>", value}}, interfaces = {}, members = {event = {resources = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>", value}, last_tag = {resources = ["audit.last_tag"], type = "cell<u32>", value}}, methods = [], name = "audit", outputs = {resources = ["audit.last_tag"], type = "cell<u32>", value}, parent = "top"}, {contract = "Module:top", definition = "src_model_3cf90884.OutOfOrder", inputs = {}, interfaces = {}, members = {adder = {instance = "adder"}, audit = {instance = "audit"}, cdb = {instance = "cdb"}, commit = {instance = "commit"}, dispatch = {instance = "dispatch"}, left = {instance = "left"}, multiplier = {instance = "multiplier"}, rf = {instance = "rf"}, right = {instance = "right"}, rob = {instance = "rob"}, scheduler = {instance = "scheduler"}, source = {resources = ["source"], type = "queue<src_payloads_25ebcaa7.MicroOp>", value}}, methods = [], name = "top", outputs = [[{resources = ["rf.registers"], type = "cell<array<u32,8>>", value}, {resources = ["rf.renamed"], type = "cell<array<u32,8>>", value}, {resources = ["rf.committed"], type = "cell<array<u32,8>>", value}], [{resources = ["rob.head"], type = "cell<u32>", value}, {resources = ["rob.tail"], type = "cell<u32>", value}], [{resources = ["commit.retired"], type = "cell<u32>", value}, {resources = ["commit.last_tag"], type = "cell<u32>", value}], {resources = ["audit.last_tag"], type = "cell<u32>", value}], parent}], modules = [{name = "scheduler", rules = [{function = "scheduler.issue_add", name = "issue_add"}, {function = "scheduler.issue_mul", name = "issue_mul"}], work = "scheduler_Work"}, {name = "adder", rules = [{function = "adder.execute", name = "execute"}], work = "adder_Work"}, {name = "multiplier", rules = [{function = "multiplier.advance", name = "advance"}, {function = "multiplier.accept", name = "accept"}], work = "multiplier_Work"}, {name = "cdb", rules = [{function = "cdb.broadcast", name = "broadcast"}], work = "cdb_Work"}, {name = "dispatch", rules = [{function = "dispatch.dispatch", name = "dispatch"}], work = "dispatch_Work"}, {name = "commit", rules = [{function = "commit.commit", name = "commit"}], work = "commit_Work"}, {name = "audit", rules = [{function = "audit.observe", name = "observe"}], work = "audit_Work"}], top = "OutOfOrder", type_order = ["src_payloads_25ebcaa7.MicroOp", "src_payloads_25ebcaa7.Operand", "src_payloads_25ebcaa7.Entry", "src_payloads_25ebcaa7.Completion", "src_payloads_25ebcaa7.RobEntry"], types = {src_payloads_25ebcaa7.Completion = [{default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.Entry = [{default = false, name = "valid", type = "bool"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default, name = "lhs", type = "src_payloads_25ebcaa7.Operand"}, {default, name = "rhs", type = "src_payloads_25ebcaa7.Operand"}], src_payloads_25ebcaa7.MicroOp = [{default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}], src_payloads_25ebcaa7.Operand = [{default = false, name = "ready", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}], src_payloads_25ebcaa7.RobEntry = [{default = false, name = "valid", type = "bool"}, {default = false, name = "done", type = "bool"}, {default = 0 : i64, name = "tag", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}]}}} {
  acir.resource "source" "queue" "src_payloads_25ebcaa7.MicroOp" {capacity_fn = "ac_init_source_cap", contract = "Queue", initial_fn = "ac_init_source", loc = {column = 13 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py", line = 90 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "rob.slot0.state" "cell" "src_payloads_25ebcaa7.RobEntry" {contract = "Cell", initial_fn = "ac_init_rob.slot0.state", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 47 : i64}, visibility = "old"}
  acir.resource "rob.slot1.state" "cell" "src_payloads_25ebcaa7.RobEntry" {contract = "Cell", initial_fn = "ac_init_rob.slot1.state", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 47 : i64}, visibility = "old"}
  acir.resource "rob.slot2.state" "cell" "src_payloads_25ebcaa7.RobEntry" {contract = "Cell", initial_fn = "ac_init_rob.slot2.state", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 47 : i64}, visibility = "old"}
  acir.resource "rob.slot3.state" "cell" "src_payloads_25ebcaa7.RobEntry" {contract = "Cell", initial_fn = "ac_init_rob.slot3.state", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 47 : i64}, visibility = "old"}
  acir.resource "rob.head" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_rob.head", loc = {column = 11 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 88 : i64}, visibility = "old"}
  acir.resource "rob.tail" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_rob.tail", loc = {column = 11 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 89 : i64}, visibility = "old"}
  acir.resource "rf.registers" "cell" "array<u32,8>" {contract = "Cell", initial_fn = "ac_init_rf.registers", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 156 : i64}, visibility = "old"}
  acir.resource "rf.renamed" "cell" "array<u32,8>" {contract = "Cell", initial_fn = "ac_init_rf.renamed", loc = {column = 14 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 157 : i64}, visibility = "old"}
  acir.resource "rf.committed" "cell" "array<u32,8>" {contract = "Cell", initial_fn = "ac_init_rf.committed", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 158 : i64}, visibility = "old"}
  acir.resource "left.slot" "cell" "src_payloads_25ebcaa7.Entry" {contract = "Cell", initial_fn = "ac_init_left.slot", loc = {column = 11 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 8 : i64}, visibility = "old"}
  acir.resource "right.slot" "cell" "src_payloads_25ebcaa7.Entry" {contract = "Cell", initial_fn = "ac_init_right.slot", loc = {column = 11 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py", line = 8 : i64}, visibility = "old"}
  acir.resource "scheduler.add_requests" "queue" "src_payloads_25ebcaa7.Entry" {capacity_fn = "ac_init_scheduler.add_requests_cap", contract = "Queue", initial_fn = "ac_init_scheduler.add_requests", loc = {column = 19 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py", line = 8 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "scheduler.mul_requests" "queue" "src_payloads_25ebcaa7.Entry" {capacity_fn = "ac_init_scheduler.mul_requests_cap", contract = "Queue", initial_fn = "ac_init_scheduler.mul_requests", loc = {column = 19 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py", line = 9 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "adder.out" "queue" "src_payloads_25ebcaa7.Completion" {capacity_fn = "ac_init_adder.out_cap", contract = "Queue", initial_fn = "ac_init_adder.out", loc = {column = 10 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py", line = 42 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "multiplier.active" "cell" "bool" {contract = "Cell", initial_fn = "ac_init_multiplier.active", loc = {column = 13 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py", line = 55 : i64}, visibility = "old"}
  acir.resource "multiplier.remaining" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_multiplier.remaining", loc = {column = 16 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py", line = 56 : i64}, visibility = "old"}
  acir.resource "multiplier.request" "cell" "src_payloads_25ebcaa7.Entry" {contract = "Cell", initial_fn = "ac_init_multiplier.request", loc = {column = 14 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py", line = 57 : i64}, visibility = "old"}
  acir.resource "multiplier.out" "queue" "src_payloads_25ebcaa7.Completion" {capacity_fn = "ac_init_multiplier.out_cap", contract = "Queue", initial_fn = "ac_init_multiplier.out", loc = {column = 10 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py", line = 58 : i64}, views = [{commit = "queue.effects", initial = "old_count == capacity", name = "full", visibility = "ehr"}], visibility = "old"}
  acir.resource "cdb.event" "signal" "src_payloads_25ebcaa7.Completion" {contract = "Signal", initial_fn = "ac_init_cdb.event", loc = {column = 12 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py", line = 39 : i64}, visibility = "current"}
  acir.resource "commit.retired" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_commit.retired", loc = {column = 14 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py", line = 61 : i64}, visibility = "old"}
  acir.resource "commit.last_tag" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_commit.last_tag", loc = {column = 15 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py", line = 62 : i64}, visibility = "old"}
  acir.resource "audit.last_tag" "cell" "u32" {contract = "Cell", initial_fn = "ac_init_audit.last_tag", loc = {column = 15 : i64, file = "/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py", line = 77 : i64}, visibility = "old"}
  func.func @ac_init_source_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_source_cap", params = [], result = "u32"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc1)
    return %c4_i32 : i32
  }
  func.func @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> attributes {acir.function = {kind = "helper", name = "ac_init_source", params = [], result = "vector<src_payloads_25ebcaa7.MicroOp>"}} {
    %c6_i32 = arith.constant {acir.source_type = "u32"} 6 : i32 loc(#loc2)
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %0 = acir.aggregate(%c1_i32, %c1_i32, %c2_i32, %c3_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc3)
    %1 = acir.aggregate(%c0_i32, %c4_i32, %c1_i32, %c2_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc4)
    %2 = acir.aggregate(%c0_i32, %c5_i32, %c2_i32, %c3_i32) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc5)
    %3 = acir.aggregate(%c0_i32, %c6_i32, %c4_i32, %c5_i32) {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc6)
    %4 = acir.aggregate(%0, %1, %2, %3) {acir.id = "v22", acir.source_type = "vector<src_payloads_25ebcaa7.MicroOp>"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> loc(#loc7)
    return %4 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
  }
  func.func @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot0.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot1.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot2.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v43", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot3.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v50", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc8)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.head() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_rob.head", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc9)
    return %c1_i32 : i32
  }
  func.func @ac_init_rob.tail() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_rob.tail", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc10)
    return %c1_i32 : i32
  }
  func.func @ac_init_rf.registers() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_rf.registers", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c6_i32 = arith.constant {acir.source_type = "u32"} 6 : i32 loc(#loc11)
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc12)
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c6_i32, %c7_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v64", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8> loc(#loc13)
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_rf.renamed() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_rf.renamed", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v74", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8> loc(#loc14)
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_rf.committed() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_rf.committed", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v84", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8> loc(#loc15)
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_left.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc16)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v90", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %1 = acir.aggregate() {acir.id = "v91", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v92", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc16)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_right.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc16)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v98", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %1 = acir.aggregate() {acir.id = "v99", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc16)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v100", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc16)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_scheduler.add_requests_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_scheduler.add_requests_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_scheduler.add_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">> attributes {acir.function = {kind = "helper", name = "ac_init_scheduler.add_requests", params = [], result = "vector<src_payloads_25ebcaa7.Entry>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Entry>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
  }
  func.func @ac_init_scheduler.mul_requests_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_scheduler.mul_requests_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_scheduler.mul_requests() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">> attributes {acir.function = {kind = "helper", name = "ac_init_scheduler.mul_requests", params = [], result = "vector<src_payloads_25ebcaa7.Entry>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Entry>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Entry">>
  }
  func.func @ac_init_adder.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_adder.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_adder.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">> attributes {acir.function = {kind = "helper", name = "ac_init_adder.out", params = [], result = "vector<src_payloads_25ebcaa7.Completion>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Completion>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
  }
  func.func @ac_init_multiplier.active() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.active", params = [], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc17)
    return %false : i1
  }
  func.func @ac_init_multiplier.remaining() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.remaining", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc18)
    return %c0_i32 : i32
  }
  func.func @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.request", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc19)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v119", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc19)
    %1 = acir.aggregate() {acir.id = "v120", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc19)
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v121", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc19)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_multiplier.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_multiplier.out() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">> attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.out", params = [], result = "vector<src_payloads_25ebcaa7.Completion>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_payloads_25ebcaa7.Completion>"} : () -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    return %0 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.Completion">>
  }
  func.func @ac_init_cdb.event() -> !acir.struct<"src_payloads_25ebcaa7.Completion"> attributes {acir.function = {kind = "helper", name = "ac_init_cdb.event", params = [], result = "src_payloads_25ebcaa7.Completion"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32) {acir.id = "v129", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc20)
    return %0 : !acir.struct<"src_payloads_25ebcaa7.Completion">
  }
  func.func @ac_init_commit.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_commit.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc21)
    return %c0_i32 : i32
  }
  func.func @ac_init_commit.last_tag() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_commit.last_tag", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc22)
    return %c0_i32 : i32
  }
  func.func @ac_init_audit.last_tag() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_audit.last_tag", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    return %c0_i32 : i32
  }
  func.func @scheduler.issue_add.0.call.0.3.0.call.0.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "left", kind = "guard", name = "scheduler.issue_add.0.call.0.3.0.call.0.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc25)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc26)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc27)
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc27)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc28)
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc28)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc29)
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1 loc(#loc30)
  }
  func.func @scheduler.issue_add.0.call.0.3(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:left", definition = "take", instance = "left", interface = "Module:left:take", kind = "method", method = "take", name = "scheduler.issue_add.0.call.0.3", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc31)
    %0 = call @scheduler.issue_add.0.call.0.3.0.call.0.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (i32) -> i1 loc(#loc32)
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc33)
    %1 = acir.get "left.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc34)
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc31)
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc35)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
  }
  func.func @scheduler.issue_add.0.call.2.13.0.call.2.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "right", kind = "guard", name = "scheduler.issue_add.0.call.2.13.0.call.2.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "right.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc25)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc26)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc27)
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc27)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc28)
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc28)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc29)
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1 loc(#loc30)
  }
  func.func @scheduler.issue_add.0.call.2.13(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:right", definition = "take", instance = "right", interface = "Module:right:take", kind = "method", method = "take", name = "scheduler.issue_add.0.call.2.13", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc31)
    %0 = call @scheduler.issue_add.0.call.2.13.0.call.2.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (i32) -> i1 loc(#loc37)
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc33)
    %1 = acir.get "right.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc34)
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc31)
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc35)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
  }
  func.func @scheduler.issue_mul.0.call.4.3.0.call.4.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "left", kind = "guard", name = "scheduler.issue_mul.0.call.4.3.0.call.4.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc25)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc26)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc27)
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc27)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc28)
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc28)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc29)
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1 loc(#loc30)
  }
  func.func @scheduler.issue_mul.0.call.4.3(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:left", definition = "take", instance = "left", interface = "Module:left:take", kind = "method", method = "take", name = "scheduler.issue_mul.0.call.4.3", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc31)
    %0 = call @scheduler.issue_mul.0.call.4.3.0.call.4.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (i32) -> i1 loc(#loc38)
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc33)
    %1 = acir.get "left.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc34)
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc31)
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc35)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
  }
  func.func @scheduler.issue_mul.0.call.6.13.0.call.6.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "right", kind = "guard", name = "scheduler.issue_mul.0.call.6.13.0.call.6.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "right.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc25)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc26)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc27)
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc27)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc28)
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc28)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc29)
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1 loc(#loc30)
  }
  func.func @scheduler.issue_mul.0.call.6.13(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:right", definition = "take", instance = "right", interface = "Module:right:take", kind = "method", method = "take", name = "scheduler.issue_mul.0.call.6.13", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc31)
    %0 = call @scheduler.issue_mul.0.call.6.13.0.call.6.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (i32) -> i1 loc(#loc39)
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc33)
    %1 = acir.get "right.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc34)
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc31)
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc35)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
  }
  func.func @cdb.broadcast.0.call.8.20.0.call.8.9(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "complete", instance = "rob.slot0", interface = "Module:rob.slot0:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.20.0.call.8.9", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @cdb.broadcast.0.call.8.20.0.call.9.13(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "complete", instance = "rob.slot1", interface = "Module:rob.slot1:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.20.0.call.9.13", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @cdb.broadcast.0.call.8.20.0.call.10.17(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "complete", instance = "rob.slot2", interface = "Module:rob.slot2:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.20.0.call.10.17", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @cdb.broadcast.0.call.8.20.0.call.11.18(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "complete", instance = "rob.slot3", interface = "Module:rob.slot3:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.20.0.call.11.18", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @cdb.broadcast.0.call.8.20(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob", definition = "complete", instance = "rob", interface = "Module:rob:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.20", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc49)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc50)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc51)
    %1 = arith.andi %0, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc52)
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc53)
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    call @cdb.broadcast.0.call.8.20.0.call.8.9(%arg0, %arg1) {acir.id = "v10", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:complete", method = "complete", role = "method"} : (i32, i32) -> () loc(#loc54)
    cf.br ^bb7
  ^bb2:  // pred: ^bb0
    %3 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc55)
    cf.cond_br %3, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    call @cdb.broadcast.0.call.8.20.0.call.9.13(%arg0, %arg1) {acir.id = "v14", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:complete", method = "complete", role = "method"} : (i32, i32) -> () loc(#loc56)
    cf.br ^bb7
  ^bb4:  // pred: ^bb2
    %4 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc57)
    cf.cond_br %4, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    call @cdb.broadcast.0.call.8.20.0.call.10.17(%arg0, %arg1) {acir.id = "v18", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:complete", method = "complete", role = "method"} : (i32, i32) -> () loc(#loc58)
    cf.br ^bb7
  ^bb6:  // pred: ^bb4
    call @cdb.broadcast.0.call.8.20.0.call.11.18(%arg0, %arg1) {acir.id = "v19", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:complete", method = "complete", role = "method"} : (i32, i32) -> () loc(#loc59)
    cf.br ^bb7
  ^bb7:  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    return
  }
  func.func @cdb.broadcast.0.call.13.23(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:left", definition = "wakeup", instance = "left", interface = "Module:left:wakeup", kind = "method", method = "wakeup", name = "cdb.broadcast.0.call.13.23", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc60)
    %2 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc61)
    cf.cond_br %2, ^bb1, ^bb9(%false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc62)
    %4 = acir.extract %3 {acir.id = "v9", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc62)
    %5 = acir.unary %4 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc63)
    cf.cond_br %5, ^bb2, ^bb3(%false : i1)
  ^bb2:  // pred: ^bb1
    %6 = acir.extract %1 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc64)
    %7 = acir.extract %6 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc64)
    %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc64)
    cf.br ^bb3(%8 : i1)
  ^bb3(%9: i1):  // 2 preds: ^bb1, ^bb2
    cf.cond_br %9, ^bb4, ^bb5(%false, %1, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb4:  // pred: ^bb3
    %10 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc65)
    %11 = acir.update %1, %10 {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc66)
    cf.br ^bb5(%true, %11, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb5(%12: i1, %13: !acir.struct<"src_payloads_25ebcaa7.Entry">, %14: i32, %15: i32):  // 2 preds: ^bb3, ^bb4
    %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc67)
    %17 = acir.extract %16 {acir.id = "v32", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc67)
    %18 = acir.unary %17 "not" {acir.id = "v33", acir.source_type = "bool"} : i1 -> i1 loc(#loc68)
    cf.cond_br %18, ^bb6, ^bb7(%false : i1)
  ^bb6:  // pred: ^bb5
    %19 = acir.extract %13 {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc69)
    %20 = acir.extract %19 {acir.id = "v36", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc69)
    %21 = arith.cmpi eq, %20, %14 {acir.id = "v37", acir.source_type = "bool"} : i32 loc(#loc69)
    cf.br ^bb7(%21 : i1)
  ^bb7(%22: i1):  // 2 preds: ^bb5, ^bb6
    cf.cond_br %22, ^bb8, ^bb9(%12, %13 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb8:  // pred: ^bb7
    %23 = acir.aggregate(%true, %14, %15) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc70)
    %24 = acir.update %13, %23 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc71)
    cf.br ^bb9(%true, %24 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb9(%25: i1, %26: !acir.struct<"src_payloads_25ebcaa7.Entry">):  // 3 preds: ^bb0, ^bb7, ^bb8
    cf.cond_br %25, ^bb10, ^bb11
  ^bb10:  // pred: ^bb9
    acir.write %0, %26 {acir.id = "v61", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc72)
    cf.br ^bb11
  ^bb11:  // 2 preds: ^bb9, ^bb10
    return
  }
  func.func @cdb.broadcast.0.call.14.26(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:right", definition = "wakeup", instance = "right", interface = "Module:right:wakeup", kind = "method", method = "wakeup", name = "cdb.broadcast.0.call.14.26", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "right.slot" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc60)
    %2 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc61)
    cf.cond_br %2, ^bb1, ^bb9(%false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc62)
    %4 = acir.extract %3 {acir.id = "v9", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc62)
    %5 = acir.unary %4 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc63)
    cf.cond_br %5, ^bb2, ^bb3(%false : i1)
  ^bb2:  // pred: ^bb1
    %6 = acir.extract %1 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc64)
    %7 = acir.extract %6 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc64)
    %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc64)
    cf.br ^bb3(%8 : i1)
  ^bb3(%9: i1):  // 2 preds: ^bb1, ^bb2
    cf.cond_br %9, ^bb4, ^bb5(%false, %1, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb4:  // pred: ^bb3
    %10 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc65)
    %11 = acir.update %1, %10 {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc66)
    cf.br ^bb5(%true, %11, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb5(%12: i1, %13: !acir.struct<"src_payloads_25ebcaa7.Entry">, %14: i32, %15: i32):  // 2 preds: ^bb3, ^bb4
    %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc67)
    %17 = acir.extract %16 {acir.id = "v32", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc67)
    %18 = acir.unary %17 "not" {acir.id = "v33", acir.source_type = "bool"} : i1 -> i1 loc(#loc68)
    cf.cond_br %18, ^bb6, ^bb7(%false : i1)
  ^bb6:  // pred: ^bb5
    %19 = acir.extract %13 {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc69)
    %20 = acir.extract %19 {acir.id = "v36", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc69)
    %21 = arith.cmpi eq, %20, %14 {acir.id = "v37", acir.source_type = "bool"} : i32 loc(#loc69)
    cf.br ^bb7(%21 : i1)
  ^bb7(%22: i1):  // 2 preds: ^bb5, ^bb6
    cf.cond_br %22, ^bb8, ^bb9(%12, %13 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb8:  // pred: ^bb7
    %23 = acir.aggregate(%true, %14, %15) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc70)
    %24 = acir.update %13, %23 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc71)
    cf.br ^bb9(%true, %24 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb9(%25: i1, %26: !acir.struct<"src_payloads_25ebcaa7.Entry">):  // 3 preds: ^bb0, ^bb7, ^bb8
    cf.cond_br %25, ^bb10, ^bb11
  ^bb10:  // pred: ^bb9
    acir.write %0, %26 {acir.id = "v61", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc72)
    cf.br ^bb11
  ^bb11:  // 2 preds: ^bb9, ^bb10
    return
  }
  func.func @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.15.8() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.15.8", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.16.13() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.16.13", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.17.18() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.17.18", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.18.29() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.18.29", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @dispatch.dispatch.0.call.15.6.0.call.15.31(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "dispatch.dispatch.0.call.15.6.0.call.15.31", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc75)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc76)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc77)
    %1 = arith.andi %0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc78)
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc79)
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = call @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.15.8() {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc80)
    return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc81)
  ^bb2:  // pred: ^bb0
    %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc82)
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = call @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.16.13() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc83)
    return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc84)
  ^bb4:  // pred: ^bb2
    %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc85)
    cf.cond_br %6, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %7 = call @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.17.18() {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc86)
    return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc87)
  ^bb6:  // pred: ^bb4
    %8 = call @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.18.29() {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc88)
    return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc89)
  }
  func.func @dispatch.dispatch.0.call.15.6(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> attributes {acir.function = {contract = "Module:rf", definition = "operand", instance = "rf", interface = "Module:rf:operand", kind = "method", method = "operand", name = "dispatch.dispatch.0.call.15.6", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Operand"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc90)
    %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v3", acir.source_type = "bool"} : i32 loc(#loc91)
    acir.check %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc92)
    %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc93)
    %2 = acir.get "rf.renamed" {acir.id = "v15", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc94)
    %3 = acir.get "rf.committed" {acir.id = "v18", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc95)
    %4 = acir.get "rf.registers" {acir.id = "v24", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc96)
    cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    %5 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc97)
    return %5 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc98)
  ^bb2(%6: i32):  // pred: ^bb0
    %7 = acir.read %2 {acir.id = "v16", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc99)
    %8 = acir.extract %7[%6] {acir.id = "v17", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc99)
    %9 = acir.read %3 {acir.id = "v19", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc100)
    %10 = acir.extract %9[%6] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc100)
    %11 = arith.cmpi eq, %8, %10 {acir.id = "v21", acir.source_type = "bool"} : i32 loc(#loc101)
    cf.cond_br %11, ^bb3, ^bb4(%8 : i32)
  ^bb3:  // pred: ^bb2
    %12 = acir.read %4 {acir.id = "v25", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc102)
    %13 = acir.extract %12[%6] {acir.id = "v26", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc102)
    %14 = acir.aggregate(%true, %8, %13) {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc103)
    return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc104)
  ^bb4(%15: i32):  // pred: ^bb2
    %16 = call @dispatch.dispatch.0.call.15.6.0.call.15.31(%15) {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:lookup", method = "lookup", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
    %17 = acir.extract %16 {acir.id = "v33", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc106)
    cf.cond_br %17, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %18 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
    cf.br ^bb6(%18 : i1)
  ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
    cf.cond_br %19, ^bb7, ^bb8(%false : i1)
  ^bb7:  // pred: ^bb6
    %20 = acir.extract %16 {acir.id = "v41", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc108)
    %21 = arith.cmpi eq, %20, %15 {acir.id = "v42", acir.source_type = "bool"} : i32 loc(#loc108)
    cf.br ^bb8(%21 : i1)
  ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
    cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
  ^bb9:  // pred: ^bb8
    %23 = acir.extract %16 {acir.id = "v49", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc109)
    %24 = acir.aggregate(%true, %15, %23) {acir.id = "v50", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc110)
    return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc111)
  ^bb10(%25: i32):  // pred: ^bb8
    %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v58", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc112)
    return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc113)
  }
  func.func @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.21.8() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.21.8", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.22.13() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.22.13", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.23.18() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.23.18", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.24.29() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.24.29", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @dispatch.dispatch.0.call.21.8.0.call.21.31(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "dispatch.dispatch.0.call.21.8.0.call.21.31", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc75)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc76)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc77)
    %1 = arith.andi %0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc78)
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc79)
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = call @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.21.8() {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc80)
    return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc81)
  ^bb2:  // pred: ^bb0
    %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc82)
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = call @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.22.13() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc83)
    return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc84)
  ^bb4:  // pred: ^bb2
    %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc85)
    cf.cond_br %6, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %7 = call @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.23.18() {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc86)
    return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc87)
  ^bb6:  // pred: ^bb4
    %8 = call @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.24.29() {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc88)
    return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc89)
  }
  func.func @dispatch.dispatch.0.call.21.8(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> attributes {acir.function = {contract = "Module:rf", definition = "operand", instance = "rf", interface = "Module:rf:operand", kind = "method", method = "operand", name = "dispatch.dispatch.0.call.21.8", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Operand"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc90)
    %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v3", acir.source_type = "bool"} : i32 loc(#loc91)
    acir.check %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc92)
    %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc93)
    %2 = acir.get "rf.renamed" {acir.id = "v15", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc94)
    %3 = acir.get "rf.committed" {acir.id = "v18", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc95)
    %4 = acir.get "rf.registers" {acir.id = "v24", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc96)
    cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    %5 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc97)
    return %5 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc98)
  ^bb2(%6: i32):  // pred: ^bb0
    %7 = acir.read %2 {acir.id = "v16", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc99)
    %8 = acir.extract %7[%6] {acir.id = "v17", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc99)
    %9 = acir.read %3 {acir.id = "v19", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc100)
    %10 = acir.extract %9[%6] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc100)
    %11 = arith.cmpi eq, %8, %10 {acir.id = "v21", acir.source_type = "bool"} : i32 loc(#loc101)
    cf.cond_br %11, ^bb3, ^bb4(%8 : i32)
  ^bb3:  // pred: ^bb2
    %12 = acir.read %4 {acir.id = "v25", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc102)
    %13 = acir.extract %12[%6] {acir.id = "v26", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc102)
    %14 = acir.aggregate(%true, %8, %13) {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc103)
    return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc104)
  ^bb4(%15: i32):  // pred: ^bb2
    %16 = call @dispatch.dispatch.0.call.21.8.0.call.21.31(%15) {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:lookup", method = "lookup", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
    %17 = acir.extract %16 {acir.id = "v33", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc106)
    cf.cond_br %17, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %18 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
    cf.br ^bb6(%18 : i1)
  ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
    cf.cond_br %19, ^bb7, ^bb8(%false : i1)
  ^bb7:  // pred: ^bb6
    %20 = acir.extract %16 {acir.id = "v41", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc108)
    %21 = arith.cmpi eq, %20, %15 {acir.id = "v42", acir.source_type = "bool"} : i32 loc(#loc108)
    cf.br ^bb8(%21 : i1)
  ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
    cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
  ^bb9:  // pred: ^bb8
    %23 = acir.extract %16 {acir.id = "v49", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc109)
    %24 = acir.aggregate(%true, %15, %23) {acir.id = "v50", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc110)
    return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc111)
  ^bb10(%25: i32):  // pred: ^bb8
    %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v58", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc112)
    return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc113)
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.27.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "room", instance = "rob", kind = "guard", name = "dispatch.dispatch.0.call.27.10.0.call.27.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc114)
    %0 = acir.get "rob.tail" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc115)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc116)
    %2 = acir.get "rob.head" {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc117)
    %3 = acir.read %2 {acir.id = "v5", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc118)
    %4 = arith.subi %1, %3 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc116)
    %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc116)
    return %5 : i1 loc(#loc119)
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.28.12.0.call.28.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot0", kind = "guard", name = "dispatch.dispatch.0.call.27.10.0.call.28.12.0.call.28.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.28.12(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "allocate", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.10.0.call.28.12", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @dispatch.dispatch.0.call.27.10.0.call.28.12.0.call.28.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32, i32) -> i1 loc(#loc126)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot0.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.30.16.0.call.30.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot1", kind = "guard", name = "dispatch.dispatch.0.call.27.10.0.call.30.16.0.call.30.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.30.16(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "allocate", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.10.0.call.30.16", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @dispatch.dispatch.0.call.27.10.0.call.30.16.0.call.30.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32, i32) -> i1 loc(#loc130)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot1.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.32.20.0.call.32.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot2", kind = "guard", name = "dispatch.dispatch.0.call.27.10.0.call.32.20.0.call.32.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.32.20(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "allocate", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.10.0.call.32.20", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @dispatch.dispatch.0.call.27.10.0.call.32.20.0.call.32.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32, i32) -> i1 loc(#loc131)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot2.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.34.21.0.call.34.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot3", kind = "guard", name = "dispatch.dispatch.0.call.27.10.0.call.34.21.0.call.34.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.34.21(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "allocate", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.10.0.call.34.21", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @dispatch.dispatch.0.call.27.10.0.call.34.21.0.call.34.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32, i32) -> i1 loc(#loc132)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot3.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @dispatch.dispatch.0.call.27.10(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:rob", definition = "allocate", instance = "rob", interface = "Module:rob:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.10", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc133)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc134)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = call @dispatch.dispatch.0.call.27.10.0.call.27.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob", role = "guard"} : (i32) -> i1 loc(#loc135)
    acir.guard %0 "room" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc136)
    %1 = acir.get "rob.tail" {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc115)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc137)
    %3 = arith.subi %2, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc138)
    %4 = arith.andi %3, %c3_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc139)
    %5 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc140)
    cf.cond_br %5, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    call @dispatch.dispatch.0.call.27.10.0.call.28.12(%2, %arg0) {acir.id = "v13", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", method = "allocate", role = "method"} : (i32, i32) -> () loc(#loc126)
    cf.br ^bb7(%2 : i32)
  ^bb2:  // pred: ^bb0
    %6 = arith.cmpi eq, %4, %c1_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc141)
    cf.cond_br %6, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    call @dispatch.dispatch.0.call.27.10.0.call.30.16(%2, %arg0) {acir.id = "v17", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", method = "allocate", role = "method"} : (i32, i32) -> () loc(#loc130)
    cf.br ^bb7(%2 : i32)
  ^bb4:  // pred: ^bb2
    %7 = arith.cmpi eq, %4, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc142)
    cf.cond_br %7, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    call @dispatch.dispatch.0.call.27.10.0.call.32.20(%2, %arg0) {acir.id = "v21", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", method = "allocate", role = "method"} : (i32, i32) -> () loc(#loc131)
    cf.br ^bb7(%2 : i32)
  ^bb6:  // pred: ^bb4
    call @dispatch.dispatch.0.call.27.10.0.call.34.21(%2, %arg0) {acir.id = "v22", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", method = "allocate", role = "method"} : (i32, i32) -> () loc(#loc132)
    cf.br ^bb7(%2 : i32)
  ^bb7(%8: i32):  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    %9 = arith.addi %8, %c1_i32 {acir.id = "v39", acir.source_type = "u32"} : i32 loc(#loc143)
    acir.write %1, %9 {acir.id = "v40", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc144)
    return %8 : i32 loc(#loc145)
  }
  func.func @dispatch.dispatch.0.call.37.12(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rf", definition = "rename", instance = "rf", interface = "Module:rf:rename", kind = "method", method = "rename", name = "dispatch.dispatch.0.call.37.12", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}, {consume = false, id = "v2", name = "tag", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc146)
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc147)
    %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc148)
    acir.check %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc149)
    %1 = arith.cmpi ne, %arg0, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc150)
    %2 = acir.get "rf.renamed" {acir.id = "v9", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc94)
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = acir.read %2 {acir.id = "v10", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc151)
    %4 = acir.update %3, %arg1[%arg0] {acir.id = "v11", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc151)
    acir.write %2, %4 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc151)
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @dispatch.dispatch.0.call.38.22.0.call.38.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "left", kind = "guard", name = "dispatch.dispatch.0.call.38.22.0.call.38.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.get "left.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc152)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc152)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc153)
    return %3 : i1 loc(#loc154)
  }
  func.func @dispatch.dispatch.0.call.38.22(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) attributes {acir.function = {contract = "Module:left", definition = "place", instance = "left", interface = "Module:left:place", kind = "method", method = "place", name = "dispatch.dispatch.0.call.38.22", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void"}} {
    %0 = call @dispatch.dispatch.0.call.38.22.0.call.38.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc155)
    acir.guard %0 "free" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc156)
    %1 = acir.get "left.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    acir.write %1, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc157)
    return
  }
  func.func @dispatch.dispatch.0.call.40.23.0.call.40.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "right", kind = "guard", name = "dispatch.dispatch.0.call.40.23.0.call.40.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.get "right.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc152)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc152)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc153)
    return %3 : i1 loc(#loc154)
  }
  func.func @dispatch.dispatch.0.call.40.23(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) attributes {acir.function = {contract = "Module:right", definition = "place", instance = "right", interface = "Module:right:place", kind = "method", method = "place", name = "dispatch.dispatch.0.call.40.23", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void"}} {
    %0 = call @dispatch.dispatch.0.call.40.23.0.call.40.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc158)
    acir.guard %0 "free" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc156)
    %1 = acir.get "right.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    acir.write %1, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc157)
    return
  }
  func.func @commit.commit.0.call.42.0.0.call.42.9.0.call.42.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot0", kind = "guard", name = "commit.commit.0.call.42.0.0.call.42.9.0.call.42.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @commit.commit.0.call.42.0.0.call.42.9(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "retire", instance = "rob.slot0", interface = "Module:rob.slot0:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.0.0.call.42.9", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @commit.commit.0.call.42.0.0.call.42.9.0.call.42.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32) -> i1 loc(#loc164)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot0.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @commit.commit.0.call.42.0.0.call.44.13.0.call.44.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot1", kind = "guard", name = "commit.commit.0.call.42.0.0.call.44.13.0.call.44.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @commit.commit.0.call.42.0.0.call.44.13(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "retire", instance = "rob.slot1", interface = "Module:rob.slot1:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.0.0.call.44.13", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @commit.commit.0.call.42.0.0.call.44.13.0.call.44.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32) -> i1 loc(#loc170)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot1.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @commit.commit.0.call.42.0.0.call.46.17.0.call.46.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot2", kind = "guard", name = "commit.commit.0.call.42.0.0.call.46.17.0.call.46.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @commit.commit.0.call.42.0.0.call.46.17(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "retire", instance = "rob.slot2", interface = "Module:rob.slot2:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.0.0.call.46.17", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @commit.commit.0.call.42.0.0.call.46.17.0.call.46.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32) -> i1 loc(#loc171)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot2.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @commit.commit.0.call.42.0.0.call.48.18.0.call.48.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot3", kind = "guard", name = "commit.commit.0.call.42.0.0.call.48.18.0.call.48.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @commit.commit.0.call.42.0.0.call.48.18(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "retire", instance = "rob.slot3", interface = "Module:rob.slot3:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.0.0.call.48.18", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @commit.commit.0.call.42.0.0.call.48.18.0.call.48.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32) -> i1 loc(#loc172)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot3.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @commit.commit.0.call.42.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "retire", instance = "rob", interface = "Module:rob:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.0", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc173)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc174)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = acir.get "rob.head" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc117)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc175)
    %2 = arith.subi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc176)
    %3 = arith.andi %2, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc177)
    %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc178)
    cf.cond_br %4, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %5 = call @commit.commit.0.call.42.0.0.call.42.9(%1) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc164)
    cf.br ^bb7(%5, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb2:  // pred: ^bb0
    %6 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc179)
    cf.cond_br %6, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %7 = call @commit.commit.0.call.42.0.0.call.44.13(%1) {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc170)
    cf.br ^bb7(%7, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb4:  // pred: ^bb2
    %8 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc180)
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = call @commit.commit.0.call.42.0.0.call.46.17(%1) {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc171)
    cf.br ^bb7(%9, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb6:  // pred: ^bb4
    %10 = call @commit.commit.0.call.42.0.0.call.48.18(%1) {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc172)
    cf.br ^bb7(%10, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb7(%11: !acir.struct<"src_payloads_25ebcaa7.RobEntry">, %12: i32):  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    %13 = arith.addi %12, %c1_i32 {acir.id = "v36", acir.source_type = "u32"} : i32 loc(#loc181)
    acir.write %0, %13 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc182)
    return %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc183)
  }
  func.func @commit.commit.0.call.51.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.RobEntry">) attributes {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "commit.commit.0.call.51.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.RobEntry"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc184)
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc185)
    %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc185)
    %2 = acir.get "rf.registers" {acir.id = "v6", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc96)
    %3 = acir.get "rf.committed" {acir.id = "v12", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc95)
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %4 = acir.extract %arg0 {acir.id = "v7", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc186)
    %5 = acir.extract %arg0 {acir.id = "v8", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc187)
    %6 = acir.read %2 {acir.id = "v9", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc188)
    %7 = acir.update %6, %4[%5] {acir.id = "v10", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc188)
    acir.write %2, %7 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc188)
    %8 = acir.extract %arg0 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc189)
    %9 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc190)
    %10 = acir.read %3 {acir.id = "v15", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc191)
    %11 = acir.update %10, %8[%9] {acir.id = "v16", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc191)
    acir.write %3, %11 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc191)
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @ac_summary_rob.slot0.allocate.0.call.52.2.0.call.52.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.slot0.allocate.0.call.52.2.0.call.52.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @ac_summary_rob.slot0.allocate.0.call.52.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "allocate", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot0.allocate.0.call.52.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @ac_summary_rob.slot0.allocate.0.call.52.2.0.call.52.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32, i32) -> i1 loc(#loc192)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot0.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @ac_summary_rob.slot0.complete.0.call.54.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "complete", instance = "rob.slot0", interface = "Module:rob.slot0:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot0.complete.0.call.54.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.slot0.peek.0.call.55.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot0.peek.0.call.55.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rob.slot0.retire.0.call.56.1.0.call.56.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.slot0.retire.0.call.56.1.0.call.56.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @ac_summary_rob.slot0.retire.0.call.56.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "retire", instance = "rob.slot0", interface = "Module:rob.slot0:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot0.retire.0.call.56.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.slot0.retire.0.call.56.1.0.call.56.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32) -> i1 loc(#loc193)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot0.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @ac_summary_rob.slot1.allocate.0.call.58.2.0.call.58.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.slot1.allocate.0.call.58.2.0.call.58.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @ac_summary_rob.slot1.allocate.0.call.58.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "allocate", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot1.allocate.0.call.58.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @ac_summary_rob.slot1.allocate.0.call.58.2.0.call.58.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32, i32) -> i1 loc(#loc192)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot1.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @ac_summary_rob.slot1.complete.0.call.60.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "complete", instance = "rob.slot1", interface = "Module:rob.slot1:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot1.complete.0.call.60.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.slot1.peek.0.call.61.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot1.peek.0.call.61.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rob.slot1.retire.0.call.62.1.0.call.62.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.slot1.retire.0.call.62.1.0.call.62.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @ac_summary_rob.slot1.retire.0.call.62.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "retire", instance = "rob.slot1", interface = "Module:rob.slot1:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot1.retire.0.call.62.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.slot1.retire.0.call.62.1.0.call.62.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32) -> i1 loc(#loc193)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot1.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @ac_summary_rob.slot2.allocate.0.call.64.2.0.call.64.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.slot2.allocate.0.call.64.2.0.call.64.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @ac_summary_rob.slot2.allocate.0.call.64.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "allocate", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot2.allocate.0.call.64.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @ac_summary_rob.slot2.allocate.0.call.64.2.0.call.64.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32, i32) -> i1 loc(#loc192)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot2.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @ac_summary_rob.slot2.complete.0.call.66.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "complete", instance = "rob.slot2", interface = "Module:rob.slot2:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot2.complete.0.call.66.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.slot2.peek.0.call.67.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot2.peek.0.call.67.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rob.slot2.retire.0.call.68.1.0.call.68.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.slot2.retire.0.call.68.1.0.call.68.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @ac_summary_rob.slot2.retire.0.call.68.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "retire", instance = "rob.slot2", interface = "Module:rob.slot2:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot2.retire.0.call.68.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.slot2.retire.0.call.68.1.0.call.68.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32) -> i1 loc(#loc193)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot2.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @ac_summary_rob.slot3.allocate.0.call.70.2.0.call.70.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.slot3.allocate.0.call.70.2.0.call.70.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @ac_summary_rob.slot3.allocate.0.call.70.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "allocate", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot3.allocate.0.call.70.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @ac_summary_rob.slot3.allocate.0.call.70.2.0.call.70.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32, i32) -> i1 loc(#loc192)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot3.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @ac_summary_rob.slot3.complete.0.call.72.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "complete", instance = "rob.slot3", interface = "Module:rob.slot3:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot3.complete.0.call.72.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.slot3.peek.0.call.73.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot3.peek.0.call.73.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rob.slot3.retire.0.call.74.1.0.call.74.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.slot3.retire.0.call.74.1.0.call.74.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @ac_summary_rob.slot3.retire.0.call.74.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "retire", instance = "rob.slot3", interface = "Module:rob.slot3:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot3.retire.0.call.74.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.slot3.retire.0.call.74.1.0.call.74.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32) -> i1 loc(#loc193)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot3.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @ac_summary_rob.has_room.0.call.76.0() -> i1 attributes {acir.function = {contract = "Module:rob", definition = "has_room", instance = "rob", interface = "Module:rob:has_room", kind = "method", method = "has_room", name = "ac_summary_rob.has_room.0.call.76.0", params = [], result = "bool", summary = true}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc194)
    %0 = acir.get "rob.tail" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc115)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc195)
    %2 = acir.get "rob.head" {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc117)
    %3 = acir.read %2 {acir.id = "v4", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc196)
    %4 = arith.subi %1, %3 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc195)
    %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc195)
    return %5 : i1 loc(#loc197)
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.77.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "room", instance = "rob", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.77.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc114)
    %0 = acir.get "rob.tail" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc115)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc116)
    %2 = acir.get "rob.head" {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc117)
    %3 = acir.read %2 {acir.id = "v5", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc118)
    %4 = arith.subi %1, %3 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc116)
    %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc116)
    return %5 : i1 loc(#loc119)
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.78.12.0.call.78.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.78.12.0.call.78.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.78.12(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "allocate", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.78.12", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @ac_summary_rob.allocate.0.call.77.1.0.call.78.12.0.call.78.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32, i32) -> i1 loc(#loc126)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot0.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.80.16.0.call.80.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.80.16.0.call.80.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.80.16(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "allocate", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.80.16", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @ac_summary_rob.allocate.0.call.77.1.0.call.80.16.0.call.80.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32, i32) -> i1 loc(#loc130)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot1.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.82.20.0.call.82.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.82.20.0.call.82.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.82.20(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "allocate", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.82.20", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @ac_summary_rob.allocate.0.call.77.1.0.call.82.20.0.call.82.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32, i32) -> i1 loc(#loc131)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot2.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.84.21.0.call.84.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.84.21.0.call.84.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc120)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc120)
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1 loc(#loc121)
    return %3 : i1 loc(#loc122)
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.84.21(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "allocate", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.84.21", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc123)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc124)
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc125)
    %0 = call @ac_summary_rob.allocate.0.call.77.1.0.call.84.21.0.call.84.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32, i32) -> i1 loc(#loc132)
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"} loc(#loc127)
    %1 = acir.get "rob.slot3.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc128)
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc129)
    return
  }
  func.func @ac_summary_rob.allocate.0.call.77.1(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:rob", definition = "allocate", instance = "rob", interface = "Module:rob:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "u32", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc133)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc134)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = call @ac_summary_rob.allocate.0.call.77.1.0.call.77.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob", role = "guard"} : (i32) -> i1 loc(#loc198)
    acir.guard %0 "room" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc136)
    %1 = acir.get "rob.tail" {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc115)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc137)
    %3 = arith.subi %2, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc138)
    %4 = arith.andi %3, %c3_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc139)
    %5 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc140)
    cf.cond_br %5, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    call @ac_summary_rob.allocate.0.call.77.1.0.call.78.12(%2, %arg0) {acir.id = "v13", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", method = "allocate", role = "method"} : (i32, i32) -> () loc(#loc126)
    cf.br ^bb7(%2 : i32)
  ^bb2:  // pred: ^bb0
    %6 = arith.cmpi eq, %4, %c1_i32 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc141)
    cf.cond_br %6, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    call @ac_summary_rob.allocate.0.call.77.1.0.call.80.16(%2, %arg0) {acir.id = "v17", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", method = "allocate", role = "method"} : (i32, i32) -> () loc(#loc130)
    cf.br ^bb7(%2 : i32)
  ^bb4:  // pred: ^bb2
    %7 = arith.cmpi eq, %4, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32 loc(#loc142)
    cf.cond_br %7, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    call @ac_summary_rob.allocate.0.call.77.1.0.call.82.20(%2, %arg0) {acir.id = "v21", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", method = "allocate", role = "method"} : (i32, i32) -> () loc(#loc131)
    cf.br ^bb7(%2 : i32)
  ^bb6:  // pred: ^bb4
    call @ac_summary_rob.allocate.0.call.77.1.0.call.84.21(%2, %arg0) {acir.id = "v22", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", method = "allocate", role = "method"} : (i32, i32) -> () loc(#loc132)
    cf.br ^bb7(%2 : i32)
  ^bb7(%8: i32):  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    %9 = arith.addi %8, %c1_i32 {acir.id = "v39", acir.source_type = "u32"} : i32 loc(#loc143)
    acir.write %1, %9 {acir.id = "v40", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc144)
    return %8 : i32 loc(#loc145)
  }
  func.func @ac_summary_rob.lookup.0.call.87.1.0.call.87.8() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.87.8", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rob.lookup.0.call.87.1.0.call.88.13() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.88.13", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rob.lookup.0.call.87.1.0.call.89.18() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.89.18", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rob.lookup.0.call.87.1.0.call.90.29() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.90.29", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rob.lookup.0.call.87.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "ac_summary_rob.lookup.0.call.87.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc75)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc76)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc77)
    %1 = arith.andi %0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc78)
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc79)
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = call @ac_summary_rob.lookup.0.call.87.1.0.call.87.8() {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc80)
    return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc81)
  ^bb2:  // pred: ^bb0
    %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc82)
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = call @ac_summary_rob.lookup.0.call.87.1.0.call.88.13() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc83)
    return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc84)
  ^bb4:  // pred: ^bb2
    %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc85)
    cf.cond_br %6, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %7 = call @ac_summary_rob.lookup.0.call.87.1.0.call.89.18() {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc86)
    return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc87)
  ^bb6:  // pred: ^bb4
    %8 = call @ac_summary_rob.lookup.0.call.87.1.0.call.90.29() {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc88)
    return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc89)
  }
  func.func @ac_summary_rob.complete.0.call.92.2.0.call.92.9(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "complete", instance = "rob.slot0", interface = "Module:rob.slot0:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.92.9", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.complete.0.call.92.2.0.call.93.13(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "complete", instance = "rob.slot1", interface = "Module:rob.slot1:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.93.13", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.complete.0.call.92.2.0.call.94.17(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "complete", instance = "rob.slot2", interface = "Module:rob.slot2:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.94.17", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.complete.0.call.92.2.0.call.95.18(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "complete", instance = "rob.slot3", interface = "Module:rob.slot3:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.95.18", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc41)
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc42)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc43)
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc44)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc45)
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32 loc(#loc45)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc46)
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc47)
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc48)
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.complete.0.call.92.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob", definition = "complete", instance = "rob", interface = "Module:rob:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc49)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc50)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc51)
    %1 = arith.andi %0, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc52)
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc53)
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    call @ac_summary_rob.complete.0.call.92.2.0.call.92.9(%arg0, %arg1) {acir.id = "v10", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:complete", method = "complete", role = "method"} : (i32, i32) -> () loc(#loc54)
    cf.br ^bb7
  ^bb2:  // pred: ^bb0
    %3 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc55)
    cf.cond_br %3, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    call @ac_summary_rob.complete.0.call.92.2.0.call.93.13(%arg0, %arg1) {acir.id = "v14", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:complete", method = "complete", role = "method"} : (i32, i32) -> () loc(#loc56)
    cf.br ^bb7
  ^bb4:  // pred: ^bb2
    %4 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc57)
    cf.cond_br %4, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    call @ac_summary_rob.complete.0.call.92.2.0.call.94.17(%arg0, %arg1) {acir.id = "v18", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:complete", method = "complete", role = "method"} : (i32, i32) -> () loc(#loc58)
    cf.br ^bb7
  ^bb6:  // pred: ^bb4
    call @ac_summary_rob.complete.0.call.92.2.0.call.95.18(%arg0, %arg1) {acir.id = "v19", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:complete", method = "complete", role = "method"} : (i32, i32) -> () loc(#loc59)
    cf.br ^bb7
  ^bb7:  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    return
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.97.9.0.call.97.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.97.9.0.call.97.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.97.9(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "retire", instance = "rob.slot0", interface = "Module:rob.slot0:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.97.9", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.retire.0.call.97.0.0.call.97.9.0.call.97.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32) -> i1 loc(#loc164)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot0.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.99.13.0.call.99.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.99.13.0.call.99.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.99.13(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "retire", instance = "rob.slot1", interface = "Module:rob.slot1:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.99.13", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.retire.0.call.97.0.0.call.99.13.0.call.99.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32) -> i1 loc(#loc170)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot1.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.101.17.0.call.101.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.101.17.0.call.101.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.101.17(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "retire", instance = "rob.slot2", interface = "Module:rob.slot2:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.101.17", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.retire.0.call.97.0.0.call.101.17.0.call.101.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32) -> i1 loc(#loc171)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot2.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.103.18.0.call.103.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.103.18.0.call.103.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc159)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc160)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc161)
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc162)
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32 loc(#loc162)
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1 loc(#loc163)
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.103.18(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "retire", instance = "rob.slot3", interface = "Module:rob.slot3:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.103.18", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.retire.0.call.97.0.0.call.103.18.0.call.103.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32) -> i1 loc(#loc172)
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc165)
    %1 = acir.get "rob.slot3.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc166)
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc167)
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc168)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc169)
  }
  func.func @ac_summary_rob.retire.0.call.97.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "retire", instance = "rob", interface = "Module:rob:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc173)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc174)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = acir.get "rob.head" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc117)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc175)
    %2 = arith.subi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc176)
    %3 = arith.andi %2, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc177)
    %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc178)
    cf.cond_br %4, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %5 = call @ac_summary_rob.retire.0.call.97.0.0.call.97.9(%1) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc164)
    cf.br ^bb7(%5, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb2:  // pred: ^bb0
    %6 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc179)
    cf.cond_br %6, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %7 = call @ac_summary_rob.retire.0.call.97.0.0.call.99.13(%1) {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc170)
    cf.br ^bb7(%7, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb4:  // pred: ^bb2
    %8 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc180)
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = call @ac_summary_rob.retire.0.call.97.0.0.call.101.17(%1) {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc171)
    cf.br ^bb7(%9, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb6:  // pred: ^bb4
    %10 = call @ac_summary_rob.retire.0.call.97.0.0.call.103.18(%1) {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc172)
    cf.br ^bb7(%10, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb7(%11: !acir.struct<"src_payloads_25ebcaa7.RobEntry">, %12: i32):  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    %13 = arith.addi %12, %c1_i32 {acir.id = "v36", acir.source_type = "u32"} : i32 loc(#loc181)
    acir.write %0, %13 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc182)
    return %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc183)
  }
  func.func @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.106.8() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.106.8", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.107.13() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.107.13", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.108.18() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.108.18", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.109.29() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.109.29", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> loc(#loc40)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc73)
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc74)
  }
  func.func @ac_summary_rf.operand.0.call.106.1.0.call.106.31(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.31", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc75)
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc76)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc77)
    %1 = arith.andi %0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc78)
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc79)
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = call @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.106.8() {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc80)
    return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc81)
  ^bb2:  // pred: ^bb0
    %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc82)
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = call @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.107.13() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc83)
    return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc84)
  ^bb4:  // pred: ^bb2
    %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32 loc(#loc85)
    cf.cond_br %6, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %7 = call @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.108.18() {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc86)
    return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc87)
  ^bb6:  // pred: ^bb4
    %8 = call @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.109.29() {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc88)
    return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc89)
  }
  func.func @ac_summary_rf.operand.0.call.106.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> attributes {acir.function = {contract = "Module:rf", definition = "operand", instance = "rf", interface = "Module:rf:operand", kind = "method", method = "operand", name = "ac_summary_rf.operand.0.call.106.1", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Operand", summary = true}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc90)
    %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v3", acir.source_type = "bool"} : i32 loc(#loc91)
    acir.check %0 {acir.id = "v4", acir.source_type = "void"} loc(#loc92)
    %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32 loc(#loc93)
    %2 = acir.get "rf.renamed" {acir.id = "v15", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc94)
    %3 = acir.get "rf.committed" {acir.id = "v18", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc95)
    %4 = acir.get "rf.registers" {acir.id = "v24", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc96)
    cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    %5 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc97)
    return %5 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc98)
  ^bb2(%6: i32):  // pred: ^bb0
    %7 = acir.read %2 {acir.id = "v16", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc99)
    %8 = acir.extract %7[%6] {acir.id = "v17", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc99)
    %9 = acir.read %3 {acir.id = "v19", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc100)
    %10 = acir.extract %9[%6] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc100)
    %11 = arith.cmpi eq, %8, %10 {acir.id = "v21", acir.source_type = "bool"} : i32 loc(#loc101)
    cf.cond_br %11, ^bb3, ^bb4(%8 : i32)
  ^bb3:  // pred: ^bb2
    %12 = acir.read %4 {acir.id = "v25", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc102)
    %13 = acir.extract %12[%6] {acir.id = "v26", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32 loc(#loc102)
    %14 = acir.aggregate(%true, %8, %13) {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc103)
    return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc104)
  ^bb4(%15: i32):  // pred: ^bb2
    %16 = call @ac_summary_rf.operand.0.call.106.1.0.call.106.31(%15) {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:lookup", method = "lookup", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc105)
    %17 = acir.extract %16 {acir.id = "v33", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc106)
    cf.cond_br %17, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %18 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1 loc(#loc107)
    cf.br ^bb6(%18 : i1)
  ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
    cf.cond_br %19, ^bb7, ^bb8(%false : i1)
  ^bb7:  // pred: ^bb6
    %20 = acir.extract %16 {acir.id = "v41", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc108)
    %21 = arith.cmpi eq, %20, %15 {acir.id = "v42", acir.source_type = "bool"} : i32 loc(#loc108)
    cf.br ^bb8(%21 : i1)
  ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
    cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
  ^bb9:  // pred: ^bb8
    %23 = acir.extract %16 {acir.id = "v49", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc109)
    %24 = acir.aggregate(%true, %15, %23) {acir.id = "v50", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc110)
    return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc111)
  ^bb10(%25: i32):  // pred: ^bb8
    %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v58", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc112)
    return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc113)
  }
  func.func @ac_summary_rf.rename.0.call.112.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rf", definition = "rename", instance = "rf", interface = "Module:rf:rename", kind = "method", method = "rename", name = "ac_summary_rf.rename.0.call.112.2", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}, {consume = false, id = "v2", name = "tag", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc146)
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32 loc(#loc147)
    %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc148)
    acir.check %0 {acir.id = "v5", acir.source_type = "void"} loc(#loc149)
    %1 = arith.cmpi ne, %arg0, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc150)
    %2 = acir.get "rf.renamed" {acir.id = "v9", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc94)
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = acir.read %2 {acir.id = "v10", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc151)
    %4 = acir.update %3, %arg1[%arg0] {acir.id = "v11", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc151)
    acir.write %2, %4 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc151)
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @ac_summary_rf.commit.0.call.113.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.RobEntry">) attributes {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "ac_summary_rf.commit.0.call.113.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.RobEntry"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc184)
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc185)
    %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc185)
    %2 = acir.get "rf.registers" {acir.id = "v6", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc96)
    %3 = acir.get "rf.committed" {acir.id = "v12", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>> loc(#loc95)
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %4 = acir.extract %arg0 {acir.id = "v7", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc186)
    %5 = acir.extract %arg0 {acir.id = "v8", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc187)
    %6 = acir.read %2 {acir.id = "v9", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc188)
    %7 = acir.update %6, %4[%5] {acir.id = "v10", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc188)
    acir.write %2, %7 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc188)
    %8 = acir.extract %arg0 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc189)
    %9 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc190)
    %10 = acir.read %3 {acir.id = "v15", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8> loc(#loc191)
    %11 = acir.update %10, %8[%9] {acir.id = "v16", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8> loc(#loc191)
    acir.write %3, %11 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8> loc(#loc191)
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @ac_summary_left.place.0.call.114.1.0.call.114.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "left", kind = "guard", name = "ac_summary_left.place.0.call.114.1.0.call.114.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.get "left.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc152)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc152)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc153)
    return %3 : i1 loc(#loc154)
  }
  func.func @ac_summary_left.place.0.call.114.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) attributes {acir.function = {contract = "Module:left", definition = "place", instance = "left", interface = "Module:left:place", kind = "method", method = "place", name = "ac_summary_left.place.0.call.114.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void", summary = true}} {
    %0 = call @ac_summary_left.place.0.call.114.1.0.call.114.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc199)
    acir.guard %0 "free" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc156)
    %1 = acir.get "left.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    acir.write %1, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc157)
    return
  }
  func.func @ac_summary_left.take.0.call.116.1.0.call.116.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "left", kind = "guard", name = "ac_summary_left.take.0.call.116.1.0.call.116.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc25)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc26)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc27)
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc27)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc28)
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc28)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc29)
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1 loc(#loc30)
  }
  func.func @ac_summary_left.take.0.call.116.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:left", definition = "take", instance = "left", interface = "Module:left:take", kind = "method", method = "take", name = "ac_summary_left.take.0.call.116.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc31)
    %0 = call @ac_summary_left.take.0.call.116.1.0.call.116.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (i32) -> i1 loc(#loc200)
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc33)
    %1 = acir.get "left.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc34)
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc31)
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc35)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
  }
  func.func @ac_summary_left.wakeup.0.call.118.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:left", definition = "wakeup", instance = "left", interface = "Module:left:wakeup", kind = "method", method = "wakeup", name = "ac_summary_left.wakeup.0.call.118.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc60)
    %2 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc61)
    cf.cond_br %2, ^bb1, ^bb9(%false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc62)
    %4 = acir.extract %3 {acir.id = "v9", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc62)
    %5 = acir.unary %4 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc63)
    cf.cond_br %5, ^bb2, ^bb3(%false : i1)
  ^bb2:  // pred: ^bb1
    %6 = acir.extract %1 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc64)
    %7 = acir.extract %6 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc64)
    %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc64)
    cf.br ^bb3(%8 : i1)
  ^bb3(%9: i1):  // 2 preds: ^bb1, ^bb2
    cf.cond_br %9, ^bb4, ^bb5(%false, %1, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb4:  // pred: ^bb3
    %10 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc65)
    %11 = acir.update %1, %10 {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc66)
    cf.br ^bb5(%true, %11, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb5(%12: i1, %13: !acir.struct<"src_payloads_25ebcaa7.Entry">, %14: i32, %15: i32):  // 2 preds: ^bb3, ^bb4
    %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc67)
    %17 = acir.extract %16 {acir.id = "v32", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc67)
    %18 = acir.unary %17 "not" {acir.id = "v33", acir.source_type = "bool"} : i1 -> i1 loc(#loc68)
    cf.cond_br %18, ^bb6, ^bb7(%false : i1)
  ^bb6:  // pred: ^bb5
    %19 = acir.extract %13 {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc69)
    %20 = acir.extract %19 {acir.id = "v36", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc69)
    %21 = arith.cmpi eq, %20, %14 {acir.id = "v37", acir.source_type = "bool"} : i32 loc(#loc69)
    cf.br ^bb7(%21 : i1)
  ^bb7(%22: i1):  // 2 preds: ^bb5, ^bb6
    cf.cond_br %22, ^bb8, ^bb9(%12, %13 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb8:  // pred: ^bb7
    %23 = acir.aggregate(%true, %14, %15) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc70)
    %24 = acir.update %13, %23 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc71)
    cf.br ^bb9(%true, %24 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb9(%25: i1, %26: !acir.struct<"src_payloads_25ebcaa7.Entry">):  // 3 preds: ^bb0, ^bb7, ^bb8
    cf.cond_br %25, ^bb10, ^bb11
  ^bb10:  // pred: ^bb9
    acir.write %0, %26 {acir.id = "v61", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc72)
    cf.br ^bb11
  ^bb11:  // 2 preds: ^bb9, ^bb10
    return
  }
  func.func @ac_summary_right.place.0.call.119.1.0.call.119.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "right", kind = "guard", name = "ac_summary_right.place.0.call.119.1.0.call.119.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.get "right.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc152)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc152)
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1 loc(#loc153)
    return %3 : i1 loc(#loc154)
  }
  func.func @ac_summary_right.place.0.call.119.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) attributes {acir.function = {contract = "Module:right", definition = "place", instance = "right", interface = "Module:right:place", kind = "method", method = "place", name = "ac_summary_right.place.0.call.119.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void", summary = true}} {
    %0 = call @ac_summary_right.place.0.call.119.1.0.call.119.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc199)
    acir.guard %0 "free" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc156)
    %1 = acir.get "right.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    acir.write %1, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc157)
    return
  }
  func.func @ac_summary_right.take.0.call.121.1.0.call.121.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "right", kind = "guard", name = "ac_summary_right.take.0.call.121.1.0.call.121.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "right.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc25)
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc26)
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc27)
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc27)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc28)
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc28)
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc29)
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc29)
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1 loc(#loc30)
  }
  func.func @ac_summary_right.take.0.call.121.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:right", definition = "take", instance = "right", interface = "Module:right:take", kind = "method", method = "take", name = "ac_summary_right.take.0.call.121.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc31)
    %0 = call @ac_summary_right.take.0.call.121.1.0.call.121.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (i32) -> i1 loc(#loc200)
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"} loc(#loc33)
    %1 = acir.get "right.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc34)
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc31)
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc31)
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc35)
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc36)
  }
  func.func @ac_summary_right.wakeup.0.call.123.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:right", definition = "wakeup", instance = "right", interface = "Module:right:wakeup", kind = "method", method = "wakeup", name = "ac_summary_right.wakeup.0.call.123.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "right.slot" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc60)
    %2 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc61)
    cf.cond_br %2, ^bb1, ^bb9(%false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc62)
    %4 = acir.extract %3 {acir.id = "v9", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc62)
    %5 = acir.unary %4 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1 loc(#loc63)
    cf.cond_br %5, ^bb2, ^bb3(%false : i1)
  ^bb2:  // pred: ^bb1
    %6 = acir.extract %1 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc64)
    %7 = acir.extract %6 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc64)
    %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc64)
    cf.br ^bb3(%8 : i1)
  ^bb3(%9: i1):  // 2 preds: ^bb1, ^bb2
    cf.cond_br %9, ^bb4, ^bb5(%false, %1, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb4:  // pred: ^bb3
    %10 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc65)
    %11 = acir.update %1, %10 {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc66)
    cf.br ^bb5(%true, %11, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb5(%12: i1, %13: !acir.struct<"src_payloads_25ebcaa7.Entry">, %14: i32, %15: i32):  // 2 preds: ^bb3, ^bb4
    %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc67)
    %17 = acir.extract %16 {acir.id = "v32", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc67)
    %18 = acir.unary %17 "not" {acir.id = "v33", acir.source_type = "bool"} : i1 -> i1 loc(#loc68)
    cf.cond_br %18, ^bb6, ^bb7(%false : i1)
  ^bb6:  // pred: ^bb5
    %19 = acir.extract %13 {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc69)
    %20 = acir.extract %19 {acir.id = "v36", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc69)
    %21 = arith.cmpi eq, %20, %14 {acir.id = "v37", acir.source_type = "bool"} : i32 loc(#loc69)
    cf.br ^bb7(%21 : i1)
  ^bb7(%22: i1):  // 2 preds: ^bb5, ^bb6
    cf.cond_br %22, ^bb8, ^bb9(%12, %13 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb8:  // pred: ^bb7
    %23 = acir.aggregate(%true, %14, %15) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc70)
    %24 = acir.update %13, %23 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc71)
    cf.br ^bb9(%true, %24 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb9(%25: i1, %26: !acir.struct<"src_payloads_25ebcaa7.Entry">):  // 3 preds: ^bb0, ^bb7, ^bb8
    cf.cond_br %25, ^bb10, ^bb11
  ^bb10:  // pred: ^bb9
    acir.write %0, %26 {acir.id = "v61", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc72)
    cf.br ^bb11
  ^bb11:  // 2 preds: ^bb9, ^bb10
    return
  }
  func.func @scheduler.issue_add(%arg0: i1) attributes {acir.function = {kind = "rule", name = "scheduler.issue_add", outputs = [{kind = "queue", name = "scheduler.add_requests", targets = ["scheduler.add_requests"], type = "src_payloads_25ebcaa7.Entry"}], owner = "scheduler", params = [{consume = false, id = "v1", name = "choose_left", targets = [], type = "bool"}], result = "void", rule = "issue_add"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.get "scheduler.add_requests" {acir.id = "v5", acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc201)
    cf.cond_br %arg0, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %1 = call @scheduler.issue_add.0.call.0.3(%c0_i32) {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:left", instance = "left", interface = "Module:left:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc32)
    %2 = acir.read %0 {acir.id = "v6", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "scheduler.add_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> i1 loc(#loc202)
    %3 = acir.unary %2 "not" {acir.id = "v7", acir.source_type = "bool"} : i1 -> i1 loc(#loc202)
    acir.guard %3 "Queue.push capacity" {acir.id = "v8", acir.source_type = "void", implicit = true} loc(#loc202)
    acir.push %0, %1 {acir.id = "v9", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc202)
    return loc(#loc202)
  ^bb2:  // pred: ^bb0
    %4 = call @scheduler.issue_add.0.call.2.13(%c0_i32) {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:right", instance = "right", interface = "Module:right:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc37)
    %5 = acir.read %0 {acir.id = "v15", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "scheduler.add_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> i1 loc(#loc203)
    %6 = acir.unary %5 "not" {acir.id = "v16", acir.source_type = "bool"} : i1 -> i1 loc(#loc203)
    acir.guard %6 "Queue.push capacity" {acir.id = "v17", acir.source_type = "void", implicit = true} loc(#loc203)
    acir.push %0, %4 {acir.id = "v18", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc203)
    return loc(#loc203)
  }
  func.func @scheduler.issue_mul(%arg0: i1) attributes {acir.function = {kind = "rule", name = "scheduler.issue_mul", outputs = [{kind = "queue", name = "scheduler.mul_requests", targets = ["scheduler.mul_requests"], type = "src_payloads_25ebcaa7.Entry"}], owner = "scheduler", params = [{consume = false, id = "v1", name = "choose_left", targets = [], type = "bool"}], result = "void", rule = "issue_mul"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = acir.get "scheduler.mul_requests" {acir.id = "v5", acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc204)
    cf.cond_br %arg0, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %1 = call @scheduler.issue_mul.0.call.4.3(%c1_i32) {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:left", instance = "left", interface = "Module:left:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc38)
    %2 = acir.read %0 {acir.id = "v6", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "scheduler.mul_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> i1 loc(#loc205)
    %3 = acir.unary %2 "not" {acir.id = "v7", acir.source_type = "bool"} : i1 -> i1 loc(#loc205)
    acir.guard %3 "Queue.push capacity" {acir.id = "v8", acir.source_type = "void", implicit = true} loc(#loc205)
    acir.push %0, %1 {acir.id = "v9", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc205)
    return loc(#loc205)
  ^bb2:  // pred: ^bb0
    %4 = call @scheduler.issue_mul.0.call.6.13(%c1_i32) {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:right", instance = "right", interface = "Module:right:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc39)
    %5 = acir.read %0 {acir.id = "v15", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "scheduler.mul_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> i1 loc(#loc206)
    %6 = acir.unary %5 "not" {acir.id = "v16", acir.source_type = "bool"} : i1 -> i1 loc(#loc206)
    acir.guard %6 "Queue.push capacity" {acir.id = "v17", acir.source_type = "void", implicit = true} loc(#loc206)
    acir.push %0, %4 {acir.id = "v18", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc206)
    return loc(#loc206)
  }
  func.func @scheduler_Work() attributes {acir.function = {kind = "work", name = "scheduler_Work", owner = "scheduler", params = [], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc207)
    %2 = acir.get "right.slot" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %3 = acir.read %2 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc208)
    %4 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc209)
    cf.cond_br %4, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %5 = acir.extract %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc210)
    %6 = acir.extract %5 {acir.id = "v8", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc210)
    cf.br ^bb2(%6 : i1)
  ^bb2(%7: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %7, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %8 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc211)
    %9 = acir.extract %8 {acir.id = "v15", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc211)
    cf.br ^bb4(%9 : i1)
  ^bb4(%10: i1):  // 2 preds: ^bb2, ^bb3
    %11 = acir.extract %3 {acir.id = "v20", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc212)
    cf.cond_br %11, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %12 = acir.extract %3 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc213)
    %13 = acir.extract %12 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc213)
    cf.br ^bb6(%13 : i1)
  ^bb6(%14: i1):  // 2 preds: ^bb4, ^bb5
    cf.cond_br %14, ^bb7, ^bb8(%false : i1)
  ^bb7:  // pred: ^bb6
    %15 = acir.extract %3 {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc214)
    %16 = acir.extract %15 {acir.id = "v30", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1 loc(#loc214)
    cf.br ^bb8(%16 : i1)
  ^bb8(%17: i1):  // 2 preds: ^bb6, ^bb7
    cf.cond_br %10, ^bb9, ^bb10(%false : i1)
  ^bb9:  // pred: ^bb8
    %18 = acir.extract %1 {acir.id = "v36", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc215)
    %19 = arith.cmpi eq, %18, %c0_i32 {acir.id = "v38", acir.source_type = "bool"} : i32 loc(#loc215)
    cf.br ^bb10(%19 : i1)
  ^bb10(%20: i1):  // 2 preds: ^bb8, ^bb9
    cf.cond_br %17, ^bb11, ^bb12(%false : i1)
  ^bb11:  // pred: ^bb10
    %21 = acir.extract %3 {acir.id = "v44", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc216)
    %22 = arith.cmpi eq, %21, %c0_i32 {acir.id = "v46", acir.source_type = "bool"} : i32 loc(#loc216)
    cf.br ^bb12(%22 : i1)
  ^bb12(%23: i1):  // 2 preds: ^bb10, ^bb11
    cf.cond_br %10, ^bb13, ^bb14(%false : i1)
  ^bb13:  // pred: ^bb12
    %24 = acir.extract %1 {acir.id = "v52", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc217)
    %25 = arith.cmpi eq, %24, %c1_i32 {acir.id = "v54", acir.source_type = "bool"} : i32 loc(#loc217)
    cf.br ^bb14(%25 : i1)
  ^bb14(%26: i1):  // 2 preds: ^bb12, ^bb13
    cf.cond_br %17, ^bb15, ^bb16(%false : i1)
  ^bb15:  // pred: ^bb14
    %27 = acir.extract %3 {acir.id = "v60", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc218)
    %28 = arith.cmpi eq, %27, %c1_i32 {acir.id = "v62", acir.source_type = "bool"} : i32 loc(#loc218)
    cf.br ^bb16(%28 : i1)
  ^bb16(%29: i1):  // 2 preds: ^bb14, ^bb15
    cf.cond_br %20, ^bb17, ^bb19(%false : i1)
  ^bb17:  // pred: ^bb16
    %30 = acir.unary %23 "not" {acir.id = "v68", acir.source_type = "bool"} : i1 -> i1 loc(#loc219)
    cf.cond_br %30, ^bb19(%true : i1), ^bb18
  ^bb18:  // pred: ^bb17
    %31 = acir.extract %1 {acir.id = "v71", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc220)
    %32 = acir.extract %3 {acir.id = "v72", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc221)
    %33 = arith.cmpi ult, %31, %32 {acir.id = "v73", acir.source_type = "bool"} : i32 loc(#loc220)
    cf.br ^bb19(%33 : i1)
  ^bb19(%34: i1):  // 3 preds: ^bb16, ^bb17, ^bb18
    cf.cond_br %26, ^bb20, ^bb22(%false : i1)
  ^bb20:  // pred: ^bb19
    %35 = acir.unary %29 "not" {acir.id = "v82", acir.source_type = "bool"} : i1 -> i1 loc(#loc222)
    cf.cond_br %35, ^bb22(%true : i1), ^bb21
  ^bb21:  // pred: ^bb20
    %36 = acir.extract %1 {acir.id = "v85", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc223)
    %37 = acir.extract %3 {acir.id = "v86", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc224)
    %38 = arith.cmpi ult, %36, %37 {acir.id = "v87", acir.source_type = "bool"} : i32 loc(#loc223)
    cf.br ^bb22(%38 : i1)
  ^bb22(%39: i1):  // 3 preds: ^bb19, ^bb20, ^bb21
    acir.invoke "issue_add"(%34) {acir.id = "v95", acir.source_type = "void", callee = "scheduler.issue_add"} : (i1) -> () loc(#loc225)
    acir.invoke "issue_mul"(%39) {acir.id = "v97", acir.source_type = "void", callee = "scheduler.issue_mul"} : (i1) -> () loc(#loc226)
    return
  }
  func.func @adder.execute(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) attributes {acir.function = {kind = "rule", name = "adder.execute", outputs = [{kind = "queue", name = "adder.out", targets = ["adder.out"], type = "src_payloads_25ebcaa7.Completion"}], owner = "adder", params = [{consume = true, id = "v1", name = "message", targets = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], result = "void", rule = "execute"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc227)
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc227)
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc227)
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "scheduler.add_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, i1 loc(#loc227)
    %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc228)
    %2 = acir.extract %0 {acir.id = "v7", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc229)
    %3 = acir.extract %0 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc230)
    %4 = acir.extract %3 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc230)
    %5 = acir.extract %0 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc231)
    %6 = acir.extract %5 {acir.id = "v11", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc231)
    %7 = arith.addi %4, %6 {acir.id = "v12", acir.source_type = "u32"} : i32 loc(#loc230)
    %8 = acir.aggregate(%1, %2, %7) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc232)
    %9 = acir.get "adder.out" {acir.id = "v14", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> loc(#loc233)
    %10 = acir.read %9 {acir.id = "v15", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "adder.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc234)
    %11 = acir.unary %10 "not" {acir.id = "v16", acir.source_type = "bool"} : i1 -> i1 loc(#loc234)
    acir.guard %11 "Queue.push capacity" {acir.id = "v17", acir.source_type = "void", implicit = true} loc(#loc234)
    acir.push %9, %8 {acir.id = "v18", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc234)
    return loc(#loc234)
  }
  func.func @adder_Work() attributes {acir.function = {kind = "work", name = "adder_Work", owner = "adder", params = [], result = "void"}} {
    %0 = acir.get "scheduler.add_requests" {acir.id = "v1", acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc235)
    acir.invoke "execute"(%0) {acir.id = "v2", acir.source_type = "void", callee = "adder.execute"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () loc(#loc236)
    return
  }
  func.func @multiplier.advance() attributes {acir.function = {kind = "rule", name = "multiplier.advance", outputs = [{kind = "queue", name = "multiplier.out", targets = ["multiplier.out"], type = "src_payloads_25ebcaa7.Completion"}], owner = "multiplier", params = [], result = "void", rule = "advance"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc237)
    %0 = acir.get "multiplier.remaining" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc238)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc239)
    %2 = arith.cmpi ne, %1, %c0_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc239)
    %3 = acir.get "multiplier.request" {acir.id = "v12", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc240)
    %4 = acir.get "multiplier.active" {acir.id = "v14", acir.source_type = "cell<bool>"} : !acir.cell<i1> loc(#loc241)
    %5 = acir.get "multiplier.out" {acir.id = "v25", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> loc(#loc242)
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %6 = acir.read %0 {acir.id = "v6", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc243)
    %7 = arith.subi %6, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc243)
    acir.write %0, %7 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc244)
    return loc(#loc245)
  ^bb2:  // pred: ^bb0
    %8 = acir.read %3 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc246)
    acir.write %4, %false {acir.id = "v16", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc247)
    %9 = acir.extract %8 {acir.id = "v17", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc248)
    %10 = acir.extract %8 {acir.id = "v18", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32 loc(#loc249)
    %11 = acir.extract %8 {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc250)
    %12 = acir.extract %11 {acir.id = "v20", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc250)
    %13 = acir.extract %8 {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc251)
    %14 = acir.extract %13 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32 loc(#loc251)
    %15 = arith.muli %12, %14 {acir.id = "v23", acir.source_type = "u32"} : i32 loc(#loc250)
    %16 = acir.aggregate(%9, %10, %15) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc252)
    %17 = acir.read %5 {acir.id = "v26", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "multiplier.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc253)
    %18 = acir.unary %17 "not" {acir.id = "v27", acir.source_type = "bool"} : i1 -> i1 loc(#loc253)
    acir.guard %18 "Queue.push capacity" {acir.id = "v28", acir.source_type = "void", implicit = true} loc(#loc253)
    acir.push %5, %16 {acir.id = "v29", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc253)
    return loc(#loc253)
  }
  func.func @multiplier.accept(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) attributes {acir.function = {kind = "rule", name = "multiplier.accept", outputs = [], owner = "multiplier", params = [{consume = true, id = "v1", name = "message", targets = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], result = "void", rule = "accept"}} {
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc254)
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc255)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc256)
    %0 = acir.get "multiplier.request" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc240)
    %1 = acir.read %arg0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc256)
    acir.pop %arg0 {acir.id = "v4", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc256)
    acir.write %arg0, %false {acir.id = "v6", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "scheduler.mul_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, i1 loc(#loc256)
    acir.write %0, %1 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc257)
    %2 = acir.get "multiplier.remaining" {acir.id = "v8", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc238)
    acir.write %2, %c2_i32 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc258)
    %3 = acir.get "multiplier.active" {acir.id = "v11", acir.source_type = "cell<bool>"} : !acir.cell<i1> loc(#loc241)
    acir.write %3, %true {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc259)
    return
  }
  func.func @multiplier_Work() attributes {acir.function = {kind = "work", name = "multiplier_Work", owner = "multiplier", params = [], result = "void"}} {
    %0 = acir.get "multiplier.active" {acir.id = "v1", acir.source_type = "cell<bool>"} : !acir.cell<i1> loc(#loc241)
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "bool", role = "observe", view = "old"} : !acir.cell<i1> -> i1 loc(#loc260)
    %2 = acir.get "scheduler.mul_requests" {acir.id = "v6", acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc261)
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    acir.invoke "advance"() {acir.id = "v4", acir.source_type = "void", callee = "multiplier.advance"} : () -> () loc(#loc262)
    cf.br ^bb3
  ^bb2:  // pred: ^bb0
    acir.invoke "accept"(%2) {acir.id = "v7", acir.source_type = "void", callee = "multiplier.accept"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> () loc(#loc263)
    cf.br ^bb3
  ^bb3:  // 2 preds: ^bb1, ^bb2
    return
  }
  func.func @cdb.broadcast(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg1: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg2: i1) attributes {acir.function = {kind = "rule", name = "cdb.broadcast", outputs = [{kind = "signal", name = "cdb.event", targets = ["cdb.event"], type = "src_payloads_25ebcaa7.Completion"}], owner = "cdb", params = [{consume = true, id = "v1", name = "a", targets = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {consume = true, id = "v2", name = "b", targets = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {consume = false, id = "v3", name = "choose_add", targets = [], type = "bool"}], result = "void", rule = "broadcast"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "cdb.event" {acir.id = "v28", acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">> loc(#loc264)
    cf.cond_br %arg2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %1 = acir.read %arg0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc265)
    acir.pop %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> loc(#loc265)
    acir.write %arg0, %false {acir.id = "v8", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "adder.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1 loc(#loc265)
    cf.br ^bb3(%1 : !acir.struct<"src_payloads_25ebcaa7.Completion">)
  ^bb2:  // pred: ^bb0
    %2 = acir.read %arg1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc266)
    acir.pop %arg1 {acir.id = "v10", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> loc(#loc266)
    acir.write %arg1, %false {acir.id = "v12", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "multiplier.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1 loc(#loc266)
    cf.br ^bb3(%2 : !acir.struct<"src_payloads_25ebcaa7.Completion">)
  ^bb3(%3: !acir.struct<"src_payloads_25ebcaa7.Completion">):  // 2 preds: ^bb1, ^bb2
    %4 = acir.extract %3 {acir.id = "v19", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc267)
    %5 = acir.extract %3 {acir.id = "v20", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc268)
    call @cdb.broadcast.0.call.8.20(%4, %5) {acir.id = "v21", acir.source_type = "void", contract = "Module:rob", instance = "rob", interface = "Module:rob:complete", method = "complete", role = "method"} : (i32, i32) -> () loc(#loc269)
    %6 = acir.extract %3 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc270)
    %7 = acir.extract %3 {acir.id = "v23", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc271)
    call @cdb.broadcast.0.call.13.23(%6, %7) {acir.id = "v24", acir.source_type = "void", contract = "Module:left", instance = "left", interface = "Module:left:wakeup", method = "wakeup", role = "method"} : (i32, i32) -> () loc(#loc272)
    %8 = acir.extract %3 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc273)
    %9 = acir.extract %3 {acir.id = "v26", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc274)
    call @cdb.broadcast.0.call.14.26(%8, %9) {acir.id = "v27", acir.source_type = "void", contract = "Module:right", instance = "right", interface = "Module:right:wakeup", method = "wakeup", role = "method"} : (i32, i32) -> () loc(#loc275)
    acir.write %0, %3 {acir.id = "v29", acir.source_type = "void", interface = "set", view = "current"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc276)
    return loc(#loc276)
  }
  func.func @cdb_Work() attributes {acir.function = {kind = "work", name = "cdb_Work", owner = "cdb", params = [], result = "void"}} {
    %0 = acir.get "adder.out" {acir.id = "v1", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> loc(#loc277)
    %1 = acir.query %0 "empty" {acir.id = "v2", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc278)
    %2 = acir.unary %1 "not" {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1 loc(#loc279)
    %3 = acir.get "multiplier.out" {acir.id = "v4", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> loc(#loc280)
    acir.invoke "broadcast"(%0, %3, %2) {acir.id = "v5", acir.source_type = "void", callee = "cdb.broadcast"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> () loc(#loc281)
    return
  }
  func.func @dispatch.dispatch(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) attributes {acir.function = {kind = "rule", name = "dispatch.dispatch", outputs = [], owner = "dispatch", params = [{consume = true, id = "v1", name = "message", targets = ["source"], type = "queue<src_payloads_25ebcaa7.MicroOp>"}], result = "void", rule = "dispatch"}} {
    %true = arith.constant {acir.source_type = "bool"} true loc(#loc282)
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc283)
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.MicroOp", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> -> !acir.struct<"src_payloads_25ebcaa7.MicroOp"> loc(#loc283)
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> loc(#loc283)
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "source"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>, i1 loc(#loc283)
    %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc284)
    %2 = call @dispatch.dispatch.0.call.15.6(%1) {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:rf", instance = "rf", interface = "Module:rf:operand", method = "operand", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc285)
    %3 = acir.extract %0 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc286)
    %4 = call @dispatch.dispatch.0.call.21.8(%3) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:rf", instance = "rf", interface = "Module:rf:operand", method = "operand", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> loc(#loc287)
    %5 = acir.extract %0 {acir.id = "v10", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc288)
    %6 = call @dispatch.dispatch.0.call.27.10(%5) {acir.id = "v11", acir.source_type = "u32", contract = "Module:rob", instance = "rob", interface = "Module:rob:allocate", method = "allocate", role = "method"} : (i32) -> i32 loc(#loc135)
    %7 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc289)
    call @dispatch.dispatch.0.call.37.12(%7, %6) {acir.id = "v13", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:rename", method = "rename", role = "method"} : (i32, i32) -> () loc(#loc290)
    %8 = acir.extract %0 {acir.id = "v15", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc291)
    %9 = acir.extract %0 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32 loc(#loc292)
    %10 = acir.aggregate(%true, %8, %6, %9, %2, %4) {acir.id = "v17", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc293)
    %11 = acir.get "left.slot" {acir.id = "v18", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> loc(#loc24)
    %12 = acir.read %11 {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry"> loc(#loc294)
    %13 = acir.extract %12 {acir.id = "v20", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 loc(#loc294)
    %14 = acir.unary %13 "not" {acir.id = "v21", acir.source_type = "bool"} : i1 -> i1 loc(#loc295)
    cf.cond_br %14, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    call @dispatch.dispatch.0.call.38.22(%10) {acir.id = "v23", acir.source_type = "void", contract = "Module:left", instance = "left", interface = "Module:left:place", method = "place", role = "method"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc155)
    cf.br ^bb3
  ^bb2:  // pred: ^bb0
    call @dispatch.dispatch.0.call.40.23(%10) {acir.id = "v24", acir.source_type = "void", contract = "Module:right", instance = "right", interface = "Module:right:place", method = "place", role = "method"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> () loc(#loc158)
    cf.br ^bb3
  ^bb3:  // 2 preds: ^bb1, ^bb2
    return
  }
  func.func @dispatch_Work() attributes {acir.function = {kind = "work", name = "dispatch_Work", owner = "dispatch", params = [], result = "void"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "adder.out" {acir.id = "v1", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> loc(#loc277)
    %1 = acir.query %0 "empty" {acir.id = "v2", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc296)
    %2 = acir.get "multiplier.out" {acir.id = "v4", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> loc(#loc280)
    %3 = acir.get "source" {acir.id = "v11", acir.source_type = "queue<src_payloads_25ebcaa7.MicroOp>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> loc(#loc297)
    cf.cond_br %1, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %4 = acir.query %2 "empty" {acir.id = "v5", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1 loc(#loc298)
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    acir.invoke "dispatch"(%3) {acir.id = "v12", acir.source_type = "void", callee = "dispatch.dispatch"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> () loc(#loc299)
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @commit.commit() attributes {acir.function = {kind = "rule", name = "commit.commit", outputs = [], owner = "commit", params = [], result = "void", rule = "commit"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc300)
    %0 = call @commit.commit.0.call.42.0() {acir.id = "v1", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:retire", method = "retire", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> loc(#loc301)
    call @commit.commit.0.call.51.1(%0) {acir.id = "v2", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:commit", method = "commit", role = "method"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> () loc(#loc302)
    %1 = acir.get "commit.retired" {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc303)
    %2 = acir.read %1 {acir.id = "v4", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32 loc(#loc304)
    %3 = arith.addi %2, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc304)
    acir.write %1, %3 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc305)
    %4 = acir.get "commit.last_tag" {acir.id = "v8", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc306)
    %5 = acir.extract %0 {acir.id = "v9", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32 loc(#loc307)
    acir.write %4, %5 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc308)
    return
  }
  func.func @commit_Work() attributes {acir.function = {kind = "work", name = "commit_Work", owner = "commit", params = [], result = "void"}} {
    acir.invoke "commit"() {acir.id = "v1", acir.source_type = "void", callee = "commit.commit"} : () -> () loc(#loc309)
    return
  }
  func.func @audit.observe(%arg0: !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) attributes {acir.function = {kind = "rule", name = "audit.observe", outputs = [], owner = "audit", params = [{consume = false, id = "v1", name = "value", targets = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>"}], result = "void", rule = "observe"}} {
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Completion", interface = "get", role = "observe", view = "current"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion"> loc(#loc310)
    %1 = acir.get "audit.last_tag" {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc311)
    %2 = acir.extract %0 {acir.id = "v4", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32 loc(#loc312)
    acir.write %1, %2 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc313)
    return
  }
  func.func @audit_Work() attributes {acir.function = {kind = "work", name = "audit_Work", owner = "audit", params = [], result = "void"}} {
    %0 = acir.get "cdb.event" {acir.id = "v1", acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">> loc(#loc314)
    acir.invoke "observe"(%0) {acir.id = "v2", acir.source_type = "void", callee = "audit.observe"} : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> () loc(#loc315)
    return
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:41)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":94:22)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":91:9)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":92:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":93:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":94:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:52)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":47:39)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":88:36)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":89:36)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":156:61)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":156:64)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":156:54)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":157:52)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":158:54)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":8:35)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":55:36)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":56:41)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":57:38)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":39:43)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":61:39)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":62:40)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":77:40)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":8:12)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":18:17)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":19:17)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":19:33)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:21)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":20:41)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":19:9)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:22)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":14:20)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":22:22)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":24:17)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":25:9)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":26:9)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:16)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":20:20)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:16)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":47:13)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":58:17)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":59:12)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":59:32)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":59:28)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":59:47)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":60:13)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":61:13)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":62:13)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:21)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":126:29)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":126:18)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":126:17)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":127:12)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":128:13)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":129:14)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":130:13)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":131:14)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":132:13)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":134:13)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":30:17)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":32:12)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:20)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:16)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":33:40)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:29)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":34:17)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:20)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:16)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":36:40)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:29)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":37:17)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":40:13)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":66:16)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":66:9)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:21)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":115:29)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":115:18)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":115:17)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":116:12)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":117:20)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":117:13)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":118:14)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:20)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":119:13)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":120:14)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:20)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":121:13)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:16)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":122:9)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:24)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:16)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":162:9)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":163:12)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":157:15)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":158:17)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":156:17)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":164:20)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":164:13)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":165:15)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":166:19)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":166:12)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:39)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:20)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":167:13)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":168:17)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:12)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:28)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":169:43)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:39)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:20)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":170:13)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:16)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":171:9)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":96:42)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":89:12)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":96:16)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":88:12)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":96:29)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":96:9)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":50:20)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":50:16)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":50:9)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":54:54)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":54:38)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":54:32)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":103:13)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":52:22)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":54:23)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":54:9)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":105:13)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":107:13)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":109:13)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:21)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":101:29)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:15)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":98:22)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":100:15)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":101:18)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":101:17)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":102:12)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":104:14)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":106:14)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:22)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":110:9)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":111:9)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:18)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:21)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:16)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":175:9)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":176:12)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":177:13)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":11:20)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":11:16)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":11:9)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":24:13)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":13:22)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":15:9)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":26:13)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":69:17)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":70:16)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":70:32)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":70:47)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":70:9)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":141:21)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":72:22)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":74:17)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":75:23)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":75:9)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":76:9)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":143:21)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":145:21)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":147:21)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:21)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":139:29)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":138:15)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":139:18)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":139:17)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":140:12)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":142:14)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":144:14)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:22)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":148:9)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":149:9)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":181:24)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":181:12)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:41)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:29)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":182:13)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":183:41)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":183:29)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":183:13)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":53:5)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":73:5)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":93:42)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":93:16)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":93:29)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":93:9)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":99:5)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":14:5)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/state.py":23:5)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":8:20)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":14:13)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":15:9)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":9:20)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":20:13)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":21:9)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":23:9)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":24:9)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":25:15)
#loc210 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":25:27)
#loc211 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":25:43)
#loc212 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":26:15)
#loc213 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":26:27)
#loc214 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":26:43)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":27:25)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":28:25)
#loc217 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":29:25)
#loc218 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":30:25)
#loc219 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:34)
#loc220 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:47)
#loc221 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":31:55)
#loc222 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:34)
#loc223 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:47)
#loc224 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":32:55)
#loc225 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":35:20)
#loc226 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":36:20)
#loc227 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":46:17)
#loc228 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":47:27)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":47:38)
#loc230 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":47:48)
#loc231 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":47:66)
#loc232 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":47:16)
#loc233 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":42:11)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":47:9)
#loc235 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":37:12)
#loc236 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":49:11)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:31)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":56:17)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":68:12)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":57:15)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":55:14)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":58:11)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":69:31)
#loc244 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":69:13)
#loc245 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":70:13)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":71:17)
#loc247 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":72:9)
#loc248 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:27)
#loc249 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:38)
#loc250 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:48)
#loc251 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:66)
#loc252 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:16)
#loc253 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":73:9)
#loc254 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":64:24)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:27)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:25)
#loc257 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":62:9)
#loc258 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":63:9)
#loc259 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":64:9)
#loc260 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":75:12)
#loc261 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":37:26)
#loc262 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":77:15)
#loc263 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":79:9)
#loc264 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":39:13)
#loc265 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":45:22)
#loc266 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":47:22)
#loc267 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":50:22)
#loc268 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":50:34)
#loc269 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":50:9)
#loc270 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":51:21)
#loc271 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":51:33)
#loc272 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":51:9)
#loc273 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:22)
#loc274 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:34)
#loc275 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":52:9)
#loc276 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":53:9)
#loc277 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":50:12)
#loc278 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":40:22)
#loc279 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":40:18)
#loc280 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/execution.py":80:12)
#loc281 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":55:13)
#loc282 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:23)
#loc283 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":17:16)
#loc284 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":18:26)
#loc285 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":18:15)
#loc286 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:26)
#loc287 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":19:15)
#loc288 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":20:28)
#loc289 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:19)
#loc290 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":21:9)
#loc291 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:29)
#loc292 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:45)
#loc293 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":22:17)
#loc294 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:16)
#loc295 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":23:12)
#loc296 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:8)
#loc297 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":90:14)
#loc298 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":32:29)
#loc299 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":33:9)
#loc300 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":68:41)
#loc301 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":66:17)
#loc302 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":67:9)
#loc303 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":61:15)
#loc304 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":68:25)
#loc305 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":68:9)
#loc306 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":62:16)
#loc307 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":69:26)
#loc308 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":69:9)
#loc309 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":71:5)
#loc310 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":81:17)
#loc311 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":77:16)
#loc312 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":82:26)
#loc313 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":82:9)
#loc314 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":56:12)
#loc315 = loc("/home/lc/tmp/pycircuit/examples/ooo_scheduler/model.py":84:5)

