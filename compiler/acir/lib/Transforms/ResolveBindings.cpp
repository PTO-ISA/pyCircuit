#include "acir/Transforms/ResolveBindings.h"

#include "acir/Analysis/ModelAnalysis.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Operation.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/Errc.h"

#include <algorithm>
#include <cmath>
#include <tuple>

using namespace mlir;

namespace acir {
namespace {

llvm::Error parameterError(const llvm::Twine &message) {
  return llvm::createStringError(llvm::errc::invalid_argument,
                                 "ACLOWER-PARAM-PHASE: %s",
                                 message.str().c_str());
}

llvm::Error bindingError(llvm::StringRef code, ac::ModuleExternOp module,
                         const llvm::Twine &detail) {
  llvm::StringRef binding =
      module.getImplementation().getAs<StringAttr>("name").getValue();
  return llvm::createStringError(
      llvm::errc::invalid_argument, "%s: key=@%s binding=%s %s",
      code.str().c_str(), module.getSymName().str().c_str(),
      binding.str().c_str(), detail.str().c_str());
}

std::string token(Type type) {
  std::string storage;
  llvm::raw_string_ostream output(storage);
  output << type;
  return storage;
}

std::string token(Attribute attribute) {
  std::string storage;
  llvm::raw_string_ostream output(storage);
  output << attribute;
  return storage;
}

std::string attributeTypeToken(Attribute attribute) {
  if (auto typed = dyn_cast<TypedAttr>(attribute))
    return token(typed.getType());
  if (auto dictionary = dyn_cast<DictionaryAttr>(attribute))
    if (auto amount = dictionary.getAs<IntegerAttr>("value"))
      if (dictionary.size() == 2 && dictionary.getAs<StringAttr>("unit"))
        return token(amount.getType());
  return "symbol";
}

llvm::Expected<llvm::json::Value> staticValue(Attribute attribute) {
  if (auto boolean = dyn_cast<BoolAttr>(attribute))
    return llvm::json::Value(boolean.getValue());
  if (auto integer = dyn_cast<IntegerAttr>(attribute)) {
    const llvm::APInt &value = integer.getValue();
    if (!value.isSignedIntN(64))
      return parameterError("integer static value exceeds signed i64");
    return llvm::json::Value(value.getSExtValue());
  }
  if (auto floating = dyn_cast<FloatAttr>(attribute)) {
    double value = floating.getValueAsDouble();
    if (!std::isfinite(value) || (std::signbit(value) && value == 0.0))
      return parameterError(
          "floating static value is non-finite or negative zero");
    return llvm::json::Value(value);
  }
  if (auto string = dyn_cast<StringAttr>(attribute))
    return llvm::json::Value(string.getValue());
  if (auto array = dyn_cast<ArrayAttr>(attribute)) {
    llvm::json::Array values;
    values.reserve(array.size());
    for (Attribute element : array) {
      auto value = staticValue(element);
      if (!value)
        return value.takeError();
      values.push_back(std::move(*value));
    }
    return llvm::json::Value(std::move(values));
  }
  if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
    llvm::json::Object values;
    for (NamedAttribute named : dictionary) {
      auto value = staticValue(named.getValue());
      if (!value)
        return value.takeError();
      values[named.getName().getValue()] = std::move(*value);
    }
    return llvm::json::Value(std::move(values));
  }
  if (isa<TypeAttr, SymbolRefAttr>(attribute))
    return llvm::json::Value(token(attribute));
  return parameterError(llvm::Twine("unsupported static attribute ") +
                        token(attribute));
}

llvm::Error normalizeRequest(ac::ModuleExternOp module,
                             const bindings::BindingRequest &request) {
  std::string expectedKey = (llvm::Twine("@") + module.getSymName()).str();
  llvm::StringRef declaredBinding =
      module.getImplementation().getAs<StringAttr>("name").getValue();
  if (request.resolutionKey != expectedKey ||
      request.binding != declaredBinding)
    return bindingError(
        "ACLOWER-BINDING-MISSING", module,
        "frozen request identity does not match the declaration");
  if (request.functionType != token(module.getFunctionType()))
    return bindingError("ACLOWER-TYPE-MISMATCH", module,
                        llvm::Twine("function_type expected=") +
                            request.functionType +
                            " actual=" + token(module.getFunctionType()));
  if (request.results.size() != module.getFunctionType().getNumResults())
    return bindingError("ACLOWER-TYPE-MISMATCH", module,
                        "frozen result count differs from the declaration");
  for (auto [required, actual] :
       llvm::zip_equal(request.results, module.getFunctionType().getResults()))
    if (required.acirType != token(actual))
      return bindingError("ACLOWER-TYPE-MISMATCH", module,
                          llvm::Twine("result '") + required.name +
                              "' expected=" + required.acirType +
                              " actual=" + token(actual));

  DictionaryAttr staticParameters = module.getStaticParams();
  llvm::StringSet<> consumed;
  for (const bindings::ParameterRequirement &parameter : request.parameters) {
    Attribute attribute = staticParameters.get(parameter.name);
    if (!attribute)
      return bindingError("ACLOWER-PARAM-PHASE", module,
                          llvm::Twine("missing frozen parameter '") +
                              parameter.name + "'");
    auto value = staticValue(attribute);
    if (!value)
      return value.takeError();
    if (parameter.acirType != attributeTypeToken(attribute) ||
        parameter.value != *value)
      return bindingError("ACLOWER-PARAM-PHASE", module,
                          llvm::Twine("normalized parameter '") +
                              parameter.name + "' differs");
    consumed.insert(parameter.name);
  }
  for (NamedAttribute named : staticParameters) {
    llvm::StringRef name = named.getName().getValue();
    if (!consumed.contains(name))
      return bindingError("ACLOWER-PARAM-PHASE", module,
                          llvm::Twine("unexpected frozen parameter '") + name +
                              "'");
  }
  return llvm::Error::success();
}

llvm::Expected<std::vector<bindings::BindingRequest>>
collectRequests(ModuleOp module,
                llvm::ArrayRef<bindings::BindingRequest> explicitRequests) {
  std::vector<bindings::BindingRequest> requests(explicitRequests.begin(),
                                                 explicitRequests.end());
  llvm::sort(requests, [](const bindings::BindingRequest &left,
                          const bindings::BindingRequest &right) {
    return std::tie(left.resolutionKey, left.binding) <
           std::tie(right.resolutionKey, right.binding);
  });
  llvm::StringSet<> consumed;
  for (const bindings::BindingRequest &request : requests) {
    ac::ModuleExternOp matched;
    for (ac::ModuleExternOp external : module.getOps<ac::ModuleExternOp>())
      if (request.resolutionKey ==
          (llvm::Twine("@") + external.getSymName()).str()) {
        matched = external;
        break;
      }
    if (!matched)
      return llvm::createStringError(
          llvm::errc::invalid_argument,
          "ACLOWER-BINDING-MISSING: key=%s binding=%s frozen request has no "
          "external declaration",
          request.resolutionKey.c_str(), request.binding.c_str());
    if (!consumed.insert(request.resolutionKey).second)
      return bindingError("ACLOWER-BINDING-AMBIGUOUS", matched,
                          "duplicate frozen request key");
    if (llvm::Error error = normalizeRequest(matched, request))
      return std::move(error);
  }
  for (ac::ModuleExternOp external : module.getOps<ac::ModuleExternOp>()) {
    std::string key = (llvm::Twine("@") + external.getSymName()).str();
    if (!consumed.contains(key))
      return bindingError("ACLOWER-BINDING-MISSING", external,
                          "exact frozen architecture request is absent");
  }
  return requests;
}

class ResolveBindingsPass
    : public PassWrapper<ResolveBindingsPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(ResolveBindingsPass)

  explicit ResolveBindingsPass(ResolveBindingsPassOptions options)
      : options(std::move(options)) {}

  llvm::StringRef getArgument() const override {
    return "ac-resolve-gfsim-bindings";
  }

  llvm::StringRef getDescription() const override {
    return "Resolve exact metadata-only gfsim bindings and emit a canonical "
           "lock";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    if (options.lockOutputPath.empty()) {
      module.emitError(
          "ACLOWER-BINDING-OPTIONS: binding lock output path is required");
      signalPassFailure();
      return;
    }
    auto result = resolveModuleBindings(module, options);
    if (!result) {
      module.emitError(llvm::toString(result.takeError()));
      signalPassFailure();
      return;
    }
    if (llvm::Error error = bindings::emitBindingLockAtomically(
            *result, options.lockOutputPath)) {
      module.emitError(llvm::toString(std::move(error)));
      signalPassFailure();
      return;
    }
    markAllAnalysesPreserved();
  }

private:
  ResolveBindingsPassOptions options;
};

} // namespace

llvm::Expected<bindings::BindingResolutionResult>
resolveModuleBindings(ModuleOp module,
                      const ResolveBindingsPassOptions &options) {
  auto frozen = module->getAttrOfType<BoolAttr>("ac.topology_frozen");
  auto epoch = module->getAttrOfType<StringAttr>("ac.freeze_epoch");
  if (!frozen || !frozen.getValue() || !epoch || epoch.getValue() != "0.5")
    return llvm::createStringError(
        llvm::errc::invalid_argument,
        "ACLOWER-BINDING-MISSING: frozen ACIR topology epoch 0.5 is required");
  if (failed(verifyModel(module)))
    return llvm::createStringError(llvm::errc::invalid_argument,
                                   "ACLOWER-FINGERPRINT: frozen ACIR topology "
                                   "integrity verification failed");
  if (options.profile.empty() || options.target.empty())
    return llvm::createStringError(
        llvm::errc::invalid_argument,
        "ACLOWER-PROFILE: selected profile and target must be explicit");
  for (const bindings::BindingCandidate &candidate : options.candidates)
    if (llvm::Error error = candidate.record().validateFingerprint())
      return std::move(error);
  auto requests = collectRequests(module, options.requests);
  if (!requests)
    return requests.takeError();
  return bindings::resolveBindings(options.candidates, *requests,
                                   options.profile, options.target);
}

std::unique_ptr<Pass>
createResolveBindingsPass(ResolveBindingsPassOptions options) {
  return std::make_unique<ResolveBindingsPass>(std::move(options));
}

} // namespace acir
