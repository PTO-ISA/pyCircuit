#include "pyc/Transforms/Passes.h"

#include "pyc/Dialect/PYC/PYCOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/SHA256.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

using namespace mlir;

namespace pyc {
namespace {

struct RtlSource {
  std::string path;
  std::string sha256;
  std::string license;
  bool modified = false;
};

struct RtlCandidate {
  std::string semanticId;
  std::string implementationId;
  std::string module;
  unsigned minWidth = 0;
  unsigned maxWidth = 0;
  int64_t selectionPriority = 0;
  std::vector<RtlSource> sources;
};

static std::string fingerprint(llvm::StringRef bytes) {
  llvm::SHA256 hasher;
  hasher.update(bytes);
  return "sha256:" + llvm::toHex(hasher.final(), true);
}

static FailureOr<std::vector<RtlCandidate>>
loadCatalog(llvm::StringRef path, std::string &catalogSha256,
            std::string &error) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  if (!buffer) {
    error = "cannot read RTL primitive catalog '" + path.str() + "'";
    return failure();
  }
  llvm::StringRef bytes = buffer.get()->getBuffer();
  catalogSha256 = fingerprint(bytes);
  auto parsed = llvm::json::parse(bytes);
  auto *root = parsed ? parsed->getAsObject() : nullptr;
  if (!root || root->getString("schema") != "pyc-rtl-catalog-v1") {
    error = "RTL primitive catalog must use schema pyc-rtl-catalog-v1";
    return failure();
  }
  auto *implementations = root->getArray("implementations");
  if (!implementations) {
    error = "RTL primitive catalog requires implementations array";
    return failure();
  }

  std::vector<RtlCandidate> result;
  for (const llvm::json::Value &raw : *implementations) {
    auto *entry = raw.getAsObject();
    auto semantic = entry ? entry->getString("semantic_id") : std::nullopt;
    auto implementation =
        entry ? entry->getString("implementation_id") : std::nullopt;
    auto effect = entry ? entry->getString("effect_class") : std::nullopt;
    auto module = entry ? entry->getString("module") : std::nullopt;
    auto minWidth = entry ? entry->getInteger("min_width") : std::nullopt;
    auto maxWidth = entry ? entry->getInteger("max_width") : std::nullopt;
    auto selectionPriority =
        entry ? entry->getInteger("selection_priority") : std::nullopt;
    auto *sources = entry ? entry->getArray("sources") : nullptr;
    auto *qualification = entry ? entry->getObject("qualification") : nullptr;
    auto qualificationStatus =
        qualification ? qualification->getString("status") : std::nullopt;
    auto qualificationReport =
        qualification ? qualification->getString("report") : std::nullopt;
    auto *ports = entry ? entry->getObject("ports") : nullptr;
    auto *inputPorts = ports ? ports->getArray("inputs") : nullptr;
    auto *outputPorts = ports ? ports->getArray("outputs") : nullptr;
    auto *bindings = entry ? entry->getObject("parameter_bindings") : nullptr;
    auto licenseFile = entry ? entry->getString("license_file") : std::nullopt;
    auto licenseSha256 =
        entry ? entry->getString("license_sha256") : std::nullopt;
    if (!semantic || !implementation || effect != "comb" || !module ||
        !minWidth || !maxWidth || !selectionPriority || *minWidth <= 0 ||
        *maxWidth < *minWidth || *maxWidth > 65536 || !sources ||
        sources->empty() || qualificationStatus != "validated" ||
        !qualificationReport || qualificationReport->empty() || !inputPorts ||
        inputPorts->size() != 1 ||
        inputPorts->front().getAsString() != "in_value" || !outputPorts ||
        outputPorts->size() != 2 ||
        outputPorts->front().getAsString() != "index" ||
        (*outputPorts)[1].getAsString() != "valid" || !bindings ||
        !bindings->get("WIDTH") || !bindings->get("ORDER_LOW") ||
        !licenseFile || !licenseSha256) {
      error = "RTL primitive catalog has malformed implementation entry";
      return failure();
    }
    bool licenseEscapes = llvm::sys::path::is_absolute(*licenseFile);
    for (auto part = llvm::sys::path::begin(*licenseFile),
              end = llvm::sys::path::end(*licenseFile);
         part != end; ++part)
      licenseEscapes |= *part == "..";
    llvm::SmallString<256> licensePath(llvm::sys::path::parent_path(path));
    llvm::sys::path::append(licensePath, *licenseFile);
    auto licenseBuffer = llvm::MemoryBuffer::getFile(licensePath);
    if (licenseFile->empty() || licenseFile->contains('\\') || licenseEscapes ||
        !licenseBuffer ||
        fingerprint(licenseBuffer.get()->getBuffer()) != *licenseSha256) {
      error = "RTL primitive license file is missing or has a digest mismatch";
      return failure();
    }
    RtlCandidate candidate;
    candidate.semanticId = semantic->str();
    candidate.implementationId = implementation->str();
    candidate.module = module->str();
    candidate.minWidth = static_cast<unsigned>(*minWidth);
    candidate.maxWidth = static_cast<unsigned>(*maxWidth);
    candidate.selectionPriority = *selectionPriority;
    for (const llvm::json::Value &rawSource : *sources) {
      auto *source = rawSource.getAsObject();
      auto sourcePath = source ? source->getString("path") : std::nullopt;
      auto sourceSha = source ? source->getString("sha256") : std::nullopt;
      auto sourceLicense = source ? source->getString("license") : std::nullopt;
      auto modified = source ? source->getBoolean("modified") : std::nullopt;
      if (!sourcePath || !sourceSha || !sourceLicense || !modified) {
        error = "RTL primitive catalog source entry is incomplete";
        return failure();
      }
      bool escapes = llvm::sys::path::is_absolute(*sourcePath);
      for (auto part = llvm::sys::path::begin(*sourcePath),
                end = llvm::sys::path::end(*sourcePath);
           part != end; ++part)
        escapes |= *part == "..";
      if (sourcePath->empty() || sourcePath->contains('\\') || escapes) {
        error =
            "RTL primitive catalog source path must be normalized and relative";
        return failure();
      }
      llvm::SmallString<256> sourceFile(llvm::sys::path::parent_path(path));
      llvm::sys::path::append(sourceFile, *sourcePath);
      auto sourceBuffer = llvm::MemoryBuffer::getFile(sourceFile);
      if (!sourceBuffer ||
          fingerprint(sourceBuffer.get()->getBuffer()) != *sourceSha) {
        error = "RTL primitive source digest mismatch for '" +
                sourcePath->str() + "'";
        return failure();
      }
      candidate.sources.push_back({sourcePath->str(), sourceSha->str(),
                                   sourceLicense->str(), *modified});
    }
    result.push_back(std::move(candidate));
  }
  llvm::sort(result, [](const RtlCandidate &lhs, const RtlCandidate &rhs) {
    return lhs.implementationId < rhs.implementationId;
  });
  for (size_t index = 1; index < result.size(); ++index)
    if (result[index - 1].implementationId == result[index].implementationId) {
      error = "RTL primitive implementation_id values must be unique";
      return failure();
    }
  return result;
}

struct SelectRtlPrimitivesPass
    : public PassWrapper<SelectRtlPrimitivesPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(SelectRtlPrimitivesPass)

  Option<std::string> catalog{
      *this, "catalog",
      llvm::cl::desc("Path to the qualified RTL primitive catalog"),
      llvm::cl::init("")};

  SelectRtlPrimitivesPass() = default;
  explicit SelectRtlPrimitivesPass(std::string path) {
    catalog = std::move(path);
  }
  SelectRtlPrimitivesPass(const SelectRtlPrimitivesPass &other)
      : PassWrapper(other) {
    catalog = other.catalog;
  }

  llvm::StringRef getArgument() const override {
    return "pyc-select-rtl-primitives";
  }
  llvm::StringRef getDescription() const override {
    return "Select qualified RTL implementations for semantic PYC primitives";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    bool containsSelected = false;
    module.walk([&](RtlCombOp) { containsSelected = true; });
    if (containsSelected) {
      module.emitError(
          "pyc.rtl.comb is backend-owned and cannot appear before selection");
      signalPassFailure();
      return;
    }
    SmallVector<PriorityEncodeOp> semanticOps;
    module.walk([&](PriorityEncodeOp op) { semanticOps.push_back(op); });
    if (semanticOps.empty())
      return;

    std::string path = catalog;
    if (path.empty())
      if (const char *environment = std::getenv("PYC_RTL_CATALOG"))
        path = environment;
    if (path.empty()) {
      module.emitError(
          "pyc-select-rtl-primitives requires --catalog or PYC_RTL_CATALOG");
      signalPassFailure();
      return;
    }

    std::string catalogSha256;
    std::string error;
    auto loaded = loadCatalog(path, catalogSha256, error);
    if (failed(loaded)) {
      module.emitError(error);
      signalPassFailure();
      return;
    }

    for (PriorityEncodeOp semantic : semanticOps) {
      unsigned width = cast<IntegerType>(semantic.getIn().getType()).getWidth();
      SmallVector<const RtlCandidate *> supported;
      for (const RtlCandidate &entry : *loaded)
        if (entry.semanticId == "pyc.priority_encode.v1" &&
            width >= entry.minWidth && width <= entry.maxWidth)
          supported.push_back(&entry);
      if (supported.empty()) {
        semantic.emitError()
            << "no qualified RTL implementation supports width " << width;
        signalPassFailure();
        return;
      }
      int64_t bestPriority =
          (*llvm::max_element(supported, [](const RtlCandidate *lhs,
                                            const RtlCandidate *rhs) {
            return lhs->selectionPriority < rhs->selectionPriority;
          }))->selectionPriority;
      SmallVector<const RtlCandidate *> preferred;
      llvm::copy_if(supported, std::back_inserter(preferred),
                    [&](const RtlCandidate *entry) {
                      return entry->selectionPriority == bestPriority;
                    });
      if (preferred.size() != 1) {
        semantic.emitError()
            << "RTL selection is ambiguous at priority " << bestPriority;
        signalPassFailure();
        return;
      }
      const RtlCandidate *candidate = preferred.front();

      OpBuilder builder(semantic);
      SmallVector<NamedAttribute> parameterValues{
          builder.getNamedAttr(
              "ORDER_LOW",
              builder.getI64IntegerAttr(semantic.getOrder() == "low" ? 1 : 0)),
          builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width)),
      };
      SmallVector<Attribute> sourceValues;
      for (const RtlSource &source : candidate->sources) {
        sourceValues.push_back(builder.getDictionaryAttr({
            builder.getNamedAttr("license",
                                 builder.getStringAttr(source.license)),
            builder.getNamedAttr("modified",
                                 builder.getBoolAttr(source.modified)),
            builder.getNamedAttr("path", builder.getStringAttr(source.path)),
            builder.getNamedAttr("sha256",
                                 builder.getStringAttr(source.sha256)),
        }));
      }

      OperationState state(semantic.getLoc(), RtlCombOp::getOperationName());
      state.addOperands(semantic.getIn());
      state.addTypes(semantic->getResultTypes());
      state.addAttribute("semantic_id",
                         builder.getStringAttr("pyc.priority_encode.v1"));
      state.addAttribute("implementation_id",
                         builder.getStringAttr(candidate->implementationId));
      state.addAttribute("module", builder.getStringAttr(candidate->module));
      state.addAttribute("parameters",
                         builder.getDictionaryAttr(parameterValues));
      state.addAttribute("input_ports", builder.getStrArrayAttr({"in_value"}));
      state.addAttribute("output_ports",
                         builder.getStrArrayAttr({"index", "valid"}));
      state.addAttribute("sources", builder.getArrayAttr(sourceValues));
      state.addAttribute("catalog_sha256",
                         builder.getStringAttr(catalogSha256));
      Operation *selected = builder.create(state);
      semantic.getIndex().replaceAllUsesWith(selected->getResult(0));
      semantic.getValid().replaceAllUsesWith(selected->getResult(1));
      semantic.erase();
    }
  }
};

} // namespace

std::unique_ptr<mlir::Pass>
createSelectRtlPrimitivesPass(std::string catalogPath) {
  return std::make_unique<SelectRtlPrimitivesPass>(std::move(catalogPath));
}

static PassRegistration<SelectRtlPrimitivesPass> pass;

} // namespace pyc
