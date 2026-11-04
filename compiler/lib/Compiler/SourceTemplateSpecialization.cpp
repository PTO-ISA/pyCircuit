#include "SourceTemplateSpecialization.h"
#include "PythonImportRecords.h"
#include "SourceHeaderHelpers.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringSet.h"
#include <functional>

using namespace mlir;
namespace acir::compiler {
namespace {
using detail::SourceTemplateBinding;
using detail::SourceTemplateRequest;

DictionaryAttr requestKey(Builder &b, const SourceTemplateRequest &request) {
  SmallVector<Attribute> values;
  for (const auto &binding : request.bindings)
    values.push_back(
        b.getDictionaryAttr({b.getNamedAttr("name", binding.name),
                             b.getNamedAttr("value", binding.value)}));
  return b.getDictionaryAttr(
      {b.getNamedAttr("definition", request.target),
       b.getNamedAttr("arguments", b.getArrayAttr(values))});
}

// Source origins use the concrete executable namespace after cloning. The
// original declaration, origin and full binding identity are retained
// separately on the specialized Module; suffix spelling supplies no semantic
// authority.
Type renameType(Type type, const llvm::StringMap<std::string> &mapping);
Attribute renameAttribute(Attribute attribute,
                          const llvm::StringMap<std::string> &mapping) {
  auto rename = [&](StringRef name) -> std::string {
    auto found = mapping.find(name);
    return found == mapping.end() ? name.str() : found->second;
  };
  if (auto type = dyn_cast<TypeAttr>(attribute))
    return TypeAttr::get(renameType(type.getValue(), mapping));
  if (auto symbol = dyn_cast<SymbolRefAttr>(attribute)) {
    auto name = rename(symbol.getRootReference());
    return SymbolRefAttr::get(attribute.getContext(), name,
                              symbol.getNestedReferences());
  }
  if (auto expression = dyn_cast<ac::StaticExprAttr>(attribute))
    return ac::StaticExprAttr::get(
        attribute.getContext(),
        cast<DictionaryAttr>(renameAttribute(expression.getTree(), mapping)));
  if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
    SmallVector<NamedAttribute> values;
    for (auto field : dictionary)
      values.emplace_back(field.getName(),
                          renameAttribute(field.getValue(), mapping));
    return DictionaryAttr::get(attribute.getContext(), values);
  }
  if (auto array = dyn_cast<ArrayAttr>(attribute)) {
    SmallVector<Attribute> values;
    for (Attribute item : array)
      values.push_back(renameAttribute(item, mapping));
    return ArrayAttr::get(attribute.getContext(), values);
  }
  if (auto domain = dyn_cast<ac::SourceDomainAttr>(attribute))
    return ac::SourceDomainAttr::get(
        attribute.getContext(),
        cast<DictionaryAttr>(renameAttribute(domain.getValue(), mapping)));
  return attribute;
}

Type renameType(Type type, const llvm::StringMap<std::string> &mapping) {
  if (auto bits = dyn_cast<ac::BitsType>(type))
    return ac::BitsType::get(
        type.getContext(),
        cast<ac::StaticExprAttr>(renameAttribute(bits.getWidth(), mapping)));
  if (auto table = dyn_cast<ac::TableType>(type))
    return ac::TableType::get(
        type.getContext(),
        cast<ArrayAttr>(renameAttribute(table.getShape(), mapping)),
        renameType(table.getElementType(), mapping));
  if (auto function = dyn_cast<FunctionType>(type)) {
    SmallVector<Type> inputs, results;
    for (Type input : function.getInputs())
      inputs.push_back(renameType(input, mapping));
    for (Type result : function.getResults())
      results.push_back(renameType(result, mapping));
    return FunctionType::get(type.getContext(), inputs, results);
  }
  if (auto formal = dyn_cast<ac::TypeParamType>(type)) {
    auto owner =
        cast<FlatSymbolRefAttr>(renameAttribute(formal.getOwner(), mapping));
    return ac::TypeParamType::get(type.getContext(), owner, formal.getName());
  }
  return type;
}

struct Variant {
  OwningOpRef<ModuleOp> body;
  OwningOpRef<ModuleOp> interface;
  ac::ModuleOp definition;
  ModuleInterfaceView view;
  DictionaryAttr key;
};

FailureOr<ModuleInterfaceView> publishVariant(Variant &variant) {
  auto body = *variant.body;
  ac::SourceMethodSummariesAnalysis methods(body);
  if (!methods.isValid())
    return failure();
  ac::HardwareAnalysis analysis(body);
  auto attributes = methods.getInterfaceAttributes(variant.definition, analysis);
  if (failed(attributes))
    return failure();
  NamedAttrList attrs(*attributes);
  attrs.erase("ac.template_specialization");
  attrs.erase("ac.template_origin");
  variant.interface = cast<ModuleOp>(body->clone());
  for (auto module :
       llvm::make_early_inc_range(variant.interface->getOps<ac::ModuleOp>()))
    module.erase();
  OperationState state(variant.definition.getLoc(),
                       ac::ModuleImportOp::getOperationName());
  state.addAttributes(attrs);
  auto declaration = cast<ac::ModuleImportOp>(Operation::create(state));
  variant.interface->getBody()->push_back(declaration);
  if (failed(verify(variant.interface->getOperation())))
    return failure();
  variant.view = detail::moduleInterfaceView(declaration);
  return variant.view;
}
} // namespace

FailureOr<std::string>
specializeSourceTemplates(ModuleOp candidate,
                          const SourceHeaderRegistry &headers, StringRef top,
                          ArrayRef<SourceLinkParameterBinding> parameters,
                          ac::detail::EmitError emitError) {
  if (candidate.getOps<ac::SourceTemplateOp>().empty())
    return top.str();
  Builder b(candidate.getContext());
  llvm::StringMap<ac::SourceTemplateOp> templates;
  llvm::DenseMap<Attribute, ArrayAttr> captures;
  if (auto payloads =
          candidate->getAttrOfType<ArrayAttr>("ac.source_templates"))
    for (Attribute raw : payloads) {
      auto payload = cast<DictionaryAttr>(raw);
      if (!captures
               .try_emplace(payload.get("owner"),
                            payload.getAs<ArrayAttr>("capture"))
               .second)
        return emitError() << "duplicate source template capture owner";
    }
  for (auto definition : candidate.getOps<ac::SourceTemplateOp>())
    templates[definition.getSymName()] = definition;
  llvm::DenseMap<Attribute, Variant *> complete;
  llvm::DenseSet<Attribute> active;
  SmallVector<std::unique_ptr<Variant>> storage;
  llvm::StringSet<> concreteSymbols;
  llvm::StringSet<> reservedSymbols;
  for (Operation &operation : candidate.getBody()->getOperations())
    if (auto symbol = SymbolTable::getSymbolName(&operation))
      reservedSymbols.insert(symbol.getValue());
  DictionaryAttr selectedRootKey;
  std::function<FailureOr<ModuleInterfaceView>(const SourceTemplateRequest &)>
      instantiate;
  instantiate = [&](const SourceTemplateRequest &request)
      -> FailureOr<ModuleInterfaceView> {
    auto source = templates.find(request.target.getValue());
    if (source == templates.end()) {
      auto view = headers.lookupModule(request.target);
      if (!view || view.semanticKind.getValue() == "source_template")
        return emitError()
               << "source template child has no supplied owning body: "
               << request.target;
      return view;
    }
    auto key = requestKey(b, request);
    if (auto found = complete.find(key); found != complete.end())
      return found->second->view;
    if (active.size() >= 128 || storage.size() >= 4096 ||
        !active.insert(key).second)
      return emitError()
             << "recursive or over-budget source template specialization";
    auto leave = llvm::scope_exit([&] { active.erase(key); });
    auto definition = source->second;
    if (request.bindings.size() != definition.getParameters().size())
      return emitError() << "source template must bind each Integer parameter";
    for (auto [raw, binding] :
         llvm::zip(definition.getParameters(), request.bindings))
      if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name") != binding.name ||
          !binding.value)
        return emitError()
               << "source template bindings must follow declaration order";
    auto capture = captures.lookup(definition.getCaptureOwner());
    if (!capture)
      return emitError() << "source template capture is absent";
    // The importer's declaration-only phase discovers callees and resolves
    // them before preparing this concrete executable body.
    auto lowered = detail::lowerBoundSourceTemplate(
        capture, definition.getSourceOwner(), request.target, request.bindings,
        headers, instantiate, emitError);
    if (failed(lowered))
      return failure();
    auto variant = std::make_unique<Variant>();
    variant->body = std::move(*lowered);
    for (auto module : variant->body->getOps<ac::ModuleOp>())
      if (module.getSymName() == request.target.getValue())
        variant->definition = module;
    if (!variant->definition)
      return emitError()
             << "source template lowering lost its selected definition";
    SmallVector<func::FuncOp> lexicalHelpers;
    for (Operation &operation : variant->body->getBody()->getOperations()) {
      if (&operation == variant->definition.getOperation())
        continue;
      if (auto helper = dyn_cast<func::FuncOp>(operation);
          helper && helper->getAttrOfType<FlatSymbolRefAttr>(
                        "ac.lexical_owner") == request.target) {
        lexicalHelpers.push_back(helper);
        continue;
      }
      if (auto symbol = SymbolTable::getSymbolName(&operation))
        reservedSymbols.insert(symbol.getValue());
    }
    auto helperName = [&](func::FuncOp helper, StringRef owner) {
      return owner.str() + helper.getSymName()
                               .drop_front(request.target.getValue().size())
                               .str();
    };
    std::string name = request.target.getValue().str();
    if (!request.bindings.empty() && key != selectedRootKey) {
      name += "_pyc_spec_" + std::to_string(storage.size());
      // The Module and its lexical helpers share the linked symbol table with
      // source declarations and every other specialization.
      while (reservedSymbols.contains(name) ||
             llvm::any_of(lexicalHelpers, [&](func::FuncOp helper) {
               return reservedSymbols.contains(helperName(helper, name));
             }))
        name += "_";
    }
    concreteSymbols.insert(name);
    reservedSymbols.insert(name);
    auto originalOrigin = variant->definition->getAttr("ac.origin");
    llvm::StringMap<std::string> symbolMapping;
    symbolMapping[request.target.getValue()] = name;
    for (auto helper : lexicalHelpers) {
      auto renamed = helperName(helper, name);
      reservedSymbols.insert(renamed);
      symbolMapping[helper.getSymName()] = std::move(renamed);
    }
    variant->body->walk([&](Operation *operation) {
      NamedAttrList attrs;
      for (NamedAttribute attr : operation->getAttrs())
        attrs.append(attr.getName(),
                     renameAttribute(attr.getValue(), symbolMapping));
      operation->setAttrs(attrs);
      for (Value result : operation->getResults())
        result.setType(renameType(result.getType(), symbolMapping));
      for (Region &region : operation->getRegions())
        for (Block &block : region)
          for (BlockArgument argument : block.getArguments())
            argument.setType(renameType(argument.getType(), symbolMapping));
      if (auto symbol = SymbolTable::getSymbolName(operation))
        if (auto found = symbolMapping.find(symbol.getValue());
            found != symbolMapping.end())
          SymbolTable::setSymbolName(operation, found->second);
    });
    variant->key = key;
    variant->definition->setAttr("ac.template_specialization", key);
    variant->definition->setAttr("ac.template_origin", originalOrigin);
    auto view = publishVariant(*variant);
    if (failed(view))
      return failure();
    auto *saved = variant.get();
    storage.push_back(std::move(variant));
    complete[key] = saved;
    return saved->view;
  };

  auto selected = templates.find(top);
  std::string selectedName = top.str();
  if (selected != templates.end()) {
    auto declaration = headers.lookupModule(selected->second.getTargetAttr());
    ac::HardwareAnalysis analysis(candidate);
    ac::HardwareBindings bindings;
    bindings.owner = declaration.declaration;
    auto arguments = detail::bindSourceRootParameters(declaration.declaration,
                                                     parameters, emitError);
    if (failed(arguments))
      return failure();
    SourceTemplateRequest request{selected->second.getTargetAttr(), {}};
    for (auto [raw, argument] :
         llvm::zip(selected->second.getParameters(), *arguments)) {
      auto name = cast<DictionaryAttr>(raw).getAs<StringAttr>("name");
      auto value = dyn_cast<ac::MathIntAttr>(argument);
      if (!value) {
        auto evaluated = analysis.evaluateStatic(
            cast<ac::StaticExprAttr>(argument), bindings, declaration.declaration);
        if (failed(evaluated))
          return failure();
        value = dyn_cast<ac::MathIntAttr>(*evaluated);
      }
      if (!value)
        return emitError() << "source template root parameter requires "
                              "mathematical Integer";
      bindings.integers[name.getValue()] = value;
      request.bindings.push_back({name, value});
    }
    selectedRootKey = requestKey(b, request);
    auto view = instantiate(request);
    if (failed(view))
      return failure();
    selectedName = view->symbol.getValue().str();
  }
  // All ordinary closed definitions remain validation roots, including siblings
  // in a deferred unit that are not reachable from the selected top.
  for (auto &entry : templates)
    if (entry.second.getParameters().empty() &&
        !entry.second->hasAttr("ac.reference_parameters") &&
        entry.first() != top)
      if (failed(instantiate({entry.second.getTargetAttr(), {}})))
        return failure();
  for (auto definition :
       llvm::make_early_inc_range(candidate.getOps<ac::SourceTemplateOp>()))
    definition.erase();
  for (auto imported :
       llvm::make_early_inc_range(candidate.getOps<ac::ModuleImportOp>()))
    if (imported.getSemanticKind() == "source_template")
      imported.erase();
  llvm::StringMap<Operation *> existing;
  for (Operation &operation : candidate.getBody()->getOperations())
    if (auto name = SymbolTable::getSymbolName(&operation))
      existing[name.getValue()] = &operation;
  for (auto &variant : storage) {
    for (Operation &operation : variant->body->getBody()->getOperations()) {
      auto imported = dyn_cast<ac::ModuleImportOp>(operation);
      if (imported && (imported.getSemanticKind() == "source_template" ||
                       concreteSymbols.contains(imported.getSymName())))
        continue;
      auto symbol = SymbolTable::getSymbolName(&operation);
      if (!symbol)
        return emitError()
               << "source template lowering produced a non-declaration";
      auto prior = existing.find(symbol.getValue());
      if (prior != existing.end()) {
        if (auto body = dyn_cast<ac::ModuleOp>(prior->second);
            body && imported) {
          // The owning body was admitted before specialization. A replay may
          // retain its declaration, but must still match that exact admitted
          // provider contract before the existing definition takes precedence.
          if (body.getSourceOwner() != imported.getSourceOwner() ||
              failed(headers.verifyModuleImport(imported, emitError)))
            return emitError() << "source template support import differs from "
                                  "its owning provider";
          continue;
        }
        if (isa<ac::ModuleOp>(operation) &&
            isa<ac::ModuleImportOp>(prior->second)) {
          prior->second->erase();
          existing.erase(prior);
        } else if (detail::sameDeclaration(prior->second, &operation))
          continue;
        else
          return emitError()
                 << "source template support declarations conflict: " << symbol;
      }
      Operation *copy = operation.clone();
      candidate.getBody()->push_back(copy);
      existing[symbol.getValue()] = copy;
    }
  }
  candidate->removeAttr("ac.source_templates");
  for (auto module : candidate.getOps<ac::ModuleOp>())
    if (concreteSymbols.contains(module.getSymName()))
      module->setAttr("parameters", b.getArrayAttr({}));
  candidate.walk([&](ac::InstanceOp instance) {
    if (concreteSymbols.contains(instance.getCallee()))
      instance->setAttr("parameters", b.getArrayAttr({}));
  });
  return selectedName;
}
} // namespace acir::compiler
