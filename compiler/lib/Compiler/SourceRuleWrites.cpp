#include "SourceRuleWrites.h"
#include "PythonImportContext.h"

#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringSet.h"
#include <algorithm>
#include <functional>

using namespace mlir;

namespace acir::compiler::detail {
namespace {
bool sameOccurrence(const AstNode &a, const AstNode &b) {
  if (a.value != b.value || a.path.size() != b.path.size())
    return false;
  for (auto [left, right] : llvm::zip(a.path, b.path))
    if (left.field != right.field || left.index != right.index)
      return false;
  return true;
}
bool covers(ArrayRef<SourceWriteSelector> a, ArrayRef<SourceWriteSelector> b) {
  if (a.size() > b.size())
    return false;
  for (auto [left, right] : llvm::zip(a, b)) {
    if (left.kind != right.kind)
      return false;
    if (left.kind == SourceWriteSelector::Kind::Field) {
      if (left.field != right.field)
        return false;
    } else if (left.directConstantInteger || right.directConstantInteger) {
      if (!left.directConstantInteger || !right.directConstantInteger ||
          left.directConstantInteger != right.directConstantInteger)
        return false;
    } else if (!sameOccurrence(left.occurrence, right.occurrence))
      return false;
  }
  return true;
}
bool overlaps(ArrayRef<SourceWriteSelector> a,
              ArrayRef<SourceWriteSelector> b) {
  for (auto [left, right] : llvm::zip(a, b)) {
    if (left.kind != right.kind)
      return true;
    if (left.kind == SourceWriteSelector::Kind::Field) {
      if (left.field != right.field)
        return false;
    } else if (left.directConstantInteger && right.directConstantInteger &&
               left.directConstantInteger != right.directConstantInteger)
      return false;
  }
  return true;
}
FailureOr<StringRef> targetPath(const AstNode &target,
                                SmallVectorImpl<SourceWriteSelector> &selectors,
                                OpBuilder &builder,
                                ac::detail::EmitError emitError,
                                const std::function<bool(uint64_t)> &charge) {
  AstNode base = target;
  while (base.kind() == "Attribute" || base.kind() == "Subscript") {
    if (!charge(1))
      return failure();
    if (base.kind() == "Attribute")
      selectors.push_back({SourceWriteSelector::Kind::Field,
                           base.string("attr").str(),
                           base,
                           {}});
    else {
      auto index = base.child("slice");
      ac::MathIntAttr integer;
      auto raw = dyn_cast_or_null<DictionaryAttr>(index.get("value"));
      if (index.kind() == "Constant" && raw &&
          raw.getAs<StringAttr>("integer")) {
        if (!charge(raw.getAs<StringAttr>("integer").getValue().size()))
          return failure();
        auto encoded = staticValue(builder, raw, emitError);
        if (!encoded)
          return failure();
        integer = encoded.getAs<ac::MathIntAttr>("value");
      }
      selectors.push_back(
          {SourceWriteSelector::Kind::Index, {}, index, integer});
    }
    base = base.child("value");
  }
  std::reverse(selectors.begin(), selectors.end());
  return base.kind() == "Name" ? base.string("id") : StringRef();
}

bool addPath(SourceOwnerWrite &owner, SourceWritePath path,
             const std::function<bool(uint64_t)> &charge) {
  for (const auto &prior : owner.paths) {
    if (!charge(1 + prior.selectors.size() + path.selectors.size()))
      return false;
    if (covers(prior.selectors, path.selectors))
      return true;
  }
  // Charge the full removal scan before changing the plan.
  for (const auto &prior : owner.paths)
    if (!charge(1 + prior.selectors.size() + path.selectors.size()))
      return false;
  llvm::erase_if(owner.paths, [&](const SourceWritePath &prior) {
    return covers(path.selectors, prior.selectors);
  });
  owner.paths.push_back(std::move(path));
  return true;
}
} // namespace

SourceRuleWritesAnalysis::SourceRuleWritesAnalysis(Operation *operation)
    : operation(operation) {
  auto capture = readSingleCapture(cast<ModuleOp>(operation),
                                   [&] { return operation->emitError(); });
  if (failed(capture))
    return;
  source = std::move(*capture);
  valid = succeeded(analyze());
}

std::optional<ResolvedSourceBinding>
SourceRuleWritesAnalysis::binding(const AstNode &node) const {
  auto base = node.kind() == "Attribute" ? node.child("value") : node;
  if (base.kind() != "Name" ||
      sourceBindingShadowed(source, base, base.string("id")))
    return std::nullopt;
  auto found = imports.find(base.string("id"));
  if (found == imports.end())
    return std::nullopt;
  const auto &imported = found->second;
  if (node.kind() == "Attribute") {
    if (!imported.nameSpace)
      return std::nullopt;
    return ResolvedSourceBinding{
        classifyImportedMarker(imported.module, node.string("attr")), {}};
  }
  if (imported.nameSpace)
    return std::nullopt;
  return ResolvedSourceBinding{
      classifyImportedMarker(imported.module, imported.remote), {}};
}
bool SourceRuleWritesAnalysis::resolves(const AstNode &node,
                                        MarkerKind marker) const {
  return resolvesMarker(node, marker,
                        [&](const AstNode &n) { return binding(n); });
}
bool SourceRuleWritesAnalysis::hasDecorator(const AstNode &node,
                                            StringRef name) const {
  auto marker = classifyImportedMarker("pycircuit", name);
  return hasSourceDecorator(node, marker,
                            [&](const AstNode &n) { return binding(n); });
}
FailureOr<const SourceRuleCallPlan *> SourceRuleWritesAnalysis::lookup(
    const AstNode &module, const AstNode &call,
    llvm::function_ref<bool(uint64_t)> charge) const {
  auto same = [&](const AstNode &a, const AstNode &c) -> FailureOr<bool> {
    if (!charge(1)) return failure();
    if (a.value != c.value || a.path.size() != c.path.size()) return false;
    for (auto [left, right] : llvm::zip(a.path, c.path)) {
      if (!charge(left.field.size()) || !charge(right.field.size()) || !charge(1))
        return failure();
      if (left.field != right.field || left.index != right.index) return false;
    }
    return true;
  };
  for (const auto &plan : calls) {
    if (!charge(1)) return failure();
    auto owner = same(plan.module, module);
    if (failed(owner)) return failure();
    if (!*owner) continue;
    auto occurrence = same(plan.call, call);
    if (failed(occurrence)) return failure();
    if (*occurrence) return &plan;
  }
  return static_cast<const SourceRuleCallPlan *>(nullptr);
}

bool SourceRuleWritesAnalysis::charge(uint64_t amount) {
  constexpr uint64_t limit = 1048576;
  if (amount > limit - work) {
    operation->emitError(
        "owner-enable proof work budget exhausted during write planning");
    return false;
  }
  work += amount;
  return true;
}

LogicalResult SourceRuleWritesAnalysis::analyze() {
  auto diagnostic = [&](const AstNode &node) {
    return emitError(node.location(operation->getContext(), source.path));
  };
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    if (!charge())
      return failure();
    auto stmt = source.module.item("body", i);
    if (stmt.kind() != "Import" && stmt.kind() != "ImportFrom")
      continue;
    for (size_t j = 0; j < stmt.array("names").size(); ++j) {
      if (!charge())
        return failure();
      auto alias = stmt.item("names", j);
      auto rename = dyn_cast_or_null<StringAttr>(alias.get("asname"));
      auto remote = alias.string("name");
      auto name = rename ? rename.getValue() : remote;
      if (imports.contains(name))
        return diagnostic(alias)
               << "duplicate or conflicting import binding '" << name << "'";
      auto module = stmt.kind() == "Import" ? remote : stmt.string("module");
      imports[name] = {module.str(), remote.str(), stmt.kind() == "Import"};
    }
  }
  llvm::StringMap<AstNode> rules;
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    if (!charge())
      return failure();
    auto stmt = source.module.item("body", i);
    if (stmt.kind() == "FunctionDef" && hasDecorator(stmt, "rule")) {
      if (!rules.try_emplace(stmt.string("name"), stmt).second)
        return diagnostic(stmt) << "duplicate rule name";
    }
  }
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    if (!charge())
      return failure();
    auto module = source.module.item("body", i);
    if (module.kind() != "FunctionDef" ||
        (!hasDecorator(module, "module") && !hasDecorator(module, "system")))
      continue;
    auto globalRules = rules;
    llvm::StringSet<> nestedRuleNames;
    for (size_t j = 0; j < module.array("body").size(); ++j) {
      auto declaration = module.item("body", j);
      if (declaration.kind() == "FunctionDef" && hasDecorator(declaration, "rule")) {
        if (!nestedRuleNames.insert(declaration.string("name")).second)
          return diagnostic(declaration) << "duplicate Rule declaration";
        rules[declaration.string("name")] = declaration;
      }
    }
    auto restoreRules = llvm::scope_exit([&] { rules = std::move(globalRules); });
    auto collectLocals = [&](auto &&self, const AstNode &node, llvm::StringSet<> &bound) -> LogicalResult {
      if (!node || !charge()) return failure();
      auto bindTarget = [&](auto &&targetWalk, const AstNode &target) -> void {
        if (target.kind() == "Name") bound.insert(target.string("id"));
        else if (target.kind() == "Tuple" || target.kind() == "List")
          for (size_t i = 0; i < target.array("elts").size(); ++i) targetWalk(targetWalk, target.item("elts", i));
      };
      if (node.kind() == "FunctionDef" || node.kind() == "ClassDef") {
        if (!hasDecorator(node, "rule")) bound.insert(node.string("name"));
        return success();
      }
      if (node.kind() == "Assign")
        for (size_t i = 0; i < node.array("targets").size(); ++i) bindTarget(bindTarget, node.item("targets", i));
      else if (llvm::is_contained(ArrayRef<StringRef>{"AnnAssign", "AugAssign", "For", "NamedExpr"}, node.kind()))
        bindTarget(bindTarget, node.child("target"));
      for (StringRef field : {"body", "orelse"}) {
        auto statements = node.array(field);
        for (size_t i = 0; statements && i < statements.size(); ++i)
          if (failed(self(self, node.item(field, i), bound))) return failure();
      }
      return success();
    };
    llvm::StringSet<> moduleShadows, moduleFormalShadows;
    auto moduleArguments = module.child("args");
    for (StringRef field : {"args", "posonlyargs", "kwonlyargs"})
      for (size_t j = 0; j < moduleArguments.array(field).size(); ++j) {
        auto name = moduleArguments.item(field, j).string("arg");
        moduleShadows.insert(name);
        moduleFormalShadows.insert(name);
      }
    for (size_t j = 0; j < module.array("body").size(); ++j)
      if (failed(collectLocals(collectLocals, module.item("body", j), moduleShadows))) return failure();
    for (const auto &name : nestedRuleNames) moduleShadows.erase(name.getKey());
    llvm::StringMap<unsigned> registrations;
    llvm::StringSet<> ordinaryData;
    auto orderBefore = [&](const AstNode &use, StringRef name) -> FailureOr<bool> {
      auto found = rules.find(name);
      if (!nestedRuleNames.contains(name) || found == rules.end()) return false;
      auto order = sourceDeclarationOrder(source, module, use, found->second,
                                          [&](uint64_t work) { return charge(work); });
      if (failed(order)) return failure();
      return *order == SourceDeclarationOrder::Before;
    };
    llvm::StringMap<AstNode> owners;
    size_t firstCall = calls.size();
    auto registerCall = [&](const AstNode &call) -> LogicalResult {
      if (!charge())
        return failure();
      auto function = call.child("func");
      if (function.kind() != "Name")
        return success();
      auto found = rules.find(function.string("id"));
      if (found == rules.end())
        return success();
      if (++registrations[function.string("id")] != 1)
        return diagnostic(call) << "Rule declaration must be registered exactly once";
      if (!nestedRuleNames.contains(function.string("id")) &&
          sourceBindingShadowed(source, function, function.string("id")))
        return diagnostic(function)
               << "source rule binding is shadowed in its lexical scope: '"
               << function.string("id") << "'";
      SourceRuleCallPlan plan{module, call, found->second, {}};
      auto formals = plan.rule.child("args");
      if (formals.array("args").size() != call.array("args").size() ||
          !call.array("keywords").empty())
        return diagnostic(call)
               << "behavioral rule call argument arity mismatch";
      llvm::StringMap<size_t> ownerForFormal;
      llvm::StringMap<unsigned> aliases;
      for (size_t j = 0; j < formals.array("args").size(); ++j) {
        if (!charge())
          return failure();
        auto actual = call.item("args", j);
        if (actual.kind() != "Name")
          continue;
        auto owner = owners.find(actual.string("id"));
        if (owner == owners.end())
          continue;
        ++aliases[owner->getKey()];
        ownerForFormal[formals.item("args", j).string("arg")] =
            plan.writes.size();
        plan.writes.push_back(
            {owner->second, formals.item("args", j).string("arg").str(), {}});
      }
      OpBuilder builder(operation->getContext());
      auto scanWrites = [&](auto &&self, const AstNode &parent,
                            StringRef field) -> LogicalResult {
        for (size_t j = 0; j < parent.array(field).size(); ++j) {
          auto stmt = parent.item(field, j);
          if (!charge())
            return failure();
          if (stmt.kind() == "Assign" || stmt.kind() == "AnnAssign" ||
              stmt.kind() == "For") {
            size_t count =
                stmt.kind() == "Assign" ? stmt.array("targets").size() : 1;
            for (size_t k = 0; k < count; ++k) {
              auto target = stmt.kind() == "Assign" ? stmt.item("targets", k)
                                                    : stmt.child("target");
              SmallVector<SourceWriteSelector> selectors;
              auto root = targetPath(
                  target, selectors, builder, [&] { return diagnostic(stmt); },
                  [&](uint64_t amount) { return charge(amount); });
              if (failed(root))
                return failure();
              auto owner = ownerForFormal.find(*root);
              if (owner != ownerForFormal.end() &&
                  !addPath(plan.writes[owner->second],
                           {std::move(selectors), stmt},
                           [&](uint64_t amount) { return charge(amount); }))
                return failure();
            }
          }
          if (stmt.kind() == "For") {
            if (failed(self(self, stmt, "body")))
              return failure();
          } else if (stmt.kind() == "If") {
            if (failed(self(self, stmt, "body")) ||
                failed(self(self, stmt, "orelse")))
              return failure();
          } else if (stmt.kind() == "Match") {
            for (size_t k = 0; k < stmt.array("cases").size(); ++k)
              if (failed(self(self, stmt.item("cases", k), "body")))
                return failure();
          }
        }
        return success();
      };
      if (failed(scanWrites(scanWrites, plan.rule, "body")))
        return failure();
      llvm::erase_if(plan.writes, [](const SourceOwnerWrite &write) {
        return write.paths.empty();
      });
      for (const auto &write : plan.writes) {
        auto name = write.owner.kind() == "AnnAssign"
                        ? write.owner.child("target").string("id")
                        : write.owner.item("targets", 0).string("id");
        if (aliases.lookup(name) > 1)
          return diagnostic(call) << "overlapping writable owner aliases";
        for (size_t prior = firstCall; prior < calls.size(); ++prior)
          for (const auto &other : calls[prior].writes) {
            if (!charge())
              return failure();
            if (!sameOccurrence(write.owner, other.owner))
              continue;
            bool recorded = false;
            for (const auto &path : write.paths) {
              for (const auto &previous : other.paths) {
                if (!charge(1 + path.selectors.size() +
                            previous.selectors.size()))
                  return failure();
                if (!overlaps(path.selectors, previous.selectors))
                  continue;
                if (pending.size() == 8192)
                  return diagnostic(call)
                         << "owner-enable proof pending pair budget exhausted";
                pending.push_back({module, write.owner, calls[prior].call, call,
                                   previous.assignment, path.assignment});
                recorded = true;
                break;
              }
              if (recorded)
                break;
            }
          }
      }
      calls.push_back(std::move(plan));
      return success();
    };
    // Registration is a source statement category, independent of effects or
    // return annotations. Inspect every other occurrence, including unused
    // callable bodies, before lowering any registration.
    auto expressions = [&](auto &&self, const AstNode &node,
                           const AstNode &registration,
                           const llvm::StringSet<> &shadows,
                           bool callableScope, const AstNode *captureSite) -> LogicalResult {
      if (!charge()) return failure();
      if (!node) return success();
      llvm::StringSet<> nestedShadows;
      const llvm::StringSet<> *scope = &shadows;
      if (node.kind() == "FunctionDef" || node.kind() == "Lambda") {
        callableScope = true;
        if (node.kind() == "FunctionDef" && node.array("decorator_list").empty())
          captureSite = &node;
        nestedShadows = shadows;
        auto arguments = node.child("args");
        for (StringRef field : {"args", "posonlyargs", "kwonlyargs"})
          for (size_t i = 0; i < arguments.array(field).size(); ++i)
            nestedShadows.insert(arguments.item(field, i).string("arg"));
        auto statements = node.array("body");
        for (size_t i = 0; statements && i < statements.size(); ++i)
          if (failed(collectLocals(collectLocals, node.item("body", i), nestedShadows))) return failure();
        scope = &nestedShadows;
      }
      // Module data formals do not turn an existing Rule into a callable
      // value. Preserve the write-analysis diagnostic before planning can
      // discard that registration; helper-local value shadows remain lexical.
      if (!callableScope && node.kind() == "Call" &&
          node.child("func").kind() == "Name" &&
          rules.contains(node.child("func").string("id")) &&
          moduleFormalShadows.contains(node.child("func").string("id")))
        return diagnostic(node.child("func"))
               << "source rule binding is shadowed in its lexical scope: '"
               << node.child("func").string("id") << "'";
      bool earlierData = false;
      StringRef referenced = node.kind() == "Name" ? node.string("id")
          : node.kind() == "Call" && node.child("func").kind() == "Name"
              ? node.child("func").string("id") : StringRef();
      if (!referenced.empty() && nestedRuleNames.contains(referenced) &&
          !scope->contains(referenced)) {
        auto before = orderBefore(captureSite ? *captureSite : node, referenced);
        if (failed(before)) return failure();
        bool assignmentTarget = false;
        if (!callableScope && node.kind() == "Name" && node.path.size() > module.path.size() + 2) {
          const auto &step = node.path[module.path.size() + 2];
          assignmentTarget = step.field == "targets";
        }
        earlierData = *before && (ordinaryData.contains(referenced) || assignmentTarget);
        if (earlierData && node.kind() == "Call")
          return diagnostic(node) << "ordinary data binding is not a callable Rule";
      }
      if (node.kind() == "Name" && rules.contains(node.string("id")) && !scope->contains(node.string("id")) && !earlierData) {
        auto function = registration ? registration.child("func") : AstNode();
        if (!function || !sameOccurrence(node, function))
          return diagnostic(node) << "Rule is only a complete Module/System registration; it is not a helper or callback";
      }
      if (node.kind() == "Call" && node.child("func").kind() == "Name" &&
          rules.contains(node.child("func").string("id")) && !scope->contains(node.child("func").string("id")) && !earlierData) {
        if (!registration || !sameOccurrence(node, registration))
          return diagnostic(node) << "Rule call must be a complete Module/System registration";
      }
      for (NamedAttribute raw : node.fields()) {
        auto name = raw.getName().getValue();
        if (name == "decorator_list" || name == "args" && node.kind() == "FunctionDef" || name == "returns") continue;
        if (auto array = dyn_cast<ArrayAttr>(raw.getValue())) {
          for (size_t j = 0; j < array.size(); ++j)
            if (auto child = dyn_cast<DictionaryAttr>(array[j]); child && child.getAs<StringAttr>("kind"))
              if (failed(self(self, node.item(name, j), registration, *scope, callableScope, captureSite))) return failure();
        } else if (auto child = dyn_cast<DictionaryAttr>(raw.getValue())) {
          if (child.getAs<StringAttr>("kind") && failed(self(self, node.child(name), registration, *scope, callableScope, captureSite))) return failure();
        }
      }
      return success();
    };
    for (size_t j = 0; j < module.array("body").size(); ++j) {
      auto stmt = module.item("body", j);

      if (stmt.kind() == "AnnAssign") {
        auto target = stmt.child("target");
        if (target.kind() == "Name")
          owners[target.string("id")] = stmt;
      } else if (stmt.kind() == "Assign" && stmt.array("targets").size() == 1) {
        auto rhs = stmt.child("value");
        if (rhs.kind() == "Call" && rhs.child("func").kind() == "Subscript" &&
            resolves(rhs.child("func").child("value"), MarkerKind::Table))
          owners[stmt.item("targets", 0).string("id")] = stmt;
      }
      AstNode call = llvm::is_contained(ArrayRef<StringRef>{"Expr", "Assign", "AnnAssign", "Return"}, stmt.kind())
                         ? stmt.child("value") : AstNode();
      bool registration = call.kind() == "Call" && call.child("func").kind() == "Name" &&
                          rules.contains(call.child("func").string("id")) && !moduleShadows.contains(call.child("func").string("id"));
      if (registration && ordinaryData.contains(call.child("func").string("id"))) {
        auto before = orderBefore(call, call.child("func").string("id"));
        if (failed(before)) return failure();
        if (*before) registration = false;
      }
      if (failed(expressions(expressions, stmt, registration ? call : AstNode(), moduleShadows, false, nullptr))) return failure();
      if (registration && failed(registerCall(call))) return failure();
      if (stmt.kind() == "Assign") {
        auto bind = [&](auto &&self, const AstNode &target) -> LogicalResult {
          if (target.kind() == "Tuple" || target.kind() == "List") {
            for (size_t k = 0; k < target.array("elts").size(); ++k)
              if (failed(self(self, target.item("elts", k)))) return failure();
            return success();
          }
          if (target.kind() != "Name" || !nestedRuleNames.contains(target.string("id"))) return success();
          auto before = orderBefore(stmt, target.string("id"));
          if (failed(before)) return failure();
          if (!*before || registrations.lookup(target.string("id")))
            return diagnostic(target) << "ordinary data cannot replace a declared or registered Rule";
          StringRef name = target.string("id");
          // contains and insertion each hash the borrowed name. A new key
          // owns its entry/text; bucket growth owns pointers, hashes and a
          // sentinel, and scans the previous bucket array before rehashing.
          if (!charge(name.size()) || !charge(1 + name.size())) return failure();
          if (!ordinaryData.contains(name)) {
            uint64_t oldCapacity = ordinaryData.getNumBuckets();
            uint64_t capacity = std::max(uint64_t(16), oldCapacity);
            uint64_t count = ordinaryData.size() + 1;
            while (count * 4 >= capacity * 3) capacity *= 2;
            uint64_t bytes = sizeof(llvm::StringMapEntry<std::nullopt_t>) + name.size() + 1;
            if (capacity > oldCapacity)
              bytes += capacity * (sizeof(void *) + sizeof(unsigned)) + sizeof(void *);
            if (!charge(1 + bytes / 8 + (bytes % 8 != 0) +
                        (capacity > oldCapacity ? oldCapacity : 0)))
              return failure();
            ordinaryData.insert(name);
          }
          return success();
        };
        for (size_t k = 0; k < stmt.array("targets").size(); ++k)
          if (failed(bind(bind, stmt.item("targets", k)))) return failure();
      }
    }
    for (const auto &name : nestedRuleNames)
      if (registrations.lookup(name.getKey()) != 1)
        return diagnostic(rules[name.getKey()]) << "Rule declaration must be registered exactly once";
  }
  return success();
}

namespace {
class AnalyzeRuleWritesPass
    : public PassWrapper<AnalyzeRuleWritesPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(AnalyzeRuleWritesPass)
  StringRef getArgument() const final { return "ac-analyze-rule-writes"; }
  StringRef getDescription() const final {
    return "Analyze registered source rule writer footprints before lowering";
  }
  void runOnOperation() override {
    const auto &analysis = getAnalysis<SourceRuleWritesAnalysis>();
    if (!analysis.isPlanValid()) {
      signalPassFailure();
      return;
    }
    if (!analysis.pendingPairs().empty())
      mlir::emitRemark(getOperation()->getLoc())
          << "collected " << analysis.pendingPairs().size()
          << " overlapping writer pairs; awaiting lowered owner-enable proof";
    markAllAnalysesPreserved();
  }
};
} // namespace
std::unique_ptr<Pass> createAnalyzeRuleWritesPass() {
  return std::make_unique<AnalyzeRuleWritesPass>();
}
void registerSourceCompilerPasses() {
  registerPass([] { return createAnalyzeRuleWritesPass(); });
}
} // namespace acir::compiler::detail
