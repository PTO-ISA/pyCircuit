#include "ACIRSourceContracts.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceEffects.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/PointerUnion.h"
#include <limits>
#include <type_traits>

using namespace mlir;

namespace acir::ac {
namespace {

HardwareBindings scope(Operation *operation) {
  HardwareBindings bindings;
  bindings.owner = operation->getParentOfType<ModuleOp>();
  return bindings;
}

LogicalResult moduleOccurrence(Operation *operation, DictionaryAttr identity,
                               StringRef description) {
  auto error = [&] { return operation->emitOpError(); };
  if (failed(detail::verifyOccurrence(identity, error)))
    return failure();
  auto owner = operation->getParentOfType<ModuleOp>();
  auto symbol =
      owner ? owner->getAttrOfType<StringAttr>("sym_name") : StringAttr();
  if (!symbol)
    return error() << description << " requires a named owning module";
  return detail::verifyOriginDefinition(
      identity,
      FlatSymbolRefAttr::get(operation->getContext(), symbol.getValue()),
      description, error);
}

LogicalResult occurrence(Operation *operation) {
  return moduleOccurrence(
      operation, operation->getAttrOfType<DictionaryAttr>("occurrence"),
      "typed source operation");
}

LogicalResult namedDeclaration(Operation *operation, StringAttr name) {
  auto module = dyn_cast_or_null<ModuleOp>(operation->getParentOp());
  if (!module || !name)
    return operation->emitOpError() << "requires named direct module placement";
  if (failed(detail::verifyPythonAstIdentifier(
          name.getValue(), true, [&] { return operation->emitOpError(); })))
    return failure();
  return occurrence(operation);
}

LogicalResult predicate(const HardwareAnalysis &analysis, Value value,
                        const HardwareBindings &bindings, Operation *site,
                        StringRef description) {
  if (!isa<BitsType>(value.getType()))
    return site->emitOpError() << description << " requires width-one Bits";
  auto width = analysis.getPackedWidth(value.getType(), bindings, site);
  if (failed(width))
    return failure();
  if (*width != 1)
    return site->emitOpError() << description << " requires width-one Bits";
  return success();
}

bool equivalentTypes(TypeRange left, TypeRange right) {
  if (left.size() != right.size())
    return false;
  for (auto [lhs, rhs] : llvm::zip(left, right))
    if (!areEquivalentHardwareTypes(lhs, rhs))
      return false;
  return true;
}

LogicalResult transactionPlacement(Operation *operation, bool allowGuard,
                                   bool allowBranch = false) {
  auto *placed = operation;
  auto *parent = placed->getParentOp();
  while (allowBranch && isa_and_nonnull<SourceBranchOp>(parent)) {
    placed = parent;
    parent = placed->getParentOp();
  }
  if (!isa_and_nonnull<SourceMethodOp, SourceTransactionOp>(parent))
    return operation->emitOpError() << "requires direct method or transaction "
                                       "region placement, or legal branch arms";
  if ((!allowGuard || placed != operation) &&
      placed->getParentRegion() != &parent->getRegion(1))
    return operation->emitOpError()
           << "state writes and method calls are forbidden in guards";
  return occurrence(operation);
}

LogicalResult region(Operation *operation, unsigned ordinal, TypeRange inputs,
                     TypeRange outputs) {
  auto &region = operation->getRegion(ordinal);
  bool guard = ordinal == 0;
  if (!region.hasOneBlock() || region.front().empty() ||
      !equivalentTypes(region.front().getArgumentTypes(), inputs) ||
      !isa<YieldOp>(region.front().back()))
    return operation->emitOpError()
           << "guard/body requires one block, matching arguments and final "
              "ac.yield";
  auto &block = region.front();
  auto &yield = block.back();
  HardwareAnalysis analysis(operation->getParentOfType<mlir::ModuleOp>());
  if (guard) {
    if (yield.getNumOperands() != 1)
      return yield.emitOpError() << "guard must yield exactly one predicate";
    if (failed(predicate(analysis, yield.getOperand(0), scope(operation),
                         &yield, "guard yield")))
      return failure();
  } else if (!equivalentTypes(yield.getOperandTypes(), outputs)) {
    return yield.emitOpError() << "body yield types do not match results";
  }
  for (Operation &nested : block.without_terminator()) {
    if (isa<SourceStateReadOp>(nested))
      continue;
    if (!guard &&
        isa<SourceStateWriteOp, SourceFormalWriteOp, SourceMethodCallOp,
            SourceExpectOp, SourceObserveOp, SourceBranchOp>(nested))
      continue;
    if (!detail::isCombinationalHardwareOperation(&nested))
      return nested.emitOpError()
             << (guard ? "guard accepts only closed pure payload operations "
                         "and state reads"
                       : "body accepts only closed payload operations, state "
                         "accesses, method calls, checks and observations");
    // Pure nested collection regions do not open another effect placement.
    auto walk =
        nested.walk([&](Operation *child) {
          if (child == &nested ||
              detail::isCombinationalHardwareOperation(child) ||
              isa<YieldOp>(child))
            return WalkResult::advance();
          child->emitOpError()
              << "nested payload region cannot contain transaction "
                 "effects or declarations";
          return WalkResult::interrupt();
        });
    if (walk.wasInterrupted())
      return failure();
  }
  return success();
}

// Structured arms are the only nested regions that can carry transaction
// effects. Pure collection regions remain closed even when enclosed by a
// branch.
LogicalResult branchPayload(Operation *operation, bool arm, unsigned depth,
                            uint64_t &work) {
  if (depth > 128 || ++work > 1024 * 1024)
    return operation->emitOpError()
           << "source branch verification budget exceeded";
  if (arm && isa<SourceBranchOp>(operation)) {
    for (Region &nested : operation->getRegions())
      for (Block &block : nested)
        for (Operation &child : block)
          if (failed(branchPayload(&child, true, depth + 1, work)))
            return failure();
    return success();
  }
  if (arm && isa<SourceStateReadOp, SourceStateWriteOp>(operation)) {
    auto selectors = operation->getAttrOfType<ArrayAttr>("selectors");
    Value payload = isa<SourceStateReadOp>(operation)
                        ? operation->getResult(0)
                        : operation->getOperand(1);
    if (!selectors ||
        !isa<BitsType, EnumType, StructType, TableType>(payload.getType()))
      return operation->emitOpError()
             << "source branch arms require selected finite state accesses";
    return success();
  }
  if (arm &&
      isa<SourceFormalWriteOp, SourceMethodCallOp, SourceExpectOp,
          SourceObserveOp>(operation))
    return success();
  if (!isa<YieldOp>(operation) &&
      !detail::isCombinationalHardwareOperation(operation))
    return operation->emitOpError()
           << "source branch arms accept only closed pure payload operations, "
              "selected finite state accesses, Method calls, checks, "
              "observations "
              "and nested branches";
  for (Region &nested : operation->getRegions())
    for (Block &block : nested)
      for (Operation &child : block)
        if (failed(branchPayload(&child, false, depth + 1, work)))
          return failure();
  return success();
}

FailureOr<Type> selectedPayloadType(Operation *operation, Type root,
                                    ArrayAttr selectors, ValueRange indices) {
  HardwareAnalysis analysis(operation->getParentOfType<mlir::ModuleOp>());
  auto bindings = scope(operation);
  auto resolved = analysis.resolveType(root, bindings, operation);
  if (failed(resolved))
    return failure();
  Type current = *resolved;
  unsigned nextIndex = 0;
  for (Attribute raw : selectors) {
    auto selector = dyn_cast<DictionaryAttr>(raw);
    if (!selector || selector.size() != 1)
      return operation->emitOpError()
             << "state selector requires exactly field or index";
    if (auto field = selector.getAs<StringAttr>("field")) {
      auto record = dyn_cast<StructType>(current);
      auto declaration = record ? analysis.lookupStruct(record) : StructOp();
      if (!declaration)
        return operation->emitOpError()
               << "state field selector requires a resolved nominal struct";
      Type selected;
      for (Attribute rawField : declaration.getFields()) {
        auto item = cast<DictionaryAttr>(rawField);
        if (item.getAs<StringAttr>("name") == field) {
          selected = item.getAs<TypeAttr>("type").getValue();
          break;
        }
      }
      if (!selected)
        return operation->emitOpError()
               << "unknown state field '" << field.getValue() << "'";
      auto resolvedField = analysis.resolveType(selected, bindings, operation);
      if (failed(resolvedField))
        return failure();
      current = *resolvedField;
      continue;
    }
    auto ordinal = selector.getAs<IntegerAttr>("index");
    auto table = dyn_cast<TableType>(current);
    if (!ordinal || !ordinal.getType().isSignlessInteger(64) ||
        ordinal.getValue().isNegative() ||
        ordinal.getValue().getZExtValue() != nextIndex ||
        nextIndex >= indices.size() || !table || table.getShape().empty())
      return operation->emitOpError()
             << "state index selector requires a Table and sequential i64 "
                "SSA index ordinal";
    auto indexWidth = analysis.getPackedWidth(indices[nextIndex].getType(),
                                              bindings, operation);
    if (!isa<BitsType>(indices[nextIndex].getType()) || failed(indexWidth))
      return operation->emitOpError() << "state index must have resolved Bits "
                                         "type";
    auto shape = analysis.getTableShape(table, bindings, operation);
    if (failed(shape))
      return failure();
    ++nextIndex;
    if (table.getShape().size() == 1)
      current = table.getElementType();
    else
      current = TableType::get(
          operation->getContext(),
          ArrayAttr::get(operation->getContext(),
                         table.getShape().getValue().drop_front()),
          table.getElementType());
  }
  if (nextIndex != indices.size())
    return operation->emitOpError()
           << "state selector index count must match SSA indices exactly";
  return current;
}

FailureOr<Type> selectedStateType(Operation *operation, ArrayAttr selectors,
                                  ValueRange indices) {
  auto owner = operation->getParentOfType<ModuleOp>();
  auto name = operation->getAttrOfType<StringAttr>("state");
  SourceStateOp state;
  if (owner && owner.getBody().hasOneBlock() && name)
    for (auto candidate : owner.getBody().front().getOps<SourceStateOp>())
      if (candidate->getAttrOfType<StringAttr>("instance_name") == name) {
        if (state)
          return operation->emitOpError() << "state reference is ambiguous";
        state = candidate;
      }
  if (!state)
    return operation->emitOpError()
           << "state reference must resolve to a state in the nearest module";
  if (state->getNumOperands() != 3 || state->getNumResults() != 1 ||
      state->getNumRegions() != 0)
    return state.emitOpError()
           << "state requires clock, reset, initial value and one old-Q result";
  if (failed(state.verifyInvariantsImpl()))
    return failure();
  return selectedPayloadType(operation, state.getQ().getType(), selectors,
                             indices);
}

LogicalResult staticInitializer(const HardwareAnalysis &analysis, Value initial,
                                const HardwareBindings &bindings,
                                Operation *state) {
  // Prove closure of the existing literal/constructor graph. This performs
  // no SSA arithmetic evaluation and never obtains values from runtime wires.
  struct Frame {
    Operation *operation;
    unsigned nextOperand = 0;
  };
  SmallVector<Frame> stack;
  llvm::DenseSet<Operation *> active, complete;
  auto push = [&](Value value) -> LogicalResult {
    auto *operation = value.getDefiningOp();
    if (!operation || operation->getParentOp() != bindings.owner ||
        !isa<BitsConstantOp, BitsConstant4Op, EnumCreateOp, StructCreateOp,
             TableCreateOp, TableSplatOp, StructGetOp, EnumToBitsOp,
             BitsExtractOp, BitsResizeOp>(operation))
      return state->emitOpError()
             << "state initializer requires a closed static "
                "literal/constructor "
                "DAG; runtime values and unsupported computations are rejected";
    if (!active.insert(operation).second)
      return state->emitOpError()
             << "state initializer constructor DAG is cyclic";
    if (failed(mlir::verify(operation)) ||
        failed(analysis.verifyResolvedOperation(operation, bindings)))
      return failure();
    stack.push_back({operation});
    return success();
  };
  if (failed(push(initial)))
    return failure();
  while (!stack.empty()) {
    auto &frame = stack.back();
    if (frame.nextOperand == frame.operation->getNumOperands()) {
      complete.insert(frame.operation);
      active.erase(frame.operation);
      stack.pop_back();
      continue;
    }
    Value input = frame.operation->getOperand(frame.nextOperand++);
    if (complete.contains(input.getDefiningOp()))
      continue;
    if (failed(push(input)))
      return failure();
  }
  return success();
}

} // namespace

LogicalResult detail::verifyClosedStateInitializer(
    const HardwareAnalysis &analysis, Value initial,
    const HardwareBindings &bindings, Operation *site) {
  return staticInitializer(analysis, initial, bindings, site);
}

FailureOr<Type> detail::resolveSourceBranchResultType(
    Type actualType, const HardwareAnalysis &analysis,
    const HardwareBindings &lexicalBindings, Operation *diagnosticSite,
    llvm::function_ref<LogicalResult(uint64_t, uint64_t)> charge) {
  auto error = [&] {
    return mlir::emitError(UnknownLoc::get(diagnosticSite->getContext()));
  };
  auto *owner = lexicalBindings.owner;
  auto package = analysis.getPackage();
  auto *lexical = isa<ModuleOp, ModuleImportOp>(diagnosticSite)
                      ? diagnosticSite
                      : diagnosticSite->getParentOfType<ModuleOp>().getOperation();
  if (!owner || owner != lexical || !isa<ModuleOp, ModuleImportOp>(owner) ||
      owner->getParentOp() != package.getOperation())
    return error() << "source branch result requires actual lexical bindings";
  auto ownerName = owner->getAttrOfType<StringAttr>("sym_name");
  if (!ownerName || failed(charge(1 + ownerName.getValue().size(), 0)) ||
      analysis.lookupDefinition(ownerName.getValue()) != owner)
    return error() << "source branch result bindings have a foreign owner";

  // Traverse raw metadata with an explicit, paid stack. No recursive type or
  // declaration validator runs until the complete reachable closure is paid.
  using Node = llvm::PointerUnion<Attribute, Type>;
  struct RawNode {
    Node node;
    unsigned depth;
    bool exiting = false, substituted = false, expressionTree = false;
  };
  SmallVector<RawNode, 0> raw;
  SmallVector<Node, 0> activeMetadata;
  uint64_t nodes = 0, dictionarySlots = 0, arraySlots = 0;
  uint64_t dictionaries = 0, arrays = 0, expressions = 0;
  uint64_t substitutionNodes = 0, substitutionTextBytes = 0, typeFormals = 0;
  uint64_t integerBytes = 0;
  uint64_t enumMembers = 0, enumTextBytes = 0, enumOccurrences = 0;
  uint64_t enumParseBytes = 0;
  uint64_t bitsCalls = 0, fieldPathSlots = 0, shapeSlots = 0, shapeArrays = 0;
  unsigned metadataDepth = 0, nominalDepth = 0;
  auto add = [](uint64_t a, uint64_t c) {
    return c > std::numeric_limits<uint64_t>::max() - a
               ? std::numeric_limits<uint64_t>::max() : a + c;
  };
  auto multiply = [](uint64_t a, uint64_t c) {
    return c && a > std::numeric_limits<uint64_t>::max() / c
               ? std::numeric_limits<uint64_t>::max() : a * c;
  };
  auto pay = [&](uint64_t work, uint64_t bytes = 0) -> LogicalResult {
    if (work == std::numeric_limits<uint64_t>::max() ||
        bytes == std::numeric_limits<uint64_t>::max())
      return error() << "source branch result closure cost overflow";
    return charge(work, bytes);
  };
  auto reserveOne = [&](auto &values) -> LogicalResult {
    if (values.size() < values.capacity()) return success();
    using Element = typename std::decay_t<decltype(values)>::value_type;
    uint64_t requested = add(values.size(), 1);
    uint64_t capacity = std::max(requested, add(multiply(values.capacity(), 2), 1));
    if (capacity > values.max_size())
      return error() << "source branch result closure storage overflow";
    if (failed(pay(values.size(), multiply(capacity, sizeof(Element)))))
      return failure();
    values.reserve(capacity);
    return success();
  };
  auto appendRaw = [&](Node node, unsigned depth,
                       bool substituted = false,
                       bool expressionTree = false) -> LogicalResult {
    if (!node || depth > 128)
      return error() << "source branch result metadata depth exceeded";
    metadataDepth = std::max(metadataDepth, depth);
    if (failed(pay(1)) || failed(reserveOne(raw))) return failure();
    if (auto attr = node.dyn_cast<Attribute>())
      substituted |= isa<StaticExprAttr>(attr);
    raw.push_back({node, depth, false, substituted, expressionTree});
    return success();
  };
  auto metadata = [&](Node root) -> LogicalResult {
    if (failed(appendRaw(root, 0)))
      return failure();
    while (!raw.empty()) {
      auto entry = raw.pop_back_val();
      if (entry.exiting) {
        activeMetadata.pop_back();
        continue;
      }
      if (failed(pay(1 + activeMetadata.size())) ||
          failed(reserveOne(activeMetadata))) return failure();
      if (llvm::is_contained(activeMetadata, entry.node))
        return error() << "recursive source branch result metadata or nominal declaration";
      activeMetadata.push_back(entry.node);
      nodes = add(nodes, 1);
      if (entry.substituted)
        substitutionNodes = add(substitutionNodes, 1);
      if (failed(appendRaw(entry.node, entry.depth))) return failure();
      raw.back().exiting = true;
      bool valid = true;
      auto attr = [&](Attribute a) {
        bool substituted = entry.substituted;
        if (entry.expressionTree) {
          auto tree = cast<DictionaryAttr>(entry.node.get<Attribute>());
          if (a == tree.get("origin") || a == tree.get("location"))
            substituted = false;
        }
        if (valid && failed(appendRaw(Node(a), entry.depth + 1,
                                     substituted))) valid = false;
      };
      auto type = [&](Type t) {
        if (valid && failed(appendRaw(Node(t), entry.depth + 1))) valid = false;
      };
      if (auto a = entry.node.dyn_cast<Attribute>()) {
        uint64_t bytes = 0;
        if (auto text = dyn_cast<StringAttr>(a)) bytes = text.getValue().size();
        if (auto integer = dyn_cast<IntegerAttr>(a))
          bytes = (uint64_t(integer.getValue().getBitWidth()) + 7) / 8;
        if (entry.substituted) {
          substitutionTextBytes = add(substitutionTextBytes, bytes);
          if (auto dictionary = dyn_cast<DictionaryAttr>(a)) {
            dictionaries = add(dictionaries, 1);
            dictionarySlots = add(dictionarySlots, dictionary.size());
          }
          if (auto array = dyn_cast<ArrayAttr>(a)) {
            arrays = add(arrays, 1);
            arraySlots = add(arraySlots, array.size());
          }
        }
        if (isa<StaticExprAttr>(a)) expressions = add(expressions, 1);
        if (auto integer = dyn_cast<MathIntAttr>(a))
          integerBytes = add(integerBytes, integer.getCanonicalValue().size());
        if (failed(pay(1 + bytes))) return failure();
        if (auto expression = dyn_cast<StaticExprAttr>(a)) {
          if (failed(appendRaw(Node(expression.getTree()), entry.depth + 1,
                               true, true))) valid = false;
        } else a.walkImmediateSubElements(attr, type);
      } else {
        if (failed(pay(1))) return failure();
        Type current = entry.node.get<Type>();
        if (auto table = dyn_cast<TableType>(current)) {
          shapeSlots = add(shapeSlots, table.getShape().size());
          shapeArrays = add(shapeArrays, 1);
        }
        typeFormals += isa<TypeParamType>(current);
        current.walkImmediateSubElements(attr, type);
        Operation *declaration = nullptr;
        if (auto record = dyn_cast<StructType>(current)) {
          if (!record.getName()) return error() << "malformed nominal Struct type";
          declaration = analysis.lookupStruct(record);
        } else if (auto enumeration = dyn_cast<EnumType>(current)) {
          if (!enumeration.getName()) return error() << "malformed nominal Enum type";
          declaration = analysis.lookupEnum(enumeration);
        }
        if (isa<StructType, EnumType>(current)) {
          if (!declaration || declaration->getParentOp() != package.getOperation())
            return error() << "unresolved source branch nominal declaration";
          if (auto enumeration = dyn_cast<EnumType>(current)) {
            ++enumOccurrences;
            if (auto width = declaration->getAttrOfType<MathIntAttr>("width"))
              enumParseBytes = add(enumParseBytes, width.getCanonicalValue().size());
            auto members = declaration->getAttrOfType<ArrayAttr>("members");
            if (members) {
              if (failed(pay(members.size()))) return failure();
              enumMembers = add(enumMembers, members.size());
              for (Attribute rawMember : members)
                if (auto member = dyn_cast<DictionaryAttr>(rawMember)) {
                  if (auto name = member.getAs<StringAttr>("name"))
                    enumTextBytes = add(enumTextBytes, name.getValue().size() + 1);
                  if (auto code = member.getAs<MathIntAttr>("code")) {
                    enumTextBytes = add(enumTextBytes, code.getCanonicalValue().size() + 1);
                    enumParseBytes = add(enumParseBytes, code.getCanonicalValue().size());
                  }
                }
            }
          }
          if (failed(pay(1, declaration->getPropertiesStorageSize()))) return failure();
          // Native static substitution borrows nominal TypeAttrs; it never
          // reconstructs declaration dictionaries, arrays or locations. Keep
          // their raw traversal separate from actual substitution storage.
          for (Attribute metadata : {Attribute(declaration->getRawDictionaryAttrs()),
                                     Attribute(declaration->getLoc())})
            if (valid && failed(appendRaw(Node(metadata), entry.depth + 1)))
              valid = false;
          if (valid && declaration->getPropertiesStorageSize() &&
              failed(appendRaw(Node(declaration->getPropertiesAsAttribute()),
                               entry.depth + 1)))
            valid = false;
        }
      }
      if (!valid) return failure();
    }
    return success();
  };
  for (const auto &binding : lexicalBindings.integers) {
    if (failed(pay(1 + binding.getKey().size())) ||
        failed(metadata(Node(binding.second)))) return failure();
  }
  for (const auto &binding : lexicalBindings.types) {
    if (failed(pay(1 + binding.getKey().size())) ||
        failed(metadata(Node(binding.second)))) return failure();
  }
  // Scope checking reads these inventories. Inspect them before it can recurse.
  for (StringRef name : {"type_parameters", "parameters"})
    if (auto attr = owner->getAttr(name))
      if (failed(metadata(Node(attr)))) return failure();

  struct Frame { Type type; ArrayAttr fields; size_t next = 0; bool entered = false; };
  SmallVector<Frame, 0> stack;
  SmallVector<Type, 0> validated;
  auto push = [&](Type type) -> LogicalResult {
    if (!type || stack.size() > 128)
      return error() << "source branch result nominal depth exceeded";
    if (failed(pay(1 + stack.size())) || failed(reserveOne(stack)))
      return failure();
    for (const Frame &ancestor : stack)
      if (ancestor.type == type)
        return error() << "recursive source branch result nominal declaration";
    stack.push_back({type, {}, 0, false});
    nominalDepth = std::max(nominalDepth, unsigned(stack.size()));
    return success();
  };
  if (failed(metadata(Node(actualType))) || failed(push(actualType))) return failure();
  while (!stack.empty()) {
    auto &frame = stack.back();
    if (!frame.entered) {
      frame.entered = true;
      if (!isa<BitsType, EnumType, StructType, TableType>(frame.type))
        return error() << "source branch results require finite hardware payloads";
      Operation *declaration = nullptr;
      if (auto record = dyn_cast<StructType>(frame.type))
        declaration = analysis.lookupStruct(record);
      else if (auto enumeration = dyn_cast<EnumType>(frame.type))
        declaration = analysis.lookupEnum(enumeration);
      if (isa<EnumType, StructType>(frame.type)) {
        if (!declaration || declaration->getParentOp() != package.getOperation())
          return error() << "unresolved source branch nominal declaration";
        auto name = declaration->getAttrOfType<StringAttr>("sym_name");
        if (!name || name.getValue().empty())
          return error() << "source branch nominal declaration requires a name";
        if (isa<StructOp>(declaration)) {
          frame.fields = declaration->getAttrOfType<ArrayAttr>("fields");
          if (!frame.fields || frame.fields.empty() || declaration->hasAttr("constructor"))
            return error() << "invalid source branch Struct declaration";
          for (auto [i, raw] : llvm::enumerate(frame.fields)) {
            auto field = dyn_cast<DictionaryAttr>(raw);
            auto key = field ? field.getAs<StringAttr>("name") : StringAttr();
            auto type = field ? field.getAs<TypeAttr>("type") : TypeAttr();
            if (!field || field.size() != 2 || !key || key.getValue().empty() || !type)
              return error() << "source branch Struct fields require name and type";
            if (failed(pay(i + key.getValue().size()))) return failure();
            for (Attribute previous : frame.fields.getValue().take_front(i))
              if (cast<DictionaryAttr>(previous).getAs<StringAttr>("name") == key)
                return error() << "source branch Struct field names must be unique";
          }
        } else {
          auto width = declaration->getAttrOfType<MathIntAttr>("width");
          auto encoding = declaration->getAttrOfType<StringAttr>("encoding");
          auto members = declaration->getAttrOfType<ArrayAttr>("members");
          if (!width || !encoding || !members || members.empty())
            return error() << "invalid source branch Enum declaration";
          for (Attribute raw : members) {
            auto member = dyn_cast<DictionaryAttr>(raw);
            if (!member || member.size() != 2 || !member.getAs<StringAttr>("name") ||
                !member.getAs<MathIntAttr>("code"))
              return error() << "invalid source branch Enum member";
          }
        }
      }
    }
    if (auto table = dyn_cast<TableType>(frame.type); table && frame.next == 0) {
      ++frame.next;
      if (failed(TableType::verify(error, table.getShape(), table.getElementType())) ||
          failed(push(table.getElementType()))) return failure();
      continue;
    }
    if (frame.fields && frame.next < frame.fields.size()) {
      Type child = cast<DictionaryAttr>(frame.fields[frame.next++])
                       .getAs<TypeAttr>("type").getValue();
      // Table roots carry existing scalar element layouts. A Table inside a
      // nominal Struct is still outside the current declared Struct contract.
      if (isa<TableType>(child))
        return error() << "source branch Struct fields cannot contain Tables";
      fieldPathSlots = add(fieldPathSlots, stack.size());
      if (failed(push(child))) return failure();
      continue;
    }
    if (failed(pay(1)) || failed(reserveOne(validated))) return failure();
    bitsCalls += isa<BitsType>(frame.type);
    validated.push_back(frame.type);
    stack.pop_back();
  }
  // Grammar validation owns one completed DenseMap and active DenseSet per
  // Bits width. LLVM starts at 64 buckets and doubles before 3/4 occupancy;
  // twice the final bound covers every allocation over that invocation.
  uint64_t grammarBuckets = multiply(2, add(multiply(64, bitsCalls),
                                            multiply(4, expressions)));
  uint64_t grammarBytes = multiply(grammarBuckets,
      sizeof(std::pair<DictionaryAttr, bool>) + sizeof(DictionaryAttr));
  // Native substitution visits computational expression children and borrows
  // source origin/location trees unchanged. Those trees still paid their full
  // raw traversal above and native grammar/scope validation below. Summed temporary
  // vector allocations stay below four times the logical handle count;
  // interning stores another copy. Charge cumulative allocation, including
  // buffers freed during growth. Strings in these records remain borrowed.
  uint64_t cloneBytes = add(
      multiply(dictionarySlots, 5 * sizeof(NamedAttribute)),
      multiply(add(arraySlots, shapeSlots), 5 * sizeof(Attribute)));
  cloneBytes = add(cloneBytes, multiply(add(add(dictionaries, arrays), shapeArrays),
                                       3 * sizeof(Attribute)));
  cloneBytes = add(cloneBytes, multiply(expressions, 2 * sizeof(Attribute)));
  // resolveType substitutes once; evaluateStatic can substitute each nested
  // expression once more. Explicit binding chains add their measured length.
  uint64_t substitutionPasses = add(1 + metadataDepth,
      add(lexicalBindings.types.size(), lexicalBindings.integers.size()));
  uint64_t substitutionBytes = multiply(cloneBytes, substitutionPasses);
  // Enum resolution owns a member vector and two StringSets. Each StringMap
  // starts at 16 buckets, doubles at 3/4 occupancy, and copies its own keys.
  // Layout resolves each Enum twice; resolving a standalone root adds one.
  uint64_t enumBuckets = multiply(4, add(multiply(16, enumOccurrences),
                                         multiply(4, enumMembers)));
  uint64_t enumBytes = multiply(enumBuckets, sizeof(void *) + sizeof(unsigned));
  enumBytes = add(enumBytes, multiply(enumMembers,
      4 * sizeof(EnumMember) + 2 * sizeof(llvm::StringMapEntry<std::nullopt_t>)));
  enumBytes = add(enumBytes, enumTextBytes);
  enumBytes = multiply(enumBytes, isa<EnumType>(actualType) ? 3 : 2);
  // getPackedWidth keeps an active Type set and copies field-path handles
  // twice plus possible vector growth at each actual field edge.
  uint64_t layoutBytes = multiply(2 * (64 + 4 * uint64_t(nominalDepth)), sizeof(Type));
  layoutBytes = add(layoutBytes, multiply(fieldPathSlots, 4 * sizeof(StringAttr)));
  if (isa<BitsType>(actualType)) layoutBytes = 0;
  uint64_t traversalBytes = multiply(2 * (metadataDepth + 1),
                                     sizeof(std::pair<Attribute, Operation *>));
  // Prepay parsing/copying of existing numeric spellings separately from
  // traversal work. Arbitrary evaluated APInt result precision remains an
  // inherited evaluator limitation; source size does not bound that allocation.
  uint64_t spellingBytes = multiply(integerBytes, add(substitutionPasses, 2));
  uint64_t nativeBytes = add(add(grammarBytes, substitutionBytes),
      add(add(enumBytes, layoutBytes), add(traversalBytes, spellingBytes)));
  // Only computational expression fields undergo repeated substitution.
  // Provenance and declaration metadata retain three complete native visits
  // for grammar/scope/layout checks, in addition to the raw traversal above;
  // they no longer pay for metadata copies the evaluator does not perform.
  uint64_t substitutionVisits =
      add(add(substitutionNodes, dictionarySlots), arraySlots);
  uint64_t nativeWork = add(multiply(nodes - substitutionNodes, 3),
      add(multiply(add(substitutionVisits, shapeSlots), add(substitutionPasses, 3)),
          multiply(substitutionTextBytes, substitutionPasses)));
  // Enum layout resolves each leaf twice (three times for a standalone root).
  // Each resolution revisits members, hashes names/codes and parses numeric
  // spellings; these are distinct from expression substitution visits.
  uint64_t enumVisits = add(add(enumOccurrences, enumMembers),
                           add(enumTextBytes, enumParseBytes));
  nativeWork = add(nativeWork,
      multiply(enumVisits, isa<EnumType>(actualType) ? 3 : 2));
  nativeWork = add(nativeWork, add(validated.size(), fieldPathSlots));
  // Scope verification scans the actual owner's formal inventories at each
  // reference. Counting every expression as a parameter reference is a safe
  // bound and retains the cost of repeated owner/name comparisons above.
  auto parameters = owner->getAttrOfType<ArrayAttr>("parameters");
  auto typeParameters = owner->getAttrOfType<ArrayAttr>("type_parameters");
  nativeWork = add(nativeWork,
      add(multiply(expressions, parameters ? parameters.size() : 0),
          multiply(typeFormals, typeParameters ? typeParameters.size() : 0)));
  if (failed(pay(nativeWork, nativeBytes))) return failure();
  for (Type type : validated) {
    if (auto bits = dyn_cast<BitsType>(type)) {
      if (failed(BitsType::verify(error, bits.getWidth()))) return failure();
    } else if (auto enumeration = dyn_cast<EnumType>(type)) {
      if (failed(EnumType::verify(error, enumeration.getName()))) return failure();
    } else if (auto record = dyn_cast<StructType>(type)) {
      if (failed(StructType::verify(error, record.getName()))) return failure();
    }
    if (failed(detail::verifyHardwareTypeScope(type, owner))) return failure();
  }
  if (!isa<BitsType>(actualType)) {
    auto width = analysis.getPackedWidth(actualType, lexicalBindings, diagnosticSite);
    if (failed(width) || !*width) return failure();
  }
  return analysis.resolveType(actualType, lexicalBindings, diagnosticSite);
}

LogicalResult SourceStateOp::verify() {
  auto package = (*this)->getParentOfType<mlir::ModuleOp>();
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage")
                       : StringAttr();
  if (stage && stage.getValue() == "final")
    return emitOpError() << "logical state is forbidden in a final package";
  if ((*this)->hasAttr("sym_name"))
    return emitOpError() << "state uses instance_name; sym_name is unsupported";
  if (failed(namedDeclaration(*this, getInstanceNameAttr())))
    return failure();
  auto owner = cast<ModuleOp>((*this)->getParentOp());
  for (Operation &other : owner.getBody().front())
    if (&other != getOperation() &&
        isa<SourceStateOp, InstanceOp, CollectionOp>(other) &&
        other.getAttrOfType<StringAttr>("instance_name") ==
            getInstanceNameAttr())
      return emitOpError() << "state instance_name must be unique across state "
                              "and child instance declarations";
  for (auto other : owner.getBody().front().getOps<SourceStateOp>()) {
    if (other == *this)
      continue;
    if (failed(occurrence(other)))
      return failure();
    if (detail::compareClosedSourceStructure(getOccurrence(),
                                             other.getOccurrence()) == 0)
      return emitOpError() << "state declarations require distinct occurrence "
                              "identities within their module";
  }
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto bindings = scope(*this);
  if (failed(predicate(analysis, getClk(), bindings, *this, "state clock")) ||
      failed(predicate(analysis, getRst(), bindings, *this, "state reset")))
    return failure();
  if (!areEquivalentHardwareTypes(getInitial().getType(), getQ().getType()))
    return emitOpError() << "initial value and old-Q payload types must match";
  if (Attribute constraint = (*this)->getAttr("ac.payload_constraint"))
    if (failed(detail::verifySourceCallHardwareConstraint(
            constraint, getQ().getType(), analysis, owner, *this,
            "state payload")))
      return failure();
  if (stage && stage.getValue() == "routed") {
    StateUpdateOp pairedUpdate;
    for (Operation *user : getQ().getUsers()) {
      auto update = dyn_cast<StateUpdateOp>(user);
      if (!update || update->getBlock() != (*this)->getBlock() ||
          update->getNumOperands() != 3 || update.getState() != getQ())
        continue;
      if (pairedUpdate && pairedUpdate != update)
        return emitOpError()
               << "routed logical state requires exactly one same-block update";
      // Q may also be this update's candidate, so its users can repeat it.
      pairedUpdate = update;
    }
    if (!pairedUpdate)
      return emitOpError()
             << "routed logical state requires exactly one same-block update";
  }
  return staticInitializer(analysis, getInitial(), bindings, *this);
}

void StateUpdateOp::getEffects(
    llvm::SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(MemoryEffects::Write::get(), SourceStateResource::get());
}

LogicalResult StateUpdateOp::verify() {
  auto owner = dyn_cast_or_null<ModuleOp>((*this)->getParentOp());
  auto package = owner
                     ? dyn_cast_or_null<mlir::ModuleOp>(owner->getParentOp())
                     : mlir::ModuleOp();
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage")
                       : StringAttr();
  auto contract = package
                      ? package->getAttrOfType<IntegerAttr>("ac.routed_contract")
                      : IntegerAttr();
  if (!owner || !package || !stage || stage.getValue() != "routed" ||
      package->hasAttr("ac.unit_kind") || !contract ||
      !contract.getType().isSignlessInteger(64) || contract.getValue() != 2)
    return emitOpError()
           << "requires direct module graph placement in a routed contract-2 "
              "package";
  if (!(*this)->getAttrs().empty())
    return emitOpError() << "state update does not accept attributes";
  auto state = getState().getDefiningOp<SourceStateOp>();
  if (!state || state->getBlock() != (*this)->getBlock())
    return emitOpError()
           << "state must be the direct result of a same-module ac.state";
  if (!areEquivalentHardwareTypes(getState().getType(),
                                  getCandidate().getType()))
    return emitOpError() << "candidate must exactly match the state payload type";
  HardwareAnalysis analysis(package);
  return predicate(analysis, getIntent(), scope(*this), *this,
                   "state update intent");
}

LogicalResult SourceStateReadOp::verify() {
  if (failed(transactionPlacement(*this, true, true)))
    return failure();
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  if (failed(predicate(analysis, getPath(), scope(*this), *this,
                       "state read path")))
    return failure();
  auto selected = selectedStateType(*this, getSelectors(), getIndices());
  if (failed(selected))
    return failure();
  if (!areEquivalentHardwareTypes(*selected, getValue().getType()))
    return emitOpError()
           << "state read result must match selected payload type";
  return success();
}

LogicalResult SourceStateWriteOp::verify() {
  if (failed(transactionPlacement(*this, false, true)))
    return failure();
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  if (failed(predicate(analysis, getPath(), scope(*this), *this,
                       "state write path")))
    return failure();
  auto selected = selectedStateType(*this, getSelectors(), getIndices());
  if (failed(selected))
    return failure();
  if (!areEquivalentHardwareTypes(*selected, getValue().getType()))
    return emitOpError()
           << "state write value must match selected payload type";
  return success();
}

LogicalResult SourceFormalWriteOp::verify() {
  for (NamedAttribute attr : (*this)->getAttrs())
    if (!llvm::is_contained(ArrayRef<StringRef>{"formal", "selectors",
                                                "operandSegmentSizes",
                                                "occurrence", "ac.location"},
                            attr.getName().getValue()))
      return emitOpError() << "unknown formal write attribute";
  if (failed(transactionPlacement(*this, false, true)))
    return failure();
  auto method = (*this)->getParentOfType<SourceMethodOp>();
  if (!method)
    return emitOpError() << "formal write requires a lexical Method formal";
  auto formal = getFormalAttr();
  auto signature = method.getFunctionType();
  auto proposals = method->getAttrOfType<ArrayAttr>("ac.formal_proposals");
  if (!formal.getType().isSignlessInteger(64) || formal.getInt() < 0 ||
      static_cast<uint64_t>(formal.getInt()) >= signature.getNumInputs() ||
      !proposals || !llvm::is_contained(proposals, formal))
    return emitOpError()
           << "formal write must identify a declared writable input formal";
  if (!areEquivalentHardwareTypes(getCandidate().getType(),
                                  signature.getInput(formal.getInt())))
    return emitOpError()
           << "formal write candidate must match the whole input formal type";
  auto selected = selectedPayloadType(
      *this, signature.getInput(formal.getInt()), getSelectors(), getIndices());
  if (failed(selected))
    return failure();
  if (!areEquivalentHardwareTypes(*selected, getPayload().getType()))
    return emitOpError()
           << "formal write payload must match selected input formal type";
  if (bool(getPrevious()) != bool(getSnapshot()))
    return emitOpError() << "forwarded formal previous and snapshot must be present together";
  if (getPrevious() &&
      (!areEquivalentHardwareTypes(getPrevious().getType(), getCandidate().getType()) ||
       !areEquivalentHardwareTypes(getSnapshot().getType(), getCandidate().getType())))
    return emitOpError() << "forwarded formal snapshot must preserve the whole formal type";
  if (getPrevious() && (!getSelectors().empty() || !getIndices().empty()))
    return emitOpError()
           << "forwarded formal proposal must inherit callee write domains";
  auto owner = (*this)->getParentOfType<ModuleOp>();
  if (failed(detail::verifySourceEffectMetadata(
          getOccurrence(), getSourceLocation(), owner,
          [&] { return emitOpError(); })) ||
      failed(detail::verifyHardwareTypeScope(getPayload().getType(), owner)) ||
      failed(detail::verifyHardwareTypeScope(getCandidate().getType(), owner)))
    return failure();
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  return predicate(analysis, getPath(), scope(*this), *this,
                   "formal write path");
}

LogicalResult SourceBranchOp::verify() {
  if (failed(transactionPlacement(*this, false, true)))
    return failure();
  auto owner = (*this)->getParentOfType<ModuleOp>();
  if (failed(detail::verifySourceEffectMetadata(getOccurrence(),
                                                getConditionLocation(), owner,
                                                [&] { return emitOpError(); })))
    return failure();
  if (Attribute raw = (*this)->getAttr("ac.loop_continuation")) {
    auto continuation = dyn_cast<BoolAttr>(raw);
    if (!continuation || !continuation.getValue())
      return emitOpError() << "ac.loop_continuation requires BoolAttr true";
  }
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  return predicate(analysis, getCondition(), scope(*this), *this,
                   "source branch condition");
}

LogicalResult SourceBranchOp::verifyRegions() {
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto bindings = scope(*this);
  uint64_t work = 0;
  for (Region *arm : {&getThenRegion(), &getElseRegion()}) {
    if (!arm->hasOneBlock() || arm->front().empty() ||
        arm->front().getNumArguments() || !isa<YieldOp>(arm->front().back()))
      return emitOpError() << "source branch arm requires one block, no "
                              "arguments and final ac.yield";
    auto &yield = arm->front().back();
    if (yield.getNumOperands() != getNumResults())
      return yield.emitOpError()
             << "source branch yield arity must match results";
    for (auto [actual, expected] :
         llvm::zip(yield.getOperandTypes(), getResultTypes())) {
      auto charge = [&](uint64_t units, uint64_t bytes) -> LogicalResult {
        uint64_t slots = bytes / 8 + (bytes % 8 != 0);
        if (units > 1024 * 1024 - work || slots > 1024 * 1024 - work - units)
          return mlir::emitError(UnknownLoc::get(getContext()))
                 << "source branch verification budget exceeded";
        work += units + slots;
        return success();
      };
      auto left = detail::resolveSourceBranchResultType(actual, analysis, bindings, &yield, charge);
      auto right = detail::resolveSourceBranchResultType(expected, analysis, bindings, *this, charge);
      if (failed(left) || failed(right))
        return failure();
      if (!areEquivalentHardwareTypes(*left, *right))
        return yield.emitOpError()
               << "source branch yields require matching nominal or Bits results";
    }
    for (Operation &operation : arm->front().without_terminator())
      if (failed(branchPayload(&operation, true, 0, work)))
        return failure();
  }
  return success();
}

LogicalResult SourceMethodOp::verify() {
  if (failed(namedDeclaration(*this, getSymNameAttr())))
    return failure();
  if (Attribute raw = (*this)->getAttr("sym_visibility")) {
    auto visibility = dyn_cast<StringAttr>(raw);
    if (!visibility || visibility.getValue() != "private")
      return emitOpError()
             << "source Method visibility must be absent or private";
  }
  if (Attribute raw = (*this)->getAttr("ac.callable_after")) {
    auto after = dyn_cast<ArrayAttr>(raw);
    if (!after || after.empty())
      return emitOpError() << "ac.callable_after requires a nonempty declaration array";
    llvm::DenseSet<Attribute> seen;
    auto module = (*this)->getParentOfType<ModuleOp>();
    for (Attribute predecessor : after) {
      auto occurrence = dyn_cast<DictionaryAttr>(predecessor);
      if (!occurrence ||
          failed(detail::verifyOccurrence(occurrence, [&] { return emitOpError(); })) ||
          occurrence == getOccurrence() || !seen.insert(occurrence).second)
        return emitOpError() << "ac.callable_after requires distinct other local callable declarations";
      bool found = false;
      for (auto method : module.getBody().front().getOps<SourceMethodOp>())
        found |= method.getOccurrence() == occurrence;
      if (!found)
        return emitOpError() << "ac.callable_after target must identify a local callable declaration";
    }
  }
  auto signature = getFunctionType();
  if (getArgumentNames().size() != signature.getNumInputs())
    return emitOpError() << "argument name count must match method signature";
  llvm::StringSet<> names;
  for (Attribute raw : getArgumentNames()) {
    auto name = cast<StringAttr>(raw);
    if (failed(detail::verifyPythonAstIdentifier(
            name.getValue(), true, [&] { return emitOpError(); })))
      return failure();
    if (!names.insert(name.getValue()).second)
      return emitOpError() << "method argument names must be unique";
  }
  auto form = (*this)->getAttrOfType<StringAttr>("ac.return_form");
  auto parameters = (*this)->getAttrOfType<ArrayAttr>("ac.parameters");
  auto results = (*this)->getAttrOfType<ArrayAttr>("ac.result_constraints");
  if (!form || !parameters || !results ||
      parameters.size() != signature.getNumInputs() ||
      results.size() != signature.getNumResults())
    return emitOpError()
           << "method source contract requires ac.parameters, "
              "ac.result_constraints and matching none/single/nonempty tuple "
              "ac.return_form";
  auto lexicalModule = (*this)->getParentOp();
  auto error = [&] { return emitOpError(); };
  auto sourceOwner =
      lexicalModule->getAttrOfType<DictionaryAttr>("source_owner");
  if (failed(detail::verifySourceGuardLocation(
          (*this)->getAttrOfType<DictionaryAttr>("ac.guard_location"),
          sourceOwner, error)))
    return failure();
  for (TypeRange types : {signature.getInputs(), signature.getResults()})
    for (Type type : types) {
      if (!isa<BitsType, StructType, EnumType, TypeParamType, TableType>(type))
        return emitOpError()
               << "method signature requires hardware payload types";
      if (failed(detail::verifyHardwareTypeScope(type, lexicalModule)))
        return failure();
    }
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  for (auto [index, raw] : llvm::enumerate(parameters)) {
    if (failed(detail::verifyRequiredSourceParameter(
            raw, cast<StringAttr>(getArgumentNames()[index]), lexicalModule,
            *this, "source method")))
      return failure();
    auto parameter = cast<DictionaryAttr>(raw);
    if (failed(detail::verifySourceCallHardwareConstraint(
            parameter.get("constraint"), signature.getInput(index), analysis,
            lexicalModule, *this, "source method")))
      return failure();
  }
  for (auto [index, raw] : llvm::enumerate(results))
    if (failed(detail::verifySourceCallHardwareConstraint(
            raw, signature.getResult(index), analysis, lexicalModule, *this,
            "source method")))
      return failure();
  SourceMethodView contract{lexicalModule, *this, {}};
  if (failed(verifySourceMethodFormalProposals(contract, *this)))
    return failure();
  unsigned publicCount = contract.getNumPublicResults();
  if (!((form.getValue() == "none" && publicCount == 0) ||
        (form.getValue() == "single" && publicCount == 1) ||
        (form.getValue() == "tuple" && publicCount != 0)))
    return emitOpError()
           << "method ac.return_form must match its public result count";
  return success();
}

LogicalResult SourceMethodOp::verifyRegions() {
  SmallVector<Type> arguments(getFunctionType().getInputs());
  llvm::append_range(arguments, getCaptures().getTypes());
  if (failed(region(*this, 0, arguments, {})))
    return failure();
  return region(*this, 1, arguments, getFunctionType().getResults());
}

LogicalResult SourceMethodCallOp::verify() {
  if ((*this)->hasAttr("receiver_kind") || (*this)->hasAttr("ancestor_depth")) {
    if ((*this)->hasAttr("ac.reference"))
      return emitOpError() << "relative Method receiver cannot also name a Module reference formal";
    NamedAttrList receiver;
    for (StringRef name : {"receiver_kind", "ancestor_depth", "instance_path"})
      if (Attribute value = (*this)->getAttr(name)) receiver.append(name, value);
    if (failed(verifySourceRelativeReceiver(receiver.getDictionary(getContext()), *this)))
      return failure();
  }
  if (failed(transactionPlacement(*this, false, true)))
    return failure();
  if (Attribute raw = (*this)->getAttr("ac.reference")) {
    auto reference = dyn_cast<StringAttr>(raw);
    if (!reference || failed(detail::verifyPythonAstIdentifier(reference.getValue(), true,
                                      [&] { return emitOpError(); })))
      return emitOpError() << "ac.reference requires a Module reference formal name";
  }
  auto callee = getCalleeAttr();
  bool nativeQueue =
      callee.getRootReference().getValue() ==
          "pycircuit.__builtins__.fifo_local" ||
      callee.getRootReference().getValue() ==
          "pycircuit.__builtins__.fifo_downstream_pop" ||
      callee.getRootReference().getValue() == "pycircuit.__builtins__.sync_mem" ||
      callee.getRootReference().getValue() == "pycircuit.__builtins__.sync_mem_dp" ||
      callee.getRootReference().getValue() == "pycircuit.__builtins__.byte_mem";
  auto owner = (*this)->getParentOfType<ModuleOp>();
  Attribute rawLocation = (*this)->getAttr("ac.location");
  auto location = dyn_cast_or_null<DictionaryAttr>(rawLocation);
  if (rawLocation && !location)
    return emitOpError() << "method call ac.location requires a SourceSpan";
  if (nativeQueue && !location)
    return emitOpError()
           << "native FIFO calls require ac.location as a SourceSpan";
  if (location &&
      failed(detail::verifySourceEffectMetadata(
          getOccurrence(), location, owner, [&] { return emitOpError(); })))
      return failure();
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  if (failed(predicate(analysis, getPath(), scope(*this), *this,
                       "method call path")))
    return failure();
  if (callee.getNestedReferences().size() != 1)
    return emitOpError() << "method callee must be @Module::@method";
  for (Attribute raw : getInstancePath()) {
    auto name = dyn_cast<StringAttr>(raw);
    if (!name || name.getValue().empty() || name.getValue().contains('\0'))
      return emitOpError() << "method instance path requires nonempty names";
  }
  return success();
}

LogicalResult SourceMethodCallOp::verifySymbolUses(SymbolTableCollection &) {
  if ((*this)->getNumOperands() == 0 || (*this)->getNumRegions() != 0)
    return emitOpError() << "method call requires path operand and no regions";
  if (failed(verifyInvariantsImpl()))
    return failure();
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto resolved = resolveSourceMethodCall(*this);
  if (failed(resolved))
    return failure();
  auto bindings = resolved->bindings;
  auto signature = resolved->method.getFunctionType();
  if (getArguments().size() != signature.getNumInputs() ||
      getResults().size() != signature.getNumResults())
    return emitOpError() << "method call arity must match signature";
  auto check = [&](Type actual, Type formal) -> LogicalResult {
    auto expected = analysis.resolveType(formal, bindings, *this);
    auto supplied = analysis.resolveType(actual, scope(*this), *this);
    if (failed(expected) || failed(supplied))
      return failure();
    if (!areEquivalentHardwareTypes(*expected, *supplied))
      return emitOpError() << "method call payload type must match bound "
                              "receiver signature";
    return success();
  };
  for (auto [actual, formal] :
       llvm::zip(getArguments().getTypes(), signature.getInputs()))
    if (failed(check(actual, formal)))
      return failure();
  for (auto [actual, formal] :
       llvm::zip(getResultTypes(), signature.getResults()))
    if (failed(check(actual, formal)))
      return failure();
  auto caller = (*this)->getParentOfType<SourceMethodOp>();
  auto lexical = (*this)->getParentOfType<ModuleOp>();
  return verifySourceFormalOwnerBindings(
      (*this)->getAttr("ac.formal_owner_bindings"), getArguments().size(),
      resolved->method.getFormalProposals(),
      caller ? caller.getFunctionType() : FunctionType(),
      caller ? caller->getAttrOfType<ArrayAttr>("ac.formal_proposals")
             : ArrayAttr(),
      [&](StringRef name) {
        for (auto state : lexical.getBody().front().getOps<SourceStateOp>())
          if (state.getInstanceName() == name)
            return true;
        return false;
      }, *this);
}

LogicalResult SourceTransactionOp::verify() {
  if ((*this)->hasAttr("sym_name"))
    return emitOpError() << "transaction uses name; sym_name is unsupported";
  if (failed(namedDeclaration(*this, getNameAttr())))
    return failure();
  if (failed(
          moduleOccurrence(*this, getDeclaration(), "transaction declaration")))
    return failure();
  auto owner = cast<ModuleOp>((*this)->getParentOp());
  auto error = [&] { return emitOpError(); };
  if (failed(detail::verifySourceGuardLocation(
          (*this)->getAttrOfType<DictionaryAttr>("ac.guard_location"),
          owner.getSourceOwner(), error)))
    return failure();
  for (auto other : owner.getBody().front().getOps<SourceTransactionOp>()) {
    if (other == *this)
      continue;
    if (failed(moduleOccurrence(other, other.getDeclaration(),
                                "transaction declaration")))
      return failure();
    if (detail::compareClosedSourceStructure(getOccurrence(),
                                             other.getOccurrence()) == 0)
      return emitOpError() << "transaction registration occurrence must be unique";
  }
  if (Attribute raw = (*this)->getAttr("ac.after")) {
    auto after = dyn_cast<ArrayAttr>(raw);
    if (!after) return emitOpError() << "ac.after requires a registration array";
    llvm::DenseSet<Attribute> seen;
    for (Attribute entry : after) {
      auto predecessor = dyn_cast<DictionaryAttr>(entry);
      if (failed(moduleOccurrence(*this, predecessor, "transaction predecessor")) ||
          !seen.insert(entry).second)
        return emitOpError() << "ac.after requires distinct local registrations";
      unsigned matches = 0;
      for (auto candidate : owner.getBody().front().getOps<SourceTransactionOp>())
        if (detail::compareClosedSourceStructure(predecessor,
                                                 candidate.getOccurrence()) == 0) {
          if (candidate == *this)
            return emitOpError() << "ac.after cannot reference its own registration";
          ++matches;
        }
      if (matches != 1)
        return emitOpError() << "ac.after requires exactly one local outer registration";
    }
  }
  HardwareAnalysis analysis((*this)->getParentOfType<mlir::ModuleOp>());
  auto bindings = scope(*this);
  if (failed(predicate(analysis, getClk(), bindings, *this,
                       "transaction clock")) ||
      failed(
          predicate(analysis, getRst(), bindings, *this, "transaction reset")))
    return failure();
  return success();
}

LogicalResult SourceTransactionOp::verifyRegions() {
  if (failed(region(*this, 0, getCaptures().getTypes(), {})))
    return failure();
  if (failed(region(*this, 1, getCaptures().getTypes(), getResultTypes())))
    return failure();
  return failed(getSourceRegistrationEntry(*this)) ? failure() : success();
}

Operation *detail::findUnloweredTransaction(Operation *root) {
  SmallVector<Operation *> pending;
  if (root)
    pending.push_back(root);
  while (!pending.empty()) {
    Operation *operation = pending.pop_back_val();
    if (isa<SourceStateOp, StateUpdateOp, SourceStateReadOp, SourceStateWriteOp,
            SourceFormalWriteOp, SourceMethodOp, SourceMethodCallOp, SourceTransactionOp,
            SourceBranchOp, SourceQueuePopOp, SourceQueuePeekOp,
            SourceQueuePushOp>(operation) ||
        (isa<InstanceOp>(operation) &&
         (operation->hasAttr("ac.source_unbound_push") ||
          operation->hasAttr("ac.source_unbound_pop") ||
          operation->hasAttr("ac.payload_constraint") ||
          operation->hasAttr("ac.reference_bindings"))) ||
        (isa<ModuleOp, ModuleImportOp>(operation) &&
         operation->hasAttr("ac.reference_parameters")))
      return operation;
    for (Region &region : llvm::reverse(operation->getRegions()))
      for (Block &block : llvm::reverse(region))
        for (Operation &child : llvm::reverse(block))
          pending.push_back(&child);
  }
  return nullptr;
}

LogicalResult detail::verifyNoUnloweredTransactions(Operation *root) {
  if (Operation *operation = findUnloweredTransaction(root))
    return operation->emitOpError() << "unlowered source transaction operation "
                                       "cannot cross the verified "
                                       "hardware boundary";
  return success();
}

} // namespace acir::ac
