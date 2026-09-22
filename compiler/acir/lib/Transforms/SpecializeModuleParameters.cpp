#include "acir/Transforms/Passes.h"

#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <utility>

using namespace mlir;

namespace acir {
namespace {

#define GEN_PASS_DEF_SPECIALIZEMODULEPARAMETERSPASS
#include "acir/Transforms/Passes.h.inc"

/// Fold every parameter read in ``module`` to the concrete constant bound by
/// ``environment``. A read is only ever replaced by its value, so the result
/// keeps its type and no downstream stage has to know the body was generic.
void foldParameters(ac::ModuleOp module, DictionaryAttr environment) {
  SmallVector<ac::ParamGetOp> reads;
  module.walk([&](ac::ParamGetOp read) { reads.push_back(read); });
  for (ac::ParamGetOp read : reads) {
    Attribute value = environment.get(read.getParameter());
    if (!value)
      continue;
    OpBuilder builder(read);
    auto constant = builder.create<ac::VarConstantOp>(
        read.getLoc(), read.getResult().getType(), value);
    read.getResult().replaceAllUsesWith(constant.getResult());
    read.erase();
  }
}

/// Deterministic clone name for one static-argument environment. The suffix is
/// derived from the environment itself, so one argument set always yields one
/// clone and the name never depends on traversal order.
std::string specializedName(ac::ModuleOp source, DictionaryAttr environment) {
  std::string printed;
  llvm::raw_string_ostream stream(printed);
  environment.print(stream);
  stream.flush();
  llvm::SHA256 hash;
  hash.update(source.getSymName());
  hash.update("\0");
  hash.update(printed);
  const auto digest = hash.final();
  std::string hex;
  for (unsigned index = 0; index < 6; ++index) {
    hex.push_back(llvm::hexdigit(digest[index] >> 4, /*LowerCase=*/true));
    hex.push_back(llvm::hexdigit(digest[index] & 0xF, /*LowerCase=*/true));
  }
  return (source.getSymName() + "__p" + hex).str();
}

struct SpecializeModuleParametersPass
    : impl::SpecializeModuleParametersPassBase<SpecializeModuleParametersPass> {
  void runOnOperation() override {
    ModuleOp model = getOperation();

    // A definition is generic only when its body actually reads a parameter.
    SmallVector<ac::ModuleOp> generic;
    llvm::SmallPtrSet<Operation *, 8> seen;
    model.walk([&](ac::ParamGetOp read) {
      auto module = read->getParentOfType<ac::ModuleOp>();
      if (module && seen.insert(module.getOperation()).second)
        generic.push_back(module);
    });
    if (generic.empty())
      return;

    llvm::StringMap<SmallVector<ac::InstanceOp>> instances;
    model.walk([&](ac::InstanceOp instance) {
      instances[instance.getDefinition()].push_back(instance);
    });

    for (ac::ModuleOp source : generic) {
      auto found = instances.find(source.getSymName());
      if (found == instances.end())
        continue;

      // Every argument dictionary this definition has to serve: its own
      // declared dictionary plus each instance binding.
      SmallVector<DictionaryAttr> environments;
      auto remember = [&](DictionaryAttr environment) {
        if (environment && !llvm::is_contained(environments, environment))
          environments.push_back(environment);
      };
      remember(source.getStaticParams());
      for (ac::InstanceOp instance : found->second)
        remember(instance.getStaticArgs());

      // Clone every non-declared environment from the still-generic source
      // before any folding, otherwise a clone made after the in-place fold
      // would inherit the wrong constants.
      SmallVector<std::pair<DictionaryAttr, ac::ModuleOp>> targets;
      OpBuilder builder(source);
      builder.setInsertionPointAfter(source);
      for (DictionaryAttr environment : environments) {
        if (environment == source.getStaticParams()) {
          targets.push_back({environment, source});
          continue;
        }
        auto clone = cast<ac::ModuleOp>(builder.clone(*source.getOperation()));
        clone.setSymName(specializedName(source, environment));
        clone->setAttr("static_params", environment);
        targets.push_back({environment, clone});
      }

      for (auto &entry : targets)
        foldParameters(entry.second, entry.first);

      for (auto &entry : targets) {
        if (entry.second == source)
          continue;
        for (ac::InstanceOp instance : found->second)
          if (instance.getStaticArgs() == entry.first)
            instance.setDefinition(entry.second.getSymName());
      }
    }
  }
};

} // namespace

std::unique_ptr<Pass> createSpecializeModuleParametersPass() {
  return std::make_unique<SpecializeModuleParametersPass>();
}

} // namespace acir
