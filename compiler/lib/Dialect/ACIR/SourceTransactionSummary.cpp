#include "SourceTransactionSummary.h"
#include "ACIRSourceContracts.h"
#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSet.h"
#include <algorithm>
#include <set>

using namespace mlir;
namespace acir::ac {
namespace detail {
FailureOr<Value>
getSourcePredicateIdentityInput(Value value, const HardwareAnalysis &hardware) {
  if (!value)
    return Value();
  auto extract = value.getDefiningOp<BitsExtractOp>();
  if (!extract || extract->getNumOperands() != 1 ||
      extract->getNumResults() != 1)
    return Value();
  auto input = dyn_cast<BitsType>(extract.getInput().getType());
  auto output = dyn_cast<BitsType>(value.getType());
  auto low = extract->getAttrOfType<StaticExprAttr>("low");
  if (!input || !output || !low)
    return Value();
  HardwareBindings bindings;
  bindings.owner = extract->getParentOfType<ModuleOp>();
  if (!hardware.isStaticEvaluable(input.getWidth(), bindings) ||
      !hardware.isStaticEvaluable(output.getWidth(), bindings))
    return Value();
  auto inputWidth = hardware.getPackedWidth(input, bindings, extract);
  auto outputWidth = hardware.getPackedWidth(output, bindings, extract);
  if (failed(inputWidth) || failed(outputWidth))
    return failure();
  if (*inputWidth != 1 || *outputWidth != 1 ||
      !hardware.isStaticEvaluable(low, bindings))
    return Value();
  auto evaluated = hardware.evaluateStatic(low, bindings, extract);
  if (failed(evaluated))
    return failure();
  auto integer = dyn_cast<MathIntAttr>(*evaluated);
  if (!integer || integer.getCanonicalValue() != "0")
    return Value();
  return extract.getInput();
}

FailureOr<BoolAttr>
getSourceLiteralPredicate(Value value, const HardwareAnalysis &hardware) {
  if (!value)
    return BoolAttr();
  llvm::DenseSet<Value> seen;
  for (unsigned depth = 0;; ++depth) {
    if (depth > 128)
      return mlir::emitError(UnknownLoc::get(value.getContext()))
             << "source predicate identity depth budget exceeded";
    if (!seen.insert(value).second)
      return mlir::emitError(UnknownLoc::get(value.getContext()))
             << "cyclic source predicate identity";
    auto input = getSourcePredicateIdentityInput(value, hardware);
    if (failed(input))
      return failure();
    if (!*input)
      break;
    value = *input;
  }
  auto literal = value.getDefiningOp<BitsConstantOp>();
  if (!literal)
    return BoolAttr();
  HardwareBindings bindings;
  bindings.owner = literal->getParentOfType<ModuleOp>();
  if (!hardware.isStaticEvaluable(cast<BitsType>(value.getType()).getWidth(),
                                  bindings) ||
      !hardware.isStaticEvaluable(literal.getValue(), bindings))
    return BoolAttr();
  auto width = hardware.getPackedWidth(value.getType(), bindings, literal);
  if (failed(width) || *width != 1)
    return BoolAttr();
  auto evaluated =
      hardware.evaluateStatic(literal.getValue(), bindings, literal);
  if (failed(evaluated))
    return failure();
  if (auto boolean = dyn_cast<BoolAttr>(*evaluated))
    return boolean;
  if (auto integer = dyn_cast<MathIntAttr>(*evaluated)) {
    if (integer.getCanonicalValue() == "0")
      return BoolAttr::get(value.getContext(), false);
    if (integer.getCanonicalValue() == "1")
      return BoolAttr::get(value.getContext(), true);
  }
  return BoolAttr();
}
} // namespace detail
namespace {

// Publication is a separate bounded request. Neither exhaustion nor an
// unrepresentable domain changes successful source Method totality facts.
class PublicationBudget {
public:
  explicit PublicationBudget(Operation *site) : site(site) {}
  bool charge(uint64_t units = 1) {
    if (exhausted)
      return false;
    if (units > maxWork - work) {
      site->emitOpError()
          << "transaction publication summary work budget exceeded";
      exhausted = true;
      return false;
    }
    work += units;
    return true;
  }
  bool depth(unsigned depth) {
    if (exhausted)
      return false;
    if (depth > maxDepth) {
      site->emitOpError() << "transaction publication summary depth exceeded";
      exhausted = true;
      return false;
    }
    return charge();
  }
  bool ok() const { return !exhausted; }

private:
  static constexpr uint64_t maxWork = 1024 * 1024;
  static constexpr unsigned maxDepth = 128;
  Operation *site;
  uint64_t work = 0;
  bool exhausted = false;
};

struct ValueScope {
  SmallVector<Type> arguments, captures;
};

class PublicationBuilder {
public:
  explicit PublicationBuilder(ModuleOp definition)
      : definition(definition),
        hardware(definition->getParentOfType<mlir::ModuleOp>()),
        builder(definition.getContext()), budget(definition) {}
  FailureOr<DictionaryAttr> run() {
    if (failed(checkInstances(definition, 0)))
      return failure();
    SmallVector<Attribute> owners, methods, autonomous;
    for (Operation &operation : definition.getBody().front()) {
      if (!budget.charge(1 + operation.getNumOperands() +
                         operation.getNumResults()))
        return failure();
      if (auto state = dyn_cast<SourceStateOp>(operation)) {
        auto domain = getDomain(state.getClk(), state.getRst());
        if (failed(domain))
          return failure();
        owners.push_back(dict({{"kind", str("state")},
                               {"name", state.getInstanceNameAttr()},
                               {"state", stateRef(state)},
                               {"type", TypeAttr::get(state.getQ().getType())},
                               {"domain", *domain}}));
      } else if (auto instance = dyn_cast<InstanceOp>(operation)) {
        if (failed(hardware.verifyInstance(instance)))
          return failure();
        auto arguments = values(instance.getInputs(), nullptr);
        if (failed(arguments))
          return failure();
        owners.push_back(
            dict({{"kind", str("instance")},
                  {"name", instance.getInstanceNameAttr()},
                  {"occurrence", instance.getOccurrence()},
                  {"callee", instance.getCalleeAttr()},
                  {"parameters", instance.getParameters()},
                  {"type_arguments", instance.getTypeArguments()},
                  {"input_types", types(instance.getInputs().getTypes())},
                  {"arguments", *arguments},
                  {"result_types", types(instance.getResultTypes())}}));
      } else if (auto method = dyn_cast<SourceMethodOp>(operation)) {
        auto row = callable(method);
        if (failed(row))
          return failure();
        methods.push_back(*row);
      } else if (auto transaction = dyn_cast<SourceTransactionOp>(operation)) {
        auto row = callable(transaction);
        if (failed(row))
          return failure();
        autonomous.push_back(*row);
      } else if (!isa<YieldOp, RuleOp>(operation) &&
                 !detail::isCombinationalHardwareOperation(&operation)) {
        return operation.emitOpError()
               << "transaction publication summary cannot represent this "
                  "module operation";
      } else if (failed(scanPure(&operation, 0))) {
        return failure();
      }
    }
    auto ownerIdentity = [](Attribute raw) {
      auto row = cast<DictionaryAttr>(raw);
      return row.getAs<StringAttr>("kind").getValue() == "state"
                 ? row.getAs<DictionaryAttr>("state").get("declaration")
                 : row.get("occurrence");
    };
    llvm::sort(owners, [&](Attribute a, Attribute b) {
      return detail::compareClosedSourceStructure(ownerIdentity(a),
                                                  ownerIdentity(b)) < 0;
    });
    llvm::sort(methods, [](Attribute a, Attribute b) {
      return cast<DictionaryAttr>(a).getAs<StringAttr>("name").getValue() <
             cast<DictionaryAttr>(b).getAs<StringAttr>("name").getValue();
    });
    llvm::sort(autonomous, [](Attribute a, Attribute b) {
      return detail::compareClosedSourceStructure(
                 cast<DictionaryAttr>(a).get("registration"),
                 cast<DictionaryAttr>(b).get("registration")) < 0;
    });
    auto summary = dict({{"owners", array(owners)},
                         {"methods", array(methods)},
                         {"autonomous", array(autonomous)}});
    if (!budget.ok() ||
        failed(verifySourceTransactionSummary(definition, summary)))
      return failure();
    return summary;
  }

private:
  StringAttr str(StringRef value) { return builder.getStringAttr(value); }
  DictionaryAttr
  dict(std::initializer_list<std::pair<StringRef, Attribute>> fields) {
    budget.charge(1 + fields.size());
    NamedAttrList result;
    for (auto field : fields)
      result.append(field.first, field.second);
    return result.getDictionary(builder.getContext());
  }
  ArrayAttr array(ArrayRef<Attribute> attributes) {
    budget.charge(1 + attributes.size());
    return builder.getArrayAttr(attributes);
  }
  ArrayAttr types(TypeRange inputs) {
    SmallVector<Attribute> result;
    for (Type type : inputs) {
      if (!budget.charge())
        break;
      result.push_back(TypeAttr::get(type));
    }
    return array(result);
  }
  DictionaryAttr unknown() { return dict({{"kind", str("unknown")}}); }
  DictionaryAttr stateRef(SourceStateOp state) {
    return dict({{"kind", str("owned")},
                 {"declaration", state.getOccurrence()},
                 {"element", array({})}});
  }
  bool oneBit(Type type) {
    auto bits = dyn_cast<BitsType>(type);
    if (!bits)
      return false;
    HardwareBindings bindings;
    bindings.owner = definition;
    if (!hardware.isStaticEvaluable(bits.getWidth(), bindings))
      return false;
    auto width = hardware.getPackedWidth(type, bindings, definition);
    return succeeded(width) && *width == 1;
  }
  DictionaryAttr fact(Value value, Operation *scope, unsigned depth = 0) {
    if (!budget.depth(depth))
      return unknown();
    auto input = detail::getSourcePredicateIdentityInput(value, hardware);
    if (failed(input)) {
      invalid = true;
      return unknown();
    }
    if (*input)
      return fact(*input, scope, depth + 1);
    if (!oneBit(value.getType()))
      return unknown();
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      if (!scope && argument.getOwner()->getParentOp() == definition)
        return dict(
            {{"kind", str("argument")},
             {"ordinal", builder.getI64IntegerAttr(argument.getArgNumber())}});
      if (argument.getOwner()->getParentOp() != scope)
        return unknown();
      unsigned index = argument.getArgNumber(), formals = 0;
      ValueRange captures;
      if (auto method = dyn_cast_or_null<SourceMethodOp>(scope)) {
        formals = method.getFunctionType().getNumInputs();
        captures = method.getCaptures();
      } else if (auto transaction =
                     dyn_cast_or_null<SourceTransactionOp>(scope)) {
        captures = transaction.getCaptures();
      }
      if (index < formals)
        return dict({{"kind", str("argument")},
                     {"ordinal", builder.getI64IntegerAttr(index)}});
      if (scope && index - formals < captures.size()) {
        auto lifted = fact(captures[index - formals], nullptr, depth + 1);
        if (lifted.getAs<StringAttr>("kind").getValue() == "constant")
          return lifted;
      }
      return unknown();
    }
    auto constant = detail::getSourceLiteralPredicate(value, hardware);
    if (failed(constant)) {
      invalid = true;
      return unknown();
    }
    if (*constant)
      return dict({{"kind", str("constant")}, {"value", *constant}});
    return unknown();
  }
  DictionaryAttr dependencies(Value value, Operation *scope,
                              unsigned depth = 0) {
    if (!budget.depth(depth))
      return unknown();
    auto key = std::make_pair(value, scope);
    auto found = dependencyCache.find(key);
    if (found != dependencyCache.end())
      return found->second;
    std::set<uint64_t> arguments, captures;
    bool known = true;
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      auto owner = argument.getOwner()->getParentOp();
      if (!scope && owner == definition) {
        arguments.insert(argument.getArgNumber());
      } else if (scope && owner == scope) {
        unsigned formals = 0;
        if (auto method = dyn_cast<SourceMethodOp>(scope))
          formals = method.getFunctionType().getNumInputs();
        unsigned ordinal = argument.getArgNumber();
        if (ordinal < formals)
          arguments.insert(ordinal);
        else
          captures.insert(ordinal - formals);
      } else
        known = false;
    } else {
      auto *operation = value.getDefiningOp();
      if (!operation || !detail::isCombinationalHardwareOperation(operation) ||
          operation->getNumRegions()) {
        known = false;
      } else {
        for (Value operand : operation->getOperands()) {
          auto dependency = dependencies(operand, scope, depth + 1);
          if (dependency.getAs<StringAttr>("kind").getValue() == "unknown") {
            known = false;
            break;
          }
          for (Attribute raw : dependency.getAs<ArrayAttr>("arguments")) {
            if (!budget.charge())
              return unknown();
            arguments.insert(cast<IntegerAttr>(raw).getInt());
          }
          for (Attribute raw : dependency.getAs<ArrayAttr>("captures")) {
            if (!budget.charge())
              return unknown();
            captures.insert(cast<IntegerAttr>(raw).getInt());
          }
        }
      }
    }
    if (!known)
      return dependencyCache[key] = unknown();
    auto ordinals = [&](const std::set<uint64_t> &set) {
      SmallVector<Attribute> result;
      for (uint64_t ordinal : set) {
        if (!budget.charge())
          break;
        result.push_back(builder.getI64IntegerAttr(ordinal));
      }
      return array(result);
    };
    return dependencyCache[key] = dict({{"kind", str("known")},
                                        {"arguments", ordinals(arguments)},
                                        {"captures", ordinals(captures)}});
  }
  DictionaryAttr value(Value value, Operation *scope) {
    return dict({{"fact", fact(value, scope)},
                 {"dependencies", dependencies(value, scope)}});
  }
  FailureOr<ArrayAttr> values(ValueRange inputs, Operation *scope) {
    SmallVector<Attribute> result;
    for (Value input : inputs) {
      result.push_back(value(input, scope));
      if (invalid || !budget.ok())
        return failure();
    }
    if (invalid || !budget.ok())
      return failure();
    return array(result);
  }
  FailureOr<DictionaryAttr> endpoint(Value input) {
    if (!budget.charge())
      return failure();
    if (auto argument = dyn_cast<BlockArgument>(input)) {
      if (argument.getOwner()->getParentOp() == definition &&
          oneBit(input.getType()))
        return dict(
            {{"kind", str("input")},
             {"ordinal", builder.getI64IntegerAttr(argument.getArgNumber())}});
    }
    // Domain publication retains direct physical endpoints. Predicate identity
    // forwarding must not turn an extract into a direct literal domain wire.
    if (!input.getDefiningOp<BitsConstantOp>())
      return definition.emitOpError()
             << "transaction publication domain endpoint requires a physical "
                "one-bit module input or direct static literal";
    auto literal = detail::getSourceLiteralPredicate(input, hardware);
    if (failed(literal))
      return failure();
    if (*literal)
      return dict({{"kind", str("constant")}, {"value", *literal}});
    return definition.emitOpError()
           << "transaction publication domain endpoint requires a physical "
              "one-bit module input or direct static literal";
  }
  FailureOr<DictionaryAttr> getDomain(Value clock, Value reset) {
    auto c = endpoint(clock), r = endpoint(reset);
    if (failed(c) || failed(r))
      return failure();
    return dict({{"clock", *c}, {"reset", *r}});
  }
  LogicalResult scanPure(Operation *operation, unsigned depth) {
    if (!budget.depth(depth) || !budget.charge(operation->getNumOperands() +
                                               operation->getNumResults()))
      return failure();
    for (Region &region : operation->getRegions())
      for (Block &block : region) {
        if (!budget.charge(block.getNumArguments()))
          return failure();
        for (Operation &nested : block) {
          if (!isa<YieldOp>(nested) &&
              !detail::isCombinationalHardwareOperation(&nested))
            return nested.emitOpError()
                   << "transaction publication cannot omit nested effects";
          if (failed(scanPure(&nested, depth + 1)))
            return failure();
        }
      }
    return success();
  }
  LogicalResult checkInstances(ModuleOp owner, unsigned depth) {
    if (!budget.depth(depth))
      return failure();
    if (visiting.contains(owner))
      return owner.emitOpError()
             << "recursive instance graph cannot be published";
    if (visited.contains(owner))
      return success();
    visiting.insert(owner);
    for (Operation &operation : owner.getBody().front()) {
      if (!budget.charge())
        return failure();
      auto instance = dyn_cast<InstanceOp>(operation);
      if (!instance)
        continue;
      auto *callee = hardware.resolveCallee(instance);
      if (!callee)
        return instance.emitOpError()
               << "publication requires a resolved instance callee";
      if (auto child = dyn_cast<ModuleOp>(callee))
        if (failed(checkInstances(child, depth + 1)))
          return failure();
    }
    visiting.erase(owner);
    visited.insert(owner);
    return success();
  }
  FailureOr<ArrayAttr> captures(ValueRange inputs) {
    SmallVector<Attribute> result;
    for (Value input : inputs) {
      result.push_back(dict({{"type", TypeAttr::get(input.getType())},
                             {"value", value(input, nullptr)}}));
      if (invalid || !budget.ok())
        return failure();
    }
    if (invalid || !budget.ok())
      return failure();
    return array(result);
  }
  FailureOr<ArrayAttr> selectors(ArrayAttr selectors, ValueRange indices,
                                 Operation *scope) {
    SmallVector<Attribute> result;
    for (Attribute raw : selectors) {
      auto selector = cast<DictionaryAttr>(raw);
      if (auto field = selector.getAs<StringAttr>("field"))
        result.push_back(dict({{"kind", str("field")}, {"name", field}}));
      else {
        auto ordinal = selector.getAs<IntegerAttr>("index").getInt();
        auto index = indices[ordinal];
        result.push_back(dict({{"kind", str("any_element")},
                               {"index_type", TypeAttr::get(index.getType())},
                               {"index", value(index, scope)}}));
      }
      if (invalid || !budget.ok())
        return failure();
    }
    return array(result);
  }
  FailureOr<DictionaryAttr> receiver(SourceMethodCallOp call) {
    auto resolved = resolveSourceMethodCall(call);
    if (failed(resolved))
      return failure();
    return resolved->receiver;
  }
  FailureOr<ArrayAttr> effects(Region &region, Operation *scope) {
    SmallVector<Attribute> result;
    for (Operation &operation : region.front().without_terminator()) {
      if (!budget.charge(1 + operation.getNumOperands() +
                         operation.getNumResults()))
        return failure();
      if (auto call = dyn_cast<SourceMethodCallOp>(operation)) {
        auto actuals = values(call.getArguments(), scope);
        auto owner = receiver(call);
        if (failed(actuals) || failed(owner))
          return failure();
        result.push_back(dict({{"kind", str("call")},
                               {"occurrence", call.getOccurrence()},
                               {"path", fact(call.getPath(), scope)},
                               {"callee", call.getCalleeAttr()},
                               {"receiver", *owner},
                               {"arguments", *actuals}}));
      } else if (isa<SourceStateReadOp, SourceStateWriteOp>(operation)) {
        auto stateName = operation.getAttrOfType<StringAttr>("state");
        SourceStateOp state;
        for (Operation &operation : definition.getBody().front()) {
          if (!budget.charge())
            return failure();
          auto candidate = dyn_cast<SourceStateOp>(operation);
          if (candidate && candidate.getInstanceNameAttr() == stateName)
            state = candidate;
        }
        if (!state)
          return operation.emitOpError()
                 << "publication requires an actual local state";
        bool write = isa<SourceStateWriteOp>(operation);
        ValueRange indices = operation.getOperands().drop_front(write ? 2 : 1);
        auto selected = selectors(
            operation.getAttrOfType<ArrayAttr>("selectors"), indices, scope);
        if (failed(selected))
          return failure();
        NamedAttrList row(dict({{"kind", str(write ? "write" : "read")},
                                {"occurrence", operation.getAttr("occurrence")},
                                {"path", fact(operation.getOperand(0), scope)},
                                {"state", stateRef(state)},
                                {"selectors", *selected}}));
        if (write)
          row.append("value", value(operation.getOperand(1), scope));
        result.push_back(row.getDictionary(builder.getContext()));
      } else if (!detail::isCombinationalHardwareOperation(&operation)) {
        return operation.emitOpError()
               << "transaction publication cannot omit this callable operation";
      } else if (failed(scanPure(&operation, 0)))
        return failure();
    }
    if (invalid || !budget.ok())
      return failure();
    return array(result);
  }
  FailureOr<DictionaryAttr> callable(Operation *operation) {
    ValueRange captured;
    auto method = dyn_cast<SourceMethodOp>(operation);
    if (method)
      captured = method.getCaptures();
    else
      captured = cast<SourceTransactionOp>(operation).getCaptures();
    auto captureRows = captures(captured);
    auto guardEffects = effects(operation->getRegion(0), operation);
    auto bodyEffects = effects(operation->getRegion(1), operation);
    auto guard = cast<YieldOp>(operation->getRegion(0).front().back());
    auto results = values(
        cast<YieldOp>(operation->getRegion(1).front().back()).getValues(),
        operation);
    if (failed(captureRows) || failed(guardEffects) || failed(bodyEffects) ||
        failed(results))
      return failure();
    NamedAttrList row(
        dict({{"captures", *captureRows},
              {"guard", value(guard.getValues().front(), operation)},
              {"guard_effects", *guardEffects},
              {"body_effects", *bodyEffects},
              {"results", *results}}));
    if (method) {
      row.append("name", method.getSymNameAttr());
      row.append("declaration", method.getOccurrence());
      row.append("function_type", method.getFunctionTypeAttr());
      row.append("argument_names", method.getArgumentNames());
      for (StringRef key :
           {"ac.parameters", "ac.result_constraints", "ac.return_form"})
        row.append(key, method->getAttr(key));
    } else {
      auto transaction = cast<SourceTransactionOp>(operation);
      auto domain = getDomain(transaction.getClk(), transaction.getRst());
      if (failed(domain))
        return failure();
      row.append("name", transaction.getNameAttr());
      row.append("declaration", transaction.getDeclaration());
      row.append("registration", transaction.getOccurrence());
      row.append("domain", *domain);
      row.append("result_types", types(transaction.getResultTypes()));
    }
    if (invalid || !budget.ok())
      return failure();
    budget.charge(row.getAttrs().size());
    return row.getDictionary(builder.getContext());
  }
  ModuleOp definition;
  HardwareAnalysis hardware;
  Builder builder;
  PublicationBudget budget;
  llvm::DenseSet<Operation *> visiting, visited;
  llvm::DenseMap<std::pair<Value, Operation *>, DictionaryAttr> dependencyCache;
  bool invalid = false;
};

class SummaryStructureVerifier {
public:
  explicit SummaryStructureVerifier(Operation *declaration)
      : declaration(declaration),
        hardware(declaration->getParentOfType<mlir::ModuleOp>()),
        budget(declaration) {}
  LogicalResult run(DictionaryAttr summary) {
    // Header validation deliberately never walks or verifies provider bodies.
    if (failed(preflight(summary, 0)))
      return failure();
    if (auto module = dyn_cast<ModuleOp>(declaration)) {
      if (failed(module.verifyInvariantsImpl()))
        return failure();
    } else if (auto imported = dyn_cast<ModuleImportOp>(declaration)) {
      if (failed(imported.verifyInvariantsImpl()))
        return failure();
    } else
      return error("summary owner must be ac.module or ac.module.import");
    if (failed(detail::verifyModuleSignature(declaration)))
      return failure();
    signature = cast<FunctionType>(
        declaration->getAttrOfType<TypeAttr>("function_type").getValue());
    moduleScope.arguments.assign(signature.getInputs().begin(),
                                 signature.getInputs().end());
    if (failed(exact(summary, {"owners", "methods", "autonomous"})))
      return failure();
    auto owners = summary.getAs<ArrayAttr>("owners");
    auto methods = summary.getAs<ArrayAttr>("methods");
    auto autonomous = summary.getAs<ArrayAttr>("autonomous");
    if (!owners || !methods || !autonomous)
      return error("top rows must be arrays");
    DictionaryAttr previousOwner;
    llvm::StringSet<> ownerNames;
    for (Attribute raw : owners) {
      auto row = dyn_cast<DictionaryAttr>(raw);
      auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
      if (!kind)
        return error("owner kind must be a string");
      auto name = row.getAs<StringAttr>("name");
      if (failed(nameIsIdentifier(name)) ||
          !ownerNames.insert(name.getValue()).second)
        return error("owner names must be unique");
      DictionaryAttr identity;
      if (kind.getValue() == "state") {
        if (failed(exact(row, {"kind", "name", "state", "type", "domain"})))
          return failure();
        auto state = row.getAs<DictionaryAttr>("state");
        if (failed(ownedState(state)) || failed(type(row.get("type"))) ||
            failed(domain(row.getAs<DictionaryAttr>("domain"))))
          return failure();
        identity = state.getAs<DictionaryAttr>("declaration");
        stateRows[state] = row.getAs<TypeAttr>("type").getValue();
      } else if (kind.getValue() == "instance") {
        if (failed(exact(row, {"kind", "name", "occurrence", "callee",
                               "parameters", "type_arguments", "input_types",
                               "arguments", "result_types"})))
          return failure();
        identity = row.getAs<DictionaryAttr>("occurrence");
        auto callee = row.getAs<FlatSymbolRefAttr>("callee");
        auto parameters = row.getAs<ArrayAttr>("parameters");
        auto typeArguments = row.getAs<ArrayAttr>("type_arguments");
        auto inputs = row.getAs<ArrayAttr>("input_types");
        auto results = row.getAs<ArrayAttr>("result_types");
        auto actuals = row.getAs<ArrayAttr>("arguments");
        if (!callee || callee.getValue().empty() || !parameters ||
            !typeArguments || !inputs || !results || !actuals ||
            inputs.size() != actuals.size())
          return error("instance binding fields or input arity are invalid");
        for (Attribute parameter : parameters)
          if (!isa<StaticExprAttr>(parameter) ||
              failed(
                  detail::verifyHardwareAttributeScope(parameter, declaration)))
            return error(
                "instance parameters require scoped static expressions");
        for (Attribute raw : typeArguments)
          if (failed(type(raw)))
            return failure();
        for (Attribute raw : results)
          if (failed(type(raw)))
            return failure();
        for (auto [input, actual] : llvm::zip(inputs, actuals)) {
          if (failed(type(input)) ||
              failed(
                  value(actual, moduleScope, cast<TypeAttr>(input).getValue())))
            return failure();
        }
        instanceRows[identity] = row;
      } else
        return error("unsupported owner kind");
      if (failed(occurrence(identity)))
        return failure();
      if (previousOwner &&
          detail::compareClosedSourceStructure(previousOwner, identity) >= 0)
        return error(
            "owner occurrences must be unique and canonically ordered");
      previousOwner = identity;
    }
    // Make all local signatures available before checking calls. External
    // signatures and suffix receivers belong to later explicit header closure.
    StringAttr previousMethod;
    for (Attribute raw : methods) {
      auto row = dyn_cast<DictionaryAttr>(raw);
      if (failed(methodHeader(row)))
        return failure();
      auto name = row.getAs<StringAttr>("name");
      if (previousMethod && previousMethod.getValue() >= name.getValue())
        return error("methods must be unique and canonically ordered by name");
      previousMethod = name;
      methodRows[name] = row;
    }
    for (Attribute raw : methods)
      if (failed(callable(cast<DictionaryAttr>(raw), true)))
        return failure();
    DictionaryAttr previousRegistration;
    llvm::DenseSet<Attribute> declarations;
    for (Attribute raw : autonomous) {
      auto row = dyn_cast<DictionaryAttr>(raw);
      if (failed(exact(row, {"name", "declaration", "registration", "domain",
                             "captures", "guard", "guard_effects",
                             "body_effects", "result_types", "results"})) ||
          failed(nameIsIdentifier(row.getAs<StringAttr>("name"))) ||
          failed(occurrence(row.getAs<DictionaryAttr>("declaration"))) ||
          failed(occurrence(row.getAs<DictionaryAttr>("registration"))) ||
          failed(domain(row.getAs<DictionaryAttr>("domain"))))
        return failure();
      auto registration = row.getAs<DictionaryAttr>("registration");
      if (previousRegistration && detail::compareClosedSourceStructure(
                                      previousRegistration, registration) >= 0)
        return error(
            "autonomous registrations must be unique and canonically ordered");
      previousRegistration = registration;
      if (!declarations.insert(row.get("declaration")).second)
        return error("autonomous declarations must be registered exactly once");
      if (failed(callable(row, false)))
        return failure();
    }
    return success(budget.ok());
  }

private:
  LogicalResult error(StringRef message) {
    return declaration->emitOpError() << "transaction summary " << message;
  }
  LogicalResult exact(DictionaryAttr row,
                      std::initializer_list<StringRef> fields) {
    if (!row || row.size() != fields.size())
      return error("has missing or extra fields");
    for (StringRef field : fields)
      if (!row.get(field))
        return error("has missing or extra fields");
    return success();
  }
  LogicalResult preflight(Attribute attr, unsigned depth) {
    if (!attr || !budget.depth(depth))
      return failure();
    if (auto dict = dyn_cast<DictionaryAttr>(attr)) {
      for (auto field : dict)
        if (failed(preflight(field.getValue(), depth + 1)))
          return failure();
    } else if (auto array = dyn_cast<ArrayAttr>(attr)) {
      for (Attribute raw : array)
        if (failed(preflight(raw, depth + 1)))
          return failure();
    } else if (auto expression = dyn_cast<StaticExprAttr>(attr)) {
      if (failed(preflight(expression.getTree(), depth + 1)))
        return failure();
      return StaticExprAttr::verify([&] { return declaration->emitOpError(); },
                                    expression.getTree());
    } else if (auto domain = dyn_cast<SourceDomainAttr>(attr)) {
      if (failed(preflight(domain.getValue(), depth + 1)))
        return failure();
      return SourceDomainAttr::verify(
          [&] { return declaration->emitOpError(); }, domain.getValue());
    } else if (auto type = dyn_cast<TypeAttr>(attr)) {
      return preflightType(type.getValue(), depth + 1);
    }
    return success();
  }
  LogicalResult preflightType(Type type, unsigned depth) {
    if (!budget.depth(depth))
      return failure();
    auto emit = [&] { return declaration->emitOpError(); };
    if (auto bits = dyn_cast<BitsType>(type)) {
      if (failed(preflight(bits.getWidth(), depth + 1)))
        return failure();
      return BitsType::verify(emit, bits.getWidth());
    }
    if (auto record = dyn_cast<StructType>(type))
      return StructType::verify(emit, record.getName());
    if (auto enumeration = dyn_cast<EnumType>(type))
      return EnumType::verify(emit, enumeration.getName());
    if (auto formal = dyn_cast<TypeParamType>(type))
      return TypeParamType::verify(emit, formal.getOwner(), formal.getName());
    if (auto table = dyn_cast<TableType>(type)) {
      if (failed(preflight(table.getShape(), depth + 1)))
        return failure();
      if (failed(preflightType(table.getElementType(), depth + 1)))
        return failure();
      return TableType::verify(emit, table.getShape(), table.getElementType());
    }
    if (auto function = dyn_cast<FunctionType>(type))
      for (TypeRange types : {function.getInputs(), function.getResults()})
        for (Type nested : types)
          if (failed(preflightType(nested, depth + 1)))
            return failure();
    return success();
  }
  LogicalResult nameIsIdentifier(StringAttr name) {
    if (!name)
      return error("name must be a string");
    return detail::verifyPythonAstIdentifier(
        name.getValue(), true, [&] { return declaration->emitOpError(); });
  }
  LogicalResult occurrence(DictionaryAttr attr,
                           FlatSymbolRefAttr expected = {}) {
    auto emit = [&] { return declaration->emitOpError(); };
    if (failed(detail::verifyOccurrence(attr, emit)))
      return failure();
    if (!expected)
      expected = FlatSymbolRefAttr::get(
          declaration->getContext(),
          declaration->getAttrOfType<StringAttr>("sym_name").getValue());
    return detail::verifyOriginDefinition(
        attr, expected, "transaction summary occurrence", emit);
  }
  LogicalResult type(Attribute raw) {
    auto attr = dyn_cast_or_null<TypeAttr>(raw);
    if (!attr || !isa<BitsType, StructType, EnumType, TypeParamType, TableType>(
                     attr.getValue()))
      return error("type must be a hardware TypeAttr");
    if (failed(detail::verifyHardwareTypeScope(attr.getValue(), declaration)))
      return failure();
    HardwareBindings bindings;
    bindings.owner = declaration;
    auto width =
        hardware.getPackedWidth(attr.getValue(), bindings, declaration);
    if (failed(width))
      return failure();
    if (!*width)
      return error("hardware type requires a positive resolved packed width");
    return success();
  }
  bool oneBit(Type type) {
    auto bits = dyn_cast_or_null<BitsType>(type);
    if (!bits)
      return false;
    HardwareBindings bindings;
    bindings.owner = declaration;
    if (!hardware.isStaticEvaluable(bits.getWidth(), bindings))
      return false;
    auto width = hardware.getPackedWidth(type, bindings, declaration);
    return succeeded(width) && *width == 1;
  }
  FailureOr<uint64_t> ordinal(Attribute raw, uint64_t size) {
    auto number = dyn_cast_or_null<IntegerAttr>(raw);
    if (!number || !number.getType().isSignlessInteger(64) ||
        number.getValue().isNegative() ||
        number.getValue().getZExtValue() >= size) {
      (void)error("ordinal requires nonnegative signless i64 in scope");
      return failure();
    }
    return number.getValue().getZExtValue();
  }
  LogicalResult fact(Attribute raw, const ValueScope &scope,
                     Type physical = {}) {
    auto row = dyn_cast_or_null<DictionaryAttr>(raw);
    auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
    if (!kind)
      return error("predicate fact kind must be a string");
    if (kind.getValue() == "unknown")
      return exact(row, {"kind"});
    // Call summaries carry no actual type. Without it the standalone verifier
    // checks fact shape/scope only; provider rederivation supplies authority.
    if (physical && !oneBit(physical))
      return error("known predicate facts require physical one-bit values");
    if (kind.getValue() == "constant") {
      if (failed(exact(row, {"kind", "value"})) ||
          !row.getAs<BoolAttr>("value"))
        return error("constant predicate fact requires BoolAttr");
      return success();
    }
    if (kind.getValue() == "argument") {
      if (failed(exact(row, {"kind", "ordinal"})))
        return failure();
      auto index = ordinal(row.get("ordinal"), scope.arguments.size());
      if (failed(index))
        return failure();
      if (!oneBit(scope.arguments[*index]))
        return error("predicate argument must be physical one-bit");
      return success();
    }
    return error("unsupported predicate fact kind");
  }
  LogicalResult dependencies(Attribute raw, const ValueScope &scope) {
    auto row = dyn_cast_or_null<DictionaryAttr>(raw);
    auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
    if (!kind)
      return error("dependency kind must be a string");
    if (kind.getValue() == "unknown")
      return exact(row, {"kind"});
    if (kind.getValue() != "known" ||
        failed(exact(row, {"kind", "arguments", "captures"})))
      return error("unsupported dependency fields or kind");
    for (auto [field, count] :
         {std::make_pair(StringRef("arguments"), scope.arguments.size()),
          std::make_pair(StringRef("captures"), scope.captures.size())}) {
      auto ordinals = row.getAs<ArrayAttr>(field);
      if (!ordinals)
        return error("known dependencies require ordinal arrays");
      std::optional<uint64_t> previous;
      for (Attribute raw : ordinals) {
        auto index = ordinal(raw, count);
        if (failed(index))
          return failure();
        if (previous && *previous >= *index)
          return error("dependency ordinals must be strictly increasing");
        previous = *index;
      }
    }
    return success();
  }
  LogicalResult value(Attribute raw, const ValueScope &scope,
                      Type physical = {}) {
    auto row = dyn_cast_or_null<DictionaryAttr>(raw);
    if (failed(exact(row, {"fact", "dependencies"})))
      return failure();
    if (failed(fact(row.get("fact"), scope, physical)))
      return failure();
    return dependencies(row.get("dependencies"), scope);
  }
  LogicalResult endpoint(Attribute raw) {
    auto row = dyn_cast_or_null<DictionaryAttr>(raw);
    auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
    if (!kind)
      return error("domain endpoint kind must be a string");
    if (kind.getValue() == "constant") {
      if (failed(exact(row, {"kind", "value"})) ||
          !row.getAs<BoolAttr>("value"))
        return error("constant domain endpoint requires BoolAttr");
      return success();
    }
    if (kind.getValue() == "input") {
      if (failed(exact(row, {"kind", "ordinal"})))
        return failure();
      auto index = ordinal(row.get("ordinal"), signature.getNumInputs());
      if (failed(index))
        return failure();
      if (!oneBit(signature.getInput(*index)))
        return error("domain input requires physical one-bit width");
      return success();
    }
    return error("domain endpoint must be input or constant");
  }
  LogicalResult domain(DictionaryAttr row) {
    if (failed(exact(row, {"clock", "reset"})))
      return failure();
    if (failed(endpoint(row.get("clock"))))
      return failure();
    return endpoint(row.get("reset"));
  }
  LogicalResult ownedState(DictionaryAttr state) {
    if (failed(detail::verifyStateRef(
            state, [&] { return declaration->emitOpError(); })))
      return failure();
    if (state.getAs<StringAttr>("kind").getValue() != "owned" ||
        !state.getAs<ArrayAttr>("element").empty())
      return error("state reference must identify a whole owned state");
    return occurrence(state.getAs<DictionaryAttr>("declaration"));
  }
  LogicalResult methodHeader(DictionaryAttr row) {
    if (failed(exact(row,
                     {"name", "declaration", "function_type", "argument_names",
                      "ac.parameters", "ac.result_constraints",
                      "ac.return_form", "captures", "guard", "guard_effects",
                      "body_effects", "results"})) ||
        failed(nameIsIdentifier(row.getAs<StringAttr>("name"))) ||
        failed(occurrence(row.getAs<DictionaryAttr>("declaration"))))
      return failure();
    auto attr = row.getAs<TypeAttr>("function_type");
    auto function =
        attr ? dyn_cast<FunctionType>(attr.getValue()) : FunctionType();
    auto names = row.getAs<ArrayAttr>("argument_names");
    auto parameters = row.getAs<ArrayAttr>("ac.parameters");
    auto constraints = row.getAs<ArrayAttr>("ac.result_constraints");
    auto form = row.getAs<StringAttr>("ac.return_form");
    if (!function || !names || !parameters || !constraints || !form ||
        names.size() != function.getNumInputs() ||
        parameters.size() != function.getNumInputs() ||
        constraints.size() != function.getNumResults() ||
        function.getNumResults() > 1 ||
        form.getValue() != (function.getNumResults() ? "single" : "none"))
      return error("method source signature fields disagree");
    llvm::StringSet<> argumentNames;
    for (auto [index, raw] : llvm::enumerate(names)) {
      auto name = dyn_cast<StringAttr>(raw);
      if (failed(nameIsIdentifier(name)) ||
          !argumentNames.insert(name.getValue()).second)
        return error("method argument names must be unique");
      if (failed(type(TypeAttr::get(function.getInput(index)))) ||
          failed(detail::verifyRequiredSourceParameter(
              parameters[index], name, declaration, declaration,
              "transaction summary method")))
        return failure();
      auto parameter = cast<DictionaryAttr>(parameters[index]);
      if (failed(detail::verifySourceCallHardwareConstraint(
              parameter.get("constraint"), function.getInput(index), hardware,
              declaration, declaration, "transaction summary method")))
        return failure();
    }
    for (auto [index, raw] : llvm::enumerate(constraints)) {
      if (failed(type(TypeAttr::get(function.getResult(index)))) ||
          failed(detail::verifySourceCallHardwareConstraint(
              raw, function.getResult(index), hardware, declaration,
              declaration, "transaction summary method")))
        return failure();
    }
    return success();
  }
  LogicalResult callable(DictionaryAttr row, bool method) {
    ValueScope scope;
    SmallVector<Type> outputs;
    if (method) {
      auto function =
          cast<FunctionType>(row.getAs<TypeAttr>("function_type").getValue());
      scope.arguments.assign(function.getInputs().begin(),
                             function.getInputs().end());
      outputs.assign(function.getResults().begin(),
                     function.getResults().end());
    } else {
      auto types = row.getAs<ArrayAttr>("result_types");
      if (!types)
        return error("autonomous result types must be an array");
      for (Attribute raw : types) {
        if (failed(type(raw)))
          return failure();
        outputs.push_back(cast<TypeAttr>(raw).getValue());
      }
    }
    auto captures = row.getAs<ArrayAttr>("captures");
    auto results = row.getAs<ArrayAttr>("results");
    auto guardEffects = row.getAs<ArrayAttr>("guard_effects");
    auto bodyEffects = row.getAs<ArrayAttr>("body_effects");
    if (!captures || !results || !guardEffects || !bodyEffects ||
        results.size() != outputs.size())
      return error("callable captures/effects/results must be arrays with "
                   "signature result arity");
    for (Attribute raw : captures) {
      auto capture = dyn_cast<DictionaryAttr>(raw);
      if (failed(exact(capture, {"type", "value"})) ||
          failed(type(capture.get("type"))))
        return failure();
      Type captureType = capture.getAs<TypeAttr>("type").getValue();
      if (failed(value(capture.get("value"), moduleScope, captureType)))
        return failure();
      scope.captures.push_back(captureType);
    }
    // Guard IR has no physical-type field in its summary: its admission
    // contract is one-bit. Check fact shape/scope and exact provider
    // derivation.
    if (failed(value(row.get("guard"), scope)))
      return failure();
    for (auto [raw, output] : llvm::zip(results, outputs))
      if (failed(value(raw, scope, output)))
        return failure();
    if (failed(effects(guardEffects, scope, true)))
      return failure();
    return effects(bodyEffects, scope, false);
  }
  FailureOr<Type> selectedType(DictionaryAttr state, ArrayAttr selectors,
                               const ValueScope &scope) {
    auto found = stateRows.find(state);
    if (found == stateRows.end()) {
      (void)error("effect state must name a local owner row");
      return failure();
    }
    HardwareBindings bindings;
    bindings.owner = declaration;
    auto resolved = hardware.resolveType(found->second, bindings, declaration);
    if (failed(resolved))
      return failure();
    Type current = *resolved;
    for (Attribute raw : selectors) {
      auto selector = dyn_cast<DictionaryAttr>(raw);
      auto kind = selector ? selector.getAs<StringAttr>("kind") : StringAttr();
      if (!kind) {
        (void)error("selector kind must be a string");
        return failure();
      }
      if (kind.getValue() == "field") {
        if (failed(exact(selector, {"kind", "name"})))
          return failure();
        auto name = selector.getAs<StringAttr>("name");
        auto structure = dyn_cast<StructType>(current);
        auto nominal =
            structure ? hardware.lookupStruct(structure) : StructOp();
        if (!name || !nominal) {
          (void)error("field selector requires a resolved nominal struct");
          return failure();
        }
        if (failed(nominal.verifyInvariantsImpl()) || failed(nominal.verify()))
          return failure();
        Type field;
        for (Attribute rawField : nominal.getFields()) {
          if (!budget.charge())
            return failure();
          auto item = cast<DictionaryAttr>(rawField);
          if (item.getAs<StringAttr>("name") == name)
            field = item.getAs<TypeAttr>("type").getValue();
        }
        if (!field) {
          (void)error("field selector does not name a nominal field");
          return failure();
        }
        auto resolvedField = hardware.resolveType(field, bindings, declaration);
        if (failed(resolvedField))
          return failure();
        current = *resolvedField;
      } else if (kind.getValue() == "any_element") {
        if (failed(exact(selector, {"kind", "index_type", "index"})) ||
            failed(type(selector.get("index_type"))))
          return failure();
        auto index = selector.getAs<TypeAttr>("index_type").getValue();
        auto table = dyn_cast<TableType>(current);
        if (!isa<BitsType>(index) || !table || table.getShape().empty()) {
          (void)error("wildcard selector requires a Bits index and Table axis");
          return failure();
        }
        if (failed(hardware.getPackedWidth(index, bindings, declaration)) ||
            failed(hardware.getTableShape(table, bindings, declaration)) ||
            failed(value(selector.get("index"), scope, index)))
          return failure();
        current =
            table.getShape().size() == 1
                ? table.getElementType()
                : TableType::get(
                      declaration->getContext(),
                      ArrayAttr::get(declaration->getContext(),
                                     table.getShape().getValue().drop_front()),
                      table.getElementType());
      } else {
        (void)error("unsupported selector kind");
        return failure();
      }
    }
    return current;
  }
  LogicalResult call(DictionaryAttr row, const ValueScope &scope) {
    if (failed(exact(row, {"kind", "occurrence", "path", "callee", "receiver",
                           "arguments"})))
      return failure();
    auto callee = row.getAs<SymbolRefAttr>("callee");
    auto receiver = row.getAs<DictionaryAttr>("receiver");
    auto actuals = row.getAs<ArrayAttr>("arguments");
    if (!callee || callee.getRootReference().getValue().empty() ||
        callee.getNestedReferences().size() != 1 || !actuals ||
        failed(detail::verifyOwnerRef(
            receiver, [&] { return declaration->emitOpError(); })))
      return error("call requires a qualified Module::Method, OwnerRef and "
                   "argument array");
    auto path = receiver.getAs<ArrayAttr>("instance_path");
    if (path.empty()) {
      if (callee.getRootReference() !=
          declaration->getAttrOfType<StringAttr>("sym_name"))
        return error("self call callee must name this module");
      auto found =
          methodRows.find(callee.getNestedReferences().front().getAttr());
      if (found == methodRows.end())
        return error("self call must name a local Method row");
      auto function = cast<FunctionType>(
          found->second.getAs<TypeAttr>("function_type").getValue());
      if (actuals.size() != function.getNumInputs())
        return error("self call argument arity must match Method signature");
      for (auto [actual, type] : llvm::zip(actuals, function.getInputs()))
        if (failed(value(actual, scope, type)))
          return failure();
    } else {
      auto first = dyn_cast<DictionaryAttr>(path[0]);
      auto found = instanceRows.find(first);
      if (found == instanceRows.end())
        return error(
            "receiver path must start at a local actual instance owner");
      auto firstCallee = found->second.getAs<FlatSymbolRefAttr>("callee");
      if (path.size() == 1 &&
          callee.getRootReference() != firstCallee.getAttr())
        return error(
            "callee module must match local receiver instance binding");
      if (path.size() > 1 &&
          failed(occurrence(cast<DictionaryAttr>(path[1]), firstCallee)))
        return failure();
      // Later occurrence existence, suffix binding, and external signature
      // arity/type are deliberately deferred to explicit header closure.
      for (Attribute actual : actuals)
        if (failed(value(actual, scope)))
          return failure();
    }
    return success();
  }
  LogicalResult effects(ArrayAttr rows, const ValueScope &scope, bool guard) {
    for (Attribute raw : rows) {
      auto row = dyn_cast<DictionaryAttr>(raw);
      auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
      if (!kind)
        return error("effect kind must be a string");
      if (failed(occurrence(row.getAs<DictionaryAttr>("occurrence"))))
        return failure();
      if (failed(fact(row.get("path"), scope)))
        return failure();
      if (kind.getValue() == "call") {
        if (guard)
          return error("guard effects permit reads only");
        if (failed(call(row, scope)))
          return failure();
      } else if (kind.getValue() == "read" || kind.getValue() == "write") {
        bool write = kind.getValue() == "write";
        if (guard && write)
          return error("guard effects permit reads only");
        if (write) {
          if (failed(exact(row, {"kind", "occurrence", "path", "state",
                                 "selectors", "value"})))
            return failure();
        } else if (failed(exact(row, {"kind", "occurrence", "path", "state",
                                      "selectors"})))
          return failure();
        auto state = row.getAs<DictionaryAttr>("state");
        auto selectors = row.getAs<ArrayAttr>("selectors");
        if (failed(ownedState(state)) || !selectors)
          return error("state effect requires a selector array");
        auto selected = selectedType(state, selectors, scope);
        if (failed(selected))
          return failure();
        if (write && failed(value(row.get("value"), scope, *selected)))
          return failure();
      } else
        return error("unsupported effect kind");
    }
    return success();
  }
  Operation *declaration;
  HardwareAnalysis hardware;
  PublicationBudget budget;
  FunctionType signature;
  ValueScope moduleScope;
  llvm::DenseMap<DictionaryAttr, Type> stateRows;
  llvm::DenseMap<DictionaryAttr, DictionaryAttr> instanceRows;
  llvm::DenseMap<StringAttr, DictionaryAttr> methodRows;
};
} // namespace

FailureOr<DictionaryAttr>
detail::deriveSourceTransactionSummary(ModuleOp definition) {
  return PublicationBuilder(definition).run();
}

LogicalResult verifySourceTransactionSummary(Operation *moduleDeclaration,
                                             DictionaryAttr summary) {
  if (!moduleDeclaration || !isa<ModuleOp, ModuleImportOp>(moduleDeclaration) ||
      !moduleDeclaration->getParentOfType<mlir::ModuleOp>()) {
    if (moduleDeclaration)
      moduleDeclaration->emitOpError()
          << "transaction summary requires a package module declaration";
    return failure();
  }
  return SummaryStructureVerifier(moduleDeclaration).run(summary);
}
} // namespace acir::ac
