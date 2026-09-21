// Bridges the structural `pyc.module` families the classic frontend emits onto
// the `func.func` representation the pyc checker/optimizer pipeline is written
// against.
//
// The hard break moved the frontend to finite-family modules, but the passes
// that verify combinational cycles, clock domains, logic depth, and compile
// statistics still iterate `func.func`. Those checks therefore silently did
// nothing for any module the frontend produced. This pass runs at the head of
// the pycc pipeline and rewrites the shapes it can represent exactly: one
// concrete case per family and no static parameters. Parameterized families
// keep the structural representation so the family emitters still see them.

#include "pyc/Transforms/Passes.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "pyc/Dialect/PYC/PYCOps.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

#include <string>

using namespace mlir;

namespace pyc {
namespace {

// Physical port names in the order the case emits: controls first, then the
// logical interface. Control names repeat across domains, so they carry the
// same suffix scheme as the emitters' name table: clk, rst, clk_2, rst_2, ...
static void collectCasePortNames(pyc::ModuleCaseOp moduleCase,
                                 llvm::SmallVectorImpl<std::string> &argNames,
                                 llvm::SmallVectorImpl<std::string> &resultNames) {
  auto signature = moduleCase.getSignature();
  auto physical = cast<FunctionType>(signature.getPhysical().getValue());
  argNames.assign(physical.getNumInputs(), std::string());
  resultNames.assign(physical.getNumResults(), std::string());

  llvm::StringMap<unsigned> controlCounts;
  for (auto control : signature.getMapping()
                          .getControls()
                          .getAsRange<pyc::ControlPortMappingAttr>()) {
    if (control.getPhysicalInputIndex() >= argNames.size())
      continue;
    const llvm::StringRef base =
        control.getKind().getValue() == "clock" ? "clk" : "rst";
    unsigned &count = controlCounts[base];
    ++count;
    argNames[control.getPhysicalInputIndex()] =
        count == 1 ? base.str() : base.str() + "_" + std::to_string(count);
  }

  for (auto logical : signature.getMapping()
                         .getLogicalPorts()
                         .getAsRange<pyc::LogicalPortMappingAttr>()) {
    const bool isInput = logical.getDirection().getValue() == "input";
    uint64_t lanes = 0;
    for (auto carrier : logical.getCarriers().getAsRange<pyc::PhysicalPortAttr>())
      if (carrier.getLane())
        lanes = std::max(lanes, carrier.getLane().getValue().getZExtValue() + 1);
    for (auto carrier :
         logical.getCarriers().getAsRange<pyc::PhysicalPortAttr>()) {
      auto &ports = isInput ? argNames : resultNames;
      if (carrier.getIndex() >= ports.size())
        continue;
      std::string name = logical.getName().getValue().str();
      llvm::StringRef role = carrier.getRole().getValue();
      if (role == "queue_valid")
        name += "_valid";
      else if (role == "queue_data")
        name += "_data";
      else if (role == "queue_ready")
        name += "_ready";
      if (carrier.getLane() && lanes > 1)
        name += "_" + std::to_string(carrier.getLane().getValue().getZExtValue());
      ports[carrier.getIndex()] = std::move(name);
    }
  }

  for (unsigned i = 0; i < argNames.size(); ++i)
    if (argNames[i].empty())
      argNames[i] = "arg" + std::to_string(i);
  for (unsigned i = 0; i < resultNames.size(); ++i)
    if (resultNames[i].empty())
      resultNames[i] = "out" + std::to_string(i);
}

static constexpr llvm::StringLiteral kEmptyMetrics =
    "{\"source_loc\":0,\"ast_node_count\":0,\"hardware_call_count\":0,"
    "\"loop_count\":0,\"module_call_count\":0,\"state_call_count\":0,"
    "\"estimated_inline_cost\":0,\"instance_count\":0,"
    "\"state_alloc_count\":0,\"collection_count\":0,"
    "\"collection_instance_count\":0,"
    "\"module_family_collection_count\":0,"
    "\"repeated_body_clusters\":[]}";

static bool isBridgeableFamily(pyc::FamilyOp family) {
  if (!family.getSchema().getParameters().getParameters().empty())
    return false;
  return llvm::hasSingleElement(family.getBody().front().getOps<pyc::ModuleCaseOp>());
}

// A source-owned import carries the logical interface but no physical mapping,
// so only value-only interfaces can be reconstructed exactly.
static bool importIsBridgeable(pyc::ModuleImportOp import) {
  for (auto port : import.getSchema()
                       .getInterface()
                       .getPorts()
                       .getAsRange<acir::ac::InterfacePortAttr>()) {
    auto concrete = dyn_cast<acir::ac::TypeExprConcreteAttr>(
        port.getLogicalType().getValue());
    if (!concrete || isa<acir::ac::QueueType>(concrete.getType().getValue()))
      return false;
  }
  return true;
}

static void copyFamilyAttrs(Operation *from, Operation *to) {
  for (auto attr : from->getAttrs()) {
    llvm::StringRef name = attr.getName().getValue();
    if (name == "sym_name" || name == "sym_visibility" || name == "source" ||
        name == "schema")
      continue;
    to->setAttr(attr.getName(), attr.getValue());
  }
}

struct LowerModuleFamiliesToFuncsPass
    : public PassWrapper<LowerModuleFamiliesToFuncsPass,
                         OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerModuleFamiliesToFuncsPass)

  StringRef getArgument() const override { return "pyc-bridge-module-families"; }
  StringRef getDescription() const override {
    return "Rewrite single-case unparameterized pyc.module families as func.func "
           "so the func-based pyc pipeline keeps checking them";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    llvm::SmallVector<pyc::FamilyOp> families(module.getOps<pyc::FamilyOp>());
    llvm::SmallVector<pyc::ModuleImportOp> imports(
        module.getOps<pyc::ModuleImportOp>());
    if (families.empty() && imports.empty())
      return;
    for (pyc::FamilyOp family : families)
      if (!isBridgeableFamily(family))
        return;
    for (pyc::ModuleImportOp import : imports)
      if (!importIsBridgeable(import))
        return;

    OpBuilder builder(module.getContext());
    builder.setInsertionPointToStart(module.getBody());

    for (pyc::FamilyOp family : families) {
      auto moduleCase =
          *family.getBody().front().getOps<pyc::ModuleCaseOp>().begin();
      auto functionType = cast<FunctionType>(
          moduleCase.getSignature().getPhysical().getValue());
      auto func = builder.create<func::FuncOp>(family.getLoc(),
                                               family.getSymName(),
                                               functionType);
      copyFamilyAttrs(family.getOperation(), func.getOperation());
      func->setAttr("pyc.kind", builder.getStringAttr("module"));
      func->setAttr("pyc.params", builder.getStringAttr("{}"));
      func->setAttr("pyc.base", builder.getStringAttr(family.getSymName()));

      llvm::SmallVector<std::string> argNames;
      llvm::SmallVector<std::string> resultNames;
      collectCasePortNames(moduleCase, argNames, resultNames);
      llvm::SmallVector<Attribute> argAttrs;
      llvm::SmallVector<Attribute> resultAttrs;
      for (const std::string &name : argNames)
        argAttrs.push_back(builder.getStringAttr(name));
      for (const std::string &name : resultNames)
        resultAttrs.push_back(builder.getStringAttr(name));
      func->setAttr("arg_names", builder.getArrayAttr(argAttrs));
      func->setAttr("result_names", builder.getArrayAttr(resultAttrs));

      Block *entry = func.addEntryBlock();
      Block &body = moduleCase.getBody().front();
      IRMapping mapping;
      for (auto [target, source] :
           llvm::zip_equal(entry->getArguments(), body.getArguments()))
        mapping.map(source, target);

      builder.setInsertionPointToStart(entry);
      for (Operation &nested : body.without_terminator())
        builder.clone(nested, mapping);
      auto ret = cast<pyc::ReturnOp>(body.getTerminator());
      llvm::SmallVector<Value> results;
      for (Value value : ret.getValues())
        results.push_back(mapping.lookupOrDefault(value));
      builder.create<func::ReturnOp>(ret.getLoc(), results);
      builder.setInsertionPointToStart(module.getBody());

      family.erase();
    }

    for (pyc::ModuleImportOp import : imports) {
      llvm::SmallVector<Type> inputs;
      llvm::SmallVector<Type> results;
      llvm::SmallVector<Attribute> argAttrs;
      llvm::SmallVector<Attribute> resultAttrs;
      inputs.push_back(pyc::ClockType::get(module.getContext()));
      inputs.push_back(pyc::ResetType::get(module.getContext()));
      argAttrs.push_back(builder.getStringAttr("clk"));
      argAttrs.push_back(builder.getStringAttr("rst"));
      for (auto port : import.getSchema()
                           .getInterface()
                           .getPorts()
                           .getAsRange<acir::ac::InterfacePortAttr>()) {
        auto concrete = cast<acir::ac::TypeExprConcreteAttr>(
            port.getLogicalType().getValue());
        Type type = concrete.getType().getValue();
        if (port.getDirection().getValue() == "input") {
          inputs.push_back(type);
          argAttrs.push_back(builder.getStringAttr(port.getName().getValue()));
        } else {
          results.push_back(type);
          resultAttrs.push_back(builder.getStringAttr(port.getName().getValue()));
        }
      }
      auto functionType = builder.getFunctionType(inputs, results);
      auto decl = builder.create<func::FuncOp>(import.getLoc(),
                                               import.getSymName(),
                                               functionType);
      // A `func.func` declaration must be private; instances still reference it.
      decl.setPrivate();
      copyFamilyAttrs(import.getOperation(), decl.getOperation());
      decl->setAttr("pyc.kind", builder.getStringAttr("module"));
      decl->setAttr("pyc.params", builder.getStringAttr("{}"));
      decl->setAttr("pyc.base", builder.getStringAttr(import.getSymName()));
      decl->setAttr("arg_names", builder.getArrayAttr(argAttrs));
      decl->setAttr("result_names", builder.getArrayAttr(resultAttrs));
      // A declaration carries no body, so it stamps the neutral structural
      // summary the frontend contract requires of every module symbol.
      decl->setAttr("pyc.inline", builder.getStringAttr("false"));
      decl->setAttr("pyc.struct.metrics", builder.getStringAttr(kEmptyMetrics));
      decl->setAttr("pyc.struct.collections", builder.getStringAttr("[]"));
      import.erase();
    }
  }
};

} // namespace

std::unique_ptr<::mlir::Pass> createLowerModuleFamiliesToFuncsPass() {
  return std::make_unique<LowerModuleFamiliesToFuncsPass>();
}

static PassRegistration<LowerModuleFamiliesToFuncsPass> pass;

} // namespace pyc
