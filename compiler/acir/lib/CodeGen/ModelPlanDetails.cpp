#include "ModelPlanInternal.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Index/IR/IndexOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <system_error>
#include <type_traits>

namespace acir::codegen::detail {
namespace {

template <class... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

llvm::Error detailError(const llvm::Twine &code, const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument), code + ": " + message);
}

std::string printType(mlir::Type type) {
  std::string result;
  llvm::raw_string_ostream(result) << type;
  return result;
}

std::string printAttribute(mlir::Attribute attribute) {
  std::string result;
  llvm::raw_string_ostream(result) << attribute;
  return result;
}

std::string flatSymbol(mlir::DictionaryAttr record, llvm::StringRef name) {
  return record.getAs<mlir::FlatSymbolRefAttr>(name).getValue().str();
}

std::string stringField(mlir::DictionaryAttr record, llvm::StringRef name) {
  return record.getAs<mlir::StringAttr>(name).getValue().str();
}

std::string symbolRefString(mlir::SymbolRefAttr symbol) {
  std::string result = symbol.getRootReference().getValue().str();
  for (mlir::FlatSymbolRefAttr nested : symbol.getNestedReferences()) {
    result += "::";
    result += nested.getValue();
  }
  return result;
}

llvm::Expected<llvm::json::Value> staticValue(mlir::Attribute attribute) {
  if (auto value = mlir::dyn_cast<mlir::BoolAttr>(attribute))
    return llvm::json::Value(value.getValue());
  if (auto value = mlir::dyn_cast<mlir::IntegerAttr>(attribute)) {
    if (!value.getValue().isSignedIntN(64))
      return detailError("ACLOWER-PARAM-PHASE", "integer exceeds int64_t");
    return llvm::json::Value(value.getInt());
  }
  if (auto value = mlir::dyn_cast<mlir::FloatAttr>(attribute)) {
    double number = value.getValueAsDouble();
    if (!std::isfinite(number) || (std::signbit(number) && number == 0.0))
      return detailError("ACLOWER-PARAM-PHASE",
                         "floating value is not canonical I-JSON");
    return llvm::json::Value(number);
  }
  if (auto value = mlir::dyn_cast<mlir::StringAttr>(attribute))
    return llvm::json::Value(value.getValue());
  if (auto values = mlir::dyn_cast<mlir::ArrayAttr>(attribute)) {
    llvm::json::Array result;
    for (mlir::Attribute element : values) {
      auto converted = staticValue(element);
      if (!converted)
        return converted.takeError();
      result.push_back(std::move(*converted));
    }
    return llvm::json::Value(std::move(result));
  }
  if (auto values = mlir::dyn_cast<mlir::DictionaryAttr>(attribute)) {
    llvm::json::Object result;
    for (mlir::NamedAttribute element : values) {
      auto converted = staticValue(element.getValue());
      if (!converted)
        return converted.takeError();
      result[element.getName().getValue()] = std::move(*converted);
    }
    return llvm::json::Value(std::move(result));
  }
  if (mlir::isa<mlir::TypeAttr, mlir::SymbolRefAttr>(attribute))
    return llvm::json::Value(printAttribute(attribute));
  return detailError("ACLOWER-PARAM-PHASE",
                     "unsupported canonical static value");
}

llvm::Expected<std::vector<llvm::json::Value>>
staticValues(mlir::ArrayAttr values) {
  std::vector<llvm::json::Value> result;
  for (mlir::Attribute value : values) {
    auto converted = staticValue(value);
    if (!converted)
      return converted.takeError();
    result.push_back(std::move(*converted));
  }
  return result;
}

std::string generatedClassName(llvm::StringRef symbol,
                               llvm::StringRef fingerprint) {
  fingerprint.consume_front("sha256:");
  return (symbol + "_s" + fingerprint.take_front(16)).str();
}

llvm::Expected<BindingPlan> extractBinding(acsim::BindingOp binding) {
  mlir::DictionaryAttr record = binding.getRecord();
  BindingPlan result;
  result.symbol = binding.getSymName().str();
  result.bindingId = stringField(record, "binding");
  result.effect = stringField(record, "effect") == "pure"
                      ? BindingEffect::Pure
                      : BindingEffect::Stateful;
  result.cppType = flatSymbol(record, "cpp_type");
  result.implementation = flatSymbol(record, "implementation");
  result.provider = flatSymbol(record, "provider");
  result.componentSchema = flatSymbol(record, "component_schema");
  result.recordFingerprint = stringField(record, "fingerprint");
  result.componentSchemaFingerprint =
      stringField(record, "component_schema_fingerprint");
  result.providerImplementationFingerprint =
      stringField(record, "provider_implementation_fingerprint");

  auto cpp = record.getAs<mlir::DictionaryAttr>("cpp");
  result.header = stringField(cpp, "header");
  result.target = stringField(cpp, "target");
  result.cppSymbol = stringField(cpp, "symbol");
  result.conceptName = stringField(cpp, "concept");
  auto entries = cpp.getAs<mlir::DictionaryAttr>("entry_points");
  result.entryPoints = {
      stringField(entries, "pure"), stringField(entries, "reset"),
      stringField(entries, "validate"), stringField(entries, "work"),
      stringField(entries, "xfer")};

  auto construction = record.getAs<mlir::DictionaryAttr>("construction");
  auto arguments =
      staticValues(construction.getAs<mlir::ArrayAttr>("arguments"));
  if (!arguments)
    return arguments.takeError();
  result.constructorArguments = std::move(*arguments);
  auto ownership = record.getAs<mlir::DictionaryAttr>("ownership");
  result.ownershipKind = stringField(ownership, "kind");
  result.ownershipPlacement = stringField(ownership, "placement");

  for (mlir::Attribute attribute :
       record.getAs<mlir::ArrayAttr>("parameters")) {
    auto parameter = mlir::cast<mlir::DictionaryAttr>(attribute);
    auto value = staticValue(parameter.get("value"));
    if (!value)
      return value.takeError();
    auto mapping =
        llvm::StringSwitch<ParameterMappingKind>(
            stringField(parameter, "mapping"))
            .Case("template_argument", ParameterMappingKind::TemplateArgument)
            .Case("constexpr_argument", ParameterMappingKind::ConstexprArgument)
            .Default(ParameterMappingKind::ConstructorConstant);
    result.parameters.push_back(
        {stringField(parameter, "name"), stringField(parameter, "acir_type"),
         stringField(parameter, "cpp_type"), std::move(*value),
         static_cast<uint32_t>(
             parameter.getAs<mlir::IntegerAttr>("ordinal").getInt()),
         mapping});
  }
  for (mlir::Attribute attribute : record.getAs<mlir::ArrayAttr>("ports")) {
    auto port = mlir::cast<mlir::DictionaryAttr>(attribute);
    result.ports.push_back(
        {flatSymbol(port, "accessor"), stringField(port, "cardinality"),
         stringField(port, "delegation"), stringField(port, "direction"),
         flatSymbol(port, "interface"), stringField(port, "ownership"),
         flatSymbol(port, "payload"), flatSymbol(port, "protocol"),
         flatSymbol(port, "role"), flatSymbol(port, "time_domain")});
  }
  for (mlir::Attribute attribute : record.getAs<mlir::ArrayAttr>("resources")) {
    auto resource = mlir::cast<mlir::DictionaryAttr>(attribute);
    result.resources.push_back(
        {flatSymbol(resource, "accessor"), stringField(resource, "delegation"),
         stringField(resource, "mode"), stringField(resource, "ownership"),
         flatSymbol(resource, "resource"), flatSymbol(resource, "role"),
         flatSymbol(resource, "time_domain")});
  }
  for (mlir::Attribute attribute : record.getAs<mlir::ArrayAttr>("results")) {
    auto value = mlir::cast<mlir::DictionaryAttr>(attribute);
    result.results.push_back(
        {stringField(value, "name"), flatSymbol(value, "cpp_type")});
  }
  for (mlir::Attribute attribute :
       record.getAs<mlir::ArrayAttr>("activation_sources")) {
    auto source = mlir::cast<mlir::DictionaryAttr>(attribute);
    result.activationSources.push_back(
        {stringField(source, "name"), flatSymbol(source, "kind")});
  }
  return result;
}

std::string valueName(llvm::DenseMap<mlir::Value, std::string> &names,
                      mlir::Value value, uint32_t &next) {
  auto found = names.find(value);
  if (found != names.end())
    return found->second;
  std::string name = (llvm::Twine("v") + llvm::Twine(next++)).str();
  names.try_emplace(value, name);
  return name;
}

std::vector<std::string>
valueNames(llvm::DenseMap<mlir::Value, std::string> &names,
           mlir::ValueRange values, uint32_t &next) {
  std::vector<std::string> result;
  for (mlir::Value value : values)
    result.push_back(valueName(names, value, next));
  return result;
}

std::vector<std::string> resultTypeNames(mlir::ResultRange values) {
  std::vector<std::string> result;
  for (mlir::Value value : values)
    result.push_back(printType(value.getType()));
  return result;
}

llvm::Expected<ProcessPlan>
extractProcess(acsim::ProcessOp process,
               llvm::DenseMap<mlir::Value, std::string> &moduleValues,
               uint32_t &nextModuleValue) {
  ProcessPlan result;
  result.symbol = process.getSymName().str();
  result.className =
      generatedClassName(result.symbol, process.getSpecializationFingerprint());
  result.specializationFingerprint =
      process.getSpecializationFingerprint().str();
  result.entryPc = process.getEntryPc().str();
  result.fairnessWork = process.getFairnessCap();

  for (auto [index, capture] : llvm::enumerate(process.getCaptures())) {
    auto name = mlir::cast<mlir::StringAttr>(process.getCaptureNames()[index]);
    result.captures.push_back(
        {name.getValue().str(),
         valueName(moduleValues, capture, nextModuleValue),
         printType(capture.getType())});
  }
  for (mlir::Attribute attribute : process.getLiveSlots()) {
    auto slot = mlir::cast<mlir::DictionaryAttr>(attribute);
    result.liveSlots.push_back(
        {stringField(slot, "name"),
         printType(slot.getAs<mlir::TypeAttr>("type").getValue())});
  }

  for (auto [stateIndex, region] : llvm::enumerate(process.getStates())) {
    PcStatePlan state;
    state.ordinal = static_cast<uint32_t>(stateIndex);
    state.name =
        mlir::cast<mlir::FlatSymbolRefAttr>(process.getPcs()[stateIndex])
            .getValue()
            .str();
    llvm::DenseMap<mlir::Value, std::string> values;
    uint32_t nextValue = 0;
    llvm::DenseMap<mlir::Block *, uint32_t> blockOrdinals;
    for (auto [blockIndex, block] : llvm::enumerate(region))
      blockOrdinals.try_emplace(&block, static_cast<uint32_t>(blockIndex));
    for (mlir::Block &block : region) {
      PcBlockPlan blockPlan;
      blockPlan.ordinal = blockOrdinals.lookup(&block);
      for (auto [index, argument] : llvm::enumerate(block.getArguments())) {
        std::string name =
            blockPlan.ordinal == 0
                ? (llvm::Twine("arg") + llvm::Twine(index)).str()
                : (llvm::Twine("block") + llvm::Twine(blockPlan.ordinal) +
                   "_value" + llvm::Twine(index))
                      .str();
        values.try_emplace(argument, std::move(name));
      }
      for (mlir::BlockArgument argument : block.getArguments())
        blockPlan.arguments.push_back({valueName(values, argument, nextValue),
                                       printType(argument.getType())});
      for (mlir::Operation &operation : block) {
        for (mlir::Value value : operation.getResults())
          valueName(values, value, nextValue);
        if (auto load = mlir::dyn_cast<acsim::LiveLoadOp>(operation)) {
          state.operations.push_back(LiveLoadPlan{
              valueName(values, load.getResult(), nextValue),
              load.getSlot().str(), printType(load.getResult().getType())});
          blockPlan.operations.push_back(state.operations.back());
        } else if (auto store = mlir::dyn_cast<acsim::LiveStoreOp>(operation)) {
          state.operations.push_back(
              LiveStorePlan{valueName(values, store.getValue(), nextValue),
                            store.getSlot().str()});
          blockPlan.operations.push_back(state.operations.back());
        } else if (auto call = mlir::dyn_cast<acsim::InlineOp>(operation)) {
          state.operations.push_back(
              InlineCallPlan{call.getCallee().str(),
                             valueNames(values, call.getArgs(), nextValue),
                             {valueName(values, call.getResult(), nextValue)},
                             {printType(call.getResult().getType())}});
          blockPlan.operations.push_back(state.operations.back());
        } else if (auto call = mlir::dyn_cast<acsim::InvokeOp>(operation)) {
          state.operations.push_back(
              InvokePlan{call.getCallee().str(),
                         valueNames(values, call.getArgs(), nextValue),
                         valueNames(values, call.getResults(), nextValue),
                         resultTypeNames(call.getResults())});
          blockPlan.operations.push_back(state.operations.back());
        } else if (auto branch =
                       mlir::dyn_cast<mlir::cf::BranchOp>(operation)) {
          blockPlan.terminator = BranchPlan{
              blockOrdinals.lookup(branch.getDest()),
              valueNames(values, branch.getDestOperands(), nextValue)};
        } else if (auto branch =
                       mlir::dyn_cast<mlir::cf::CondBranchOp>(operation)) {
          blockPlan.terminator = ConditionalBranchPlan{
              valueName(values, branch.getCondition(), nextValue),
              blockOrdinals.lookup(branch.getTrueDest()),
              valueNames(values, branch.getTrueDestOperands(), nextValue),
              blockOrdinals.lookup(branch.getFalseDest()),
              valueNames(values, branch.getFalseDestOperands(), nextValue)};
        } else if (auto transition =
                       mlir::dyn_cast<acsim::ContinueOp>(operation)) {
          ContinuePlan plan{transition.getTargetPc().str()};
          state.terminator = plan;
          blockPlan.terminator = std::move(plan);
        } else if (auto suspend = mlir::dyn_cast<acsim::SuspendOp>(operation)) {
          SuspendPlan plan{valueName(values, suspend.getWake(), nextValue),
                           suspend.getTargetPc().str()};
          state.terminator = plan;
          blockPlan.terminator = std::move(plan);
        } else if (auto terminate =
                       mlir::dyn_cast<acsim::TerminateOp>(operation)) {
          TerminatePlan plan{terminate.getStatus().str()};
          state.terminator = plan;
          blockPlan.terminator = std::move(plan);
        } else if (auto constant =
                       mlir::dyn_cast<mlir::arith::ConstantOp>(operation)) {
          auto value = staticValue(constant.getValue());
          if (!value)
            return value.takeError();
          state.operations.push_back(ConstantPlan{
              valueName(values, constant.getResult(), nextValue),
              printType(constant.getResult().getType()), std::move(*value)});
          blockPlan.operations.push_back(state.operations.back());
        } else if (auto constant =
                       mlir::dyn_cast<mlir::index::ConstantOp>(operation)) {
          if (!constant.getValue().isSignedIntN(64))
            return detailError("ACLOWER-PROCESS-STATE",
                               "index constant exceeds int64_t");
          state.operations.push_back(ConstantPlan{
              valueName(values, constant.getResult(), nextValue),
              printType(constant.getResult().getType()),
              llvm::json::Value(constant.getValue().getSExtValue())});
          blockPlan.operations.push_back(state.operations.back());
        } else if (operation.getName().getStringRef().starts_with("arith.")) {
          std::string predicate;
          if (auto compare = mlir::dyn_cast<mlir::arith::CmpIOp>(operation))
            predicate =
                mlir::arith::stringifyCmpIPredicate(compare.getPredicate())
                    .str();
          else if (auto compare =
                       mlir::dyn_cast<mlir::arith::CmpFOp>(operation))
            predicate =
                mlir::arith::stringifyCmpFPredicate(compare.getPredicate())
                    .str();
          state.operations.push_back(ArithmeticPlan{
              operation.getName().getStringRef().str(),
              valueNames(values, operation.getOperands(), nextValue),
              valueNames(values, operation.getResults(), nextValue),
              resultTypeNames(operation.getResults()), std::move(predicate)});
          blockPlan.operations.push_back(state.operations.back());
        } else if (operation.getName().getStringRef().starts_with("index.")) {
          std::string predicate;
          if (auto compare = mlir::dyn_cast<mlir::index::CmpOp>(operation))
            predicate =
                mlir::index::stringifyIndexCmpPredicate(compare.getPred())
                    .str();
          state.operations.push_back(IndexPlan{
              operation.getName().getStringRef().str(),
              valueNames(values, operation.getOperands(), nextValue),
              valueNames(values, operation.getResults(), nextValue),
              resultTypeNames(operation.getResults()), std::move(predicate)});
          blockPlan.operations.push_back(state.operations.back());
        } else {
          return detailError("ACLOWER-PROCESS-STATE",
                             "process operation is outside the closed C++ "
                             "generation subset");
        }
      }
      state.blocks.push_back(std::move(blockPlan));
    }
    result.states.push_back(std::move(state));
  }
  return result;
}

llvm::Expected<ModulePlan>
extractModule(acsim::ModuleOp module,
              const llvm::DenseSet<llvm::StringRef> &moduleSymbols) {
  ModulePlan result;
  result.symbol = module.getSymName().str();
  result.className =
      generatedClassName(result.symbol, module.getSpecializationFingerprint());
  result.specializationFingerprint =
      module.getSpecializationFingerprint().str();
  llvm::DenseMap<mlir::Value, std::string> values;
  uint32_t nextValue = 0;

  for (mlir::Operation &operation : module.getBody().front()) {
    for (mlir::Value value : operation.getResults())
      valueName(values, value, nextValue);
    if (auto instance = mlir::dyn_cast<acsim::InstanceOp>(operation)) {
      auto staticArgs = staticValues(instance.getStaticArgs());
      if (!staticArgs)
        return staticArgs.takeError();
      PlacementKind kind =
          moduleSymbols.contains(
              instance.getTarget().getRootReference().getValue())
              ? PlacementKind::GeneratedModule
              : PlacementKind::ExternalStateful;
      result.placements.push_back(
          {kind,
           instance.getSymName().str(),
           (instance.getSymName() + "_").str(),
           symbolRefString(instance.getTargetAttr()),
           valueName(values, instance.getResult(), nextValue),
           instance.getSpecializationFingerprint().str(),
           {},
           std::move(*staticArgs)});
    } else if (auto array = mlir::dyn_cast<acsim::ArrayOp>(operation)) {
      auto staticArgs = staticValues(array.getStaticArgs());
      if (!staticArgs)
        return staticArgs.takeError();
      PlacementPlan placement{PlacementKind::HomogeneousArray,
                              array.getSymName().str(),
                              (array.getSymName() + "_").str(),
                              symbolRefString(array.getTargetAttr()),
                              valueName(values, array.getResult(), nextValue),
                              array.getSpecializationFingerprint().str(),
                              {},
                              std::move(*staticArgs)};
      for (int64_t extent : array.getShape())
        placement.shape.push_back(static_cast<uint64_t>(extent));
      result.placements.push_back(std::move(placement));
    } else if (auto element = mlir::dyn_cast<acsim::ElementOp>(operation)) {
      ProjectionPlan projection;
      projection.kind = ProjectionKind::Element;
      projection.resultValue =
          valueName(values, element.getResult(), nextValue);
      projection.baseValue = valueName(values, element.getArray(), nextValue);
      projection.resultType = printType(element.getResult().getType());
      for (int64_t index : element.getIndices())
        projection.indices.push_back(static_cast<uint64_t>(index));
      result.projections.push_back(std::move(projection));
    } else if (auto port = mlir::dyn_cast<acsim::PortOp>(operation)) {
      result.projections.push_back(
          {ProjectionKind::Port,
           valueName(values, port.getResult(), nextValue),
           valueName(values, port.getBase(), nextValue),
           {},
           port.getAccessor().str(),
           printType(port.getResult().getType())});
    } else if (auto resource = mlir::dyn_cast<acsim::ResourceOp>(operation)) {
      result.projections.push_back(
          {ProjectionKind::Resource,
           valueName(values, resource.getResult(), nextValue),
           valueName(values, resource.getBase(), nextValue),
           {},
           resource.getAccessor().str(),
           printType(resource.getResult().getType())});
    } else if (auto bind = mlir::dyn_cast<acsim::BindOp>(operation)) {
      result.binds.push_back({valueName(values, bind.getSource(), nextValue),
                              valueName(values, bind.getTarget(), nextValue),
                              bind.getKind().str()});
    } else if (auto expression = mlir::dyn_cast<acsim::InlineOp>(operation)) {
      result.expressions.push_back(
          {valueName(values, expression.getResult(), nextValue),
           expression.getCallee().str(),
           valueNames(values, expression.getArgs(), nextValue),
           printType(expression.getResult().getType())});
    } else if (auto process = mlir::dyn_cast<acsim::ProcessOp>(operation)) {
      auto extracted = extractProcess(process, values, nextValue);
      if (!extracted)
        return extracted.takeError();
      result.processes.push_back(std::move(*extracted));
    } else if (auto exportOp = mlir::dyn_cast<acsim::ExportOp>(operation)) {
      result.exports.push_back(
          {exportOp.getSymName().str(),
           valueName(values, exportOp.getValue(), nextValue),
           valueName(values, exportOp.getResult(), nextValue),
           exportOp.getRole().str(),
           printType(exportOp.getResult().getType())});
    } else if (auto returnOp = mlir::dyn_cast<acsim::ReturnOp>(operation)) {
      result.returnValues =
          valueNames(values, returnOp.getOperands(), nextValue);
    }
  }
  return result;
}

} // namespace

llvm::Error populateModelDetails(acsim::ModelOp model, ModelPlan &plan) {
  llvm::DenseSet<llvm::StringRef> moduleSymbols;
  for (mlir::Operation &operation : model.getBody().front())
    if (auto module = mlir::dyn_cast<acsim::ModuleOp>(operation))
      moduleSymbols.insert(module.getSymName());

  for (mlir::Operation &operation : model.getBody().front()) {
    if (auto binding = mlir::dyn_cast<acsim::BindingOp>(operation)) {
      auto extracted = extractBinding(binding);
      if (!extracted)
        return extracted.takeError();
      plan.bindings.push_back(std::move(*extracted));
    } else if (auto module = mlir::dyn_cast<acsim::ModuleOp>(operation)) {
      auto extracted = extractModule(module, moduleSymbols);
      if (!extracted)
        return extracted.takeError();
      plan.modules.push_back(std::move(*extracted));
    }
  }
  std::sort(plan.bindings.begin(), plan.bindings.end(),
            [](const BindingPlan &left, const BindingPlan &right) {
              return left.symbol < right.symbol;
            });
  std::sort(plan.modules.begin(), plan.modules.end(),
            [](const ModulePlan &left, const ModulePlan &right) {
              return left.symbol < right.symbol;
            });
  return llvm::Error::success();
}

llvm::Error validateModelDetails(const ModelPlan &plan) {
  llvm::StringRef prior;
  for (const BindingPlan &binding : plan.bindings) {
    if (binding.symbol.empty() || (!prior.empty() && prior >= binding.symbol) ||
        binding.header.empty() || binding.cppSymbol.empty() ||
        binding.conceptName.empty() ||
        !isValidFingerprint(binding.recordFingerprint))
      return detailError("ACLOWER-BINDING-MISSING",
                         "binding plan is incomplete or non-canonical");
    prior = binding.symbol;
  }
  prior = {};
  for (const ModulePlan &module : plan.modules) {
    if (module.symbol.empty() || module.className.empty() ||
        (!prior.empty() && prior >= module.symbol) ||
        !isValidFingerprint(module.specializationFingerprint))
      return detailError("ACLOWER-OWNERSHIP",
                         "module plan is incomplete or non-canonical");
    prior = module.symbol;
    for (const ProcessPlan &process : module.processes) {
      if (process.symbol.empty() || process.className.empty() ||
          process.fairnessWork == 0 || process.states.empty() ||
          !isValidFingerprint(process.specializationFingerprint))
        return detailError("ACLOWER-PROCESS-STATE",
                           "process plan is incomplete");
      std::set<std::string> pcs;
      for (auto [ordinal, state] : llvm::enumerate(process.states)) {
        if (state.ordinal != ordinal || state.name.empty() ||
            !pcs.insert(state.name).second)
          return detailError("ACLOWER-PROCESS-STATE",
                             "process PCs are not dense and unique");
      }
      if (!pcs.contains(process.entryPc))
        return detailError("ACLOWER-PROCESS-STATE",
                           "entry PC is outside the closed PC set");
      for (const PcStatePlan &state : process.states) {
        llvm::Error transitionError = std::visit(
            [&](const auto &terminator) -> llvm::Error {
              using Terminator = std::decay_t<decltype(terminator)>;
              if constexpr (std::is_same_v<Terminator, ContinuePlan>) {
                if (!pcs.contains(terminator.targetPc))
                  return detailError("ACLOWER-PROCESS-STATE",
                                     "continue target is outside the PC set");
              } else if constexpr (std::is_same_v<Terminator, SuspendPlan>) {
                if (terminator.wakeValue.empty() ||
                    !pcs.contains(terminator.targetPc))
                  return detailError("ACLOWER-PROCESS-STATE",
                                     "suspend wake or target is invalid");
              } else if (terminator.status != "success" &&
                         terminator.status != "failure") {
                return detailError("ACLOWER-PROCESS-STATE",
                                   "terminate status is not closed");
              }
              return llvm::Error::success();
            },
            state.terminator);
        if (transitionError)
          return transitionError;
        for (auto [blockOrdinal, block] : llvm::enumerate(state.blocks)) {
          if (block.ordinal != blockOrdinal)
            return detailError("ACLOWER-PROCESS-STATE",
                               "PC blocks are not dense and ordered");
          auto checkSuccessor =
              [&](uint32_t target,
                  const std::vector<std::string> &arguments) -> llvm::Error {
            if (target >= state.blocks.size())
              return detailError("ACLOWER-PROCESS-STATE",
                                 "branch target is outside its PC");
            if (arguments.size() != state.blocks[target].arguments.size())
              return detailError("ACLOWER-PROCESS-STATE",
                                 "branch successor arity is inconsistent");
            return llvm::Error::success();
          };
          llvm::Error blockError = std::visit(
              Overloaded{
                  [&](const BranchPlan &branch) -> llvm::Error {
                    return checkSuccessor(branch.targetBlock, branch.arguments);
                  },
                  [&](const ConditionalBranchPlan &branch) -> llvm::Error {
                    if (branch.condition.empty())
                      return detailError("ACLOWER-PROCESS-STATE",
                                         "conditional branch has no condition");
                    if (auto error = checkSuccessor(branch.trueBlock,
                                                    branch.trueArguments))
                      return error;
                    return checkSuccessor(branch.falseBlock,
                                          branch.falseArguments);
                  },
                  [&](const ContinuePlan &next) -> llvm::Error {
                    return pcs.contains(next.targetPc)
                               ? llvm::Error::success()
                               : detailError(
                                     "ACLOWER-PROCESS-STATE",
                                     "continue target is outside PC set");
                  },
                  [&](const SuspendPlan &suspend) -> llvm::Error {
                    return !suspend.wakeValue.empty() &&
                                   pcs.contains(suspend.targetPc)
                               ? llvm::Error::success()
                               : detailError("ACLOWER-PROCESS-STATE",
                                             "suspend target is invalid");
                  },
                  [&](const TerminatePlan &terminate) -> llvm::Error {
                    return terminate.status == "success" ||
                                   terminate.status == "failure"
                               ? llvm::Error::success()
                               : detailError("ACLOWER-PROCESS-STATE",
                                             "terminate status is not closed");
                  }},
              block.terminator);
          if (blockError)
            return std::move(blockError);
        }
      }
    }
  }
  return llvm::Error::success();
}

} // namespace acir::codegen::detail
