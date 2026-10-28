#include "PythonImportRecords.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "PythonImportAST.h"
#include "PythonImportContext.h"
#include "PythonImportEnumSyntax.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>

using namespace mlir;
namespace acir::compiler {
namespace {
using detail::AstNode;
using detail::CapturedSource;
constexpr unsigned MaxNodes = 262144;
constexpr unsigned MaxWidth = 1048576;

struct NamedType {
  std::string name;
  Type type;
  AstNode node;
};
struct Callable {
  AstNode node, guard;
  std::string kind, name;
  SmallVector<NamedType> inputs;
  SmallVector<Type> results;
  bool autonomous = false, global = false;
  unsigned registrations = 0;
  bool defined = false;
};
struct Declaration {
  AstNode node;
  std::string name, symbol;
  SmallVector<NamedType> inputs;
  SmallVector<Type> results;
  std::map<std::string, Callable> callables;
  Operation *operation = nullptr;
  bool system = false;
  unsigned phase = 0;
};
struct Binding {
  Value value;
  std::string instanceTarget, instanceName;
  Declaration *pending = nullptr;
  Operation *pendingImport = nullptr;
  AstNode declaration;
  bool parameter = false;
  Value cachedRead;
  Type type() const { return value ? value.getType() : Type(); }
};
struct Scope {
  Declaration *owner = nullptr;
  Callable *callable = nullptr;
  std::map<std::string, Binding> bindings;
  StringRef pendingResultRequirement = "scalar";
  unsigned pendingResultArity = 1;
  SmallVector<Value> outputs;
  SmallVector<Type> resultTypes;
  SmallVector<Value> returned;
  unsigned conditionalDepth = 0;
  bool pure = false, module = false, terminated = false, globalRule = false;
};
struct ImportedName {
  std::string module, remote;
  FlatSymbolRefAttr symbol;
  Operation *declaration = nullptr;
  DictionaryAttr provider;
  AstNode site;
};
class Importer {
public:
  Importer(CapturedSource source, DictionaryAttr owner,
           const SourceHeaderRegistry &headers, ac::detail::EmitError error)
      : source(std::move(source)), owner(owner), headers(headers), error(error),
        b(owner.getContext()),
        result(ModuleOp::create(UnknownLoc::get(owner.getContext()))) {
    owner.getContext()->getOrLoadDialect<scf::SCFDialect>();
    prefix = owner.getAs<StringAttr>("package").getValue().str();
    StringRef path = owner.getAs<StringAttr>("path").getValue();
    SmallVector<StringRef> parts;
    path.drop_back(3).split(parts, '/');
    if (!parts.empty() && parts.back() == "__init__")
      parts.pop_back();
    for (auto part : parts) {
      if (!prefix.empty())
        prefix += '.';
      prefix += part.str();
    }
  }
  FailureOr<OwningOpRef<ModuleOp>> run();

private:
  CapturedSource source;
  DictionaryAttr owner;
  const SourceHeaderRegistry &headers;
  ac::detail::EmitError error;
  OpBuilder b;
  OwningOpRef<ModuleOp> result;
  std::string prefix;
  std::string currentDefinition;
  std::map<std::string, ImportedName> names;
  std::map<std::string, std::unique_ptr<Declaration>> declarations;
  std::map<std::string, Callable> globalRules;
  SmallVector<Attribute> dependencies, exports, imports;
  unsigned spent = 0, unique = 0;
  bool invalid = false;

  LogicalResult fail(const AstNode &node, const Twine &message) {
    invalid = true;
    emitError(node.location(b.getContext(), source.path)) << message;
    return failure();
  }
  bool charge(const AstNode &node) {
    if (++spent <= MaxNodes)
      return true;
    (void)fail(node, "transactional source lowering capacity exceeded");
    return false;
  }
  std::string qualify(StringRef name) const {
    return prefix.empty() ? name.str() : prefix + "." + name.str();
  }
  FlatSymbolRefAttr symbol(StringRef name) {
    return FlatSymbolRefAttr::get(b.getContext(), name);
  }
  DictionaryAttr origin(const AstNode &node, StringRef definition) {
    return detail::occurrence(b, symbol(definition), node);
  }
  DictionaryAttr site(const AstNode &node) {
    auto occ = origin(node, prefix.empty() ? "__source__" : prefix);
    return b.getDictionaryAttr(
        {b.getNamedAttr("ast_path",
                        occ.getAs<DictionaryAttr>("site").get("ast_path")),
         b.getNamedAttr("location", detail::sourceSpan(b, source.path, node))});
  }
  Operation *op(const AstNode &node, StringRef name, ValueRange operands,
                TypeRange types, ArrayRef<NamedAttribute> attrs = {},
                unsigned regions = 0) {
    // Complete the current operation even after a capacity diagnostic so
    // callers never dereference a null Operation. Expression/statement entry
    // checks stop further lowering, and run() rejects the invalid package.
    (void)charge(node);
    SmallVector<NamedAttribute> annotated(attrs);
    if (!currentDefinition.empty() &&
        !llvm::any_of(annotated, [](NamedAttribute a) {
          return a.getName() == "ac.origin";
        }))
      annotated.push_back(
          b.getNamedAttr("ac.origin", origin(node, currentDefinition)));
    return detail::createSourceOperation(
        b, node.location(b.getContext(), source.path), name, operands, types,
        annotated, regions);
  }
  void yield(const AstNode &node, ValueRange values = {}) {
    op(node, "ac.yield", values, {});
  }
  Type bits(unsigned width, const AstNode &node, StringRef definition = {}) {
    auto value = ac::MathIntAttr::get(
        b.getContext(), llvm::APSInt(llvm::APInt(64, width), true));
    auto tree = b.getDictionaryAttr(
        {b.getNamedAttr("kind", b.getStringAttr("literal")),
         b.getNamedAttr("value",
                        b.getDictionaryAttr(
                            {b.getNamedAttr("kind", b.getStringAttr("integer")),
                             b.getNamedAttr("value", value)})),
         b.getNamedAttr("origin", origin(node, definition.empty()
                                                   ? (currentDefinition.empty()
                                                          ? qualify("__types__")
                                                          : currentDefinition)
                                                   : definition)),
         b.getNamedAttr("location", detail::sourceSpan(b, source.path, node))});
    return ac::BitsType::get(b.getContext(),
                             ac::StaticExprAttr::get(b.getContext(), tree));
  }
  bool isResource(Type type) {
    return type && isa<ac::StateType, ac::QueueType, ac::PrimitiveType>(type);
  }
  Type payload(Type type) {
    if (auto r = dyn_cast_or_null<ac::StateType>(type))
      return r.getPayload();
    if (auto r = dyn_cast_or_null<ac::QueueType>(type))
      return r.getPayload();
    if (auto r = dyn_cast_or_null<ac::PrimitiveType>(type))
      return r.getPayload();
    return type;
  }
  Type resource(Type data, StringRef kind = "abstract", unsigned depth = 0) {
    if (isResource(data))
      return data;
    if (kind == "state")
      return ac::StateType::get(b.getContext(), data);
    if (kind == "queue")
      return ac::QueueType::get(b.getContext(), data);
    return ac::PrimitiveType::get(b.getContext(), data);
  }
  bool equal(Type a, Type c) {
    return a && c && ac::transactionalTypesEqual(a, c);
  }
  std::optional<llvm::APSInt> integer(const AstNode &node) {
    if (auto binding = resolve(node))
      if (isa_and_nonnull<ac::ConstantOp>(binding->declaration)) {
        auto value =
            binding->declaration->getAttrOfType<DictionaryAttr>("value");
        if (auto n = value.getAs<ac::MathIntAttr>("value"))
          return llvm::APSInt(n.getCanonicalValue());
        if (auto flag = value.getAs<BoolAttr>("value"))
          return llvm::APSInt(llvm::APInt(2, flag.getValue()), true);
      }
    if (node.kind() == "Constant") {
      if (auto flag = dyn_cast_or_null<BoolAttr>(node.get("value")))
        return llvm::APSInt(llvm::APInt(2, flag.getValue()), true);
      auto record = dyn_cast_or_null<DictionaryAttr>(node.get("value"));
      auto spelling =
          record ? record.getAs<StringAttr>("integer") : StringAttr();
      if (spelling) {
        auto parsed = detail::parseStaticInteger(b, spelling.getValue(), error);
        if (succeeded(parsed))
          return llvm::APSInt(parsed->getCanonicalValue());
      }
    }
    if (node.kind() == "UnaryOp" && node.child("op").kind() == "USub") {
      auto n = integer(node.child("operand"));
      if (n) {
        auto value = n->extend(n->getBitWidth() + 1);
        value.setIsSigned(true);
        return -value;
      }
    }
    return std::nullopt;
  }
  std::optional<ImportedName> resolve(const AstNode &node) {
    if (node.kind() == "Name") {
      auto found = names.find(node.string("id").str());
      if (found != names.end())
        return found->second;
    }
    if (node.kind() == "Attribute") {
      auto base = resolve(node.child("value"));
      if (base && !base->symbol && base->remote.empty()) {
        auto copy = *base;
        copy.remote = node.string("attr").str();
        if (copy.module != "pycircuit" && copy.module != "enum") {
          copy.symbol = headers.lookupExport(copy.module, copy.remote);
          if (copy.symbol)
            copy.declaration = headers.lookupDeclaration(copy.symbol);
        }
        return copy;
      }
    }
    return std::nullopt;
  }
  bool marker(const AstNode &node, StringRef name) {
    auto binding = resolve(node);
    return binding && binding->module == "pycircuit" &&
           binding->remote == name && !binding->symbol;
  }
  std::string decorator(const AstNode &node, AstNode *guard = nullptr) {
    auto list = node.array("decorator_list");
    if (!list || list.size() != 1)
      return {};
    auto dec = node.item("decorator_list", 0);
    auto target = dec.kind() == "Call" ? dec.child("func") : dec;
    for (StringRef name :
         {"module", "system", "rule", "autorule", "method", "struct"})
      if (marker(target, name)) {
        if (dec.kind() == "Call") {
          if (!dec.array("args").empty()) {
            (void)fail(dec, "decorators accept named options only");
            return {};
          }
          for (size_t i = 0; i < dec.array("keywords").size(); ++i) {
            auto kw = dec.item("keywords", i);
            if (kw.string("arg") == "guard" &&
                (name == "rule" || name == "autorule" || name == "method") &&
                (!guard || !*guard)) {
              auto lambda = kw.child("value");
              if (lambda.kind() != "Lambda") {
                (void)fail(lambda, "guard requires a source lambda");
                return {};
              }
              auto parameters = lambda.child("args");
              auto formals = node.child("args");
              if (!parameters.array("posonlyargs").empty() ||
                  !parameters.array("kwonlyargs").empty() ||
                  !parameters.array("defaults").empty() ||
                  parameters.child("vararg") || parameters.child("kwarg")) {
                (void)fail(
                    lambda,
                    "guard lambda requires required positional parameters");
                return {};
              }
              // A zero-argument guard captures the callable's lexical state.
              // Explicit argument-aware guards name the full SSA signature.
              if (!parameters.array("args").empty()) {
                if (parameters.array("args").size() !=
                    formals.array("args").size()) {
                  (void)fail(lambda, "guard lambda parameters must match the "
                                     "callable signature");
                  return {};
                }
                for (size_t argument = 0;
                     argument < parameters.array("args").size(); ++argument)
                  if (parameters.item("args", argument).string("arg") !=
                      formals.item("args", argument).string("arg")) {
                    (void)fail(lambda, "guard lambda parameter names and order "
                                       "must match the callable");
                    return {};
                  }
              }
              if (guard)
                *guard = lambda.child("body");
            } else {
              (void)fail(
                  kw,
                  "unsupported or duplicate transactional decorator option");
              return {};
            }
          }
        }
        return name.str();
      }
    return {};
  }
  Type annotation(const AstNode &node, StringRef definition);
  LogicalResult resultTypes(const AstNode &node, StringRef definition,
                            SmallVectorImpl<Type> &types);
  LogicalResult scan();
  LogicalResult nominals();
  LogicalResult signature(Callable &call, StringRef ownerSymbol);
  LogicalResult lower(Declaration &decl);
  LogicalResult statements(const AstNode &node, StringRef field, Scope &scope);
  LogicalResult statement(const AstNode &node, Scope &scope);
  FailureOr<SmallVector<Value>> expression(const AstNode &node, Scope &scope,
                                           Type expected = {},
                                           ArrayRef<Value> destinations = {});
  FailureOr<Value> scalar(const AstNode &node, Scope &scope,
                          Type expected = {});
  FailureOr<Value> convert(Value value, Type target, const AstNode &node);
  FailureOr<Value> constant(const AstNode &node, Type type,
                            const llvm::APSInt &number);
  Value truth(const AstNode &node, bool value) {
    auto type = bits(1, node);
    auto operation =
        op(node, "ac.bits.constant4", {}, type,
           {b.getNamedAttr("pattern", b.getStringAttr(value ? "1" : "0"))});
    return operation ? operation->getResult(0) : Value();
  }
  FailureOr<Value> read(Value handle, const AstNode &node, Scope &scope);
  LogicalResult write(Value handle, Value value, const AstNode &node,
                      Scope &scope);
  FailureOr<Value> allocate(Type data, StringRef name, const AstNode &node,
                            Scope &scope, AstNode initial = {},
                            unsigned depth = 0);
  FailureOr<SmallVector<Value>> invoke(const AstNode &node, Scope &scope,
                                       Type expected,
                                       ArrayRef<Value> destinations = {});
  LogicalResult defineRule(Callable &call, Scope &scope);
  FailureOr<SmallVector<Value>> registerRule(Callable &call,
                                             const AstNode &registration,
                                             Scope &scope,
                                             ArrayRef<Value> destinations = {});
  LogicalResult emitMethod(Callable &call, Scope &scope);
  FailureOr<SmallVector<Value>> methodCall(Callable &call, const AstNode &node,
                                           Scope &scope);
  LogicalResult emitGuard(Callable &call, Scope &scope);
  FailureOr<SmallVector<Value>> referenceCall(const AstNode &node, Scope &scope,
                                              Type expected);

  LogicalResult bind(const AstNode &target, ArrayRef<Value> values,
                     Scope &scope);
  FailureOr<SmallVector<Value>> instantiate(Declaration *decl,
                                            Operation *imported,
                                            StringRef instanceName,
                                            const AstNode &call, Scope &scope,
                                            ArrayRef<Value> destinations = {});
};

Type Importer::annotation(const AstNode &node, StringRef definition) {
  if (!node)
    return {};
  if (node.kind() == "Name" && node.string("id") == "bool")
    return bits(1, node, definition);
  if (auto binding = resolve(node)) {
    if (binding->module == "pycircuit") {
      StringRef name = binding->remote;
      unsigned size = 0;
      if (name.consume_front("u") && !name.getAsInteger(10, size) && size &&
          size <= 64)
        return bits(size, node, definition);
    }
    if (binding->symbol) {
      auto *decl = binding->declaration;
      if (isa_and_nonnull<ac::EnumOp>(decl))
        return ac::EnumType::get(b.getContext(),
                                 b.getStringAttr(binding->symbol.getValue()));
      if (isa_and_nonnull<ac::StructOp>(decl))
        return ac::StructType::get(b.getContext(),
                                   b.getStringAttr(binding->symbol.getValue()));
    }
  }
  if (node.kind() == "Subscript" && marker(node.child("value"), "queue")) {
    auto data = annotation(node.child("slice"), definition);
    return data ? resource(data, "queue") : Type();
  }
  if (node.kind() == "Subscript" && marker(node.child("value"), "bits")) {
    auto n = integer(node.child("slice"));
    if (n && !n->isNegative() && !n->isZero() && n->getActiveBits() <= 32 &&
        n->getZExtValue() <= MaxWidth)
      return bits(n->getZExtValue(), node, definition);
  }
  (void)fail(node,
             "transactional annotation requires a fixed Bits/bool or declared "
             "nominal payload");
  return {};
}

LogicalResult Importer::resultTypes(const AstNode &node, StringRef definition,
                                    SmallVectorImpl<Type> &types) {
  if (!node || (node.kind() == "Constant" &&
                isa_and_nonnull<UnitAttr>(node.get("value"))))
    return success();
  if (node.kind() == "Subscript" && node.child("value").kind() == "Name" &&
      node.child("value").string("id") == "tuple") {
    auto items = node.child("slice");
    if (items.kind() != "Tuple" || items.array("elts").size() < 2)
      return fail(node, "independent tuple results require at least two "
                        "declared payload types");
    for (size_t i = 0; i < items.array("elts").size(); ++i) {
      auto type = annotation(items.item("elts", i), definition);
      if (!type)
        return failure();
      types.push_back(type);
    }
    return success();
  }
  auto type = annotation(node, definition);
  if (!type)
    return failure();
  types.push_back(type);
  return success();
}

LogicalResult Importer::signature(Callable &call, StringRef ownerSymbol) {
  auto args = call.node.child("args");
  if (!args.array("posonlyargs").empty() || !args.array("kwonlyargs").empty() ||
      !args.array("defaults").empty() || args.child("vararg") ||
      args.child("kwarg"))
    return fail(call.node, "transactional callable requires explicit required "
                           "positional parameters");
  for (size_t i = 0; i < args.array("args").size(); ++i) {
    auto arg = args.item("args", i);
    if (names.count(arg.string("arg").str()))
      return fail(arg,
                  "parameters cannot shadow imported or declared source names");
    auto type = annotation(arg.child("annotation"), ownerSymbol);
    if (!type)
      return failure();
    call.inputs.push_back({arg.string("arg").str(), type, arg});
  }
  if (failed(
          resultTypes(call.node.child("returns"), ownerSymbol, call.results)))
    return failure();
  if (call.autonomous && (!call.inputs.empty() || !call.results.empty()))
    return fail(call.node, "autorule requires no parameters or results");
  return success();
}

LogicalResult Importer::scan() {
  dependencies.push_back(owner);
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto node = source.module.item("body", i);
    if (node.kind() == "Assign" || node.kind() == "Import" ||
        node.kind() == "ImportFrom" || node.kind() == "FunctionDef" ||
        node.kind() == "ClassDef" ||
        (node.kind() == "Expr" && node.child("value").kind() == "Constant" &&
         isa_and_nonnull<StringAttr>(node.child("value").get("value"))))
      continue;
    return fail(node, "transactional source top level admits declarations, "
                      "imports and documentation only");
  }
  // Import identity is resolved without importing or executing Python modules.
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto node = source.module.item("body", i);
    if (node.kind() != "Import" && node.kind() != "ImportFrom")
      continue;
    std::string fromProvider = node.string("module").str();
    if (node.kind() == "ImportFrom") {
      auto encoded = dyn_cast_or_null<DictionaryAttr>(node.get("level"));
      auto level =
          encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
      unsigned distance = 0;
      if (!level || level.getValue().getAsInteger(10, distance))
        return fail(node, "import level must be a nonnegative literal");
      if (distance) {
        std::string package = prefix;
        if (!StringRef(source.path).ends_with("__init__.py")) {
          auto dot = package.rfind('.');
          package = dot == std::string::npos ? "" : package.substr(0, dot);
        }
        for (unsigned i = 1; i < distance; ++i) {
          auto dot = package.rfind('.');
          if (dot == std::string::npos)
            return fail(node,
                        "relative import escapes the declared source package");
          package.resize(dot);
        }
        if (package.empty())
          return fail(node, "relative import has no declared package");
        fromProvider =
            package + (fromProvider.empty() ? "" : "." + fromProvider);
      }
    }
    for (size_t j = 0; j < node.array("names").size(); ++j) {
      auto alias = node.item("names", j);
      std::string remote = alias.string("name").str();
      std::string local = alias.string("asname").str();
      std::string provider = node.kind() == "Import" ? remote : fromProvider;
      if (local.empty())
        local = remote;
      if (names.count(local))
        return fail(alias, "duplicate imported binding");
      ImportedName binding{provider, node.kind() == "Import" ? "" : remote,
                           {},       nullptr,
                           {},       alias};
      if (provider != "pycircuit" && provider != "enum") {
        binding.provider = headers.ownerForModule(provider);
        if (!binding.provider)
          return fail(node,
                      "source import requires an explicit published interface");
        if (!llvm::is_contained(dependencies, binding.provider))
          dependencies.push_back(binding.provider);
        if (node.kind() == "ImportFrom") {
          binding.symbol = headers.lookupExport(provider, remote);
          if (!binding.symbol)
            return fail(alias, "source import names an unpublished definition");
          binding.declaration = headers.lookupDeclaration(binding.symbol);
          imports.push_back(b.getDictionaryAttr(
              {b.getNamedAttr("source", binding.provider),
               b.getNamedAttr("name", b.getStringAttr(remote)),
               b.getNamedAttr("target", binding.symbol),
               b.getNamedAttr("site", site(alias))}));
        }
      }
      names.emplace(local, std::move(binding));
    }
  }
  // Retain the complete explicitly supplied owner closure of each imported
  // interface. The registry has already authenticated these header records.
  for (size_t i = 0; i < dependencies.size(); ++i)
    for (auto header : headers.suppliedHeaders())
      if (header->getAttr("ac.source_owner") == dependencies[i])
        for (auto dependency :
             header->getAttrOfType<ArrayAttr>("ac.interfaces"))
          if (!llvm::is_contained(dependencies, dependency))
            dependencies.push_back(dependency);
  llvm::sort(dependencies.begin() + 1, dependencies.end(),
             [](Attribute a, Attribute c) {
               std::string left, right;
               llvm::raw_string_ostream lhs(left), rhs(right);
               a.print(lhs);
               c.print(rhs);
               return lhs.str() < rhs.str();
             });
  // Retain canonical imported declaration closure; no imported body is
  // invented.
  llvm::StringSet<> retained;
  for (auto header : headers.suppliedHeaders())
    if (llvm::is_contained(dependencies, header->getAttr("ac.source_owner")))
      for (Operation &declaration : header.getBody()->getOperations()) {
        auto name = declaration.getAttrOfType<StringAttr>("sym_name");
        if (!name || !retained.insert(name.getValue()).second)
          continue;
        auto *copy = declaration.clone();
        copy->setAttr("ac.declaration_role",
                      b.getStringAttr("import_snapshot"));
        result->getBody()->push_back(copy);
      }
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto node = source.module.item("body", i);
    if (node.kind() != "Assign")
      continue;
    auto target = node.item("targets", 0);
    if (node.array("targets").size() != 1 || target.kind() != "Name" ||
        names.count(target.string("id").str()))
      return fail(node, "source constant requires one unique named literal");
    auto n = integer(node.child("value"));
    if (!n)
      return fail(node,
                  "source constant requires an Integer or Boolean literal");
    bool flag = isa_and_nonnull<BoolAttr>(node.child("value").get("value"));
    auto kind = b.getStringAttr(flag ? "bool" : "integer");
    auto value = b.getDictionaryAttr(
        {b.getNamedAttr("kind", kind),
         b.getNamedAttr("value", flag ? Attribute(b.getBoolAttr(!n->isZero()))
                                      : Attribute(ac::MathIntAttr::get(
                                            b.getContext(), *n)))});
    auto name = target.string("id").str();
    auto sym = symbol(qualify(name));
    b.setInsertionPointToEnd(result->getBody());
    auto *declaration = op(
        node, "ac.constant", {}, {},
        {b.getNamedAttr("sym_name", b.getStringAttr(sym.getValue())),
         b.getNamedAttr("type",
                        b.getDictionaryAttr({b.getNamedAttr("kind", kind)})),
         b.getNamedAttr("value", value),
         b.getNamedAttr("ac.source_owner", owner),
         b.getNamedAttr("ac.origin", origin(node, sym.getValue())),
         b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition"))});
    names.emplace(name, ImportedName{"", name, sym, declaration, {}, node});
  }
  if (failed(nominals()))
    return failure();
  // Declarations precede body lowering so forward local instance references do
  // not require executing design functions or guessing from a fixture name.
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto node = source.module.item("body", i);
    if (node.kind() != "FunctionDef")
      continue;
    std::string kind = decorator(node);
    if (kind != "module" && kind != "system" && kind != "rule")
      return fail(node, "transactional source functions require "
                        "module/system/rule decoration");
    auto name = node.string("name").str();
    if (names.count(name))
      return fail(node,
                  "source declaration shadows an imported or nominal binding");
    if (kind == "rule") {
      Callable call;
      call.node = node;
      call.kind = "rule";
      call.name = qualify(name);
      call.global = true;
      call.guard = {};
      decorator(node, &call.guard);
      if (failed(signature(call, call.name)))
        return failure();
      globalRules.emplace(name, std::move(call));
      names.emplace(
          name,
          ImportedName{"", name, symbol(qualify(name)), nullptr, {}, node});
      continue;
    }
    auto decl = std::make_unique<Declaration>();
    decl->node = node;
    decl->name = name;
    decl->symbol = qualify(name);
    decl->system = kind == "system";
    names.emplace(
        name, ImportedName{"", name, symbol(decl->symbol), nullptr, {}, node});
    declarations.emplace(name, std::move(decl));
  }
  for (auto &[name, owned] : declarations) {
    auto &decl = *owned;
    auto args = decl.node.child("args");
    if (!args.array("posonlyargs").empty() ||
        !args.array("kwonlyargs").empty() || !args.array("defaults").empty() ||
        args.child("vararg") || args.child("kwarg"))
      return fail(decl.node, "Module parameters must be explicit required "
                             "resource/reference arguments");
    for (size_t i = 0; i < args.array("args").size(); ++i) {
      auto arg = args.item("args", i);
      if (!marker(arg.child("annotation"), "module")) {
        auto data = annotation(arg.child("annotation"), decl.symbol);
        if (!data)
          return failure();
        decl.inputs.push_back({arg.string("arg").str(), resource(data), arg});
        continue;
      }
      std::string name = arg.string("arg").str();
      if (names.count(name))
        return fail(arg, "Module parameter shadows a source declaration");
      std::string target = decl.symbol + ".__ref_" + name;
      auto type = ac::ModuleRefType::get(b.getContext(), symbol(target));
      OpBuilder::InsertionGuard save(b);
      b.setInsertionPointToEnd(result->getBody());
      op(arg, "ac.module.import", {}, {},
         {b.getNamedAttr("sym_name", b.getStringAttr(target)),
          b.getNamedAttr("source_owner", owner),
          b.getNamedAttr("parameters", b.getArrayAttr({})),
          b.getNamedAttr("type_parameters", b.getArrayAttr({})),
          b.getNamedAttr("function_type",
                         TypeAttr::get(b.getFunctionType({}, {}))),
          b.getNamedAttr("input_names", b.getArrayAttr({})),
          b.getNamedAttr("output_names", b.getArrayAttr({})),
          b.getNamedAttr("output_kinds", b.getArrayAttr({})),
          b.getNamedAttr("semantic_kind", b.getStringAttr("resources")),
          b.getNamedAttr("ac.reference_requirement", b.getBoolAttr(true)),
          b.getNamedAttr("ac.methods", b.getArrayAttr({})),
          b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition")),
          b.getNamedAttr("ac.origin", origin(arg, target))});
      decl.inputs.push_back({name, type, arg});
    }
    if (failed(
            resultTypes(decl.node.child("returns"), decl.symbol, decl.results)))
      return failure();
    if (decl.system && (!decl.inputs.empty() || !decl.results.empty()))
      return fail(decl.node, "System requires a closed zero-port root");
    for (size_t i = 0; i < decl.node.array("body").size(); ++i) {
      auto child = decl.node.item("body", i);
      if (child.kind() != "FunctionDef")
        continue;
      Callable call;
      call.node = child;
      call.name = child.string("name").str();
      call.kind = decorator(child, &call.guard);
      call.autonomous = call.kind == "autorule";
      if (call.kind != "rule" && call.kind != "method" && !call.autonomous)
        return fail(child, "nested functions require Rule/Method decoration");
      if (failed(signature(call, decl.symbol)))
        return failure();
      if (!decl.callables.emplace(call.name, std::move(call)).second)
        return fail(child, "duplicate nested callable");
    }
  }
  return success();
}

LogicalResult Importer::nominals() {
  // Register nominal names first so nested Structs can reference later types.
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto node = source.module.item("body", i);
    if (node.kind() != "ClassDef")
      continue;
    auto name = node.string("name").str();
    if (names.count(name))
      return fail(node, "duplicate nominal name");
    names.emplace(
        name, ImportedName{"", name, symbol(qualify(name)), nullptr, {}, node});
  }
  for (size_t i = 0; i < source.module.array("body").size(); ++i) {
    auto node = source.module.item("body", i);
    if (node.kind() != "ClassDef")
      continue;
    auto &binding = names[node.string("name").str()];
    OpBuilder::InsertionGuard save(b);
    b.setInsertionPointToEnd(result->getBody());
    auto metadata = SmallVector<NamedAttribute>{
        b.getNamedAttr("sym_name", b.getStringAttr(binding.symbol.getValue())),
        b.getNamedAttr("ac.source_owner", owner),
        b.getNamedAttr("ac.origin", origin(node, binding.symbol.getValue())),
        b.getNamedAttr("ac.declaration_role", b.getStringAttr("definition"))};
    if (decorator(node) == "struct") {
      if (!node.array("bases").empty() || !node.array("keywords").empty())
        return fail(node, "Struct does not admit inheritance");
      metadata.push_back(b.getNamedAttr("fields", b.getArrayAttr({})));
      binding.declaration = op(node, "ac.struct", {}, {}, metadata);
    } else {
      auto syntax = detail::readEnumSyntax(
          node,
          [&](const AstNode &at)
              -> std::optional<detail::ResolvedSourceBinding> {
            auto resolved = resolve(at);
            if (!resolved)
              return std::nullopt;
            return detail::ResolvedSourceBinding{
                detail::classifyImportedMarker(resolved->module,
                                               resolved->remote),
                resolved->symbol};
          },
          error);
      if (failed(syntax))
        return failure();
      auto size = integer(syntax->width);
      if (!size || size->isNegative() || size->isZero() ||
          size->getActiveBits() > 32 || size->getZExtValue() > MaxWidth)
        return fail(node, "Enum width requires a positive finite literal");
      SmallVector<Attribute> members;
      for (auto [index, member] : llvm::enumerate(syntax->members)) {
        std::optional<llvm::APSInt> number;
        if (!member.automatic)
          number = integer(member.declaration.child("value"));
        else {
          llvm::APInt value(std::max<unsigned>(65, index + 2), index);
          if (syntax->encoding == "gray_sequential")
            value ^= value.lshr(1);
          if (syntax->encoding == "binary_one_hot")
            value = llvm::APInt(value.getBitWidth(), 1).shl(index);
          number = llvm::APSInt(value, true);
        }
        if (!number)
          return fail(member.declaration,
                      "Enum code requires a closed integer literal");
        members.push_back(b.getDictionaryAttr(
            {b.getNamedAttr("name", b.getStringAttr(member.name)),
             b.getNamedAttr("code",
                            ac::MathIntAttr::get(b.getContext(), *number))}));
      }
      metadata.push_back(
          b.getNamedAttr("width", ac::MathIntAttr::get(b.getContext(), *size)));
      metadata.push_back(
          b.getNamedAttr("encoding", b.getStringAttr(syntax->encoding)));
      metadata.push_back(b.getNamedAttr("members", b.getArrayAttr(members)));
      binding.declaration = op(node, "ac.enum", {}, {}, metadata);
    }
  }
  for (auto &[name, binding] : names)
    if (isa_and_nonnull<ac::StructOp>(binding.declaration) &&
        !binding.provider) {
      SmallVector<Attribute> fields;
      for (size_t i = 0; i < binding.site.array("body").size(); ++i) {
        auto field = binding.site.item("body", i);
        if (field.kind() == "Pass")
          continue;
        if (field.kind() != "AnnAssign" ||
            field.child("target").kind() != "Name" || field.child("value"))
          return fail(field, "Struct fields require named type annotations "
                             "without runtime defaults");
        auto type =
            annotation(field.child("annotation"), binding.symbol.getValue());
        if (!type)
          return failure();
        fields.push_back(b.getDictionaryAttr(
            {b.getNamedAttr(
                 "name", b.getStringAttr(field.child("target").string("id"))),
             b.getNamedAttr("type", TypeAttr::get(type))}));
      }
      binding.declaration->setAttr("fields", b.getArrayAttr(fields));
    }
  return success();
}

// The production importer owns expression and statement lowering; immutable
// capture decoding and source provenance remain shared utilities.
#include "PythonImportExpressions.inc"
#include "PythonImportStatements.inc"

FailureOr<OwningOpRef<ModuleOp>> Importer::run() {
  result->getOperation()->setAttr("ac.transactional", b.getI64IntegerAttr(2));
  result->getOperation()->setAttr("ac.source_contract", b.getI64IntegerAttr(3));
  result->getOperation()->setAttr("ac.stage", b.getStringAttr("unlinked"));
  result->getOperation()->setAttr("ac.unit_kind",
                                  b.getStringAttr("implementation"));
  result->getOperation()->setAttr("ac.source_owner", owner);
  if (failed(scan()))
    return failure();
  Declaration globalOwner;
  globalOwner.symbol = prefix;
  Scope globalScope;
  globalScope.owner = &globalOwner;
  globalScope.module = true;
  b.setInsertionPointToEnd(result->getBody());
  for (auto &[name, call] : globalRules) {
    if (failed(defineRule(call, globalScope)))
      return failure();
    names[name].declaration = SymbolTable::lookupSymbolIn(*result, call.name);
  }
  for (auto &[name, decl] : declarations)
    if (failed(lower(*decl)))
      return failure();
  if (declarations.empty())
    result->getOperation()->setAttr("ac.unit_kind",
                                    b.getStringAttr("declarations"));
  for (auto &[name, binding] : names)
    if (binding.symbol && binding.declaration) {
      exports.push_back(
          b.getDictionaryAttr({b.getNamedAttr("name", b.getStringAttr(name)),
                               b.getNamedAttr("target", binding.symbol),
                               b.getNamedAttr("site", site(binding.site))}));
    }
  result->getOperation()->setAttr("ac.interfaces",
                                  b.getArrayAttr(dependencies));
  result->getOperation()->setAttr("ac.exports", b.getArrayAttr(exports));
  llvm::sort(imports, [](Attribute left, Attribute right) {
    return ac::detail::compareSourceImportBindings(
               cast<DictionaryAttr>(left), cast<DictionaryAttr>(right)) < 0;
  });
  result->getOperation()->setAttr("ac.import_bindings",
                                  b.getArrayAttr(imports));
  if (invalid)
    return failure();
  return std::move(result);
}
} // namespace

FailureOr<OwningOpRef<ModuleOp>>
importPythonSource(ModuleOp transport, DictionaryAttr owner,
                   const SourceHeaderRegistry &headers,
                   ac::detail::EmitError emitError) {
  if (failed(ac::detail::verifySourceOwner(owner, emitError)))
    return failure();
  auto captured = detail::readSingleCapture(transport, emitError);
  if (failed(captured))
    return failure();
  if (captured->path != owner.getAs<StringAttr>("path").getValue())
    return emitError()
           << "transactional capture path disagrees with SourceOwner";
  return Importer(std::move(*captured), owner, headers, emitError).run();
}
} // namespace acir::compiler
