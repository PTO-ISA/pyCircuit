#include "pyc/Transforms/Passes.h"

#include "pyc/Dialect/PYC/PYCOps.h"
#include "pyc/Generated/SemanticPrimitiveRegistry.h"

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

// Catalog digests describe Git's canonical text blobs.  A Windows checkout
// may materialize those files with CRLF, so normalize only CRLF pairs before
// checking source and license content.
static std::string fingerprintRepositoryText(llvm::StringRef bytes) {
  if (!bytes.contains("\r\n"))
    return fingerprint(bytes);
  std::string normalized;
  normalized.reserve(bytes.size());
  for (size_t index = 0; index < bytes.size(); ++index) {
    if (bytes[index] == '\r' && index + 1 < bytes.size() &&
        bytes[index + 1] == '\n')
      continue;
    normalized.push_back(bytes[index]);
  }
  return fingerprint(normalized);
}

// These semantics are implementation details of wrapping_bitfield lowering,
// not public PYC operations.  Keep the list closed here so catalog loading
// still rejects arbitrary semantic IDs while allowing the decomposed physical
// primitives to participate in selection.
static bool isBitfieldDecompositionSemantic(llvm::StringRef semanticId) {
  return semanticId == "pyc.wrapping_field_normalize.v1" ||
         semanticId == "pyc.bitfield_clear.v1" ||
         semanticId == "pyc.bitfield_set.v1" ||
         semanticId == "pyc.bitfield_insert.v1" ||
         semanticId == "pyc.reverse_bytes.v1" ||
         semanticId == "pyc.dynamic_sign_extend.v1" ||
         semanticId == "pyc.runtime_zero_count.v1";
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
    bool knownSemanticShape = false;
    auto hasPorts = [](const llvm::json::Array *actual,
                       std::initializer_list<llvm::StringRef> expected) {
      if (!actual || actual->size() != expected.size())
        return false;
      size_t index = 0;
      for (llvm::StringRef name : expected) {
        auto value = (*actual)[index++].getAsString();
        if (!value || *value != name)
          return false;
      }
      return true;
    };
    auto hasBindings = [](const llvm::json::Object *actual,
                          std::initializer_list<llvm::StringRef> expected) {
      if (!actual || actual->size() != expected.size())
        return false;
      return llvm::all_of(
          expected, [&](llvm::StringRef name) { return actual->get(name); });
    };
    if (semantic && inputPorts && outputPorts && bindings) {
      if (*semantic == "pyc.priority_encode.v1")
        knownSemanticShape = hasPorts(inputPorts, {"in_value"}) &&
                             hasPorts(outputPorts, {"index", "valid"}) &&
                             hasBindings(bindings, {"WIDTH", "ORDER_LOW"});
      else if (*semantic == "pyc.popcount.v1")
        knownSemanticShape = hasPorts(inputPorts, {"in_value"}) &&
                             hasPorts(outputPorts, {"count"}) &&
                             hasBindings(bindings, {"WIDTH", "COUNT_WIDTH"});
      else if (*semantic == "pyc.count_zeros.v1")
        knownSemanticShape =
            hasPorts(inputPorts, {"in_value"}) &&
            hasPorts(outputPorts, {"count"}) &&
            hasBindings(bindings, {"WIDTH", "COUNT_WIDTH", "DIRECTION_LOW"});
      else if (*semantic == "pyc.wrapping_bitfield.v1")
        knownSemanticShape =
            hasPorts(inputPorts,
                     {"value", "source", "bit_width", "bit_offset", "mode"}) &&
            hasPorts(outputPorts, {"result"}) &&
            hasBindings(bindings, {"WIDTH", "CONTROL_WIDTH", "MODE_WIDTH"});
      else if (*semantic == "pyc.wrapping_field_normalize.v1")
        knownSemanticShape =
            hasPorts(inputPorts, {"value", "bit_width", "bit_offset"}) &&
            hasPorts(outputPorts, {"field", "mask"}) &&
            hasBindings(bindings, {"WIDTH", "CONTROL_WIDTH"});
      else if (*semantic == "pyc.bitfield_clear.v1" ||
               *semantic == "pyc.bitfield_set.v1")
        knownSemanticShape = hasPorts(inputPorts, {"value", "mask"}) &&
                             hasPorts(outputPorts, {"result"}) &&
                             hasBindings(bindings, {"WIDTH"});
      else if (*semantic == "pyc.bitfield_insert.v1")
        knownSemanticShape =
            hasPorts(inputPorts,
                     {"value", "source", "mask", "bit_width", "bit_offset"}) &&
            hasPorts(outputPorts, {"result"}) &&
            hasBindings(bindings, {"WIDTH", "CONTROL_WIDTH"});
      else if (*semantic == "pyc.reverse_bytes.v1")
        knownSemanticShape = hasPorts(inputPorts, {"field", "bit_width"}) &&
                             hasPorts(outputPorts, {"result"}) &&
                             hasBindings(bindings, {"WIDTH", "CONTROL_WIDTH"});
      else if (*semantic == "pyc.dynamic_sign_extend.v1")
        knownSemanticShape = hasPorts(inputPorts, {"field", "bit_width"}) &&
                             hasPorts(outputPorts, {"result"}) &&
                             hasBindings(bindings, {"WIDTH", "CONTROL_WIDTH"});
      else if (*semantic == "pyc.runtime_zero_count.v1")
        knownSemanticShape = hasPorts(inputPorts, {"value", "direction_low"}) &&
                             hasPorts(outputPorts, {"count"}) &&
                             hasBindings(bindings, {"WIDTH", "COUNT_WIDTH"});
    }
    if (!semantic || !implementation || effect != "comb" || !module ||
        !minWidth || !maxWidth || !selectionPriority || *minWidth <= 0 ||
        *maxWidth < *minWidth || *maxWidth > 65536 || !sources ||
        sources->empty() || qualificationStatus != "validated" ||
        !qualificationReport || qualificationReport->empty() ||
        !knownSemanticShape || !licenseFile || !licenseSha256) {
      error = "RTL primitive catalog has malformed implementation entry";
      return failure();
    }
    const generated::SemanticPrimitiveContract *semanticContract =
        generated::findSemanticPrimitive(semantic->str());
    const bool isInternalBitfieldSemantic =
        isBitfieldDecompositionSemantic(semantic->str());
    if ((!semanticContract && !isInternalBitfieldSemantic) ||
        (semanticContract && (static_cast<unsigned>(*minWidth) <
                                  semanticContract->minimumInputWidth ||
                              static_cast<unsigned>(*maxWidth) >
                                  semanticContract->maximumInputWidth))) {
      error = "RTL primitive catalog entry is outside the semantic registry";
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
        fingerprintRepositoryText(licenseBuffer.get()->getBuffer()) !=
            *licenseSha256) {
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
      if (!sourceBuffer || fingerprintRepositoryText(
                               sourceBuffer.get()->getBuffer()) != *sourceSha) {
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
    SmallVector<PriorityEncodeOp> priorityOps;
    SmallVector<PopcountOp> popcountOps;
    SmallVector<CountZerosOp> zeroCountOps;
    SmallVector<WrappingBitfieldOp> wrappingBitfieldOps;
    module.walk([&](PriorityEncodeOp op) { priorityOps.push_back(op); });
    module.walk([&](PopcountOp op) { popcountOps.push_back(op); });
    module.walk([&](CountZerosOp op) { zeroCountOps.push_back(op); });
    module.walk(
        [&](WrappingBitfieldOp op) { wrappingBitfieldOps.push_back(op); });
    if (priorityOps.empty() && popcountOps.empty() && zeroCountOps.empty() &&
        wrappingBitfieldOps.empty())
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

    auto selectCandidate = [&](Operation *semantic, llvm::StringRef semanticId,
                               unsigned width) -> const RtlCandidate * {
      SmallVector<const RtlCandidate *> supported;
      for (const RtlCandidate &entry : *loaded)
        if (entry.semanticId == semanticId && width >= entry.minWidth &&
            width <= entry.maxWidth)
          supported.push_back(&entry);
      if (supported.empty()) {
        semantic->emitError()
            << "no qualified RTL implementation supports width " << width;
        return nullptr;
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
        semantic->emitError()
            << "RTL selection is ambiguous at priority " << bestPriority;
        return nullptr;
      }
      return preferred.front();
    };

    auto sourceAttributes = [&](OpBuilder &builder,
                                const RtlCandidate &candidate) {
      SmallVector<Attribute> sourceValues;
      for (const RtlSource &source : candidate.sources)
        sourceValues.push_back(builder.getDictionaryAttr({
            builder.getNamedAttr("license",
                                 builder.getStringAttr(source.license)),
            builder.getNamedAttr("modified",
                                 builder.getBoolAttr(source.modified)),
            builder.getNamedAttr("path", builder.getStringAttr(source.path)),
            builder.getNamedAttr("sha256",
                                 builder.getStringAttr(source.sha256)),
        }));
      return sourceValues;
    };

    auto createRtlComb =
        [&](Operation *anchor, llvm::StringRef semanticId,
            unsigned candidateWidth, ArrayRef<Value> operands,
            ArrayRef<Type> resultTypes, ArrayRef<NamedAttribute> parameters,
            ArrayRef<llvm::StringRef> inputPorts,
            ArrayRef<llvm::StringRef> outputPorts) -> Operation * {
      const RtlCandidate *candidate =
          selectCandidate(anchor, semanticId, candidateWidth);
      if (!candidate)
        return nullptr;
      OpBuilder builder(anchor);
      OperationState state(anchor->getLoc(), RtlCombOp::getOperationName());
      state.addOperands(operands);
      state.addTypes(resultTypes);
      state.addAttribute("semantic_id", builder.getStringAttr(semanticId));
      state.addAttribute("implementation_id",
                         builder.getStringAttr(candidate->implementationId));
      state.addAttribute("module", builder.getStringAttr(candidate->module));
      state.addAttribute("parameters", builder.getDictionaryAttr(parameters));
      state.addAttribute("input_ports", builder.getStrArrayAttr(inputPorts));
      state.addAttribute("output_ports", builder.getStrArrayAttr(outputPorts));
      state.addAttribute("sources", builder.getArrayAttr(
                                        sourceAttributes(builder, *candidate)));
      state.addAttribute("catalog_sha256",
                         builder.getStringAttr(catalogSha256));
      return builder.create(state);
    };

    for (PriorityEncodeOp semantic : priorityOps) {
      unsigned width = cast<IntegerType>(semantic.getIn().getType()).getWidth();
      OpBuilder builder(semantic);
      SmallVector<NamedAttribute> parameterValues{
          builder.getNamedAttr(
              "ORDER_LOW",
              builder.getI64IntegerAttr(semantic.getOrder() == "low" ? 1 : 0)),
          builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width)),
      };
      SmallVector<Value> inputs{semantic.getIn()};
      SmallVector<Type> outputs(semantic->getResultTypes());
      Operation *selected = createRtlComb(
          semantic, "pyc.priority_encode.v1", width, inputs, outputs,
          parameterValues, {"in_value"}, {"index", "valid"});
      if (!selected) {
        signalPassFailure();
        return;
      }
      semantic.getIndex().replaceAllUsesWith(selected->getResult(0));
      semantic.getValid().replaceAllUsesWith(selected->getResult(1));
      semantic.erase();
    }

    for (PopcountOp semantic : popcountOps) {
      unsigned width = cast<IntegerType>(semantic.getIn().getType()).getWidth();
      unsigned countWidth =
          cast<IntegerType>(semantic.getCount().getType()).getWidth();
      OpBuilder builder(semantic);
      SmallVector<NamedAttribute> parameterValues{
          builder.getNamedAttr("COUNT_WIDTH",
                               builder.getI64IntegerAttr(countWidth)),
          builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width)),
      };
      SmallVector<Value> inputs{semantic.getIn()};
      SmallVector<Type> outputs(semantic->getResultTypes());
      Operation *selected =
          createRtlComb(semantic, "pyc.popcount.v1", width, inputs, outputs,
                        parameterValues, {"in_value"}, {"count"});
      if (!selected) {
        signalPassFailure();
        return;
      }
      semantic.getCount().replaceAllUsesWith(selected->getResult(0));
      semantic.erase();
    }

    for (WrappingBitfieldOp semantic : wrappingBitfieldOps) {
      if (semantic.getInputs().size() != 5) {
        semantic.emitError("wrapping_bitfield requires five inputs");
        signalPassFailure();
        return;
      }
      unsigned width =
          cast<IntegerType>(semantic.getInputs()[0].getType()).getWidth();
      unsigned controlWidth =
          cast<IntegerType>(semantic.getInputs()[2].getType()).getWidth();
      OpBuilder builder(semantic);
      auto modeConstant =
          semantic.getInputs()[4].getDefiningOp<pyc::ConstantOp>();
      if (!modeConstant) {
        semantic.emitError(
            "wrapping_bitfield mode must be constant so selection can use the "
            "decomposed primitive path");
        signalPassFailure();
        return;
      }

      uint64_t mode = modeConstant.getValueAttr().getValue().getZExtValue();
      if (mode > 8) {
        semantic.emitError()
            << "constant wrapping_bitfield mode must be in 0..8, got " << mode;
        signalPassFailure();
        return;
      }

      Value value = semantic.getInputs()[0];
      Value source = semantic.getInputs()[1];
      Value bitWidth = semantic.getInputs()[2];
      Value bitOffset = semantic.getInputs()[3];
      Type dataType = value.getType();
      SmallVector<NamedAttribute> normalizeParameters{
          builder.getNamedAttr("CONTROL_WIDTH",
                               builder.getI64IntegerAttr(controlWidth)),
          builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width)),
      };
      SmallVector<Value> normalizeInputs{value, bitWidth, bitOffset};
      SmallVector<Type> normalizeOutputs{dataType, dataType};
      Operation *normalize = createRtlComb(
          semantic, "pyc.wrapping_field_normalize.v1", width, normalizeInputs,
          normalizeOutputs, normalizeParameters,
          {"value", "bit_width", "bit_offset"}, {"field", "mask"});
      if (!normalize) {
        signalPassFailure();
        return;
      }
      Value field = normalize->getResult(0);
      Value mask = normalize->getResult(1);
      Value replacement;

      auto selectSingleOutput = [&](llvm::StringRef semanticId,
                                    ArrayRef<Value> inputs,
                                    ArrayRef<NamedAttribute> parameters,
                                    ArrayRef<llvm::StringRef> inputPorts) {
        SmallVector<Type> outputs{dataType};
        Operation *selected =
            createRtlComb(semantic, semanticId, width, inputs, outputs,
                          parameters, inputPorts, {"result"});
        return selected ? selected->getResult(0) : Value();
      };

      switch (mode) {
      case 0:
        replacement = field;
        break;
      case 1: {
        SmallVector<NamedAttribute> parameters{
            builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width))};
        SmallVector<Value> inputs{value, mask};
        replacement = selectSingleOutput("pyc.bitfield_clear.v1", inputs,
                                         parameters, {"value", "mask"});
        break;
      }
      case 2: {
        SmallVector<NamedAttribute> parameters{
            builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width))};
        SmallVector<Value> inputs{value, mask};
        replacement = selectSingleOutput("pyc.bitfield_set.v1", inputs,
                                         parameters, {"value", "mask"});
        break;
      }
      case 3: {
        SmallVector<NamedAttribute> parameters{
            builder.getNamedAttr("CONTROL_WIDTH",
                                 builder.getI64IntegerAttr(controlWidth)),
            builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width))};
        SmallVector<Value> inputs{value, source, mask, bitWidth, bitOffset};
        replacement = selectSingleOutput(
            "pyc.bitfield_insert.v1", inputs, parameters,
            {"value", "source", "mask", "bit_width", "bit_offset"});
        break;
      }
      case 4: {
        SmallVector<NamedAttribute> parameters{
            builder.getNamedAttr("CONTROL_WIDTH",
                                 builder.getI64IntegerAttr(controlWidth)),
            builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width))};
        SmallVector<Value> inputs{field, bitWidth};
        replacement = selectSingleOutput("pyc.reverse_bytes.v1", inputs,
                                         parameters, {"field", "bit_width"});
        break;
      }
      case 5: {
        SmallVector<NamedAttribute> parameters{
            builder.getNamedAttr("CONTROL_WIDTH",
                                 builder.getI64IntegerAttr(controlWidth)),
            builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width))};
        SmallVector<Value> inputs{field, bitWidth};
        replacement = selectSingleOutput("pyc.dynamic_sign_extend.v1", inputs,
                                         parameters, {"field", "bit_width"});
        break;
      }
      case 6: {
        unsigned countWidth = 1;
        while ((uint64_t{1} << countWidth) < uint64_t{width} + 1)
          ++countWidth;
        Type countType = builder.getIntegerType(countWidth);
        SmallVector<NamedAttribute> parameters{
            builder.getNamedAttr("COUNT_WIDTH",
                                 builder.getI64IntegerAttr(countWidth)),
            builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width))};
        SmallVector<Value> inputs{field};
        SmallVector<Type> outputs{countType};
        Operation *selected =
            createRtlComb(semantic, "pyc.popcount.v1", width, inputs, outputs,
                          parameters, {"in_value"}, {"count"});
        if (selected) {
          if (countType == dataType)
            replacement = selected->getResult(0);
          else
            replacement = builder
                              .create<pyc::ZextOp>(semantic.getLoc(), dataType,
                                                   selected->getResult(0))
                              .getResult();
        }
        break;
      }
      case 7:
      case 8: {
        unsigned countWidth = 1;
        while ((uint64_t{1} << countWidth) < uint64_t{width} + 1)
          ++countWidth;
        unsigned effectiveWidthBits = std::max(controlWidth, countWidth);
        auto effectiveType = builder.getIntegerType(effectiveWidthBits);
        auto countType = builder.getIntegerType(countWidth);
        Value widenedBitWidth = bitWidth;
        if (controlWidth < effectiveWidthBits)
          widenedBitWidth = builder
                                .create<pyc::ZextOp>(semantic.getLoc(),
                                                     effectiveType, bitWidth)
                                .getResult();
        Value widthLimit = builder
                               .create<pyc::ConstantOp>(
                                   semantic.getLoc(), effectiveType,
                                   builder.getIntegerAttr(effectiveType, width))
                               .getResult();
        Value widthIsOver =
            builder
                .create<pyc::CmpOp>(semantic.getLoc(), builder.getI1Type(),
                                    widthLimit, widenedBitWidth,
                                    builder.getStringAttr("ult"))
                .getResult();
        Value effectiveWidth =
            builder
                .create<pyc::SelectOp>(semantic.getLoc(), effectiveType,
                                       widthIsOver, widthLimit, widenedBitWidth)
                .getResult();
        Value countInput = field;
        if (mode == 7) {
          Value zero = builder
                           .create<pyc::ConstantOp>(
                               semantic.getLoc(), effectiveType,
                               builder.getIntegerAttr(effectiveType, 0))
                           .getResult();
          Value isZeroWidth =
              builder
                  .create<pyc::CmpOp>(semantic.getLoc(), builder.getI1Type(),
                                      effectiveWidth, zero,
                                      builder.getStringAttr("eq"))
                  .getResult();
          Value rawShift =
              builder
                  .create<pyc::SubOp>(semantic.getLoc(), effectiveType,
                                      widthLimit, effectiveWidth)
                  .getResult();
          Value safeShift =
              builder
                  .create<pyc::SelectOp>(semantic.getLoc(), effectiveType,
                                         isZeroWidth, zero, rawShift)
                  .getResult();
          countInput = builder
                           .create<pyc::ShlOp>(semantic.getLoc(), dataType,
                                               field, safeShift)
                           .getResult();
        }
        Value directionLow =
            builder
                .create<pyc::ConstantOp>(
                    semantic.getLoc(), builder.getI1Type(),
                    builder.getIntegerAttr(builder.getI1Type(), mode == 8))
                .getResult();
        SmallVector<NamedAttribute> parameters{
            builder.getNamedAttr("COUNT_WIDTH",
                                 builder.getI64IntegerAttr(countWidth)),
            builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width))};
        SmallVector<Value> inputs{countInput, directionLow};
        SmallVector<Type> outputs{countType};
        Operation *selected = createRtlComb(
            semantic, "pyc.runtime_zero_count.v1", width, inputs, outputs,
            parameters, {"value", "direction_low"}, {"count"});
        if (!selected)
          break;
        Value rawCount = selected->getResult(0);
        Value comparableCount = rawCount;
        if (countWidth < effectiveWidthBits)
          comparableCount = builder
                                .create<pyc::ZextOp>(semantic.getLoc(),
                                                     effectiveType, rawCount)
                                .getResult();
        Value rawExceedsWidth =
            builder
                .create<pyc::CmpOp>(semantic.getLoc(), builder.getI1Type(),
                                    effectiveWidth, comparableCount,
                                    builder.getStringAttr("ult"))
                .getResult();
        Value clampedCount =
            builder
                .create<pyc::SelectOp>(semantic.getLoc(), effectiveType,
                                       rawExceedsWidth, effectiveWidth,
                                       comparableCount)
                .getResult();
        if (effectiveType == dataType)
          replacement = clampedCount;
        else
          replacement = builder
                            .create<pyc::ZextOp>(semantic.getLoc(), dataType,
                                                 clampedCount)
                            .getResult();
        break;
      }
      }
      if (!replacement) {
        signalPassFailure();
        return;
      }
      semantic.getResult().replaceAllUsesWith(replacement);
      semantic.erase();
    }

    for (CountZerosOp semantic : zeroCountOps) {
      unsigned width = cast<IntegerType>(semantic.getIn().getType()).getWidth();
      unsigned countWidth =
          cast<IntegerType>(semantic.getCount().getType()).getWidth();
      OpBuilder builder(semantic);
      SmallVector<NamedAttribute> parameterValues{
          builder.getNamedAttr("COUNT_WIDTH",
                               builder.getI64IntegerAttr(countWidth)),
          builder.getNamedAttr(
              "DIRECTION_LOW",
              builder.getI64IntegerAttr(
                  semantic.getDirection() == "trailing" ? 1 : 0)),
          builder.getNamedAttr("WIDTH", builder.getI64IntegerAttr(width)),
      };
      SmallVector<Value> inputs{semantic.getIn()};
      SmallVector<Type> outputs(semantic->getResultTypes());
      Operation *selected =
          createRtlComb(semantic, "pyc.count_zeros.v1", width, inputs, outputs,
                        parameterValues, {"in_value"}, {"count"});
      if (!selected) {
        signalPassFailure();
        return;
      }
      semantic.getCount().replaceAllUsesWith(selected->getResult(0));
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
