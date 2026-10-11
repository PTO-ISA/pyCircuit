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
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    return %c4_i32 : i32
  }
  func.func @ac_init_source() -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> attributes {acir.function = {kind = "helper", name = "ac_init_source", params = [], result = "vector<src_payloads_25ebcaa7.MicroOp>"}} {
    %c6_i32 = arith.constant {acir.source_type = "u32"} 6 : i32
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %0 = acir.aggregate(%c1_i32, %c1_i32, %c2_i32, %c3_i32) {acir.id = "v6", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %1 = acir.aggregate(%c0_i32, %c4_i32, %c1_i32, %c2_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %2 = acir.aggregate(%c0_i32, %c5_i32, %c2_i32, %c3_i32) {acir.id = "v16", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %3 = acir.aggregate(%c0_i32, %c6_i32, %c4_i32, %c5_i32) {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.MicroOp"} : (i32, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    %4 = acir.aggregate(%0, %1, %2, %3) {acir.id = "v22", acir.source_type = "vector<src_payloads_25ebcaa7.MicroOp>"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">, !acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    return %4 : !acir.vector<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
  }
  func.func @ac_init_rob.slot0.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot0.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.slot1.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot1.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v36", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.slot2.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot2.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v43", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.slot3.state() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {kind = "helper", name = "ac_init_rob.slot3.state", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v50", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %0 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_init_rob.head() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_rob.head", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_rob.tail() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_rob.tail", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_rf.registers() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_rf.registers", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c6_i32 = arith.constant {acir.source_type = "u32"} 6 : i32
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c6_i32, %c7_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v64", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8>
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_rf.renamed() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_rf.renamed", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v74", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8>
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_rf.committed() -> !acir.array<i32, 8> attributes {acir.function = {kind = "helper", name = "ac_init_rf.committed", params = [], result = "array<u32,8>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v84", acir.source_type = "array<u32,8>"} : (i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 8>
    return %0 : !acir.array<i32, 8>
  }
  func.func @ac_init_left.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_left.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v90", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1 = acir.aggregate() {acir.id = "v91", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v92", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_init_right.slot() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_right.slot", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v98", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1 = acir.aggregate() {acir.id = "v99", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v100", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
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
    %false = arith.constant {acir.source_type = "bool"} false
    return %false : i1
  }
  func.func @ac_init_multiplier.remaining() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.remaining", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_multiplier.request() -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {kind = "helper", name = "ac_init_multiplier.request", params = [], result = "src_payloads_25ebcaa7.Entry"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate() {acir.id = "v119", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %1 = acir.aggregate() {acir.id = "v120", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %2 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %0, %1) {acir.id = "v121", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
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
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32) {acir.id = "v129", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    return %0 : !acir.struct<"src_payloads_25ebcaa7.Completion">
  }
  func.func @ac_init_commit.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_commit.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_commit.last_tag() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_commit.last_tag", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @ac_init_audit.last_tag() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_audit.last_tag", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    return %c0_i32 : i32
  }
  func.func @scheduler.issue_add.0.call.0.3.0.call.0.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "left", kind = "guard", name = "scheduler.issue_add.0.call.0.3.0.call.0.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1
  }
  func.func @scheduler.issue_add.0.call.0.3(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:left", definition = "take", instance = "left", interface = "Module:left:take", kind = "method", method = "take", name = "scheduler.issue_add.0.call.0.3", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @scheduler.issue_add.0.call.0.3.0.call.0.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (i32) -> i1
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "left.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @scheduler.issue_add.0.call.2.13.0.call.2.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "right", kind = "guard", name = "scheduler.issue_add.0.call.2.13.0.call.2.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "right.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1
  }
  func.func @scheduler.issue_add.0.call.2.13(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:right", definition = "take", instance = "right", interface = "Module:right:take", kind = "method", method = "take", name = "scheduler.issue_add.0.call.2.13", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @scheduler.issue_add.0.call.2.13.0.call.2.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (i32) -> i1
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "right.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @scheduler.issue_mul.0.call.4.3.0.call.4.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "left", kind = "guard", name = "scheduler.issue_mul.0.call.4.3.0.call.4.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1
  }
  func.func @scheduler.issue_mul.0.call.4.3(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:left", definition = "take", instance = "left", interface = "Module:left:take", kind = "method", method = "take", name = "scheduler.issue_mul.0.call.4.3", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @scheduler.issue_mul.0.call.4.3.0.call.4.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (i32) -> i1
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "left.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @scheduler.issue_mul.0.call.6.13.0.call.6.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "right", kind = "guard", name = "scheduler.issue_mul.0.call.6.13.0.call.6.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "right.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1
  }
  func.func @scheduler.issue_mul.0.call.6.13(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:right", definition = "take", instance = "right", interface = "Module:right:take", kind = "method", method = "take", name = "scheduler.issue_mul.0.call.6.13", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @scheduler.issue_mul.0.call.6.13.0.call.6.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (i32) -> i1
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "right.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @cdb.broadcast.0.call.8.20.0.call.8.9(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "complete", instance = "rob.slot0", interface = "Module:rob.slot0:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.20.0.call.8.9", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @cdb.broadcast.0.call.8.20.0.call.9.13(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "complete", instance = "rob.slot1", interface = "Module:rob.slot1:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.20.0.call.9.13", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @cdb.broadcast.0.call.8.20.0.call.10.17(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "complete", instance = "rob.slot2", interface = "Module:rob.slot2:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.20.0.call.10.17", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @cdb.broadcast.0.call.8.20.0.call.11.18(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "complete", instance = "rob.slot3", interface = "Module:rob.slot3:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.20.0.call.11.18", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @cdb.broadcast.0.call.8.20(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob", definition = "complete", instance = "rob", interface = "Module:rob:complete", kind = "method", method = "complete", name = "cdb.broadcast.0.call.8.20", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32
    %1 = arith.andi %0, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    call @cdb.broadcast.0.call.8.20.0.call.8.9(%arg0, %arg1) {acir.id = "v10", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:complete", method = "complete", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7
  ^bb2:  // pred: ^bb0
    %3 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32
    cf.cond_br %3, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    call @cdb.broadcast.0.call.8.20.0.call.9.13(%arg0, %arg1) {acir.id = "v14", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:complete", method = "complete", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7
  ^bb4:  // pred: ^bb2
    %4 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v16", acir.source_type = "bool"} : i32
    cf.cond_br %4, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    call @cdb.broadcast.0.call.8.20.0.call.10.17(%arg0, %arg1) {acir.id = "v18", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:complete", method = "complete", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7
  ^bb6:  // pred: ^bb4
    call @cdb.broadcast.0.call.8.20.0.call.11.18(%arg0, %arg1) {acir.id = "v19", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:complete", method = "complete", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7
  ^bb7:  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    return
  }
  func.func @cdb.broadcast.0.call.13.23(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:left", definition = "wakeup", instance = "left", interface = "Module:left:wakeup", kind = "method", method = "wakeup", name = "cdb.broadcast.0.call.13.23", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %2, ^bb1, ^bb9(%false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.extract %3 {acir.id = "v9", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    %5 = acir.unary %4 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %5, ^bb2, ^bb3(%false : i1)
  ^bb2:  // pred: ^bb1
    %6 = acir.extract %1 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %7 = acir.extract %6 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v14", acir.source_type = "bool"} : i32
    cf.br ^bb3(%8 : i1)
  ^bb3(%9: i1):  // 2 preds: ^bb1, ^bb2
    cf.cond_br %9, ^bb4, ^bb5(%false, %1, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb4:  // pred: ^bb3
    %10 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %11 = acir.update %1, %10 {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb5(%true, %11, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb5(%12: i1, %13: !acir.struct<"src_payloads_25ebcaa7.Entry">, %14: i32, %15: i32):  // 2 preds: ^bb3, ^bb4
    %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %17 = acir.extract %16 {acir.id = "v32", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    %18 = acir.unary %17 "not" {acir.id = "v33", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %18, ^bb6, ^bb7(%false : i1)
  ^bb6:  // pred: ^bb5
    %19 = acir.extract %13 {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %20 = acir.extract %19 {acir.id = "v36", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %21 = arith.cmpi eq, %20, %14 {acir.id = "v37", acir.source_type = "bool"} : i32
    cf.br ^bb7(%21 : i1)
  ^bb7(%22: i1):  // 2 preds: ^bb5, ^bb6
    cf.cond_br %22, ^bb8, ^bb9(%12, %13 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb8:  // pred: ^bb7
    %23 = acir.aggregate(%true, %14, %15) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %24 = acir.update %13, %23 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb9(%true, %24 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb9(%25: i1, %26: !acir.struct<"src_payloads_25ebcaa7.Entry">):  // 3 preds: ^bb0, ^bb7, ^bb8
    cf.cond_br %25, ^bb10, ^bb11
  ^bb10:  // pred: ^bb9
    acir.write %0, %26 {acir.id = "v61", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb11
  ^bb11:  // 2 preds: ^bb9, ^bb10
    return
  }
  func.func @cdb.broadcast.0.call.14.26(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:right", definition = "wakeup", instance = "right", interface = "Module:right:wakeup", kind = "method", method = "wakeup", name = "cdb.broadcast.0.call.14.26", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "right.slot" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %2, ^bb1, ^bb9(%false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.extract %3 {acir.id = "v9", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    %5 = acir.unary %4 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %5, ^bb2, ^bb3(%false : i1)
  ^bb2:  // pred: ^bb1
    %6 = acir.extract %1 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %7 = acir.extract %6 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v14", acir.source_type = "bool"} : i32
    cf.br ^bb3(%8 : i1)
  ^bb3(%9: i1):  // 2 preds: ^bb1, ^bb2
    cf.cond_br %9, ^bb4, ^bb5(%false, %1, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb4:  // pred: ^bb3
    %10 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %11 = acir.update %1, %10 {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb5(%true, %11, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb5(%12: i1, %13: !acir.struct<"src_payloads_25ebcaa7.Entry">, %14: i32, %15: i32):  // 2 preds: ^bb3, ^bb4
    %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %17 = acir.extract %16 {acir.id = "v32", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    %18 = acir.unary %17 "not" {acir.id = "v33", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %18, ^bb6, ^bb7(%false : i1)
  ^bb6:  // pred: ^bb5
    %19 = acir.extract %13 {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %20 = acir.extract %19 {acir.id = "v36", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %21 = arith.cmpi eq, %20, %14 {acir.id = "v37", acir.source_type = "bool"} : i32
    cf.br ^bb7(%21 : i1)
  ^bb7(%22: i1):  // 2 preds: ^bb5, ^bb6
    cf.cond_br %22, ^bb8, ^bb9(%12, %13 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb8:  // pred: ^bb7
    %23 = acir.aggregate(%true, %14, %15) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %24 = acir.update %13, %23 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb9(%true, %24 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb9(%25: i1, %26: !acir.struct<"src_payloads_25ebcaa7.Entry">):  // 3 preds: ^bb0, ^bb7, ^bb8
    cf.cond_br %25, ^bb10, ^bb11
  ^bb10:  // pred: ^bb9
    acir.write %0, %26 {acir.id = "v61", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb11
  ^bb11:  // 2 preds: ^bb9, ^bb10
    return
  }
  func.func @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.15.8() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.15.8", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.16.13() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.16.13", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.17.18() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.17.18", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.18.29() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.18.29", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @dispatch.dispatch.0.call.15.6.0.call.15.31(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "dispatch.dispatch.0.call.15.6.0.call.15.31", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v3", acir.source_type = "u32"} : i32
    %1 = arith.andi %0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = call @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.15.8() {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb2:  // pred: ^bb0
    %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = call @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.16.13() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb4:  // pred: ^bb2
    %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32
    cf.cond_br %6, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %7 = call @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.17.18() {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb6:  // pred: ^bb4
    %8 = call @dispatch.dispatch.0.call.15.6.0.call.15.31.0.call.18.29() {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @dispatch.dispatch.0.call.15.6(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> attributes {acir.function = {contract = "Module:rf", definition = "operand", instance = "rf", interface = "Module:rf:operand", kind = "method", method = "operand", name = "dispatch.dispatch.0.call.15.6", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Operand"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
    %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v3", acir.source_type = "bool"} : i32
    acir.check %0 {acir.id = "v4", acir.source_type = "void"}
    %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    %2 = acir.get "rf.renamed" {acir.id = "v15", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    %3 = acir.get "rf.committed" {acir.id = "v18", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    %4 = acir.get "rf.registers" {acir.id = "v24", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    %5 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %5 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  ^bb2(%6: i32):  // pred: ^bb0
    %7 = acir.read %2 {acir.id = "v16", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %8 = acir.extract %7[%6] {acir.id = "v17", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
    %9 = acir.read %3 {acir.id = "v19", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %10 = acir.extract %9[%6] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
    %11 = arith.cmpi eq, %8, %10 {acir.id = "v21", acir.source_type = "bool"} : i32
    cf.cond_br %11, ^bb3, ^bb4(%8 : i32)
  ^bb3:  // pred: ^bb2
    %12 = acir.read %4 {acir.id = "v25", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %13 = acir.extract %12[%6] {acir.id = "v26", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
    %14 = acir.aggregate(%true, %8, %13) {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  ^bb4(%15: i32):  // pred: ^bb2
    %16 = call @dispatch.dispatch.0.call.15.6.0.call.15.31(%15) {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:lookup", method = "lookup", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %17 = acir.extract %16 {acir.id = "v33", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %17, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %18 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb6(%18 : i1)
  ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
    cf.cond_br %19, ^bb7, ^bb8(%false : i1)
  ^bb7:  // pred: ^bb6
    %20 = acir.extract %16 {acir.id = "v41", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %21 = arith.cmpi eq, %20, %15 {acir.id = "v42", acir.source_type = "bool"} : i32
    cf.br ^bb8(%21 : i1)
  ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
    cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
  ^bb9:  // pred: ^bb8
    %23 = acir.extract %16 {acir.id = "v49", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %24 = acir.aggregate(%true, %15, %23) {acir.id = "v50", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  ^bb10(%25: i32):  // pred: ^bb8
    %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v58", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  }
  func.func @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.21.8() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.21.8", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.22.13() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.22.13", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.23.18() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.23.18", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.24.29() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.24.29", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @dispatch.dispatch.0.call.21.8.0.call.21.31(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "dispatch.dispatch.0.call.21.8.0.call.21.31", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v3", acir.source_type = "u32"} : i32
    %1 = arith.andi %0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = call @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.21.8() {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb2:  // pred: ^bb0
    %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = call @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.22.13() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb4:  // pred: ^bb2
    %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32
    cf.cond_br %6, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %7 = call @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.23.18() {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb6:  // pred: ^bb4
    %8 = call @dispatch.dispatch.0.call.21.8.0.call.21.31.0.call.24.29() {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @dispatch.dispatch.0.call.21.8(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> attributes {acir.function = {contract = "Module:rf", definition = "operand", instance = "rf", interface = "Module:rf:operand", kind = "method", method = "operand", name = "dispatch.dispatch.0.call.21.8", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Operand"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
    %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v3", acir.source_type = "bool"} : i32
    acir.check %0 {acir.id = "v4", acir.source_type = "void"}
    %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    %2 = acir.get "rf.renamed" {acir.id = "v15", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    %3 = acir.get "rf.committed" {acir.id = "v18", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    %4 = acir.get "rf.registers" {acir.id = "v24", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    %5 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %5 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  ^bb2(%6: i32):  // pred: ^bb0
    %7 = acir.read %2 {acir.id = "v16", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %8 = acir.extract %7[%6] {acir.id = "v17", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
    %9 = acir.read %3 {acir.id = "v19", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %10 = acir.extract %9[%6] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
    %11 = arith.cmpi eq, %8, %10 {acir.id = "v21", acir.source_type = "bool"} : i32
    cf.cond_br %11, ^bb3, ^bb4(%8 : i32)
  ^bb3:  // pred: ^bb2
    %12 = acir.read %4 {acir.id = "v25", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %13 = acir.extract %12[%6] {acir.id = "v26", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
    %14 = acir.aggregate(%true, %8, %13) {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  ^bb4(%15: i32):  // pred: ^bb2
    %16 = call @dispatch.dispatch.0.call.21.8.0.call.21.31(%15) {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:lookup", method = "lookup", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %17 = acir.extract %16 {acir.id = "v33", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %17, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %18 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb6(%18 : i1)
  ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
    cf.cond_br %19, ^bb7, ^bb8(%false : i1)
  ^bb7:  // pred: ^bb6
    %20 = acir.extract %16 {acir.id = "v41", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %21 = arith.cmpi eq, %20, %15 {acir.id = "v42", acir.source_type = "bool"} : i32
    cf.br ^bb8(%21 : i1)
  ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
    cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
  ^bb9:  // pred: ^bb8
    %23 = acir.extract %16 {acir.id = "v49", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %24 = acir.aggregate(%true, %15, %23) {acir.id = "v50", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  ^bb10(%25: i32):  // pred: ^bb8
    %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v58", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.27.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "room", instance = "rob", kind = "guard", name = "dispatch.dispatch.0.call.27.10.0.call.27.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    %0 = acir.get "rob.tail" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %2 = acir.get "rob.head" {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %3 = acir.read %2 {acir.id = "v5", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %4 = arith.subi %1, %3 {acir.id = "v6", acir.source_type = "u32"} : i32
    %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
    return %5 : i1
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.28.12.0.call.28.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot0", kind = "guard", name = "dispatch.dispatch.0.call.27.10.0.call.28.12.0.call.28.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.28.12(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "allocate", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.10.0.call.28.12", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @dispatch.dispatch.0.call.27.10.0.call.28.12.0.call.28.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot0.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.30.16.0.call.30.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot1", kind = "guard", name = "dispatch.dispatch.0.call.27.10.0.call.30.16.0.call.30.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.30.16(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "allocate", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.10.0.call.30.16", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @dispatch.dispatch.0.call.27.10.0.call.30.16.0.call.30.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot1.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.32.20.0.call.32.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot2", kind = "guard", name = "dispatch.dispatch.0.call.27.10.0.call.32.20.0.call.32.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.32.20(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "allocate", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.10.0.call.32.20", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @dispatch.dispatch.0.call.27.10.0.call.32.20.0.call.32.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot2.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.34.21.0.call.34.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot3", kind = "guard", name = "dispatch.dispatch.0.call.27.10.0.call.34.21.0.call.34.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @dispatch.dispatch.0.call.27.10.0.call.34.21(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "allocate", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.10.0.call.34.21", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @dispatch.dispatch.0.call.27.10.0.call.34.21.0.call.34.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot3.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @dispatch.dispatch.0.call.27.10(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:rob", definition = "allocate", instance = "rob", interface = "Module:rob:allocate", kind = "method", method = "allocate", name = "dispatch.dispatch.0.call.27.10", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "u32"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = call @dispatch.dispatch.0.call.27.10.0.call.27.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob", role = "guard"} : (i32) -> i1
    acir.guard %0 "room" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.tail" {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %3 = arith.subi %2, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32
    %4 = arith.andi %3, %c3_i32 {acir.id = "v9", acir.source_type = "u32"} : i32
    %5 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32
    cf.cond_br %5, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    call @dispatch.dispatch.0.call.27.10.0.call.28.12(%2, %arg0) {acir.id = "v13", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7(%2 : i32)
  ^bb2:  // pred: ^bb0
    %6 = arith.cmpi eq, %4, %c1_i32 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.cond_br %6, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    call @dispatch.dispatch.0.call.27.10.0.call.30.16(%2, %arg0) {acir.id = "v17", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7(%2 : i32)
  ^bb4:  // pred: ^bb2
    %7 = arith.cmpi eq, %4, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32
    cf.cond_br %7, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    call @dispatch.dispatch.0.call.27.10.0.call.32.20(%2, %arg0) {acir.id = "v21", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7(%2 : i32)
  ^bb6:  // pred: ^bb4
    call @dispatch.dispatch.0.call.27.10.0.call.34.21(%2, %arg0) {acir.id = "v22", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7(%2 : i32)
  ^bb7(%8: i32):  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    %9 = arith.addi %8, %c1_i32 {acir.id = "v39", acir.source_type = "u32"} : i32
    acir.write %1, %9 {acir.id = "v40", acir.source_type = "void"} : !acir.cell<i32>, i32
    return %8 : i32
  }
  func.func @dispatch.dispatch.0.call.37.12(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rf", definition = "rename", instance = "rf", interface = "Module:rf:rename", kind = "method", method = "rename", name = "dispatch.dispatch.0.call.37.12", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}, {consume = false, id = "v2", name = "tag", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
    %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v4", acir.source_type = "bool"} : i32
    acir.check %0 {acir.id = "v5", acir.source_type = "void"}
    %1 = arith.cmpi ne, %arg0, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32
    %2 = acir.get "rf.renamed" {acir.id = "v9", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = acir.read %2 {acir.id = "v10", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %4 = acir.update %3, %arg1[%arg0] {acir.id = "v11", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
    acir.write %2, %4 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @dispatch.dispatch.0.call.38.22.0.call.38.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "left", kind = "guard", name = "dispatch.dispatch.0.call.38.22.0.call.38.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.get "left.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @dispatch.dispatch.0.call.38.22(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) attributes {acir.function = {contract = "Module:left", definition = "place", instance = "left", interface = "Module:left:place", kind = "method", method = "place", name = "dispatch.dispatch.0.call.38.22", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void"}} {
    %0 = call @dispatch.dispatch.0.call.38.22.0.call.38.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    acir.guard %0 "free" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "left.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    acir.write %1, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return
  }
  func.func @dispatch.dispatch.0.call.40.23.0.call.40.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "right", kind = "guard", name = "dispatch.dispatch.0.call.40.23.0.call.40.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.get "right.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @dispatch.dispatch.0.call.40.23(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) attributes {acir.function = {contract = "Module:right", definition = "place", instance = "right", interface = "Module:right:place", kind = "method", method = "place", name = "dispatch.dispatch.0.call.40.23", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void"}} {
    %0 = call @dispatch.dispatch.0.call.40.23.0.call.40.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    acir.guard %0 "free" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "right.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    acir.write %1, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return
  }
  func.func @commit.commit.0.call.42.0.0.call.42.9.0.call.42.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot0", kind = "guard", name = "commit.commit.0.call.42.0.0.call.42.9.0.call.42.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @commit.commit.0.call.42.0.0.call.42.9(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "retire", instance = "rob.slot0", interface = "Module:rob.slot0:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.0.0.call.42.9", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @commit.commit.0.call.42.0.0.call.42.9.0.call.42.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot0.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @commit.commit.0.call.42.0.0.call.44.13.0.call.44.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot1", kind = "guard", name = "commit.commit.0.call.42.0.0.call.44.13.0.call.44.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @commit.commit.0.call.42.0.0.call.44.13(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "retire", instance = "rob.slot1", interface = "Module:rob.slot1:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.0.0.call.44.13", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @commit.commit.0.call.42.0.0.call.44.13.0.call.44.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot1.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @commit.commit.0.call.42.0.0.call.46.17.0.call.46.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot2", kind = "guard", name = "commit.commit.0.call.42.0.0.call.46.17.0.call.46.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @commit.commit.0.call.42.0.0.call.46.17(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "retire", instance = "rob.slot2", interface = "Module:rob.slot2:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.0.0.call.46.17", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @commit.commit.0.call.42.0.0.call.46.17.0.call.46.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot2.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @commit.commit.0.call.42.0.0.call.48.18.0.call.48.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot3", kind = "guard", name = "commit.commit.0.call.42.0.0.call.48.18.0.call.48.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @commit.commit.0.call.42.0.0.call.48.18(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "retire", instance = "rob.slot3", interface = "Module:rob.slot3:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.0.0.call.48.18", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @commit.commit.0.call.42.0.0.call.48.18.0.call.48.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot3.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @commit.commit.0.call.42.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "retire", instance = "rob", interface = "Module:rob:retire", kind = "method", method = "retire", name = "commit.commit.0.call.42.0", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = acir.get "rob.head" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %2 = arith.subi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32
    %3 = arith.andi %2, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
    %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
    cf.cond_br %4, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %5 = call @commit.commit.0.call.42.0.0.call.42.9(%1) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb7(%5, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb2:  // pred: ^bb0
    %6 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32
    cf.cond_br %6, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %7 = call @commit.commit.0.call.42.0.0.call.44.13(%1) {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb7(%7, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb4:  // pred: ^bb2
    %8 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v16", acir.source_type = "bool"} : i32
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = call @commit.commit.0.call.42.0.0.call.46.17(%1) {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb7(%9, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb6:  // pred: ^bb4
    %10 = call @commit.commit.0.call.42.0.0.call.48.18(%1) {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb7(%10, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb7(%11: !acir.struct<"src_payloads_25ebcaa7.RobEntry">, %12: i32):  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    %13 = arith.addi %12, %c1_i32 {acir.id = "v36", acir.source_type = "u32"} : i32
    acir.write %0, %13 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<i32>, i32
    return %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @commit.commit.0.call.51.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.RobEntry">) attributes {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "commit.commit.0.call.51.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.RobEntry"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v4", acir.source_type = "bool"} : i32
    %2 = acir.get "rf.registers" {acir.id = "v6", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    %3 = acir.get "rf.committed" {acir.id = "v12", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %4 = acir.extract %arg0 {acir.id = "v7", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %5 = acir.extract %arg0 {acir.id = "v8", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = acir.read %2 {acir.id = "v9", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %7 = acir.update %6, %4[%5] {acir.id = "v10", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
    acir.write %2, %7 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
    %8 = acir.extract %arg0 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %9 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %10 = acir.read %3 {acir.id = "v15", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %11 = acir.update %10, %8[%9] {acir.id = "v16", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
    acir.write %3, %11 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @ac_summary_rob.slot0.allocate.0.call.52.2.0.call.52.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.slot0.allocate.0.call.52.2.0.call.52.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @ac_summary_rob.slot0.allocate.0.call.52.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "allocate", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot0.allocate.0.call.52.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @ac_summary_rob.slot0.allocate.0.call.52.2.0.call.52.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot0.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @ac_summary_rob.slot0.complete.0.call.54.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "complete", instance = "rob.slot0", interface = "Module:rob.slot0:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot0.complete.0.call.54.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.slot0.peek.0.call.55.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot0.peek.0.call.55.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.slot0.retire.0.call.56.1.0.call.56.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.slot0.retire.0.call.56.1.0.call.56.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @ac_summary_rob.slot0.retire.0.call.56.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "retire", instance = "rob.slot0", interface = "Module:rob.slot0:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot0.retire.0.call.56.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.slot0.retire.0.call.56.1.0.call.56.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot0.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.slot1.allocate.0.call.58.2.0.call.58.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.slot1.allocate.0.call.58.2.0.call.58.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @ac_summary_rob.slot1.allocate.0.call.58.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "allocate", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot1.allocate.0.call.58.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @ac_summary_rob.slot1.allocate.0.call.58.2.0.call.58.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot1.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @ac_summary_rob.slot1.complete.0.call.60.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "complete", instance = "rob.slot1", interface = "Module:rob.slot1:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot1.complete.0.call.60.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.slot1.peek.0.call.61.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot1.peek.0.call.61.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.slot1.retire.0.call.62.1.0.call.62.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.slot1.retire.0.call.62.1.0.call.62.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @ac_summary_rob.slot1.retire.0.call.62.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "retire", instance = "rob.slot1", interface = "Module:rob.slot1:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot1.retire.0.call.62.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.slot1.retire.0.call.62.1.0.call.62.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot1.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.slot2.allocate.0.call.64.2.0.call.64.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.slot2.allocate.0.call.64.2.0.call.64.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @ac_summary_rob.slot2.allocate.0.call.64.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "allocate", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot2.allocate.0.call.64.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @ac_summary_rob.slot2.allocate.0.call.64.2.0.call.64.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot2.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @ac_summary_rob.slot2.complete.0.call.66.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "complete", instance = "rob.slot2", interface = "Module:rob.slot2:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot2.complete.0.call.66.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.slot2.peek.0.call.67.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot2.peek.0.call.67.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.slot2.retire.0.call.68.1.0.call.68.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.slot2.retire.0.call.68.1.0.call.68.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @ac_summary_rob.slot2.retire.0.call.68.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "retire", instance = "rob.slot2", interface = "Module:rob.slot2:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot2.retire.0.call.68.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.slot2.retire.0.call.68.1.0.call.68.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot2.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.slot3.allocate.0.call.70.2.0.call.70.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.slot3.allocate.0.call.70.2.0.call.70.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @ac_summary_rob.slot3.allocate.0.call.70.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "allocate", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.slot3.allocate.0.call.70.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @ac_summary_rob.slot3.allocate.0.call.70.2.0.call.70.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot3.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @ac_summary_rob.slot3.complete.0.call.72.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "complete", instance = "rob.slot3", interface = "Module:rob.slot3:complete", kind = "method", method = "complete", name = "ac_summary_rob.slot3.complete.0.call.72.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.slot3.peek.0.call.73.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "ac_summary_rob.slot3.peek.0.call.73.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.slot3.retire.0.call.74.1.0.call.74.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.slot3.retire.0.call.74.1.0.call.74.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @ac_summary_rob.slot3.retire.0.call.74.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "retire", instance = "rob.slot3", interface = "Module:rob.slot3:retire", kind = "method", method = "retire", name = "ac_summary_rob.slot3.retire.0.call.74.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.slot3.retire.0.call.74.1.0.call.74.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot3.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.has_room.0.call.76.0() -> i1 attributes {acir.function = {contract = "Module:rob", definition = "has_room", instance = "rob", interface = "Module:rob:has_room", kind = "method", method = "has_room", name = "ac_summary_rob.has_room.0.call.76.0", params = [], result = "bool", summary = true}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    %0 = acir.get "rob.tail" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %2 = acir.get "rob.head" {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %3 = acir.read %2 {acir.id = "v4", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %4 = arith.subi %1, %3 {acir.id = "v5", acir.source_type = "u32"} : i32
    %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v7", acir.source_type = "bool"} : i32
    return %5 : i1
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.77.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "room", instance = "rob", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.77.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
    %0 = acir.get "rob.tail" {acir.id = "v2", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %2 = acir.get "rob.head" {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %3 = acir.read %2 {acir.id = "v5", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %4 = arith.subi %1, %3 {acir.id = "v6", acir.source_type = "u32"} : i32
    %5 = arith.cmpi ult, %4, %c4_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
    return %5 : i1
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.78.12.0.call.78.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.78.12.0.call.78.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.78.12(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "allocate", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.78.12", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @ac_summary_rob.allocate.0.call.77.1.0.call.78.12.0.call.78.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot0.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.80.16.0.call.80.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.80.16.0.call.80.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.80.16(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "allocate", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.80.16", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @ac_summary_rob.allocate.0.call.77.1.0.call.80.16.0.call.80.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot1.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.82.20.0.call.82.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.82.20.0.call.82.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.82.20(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "allocate", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.82.20", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @ac_summary_rob.allocate.0.call.77.1.0.call.82.20.0.call.82.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot2.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.84.21.0.call.84.2(%arg0: i32, %arg1: i32) -> i1 attributes {acir.function = {definition = "free", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.allocate.0.call.77.1.0.call.84.21.0.call.84.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "bool"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v6", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @ac_summary_rob.allocate.0.call.77.1.0.call.84.21(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "allocate", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1.0.call.84.21", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "rd", targets = [], type = "u32"}], result = "void"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = call @ac_summary_rob.allocate.0.call.77.1.0.call.84.21.0.call.84.2(%arg0, %arg1) {acir.id = "v3", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32, i32) -> i1
    acir.guard %0 "free" {acir.id = "v4", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot3.state" {acir.id = "v5", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.aggregate(%true, %false, %arg0, %arg1, %c0_i32) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %2 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return
  }
  func.func @ac_summary_rob.allocate.0.call.77.1(%arg0: i32) -> i32 attributes {acir.function = {contract = "Module:rob", definition = "allocate", instance = "rob", interface = "Module:rob:allocate", kind = "method", method = "allocate", name = "ac_summary_rob.allocate.0.call.77.1", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}], result = "u32", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = call @ac_summary_rob.allocate.0.call.77.1.0.call.77.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob", role = "guard"} : (i32) -> i1
    acir.guard %0 "room" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.tail" {acir.id = "v4", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %3 = arith.subi %2, %c1_i32 {acir.id = "v7", acir.source_type = "u32"} : i32
    %4 = arith.andi %3, %c3_i32 {acir.id = "v9", acir.source_type = "u32"} : i32
    %5 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v11", acir.source_type = "bool"} : i32
    cf.cond_br %5, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    call @ac_summary_rob.allocate.0.call.77.1.0.call.78.12(%2, %arg0) {acir.id = "v13", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7(%2 : i32)
  ^bb2:  // pred: ^bb0
    %6 = arith.cmpi eq, %4, %c1_i32 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.cond_br %6, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    call @ac_summary_rob.allocate.0.call.77.1.0.call.80.16(%2, %arg0) {acir.id = "v17", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7(%2 : i32)
  ^bb4:  // pred: ^bb2
    %7 = arith.cmpi eq, %4, %c2_i32 {acir.id = "v19", acir.source_type = "bool"} : i32
    cf.cond_br %7, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    call @ac_summary_rob.allocate.0.call.77.1.0.call.82.20(%2, %arg0) {acir.id = "v21", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7(%2 : i32)
  ^bb6:  // pred: ^bb4
    call @ac_summary_rob.allocate.0.call.77.1.0.call.84.21(%2, %arg0) {acir.id = "v22", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:allocate", method = "allocate", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7(%2 : i32)
  ^bb7(%8: i32):  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    %9 = arith.addi %8, %c1_i32 {acir.id = "v39", acir.source_type = "u32"} : i32
    acir.write %1, %9 {acir.id = "v40", acir.source_type = "void"} : !acir.cell<i32>, i32
    return %8 : i32
  }
  func.func @ac_summary_rob.lookup.0.call.87.1.0.call.87.8() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.87.8", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.lookup.0.call.87.1.0.call.88.13() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.88.13", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.lookup.0.call.87.1.0.call.89.18() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.89.18", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.lookup.0.call.87.1.0.call.90.29() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "ac_summary_rob.lookup.0.call.87.1.0.call.90.29", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.lookup.0.call.87.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "ac_summary_rob.lookup.0.call.87.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v3", acir.source_type = "u32"} : i32
    %1 = arith.andi %0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = call @ac_summary_rob.lookup.0.call.87.1.0.call.87.8() {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb2:  // pred: ^bb0
    %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = call @ac_summary_rob.lookup.0.call.87.1.0.call.88.13() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb4:  // pred: ^bb2
    %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32
    cf.cond_br %6, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %7 = call @ac_summary_rob.lookup.0.call.87.1.0.call.89.18() {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb6:  // pred: ^bb4
    %8 = call @ac_summary_rob.lookup.0.call.87.1.0.call.90.29() {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.complete.0.call.92.2.0.call.92.9(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot0", definition = "complete", instance = "rob.slot0", interface = "Module:rob.slot0:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.92.9", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.complete.0.call.92.2.0.call.93.13(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot1", definition = "complete", instance = "rob.slot1", interface = "Module:rob.slot1:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.93.13", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.complete.0.call.92.2.0.call.94.17(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot2", definition = "complete", instance = "rob.slot2", interface = "Module:rob.slot2:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.94.17", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.complete.0.call.92.2.0.call.95.18(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob.slot3", definition = "complete", instance = "rob.slot3", interface = "Module:rob.slot3:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2.0.call.95.18", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v7", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %7 = arith.cmpi eq, %6, %arg0 {acir.id = "v15", acir.source_type = "bool"} : i32
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = acir.update %1, %true {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["done"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i1) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %10 = acir.update %9, %arg1 {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.RobEntry", path = ["value"]} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %0, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb6
  ^bb6:  // 2 preds: ^bb4, ^bb5
    return
  }
  func.func @ac_summary_rob.complete.0.call.92.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rob", definition = "complete", instance = "rob", interface = "Module:rob:complete", kind = "method", method = "complete", name = "ac_summary_rob.complete.0.call.92.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32
    %1 = arith.andi %0, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    call @ac_summary_rob.complete.0.call.92.2.0.call.92.9(%arg0, %arg1) {acir.id = "v10", acir.source_type = "void", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:complete", method = "complete", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7
  ^bb2:  // pred: ^bb0
    %3 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32
    cf.cond_br %3, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    call @ac_summary_rob.complete.0.call.92.2.0.call.93.13(%arg0, %arg1) {acir.id = "v14", acir.source_type = "void", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:complete", method = "complete", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7
  ^bb4:  // pred: ^bb2
    %4 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v16", acir.source_type = "bool"} : i32
    cf.cond_br %4, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    call @ac_summary_rob.complete.0.call.92.2.0.call.94.17(%arg0, %arg1) {acir.id = "v18", acir.source_type = "void", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:complete", method = "complete", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7
  ^bb6:  // pred: ^bb4
    call @ac_summary_rob.complete.0.call.92.2.0.call.95.18(%arg0, %arg1) {acir.id = "v19", acir.source_type = "void", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:complete", method = "complete", role = "method"} : (i32, i32) -> ()
    cf.br ^bb7
  ^bb7:  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    return
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.97.9.0.call.97.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot0", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.97.9.0.call.97.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot0.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.97.9(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "retire", instance = "rob.slot0", interface = "Module:rob.slot0:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.97.9", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.retire.0.call.97.0.0.call.97.9.0.call.97.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot0", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot0.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.99.13.0.call.99.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot1", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.99.13.0.call.99.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot1.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.99.13(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "retire", instance = "rob.slot1", interface = "Module:rob.slot1:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.99.13", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.retire.0.call.97.0.0.call.99.13.0.call.99.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot1", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot1.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.101.17.0.call.101.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot2", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.101.17.0.call.101.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot2.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.101.17(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "retire", instance = "rob.slot2", interface = "Module:rob.slot2:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.101.17", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.retire.0.call.97.0.0.call.101.17.0.call.101.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot2", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot2.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.103.18.0.call.103.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "completed", instance = "rob.slot3", kind = "guard", name = "ac_summary_rob.retire.0.call.97.0.0.call.103.18.0.call.103.1", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "rob.slot3.state" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb2(%3 : i1)
  ^bb2(%4: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %4, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %5 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = arith.cmpi eq, %5, %arg0 {acir.id = "v13", acir.source_type = "bool"} : i32
    cf.br ^bb4(%6 : i1)
  ^bb4(%7: i1):  // 2 preds: ^bb2, ^bb3
    return %7 : i1
  }
  func.func @ac_summary_rob.retire.0.call.97.0.0.call.103.18(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "retire", instance = "rob.slot3", interface = "Module:rob.slot3:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0.0.call.103.18", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_rob.retire.0.call.97.0.0.call.103.18.0.call.103.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "rob.slot3", role = "guard"} : (i32) -> i1
    acir.guard %0 "completed" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "rob.slot3.state" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %3 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.RobEntry"} : (i1, i1, i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    acir.write %1, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>, !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rob.retire.0.call.97.0() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "retire", instance = "rob", interface = "Module:rob:retire", kind = "method", method = "retire", name = "ac_summary_rob.retire.0.call.97.0", params = [], result = "src_payloads_25ebcaa7.RobEntry", summary = true}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = acir.get "rob.head" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %2 = arith.subi %1, %c1_i32 {acir.id = "v4", acir.source_type = "u32"} : i32
    %3 = arith.andi %2, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
    %4 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32
    cf.cond_br %4, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %5 = call @ac_summary_rob.retire.0.call.97.0.0.call.97.9(%1) {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb7(%5, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb2:  // pred: ^bb0
    %6 = arith.cmpi eq, %3, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32
    cf.cond_br %6, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %7 = call @ac_summary_rob.retire.0.call.97.0.0.call.99.13(%1) {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb7(%7, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb4:  // pred: ^bb2
    %8 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v16", acir.source_type = "bool"} : i32
    cf.cond_br %8, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %9 = call @ac_summary_rob.retire.0.call.97.0.0.call.101.17(%1) {acir.id = "v18", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb7(%9, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb6:  // pred: ^bb4
    %10 = call @ac_summary_rob.retire.0.call.97.0.0.call.103.18(%1) {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:retire", method = "retire", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    cf.br ^bb7(%10, %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">, i32)
  ^bb7(%11: !acir.struct<"src_payloads_25ebcaa7.RobEntry">, %12: i32):  // 4 preds: ^bb1, ^bb3, ^bb5, ^bb6
    %13 = arith.addi %12, %c1_i32 {acir.id = "v36", acir.source_type = "u32"} : i32
    acir.write %0, %13 {acir.id = "v37", acir.source_type = "void"} : !acir.cell<i32>, i32
    return %11 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.106.8() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot0", definition = "peek", instance = "rob.slot0", interface = "Module:rob.slot0:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.106.8", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot0.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.107.13() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot1", definition = "peek", instance = "rob.slot1", interface = "Module:rob.slot1:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.107.13", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot1.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.108.18() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot2", definition = "peek", instance = "rob.slot2", interface = "Module:rob.slot2:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.108.18", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot2.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.109.29() -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob.slot3", definition = "peek", instance = "rob.slot3", interface = "Module:rob.slot3:peek", kind = "method", method = "peek", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.109.29", params = [], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %0 = acir.get "rob.slot3.state" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.RobEntry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.RobEntry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.RobEntry">> -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %1 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rf.operand.0.call.106.1.0.call.106.31(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry"> attributes {acir.function = {contract = "Module:rob", definition = "lookup", instance = "rob", interface = "Module:rob:lookup", kind = "method", method = "lookup", name = "ac_summary_rf.operand.0.call.106.1.0.call.106.31", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.RobEntry"}} {
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = arith.subi %arg0, %c1_i32 {acir.id = "v3", acir.source_type = "u32"} : i32
    %1 = arith.andi %0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
    %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = call @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.106.8() {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot0", instance = "rob.slot0", interface = "Module:rob.slot0:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %3 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb2:  // pred: ^bb0
    %4 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v12", acir.source_type = "bool"} : i32
    cf.cond_br %4, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    %5 = call @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.107.13() {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot1", instance = "rob.slot1", interface = "Module:rob.slot1:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %5 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb4:  // pred: ^bb2
    %6 = arith.cmpi eq, %1, %c2_i32 {acir.id = "v17", acir.source_type = "bool"} : i32
    cf.cond_br %6, ^bb5, ^bb6
  ^bb5:  // pred: ^bb4
    %7 = call @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.108.18() {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot2", instance = "rob.slot2", interface = "Module:rob.slot2:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %7 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  ^bb6:  // pred: ^bb4
    %8 = call @ac_summary_rf.operand.0.call.106.1.0.call.106.31.0.call.109.29() {acir.id = "v30", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob.slot3", instance = "rob.slot3", interface = "Module:rob.slot3:peek", method = "peek", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    return %8 : !acir.struct<"src_payloads_25ebcaa7.RobEntry">
  }
  func.func @ac_summary_rf.operand.0.call.106.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand"> attributes {acir.function = {contract = "Module:rf", definition = "operand", instance = "rf", interface = "Module:rf:operand", kind = "method", method = "operand", name = "ac_summary_rf.operand.0.call.106.1", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Operand", summary = true}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %true = arith.constant {acir.source_type = "bool"} true
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
    %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v3", acir.source_type = "bool"} : i32
    acir.check %0 {acir.id = "v4", acir.source_type = "void"}
    %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v6", acir.source_type = "bool"} : i32
    %2 = acir.get "rf.renamed" {acir.id = "v15", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    %3 = acir.get "rf.committed" {acir.id = "v18", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    %4 = acir.get "rf.registers" {acir.id = "v24", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    cf.cond_br %1, ^bb1, ^bb2(%arg0 : i32)
  ^bb1:  // pred: ^bb0
    %5 = acir.aggregate(%true, %c0_i32, %c0_i32) {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %5 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  ^bb2(%6: i32):  // pred: ^bb0
    %7 = acir.read %2 {acir.id = "v16", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %8 = acir.extract %7[%6] {acir.id = "v17", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
    %9 = acir.read %3 {acir.id = "v19", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %10 = acir.extract %9[%6] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
    %11 = arith.cmpi eq, %8, %10 {acir.id = "v21", acir.source_type = "bool"} : i32
    cf.cond_br %11, ^bb3, ^bb4(%8 : i32)
  ^bb3:  // pred: ^bb2
    %12 = acir.read %4 {acir.id = "v25", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %13 = acir.extract %12[%6] {acir.id = "v26", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 8>, i32) -> i32
    %14 = acir.aggregate(%true, %8, %13) {acir.id = "v27", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %14 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  ^bb4(%15: i32):  // pred: ^bb2
    %16 = call @ac_summary_rf.operand.0.call.106.1.0.call.106.31(%15) {acir.id = "v32", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:lookup", method = "lookup", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    %17 = acir.extract %16 {acir.id = "v33", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.cond_br %17, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %18 = acir.extract %16 {acir.id = "v35", acir.source_type = "bool", field = "done"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i1
    cf.br ^bb6(%18 : i1)
  ^bb6(%19: i1):  // 2 preds: ^bb4, ^bb5
    cf.cond_br %19, ^bb7, ^bb8(%false : i1)
  ^bb7:  // pred: ^bb6
    %20 = acir.extract %16 {acir.id = "v41", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %21 = arith.cmpi eq, %20, %15 {acir.id = "v42", acir.source_type = "bool"} : i32
    cf.br ^bb8(%21 : i1)
  ^bb8(%22: i1):  // 2 preds: ^bb6, ^bb7
    cf.cond_br %22, ^bb9, ^bb10(%15 : i32)
  ^bb9:  // pred: ^bb8
    %23 = acir.extract %16 {acir.id = "v49", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %24 = acir.aggregate(%true, %15, %23) {acir.id = "v50", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %24 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  ^bb10(%25: i32):  // pred: ^bb8
    %26 = acir.aggregate(%false, %25, %c0_i32) {acir.id = "v58", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    return %26 : !acir.struct<"src_payloads_25ebcaa7.Operand">
  }
  func.func @ac_summary_rf.rename.0.call.112.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:rf", definition = "rename", instance = "rf", interface = "Module:rf:rename", kind = "method", method = "rename", name = "ac_summary_rf.rename.0.call.112.2", params = [{consume = false, id = "v1", name = "rd", targets = [], type = "u32"}, {consume = false, id = "v2", name = "tag", targets = [], type = "u32"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c8_i32 = arith.constant {acir.source_type = "u32"} 8 : i32
    %0 = arith.cmpi ult, %arg0, %c8_i32 {acir.id = "v4", acir.source_type = "bool"} : i32
    acir.check %0 {acir.id = "v5", acir.source_type = "void"}
    %1 = arith.cmpi ne, %arg0, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32
    %2 = acir.get "rf.renamed" {acir.id = "v9", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %3 = acir.read %2 {acir.id = "v10", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %4 = acir.update %3, %arg1[%arg0] {acir.id = "v11", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
    acir.write %2, %4 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @ac_summary_rf.commit.0.call.113.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.RobEntry">) attributes {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "ac_summary_rf.commit.0.call.113.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.RobEntry"}], result = "void", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %1 = arith.cmpi ne, %0, %c0_i32 {acir.id = "v4", acir.source_type = "bool"} : i32
    %2 = acir.get "rf.registers" {acir.id = "v6", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    %3 = acir.get "rf.committed" {acir.id = "v12", acir.source_type = "cell<array<u32,8>>"} : !acir.cell<!acir.array<i32, 8>>
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %4 = acir.extract %arg0 {acir.id = "v7", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %5 = acir.extract %arg0 {acir.id = "v8", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %6 = acir.read %2 {acir.id = "v9", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %7 = acir.update %6, %4[%5] {acir.id = "v10", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
    acir.write %2, %7 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
    %8 = acir.extract %arg0 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %9 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    %10 = acir.read %3 {acir.id = "v15", acir.source_type = "array<u32,8>", role = "observe", view = "old"} : !acir.cell<!acir.array<i32, 8>> -> !acir.array<i32, 8>
    %11 = acir.update %10, %8[%9] {acir.id = "v16", acir.source_type = "array<u32,8>", path = [unit]} : (!acir.array<i32, 8>, i32, i32) -> !acir.array<i32, 8>
    acir.write %3, %11 {acir.id = "v17", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 8>>, !acir.array<i32, 8>
    cf.br ^bb2
  ^bb2:  // 2 preds: ^bb0, ^bb1
    return
  }
  func.func @ac_summary_left.place.0.call.114.1.0.call.114.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "left", kind = "guard", name = "ac_summary_left.place.0.call.114.1.0.call.114.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.get "left.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @ac_summary_left.place.0.call.114.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) attributes {acir.function = {contract = "Module:left", definition = "place", instance = "left", interface = "Module:left:place", kind = "method", method = "place", name = "ac_summary_left.place.0.call.114.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void", summary = true}} {
    %0 = call @ac_summary_left.place.0.call.114.1.0.call.114.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    acir.guard %0 "free" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "left.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    acir.write %1, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return
  }
  func.func @ac_summary_left.take.0.call.116.1.0.call.116.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "left", kind = "guard", name = "ac_summary_left.take.0.call.116.1.0.call.116.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1
  }
  func.func @ac_summary_left.take.0.call.116.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:left", definition = "take", instance = "left", interface = "Module:left:take", kind = "method", method = "take", name = "ac_summary_left.take.0.call.116.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_left.take.0.call.116.1.0.call.116.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "left", role = "guard"} : (i32) -> i1
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "left.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_summary_left.wakeup.0.call.118.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:left", definition = "wakeup", instance = "left", interface = "Module:left:wakeup", kind = "method", method = "wakeup", name = "ac_summary_left.wakeup.0.call.118.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %2, ^bb1, ^bb9(%false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.extract %3 {acir.id = "v9", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    %5 = acir.unary %4 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %5, ^bb2, ^bb3(%false : i1)
  ^bb2:  // pred: ^bb1
    %6 = acir.extract %1 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %7 = acir.extract %6 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v14", acir.source_type = "bool"} : i32
    cf.br ^bb3(%8 : i1)
  ^bb3(%9: i1):  // 2 preds: ^bb1, ^bb2
    cf.cond_br %9, ^bb4, ^bb5(%false, %1, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb4:  // pred: ^bb3
    %10 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %11 = acir.update %1, %10 {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb5(%true, %11, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb5(%12: i1, %13: !acir.struct<"src_payloads_25ebcaa7.Entry">, %14: i32, %15: i32):  // 2 preds: ^bb3, ^bb4
    %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %17 = acir.extract %16 {acir.id = "v32", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    %18 = acir.unary %17 "not" {acir.id = "v33", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %18, ^bb6, ^bb7(%false : i1)
  ^bb6:  // pred: ^bb5
    %19 = acir.extract %13 {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %20 = acir.extract %19 {acir.id = "v36", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %21 = arith.cmpi eq, %20, %14 {acir.id = "v37", acir.source_type = "bool"} : i32
    cf.br ^bb7(%21 : i1)
  ^bb7(%22: i1):  // 2 preds: ^bb5, ^bb6
    cf.cond_br %22, ^bb8, ^bb9(%12, %13 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb8:  // pred: ^bb7
    %23 = acir.aggregate(%true, %14, %15) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %24 = acir.update %13, %23 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb9(%true, %24 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb9(%25: i1, %26: !acir.struct<"src_payloads_25ebcaa7.Entry">):  // 3 preds: ^bb0, ^bb7, ^bb8
    cf.cond_br %25, ^bb10, ^bb11
  ^bb10:  // pred: ^bb9
    acir.write %0, %26 {acir.id = "v61", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb11
  ^bb11:  // 2 preds: ^bb9, ^bb10
    return
  }
  func.func @ac_summary_right.place.0.call.119.1.0.call.119.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1 attributes {acir.function = {definition = "free", instance = "right", kind = "guard", name = "ac_summary_right.place.0.call.119.1.0.call.119.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "bool"}} {
    %0 = acir.get "right.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    %3 = acir.unary %2 "not" {acir.id = "v5", acir.source_type = "bool"} : i1 -> i1
    return %3 : i1
  }
  func.func @ac_summary_right.place.0.call.119.1(%arg0: !acir.struct<"src_payloads_25ebcaa7.Entry">) attributes {acir.function = {contract = "Module:right", definition = "place", instance = "right", interface = "Module:right:place", kind = "method", method = "place", name = "ac_summary_right.place.0.call.119.1", params = [{consume = false, id = "v1", name = "entry", targets = [], type = "src_payloads_25ebcaa7.Entry"}], result = "void", summary = true}} {
    %0 = call @ac_summary_right.place.0.call.119.1.0.call.119.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    acir.guard %0 "free" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "right.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    acir.write %1, %arg0 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return
  }
  func.func @ac_summary_right.take.0.call.121.1.0.call.121.1(%arg0: i32) -> i1 attributes {acir.function = {definition = "issuable", instance = "right", kind = "guard", name = "ac_summary_right.take.0.call.121.1.0.call.121.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "bool"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "right.slot" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v4", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %2, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %4 = arith.cmpi eq, %3, %arg0 {acir.id = "v7", acir.source_type = "bool"} : i32
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %6 = acir.extract %1 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %7 = acir.extract %6 {acir.id = "v14", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb4(%7 : i1)
  ^bb4(%8: i1):  // 2 preds: ^bb2, ^bb3
    cf.cond_br %8, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %9 = acir.extract %1 {acir.id = "v20", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %10 = acir.extract %9 {acir.id = "v21", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb6(%10 : i1)
  ^bb6(%11: i1):  // 2 preds: ^bb4, ^bb5
    return %11 : i1
  }
  func.func @ac_summary_right.take.0.call.121.1(%arg0: i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry"> attributes {acir.function = {contract = "Module:right", definition = "take", instance = "right", interface = "Module:right:take", kind = "method", method = "take", name = "ac_summary_right.take.0.call.121.1", params = [{consume = false, id = "v1", name = "kind", targets = [], type = "u32"}], result = "src_payloads_25ebcaa7.Entry", summary = true}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = call @ac_summary_right.take.0.call.121.1.0.call.121.1(%arg0) {acir.id = "v2", acir.source_type = "bool", instance = "right", role = "guard"} : (i32) -> i1
    acir.guard %0 "issuable" {acir.id = "v3", acir.source_type = "void", source = "function"}
    %1 = acir.get "right.slot" {acir.id = "v4", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %2 = acir.read %1 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %3 = acir.aggregate() {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.aggregate() {acir.id = "v11", acir.source_type = "src_payloads_25ebcaa7.Operand"} : () -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %3, %4) {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.write %1, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return %2 : !acir.struct<"src_payloads_25ebcaa7.Entry">
  }
  func.func @ac_summary_right.wakeup.0.call.123.2(%arg0: i32, %arg1: i32) attributes {acir.function = {contract = "Module:right", definition = "wakeup", instance = "right", interface = "Module:right:wakeup", kind = "method", method = "wakeup", name = "ac_summary_right.wakeup.0.call.123.2", params = [{consume = false, id = "v1", name = "tag", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "right.slot" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.extract %1 {acir.id = "v6", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %2, ^bb1, ^bb9(%false, %1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb1:  // pred: ^bb0
    %3 = acir.extract %1 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.extract %3 {acir.id = "v9", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    %5 = acir.unary %4 "not" {acir.id = "v10", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %5, ^bb2, ^bb3(%false : i1)
  ^bb2:  // pred: ^bb1
    %6 = acir.extract %1 {acir.id = "v12", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %7 = acir.extract %6 {acir.id = "v13", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %8 = arith.cmpi eq, %7, %arg0 {acir.id = "v14", acir.source_type = "bool"} : i32
    cf.br ^bb3(%8 : i1)
  ^bb3(%9: i1):  // 2 preds: ^bb1, ^bb2
    cf.cond_br %9, ^bb4, ^bb5(%false, %1, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb4:  // pred: ^bb3
    %10 = acir.aggregate(%true, %arg0, %arg1) {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %11 = acir.update %1, %10 {acir.id = "v23", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["lhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb5(%true, %11, %arg0, %arg1 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">, i32, i32)
  ^bb5(%12: i1, %13: !acir.struct<"src_payloads_25ebcaa7.Entry">, %14: i32, %15: i32):  // 2 preds: ^bb3, ^bb4
    %16 = acir.extract %13 {acir.id = "v31", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %17 = acir.extract %16 {acir.id = "v32", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    %18 = acir.unary %17 "not" {acir.id = "v33", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %18, ^bb6, ^bb7(%false : i1)
  ^bb6:  // pred: ^bb5
    %19 = acir.extract %13 {acir.id = "v35", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %20 = acir.extract %19 {acir.id = "v36", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %21 = arith.cmpi eq, %20, %14 {acir.id = "v37", acir.source_type = "bool"} : i32
    cf.br ^bb7(%21 : i1)
  ^bb7(%22: i1):  // 2 preds: ^bb5, ^bb6
    cf.cond_br %22, ^bb8, ^bb9(%12, %13 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb8:  // pred: ^bb7
    %23 = acir.aggregate(%true, %14, %15) {acir.id = "v45", acir.source_type = "src_payloads_25ebcaa7.Operand"} : (i1, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %24 = acir.update %13, %23 {acir.id = "v46", acir.source_type = "src_payloads_25ebcaa7.Entry", path = ["rhs"]} : (!acir.struct<"src_payloads_25ebcaa7.Entry">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb9(%true, %24 : i1, !acir.struct<"src_payloads_25ebcaa7.Entry">)
  ^bb9(%25: i1, %26: !acir.struct<"src_payloads_25ebcaa7.Entry">):  // 3 preds: ^bb0, ^bb7, ^bb8
    cf.cond_br %25, ^bb10, ^bb11
  ^bb10:  // pred: ^bb9
    acir.write %0, %26 {acir.id = "v61", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    cf.br ^bb11
  ^bb11:  // 2 preds: ^bb9, ^bb10
    return
  }
  func.func @scheduler.issue_add(%arg0: i1) attributes {acir.function = {kind = "rule", name = "scheduler.issue_add", outputs = [{kind = "queue", name = "scheduler.add_requests", targets = ["scheduler.add_requests"], type = "src_payloads_25ebcaa7.Entry"}], owner = "scheduler", params = [{consume = false, id = "v1", name = "choose_left", targets = [], type = "bool"}], result = "void", rule = "issue_add"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.get "scheduler.add_requests" {acir.id = "v5", acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    cf.cond_br %arg0, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %1 = call @scheduler.issue_add.0.call.0.3(%c0_i32) {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:left", instance = "left", interface = "Module:left:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.read %0 {acir.id = "v6", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "scheduler.add_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> i1
    %3 = acir.unary %2 "not" {acir.id = "v7", acir.source_type = "bool"} : i1 -> i1
    acir.guard %3 "Queue.push capacity" {acir.id = "v8", acir.source_type = "void", implicit = true}
    acir.push %0, %1 {acir.id = "v9", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return
  ^bb2:  // pred: ^bb0
    %4 = call @scheduler.issue_add.0.call.2.13(%c0_i32) {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:right", instance = "right", interface = "Module:right:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %5 = acir.read %0 {acir.id = "v15", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "scheduler.add_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> i1
    %6 = acir.unary %5 "not" {acir.id = "v16", acir.source_type = "bool"} : i1 -> i1
    acir.guard %6 "Queue.push capacity" {acir.id = "v17", acir.source_type = "void", implicit = true}
    acir.push %0, %4 {acir.id = "v18", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return
  }
  func.func @scheduler.issue_mul(%arg0: i1) attributes {acir.function = {kind = "rule", name = "scheduler.issue_mul", outputs = [{kind = "queue", name = "scheduler.mul_requests", targets = ["scheduler.mul_requests"], type = "src_payloads_25ebcaa7.Entry"}], owner = "scheduler", params = [{consume = false, id = "v1", name = "choose_left", targets = [], type = "bool"}], result = "void", rule = "issue_mul"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = acir.get "scheduler.mul_requests" {acir.id = "v5", acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    cf.cond_br %arg0, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %1 = call @scheduler.issue_mul.0.call.4.3(%c1_i32) {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:left", instance = "left", interface = "Module:left:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.read %0 {acir.id = "v6", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "scheduler.mul_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> i1
    %3 = acir.unary %2 "not" {acir.id = "v7", acir.source_type = "bool"} : i1 -> i1
    acir.guard %3 "Queue.push capacity" {acir.id = "v8", acir.source_type = "void", implicit = true}
    acir.push %0, %1 {acir.id = "v9", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return
  ^bb2:  // pred: ^bb0
    %4 = call @scheduler.issue_mul.0.call.6.13(%c1_i32) {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Entry", contract = "Module:right", instance = "right", interface = "Module:right:take", method = "take", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %5 = acir.read %0 {acir.id = "v15", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "scheduler.mul_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> i1
    %6 = acir.unary %5 "not" {acir.id = "v16", acir.source_type = "bool"} : i1 -> i1
    acir.guard %6 "Queue.push capacity" {acir.id = "v17", acir.source_type = "void", implicit = true}
    acir.push %0, %4 {acir.id = "v18", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    return
  }
  func.func @scheduler_Work() attributes {acir.function = {kind = "work", name = "scheduler_Work", owner = "scheduler", params = [], result = "void"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "left.slot" {acir.id = "v1", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.get "right.slot" {acir.id = "v3", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %3 = acir.read %2 {acir.id = "v4", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %4 = acir.extract %1 {acir.id = "v5", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %4, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %5 = acir.extract %1 {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %6 = acir.extract %5 {acir.id = "v8", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb2(%6 : i1)
  ^bb2(%7: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %7, ^bb3, ^bb4(%false : i1)
  ^bb3:  // pred: ^bb2
    %8 = acir.extract %1 {acir.id = "v14", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %9 = acir.extract %8 {acir.id = "v15", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb4(%9 : i1)
  ^bb4(%10: i1):  // 2 preds: ^bb2, ^bb3
    %11 = acir.extract %3 {acir.id = "v20", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    cf.cond_br %11, ^bb5, ^bb6(%false : i1)
  ^bb5:  // pred: ^bb4
    %12 = acir.extract %3 {acir.id = "v22", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %13 = acir.extract %12 {acir.id = "v23", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb6(%13 : i1)
  ^bb6(%14: i1):  // 2 preds: ^bb4, ^bb5
    cf.cond_br %14, ^bb7, ^bb8(%false : i1)
  ^bb7:  // pred: ^bb6
    %15 = acir.extract %3 {acir.id = "v29", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %16 = acir.extract %15 {acir.id = "v30", acir.source_type = "bool", field = "ready"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i1
    cf.br ^bb8(%16 : i1)
  ^bb8(%17: i1):  // 2 preds: ^bb6, ^bb7
    cf.cond_br %10, ^bb9, ^bb10(%false : i1)
  ^bb9:  // pred: ^bb8
    %18 = acir.extract %1 {acir.id = "v36", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %19 = arith.cmpi eq, %18, %c0_i32 {acir.id = "v38", acir.source_type = "bool"} : i32
    cf.br ^bb10(%19 : i1)
  ^bb10(%20: i1):  // 2 preds: ^bb8, ^bb9
    cf.cond_br %17, ^bb11, ^bb12(%false : i1)
  ^bb11:  // pred: ^bb10
    %21 = acir.extract %3 {acir.id = "v44", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %22 = arith.cmpi eq, %21, %c0_i32 {acir.id = "v46", acir.source_type = "bool"} : i32
    cf.br ^bb12(%22 : i1)
  ^bb12(%23: i1):  // 2 preds: ^bb10, ^bb11
    cf.cond_br %10, ^bb13, ^bb14(%false : i1)
  ^bb13:  // pred: ^bb12
    %24 = acir.extract %1 {acir.id = "v52", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %25 = arith.cmpi eq, %24, %c1_i32 {acir.id = "v54", acir.source_type = "bool"} : i32
    cf.br ^bb14(%25 : i1)
  ^bb14(%26: i1):  // 2 preds: ^bb12, ^bb13
    cf.cond_br %17, ^bb15, ^bb16(%false : i1)
  ^bb15:  // pred: ^bb14
    %27 = acir.extract %3 {acir.id = "v60", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %28 = arith.cmpi eq, %27, %c1_i32 {acir.id = "v62", acir.source_type = "bool"} : i32
    cf.br ^bb16(%28 : i1)
  ^bb16(%29: i1):  // 2 preds: ^bb14, ^bb15
    cf.cond_br %20, ^bb17, ^bb19(%false : i1)
  ^bb17:  // pred: ^bb16
    %30 = acir.unary %23 "not" {acir.id = "v68", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %30, ^bb19(%true : i1), ^bb18
  ^bb18:  // pred: ^bb17
    %31 = acir.extract %1 {acir.id = "v71", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %32 = acir.extract %3 {acir.id = "v72", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %33 = arith.cmpi ult, %31, %32 {acir.id = "v73", acir.source_type = "bool"} : i32
    cf.br ^bb19(%33 : i1)
  ^bb19(%34: i1):  // 3 preds: ^bb16, ^bb17, ^bb18
    cf.cond_br %26, ^bb20, ^bb22(%false : i1)
  ^bb20:  // pred: ^bb19
    %35 = acir.unary %29 "not" {acir.id = "v82", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %35, ^bb22(%true : i1), ^bb21
  ^bb21:  // pred: ^bb20
    %36 = acir.extract %1 {acir.id = "v85", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %37 = acir.extract %3 {acir.id = "v86", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %38 = arith.cmpi ult, %36, %37 {acir.id = "v87", acir.source_type = "bool"} : i32
    cf.br ^bb22(%38 : i1)
  ^bb22(%39: i1):  // 3 preds: ^bb19, ^bb20, ^bb21
    acir.invoke "issue_add"(%34) {acir.id = "v95", acir.source_type = "void", callee = "scheduler.issue_add"} : (i1) -> ()
    acir.invoke "issue_mul"(%39) {acir.id = "v97", acir.source_type = "void", callee = "scheduler.issue_mul"} : (i1) -> ()
    return
  }
  func.func @adder.execute(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) attributes {acir.function = {kind = "rule", name = "adder.execute", outputs = [{kind = "queue", name = "adder.out", targets = ["adder.out"], type = "src_payloads_25ebcaa7.Completion"}], owner = "adder", params = [{consume = true, id = "v1", name = "message", targets = ["scheduler.add_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], result = "void", rule = "execute"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "scheduler.add_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, i1
    %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %2 = acir.extract %0 {acir.id = "v7", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %3 = acir.extract %0 {acir.id = "v8", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %4 = acir.extract %3 {acir.id = "v9", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %5 = acir.extract %0 {acir.id = "v10", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %6 = acir.extract %5 {acir.id = "v11", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %7 = arith.addi %4, %6 {acir.id = "v12", acir.source_type = "u32"} : i32
    %8 = acir.aggregate(%1, %2, %7) {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    %9 = acir.get "adder.out" {acir.id = "v14", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    %10 = acir.read %9 {acir.id = "v15", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "adder.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1
    %11 = acir.unary %10 "not" {acir.id = "v16", acir.source_type = "bool"} : i1 -> i1
    acir.guard %11 "Queue.push capacity" {acir.id = "v17", acir.source_type = "void", implicit = true}
    acir.push %9, %8 {acir.id = "v18", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.struct<"src_payloads_25ebcaa7.Completion">
    return
  }
  func.func @adder_Work() attributes {acir.function = {kind = "work", name = "adder_Work", owner = "adder", params = [], result = "void"}} {
    %0 = acir.get "scheduler.add_requests" {acir.id = "v1", acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    acir.invoke "execute"(%0) {acir.id = "v2", acir.source_type = "void", callee = "adder.execute"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> ()
    return
  }
  func.func @multiplier.advance() attributes {acir.function = {kind = "rule", name = "multiplier.advance", outputs = [{kind = "queue", name = "multiplier.out", targets = ["multiplier.out"], type = "src_payloads_25ebcaa7.Completion"}], owner = "multiplier", params = [], result = "void", rule = "advance"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.get "multiplier.remaining" {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %2 = arith.cmpi ne, %1, %c0_i32 {acir.id = "v4", acir.source_type = "bool"} : i32
    %3 = acir.get "multiplier.request" {acir.id = "v12", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %4 = acir.get "multiplier.active" {acir.id = "v14", acir.source_type = "cell<bool>"} : !acir.cell<i1>
    %5 = acir.get "multiplier.out" {acir.id = "v25", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    cf.cond_br %2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %6 = acir.read %0 {acir.id = "v6", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %7 = arith.subi %6, %c1_i32 {acir.id = "v8", acir.source_type = "u32"} : i32
    acir.write %0, %7 {acir.id = "v9", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  ^bb2:  // pred: ^bb0
    %8 = acir.read %3 {acir.id = "v13", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.write %4, %false {acir.id = "v16", acir.source_type = "void"} : !acir.cell<i1>, i1
    %9 = acir.extract %8 {acir.id = "v17", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %10 = acir.extract %8 {acir.id = "v18", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i32
    %11 = acir.extract %8 {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "lhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %12 = acir.extract %11 {acir.id = "v20", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %13 = acir.extract %8 {acir.id = "v21", acir.source_type = "src_payloads_25ebcaa7.Operand", field = "rhs"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %14 = acir.extract %13 {acir.id = "v22", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Operand">) -> i32
    %15 = arith.muli %12, %14 {acir.id = "v23", acir.source_type = "u32"} : i32
    %16 = acir.aggregate(%9, %10, %15) {acir.id = "v24", acir.source_type = "src_payloads_25ebcaa7.Completion"} : (i32, i32, i32) -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    %17 = acir.read %5 {acir.id = "v26", acir.source_type = "bool", aspect = "full", interface = "query.full", port = 1 : i64, ports = [{aspect = "full", port = 1 : i64, resource = "multiplier.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1
    %18 = acir.unary %17 "not" {acir.id = "v27", acir.source_type = "bool"} : i1 -> i1
    acir.guard %18 "Queue.push capacity" {acir.id = "v28", acir.source_type = "void", implicit = true}
    acir.push %5, %16 {acir.id = "v29", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.struct<"src_payloads_25ebcaa7.Completion">
    return
  }
  func.func @multiplier.accept(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) attributes {acir.function = {kind = "rule", name = "multiplier.accept", outputs = [], owner = "multiplier", params = [{consume = true, id = "v1", name = "message", targets = ["scheduler.mul_requests"], type = "queue<src_payloads_25ebcaa7.Entry>"}], result = "void", rule = "accept"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "multiplier.request" {acir.id = "v2", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %1 = acir.read %arg0 {acir.id = "v3", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    acir.pop %arg0 {acir.id = "v4", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    acir.write %arg0, %false {acir.id = "v6", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "scheduler.mul_requests"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>, i1
    acir.write %0, %1 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>, !acir.struct<"src_payloads_25ebcaa7.Entry">
    %2 = acir.get "multiplier.remaining" {acir.id = "v8", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    acir.write %2, %c2_i32 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32
    %3 = acir.get "multiplier.active" {acir.id = "v11", acir.source_type = "cell<bool>"} : !acir.cell<i1>
    acir.write %3, %true {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i1>, i1
    return
  }
  func.func @multiplier_Work() attributes {acir.function = {kind = "work", name = "multiplier_Work", owner = "multiplier", params = [], result = "void"}} {
    %0 = acir.get "multiplier.active" {acir.id = "v1", acir.source_type = "cell<bool>"} : !acir.cell<i1>
    %1 = acir.read %0 {acir.id = "v2", acir.source_type = "bool", role = "observe", view = "old"} : !acir.cell<i1> -> i1
    %2 = acir.get "scheduler.mul_requests" {acir.id = "v6", acir.source_type = "queue<src_payloads_25ebcaa7.Entry>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    cf.cond_br %1, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    acir.invoke "advance"() {acir.id = "v4", acir.source_type = "void", callee = "multiplier.advance"} : () -> ()
    cf.br ^bb3
  ^bb2:  // pred: ^bb0
    acir.invoke "accept"(%2) {acir.id = "v7", acir.source_type = "void", callee = "multiplier.accept"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Entry">>) -> ()
    cf.br ^bb3
  ^bb3:  // 2 preds: ^bb1, ^bb2
    return
  }
  func.func @cdb.broadcast(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg1: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, %arg2: i1) attributes {acir.function = {kind = "rule", name = "cdb.broadcast", outputs = [{kind = "signal", name = "cdb.event", targets = ["cdb.event"], type = "src_payloads_25ebcaa7.Completion"}], owner = "cdb", params = [{consume = true, id = "v1", name = "a", targets = ["adder.out"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {consume = true, id = "v2", name = "b", targets = ["multiplier.out"], type = "queue<src_payloads_25ebcaa7.Completion>"}, {consume = false, id = "v3", name = "choose_add", targets = [], type = "bool"}], result = "void", rule = "broadcast"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "cdb.event" {acir.id = "v28", acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    cf.cond_br %arg2, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    %1 = acir.read %arg0 {acir.id = "v5", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    acir.pop %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    acir.write %arg0, %false {acir.id = "v8", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "adder.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1
    cf.br ^bb3(%1 : !acir.struct<"src_payloads_25ebcaa7.Completion">)
  ^bb2:  // pred: ^bb0
    %2 = acir.read %arg1 {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Completion", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    acir.pop %arg1 {acir.id = "v10", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    acir.write %arg1, %false {acir.id = "v12", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "multiplier.out"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1
    cf.br ^bb3(%2 : !acir.struct<"src_payloads_25ebcaa7.Completion">)
  ^bb3(%3: !acir.struct<"src_payloads_25ebcaa7.Completion">):  // 2 preds: ^bb1, ^bb2
    %4 = acir.extract %3 {acir.id = "v19", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
    %5 = acir.extract %3 {acir.id = "v20", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
    call @cdb.broadcast.0.call.8.20(%4, %5) {acir.id = "v21", acir.source_type = "void", contract = "Module:rob", instance = "rob", interface = "Module:rob:complete", method = "complete", role = "method"} : (i32, i32) -> ()
    %6 = acir.extract %3 {acir.id = "v22", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
    %7 = acir.extract %3 {acir.id = "v23", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
    call @cdb.broadcast.0.call.13.23(%6, %7) {acir.id = "v24", acir.source_type = "void", contract = "Module:left", instance = "left", interface = "Module:left:wakeup", method = "wakeup", role = "method"} : (i32, i32) -> ()
    %8 = acir.extract %3 {acir.id = "v25", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
    %9 = acir.extract %3 {acir.id = "v26", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
    call @cdb.broadcast.0.call.14.26(%8, %9) {acir.id = "v27", acir.source_type = "void", contract = "Module:right", instance = "right", interface = "Module:right:wakeup", method = "wakeup", role = "method"} : (i32, i32) -> ()
    acir.write %0, %3 {acir.id = "v29", acir.source_type = "void", interface = "set", view = "current"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.struct<"src_payloads_25ebcaa7.Completion">
    return
  }
  func.func @cdb_Work() attributes {acir.function = {kind = "work", name = "cdb_Work", owner = "cdb", params = [], result = "void"}} {
    %0 = acir.get "adder.out" {acir.id = "v1", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    %1 = acir.query %0 "empty" {acir.id = "v2", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1
    %2 = acir.unary %1 "not" {acir.id = "v3", acir.source_type = "bool"} : i1 -> i1
    %3 = acir.get "multiplier.out" {acir.id = "v4", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    acir.invoke "broadcast"(%0, %3, %2) {acir.id = "v5", acir.source_type = "void", callee = "cdb.broadcast"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>, i1) -> ()
    return
  }
  func.func @dispatch.dispatch(%arg0: !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) attributes {acir.function = {kind = "rule", name = "dispatch.dispatch", outputs = [], owner = "dispatch", params = [{consume = true, id = "v1", name = "message", targets = ["source"], type = "queue<src_payloads_25ebcaa7.MicroOp>"}], result = "void", rule = "dispatch"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.MicroOp", role = "consume", view = "old"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">> -> !acir.struct<"src_payloads_25ebcaa7.MicroOp">
    acir.pop %arg0 {acir.id = "v3", acir.source_type = "void"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    acir.write %arg0, %false {acir.id = "v5", acir.source_type = "void", aspect = "full", interface = "pop", port = 0 : i64, ports = [{aspect = "full", port = 0 : i64, resource = "source"}], view = "ehr"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>, i1
    %1 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
    %2 = call @dispatch.dispatch.0.call.15.6(%1) {acir.id = "v7", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:rf", instance = "rf", interface = "Module:rf:operand", method = "operand", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %3 = acir.extract %0 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
    %4 = call @dispatch.dispatch.0.call.21.8(%3) {acir.id = "v9", acir.source_type = "src_payloads_25ebcaa7.Operand", contract = "Module:rf", instance = "rf", interface = "Module:rf:operand", method = "operand", role = "method"} : (i32) -> !acir.struct<"src_payloads_25ebcaa7.Operand">
    %5 = acir.extract %0 {acir.id = "v10", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
    %6 = call @dispatch.dispatch.0.call.27.10(%5) {acir.id = "v11", acir.source_type = "u32", contract = "Module:rob", instance = "rob", interface = "Module:rob:allocate", method = "allocate", role = "method"} : (i32) -> i32
    %7 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
    call @dispatch.dispatch.0.call.37.12(%7, %6) {acir.id = "v13", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:rename", method = "rename", role = "method"} : (i32, i32) -> ()
    %8 = acir.extract %0 {acir.id = "v15", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
    %9 = acir.extract %0 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_payloads_25ebcaa7.MicroOp">) -> i32
    %10 = acir.aggregate(%true, %8, %6, %9, %2, %4) {acir.id = "v17", acir.source_type = "src_payloads_25ebcaa7.Entry"} : (i1, i32, i32, i32, !acir.struct<"src_payloads_25ebcaa7.Operand">, !acir.struct<"src_payloads_25ebcaa7.Operand">) -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %11 = acir.get "left.slot" {acir.id = "v18", acir.source_type = "cell<src_payloads_25ebcaa7.Entry>"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">>
    %12 = acir.read %11 {acir.id = "v19", acir.source_type = "src_payloads_25ebcaa7.Entry", role = "observe", view = "old"} : !acir.cell<!acir.struct<"src_payloads_25ebcaa7.Entry">> -> !acir.struct<"src_payloads_25ebcaa7.Entry">
    %13 = acir.extract %12 {acir.id = "v20", acir.source_type = "bool", field = "valid"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> i1
    %14 = acir.unary %13 "not" {acir.id = "v21", acir.source_type = "bool"} : i1 -> i1
    cf.cond_br %14, ^bb1, ^bb2
  ^bb1:  // pred: ^bb0
    call @dispatch.dispatch.0.call.38.22(%10) {acir.id = "v23", acir.source_type = "void", contract = "Module:left", instance = "left", interface = "Module:left:place", method = "place", role = "method"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> ()
    cf.br ^bb3
  ^bb2:  // pred: ^bb0
    call @dispatch.dispatch.0.call.40.23(%10) {acir.id = "v24", acir.source_type = "void", contract = "Module:right", instance = "right", interface = "Module:right:place", method = "place", role = "method"} : (!acir.struct<"src_payloads_25ebcaa7.Entry">) -> ()
    cf.br ^bb3
  ^bb3:  // 2 preds: ^bb1, ^bb2
    return
  }
  func.func @dispatch_Work() attributes {acir.function = {kind = "work", name = "dispatch_Work", owner = "dispatch", params = [], result = "void"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.get "adder.out" {acir.id = "v1", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    %1 = acir.query %0 "empty" {acir.id = "v2", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1
    %2 = acir.get "multiplier.out" {acir.id = "v4", acir.source_type = "queue<src_payloads_25ebcaa7.Completion>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    %3 = acir.get "source" {acir.id = "v11", acir.source_type = "queue<src_payloads_25ebcaa7.MicroOp>"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>
    cf.cond_br %1, ^bb1, ^bb2(%false : i1)
  ^bb1:  // pred: ^bb0
    %4 = acir.query %2 "empty" {acir.id = "v5", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> i1
    cf.br ^bb2(%4 : i1)
  ^bb2(%5: i1):  // 2 preds: ^bb0, ^bb1
    cf.cond_br %5, ^bb3, ^bb4
  ^bb3:  // pred: ^bb2
    acir.invoke "dispatch"(%3) {acir.id = "v12", acir.source_type = "void", callee = "dispatch.dispatch"} : (!acir.queue<!acir.struct<"src_payloads_25ebcaa7.MicroOp">>) -> ()
    cf.br ^bb4
  ^bb4:  // 2 preds: ^bb2, ^bb3
    return
  }
  func.func @commit.commit() attributes {acir.function = {kind = "rule", name = "commit.commit", outputs = [], owner = "commit", params = [], result = "void", rule = "commit"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    %0 = call @commit.commit.0.call.42.0() {acir.id = "v1", acir.source_type = "src_payloads_25ebcaa7.RobEntry", contract = "Module:rob", instance = "rob", interface = "Module:rob:retire", method = "retire", role = "method"} : () -> !acir.struct<"src_payloads_25ebcaa7.RobEntry">
    call @commit.commit.0.call.51.1(%0) {acir.id = "v2", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:commit", method = "commit", role = "method"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> ()
    %1 = acir.get "commit.retired" {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %2 = acir.read %1 {acir.id = "v4", acir.source_type = "u32", role = "observe", view = "old"} : !acir.cell<i32> -> i32
    %3 = arith.addi %2, %c1_i32 {acir.id = "v6", acir.source_type = "u32"} : i32
    acir.write %1, %3 {acir.id = "v7", acir.source_type = "void"} : !acir.cell<i32>, i32
    %4 = acir.get "commit.last_tag" {acir.id = "v8", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %5 = acir.extract %0 {acir.id = "v9", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.RobEntry">) -> i32
    acir.write %4, %5 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @commit_Work() attributes {acir.function = {kind = "work", name = "commit_Work", owner = "commit", params = [], result = "void"}} {
    acir.invoke "commit"() {acir.id = "v1", acir.source_type = "void", callee = "commit.commit"} : () -> ()
    return
  }
  func.func @audit.observe(%arg0: !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) attributes {acir.function = {kind = "rule", name = "audit.observe", outputs = [], owner = "audit", params = [{consume = false, id = "v1", name = "value", targets = ["cdb.event"], type = "signal<src_payloads_25ebcaa7.Completion>"}], result = "void", rule = "observe"}} {
    %0 = acir.read %arg0 {acir.id = "v2", acir.source_type = "src_payloads_25ebcaa7.Completion", interface = "get", role = "observe", view = "current"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">> -> !acir.struct<"src_payloads_25ebcaa7.Completion">
    %1 = acir.get "audit.last_tag" {acir.id = "v3", acir.source_type = "cell<u32>"} : !acir.cell<i32>
    %2 = acir.extract %0 {acir.id = "v4", acir.source_type = "u32", field = "tag"} : (!acir.struct<"src_payloads_25ebcaa7.Completion">) -> i32
    acir.write %1, %2 {acir.id = "v5", acir.source_type = "void"} : !acir.cell<i32>, i32
    return
  }
  func.func @audit_Work() attributes {acir.function = {kind = "work", name = "audit_Work", owner = "audit", params = [], result = "void"}} {
    %0 = acir.get "cdb.event" {acir.id = "v1", acir.source_type = "signal<src_payloads_25ebcaa7.Completion>"} : !acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>
    acir.invoke "observe"(%0) {acir.id = "v2", acir.source_type = "void", callee = "audit.observe"} : (!acir.signal<!acir.struct<"src_payloads_25ebcaa7.Completion">>) -> ()
    return
  }
}

