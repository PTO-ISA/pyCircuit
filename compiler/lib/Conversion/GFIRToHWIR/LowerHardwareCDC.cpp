// CDC physical construction and logical-to-expanded correspondence belong
// to GFIRToHWIR. Analysis supplies only read-only equations and final checks.
#include "pycircuit/Conversion/GFIRToHWIR.h"
#include "Dialect/ACIR/HardwareCDCGraph.h"
#include "Dialect/ACIR/SourceCheckBudget.h"
#include "pycircuit/Dialect/ACIR/GFIRPackageValidation.h"
#include "pycircuit/Dialect/ACIR/HardwareCDC.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
namespace acir::ac {
using detail::cdc::Circuit;
using detail::cdc::getCircuit;
using detail::cdc::registerSymbol;
using detail::cdc::resolve;

namespace {
Operation *registerImport(Operation *resource) {
  Builder b(resource->getContext());
  Type bit = resource->getOperand(0).getType();
  auto payload = TypeParamType::get(
      b.getContext(), FlatSymbolRefAttr::get(b.getContext(), registerSymbol),
      b.getStringAttr("T"));
  SmallVector<Type> inputs{bit, bit, bit, payload, payload, payload};
  SmallVector<Attribute> names;
  for (auto name : {"clk", "rst", "en", "d", "init", "reset_value"})
    names.push_back(b.getStringAttr(name));
  OperationState state(resource->getLoc(), ModuleImportOp::getOperationName());
  state.addAttributes(
      {b.getNamedAttr("sym_name", b.getStringAttr(registerSymbol)),
       b.getNamedAttr(
           "source_owner",
           b.getDictionaryAttr(
               {b.getNamedAttr("package", b.getStringAttr("pycircuit")),
                b.getNamedAttr("path", b.getStringAttr("__builtins__.py"))})),
       b.getNamedAttr("parameters", b.getArrayAttr({})),
       b.getNamedAttr("type_parameters",
                      b.getArrayAttr({b.getStringAttr("T")})),
       b.getNamedAttr("function_type", TypeAttr::get(FunctionType::get(
                                           b.getContext(), inputs, {payload}))),
       b.getNamedAttr("input_names", b.getArrayAttr(names)),
       b.getNamedAttr("output_names", b.getArrayAttr({b.getStringAttr("q")})),
       b.getNamedAttr("semantic_kind", b.getStringAttr("hardware")),
       b.getNamedAttr("primitive_kind", b.getStringAttr("dffe")),
       b.getNamedAttr("ac.register_contract", b.getI64IntegerAttr(3)),
       b.getNamedAttr(
           "dependency_summary",
           b.getArrayAttr({b.getDictionaryAttr(
               {b.getNamedAttr(
                    "output",
                    b.getDictionaryAttr(
                        {b.getNamedAttr("port", b.getI64IntegerAttr(0)),
                         b.getNamedAttr("path", b.getArrayAttr({}))})),
                b.getNamedAttr("inputs", b.getArrayAttr({}))})}))});
  return Operation::create(state);
}
// Source authority is exact, inert attribute assembly: even unknown extension
// attributes can contain symbol/type references to the removed declaration.
// Keeping the whole dictionary also avoids collisions between a source custom
// attribute and a dictionary chosen to encode that attribute.
StringAttr provenance(Attribute value, Builder &b) {
  std::string assembly;
  llvm::raw_string_ostream stream(assembly);
  value.print(stream);
  return b.getStringAttr(stream.str());
}
DictionaryAttr witnessAttributes(const Circuit &circuit) {
  auto *source = circuit.source;
  Builder b(source->getContext());
  NamedAttrList attributes;
  // These checked fields drive the retained public component contract. All
  // remaining attributes are preserved losslessly in the source snapshot.
  for (auto name : {"instance_name", "occurrence", "resource_contract", "source_domain",
                    "target_domain", "write_domain", "read_domain",
                    "source_edge", "target_edge", "write_edge", "read_edge",
                    "reset_polarity", "reset_policy", "release_cycles"})
    if (auto value = source->getAttr(name))
      attributes.set(name, value);
  auto instance = cast<InstanceOp>(source);
  HardwareAnalysis analysis(source->getParentOfType<mlir::ModuleOp>());
  auto *declaration = analysis.resolveCallee(instance);
  attributes.set(
      "ac.cdc_source",
      b.getDictionaryAttr({
          b.getNamedAttr("instance", provenance(source->getAttrDictionary(), b)),
          b.getNamedAttr("declaration",
                         provenance(declaration->getAttrDictionary(), b))}));
  attributes.set("logical_kind", b.getStringAttr(circuit.kind));
  attributes.set("implementation", b.getI64IntegerAttr(1));
  if (circuit.depth)
    attributes.set("depth", b.getI64IntegerAttr(*circuit.depth));
  return attributes.getDictionary(b.getContext());
}
Operation *firstResource(mlir::ModuleOp package) {
  Operation *first = nullptr;
  package.walk([&](Operation *op) {
    if (!first && isCDCInstance(dyn_cast<InstanceOp>(op)))
      first = op;
  });
  return first;
}
} // namespace
FailureOr<OwningOpRef<mlir::ModuleOp>>
materializeHardwareCDC(mlir::ModuleOp logical,
                       HardwareSourceCheckLimits limits) {
  if (failed(verifyRoutedPackage(logical, limits)))
    return failure();
  detail::SourceCheckBudget budget(limits);
  budget.setPhase("CDC physicalization");
  SmallVector<Circuit, 0> circuits;
  LogicalResult valid = success();
  logical.walk([&](Operation *op) {
    if (!isCDCInstance(dyn_cast<InstanceOp>(op)) || failed(valid))
      return;
    auto spec = getCircuit(op, budget);
    if (failed(spec))
      valid = failure();
    else
      circuits.push_back(std::move(*spec));
  });
  if (failed(valid))
    return failure();
  auto cloneBudget = logical.walk([&](Operation *op) {
    return failed(budget.debit(64 + 16 * op->getNumOperands() +
                                   16 * op->getNumResults() +
                                   32 * op->getAttrs().size(),
                               op))
               ? WalkResult::interrupt()
               : WalkResult::advance();
  });
  if (cloneBudget.wasInterrupted())
    return failure();
  IRMapping mapping;
  OwningOpRef<mlir::ModuleOp> result =
      cast<mlir::ModuleOp>(logical->clone(mapping));
  if (!circuits.empty() && SymbolTable::lookupSymbolIn(logical, registerSymbol))
    return logical.emitError(
        "CDC register symbol collides with logical declaration");
  OpBuilder b(logical.getContext());
  b.setInsertionPointToStart(result->getBody());
  if (!circuits.empty())
    b.insert(registerImport(circuits.front().source));
  // Declare all nodes and logical-result replacements before binding any input.
  // SSA graph regions allow feedback through committed storage cuts.
  SmallVector<SmallVector<Operation *, 0>, 0> realized;
  for (auto &circuit : circuits) {
    b.setInsertionPoint(mapping.lookup(circuit.source));
    SmallVector<Operation *, 0> nodes;
    for (auto &node : circuit.nodes) {
      OperationState state(circuit.source->getLoc(), node.operation);
      state.addTypes(node.types);
      state.addAttributes(node.attributes);
      nodes.push_back(b.create(state));
    }
    for (auto [original, ref] :
         llvm::zip(circuit.source->getResults(), circuit.outputs)) {
      auto replacement = resolve(ref, nodes, mapping);
      mapping.lookup(original).replaceAllUsesWith(replacement);
      mapping.erase(original);
      mapping.map(original, replacement);
    }
    realized.push_back(std::move(nodes));
  }
  for (auto [circuit, nodes] : llvm::zip(circuits, realized)) {
    for (auto [equation, operation] : llvm::zip(circuit.nodes, nodes)) {
      SmallVector<Value> inputs;
      for (auto ref : equation.inputs)
        inputs.push_back(resolve(ref, nodes, mapping));
      operation->setOperands(inputs);
    }
    auto *declaration = mapping.lookup(circuit.source);
    OpBuilder witnessBuilder(declaration);
    SmallVector<Value> witnessInputs;
    for (auto input : circuit.source->getOperands())
      witnessInputs.push_back(mapping.lookup(input));
    for (auto output : circuit.outputs)
      witnessInputs.push_back(resolve(output, nodes, mapping));
    OperationState witness(circuit.source->getLoc(), "ac.cdc.refinement");
    witness.addOperands(witnessInputs);
    witness.addAttributes(witnessAttributes(circuit).getValue());
    witnessBuilder.create(witness);
    declaration->erase();
  }
  // Canonical logical declarations have no final-package meaning, including
  // unused imports. Every corresponding instance has already been expanded.
  SmallVector<Operation *> imports;
  result->walk([&](ModuleImportOp imported) {
    if (isCDCPrimitive(imported))
      imports.push_back(imported);
  });
  for (auto *imported : imports)
    imported->erase();
  return std::move(result);
}
namespace {
class Correspondence {
  mlir::ModuleOp logical, expanded;
  detail::SourceCheckBudget budget;
  IRMapping mapping;
  SmallVector<std::pair<Operation *, Operation *>, 0> unchanged;
  struct Expansion {
    Circuit circuit;
    SmallVector<Operation *, 0> nodes;
    Operation *witness = nullptr;
  };
  SmallVector<Expansion, 0> expansions;
  LogicalResult reject(StringRef reason) {
    return logical.emitError() << "CDC correspondence: " << reason;
  }
  LogicalResult index(Operation *source, Operation *target) {
    if (failed(budget.debit(32, source)))
      return failure();
    if (source->getNumResults() != target->getNumResults() ||
        source->getNumRegions() != target->getNumRegions())
      return reject("surrounding operation shape differs");
    unchanged.push_back({source, target});
    mapping.map(source, target);
    for (auto [a, b] : llvm::zip(source->getResults(), target->getResults()))
      mapping.map(a, b);
    for (auto [left, right] :
         llvm::zip(source->getRegions(), target->getRegions())) {
      if (left.getBlocks().size() != right.getBlocks().size())
        return reject("surrounding blocks differ");
      for (auto [a, b] : llvm::zip(left, right)) {
        if (a.getNumArguments() != b.getNumArguments())
          return reject("block arguments differ");
        mapping.map(&a, &b);
        for (auto [x, y] : llvm::zip(a.getArguments(), b.getArguments())) {
          if (x.getType() != y.getType() || x.getLoc() != y.getLoc())
            return reject("argument type/location differs");
          mapping.map(x, y);
        }
        auto cursor = b.begin();
        if (source == logical.getOperation()) {
          if (auto *resource = firstResource(logical)) {
            if (cursor == b.end())
              return reject("missing CDC primitive import");
            Operation *actual = &*cursor++;
            Operation *expected = registerImport(resource);
            bool same =
                actual->getName() == expected->getName() &&
                actual->getAttrDictionary() == expected->getAttrDictionary() &&
                actual->getLoc() == expected->getLoc() &&
                !actual->getNumOperands() && !actual->getNumResults() &&
                !actual->getNumRegions();
            expected->destroy();
            if (!same)
              return reject("CDC primitive import differs");
          }
        }
        for (auto &op : a) {
          if (isCDCPrimitive(&op))
            continue;
          if (isCDCInstance(dyn_cast<InstanceOp>(&op))) {
            auto equations = getCircuit(&op, budget);
            if (failed(equations))
              return failure();
            Expansion expansion{std::move(*equations), {}};
            for (auto &node : expansion.circuit.nodes) {
              if (cursor == b.end())
                return reject("missing generated node");
              auto *actual = &*cursor++;
              if (failed(budget.debit(64 + actual->getNumOperands(), &op)))
                return failure();
              if (actual->getName().getStringRef() != node.operation ||
                  actual->getAttrDictionary() !=
                      node.attributes.getDictionary(op.getContext()) ||
                  !llvm::equal(actual->getResultTypes(), node.types) ||
                  actual->getLoc() != op.getLoc() || actual->getNumRegions() ||
                  actual->getNumSuccessors())
                return reject(
                    "generated component kind/type/policy/identity differs");
              expansion.nodes.push_back(actual);
            }
            for (auto [result, ref] :
                 llvm::zip(op.getResults(), expansion.circuit.outputs))
              mapping.map(result, resolve(ref, expansion.nodes, mapping));
            if (cursor == b.end() ||
                cursor->getName().getStringRef() != "ac.cdc.refinement")
              return reject("missing component proof witness");
            expansion.witness = &*cursor++;
            expansions.push_back(std::move(expansion));
          } else {
            if (cursor == b.end())
              return reject("missing surrounding operation");
            if (failed(index(&op, &*cursor++)))
              return failure();
          }
        }
        if (cursor != b.end())
          return reject("extra generated or surrounding operation");
      }
    }
    return success();
  }

public:
  Correspondence(mlir::ModuleOp logical, mlir::ModuleOp expanded,
                 HardwareSourceCheckLimits limits)
      : logical(logical), expanded(expanded), budget(limits) {
    budget.setPhase("CDC independent correspondence");
  }
  LogicalResult run() {
    if (failed(index(logical, expanded)))
      return failure();
    for (auto [source, target] : unchanged) {
      if (failed(budget.debit(32 + source->getNumOperands(), source)))
        return failure();
      if (source->getName() != target->getName() ||
          source->getAttrDictionary() != target->getAttrDictionary() ||
          source->getLoc() != target->getLoc() ||
          !llvm::equal(source->getResultTypes(), target->getResultTypes()) ||
          source->getNumOperands() != target->getNumOperands() ||
          source->getNumSuccessors() != target->getNumSuccessors())
        return reject("surrounding operation semantics differ");
      for (auto [a, b] :
           llvm::zip(source->getOperands(), target->getOperands()))
        if (mapping.lookupOrNull(a) != b)
          return reject("surrounding SSA connection differs");
      for (auto [a, b] :
           llvm::zip(source->getSuccessors(), target->getSuccessors()))
        if (mapping.lookupOrNull(a) != b)
          return reject("surrounding successor differs");
    }
    // Complete the surrounding SSA map before checking resource inputs: another
    // resource may refer forward to a later graph operation.
    // Final-only legality cannot prove that the witness retained the source
    // endpoint identities, including controls unused by the selected circuit.
    for (auto &expansion : expansions) {
      auto *source = expansion.circuit.source;
      auto *witness = expansion.witness;
      if (failed(budget.debit(64 + source->getNumOperands() +
                                     source->getNumResults(),
                             source)))
        return failure();
      auto attributes = witnessAttributes(expansion.circuit);
      if (witness->getAttrDictionary() != attributes ||
          witness->getLoc() != source->getLoc() || witness->getNumResults() ||
          witness->getNumRegions() || witness->getNumSuccessors())
        return reject("resource witness policy/identity differs");
      SmallVector<Value> inputs(source->getOperands());
      llvm::append_range(inputs, source->getResults());
      if (inputs.size() != witness->getNumOperands())
        return reject("resource witness pin inventory differs");
      for (auto [original, actual] :
           llvm::zip(inputs, witness->getOperands()))
        if (mapping.lookupOrNull(original) != actual)
          return reject("resource witness SSA connection differs");
      if (failed(verifyHardwareCDCComponent(witness)))
        return failure();
      for (auto [equation, actual] :
           llvm::zip(expansion.circuit.nodes, expansion.nodes)) {
        if (equation.inputs.size() != actual->getNumOperands())
          return reject("component pin inventory differs");
        for (auto [ref, input] :
             llvm::zip(equation.inputs, actual->getOperands()))
          if (resolve(ref, expansion.nodes, mapping) != input)
            return reject(
                "component equation/clock/reset/enable connection differs");
      }
    }
    return success();
  }
};
} // namespace
LogicalResult
verifyHardwareCDCMaterialization(mlir::ModuleOp logical,
                                 mlir::ModuleOp expanded,
                                 HardwareSourceCheckLimits limits) {
  if (!expanded || failed(verifyRoutedPackage(logical, limits)))
    return failure();
  return Correspondence(logical, expanded, limits).run();
}
} // namespace acir::ac
