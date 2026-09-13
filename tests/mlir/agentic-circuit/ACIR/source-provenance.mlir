// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/valid.mlir | %FileCheck %s --check-prefix=VALID
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/absolute.mlir 2>&1 | %FileCheck %s --check-prefix=ABSOLUTE
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/empty.mlir 2>&1 | %FileCheck %s --check-prefix=EMPTY
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/wrong-type.mlir 2>&1 | %FileCheck %s --check-prefix=WRONG-TYPE
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/duplicate.mlir 2>&1 | %FileCheck %s --check-prefix=DUPLICATE
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/unsafe-path.mlir 2>&1 | %FileCheck %s --check-prefix=UNSAFE
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/reversed-stack.mlir 2>&1 | %FileCheck %s --check-prefix=UNSAFE

// VALID: ac.source_provenance
// ABSOLUTE: error: source provenance frame is malformed
// EMPTY: error: source provenance must be a non-empty origin array
// WRONG-TYPE: error: source provenance must be a non-empty origin array
// DUPLICATE: error: source provenance origins must be unique and canonical
// UNSAFE: error: source provenance frame is malformed

//--- valid.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "source_map"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input", ac.source_provenance = [{frames = [{column = 5 : i64, file = "src/model.py", kind = "statement", line = 7 : i64}]}]} : !ac.queue<i8>
  ac.sink %input {ac.name = "sink"} : !ac.queue<i8>
}

//--- absolute.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "source_map"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input", ac.source_provenance = [{frames = [{column = 5 : i64, file = "/tmp/model.py", kind = "statement", line = 7 : i64}]}]} : !ac.queue<i8>
  ac.sink %input {ac.name = "sink"} : !ac.queue<i8>
}

//--- empty.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "source_map"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input", ac.source_provenance = []} : !ac.queue<i8>
  ac.sink %input {ac.name = "sink"} : !ac.queue<i8>
}

//--- wrong-type.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "source_map"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input", ac.source_provenance = "forged"} : !ac.queue<i8>
  ac.sink %input {ac.name = "sink"} : !ac.queue<i8>
}

//--- duplicate.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "source_map"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input", ac.source_provenance = [{frames = [{column = 5 : i64, file = "src/model.py", kind = "statement", line = 7 : i64}]}, {frames = [{column = 5 : i64, file = "src/model.py", kind = "statement", line = 7 : i64}]}]} : !ac.queue<i8>
  ac.sink %input {ac.name = "sink"} : !ac.queue<i8>
}

//--- unsafe-path.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "source_map"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input", ac.source_provenance = [{frames = [{column = 5 : i64, file = "./src/model.py", kind = "statement", line = 7 : i64}]}]} : !ac.queue<i8>
  ac.sink %input {ac.name = "sink"} : !ac.queue<i8>
}

//--- reversed-stack.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "source_map"} {
  %input = ac.source depth 1 latency 1 {ac.name = "input", ac.source_provenance = [{frames = [{column = 5 : i64, file = "src/helper.py", kind = "inline_callsite", line = 7 : i64}, {column = 3 : i64, file = "src/model.py", kind = "statement", line = 9 : i64}]}]} : !ac.queue<i8>
  ac.sink %input {ac.name = "sink"} : !ac.queue<i8>
}
