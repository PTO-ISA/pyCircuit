#include "SourceTransactionSummary.h"
#include "ACIRSourceContracts.h"
#include "HardwareSourceChecks.h"
#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSet.h"
#include <algorithm>
#include <set>

#include <functional>

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
  bool storage(uint64_t count, uint64_t bytes) {
    uint64_t units = 1 + bytes / 8 + (bytes % 8 != 0);
    if (count > (maxWork - work) / units)
      return charge(maxWork + 1);
    return charge(count * units);
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

// Old readers require exactly the three legacy top-level fields. Version only
// summaries that use newer semantics, including relative calls nested in arms.
FailureOr<unsigned> requiredTransactionSchema(Operation *module, Attribute value,
                                          PublicationBudget &budget,
                                          unsigned depth = 0) {
  if (!budget.depth(depth)) return failure();
  unsigned needed = module->hasAttr("ac.reference_parameters") ? 2 : 0;
  if (auto row = dyn_cast_or_null<DictionaryAttr>(value)) {
    auto kind = row.getAs<StringAttr>("kind");
    auto receiver = row.getAs<DictionaryAttr>("receiver");
    if (kind && kind.getValue() == "call" && receiver &&
        (receiver.get("receiver_kind") || receiver.get("ancestor_depth")))
      needed = 3;
  }
  if (auto row = dyn_cast_or_null<DictionaryAttr>(value))
    for (NamedAttribute field : row) {
      if (!budget.charge()) return failure();
      StringRef name = field.getName().getValue();
      if (name == "reference_bindings" || name == "reference")
        needed = std::max(needed, 2u);
      auto child = requiredTransactionSchema(module, field.getValue(), budget, depth + 1);
      if (failed(child)) return failure();
      needed = std::max(needed, *child);
    }
  if (auto values = dyn_cast_or_null<ArrayAttr>(value))
    for (Attribute childValue : values) {
      auto child = requiredTransactionSchema(module, childValue, budget, depth + 1);
      if (failed(child)) return failure();
      needed = std::max(needed, *child);
    }
  return needed;
}

LogicalResult chargeAttributeSpelling(PublicationBudget &budget,
                                      Attribute attribute) {
  if (auto text = dyn_cast_or_null<StringAttr>(attribute))
    return budget.charge(text.getValue().size()) ? success() : failure();
  if (auto symbol = dyn_cast_or_null<SymbolRefAttr>(attribute)) {
    if (!budget.charge(symbol.getRootReference().getValue().size()))
      return failure();
    for (FlatSymbolRefAttr nested : symbol.getNestedReferences())
      if (!budget.charge(nested.getValue().size()))
        return failure();
  }
  return success();
}

LogicalResult chargeTypeSpelling(PublicationBudget &budget, Type type) {
  if (auto record = dyn_cast<StructType>(type))
    return chargeAttributeSpelling(budget, record.getName());
  if (auto enumeration = dyn_cast<EnumType>(type))
    return chargeAttributeSpelling(budget, enumeration.getName());
  if (auto formal = dyn_cast<TypeParamType>(type)) {
    if (failed(chargeAttributeSpelling(budget, formal.getOwner())))
      return failure();
    return chargeAttributeSpelling(budget, formal.getName());
  }
  return success();
}

struct ValueScope {
  SmallVector<Type> arguments, captures;
  FunctionType methodSignature;
  ArrayAttr formalProposals;
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
        auto row = dict({{"kind", str("state")},
                         {"name", state.getInstanceNameAttr()},
                         {"state", stateRef(state)},
                         {"type", TypeAttr::get(state.getQ().getType())},
                         {"domain", *domain}});
        if (!row)
          return failure();
        if (Attribute constraint = state->getAttr("ac.payload_constraint")) {
          NamedAttrList fields(row);
          fields.append("payload_constraint", constraint);
          row = fields.getDictionary(builder.getContext());
        }
        owners.push_back(row);
      } else if (auto instance = dyn_cast<InstanceOp>(operation)) {
        if (failed(hardware.verifyInstance(instance)))
          return failure();
        auto arguments = values(instance.getInputs(), nullptr);
        if (failed(arguments))
          return failure();
        FailureOr<DictionaryAttr> row = isQueueInstance(instance)
                                            ? getSourceQueueOwnerRow(instance)
                                            : instance->hasAttr("ac.source_memory")
                                                ? getSourceMemoryOwnerRow(instance)
                                                : FailureOr<DictionaryAttr>(dict(
                                                  {{"kind", str("instance")},
                                                   {"name", instance.getInstanceNameAttr()},
                                                   {"occurrence", instance.getOccurrence()},
                                                   {"callee", instance.getCalleeAttr()},
                                                   {"parameters", instance.getParameters()},
                                                   {"type_arguments", instance.getTypeArguments()},
                                                   {"input_types", types(instance.getInputs().getTypes())},
                                                   {"result_types", types(instance.getResultTypes())}}));
        if (failed(row))
          return failure();
        NamedAttrList fields(*row);
        fields.append("arguments", *arguments);
        if (auto references = instance->getAttr("ac.reference_bindings"))
          fields.append("reference_bindings", references);
        owners.push_back(fields.getDictionary(builder.getContext()));
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
      if (invalid || !budget.ok())
        return failure();
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
    if (!summary || invalid || !budget.ok()) return failure();
    auto newSemantics = requiredTransactionSchema(definition, summary, budget);
    if (failed(newSemantics)) return failure();
    if (*newSemantics) {
      NamedAttrList versioned(summary);
      versioned.append("schema", builder.getI64IntegerAttr(*newSemantics));
      summary = versioned.getDictionary(builder.getContext());
    }
    if (!summary || invalid || !budget.ok() ||
        failed(verifySourceTransactionSummary(definition, summary)))
      return failure();
    return summary;
  }

private:
  StringAttr str(StringRef value) {
    return budget.ok() ? builder.getStringAttr(value) : StringAttr();
  }
  DictionaryAttr
  dict(std::initializer_list<std::pair<StringRef, Attribute>> fields) {
    if (!budget.charge(1 + fields.size()))
      return {};
    for (auto field : fields)
      if (!field.second) {
        invalid = true;
        return {};
      }
    NamedAttrList result;
    for (auto field : fields)
      result.append(field.first, field.second);
    return result.getDictionary(builder.getContext());
  }
  ArrayAttr array(ArrayRef<Attribute> attributes) {
    if (!budget.charge(1 + attributes.size()))
      return {};
    for (Attribute attribute : attributes)
      if (!attribute) {
        invalid = true;
        return {};
      }
    return builder.getArrayAttr(attributes);
  }
  LogicalResult metadata(Attribute attr, unsigned depth = 0) {
    if (!attr || !budget.depth(depth))
      return failure();
    if (failed(chargeAttributeSpelling(budget, attr)))
      return failure();
    if (auto dict = dyn_cast<DictionaryAttr>(attr)) {
      for (NamedAttribute field : dict) {
        if (!budget.charge(field.getName().getValue().size()) ||
            failed(metadata(field.getValue(), depth + 1)))
          return failure();
      }
    } else if (auto values = dyn_cast<ArrayAttr>(attr)) {
      for (Attribute value : values)
        if (failed(metadata(value, depth + 1)))
          return failure();
    } else if (auto type = dyn_cast<TypeAttr>(attr)) {
      return metadataType(type.getValue(), depth + 1);
    }
    return success();
  }
  LogicalResult metadataType(Type type, unsigned depth) {
    if (!budget.depth(depth) || failed(chargeTypeSpelling(budget, type)))
      return failure();
    if (auto bits = dyn_cast<BitsType>(type))
      return metadata(bits.getWidth(), depth + 1);
    if (auto table = dyn_cast<TableType>(type)) {
      if (failed(metadata(table.getShape(), depth + 1)))
        return failure();
      return metadataType(table.getElementType(), depth + 1);
    }
    if (auto function = dyn_cast<FunctionType>(type))
      for (TypeRange types : {function.getInputs(), function.getResults()})
        for (Type nested : types)
          if (failed(metadataType(nested, depth + 1)))
            return failure();
    return success();
  }
  ArrayAttr types(TypeRange inputs) {
    SmallVector<Attribute> result;
    for (Type type : inputs) {
      if (!budget.charge())
        return {};
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
        if (!lifted)
          return {};
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
          if (!dependency)
            return {};
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
    if (!known) {
      auto row = unknown();
      if (!row)
        return {};
      return dependencyCache[key] = row;
    }
    auto ordinals = [&](const std::set<uint64_t> &set) {
      SmallVector<Attribute> result;
      for (uint64_t ordinal : set) {
        if (!budget.charge())
          return ArrayAttr();
        result.push_back(builder.getI64IntegerAttr(ordinal));
      }
      return array(result);
    };
    auto row = dict({{"kind", str("known")},
                     {"arguments", ordinals(arguments)},
                     {"captures", ordinals(captures)}});
    if (!row)
      return {};
    return dependencyCache[key] = row;
  }
  DictionaryAttr value(Value value, Operation *scope) {
    return dict({{"fact", fact(value, scope)},
                 {"dependencies", dependencies(value, scope)}});
  }
  FailureOr<ArrayAttr> values(ValueRange inputs, Operation *scope) {
    SmallVector<Attribute> result;
    for (Value input : inputs) {
      auto item = value(input, scope);
      if (!item)
        return failure();
      result.push_back(item);
      if (invalid || !budget.ok())
        return failure();
    }
    if (invalid || !budget.ok())
      return failure();
    auto rows = array(result);
    return rows ? FailureOr<ArrayAttr>(rows) : FailureOr<ArrayAttr>(failure());
  }
  FailureOr<DictionaryAttr> endpoint(Value input) {
    if (!budget.charge())
      return failure();
    if (auto argument = dyn_cast<BlockArgument>(input)) {
      if (argument.getOwner()->getParentOp() == definition &&
          oneBit(input.getType())) {
        auto row = dict(
            {{"kind", str("input")},
             {"ordinal", builder.getI64IntegerAttr(argument.getArgNumber())}});
        return row ? FailureOr<DictionaryAttr>(row)
                   : FailureOr<DictionaryAttr>(failure());
      }
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
    if (*literal) {
      auto row = dict({{"kind", str("constant")}, {"value", *literal}});
      return row ? FailureOr<DictionaryAttr>(row)
                 : FailureOr<DictionaryAttr>(failure());
    }
    return definition.emitOpError()
           << "transaction publication domain endpoint requires a physical "
              "one-bit module input or direct static literal";
  }
  FailureOr<DictionaryAttr> getDomain(Value clock, Value reset) {
    auto c = endpoint(clock), r = endpoint(reset);
    if (failed(c) || failed(r))
      return failure();
    auto row = dict({{"clock", *c}, {"reset", *r}});
    return row ? FailureOr<DictionaryAttr>(row)
               : FailureOr<DictionaryAttr>(failure());
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
      auto item = dict({{"type", TypeAttr::get(input.getType())},
                        {"value", value(input, nullptr)}});
      if (!item)
        return failure();
      result.push_back(item);
      if (invalid || !budget.ok())
        return failure();
    }
    if (invalid || !budget.ok())
      return failure();
    auto rows = array(result);
    return rows ? FailureOr<ArrayAttr>(rows) : FailureOr<ArrayAttr>(failure());
  }
  FailureOr<ArrayAttr> selectors(ArrayAttr selectors, ValueRange indices,
                                 Operation *scope) {
    SmallVector<Attribute> result;
    for (Attribute raw : selectors) {
      auto selector = cast<DictionaryAttr>(raw);
      DictionaryAttr item;
      if (auto field = selector.getAs<StringAttr>("field"))
        item = dict({{"kind", str("field")}, {"name", field}});
      else {
        auto ordinal = selector.getAs<IntegerAttr>("index").getInt();
        auto index = indices[ordinal];
        item = dict({{"kind", str("any_element")},
                     {"index_type", TypeAttr::get(index.getType())},
                     {"index", value(index, scope)}});
      }
      if (!item || invalid || !budget.ok())
        return failure();
      result.push_back(item);
    }
    auto rows = array(result);
    return rows ? FailureOr<ArrayAttr>(rows) : FailureOr<ArrayAttr>(failure());
  }
  FailureOr<DictionaryAttr> receiver(SourceMethodCallOp call) {
    auto resolved = resolveSourceMethodCall(call);
    if (failed(resolved))
      return failure();
    return resolved->receiver;
  }
  FailureOr<DictionaryAttr> branchEffect(SourceBranchOp branch,
                                         Operation *scope, unsigned depth) {
    if (!budget.depth(depth) || failed(metadata(branch.getOccurrence())) ||
        failed(metadata(branch.getConditionLocation())))
      return failure();
    auto condition = value(branch.getCondition(), scope);
    if (!condition)
      return failure();
    auto whenTrue = effects(branch.getThenRegion(), scope, depth + 1, true);
    if (failed(whenTrue))
      return failure();
    auto whenFalse = effects(branch.getElseRegion(), scope, depth + 1, true);
    if (failed(whenFalse))
      return failure();
    SmallVector<Attribute> results;
    auto trueValues =
        cast<YieldOp>(branch.getThenRegion().front().back()).getValues();
    auto falseValues =
        cast<YieldOp>(branch.getElseRegion().front().back()).getValues();
    uint64_t count = branch.getNumResults();
    if (count > results.max_size() || !budget.storage(count, sizeof(Attribute)))
      return failure();
    results.reserve(count);
    for (auto [type, left, right] :
         llvm::zip(branch.getResultTypes(), trueValues, falseValues)) {
      HardwareBindings bindings;
      bindings.owner = definition;
      if (!budget.charge()) return failure();
      // One publication request has one fixed lexical Module binding. Reuse
      // its admission of an identical type instead of rewalking the same
      // source width and nominal declarations for every loop join result.
      if (!admittedBranchTypes.contains(type)) {
        auto admitted = detail::resolveSourceBranchResultType(
            type, hardware, bindings, branch,
            [&](uint64_t work, uint64_t bytes) {
              return success(budget.charge(work) && budget.storage(1, bytes));
            });
        if (failed(admitted) || !budget.storage(1, 3 * sizeof(Type)))
          return failure();
        admittedBranchTypes.insert(type);
      }
      auto physical = TypeAttr::get(type);
      if (failed(metadata(physical)))
        return failure();
      auto item = dict({{"type", physical},
                        {"when_true", value(left, scope)},
                        {"when_false", value(right, scope)}});
      if (!item)
        return failure();
      results.push_back(item);
    }
    auto row = dict({{"kind", str("branch")},
                     {"occurrence", branch.getOccurrence()},
                     {"condition_location", branch.getConditionLocation()},
                     {"condition", condition},
                     {"then_effects", *whenTrue},
                     {"else_effects", *whenFalse},
                     {"results", array(results)}});
    if (row && branch->hasAttr("ac.loop_continuation")) {
      NamedAttrList marked(row);
      marked.append("loop_continuation", builder.getBoolAttr(true));
      row = marked.getDictionary(builder.getContext());
    }
    return row ? FailureOr<DictionaryAttr>(row)
               : FailureOr<DictionaryAttr>(failure());
  }
  FailureOr<ArrayAttr> effects(Region &region, Operation *scope,
                               unsigned depth = 0, bool arm = false) {
    if (!budget.depth(depth))
      return failure();
    SmallVector<Attribute> result;
    uint64_t count = 0;
    for (Operation &operation : region.front().without_terminator()) {
      if (!budget.charge())
        return failure();
      if (isa<SourceBranchOp, SourceFormalWriteOp, SourceMethodCallOp,
              SourceExpectOp, SourceObserveOp, SourceStateReadOp,
              SourceStateWriteOp>(operation))
        ++count;
    }
    if (count > result.max_size() || !budget.storage(count, sizeof(Attribute)))
      return failure();
    result.reserve(count);
    for (Operation &operation : region.front().without_terminator()) {
      if (!budget.charge(1 + operation.getNumOperands() +
                         operation.getNumResults()))
        return failure();
      if (auto branch = dyn_cast<SourceBranchOp>(operation)) {
        if (&region == &scope->getRegion(0))
          return branch.emitOpError()
                 << "source branches are forbidden in guards";
        auto row = branchEffect(branch, scope, depth + 1);
        if (failed(row))
          return failure();
        result.push_back(*row);
      } else if (auto call = dyn_cast<SourceMethodCallOp>(operation)) {
        auto actuals = values(call.getArguments(), scope);
        auto owner = receiver(call);
        if (failed(actuals) || failed(owner))
          return failure();
        StringRef receiverSymbol =
            call.getCalleeAttr().getRootReference().getValue();
        bool nativeQueue =
            receiverSymbol == "pycircuit.__builtins__.fifo_local" ||
            receiverSymbol == "pycircuit.__builtins__.fifo_downstream_pop" ||
            receiverSymbol == "pycircuit.__builtins__.sync_mem" ||
            receiverSymbol == "pycircuit.__builtins__.sync_mem_dp" ||
            receiverSymbol == "pycircuit.__builtins__.byte_mem";
        Attribute location = call->getAttr("ac.location");
        if (nativeQueue && failed(metadata(location)))
          return call.emitOpError()
                 << "native FIFO call publication requires ac.location";
        auto row = dict({{"kind", str("call")},
                         {"occurrence", call.getOccurrence()},
                         {"path", fact(call.getPath(), scope)},
                         {"callee", call.getCalleeAttr()},
                         {"receiver", *owner},
                         {"arguments", *actuals}});
        if (!row)
          return failure();
        NamedAttrList fields(row);
        if (nativeQueue)
          fields.append("location", location);
        if (Attribute bindings = call->getAttr("ac.formal_owner_bindings")) {
          fields.append("ac.formal_owner_bindings", bindings);
        }
        result.push_back(fields.getDictionary(builder.getContext()));
      } else if (auto write = dyn_cast<SourceFormalWriteOp>(operation)) {
        if (failed(metadata(write.getSourceLocation())))
          return failure();
        auto selected =
            selectors(write.getSelectors(), write.getIndices(), scope);
        if (failed(selected))
          return failure();
        auto row =
            dict({{"kind", str("formal_write")},
                  {"occurrence", write.getOccurrence()},
                  {"path", fact(write.getPath(), scope)},
                  {"formal", write.getFormalAttr()},
                  {"selectors", *selected},
                  {"payload_type", TypeAttr::get(write.getPayload().getType())},
                  {"payload", value(write.getPayload(), scope)},
                  {"candidate", value(write.getCandidate(), scope)},
                  {"location", write.getSourceLocation()}});
        if (!row)
          return failure();
        if (write.getPrevious()) {
          NamedAttrList forwarded(row);
          auto previous = value(write.getPrevious(), scope);
          if (!previous) return failure();
          forwarded.append("previous", previous);
          forwarded.append("snapshot_type", TypeAttr::get(write.getSnapshot().getType()));
          row = forwarded.getDictionary(builder.getContext());
        }
        result.push_back(row);
      } else if (auto check = dyn_cast<SourceExpectOp>(operation)) {
        if (failed(metadata(check->getAttr("occurrence"))) ||
            failed(metadata(check.getLocationAttr())) ||
            failed(metadata(check.getKindAttr())) ||
            (check->hasAttr("ac.message") &&
             failed(metadata(check->getAttr("ac.message")))))
          return failure();
        auto row = dict({{"kind", str("check")},
                         {"occurrence", check->getAttr("occurrence")},
                         {"path", fact(check.getPath(), scope)},
                         {"check_kind", check.getKindAttr()},
                         {"condition", value(check.getCondition(), scope)},
                         {"location", check.getLocationAttr()},
                         {"message", check->getAttr("ac.message")
                                         ? check->getAttr("ac.message")
                                         : Attribute(builder.getUnitAttr())}});
        if (!row)
          return failure();
        result.push_back(row);
      } else if (auto observe = dyn_cast<SourceObserveOp>(operation)) {
        if (failed(metadata(observe->getAttr("occurrence"))) ||
            failed(metadata(observe.getKindAttr())) ||
            failed(metadata(observe.getSpec())) ||
            failed(metadata(observe->getAttr("ac.location"))))
          return failure();
        SmallVector<Attribute, 0> observed;
        uint64_t valueCount = observe.getValues().size();
        if (valueCount > observed.max_size() ||
            !budget.storage(valueCount, sizeof(Attribute)))
          return failure();
        observed.reserve(valueCount);
        for (Value input : observe.getValues()) {
          auto type = TypeAttr::get(input.getType());
          if (failed(metadata(type)))
            return failure();
          auto item = dict({{"type", type}, {"value", value(input, scope)}});
          if (!item)
            return failure();
          observed.push_back(item);
        }
        auto row = dict({{"kind", str("observe")},
                         {"occurrence", observe->getAttr("occurrence")},
                         {"path", fact(observe.getPath(), scope)},
                         {"observation_kind", observe.getKindAttr()},
                         {"spec", observe.getSpec()},
                         {"location", observe->getAttr("ac.location")},
                         {"values", array(observed)}});
        if (!row)
          return failure();
        result.push_back(row);
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
        if (arm && !isa<BitsType, EnumType, StructType, TableType>(
                       state.getQ().getType()))
          return operation.emitOpError()
                 << "source branch arms require selected finite state accesses";
        ValueRange indices = operation.getOperands().drop_front(write ? 2 : 1);
        auto selected = selectors(
            operation.getAttrOfType<ArrayAttr>("selectors"), indices, scope);
        if (failed(selected))
          return failure();
        auto base = dict({{"kind", str(write ? "write" : "read")},
                          {"occurrence", operation.getAttr("occurrence")},
                          {"path", fact(operation.getOperand(0), scope)},
                          {"state", stateRef(state)},
                          {"selectors", *selected}});
        if (!base || !budget.charge(write ? 2 : 1))
          return failure();
        NamedAttrList row(base);
        if (write) {
          auto written = value(operation.getOperand(1), scope);
          if (!written)
            return failure();
          row.append("value", written);
        }
        if (!budget.ok() || invalid)
          return failure();
        result.push_back(row.getDictionary(builder.getContext()));
      } else if (!detail::isCombinationalHardwareOperation(&operation)) {
        return operation.emitOpError()
               << "transaction publication cannot omit this callable operation";
      } else if (failed(scanPure(&operation, depth)))
        return failure();
      if (invalid || !budget.ok())
        return failure();
    }
    if (invalid || !budget.ok())
      return failure();
    auto rows = array(result);
    return rows ? FailureOr<ArrayAttr>(rows) : FailureOr<ArrayAttr>(failure());
  }
  FailureOr<DictionaryAttr> callable(Operation *operation) {
    if (operation->hasAttr("ac.after") || operation->hasAttr("ac.callable_after"))
      return operation->emitOpError() << "source after ordering is retired; independent Rules read current state";
    std::optional<SourceRegistrationEntryView> entry;
    if (auto transaction = dyn_cast<SourceTransactionOp>(operation)) {
      auto checked = getSourceRegistrationEntry(transaction);
      if (failed(checked)) return failure();
      entry = *checked;
    }
    auto guardLocation =
        operation->getAttrOfType<DictionaryAttr>("ac.guard_location");
    if (failed(detail::verifySourceGuardLocation(
            guardLocation, definition.getSourceOwner(),
            [&] { return operation->emitOpError(); })))
      return failure();
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
    auto base = dict({{"captures", *captureRows},
                      {"guard_location", guardLocation},
                      {"guard", value(guard.getValues().front(), operation)},
                      {"guard_effects", *guardEffects},
                      {"body_effects", *bodyEffects},
                      {"results", *results}});
    if (!base || !budget.charge(method ? 7 : 5))
      return failure();
    NamedAttrList row(base);
    if (method) {
      row.append("name", method.getSymNameAttr());
      row.append("declaration", method.getOccurrence());
      row.append("function_type", method.getFunctionTypeAttr());
      row.append("argument_names", method.getArgumentNames());
      for (StringRef key :
           {"ac.parameters", "ac.result_constraints", "ac.return_form"})
        row.append(key, method->getAttr(key));
      if (auto proposals = method->getAttr("ac.formal_proposals"))
        row.append("ac.formal_proposals", proposals);
      if (auto visibility = method->getAttr("sym_visibility"))
        row.append("visibility", visibility);
    } else {
      auto transaction = cast<SourceTransactionOp>(operation);
      auto domain = getDomain(transaction.getClk(), transaction.getRst());
      if (failed(domain))
        return failure();
      row.append("name", transaction.getNameAttr());
      row.append("declaration", transaction.getDeclaration());
      row.append("registration", transaction.getOccurrence());
      if (entry)
        row.append("registration_entry", entry->call.getCalleeAttr());
      row.append("domain", *domain);
      auto resultTypes = types(transaction.getResultTypes());
      if (!resultTypes)
        return failure();
      row.append("result_types", resultTypes);
    }
    if (invalid || !budget.ok())
      return failure();
    if (!budget.charge(row.getAttrs().size()))
      return failure();
    return row.getDictionary(builder.getContext());
  }
  ModuleOp definition;
  HardwareAnalysis hardware;
  Builder builder;
  PublicationBudget budget;
  llvm::DenseSet<Operation *> visiting, visited;
  llvm::DenseMap<std::pair<Value, Operation *>, DictionaryAttr> dependencyCache;
  llvm::DenseSet<Type> admittedBranchTypes;
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
    NamedAttrList legacy(summary);
    legacy.erase("schema");
    if (failed(exact(legacy.getDictionary(declaration->getContext()),
                     {"owners", "methods", "autonomous"})))
      return failure();
    auto newSemantics = requiredTransactionSchema(declaration, summary, budget);
    if (failed(newSemantics)) return failure();
    auto schema = summary.getAs<IntegerAttr>("schema");
    if ((*newSemantics && !schema) ||
        (summary.get("schema") &&
         (!schema || !schema.getType().isSignlessInteger(64) ||
          schema.getInt() != (*newSemantics == 3 ? 3 : 2))))
      return error(*newSemantics == 3
          ? "relative receiver calls require schema = 3 : i64"
          : "reference semantics require schema = 2 : i64");
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
        NamedAttrList core(row);
        core.erase("payload_constraint");
        if (failed(exact(core.getDictionary(declaration->getContext()),
                         {"kind", "name", "state", "type", "domain"})))
          return failure();
        auto state = row.getAs<DictionaryAttr>("state");
        if (failed(ownedState(state)) || failed(type(row.get("type"))) ||
            failed(domain(row.getAs<DictionaryAttr>("domain"))))
          return failure();
        if (Attribute constraint = row.get("payload_constraint"))
          if (failed(detail::verifySourceCallHardwareConstraint(
                  constraint, row.getAs<TypeAttr>("type").getValue(), hardware,
                  declaration, declaration, "transaction summary state payload")))
            return failure();
        identity = state.getAs<DictionaryAttr>("declaration");
        stateRows[state] = row.getAs<TypeAttr>("type").getValue();
        stateNames.insert(name.getValue());
      } else if (kind.getValue() == "instance") {
        NamedAttrList core(row);
        Attribute payloadConstraint = row.get("payload_constraint");
        Attribute rawUnboundPush = row.get("unbound_push");
        Attribute rawUnboundPop = row.get("unbound_pop");
        auto unboundPush = row.getAs<BoolAttr>("unbound_push");
        auto unboundPop = row.getAs<BoolAttr>("unbound_pop");
        Attribute rawMemory = row.get("source_memory");
        auto memory = row.getAs<BoolAttr>("source_memory");
        core.erase("reference_bindings");
        core.erase("source_memory");
        core.erase("payload_constraint");
        core.erase("unbound_push");
        core.erase("unbound_pop");
        if (failed(exact(core.getDictionary(declaration->getContext()),
                         {"kind", "name", "occurrence", "callee",
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
        if ((rawUnboundPush && !unboundPush) ||
            (rawUnboundPop && !unboundPop))
          return error("queue unbound-direction facts require BoolAttr");
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
        for (Attribute raw : inputs)
          if (failed(type(raw)))
            return failure();
        bool queue = callee.getValue() == "pycircuit.__builtins__.fifo_local" ||
                     callee.getValue() ==
                         "pycircuit.__builtins__.fifo_downstream_pop";
        bool memoryKind = callee.getValue() == "pycircuit.__builtins__.sync_mem" ||
                          callee.getValue() == "pycircuit.__builtins__.sync_mem_dp" ||
                          callee.getValue() == "pycircuit.__builtins__.byte_mem";
        if (rawMemory && (!memoryKind || !memory || !memory.getValue()))
          return error("source Memory binding requires a true fact on a canonical memory");
        if (!queue && ((!memory && payloadConstraint) || unboundPush || unboundPop))
          return error("native binding facts require their canonical resource owner");
        if (memory) {
          bool byte = callee.getValue() == "pycircuit.__builtins__.byte_mem";
          unsigned ports = callee.getValue() == "pycircuit.__builtins__.sync_mem_dp" ? 2 : 1;
          unsigned write = byte ? 3 : ports == 2 ? 6 : 4;
          if (inputs.size() != write + 4 || results.size() != (byte ? 1 : 2 * ports) ||
              typeArguments.size() != 1 || parameters.size() != 2 || !payloadConstraint)
            return error("source Memory instance summary shape is invalid");
          auto payload = cast<TypeAttr>(results[0]).getValue();
          if (failed(detail::verifySourceCallHardwareConstraint(
                  payloadConstraint, payload, hardware, declaration, declaration,
                  "transaction summary Memory payload"))) return failure();
          for (unsigned port = 0; port < ports; ++port) {
            if (!areEquivalentHardwareTypes(cast<TypeAttr>(results[port]).getValue(), payload))
              return error("source Memory payload result types disagree");
            if (!byte) {
              auto live = hardware.getPackedWidth(cast<TypeAttr>(results[ports + port]).getValue(), {}, declaration);
              if (failed(live) || *live != 1) return error("Memory live result requires one bit");
            }
          }
          SmallVector<unsigned> controls{write};
          if (!byte) for (unsigned port = 0; port < ports; ++port) controls.push_back(2 + 2 * port);
          for (unsigned ordinal : controls) {
            auto binding = dyn_cast<DictionaryAttr>(actuals[ordinal]);
            auto fact = binding ? binding.getAs<DictionaryAttr>("fact") : DictionaryAttr();
            auto kind = fact ? fact.getAs<StringAttr>("kind") : StringAttr();
            if (!kind || kind.getValue() != "constant" || !fact.getAs<BoolAttr>("value") ||
                fact.getAs<BoolAttr>("value").getValue())
              return error("source Memory issue ports require known-false idle bindings");
          }
        }
        if (queue) {
          if ((unboundPush && !unboundPush.getValue()) ||
              (unboundPop && !unboundPop.getValue()))
            return error("queue unbound-direction facts must be true when present");
          if (inputs.size() != 5 || results.size() != 4 ||
              typeArguments.size() != 1 || parameters.size() != 2)
            return error("canonical FIFO instance summary shape is invalid");
          auto payload = cast<TypeAttr>(results[2]).getValue();
          auto config =
              getQueueConfiguration(callee, parameters, declaration);
          if (failed(config) ||
              failed(verifySourceQueueConfiguration(
                  declaration, *config, payload, declaration)))
            return failure();
          if (payloadConstraint &&
              failed(detail::verifySourceCallHardwareConstraint(
                  payloadConstraint, payload, hardware, declaration,
                  declaration, "transaction summary queue payload")))
            return failure();
          if ((unboundPush || unboundPop) && !payloadConstraint)
            return error("source Queue methods require a checked payload constraint");
          for (auto [index, unbound] :
               {std::pair<unsigned, bool>{2, unboundPush && unboundPush.getValue()},
                {4, unboundPop && unboundPop.getValue()}}) {
            auto binding = dyn_cast<DictionaryAttr>(actuals[index]);
            auto fact = binding
                            ? binding.getAs<DictionaryAttr>("fact")
                            : DictionaryAttr();
            auto valueKind =
                fact ? fact.getAs<StringAttr>("kind") : StringAttr();
            if (unbound &&
                (!valueKind || valueKind.getValue() != "constant" ||
                 !fact.getAs<BoolAttr>("value") ||
                 fact.getAs<BoolAttr>("value").getValue()))
              return error("unbound queue request binding must be constant false");
          }
        }
        for (auto [input, actual] : llvm::zip(inputs, actuals)) {
          if (failed(
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
    for (auto entry : instanceRows) {
      auto row = entry.second;
      auto target = hardware.lookupDefinition(row.getAs<FlatSymbolRefAttr>("callee").getValue());
      if (failed(verifySourceReferenceBindings(declaration, target, row.get("reference_bindings"),
              [&](FlatSymbolRefAttr symbol) { return hardware.lookupDefinition(symbol.getValue()); }, declaration)))
        return failure();
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
    // Identify internal autonomous bodies from real declaration registration,
    // before checking any helper's calls into those bodies.
    for (Attribute raw : autonomous) {
      auto row = dyn_cast<DictionaryAttr>(raw);
      if (row && row.get("registration") == row.get("declaration") &&
          row.get("registration_entry"))
        autonomousBodies.insert(row.get("registration_entry"));
    }
    for (Attribute raw : methods)
      if (failed(callable(cast<DictionaryAttr>(raw), true)))
        return failure();
    DictionaryAttr previousRegistration;
    for (Attribute raw : autonomous) {
      auto row = dyn_cast<DictionaryAttr>(raw);
      NamedAttrList core;
      if (row) core.assign(row.getValue());
      if (row && row.get("after"))
        return error("after ordering is retired; independent Rules read current state");
      core.erase("registration_entry");
      if (failed(exact(core.getDictionary(declaration->getContext()),
                       {"name", "declaration", "registration", "domain",
                        "guard_location", "captures", "guard", "guard_effects",
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
      if (failed(callable(row, false)))
        return failure();
      if (failed(registrationEntry(row)))
        return failure();
    }
    return success(budget.ok());
  }

private:
  llvm::DenseSet<Attribute> autonomousBodies;
  DictionaryAttr activeCallable;
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
    if (failed(chargeAttributeSpelling(budget, attr)))
      return failure();
    if (auto dict = dyn_cast<DictionaryAttr>(attr)) {
      for (auto field : dict) {
        if (!budget.charge(field.getName().getValue().size()))
          return failure();
        if (failed(preflight(field.getValue(), depth + 1)))
          return failure();
      }
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
    if (!budget.depth(depth) || failed(chargeTypeSpelling(budget, type)))
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
    NamedAttrList core(row);
    core.erase("ac.formal_proposals");
    core.erase("visibility");
    if (row.get("callable_after") || row.get("after"))
      return error("after ordering is retired; independent Rules read current state");
    if (Attribute raw = row.get("visibility")) {
      auto visibility = dyn_cast<StringAttr>(raw);
      if (!visibility || visibility.getValue() != "private")
        return error("Method visibility must be absent or private");
    }
    if (failed(exact(core.getDictionary(declaration->getContext()),
                     {"name", "declaration", "function_type", "argument_names",
                      "ac.parameters", "ac.result_constraints",
                      "ac.return_form", "guard_location", "captures", "guard",
                      "guard_effects", "body_effects", "results"})) ||
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
        constraints.size() != function.getNumResults())
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
    SourceMethodView contract{declaration, {}, row};
    if (failed(verifySourceMethodFormalProposals(contract, declaration)))
      return failure();
    unsigned publicCount = contract.getNumPublicResults();
    if (!((form.getValue() == "none" && publicCount == 0) ||
          (form.getValue() == "single" && publicCount == 1) ||
          (form.getValue() == "tuple" && publicCount != 0)))
      return error("method ac.return_form disagrees with public results");
    return success();
  }
  LogicalResult registrationEntry(DictionaryAttr row) {
    Attribute raw = row.get("registration_entry");
    if (!raw) return success();
    auto symbol = dyn_cast<SymbolRefAttr>(raw);
    if (!symbol || symbol.getRootReference() !=
                       declaration->getAttrOfType<StringAttr>("sym_name") ||
        symbol.getNestedReferences().size() != 1)
      return error("registration_entry requires a local Method reference");
    auto found = methodRows.find(symbol.getNestedReferences().front().getAttr());
    if (found == methodRows.end())
      return error("registration_entry must name a local Method row");
    auto method = found->second;
    if (row.get("registration") == row.get("declaration")) {
      auto visibility = method.getAs<StringAttr>("visibility");
      if (!visibility || visibility.getValue() != "private")
        return error("autonomous Rule declaration requires a private registration body");
    }
    if (method.get("declaration") != row.get("declaration") ||
        method.get("guard_location") != row.get("guard_location"))
      return error("registration_entry Method declaration/guard must agree");
    auto guard = row.getAs<DictionaryAttr>("guard").getAs<DictionaryAttr>("fact");
    auto guardKind = guard.getAs<StringAttr>("kind");
    auto guardValue = guard.getAs<BoolAttr>("value");
    auto body = row.getAs<ArrayAttr>("body_effects");
    if (!guardKind || guardKind.getValue() != "constant" || !guardValue ||
        !guardValue.getValue() ||
        !row.getAs<ArrayAttr>("guard_effects").empty() || body.size() != 1)
      return error("registration_entry requires a true guard and one call");
    auto call = dyn_cast<DictionaryAttr>(body[0]);
    auto receiver = call ? call.getAs<DictionaryAttr>("receiver") : DictionaryAttr();
    auto path = call ? call.getAs<DictionaryAttr>("path") : DictionaryAttr();
    auto pathKind = path ? path.getAs<StringAttr>("kind") : StringAttr();
    auto pathValue = path ? path.getAs<BoolAttr>("value") : BoolAttr();
    auto kind = call ? call.getAs<StringAttr>("kind") : StringAttr();
    if (!kind || kind.getValue() != "call" || call.get("callee") != symbol ||
        call.get("occurrence") != row.get("registration") ||
        call.get("ac.formal_owner_bindings") || !receiver ||
        receiver.get("reference") || receiver.get("receiver_kind") ||
        receiver.get("ancestor_depth") ||
        !receiver.getAs<ArrayAttr>("instance_path").empty() ||
        !pathKind || pathKind.getValue() != "constant" || !pathValue ||
        !pathValue.getValue())
      return error("registration_entry requires an exact local registration call");
    auto signature = cast<FunctionType>(
        method.getAs<TypeAttr>("function_type").getValue());
    auto captures = row.getAs<ArrayAttr>("captures");
    if (captures.size() != signature.getNumInputs())
      return error("registration_entry captures must match explicit arguments");
    for (auto [capture, input] : llvm::zip(captures, signature.getInputs()))
      if (!areEquivalentHardwareTypes(
              cast<DictionaryAttr>(capture).getAs<TypeAttr>("type").getValue(), input))
        return error("registration_entry capture types must match Method inputs");
    SourceMethodView contract{declaration, {}, method};
    auto results = row.getAs<ArrayAttr>("result_types");
    if (results.size() != contract.getNumPublicResults())
      return error("registration_entry exposes only the Method public prefix");
    for (auto [index, result] : llvm::enumerate(results))
      if (!areEquivalentHardwareTypes(cast<TypeAttr>(result).getValue(),
                                     signature.getResult(index)))
        return error("registration_entry public result order must match Method");
    // This compact reference does not claim to prove SSA forwarding. Linking
    // authenticates the provider body and rederives this row through the strict
    // native registration-entry view before using the shared Method body.
    return success();
  }
  LogicalResult callable(DictionaryAttr row, bool method) {
    activeCallable = row;
    if (failed(detail::verifySourceGuardLocation(
            row.getAs<DictionaryAttr>("guard_location"),
            declaration->getAttrOfType<DictionaryAttr>("source_owner"),
            [&] { return declaration->emitOpError(); })))
      return failure();
    ValueScope scope;
    SmallVector<Type> outputs;
    if (method) {
      auto function =
          cast<FunctionType>(row.getAs<TypeAttr>("function_type").getValue());
      scope.arguments.assign(function.getInputs().begin(),
                             function.getInputs().end());
      scope.methodSignature = function;
      scope.formalProposals = row.getAs<ArrayAttr>("ac.formal_proposals");
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
  FailureOr<Type> selectedType(Type root, ArrayAttr selectors,
                               const ValueScope &scope) {
    HardwareBindings bindings;
    bindings.owner = declaration;
    auto resolved = hardware.resolveType(root, bindings, declaration);
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
  FailureOr<Type> selectedType(DictionaryAttr state, ArrayAttr selectors,
                               const ValueScope &scope) {
    auto found = stateRows.find(state);
    if (found == stateRows.end()) {
      (void)error("effect state must name a local owner row");
      return failure();
    }
    return selectedType(found->second, selectors, scope);
  }
  LogicalResult call(DictionaryAttr row, const ValueScope &scope) {
    NamedAttrList core(row);
    core.erase("ac.formal_owner_bindings");
    auto callee = row.getAs<SymbolRefAttr>("callee");
    bool nativeQueue =
        callee &&
        (callee.getRootReference().getValue() ==
             "pycircuit.__builtins__.fifo_local" ||
         callee.getRootReference().getValue() ==
             "pycircuit.__builtins__.fifo_downstream_pop" ||
         callee.getRootReference().getValue() == "pycircuit.__builtins__.sync_mem" ||
         callee.getRootReference().getValue() == "pycircuit.__builtins__.sync_mem_dp" ||
         callee.getRootReference().getValue() == "pycircuit.__builtins__.byte_mem");
    if ((nativeQueue &&
         failed(exact(core.getDictionary(declaration->getContext()),
                      {"kind", "occurrence", "path", "callee", "receiver",
                       "arguments", "location"}))) ||
        (!nativeQueue &&
         failed(exact(core.getDictionary(declaration->getContext()),
                      {"kind", "occurrence", "path", "callee", "receiver",
                       "arguments"}))))
      return failure();
    auto receiver = row.getAs<DictionaryAttr>("receiver");
    auto actuals = row.getAs<ArrayAttr>("arguments");
    NamedAttrList concreteReceiver(receiver);
    concreteReceiver.erase("reference");
    auto reference = receiver ? receiver.getAs<StringAttr>("reference") : StringAttr();
    bool relative = receiver &&
        (receiver.get("receiver_kind") || receiver.get("ancestor_depth"));
    if (!callee || callee.getRootReference().getValue().empty() ||
        callee.getNestedReferences().size() != 1 || !actuals ||
        (receiver && receiver.get("reference") && !reference) ||
        (relative ? failed(verifySourceRelativeReceiver(receiver, declaration))
                  : failed(detail::verifyOwnerRef(
                        concreteReceiver.getDictionary(declaration->getContext()),
                        [&] { return declaration->emitOpError(); }))))
      return error("call requires a qualified Module::Method, OwnerRef and "
                   "argument array");
    if (nativeQueue &&
        failed(detail::verifySourceEffectMetadata(
            row.getAs<DictionaryAttr>("occurrence"),
            row.getAs<DictionaryAttr>("location"), declaration,
            [&] { return declaration->emitOpError(); })))
      return failure();
    auto path = receiver.getAs<ArrayAttr>("instance_path");
    if (path.size() > 1 || (reference && !path.empty()))
      return error("Method receiver requires lexical self, a direct child or a declared reference interface");
    if (autonomousBodies.contains(callee)) {
      auto registration = activeCallable.getAs<DictionaryAttr>("registration");
      if (!registration || registration != activeCallable.get("declaration") ||
          activeCallable.get("registration_entry") != callee ||
          row.get("occurrence") != registration ||
          !activeCallable.getAs<ArrayAttr>("guard_effects").empty() ||
          activeCallable.getAs<ArrayAttr>("body_effects").size() != 1 ||
          activeCallable.getAs<ArrayAttr>("body_effects")[0] != row)
        return error("autonomous Rule body is private to its declaration registration");
    }
    std::optional<ArrayAttr> proposals;
    if (relative) {
      // A selector is declaration authority only. Actual occurrence selection,
      // nominal provider and complete signature are checked by link closure.
      if (nativeQueue)
        return error("relative Method target requires an ordinary Module provider");
      for (Attribute actual : actuals)
        if (failed(value(actual, scope))) return failure();
    } else if (reference) {
      auto target = resolveSourceReferenceReceiver(declaration, receiver,
          [&](FlatSymbolRefAttr symbol) { return hardware.lookupDefinition(symbol.getValue()); }, declaration);
      if (failed(target) || SymbolTable::getSymbolName(*target) != callee.getRootReference())
        return error("symbolic receiver must match a declared Module reference provider");
      for (Attribute actual : actuals)
        if (failed(value(actual, scope))) return failure();
    } else if (path.empty()) {
      if (callee.getRootReference() !=
          declaration->getAttrOfType<StringAttr>("sym_name"))
        return error("self call callee must name this module");
      auto found =
          methodRows.find(callee.getNestedReferences().front().getAttr());
      if (found == methodRows.end())
        return error("self call must name a local Method row");
      auto function = cast<FunctionType>(
          found->second.getAs<TypeAttr>("function_type").getValue());
      proposals = found->second.getAs<ArrayAttr>("ac.formal_proposals");
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
    return verifySourceFormalOwnerBindings(
        row.get("ac.formal_owner_bindings"), actuals.size(), proposals,
        scope.methodSignature, scope.formalProposals,
        [&](StringRef name) { return stateNames.contains(name); }, declaration);
  }
  LogicalResult effects(ArrayAttr rows, const ValueScope &scope, bool guard,
                        unsigned depth = 0, bool arm = false) {
    if (!rows || !budget.depth(depth))
      return failure();
    for (Attribute raw : rows) {
      if (!budget.charge())
        return failure();
      auto row = dyn_cast<DictionaryAttr>(raw);
      auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
      if (!kind)
        return error("effect kind must be a string");
      if (failed(occurrence(row.getAs<DictionaryAttr>("occurrence"))))
        return failure();
      // A branch owns its condition and both ordered arms; it has no path row.
      if (kind.getValue() == "branch") {
        if (guard)
          return error("guard effects permit reads only");
        NamedAttrList branchCore(row);
        branchCore.erase("loop_continuation");
        if (Attribute marker = row.get("loop_continuation")) {
          auto enabled = dyn_cast<BoolAttr>(marker);
          if (!enabled || !enabled.getValue())
            return error("loop_continuation requires BoolAttr true");
        }
        if (failed(exact(branchCore.getDictionary(declaration->getContext()),
                              {"kind", "occurrence", "condition_location",
                               "condition", "then_effects", "else_effects",
                               "results"})))
          return failure();
        if (failed(detail::verifySourceEffectMetadata(
                row.getAs<DictionaryAttr>("occurrence"),
                row.getAs<DictionaryAttr>("condition_location"), declaration,
                [&] { return declaration->emitOpError(); })) ||
            failed(value(row.get("condition"), scope)))
          return failure();
        auto trueEffects = row.getAs<ArrayAttr>("then_effects");
        auto falseEffects = row.getAs<ArrayAttr>("else_effects");
        auto results = row.getAs<ArrayAttr>("results");
        if (!trueEffects || !falseEffects || !results)
          return error("branch arms and results must be arrays");
        if (failed(effects(trueEffects, scope, false, depth + 1, true)) ||
            failed(effects(falseEffects, scope, false, depth + 1, true)))
          return failure();
        for (Attribute rawResult : results) {
          auto result = dyn_cast<DictionaryAttr>(rawResult);
          if (!budget.charge() ||
              failed(exact(result, {"type", "when_true", "when_false"})))
            return failure();
          auto physicalAttr = result.getAs<TypeAttr>("type");
          if (!physicalAttr)
            return error("branch result requires a hardware TypeAttr");
          auto physical = physicalAttr.getValue();
          HardwareBindings bindings;
          bindings.owner = declaration;
          if (!admittedBranchTypes.contains(physical)) {
            if (failed(detail::resolveSourceBranchResultType(
                    physical, hardware, bindings, declaration,
                    [&](uint64_t work, uint64_t bytes) {
                      return success(budget.charge(work) && budget.storage(1, bytes));
                    })) || !budget.storage(1, 3 * sizeof(Type)))
              return failure();
            admittedBranchTypes.insert(physical);
          }
          if (failed(value(result.get("when_true"), scope, physical)) ||
              failed(value(result.get("when_false"), scope, physical)))
            return failure();
        }
        continue;
      }
      if (failed(fact(row.get("path"), scope)))
        return failure();
      if (kind.getValue() == "call") {
        if (guard)
          return error("guard effects permit reads only");
        if (failed(call(row, scope)))
          return failure();
      } else if (kind.getValue() == "formal_write") {
        if (guard || !scope.methodSignature)
          return error("formal write requires a Method body");
        NamedAttrList core(row);
        bool forwarded = bool(row.get("previous")) || bool(row.get("snapshot_type"));
        if (forwarded) {
          if (!row.getAs<DictionaryAttr>("previous") || !row.getAs<TypeAttr>("snapshot_type"))
            return error("forwarded formal previous and snapshot type must be present together");
          core.erase("previous");
          core.erase("snapshot_type");
        }
        if (failed(exact(core.getDictionary(declaration->getContext()),
                         {"kind", "occurrence", "path", "formal", "selectors",
                          "payload_type", "payload", "candidate", "location"})))
          return failure();
        auto formal = row.getAs<IntegerAttr>("formal");
        if (!formal || !formal.getType().isSignlessInteger(64) ||
            formal.getInt() < 0 || static_cast<uint64_t>(formal.getInt()) >=
                                       scope.arguments.size() ||
            !scope.formalProposals ||
            !llvm::is_contained(scope.formalProposals, formal))
          return error("formal write must identify a writable input formal");
        auto selectors = row.getAs<ArrayAttr>("selectors");
        if (!selectors)
          return error("formal write requires a selector array");
        auto selected =
            selectedType(scope.arguments[formal.getInt()], selectors, scope);
        if (failed(selected))
          return failure();
        auto payloadType = row.getAs<TypeAttr>("payload_type");
        if (!payloadType ||
            !areEquivalentHardwareTypes(*selected, payloadType.getValue()))
          return error(
              "formal write payload must match selected input formal type");
        if (forwarded && !selectors.empty())
          return error(
              "forwarded formal proposal must inherit callee write domains");
        if (forwarded &&
            (failed(value(row.get("previous"), scope, scope.arguments[formal.getInt()])) ||
             failed(type(row.get("snapshot_type"))) ||
             !areEquivalentHardwareTypes(row.getAs<TypeAttr>("snapshot_type").getValue(),
                                          scope.arguments[formal.getInt()])))
          return error("forwarded formal snapshot must preserve the whole formal type");
        if (failed(type(row.get("payload_type"))) ||
            failed(value(row.get("payload"), scope,
                         row.getAs<TypeAttr>("payload_type").getValue())) ||
            failed(value(row.get("candidate"), scope,
                         scope.arguments[formal.getInt()])) ||
            failed(detail::verifySourceEffectMetadata(
                row.getAs<DictionaryAttr>("occurrence"),
                row.getAs<DictionaryAttr>("location"), declaration,
                [&] { return declaration->emitOpError(); })))
          return failure();
      } else if (kind.getValue() == "check") {
        if (guard)
          return error("guard effects permit reads only");
        if (failed(exact(row, {"kind", "occurrence", "path", "check_kind",
                               "condition", "location", "message"})))
          return failure();
        auto checkKind = row.getAs<StringAttr>("check_kind");
        Attribute message = row.get("message");
        if (!checkKind || checkKind.getValue() != "assert")
          return error("check effect kind must be assert");
        if (!isa<StringAttr>(message) && !isa<UnitAttr>(message))
          return error("check effect message must be StringAttr or UnitAttr");
        if (failed(detail::verifySourceEffectMetadata(
                row.getAs<DictionaryAttr>("occurrence"),
                row.getAs<DictionaryAttr>("location"), declaration,
                [&] { return declaration->emitOpError(); })) ||
            failed(value(row.get("condition"), scope)))
          return failure();
      } else if (kind.getValue() == "observe") {
        if (guard)
          return error("guard effects permit reads only");
        if (failed(exact(row, {"kind", "occurrence", "path", "observation_kind",
                               "spec", "location", "values"})))
          return failure();
        auto observationKind = row.getAs<StringAttr>("observation_kind");
        auto spec = row.getAs<DictionaryAttr>("spec");
        auto observed = row.getAs<ArrayAttr>("values");
        if (!observationKind ||
            !llvm::is_contained(ArrayRef<StringRef>{"log", "report"},
                                observationKind.getValue()))
          return error("observation effect kind must be log or report");
        if (!observed)
          return error("observation effect values must be an array");
        if (failed(detail::verifySourceEffectMetadata(
                row.getAs<DictionaryAttr>("occurrence"),
                row.getAs<DictionaryAttr>("location"), declaration,
                [&] { return declaration->emitOpError(); })) ||
            failed(detail::verifySourceObservationSpec(
                observationKind, spec, observed.size(),
                [&] { return declaration->emitOpError(); })))
          return failure();
        for (Attribute rawValue : observed) {
          auto item = dyn_cast<DictionaryAttr>(rawValue);
          if (failed(exact(item, {"type", "value"})) ||
              failed(type(item.get("type"))))
            return failure();
          Type physical = item.getAs<TypeAttr>("type").getValue();
          if (!isa<BitsType, EnumType, TypeParamType>(physical))
            return error("observation values require scalar hardware types");
          if (failed(value(item.get("value"), scope, physical)))
            return failure();
        }
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
        if (arm && !isa<BitsType, EnumType, StructType, TableType>(*selected))
          return error("branch arms require selected finite state accesses");
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
  llvm::DenseSet<Type> admittedBranchTypes;
  ValueScope moduleScope;
  llvm::DenseMap<DictionaryAttr, Type> stateRows;
  llvm::StringSet<> stateNames;
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
