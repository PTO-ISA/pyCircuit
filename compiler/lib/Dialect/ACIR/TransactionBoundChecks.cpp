// Staged transaction-surface bound checks (B2 step-2 design r2 §5
// ACIRTransactionOps rows): the state selector/payload resolution, the
// Method contract and call resolution with type agreement, the
// transaction/branch control predicates, the guard-yield predicate and the
// bounded branch-result closure compile on the analysis side and are
// reached from the resolved dispatch. The moved bodies are verbatim. The
// native verifiers keep the placement, attr-allowlist, formal-index and
// region-shape grammar.
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "ACIRSourceContracts.h"
#include "pycircuit/Analysis/LogicalStateValidation.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"

#include "mlir/IR/Verifier.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/STLExtras.h"
#include <functional>
#include <limits>
#include <type_traits>

using namespace mlir;

namespace acir::ac {
namespace {

// Single-implementation delegates to the shared logical-state validation
// helpers; call sites keep their historical names.
HardwareBindings scope(Operation *operation) {
  return stateOwnerScope(operation);
}

LogicalResult predicate(const HardwareAnalysis &analysis, Value value,
                        const HardwareBindings &bindings, Operation *site,
                        StringRef description, bool requireResolved) {
  // A legal-but-unbound integer parameter defers only the width-dependent
  // part of this check; the Bits-carrier grammar stays with the native
  // verifier (r8 review batch 1: the caller's bindings and resolution
  // discipline actually reach the moved checks).
  // The carrier grammar — including the canonical width-one rejection
  // text — stays with stateControlPredicate; only the width-dependence on
  // a legal unbound parameter defers here.
  if (auto bits = dyn_cast<BitsType>(value.getType())) {
    if (!analysis.isStaticEvaluable(bits.getWidth(), bindings)) {
      if (!requireResolved && site->getParentOfType<ModuleOp>())
        return success();
      return site->emitOpError()
             << description << " requires a resolved finite width";
    }
  }
  return stateControlPredicate(analysis, value, bindings, site, description);
}

FailureOr<Type> selectedPayloadType(const HardwareAnalysis &analysis,
                                    const HardwareBindings &bindings,
                                    bool requireResolved, Operation *operation,
                                    Type root, ArrayAttr selectors,
                                    ValueRange indices) {
  // A genuine resolution failure (a malformed reference) stays a failure in
  // every tier; only the width dependence on a legal unbound parameter
  // defers, and then just the width evaluation — the carrier, ordinal,
  // field-name and element-type facts still descend and still reach the
  // caller's comparison (r8 table-layering review).
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
    if (!isa<BitsType>(indices[nextIndex].getType()))
      return operation->emitOpError() << "state index must have resolved Bits "
                                         "type";
    // Per-index deferral (r8 table-layering review): an unbound parameter
    // defers only THAT index's width evaluation. The Table template
    // verification always runs — it tolerates symbolic extents and still
    // checks the evaluable subexpressions.
    // The Table template verification always runs — it tolerates symbolic
    // extents and still checks the evaluable subexpressions. The index
    // width evaluation defers per index: decide evaluability FIRST so a
    // legal unbound parameter never reaches the evaluator (which would
    // emit "unresolved hardware static expression" before the deferral),
    // while an evaluable-but-failing expression is illegal and rejects.
    auto indexCarrier = cast<BitsType>(indices[nextIndex].getType());
    bool widthEvaluable =
        analysis.isStaticEvaluable(indexCarrier.getWidth(), bindings);
    if (widthEvaluable || requireResolved ||
        !operation->getParentOfType<ModuleOp>()) {
      auto indexWidth = analysis.getPackedWidth(indices[nextIndex].getType(),
                                                bindings, operation);
      if (failed(indexWidth))
        return failure();
    }
    if (failed(detail::verifySourceTemplateType(analysis, table, bindings,
                                                operation)))
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
FailureOr<Type> selectedStateType(const HardwareAnalysis &analysis,
                                  const HardwareBindings &bindings,
                                  bool requireResolved, Operation *operation,
                                  ArrayAttr selectors, ValueRange indices) {
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
  return selectedPayloadType(analysis, bindings, requireResolved, operation,
                             state.getQ().getType(), selectors, indices);
}

// The guard branch of the moved region() helper: the guard yield must be a
// single control predicate under the operation's owner scope. The region
// shape and yield-type grammar stay with the native region verifier; this
// re-check keeps direct staged calls safe on partially mutated IR.
LogicalResult guardYieldPredicate(const HardwareAnalysis &analysis,
                                  const HardwareBindings &bindings,
                                  Operation *operation, bool requireResolved) {
  auto &region = operation->getRegion(0);
  if (!region.hasOneBlock() || region.front().empty() ||
      !isa<YieldOp>(region.front().back()))
    return operation->emitOpError()
           << "guard/body requires one block, matching arguments and final "
              "ac.yield";
  auto &yield = region.front().back();
  if (yield.getNumOperands() != 1)
    return yield.emitOpError() << "guard must yield exactly one predicate";
  return predicate(analysis, yield.getOperand(0), bindings, &yield,
                   "guard yield", requireResolved);
}

} // namespace

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
  uint64_t coldGrammarCalls = 0, fieldPathSlots = 0, shapeSlots = 0, shapeArrays = 0;
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
          auto tree = expression.getTree();
          auto kind = tree.getAs<StringAttr>("kind");
          // Match HardwareAnalysis::substitute: literal/type-width trees are
          // borrowed intact. Computational trees can be rebuilt; a reference
          // creates a literal only for an explicit non-expression binding.
          bool rebuild = kind && (kind.getValue() == "unary" ||
                                  kind.getValue() == "binary" ||
                                  kind.getValue() == "select");
          if (kind && kind.getValue() == "reference") {
            auto reference = tree.getAs<DictionaryAttr>("ref");
            auto owner = reference
                             ? reference.getAs<FlatSymbolRefAttr>("owner")
                             : FlatSymbolRefAttr();
            auto name = reference ? reference.getAs<StringAttr>("name")
                                  : StringAttr();
            if (owner && name && owner.getValue() == ownerName.getValue()) {
              auto binding = lexicalBindings.integers.find(name.getValue());
              rebuild = binding != lexicalBindings.integers.end() &&
                        !isa<StaticExprAttr>(binding->second);
            }
          }
          if (failed(appendRaw(Node(expression.getTree()), entry.depth + 1,
                               rebuild, true))) valid = false;
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
            if (failed(verifyNominalStructField(
                    raw, analysis, declaration, diagnosticSite)))
              return failure();
            auto field = cast<DictionaryAttr>(raw);
            auto key = field.getAs<StringAttr>("name");
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
      // Fixed Table-valued Struct fields use the existing packed layout path.
      // It resolves positive, closed dimensions after the complete closure is
      // paid below; this does not admit multidimensional or Table-of-Table
      // source fields.
      if (auto table = dyn_cast<TableType>(child))
        if (table.getShape().size() != 1 ||
            !isa<BitsType, EnumType, StructType>(table.getElementType()))
          return error() << "source branch Struct Table fields require one "
                            "dimension and Bits, Enum or Struct elements";
      fieldPathSlots = add(fieldPathSlots, stack.size());
      if (failed(push(child))) return failure();
      continue;
    }
    if (failed(pay(1)) || failed(reserveOne(validated))) return failure();
    if (auto bits = dyn_cast<BitsType>(frame.type)) {
      if (failed(pay(1))) return failure();
      auto width = bits.getWidth();
      auto tree = width ? width.getTree() : DictionaryAttr();
      coldGrammarCalls += !tree || !detail::isVerifiedStaticExpressionGrammar(tree);
    }
    validated.push_back(frame.type);
    stack.pop_back();
  }
  // Grammar validation uses inline buckets for small widths. Per container,
  // 2*inline + 8*insertions covers inline/temporary storage and cumulative heap
  // growth, including the first 64-bucket spill and active-set tombstone rebuilds.
  // A warm exact grammar key returns before those local containers exist.
  // It does not replace the owner/scope, binding or layout checks below.
  uint64_t grammarBuckets = coldGrammarCalls ? multiply(2, add(
      multiply(detail::staticGrammarInlineBuckets, coldGrammarCalls),
      multiply(4, expressions))) : 0;
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
  if (failed(detail::verifySourceTemplateType(
          analysis, actualType, lexicalBindings, diagnosticSite))) return failure();
  return analysis.resolveType(actualType, lexicalBindings, diagnosticSite);
}

} // namespace acir::ac

namespace acir::ac {

LogicalResult HardwareAnalysis::verifyTransactionSurfaceOperation(
    Operation *operation, const HardwareBindings &bindings,
    bool requireResolved) const {
  // The caller's bindings and resolution discipline reach every moved
  // check (r8 review batch 1): the source stage passes the lexical module
  // scope with requireResolved=false (definite facts only, legal unbound
  // parameters defer their width-dependent parts), and the binding tier
  // passes the actual bindings with requireResolved=true. A direct staged
  // call may carry no caller scope at all; the rows then keep the
  // pre-split native shape and bind the enclosing module themselves,
  // because the template-type, receiver and payload selections below
  // reject an absent owner scope silently instead of diagnosing.
  HardwareBindings scope = bindings;
  if (!scope.owner)
    scope.owner = operation->getParentOfType<ModuleOp>();
  if (auto read = dyn_cast<SourceStateReadOp>(operation)) {
    if (failed(predicate(*this, read.getPath(), scope, read,
                         "state read path", requireResolved)))
      return failure();
    auto selected =
        selectedStateType(*this, scope, requireResolved, read,
                          read.getSelectors(), read.getIndices());
    if (failed(selected))
      return failure();
    // Both sides resolve through the caller's bindings: at the binding tier
    // the selected payload carries the actual extents while the read's SSA
    // result keeps its template spelling, and the agreement fact compares
    // the same resolution space on each side (as the Method call row does).
    // A deferred source-stage selection (legal unbound parameter) skips the
    // comparison; the binding tier re-runs it.
    auto result = resolveType(read.getValue().getType(), scope, read);
    if (failed(result))
      return failure();
    if (!areEquivalentHardwareTypes(*selected, *result))
      return read.emitOpError()
             << "state read result must match selected payload type";
    return success();
  }
  if (auto write = dyn_cast<SourceStateWriteOp>(operation)) {
    if (failed(predicate(*this, write.getPath(), scope, write,
                         "state write path", requireResolved)))
      return failure();
    auto selected = selectedStateType(*this, scope, requireResolved, write,
                                      write.getSelectors(),
                                      write.getIndices());
    if (failed(selected))
      return failure();
    auto value = resolveType(write.getValue().getType(), scope, write);
    if (failed(value))
      return failure();
    if (!areEquivalentHardwareTypes(*selected, *value))
      return write.emitOpError()
             << "state write value must match selected payload type";
    // The selected Table index obligations remain the dispatcher's existing
    // SourceStateWriteOp row.
    return success();
  }
  if (auto formal = dyn_cast<SourceFormalWriteOp>(operation)) {
    auto method = formal->getParentOfType<SourceMethodOp>();
    if (!method)
      return formal.emitOpError() << "formal write requires a lexical Method formal";
    auto signature = method.getFunctionType();
    auto index = formal.getFormalAttr();
    // The writable-formal identification grammar stays native; re-check the
    // bounds so the payload selection below indexes safely on directly
    // staged calls.
    if (!index.getType().isSignlessInteger(64) || index.getInt() < 0 ||
        static_cast<uint64_t>(index.getInt()) >= signature.getNumInputs())
      return formal.emitOpError()
             << "formal write must identify a declared writable input formal";
    auto selected =
        selectedPayloadType(*this, scope, requireResolved, formal,
                            signature.getInput(index.getInt()),
                            formal.getSelectors(), formal.getIndices());
    if (failed(selected))
      return failure();
    auto payload = resolveType(formal.getPayload().getType(), scope, formal);
    if (failed(payload))
      return failure();
    if (!areEquivalentHardwareTypes(*selected, *payload))
      return formal.emitOpError()
             << "formal write payload must match selected input formal type";
    if (failed(predicate(*this, formal.getPath(), scope, formal,
                         "formal write path", requireResolved)))
      return failure();
    return success();
  }
  if (auto method = dyn_cast<SourceMethodOp>(operation)) {
    auto lexicalModule = method->getParentOp();
    auto signature = method.getFunctionType();
    auto parameters = method->getAttrOfType<ArrayAttr>("ac.parameters");
    auto results = method->getAttrOfType<ArrayAttr>("ac.result_constraints");
    // The contract-presence and count grammar stays native; re-check it so
    // the rows below enumerate and index safely on directly staged calls.
    if (!parameters || !results ||
        parameters.size() != signature.getNumInputs() ||
        results.size() != signature.getNumResults())
      return method.emitOpError()
             << "method source contract requires ac.parameters, "
                "ac.result_constraints and matching none/single/nonempty tuple "
                "ac.return_form";
    for (auto [index, raw] : llvm::enumerate(parameters)) {
      auto parameter = cast<DictionaryAttr>(raw);
      if (failed(detail::verifySourceCallHardwareConstraint(
              parameter.get("constraint"), signature.getInput(index), *this,
              lexicalModule, method, "source method",
              detail::SourceCallWidthMode::SymbolicFixedBits)))
        return failure();
    }
    for (auto [index, raw] : llvm::enumerate(results))
      if (failed(detail::verifySourceCallHardwareConstraint(
              raw, signature.getResult(index), *this, lexicalModule, method,
              "source method", detail::SourceCallWidthMode::SymbolicFixedBits)))
        return failure();
    SourceMethodView contract{lexicalModule, method, {}};
    if (failed(verifySourceMethodFormalProposals(contract, method)))
      return failure();
    return guardYieldPredicate(*this, scope, method, requireResolved);
  }
  if (auto call = dyn_cast<SourceMethodCallOp>(operation)) {
    if (failed(predicate(*this, call.getPath(), scope, call,
                         "method call path", requireResolved)))
      return failure();
    // The caller's bindings resolve the actuals; the resolver derives the
    // receiver's bindings from this call site (r8 review batch 1).
    auto resolved = resolveSourceMethodCall(
        call, scope,
        [&](FlatSymbolRefAttr symbol) {
          return lookupDefinition(symbol.getValue());
        });
    if (failed(resolved))
      return failure();
    auto calleeBindings = resolved->bindings;
    auto signature = resolved->method.getFunctionType();
    if (call.getArguments().size() != signature.getNumInputs() ||
        call.getResults().size() != signature.getNumResults())
      return call.emitOpError() << "method call arity must match signature";
    auto check = [&](Type actual, Type formal) -> LogicalResult {
      auto expected = resolveType(formal, calleeBindings, call);
      auto supplied = resolveType(actual, scope, call);
      if (failed(expected) || failed(supplied))
        return failure();
      if (!areEquivalentHardwareTypes(*expected, *supplied))
        return call.emitOpError() << "method call payload type must match bound "
                                    "receiver signature";
      return success();
    };
    for (auto [actual, formal] :
         llvm::zip(call.getArguments().getTypes(), signature.getInputs()))
      if (failed(check(actual, formal)))
        return failure();
    for (auto [actual, formal] :
         llvm::zip(call.getResultTypes(), signature.getResults()))
      if (failed(check(actual, formal)))
        return failure();
    auto caller = call->getParentOfType<SourceMethodOp>();
    auto lexical = call->getParentOfType<ModuleOp>();
    return verifySourceFormalOwnerBindings(
        call->getAttr("ac.formal_owner_bindings"), call.getArguments().size(),
        resolved->method.getFormalProposals(),
        caller ? caller.getFunctionType() : FunctionType(),
        caller ? caller->getAttrOfType<ArrayAttr>("ac.formal_proposals")
               : ArrayAttr(),
        [&](StringRef name) {
          for (auto state : lexical.getBody().front().getOps<SourceStateOp>())
            if (state.getInstanceName() == name)
              return true;
          return false;
        },
        call);
  }
  if (auto branch = dyn_cast<SourceBranchOp>(operation)) {
    if (failed(predicate(*this, branch.getCondition(), scope, branch,
                         "source branch condition", requireResolved)))
      return failure();
    uint64_t work = 0;
    for (Region *arm : {&branch.getThenRegion(), &branch.getElseRegion()}) {
      if (!arm->hasOneBlock() || arm->front().empty() ||
          arm->front().getNumArguments() || !isa<YieldOp>(arm->front().back()))
        return branch.emitOpError() << "source branch arm requires one block, no "
                                      "arguments and final ac.yield";
      auto &yield = arm->front().back();
      if (yield.getNumOperands() != branch.getNumResults())
        return yield.emitOpError()
               << "source branch yield arity must match results";
      for (auto [actual, expected] :
           llvm::zip(yield.getOperandTypes(), branch.getResultTypes())) {
        auto charge = [&](uint64_t units, uint64_t bytes) -> LogicalResult {
          uint64_t slots = bytes / 8 + (bytes % 8 != 0);
          if (units > 1024 * 1024 - work || slots > 1024 * 1024 - work - units)
            return mlir::emitError(UnknownLoc::get(branch->getContext()))
                   << "source branch verification budget exceeded";
          work += units + slots;
          return success();
        };
        auto left = detail::resolveSourceBranchResultType(actual, *this, scope, &yield, charge);
        auto right = detail::resolveSourceBranchResultType(expected, *this, scope, branch, charge);
        if (failed(left) || failed(right))
          return failure();
        if (!areEquivalentHardwareTypes(*left, *right))
          return yield.emitOpError()
                 << "source branch yields require matching nominal or Bits results";
      }
    }
    // The arm payload-effect allowlist remains the native region grammar.
    return success();
  }
  if (auto transaction = dyn_cast<SourceTransactionOp>(operation)) {
    if (failed(predicate(*this, transaction.getClk(), scope, transaction,
                         "transaction clock", requireResolved)) ||
        failed(predicate(*this, transaction.getRst(), scope, transaction,
                         "transaction reset", requireResolved)))
      return failure();
    // The registration-entry structure left the native region verifier at
    // IX (it requires the staged Method resolution cluster); the staged
    // surface row owns it so a direct staged call stays safe on mutated IR.
    // Transactions without the marker resolve to an empty entry, exactly as
    // the native verifier saw.
    if (failed(getSourceRegistrationEntry(transaction)))
      return failure();
    return guardYieldPredicate(*this, scope, transaction, requireResolved);
  }
  return operation->emitOpError()
         << "operation is not a transaction surface bound check";
}

} // namespace acir::ac
