#include "Components.h"
#include "Dialect.h"
using namespace mlir;
acir::ComponentMethod acir::componentMethod(Operation *op) {
  auto name = op->getName().getStringRef();
  if (name == "acir.queue.peek") return {"acir.read", "peek", "old", "!empty({resource})"};
  if (name == "acir.queue.pop") return {"acir.pop", "pop", "old", "!empty({resource})"};
  if (name == "acir.queue.push") return {"acir.push", "push", "old", "!ehr.full({resource},1)"};
  if (name == "acir.queue.empty") return {"acir.query", "query.empty", "old", "true"};
  if (name == "acir.queue.size") return {"acir.query", "query.size", "old", "true"};
  if (name == "acir.analysis.capacity.read") return {"acir.read", "query.full", "ehr", "true"};
  if (name == "acir.analysis.capacity.write") return {"acir.write", "pop", "ehr", "true"};
  if (name == "acir.signal.get") return {"acir.read", "get", "current", "true"};
  if (name == "acir.signal.set") return {"acir.write", "set", "current", "true"};
  if (name == "acir.ehr.read") return {"acir.read", "read", "ehr", "true"};
  if (name == "acir.ehr.write") return {"acir.write", "write", "ehr", "true"};
  if (name == "acir.cell.read") return {"acir.read", "read", "old", "true"};
  if (name == "acir.cell.write") return {"acir.write", "write", "old", "true"};
  return {nullptr, nullptr, nullptr, nullptr};
}
llvm::json::Array acir::componentContracts() {
  auto relation = [](const char *before, const char *after, const char *origin) {
    return llvm::json::Object{{"before", before}, {"after", after}, {"relation", "SB"}, {"source", "default"}, {"origin", origin}};
  };
  llvm::json::Array contracts;
  contracts.push_back(llvm::json::Object{{"id", "Queue"},
    {"methods", llvm::json::Array{"peek", "pop", "push", "query.empty", "query.full", "query.size"}},
    {"guards", llvm::json::Object{{"push", llvm::json::Array{"query.full"}}}},
    {"views", llvm::json::Object{{"full", llvm::json::Object{{"visibility", "ehr"}, {"stages", llvm::json::Array{"pop", "query.full"}}}}}},
    {"relations", llvm::json::Array{relation("peek", "pop", "Pipeline FIFO")}}});
  for (auto name : {"Cell", "EHR"}) contracts.push_back(llvm::json::Object{{"id", name}, {"methods", llvm::json::Array{"read", "write"}}, {"relations", llvm::json::Array{}}});
  contracts.push_back(llvm::json::Object{{"id", "Signal"}, {"methods", llvm::json::Array{"set", "get"}}, {"relations", llvm::json::Array{relation("set", "get", "Signal current value")}}});
  return contracts;
}
