#include "pyc/Transforms/Passes.h"

#include "pyc/Dialect/PYC/PYCOps.h"
#include "pyc/Support/Diagnostics.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Location.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <iterator>

using namespace mlir;

namespace pyc {
namespace {

static bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

static bool isDigit(char c) { return (c >= '0' && c <= '9'); }

static bool isIdentStart(char c) { return isAlpha(c) || c == '_'; }

static bool isIdentChar(char c) { return isIdentStart(c) || isDigit(c); }

static bool isValidIdent(llvm::StringRef s) {
  if (s.empty())
    return false;
  if (!isIdentStart(s.front()))
    return false;
  for (char c : s.drop_front()) {
    if (!isIdentChar(c))
      return false;
  }
  return true;
}

static std::string sanitizeIdForBackend(llvm::StringRef s) {
  std::string out;
  out.reserve(s.size() + 1);
  for (char c : s) {
    if (isIdentChar(c))
      out.push_back(c);
    else
      out.push_back('_');
  }
  if (out.empty() || isDigit(out.front()))
    out.insert(out.begin(), '_');
  return out;
}

// Decision 0024/0025: field path segments are `ident` optionally followed by
// one or more `[<digits>]` indices, and segments are separated by dots.
static bool isValidFieldSegment(llvm::StringRef seg) {
  if (seg.empty())
    return false;
  std::size_t i = 0;
  if (!isIdentStart(seg[i]))
    return false;
  ++i;
  while (i < seg.size() && isIdentChar(seg[i]))
    ++i;
  while (i < seg.size()) {
    if (seg[i] != '[')
      return false;
    ++i;
    std::size_t digitsBegin = i;
    while (i < seg.size() && isDigit(seg[i]))
      ++i;
    if (i == digitsBegin)
      return false;
    if (i >= seg.size() || seg[i] != ']')
      return false;
    ++i;
  }
  return true;
}

static bool isValidFieldPath(llvm::StringRef path) {
  if (path.empty())
    return false;
  if (path.contains(':'))
    return false;
  llvm::SmallVector<llvm::StringRef, 8> segs;
  path.split(segs, '.', /*MaxSplit=*/-1, /*KeepEmpty=*/true);
  if (segs.empty())
    return false;
  for (llvm::StringRef seg : segs) {
    if (!isValidFieldSegment(seg))
      return false;
  }
  return true;
}

static bool jsonIntFieldNonNegative(Operation *op,
                                    const llvm::json::Object &obj,
                                    llvm::StringRef field,
                                    llvm::StringRef code,
                                    bool &ok) {
  auto it = obj.find(field);
  if (it == obj.end()) {
    pyc::emitError(op, code) << "missing JSON field `" << field << "`";
    ok = false;
    return false;
  }
  auto iv = it->second.getAsInteger();
  if (!iv || *iv < 0) {
    pyc::emitError(op, code) << "JSON field `" << field << "` must be a non-negative integer";
    ok = false;
    return false;
  }
  return true;
}

static bool isValidPythonSourcePath(llvm::StringRef path) {
  if (path.empty() || llvm::sys::path::is_absolute(path) ||
      !path.ends_with(".py") || path.contains('\\') || path.contains("//") ||
      (path.size() >= 2 && isAlpha(path[0]) && path[1] == ':'))
    return false;
  llvm::SmallVector<llvm::StringRef> segments;
  path.split(segments, '/', /*MaxSplit=*/-1, /*KeepEmpty=*/true);
  for (llvm::StringRef segment : segments) {
    if (segment.empty() || segment == "." || segment == "..")
      return false;
    for (char character : segment)
      if (!isAlpha(character) && !isDigit(character) && character != '.' &&
          character != '_' && character != '+' && character != '@' &&
          character != '-')
        return false;
  }
  return true;
}

static bool isValidSha256Fingerprint(llvm::StringRef value) {
  if (!value.consume_front("sha256:") || value.size() != 64)
    return false;
  return llvm::all_of(value, [](char character) {
    return (character >= '0' && character <= '9') ||
           (character >= 'a' && character <= 'f');
  });
}

static std::string sourceFrameKey(llvm::StringRef file, uint64_t line,
                                  uint64_t column) {
  return (file + "\n" + llvm::Twine(line) + "\n" + llvm::Twine(column))
      .str();
}

static void collectLocationFrames(
    Location location,
    llvm::function_ref<void(FileLineColLoc)> collect) {
  if (auto file = dyn_cast<FileLineColLoc>(location)) {
    collect(file);
    return;
  }
  if (auto named = dyn_cast<NameLoc>(location)) {
    collectLocationFrames(named.getChildLoc(), collect);
    return;
  }
  if (auto call = dyn_cast<CallSiteLoc>(location)) {
    collectLocationFrames(call.getCallee(), collect);
    collectLocationFrames(call.getCaller(), collect);
    return;
  }
  if (auto fused = dyn_cast<FusedLoc>(location))
    for (Location child : fused.getLocations())
      collectLocationFrames(child, collect);
}

using SourceLocationOrigin = llvm::SmallVector<std::string>;

static std::string sourceLocationOriginKey(
    llvm::ArrayRef<std::string> frames) {
  std::string key;
  for (const std::string &frame : frames) {
    key.append(frame);
    key.push_back('\0');
  }
  return key;
}

static llvm::SmallVector<SourceLocationOrigin>
sourceLocationOrigins(Location location) {
  if (auto file = dyn_cast<FileLineColLoc>(location)) {
    llvm::StringRef filename = file.getFilename().getValue();
    if (!filename.ends_with(".py"))
      return {};
    return {{sourceFrameKey(filename, file.getLine(), file.getColumn())}};
  }
  if (auto named = dyn_cast<NameLoc>(location))
    return sourceLocationOrigins(named.getChildLoc());
  if (auto call = dyn_cast<CallSiteLoc>(location)) {
    auto callees = sourceLocationOrigins(call.getCallee());
    auto callers = sourceLocationOrigins(call.getCaller());
    if (callees.empty())
      return callers;
    if (callers.empty())
      return callees;
    llvm::SmallVector<SourceLocationOrigin> combined;
    for (const SourceLocationOrigin &callee : callees)
      for (const SourceLocationOrigin &caller : callers) {
        SourceLocationOrigin origin = callee;
        origin.append(caller);
        combined.push_back(std::move(origin));
      }
    return combined;
  }
  if (auto fused = dyn_cast<FusedLoc>(location)) {
    llvm::SmallVector<SourceLocationOrigin> origins;
    for (Location child : fused.getLocations()) {
      auto nested = sourceLocationOrigins(child);
      origins.append(std::make_move_iterator(nested.begin()),
                     std::make_move_iterator(nested.end()));
    }
    return origins;
  }
  return {};
}

static void collectSourceMapOriginKeys(const llvm::json::Value &value,
                                       llvm::StringSet<> &knownOrigins,
                                       unsigned depth = 0) {
  if (depth > 256)
    return;
  if (const auto *object = value.getAsObject()) {
    if (const auto *origins = object->getArray("origins")) {
      for (const llvm::json::Value &originValue : *origins) {
        const auto *origin = originValue.getAsObject();
        const auto *frames = origin ? origin->getArray("frames") : nullptr;
        if (!frames)
          continue;
        SourceLocationOrigin frameKeys;
        for (const llvm::json::Value &frameValue : *frames) {
          const auto *frame = frameValue.getAsObject();
          auto file = frame ? frame->getString("file") : std::nullopt;
          auto line = frame ? frame->getInteger("line") : std::nullopt;
          auto column = frame ? frame->getInteger("column") : std::nullopt;
          if (file && line && column)
            frameKeys.push_back(sourceFrameKey(
                *file, static_cast<uint64_t>(*line),
                static_cast<uint64_t>(*column)));
        }
        if (!frameKeys.empty())
          knownOrigins.insert(sourceLocationOriginKey(frameKeys));
      }
      return;
    }
    for (const auto &entry : *object)
      collectSourceMapOriginKeys(entry.second, knownOrigins, depth + 1);
  } else if (const auto *array = value.getAsArray()) {
    for (const llvm::json::Value &element : *array)
      collectSourceMapOriginKeys(element, knownOrigins, depth + 1);
  }
}

static bool sourceLocationOriginIsMapped(
    llvm::ArrayRef<std::string> frames,
    const llvm::StringSet<> &knownOrigins) {
  llvm::SmallVector<bool> reachable(frames.size() + 1, false);
  reachable[0] = true;
  for (size_t begin = 0; begin < frames.size(); ++begin) {
    if (!reachable[begin])
      continue;
    SourceLocationOrigin candidate;
    for (size_t end = begin; end < frames.size(); ++end) {
      candidate.push_back(frames[end]);
      if (knownOrigins.contains(sourceLocationOriginKey(candidate)))
        reachable[end + 1] = true;
    }
  }
  return reachable.back();
}

static bool validateSourceProvenance(
    Operation *anchor, const llvm::json::Object &provenance,
    llvm::StringSet<> &knownFrames) {
  const llvm::json::Array *origins = provenance.getArray("origins");
  if (!origins || provenance.size() != 1) {
    pyc::emitError(anchor, "PYC985")
        << "source provenance must contain exactly one `origins` array";
    return false;
  }
  std::string previousOrigin;
  for (const llvm::json::Value &originValue : *origins) {
    const auto *origin = originValue.getAsObject();
    const auto *frames = origin ? origin->getArray("frames") : nullptr;
    if (!origin || origin->size() != 1 || !frames || frames->empty()) {
      pyc::emitError(anchor, "PYC985")
          << "source provenance origin must contain a non-empty `frames` array";
      return false;
    }
    std::string originKey;
    unsigned previousKindRank = 0;
    bool firstFrame = true;
    for (const llvm::json::Value &frameValue : *frames) {
      const auto *frame = frameValue.getAsObject();
      auto file = frame ? frame->getString("file") : std::nullopt;
      auto kind = frame ? frame->getString("kind") : std::nullopt;
      auto line = frame ? frame->getInteger("line") : std::nullopt;
      auto column = frame ? frame->getInteger("column") : std::nullopt;
      auto symbol = frame ? frame->getString("symbol") : std::nullopt;
      const bool kindValid =
          kind && (*kind == "statement" || *kind == "definition" ||
                   *kind == "inline_callsite" || *kind == "instance" ||
                   *kind == "specialization");
      unsigned kindRank =
          !kind || *kind == "statement" || *kind == "definition" ? 0
          : *kind == "inline_callsite"                              ? 1
          : *kind == "instance"                                     ? 2
                                                                    : 3;
      if (!frame || (frame->size() != 4 && frame->size() != 5) || !file ||
          !kindValid || !line || *line <= 0 || !column || *column <= 0 ||
          !isValidPythonSourcePath(*file) ||
          (frame->size() == 5 && (!symbol || symbol->empty())) ||
          (!firstFrame && kindRank < previousKindRank)) {
        pyc::emitError(anchor, "PYC985")
            << "source provenance frame is malformed";
        return false;
      }
      previousKindRank = kindRank;
      firstFrame = false;
      std::string key = sourceFrameKey(*file, static_cast<uint64_t>(*line),
                                       static_cast<uint64_t>(*column));
      knownFrames.insert(key);
      originKey.append(*kind).push_back('\0');
      originKey.append(key).push_back('\0');
      if (symbol)
        originKey.append(*symbol);
      originKey.push_back('\0');
    }
    if (!previousOrigin.empty() && previousOrigin >= originKey) {
      pyc::emitError(anchor, "PYC985")
          << "source provenance origins must be unique and canonical";
      return false;
    }
    previousOrigin = std::move(originKey);
  }
  return true;
}

static bool hasExactKeys(const llvm::json::Object &object,
                         std::initializer_list<llvm::StringRef> keys) {
  if (object.size() != keys.size())
    return false;
  return llvm::all_of(keys,
                      [&](llvm::StringRef key) { return object.get(key); });
}

static bool validateSourceMapExpression(
    Operation *anchor, const llvm::json::Value &value,
    llvm::StringSet<> &knownFrames, unsigned depth) {
  const auto *expression = value.getAsObject();
  const auto *nested = expression ? expression->getArray("nested") : nullptr;
  const auto *provenanceValue =
      expression ? expression->get("source_provenance") : nullptr;
  const auto *provenance =
      provenanceValue ? provenanceValue->getAsObject() : nullptr;
  auto kind = expression ? expression->getString("kind") : std::nullopt;
  auto result = expression ? expression->getString("result") : std::nullopt;
  if (depth > 128 || !expression ||
      !hasExactKeys(*expression,
                    {"kind", "nested", "result", "source_provenance"}) ||
      !kind || kind->empty() || !result || result->empty() || !nested ||
      !provenance ||
      !validateSourceProvenance(anchor, *provenance, knownFrames))
    return false;
  for (const llvm::json::Value &child : *nested)
    if (!validateSourceMapExpression(anchor, child, knownFrames, depth + 1))
      return false;
  return true;
}

static bool validateSourceMapExpressions(
    Operation *anchor, const llvm::json::Array &expressions,
    llvm::StringSet<> &knownFrames, unsigned depth) {
  for (const llvm::json::Value &expression : expressions)
    if (!validateSourceMapExpression(anchor, expression, knownFrames, depth))
      return false;
  return true;
}

static bool validateSourceMapRoot(Operation *anchor,
                                  const llvm::json::Object &root,
                                  llvm::StringSet<> &knownFrames,
                                  unsigned depth) {
  if (depth > 64 ||
      !hasExactKeys(root,
                    {"blocks", "contract_epoch", "definition", "helpers",
                     "module_instances", "module_specializations", "schema",
                     "specialization", "state_owners", "system", "table_matches",
                     "table_selections", "version"}) ||
      root.getString("schema") != "agentic-circuit-source-map" ||
      root.getString("version") != "0.1" ||
      root.getString("contract_epoch") != "0.5" ||
      !root.getString("system") || root.getString("system")->empty())
    return false;
  const llvm::json::Value *definition = root.get("definition");
  const llvm::json::Value *specialization = root.get("specialization");
  if (!definition || (!definition->getAsNull() && !definition->getAsString()) ||
      !specialization ||
      (!specialization->getAsNull() && !specialization->getAsString()))
    return false;
  const auto *blocks = root.getArray("blocks");
  const auto *helpers = root.getArray("helpers");
  const auto *instances = root.getArray("module_instances");
  const auto *specializations = root.getArray("module_specializations");
  const auto *stateOwners = root.getArray("state_owners");
  const auto *matches = root.getArray("table_matches");
  const auto *selections = root.getArray("table_selections");
  if (!blocks || !helpers || !instances || !specializations || !stateOwners ||
      !matches || !selections)
    return false;
  for (const llvm::json::Value &value : *blocks) {
    const auto *block = value.getAsObject();
    const auto *expressions = block ? block->getArray("expressions") : nullptr;
    const auto *provenanceValue =
        block ? block->get("source_provenance") : nullptr;
    const auto *provenance =
        provenanceValue ? provenanceValue->getAsObject() : nullptr;
    auto index = block ? block->getInteger("index") : std::nullopt;
    auto kind = block ? block->getString("kind") : std::nullopt;
    auto name = block ? block->getString("name") : std::nullopt;
    auto stableId = block ? block->getString("stable_id") : std::nullopt;
    if (!block ||
        !hasExactKeys(*block,
                      {"expressions", "index", "kind", "name",
                       "source_provenance", "stable_id"}) ||
        !expressions || !index || *index < 0 || !kind || kind->empty() ||
        !name || name->empty() || !stableId || !provenance ||
        !validateSourceProvenance(anchor, *provenance, knownFrames) ||
        !validateSourceMapExpressions(anchor, *expressions, knownFrames,
                                      depth + 1))
      return false;
  }
  for (const llvm::json::Value &value : *helpers) {
    const auto *helper = value.getAsObject();
    const auto *expressions = helper ? helper->getArray("expressions") : nullptr;
    const auto *provenanceValue =
        helper ? helper->get("source_provenance") : nullptr;
    const auto *provenance =
        provenanceValue ? provenanceValue->getAsObject() : nullptr;
    auto name = helper ? helper->getString("name") : std::nullopt;
    if (!helper ||
        !hasExactKeys(*helper, {"expressions", "name", "source_provenance"}) ||
        !expressions || !name || name->empty() || !provenance ||
        !validateSourceProvenance(anchor, *provenance, knownFrames) ||
        !validateSourceMapExpressions(anchor, *expressions, knownFrames,
                                      depth + 1))
      return false;
  }
  for (const llvm::json::Value &value : *instances) {
    const auto *instance = value.getAsObject();
    const auto *provenanceValue =
        instance ? instance->get("source_provenance") : nullptr;
    const auto *provenance =
        provenanceValue ? provenanceValue->getAsObject() : nullptr;
    if (!instance ||
        !hasExactKeys(*instance,
                      {"definition", "name", "scope", "source_provenance",
                       "specialization"}) ||
        !instance->getString("definition") ||
        instance->getString("definition")->empty() ||
        !instance->getString("name") || instance->getString("name")->empty() ||
        !instance->getString("scope") ||
        !instance->getString("specialization") ||
        !isValidSha256Fingerprint(*instance->getString("specialization")) ||
        !provenance ||
        !validateSourceProvenance(anchor, *provenance, knownFrames))
      return false;
  }
  for (const llvm::json::Value &value : *stateOwners) {
    const auto *owner = value.getAsObject();
    const auto *provenanceValue =
        owner ? owner->get("source_provenance") : nullptr;
    const auto *provenance =
        provenanceValue ? provenanceValue->getAsObject() : nullptr;
    auto kind = owner ? owner->getString("kind") : std::nullopt;
    auto name = owner ? owner->getString("name") : std::nullopt;
    if (!owner ||
        !hasExactKeys(*owner, {"kind", "name", "source_provenance"}) ||
        !kind || (*kind != "memory" && *kind != "slot" && *kind != "table") ||
        !name || name->empty() || !provenance ||
        !validateSourceProvenance(anchor, *provenance, knownFrames))
      return false;
  }
  auto validateTableDefinitions =
      [&](const llvm::json::Array &definitions,
          llvm::StringRef expressionField) {
        for (const llvm::json::Value &value : definitions) {
          const auto *definition = value.getAsObject();
          const auto *expressions =
              definition ? definition->getArray(expressionField) : nullptr;
          const auto *provenanceValue =
              definition ? definition->get("source_provenance") : nullptr;
          const auto *provenance =
              provenanceValue ? provenanceValue->getAsObject() : nullptr;
          if (!definition || definition->size() != 4 || !expressions ||
              !definition->getString("name") ||
              definition->getString("name")->empty() ||
              !definition->getString("table") ||
              definition->getString("table")->empty() || !provenance ||
              !validateSourceProvenance(anchor, *provenance, knownFrames) ||
              !validateSourceMapExpressions(anchor, *expressions, knownFrames,
                                            depth + 1))
            return false;
        }
        return true;
      };
  if (!validateTableDefinitions(*matches, "expressions") ||
      !validateTableDefinitions(*selections, "key_expressions"))
    return false;
  for (const llvm::json::Value &value : *specializations) {
    const auto *specialization = value.getAsObject();
    if (!specialization ||
        !validateSourceMapRoot(anchor, *specialization, knownFrames, depth + 1))
      return false;
  }
  return true;
}

static bool validatePycSourceMap(ModuleOp module, StringAttr sourceMap) {
  if (sourceMap.getValue().size() > 8 * 1024 * 1024) {
    pyc::emitError(module, "PYC983") << "`pyc.source_map` exceeds 8 MiB";
    return false;
  }
  auto parsed = llvm::json::parse(sourceMap.getValue());
  if (!parsed) {
    pyc::emitError(module, "PYC983") << "`pyc.source_map` is invalid JSON";
    return false;
  }
  std::string canonical;
  llvm::raw_string_ostream canonicalStream(canonical);
  canonicalStream << *parsed;
  canonicalStream.flush();
  if (canonical != sourceMap.getValue()) {
    pyc::emitError(module, "PYC983")
        << "`pyc.source_map` must be canonical compact JSON";
    return false;
  }
  const auto *root = parsed->getAsObject();
  llvm::StringSet<> knownFrames;
  if (!root || !validateSourceMapRoot(module, *root, knownFrames, 0)) {
    pyc::emitError(module, "PYC984")
        << "`pyc.source_map` structure is malformed";
    return false;
  }
  llvm::StringSet<> knownOrigins;
  collectSourceMapOriginKeys(*parsed, knownOrigins);
  bool locationsValid = true;
  module.walk([&](Operation *operation) {
    bool operationLocationValid = true;
    collectLocationFrames(operation->getLoc(), [&](FileLineColLoc location) {
      llvm::StringRef filename = location.getFilename().getValue();
      if (!filename.ends_with(".py"))
        return;
      const std::string key = sourceFrameKey(
          filename, location.getLine(), location.getColumn());
      if (!isValidPythonSourcePath(filename) || !knownFrames.contains(key))
        operationLocationValid = false;
    });
    for (const SourceLocationOrigin &origin :
         sourceLocationOrigins(operation->getLoc()))
      if (!sourceLocationOriginIsMapped(origin, knownOrigins))
        operationLocationValid = false;
    if (!operationLocationValid) {
      pyc::emitError(operation, "PYC986")
          << "PYC operation location stack is not present in `pyc.source_map`";
      locationsValid = false;
    }
  });
  return locationsValid;
}

class CheckFrontendContractPass : public PassWrapper<CheckFrontendContractPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CheckFrontendContractPass)

  StringRef getArgument() const override { return "pyc-check-frontend-contract"; }
  StringRef getDescription() const override {
    return "Verify required frontend contract attrs are present and match the supported contract";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    bool ok = true;

    auto emitModule = [&](llvm::StringRef code, llvm::StringRef msg, llvm::StringRef hint) {
      auto d = pyc::emitError(module, code);
      d << msg;
      if (!hint.empty())
        d << " (hint: " << hint << ")";
    };

    static constexpr const char *kRequiredContract = "pycircuit";
    auto modContract = module->getAttrOfType<StringAttr>("pyc.frontend.contract");
    if (!modContract) {
      emitModule("PYC901", "missing required module attr `pyc.frontend.contract`",
                 "regenerate .pyc with the current pyCircuit frontend and keep module attrs intact");
      ok = false;
    } else if (modContract.getValue() != kRequiredContract) {
      auto d = pyc::emitError(module, "PYC902");
      d << "frontend contract mismatch: expected `" << kRequiredContract << "`, got `"
        << modContract.getValue() << "` (hint: regenerate .pyc with matching toolchain)";
      ok = false;
    }

    if (Attribute rawSourceMap = module->getAttr("pyc.source_map")) {
      auto sourceMap = dyn_cast<StringAttr>(rawSourceMap);
      if (!sourceMap) {
        pyc::emitError(module, "PYC982")
            << "`pyc.source_map` must be a canonical JSON string";
        ok = false;
      } else if (!validatePycSourceMap(module, sourceMap)) {
        ok = false;
      }
    }

    module.walk([&](pyc::RtlCombOp selected) {
      pyc::emitError(selected, "PYC932") << "pyc.rtl.comb is backend-owned and forbidden in frontend input";
      ok = false;
    });

    module.walk([&](func::FuncOp f) {
      auto checkStrAttr = [&](StringRef name, llvm::StringRef code, llvm::StringRef hint) -> StringAttr {
        auto attr = f->getAttrOfType<StringAttr>(name);
        if (!attr) {
          auto d = pyc::emitError(f, code);
          d << "missing required func attr `" << name << "`";
          if (!hint.empty())
            d << " (hint: " << hint << ")";
          ok = false;
        }
        return attr;
      };

      auto checkArrAttr = [&](StringRef name, llvm::StringRef code, llvm::StringRef hint) -> ArrayAttr {
        auto attr = f->getAttrOfType<ArrayAttr>(name);
        if (!attr) {
          auto d = pyc::emitError(f, code);
          d << "missing required func attr `" << name << "`";
          if (!hint.empty())
            d << " (hint: " << hint << ")";
          ok = false;
        }
        return attr;
      };

      auto checkBoolAttr = [&](StringRef name, llvm::StringRef code, llvm::StringRef hint) -> BoolAttr {
        auto attr = f->getAttrOfType<BoolAttr>(name);
        if (!attr) {
          auto d = pyc::emitError(f, code);
          d << "missing required func attr `" << name << "`";
          if (!hint.empty())
            d << " (hint: " << hint << ")";
          ok = false;
        }
        return attr;
      };

      auto kind = checkStrAttr("pyc.kind", "PYC903", "frontend must stamp symbol kind metadata");
      auto inl = checkStrAttr("pyc.inline", "PYC904", "frontend must stamp inline metadata");
      (void)checkStrAttr("pyc.params", "PYC905", "frontend must stamp canonical specialization params");
      (void)checkStrAttr("pyc.base", "PYC906", "frontend must stamp canonical base symbol name");
      auto argNames = checkArrAttr("arg_names", "PYC907", "frontend must stamp canonical port names");
      auto resultNames = checkArrAttr("result_names", "PYC908", "frontend must stamp canonical port names");
      auto structMetrics =
          checkStrAttr("pyc.struct.metrics", "PYC951", "frontend must stamp structural summary metadata");
      auto structCollections =
          checkStrAttr("pyc.struct.collections", "PYC952", "frontend must stamp structural collection metadata");

      if (kind) {
        auto k = kind.getValue();
        if (k != "module" && k != "function" && k != "template") {
          pyc::emitError(f, "PYC909") << "invalid `pyc.kind` value: " << k
                        << " (hint: allowed values are module/function/template)";
          ok = false;
        }
      }

      if (inl) {
        auto v = inl.getValue();
        if (v != "true" && v != "false") {
          pyc::emitError(f, "PYC910") << "invalid `pyc.inline` value: " << v
                        << " (hint: allowed values are true|false)";
          ok = false;
        }
      }

      if (argNames && argNames.size() != f.getNumArguments()) {
        pyc::emitError(f, "PYC911") << "`arg_names` arity mismatch: attr size=" << argNames.size()
                      << " but func has " << f.getNumArguments() << " arguments";
        ok = false;
      }
      if (resultNames && resultNames.size() != f.getNumResults()) {
        pyc::emitError(f, "PYC912") << "`result_names` arity mismatch: attr size=" << resultNames.size()
                      << " but func has " << f.getNumResults() << " results";
        ok = false;
      }

      auto checkPortNames = [&](ArrayAttr arr, llvm::StringRef attrName, llvm::StringRef codeBase) {
        if (!arr)
          return;
        llvm::StringSet<> used;
        for (unsigned idx = 0, e = static_cast<unsigned>(arr.size()); idx < e; ++idx) {
          auto s = dyn_cast<StringAttr>(arr[idx]);
          if (!s) {
            pyc::emitError(f, (codeBase + "1").str()) << "`" << attrName << "` entry #" << idx << " must be a string";
            ok = false;
            continue;
          }
          llvm::StringRef v = s.getValue();
          if (!isValidFieldPath(v)) {
            pyc::emitError(f, (codeBase + "2").str()) << "invalid canonical port path in `" << attrName << "` entry #"
                          << idx << ": `" << v << "` (expected segments like foo.bar[3]; `:` is reserved)";
            ok = false;
          }
          if (!used.insert(v).second) {
            pyc::emitError(f, (codeBase + "3").str()) << "duplicate canonical port path in `" << attrName << "`: `" << v
                          << "`";
            ok = false;
          }
        }
      };

      // Decision 0009/0024/0025: port names are canonical field paths.
      checkPortNames(argNames, "arg_names", "PYC92");
      checkPortNames(resultNames, "result_names", "PYC93");

      // Decision 0009/0023: canonical port paths are unique within a module
      // instance namespace; and Decision 0145 requires Verilog trace mapping to
      // be unambiguous. Enforce that the backend-sanitized identifiers are also
      // unique to avoid order-dependent suffixing.
      if (argNames && resultNames) {
        llvm::StringSet<> inUsed;
        llvm::StringMap<llvm::StringRef> sanitizedToRaw;

        for (auto a : argNames) {
          auto s = dyn_cast<StringAttr>(a);
          if (!s)
            continue;
          llvm::StringRef v = s.getValue();
          inUsed.insert(v);
          std::string san = sanitizeIdForBackend(v);
          auto [it, inserted] = sanitizedToRaw.try_emplace(san, v);
          if (!inserted && it->second != v) {
            pyc::emitError(f, "PYC925") << "backend port id collision after sanitization: `" << san << "` from `"
                          << it->second << "` and `" << v << "` (hint: rename ports to avoid ambiguous Verilog ids)";
            ok = false;
          }
        }

        for (auto r : resultNames) {
          auto s = dyn_cast<StringAttr>(r);
          if (!s)
            continue;
          llvm::StringRef v = s.getValue();
          if (inUsed.count(v) != 0) {
            pyc::emitError(f, "PYC924") << "duplicate canonical port path across `arg_names` and `result_names`: `" << v
                          << "`";
            ok = false;
          }
          std::string san = sanitizeIdForBackend(v);
          auto [it, inserted] = sanitizedToRaw.try_emplace(san, v);
          if (!inserted && it->second != v) {
            pyc::emitError(f, "PYC925") << "backend port id collision after sanitization: `" << san << "` from `"
                          << it->second << "` and `" << v << "` (hint: rename ports to avoid ambiguous Verilog ids)";
            ok = false;
          }
        }
      }

      // Decision 0025: instance `name` must be a strict identifier (no escaping).
      // Decision 0017: optional `short_name` is preferred for path segments.
      llvm::StringSet<> segUsed;
      f.walk([&](pyc::InstanceOp inst) {
        auto nameAttr = inst->getAttrOfType<StringAttr>("name");
        if (!nameAttr) {
          pyc::emitError(inst, "PYC941") << "missing required instance name "
                           << "(hint: frontend must always stamp InstanceOp `name` for stable canonical paths)";
          ok = false;
          return;
        }
        llvm::StringRef v = nameAttr.getValue();
        if (!isValidIdent(v)) {
          pyc::emitError(inst, "PYC940") << "invalid instance name `" << v
                           << "` (expected [A-Za-z_][A-Za-z0-9_]*; no escaping supported)";
          ok = false;
        }

        llvm::StringRef seg = v;
        if (auto shortAttr = inst->getAttrOfType<StringAttr>("short_name")) {
          llvm::StringRef sv = shortAttr.getValue();
          if (!isValidIdent(sv)) {
            pyc::emitError(inst, "PYC946") << "invalid instance short_name `" << sv
                             << "` (expected [A-Za-z_][A-Za-z0-9_]*; no escaping supported)";
            ok = false;
          } else {
            seg = sv;
          }
        }

        if (!segUsed.insert(seg).second) {
          pyc::emitError(inst, "PYC947") << "duplicate instance path segment `" << seg
                           << "` within module (hint: instance name/short_name must be unique per parent for stable "
                              "canonical paths)";
          ok = false;
        }
      });

      if (!f.isDeclaration()) {
        llvm::StringSet<> portPaths;
        auto addPortPaths = [&](ArrayAttr arr) {
          if (!arr)
            return;
          for (auto a : arr) {
            auto s = dyn_cast<StringAttr>(a);
            if (!s)
              continue;
            portPaths.insert(s.getValue());
          }
        };
        addPortPaths(argNames);
        addPortPaths(resultNames);

        llvm::StringSet<> memNames;
        f.walk([&](Operation *op) {
          llvm::StringRef opKind{};
          if (isa<pyc::ByteMemOp>(op))
            opKind = "byte_mem";
          else if (isa<pyc::SyncMemOp>(op))
            opKind = "sync_mem";
          else if (isa<pyc::SyncMemDPOp>(op))
            opKind = "sync_mem_dp";
          else
            return;

          auto nameAttr = op->getAttrOfType<StringAttr>("name");
          if (!nameAttr) {
            pyc::emitError(op, "PYC942") << "missing required `name` attribute for pyc." << opKind
                            << " (hint: pass name=... in the frontend memory API for stable DFX paths)";
            ok = false;
            return;
          }
          llvm::StringRef name = nameAttr.getValue();
          if (!isValidIdent(name)) {
            pyc::emitError(op, "PYC943") << "invalid `name` for pyc." << opKind << ": `" << name
                            << "` (expected [A-Za-z_][A-Za-z0-9_]*; no escaping supported)";
            ok = false;
          }
          if (!memNames.insert(name).second) {
            pyc::emitError(op, "PYC944") << "duplicate memory name in module: `" << name
                            << "` (hint: use unique memory names within a module instance)";
            ok = false;
          }
          if (portPaths.count(name) != 0) {
            pyc::emitError(op, "PYC945") << "memory name collides with port field path: `" << name
                            << "` (hint: rename memory or port to avoid ProbeRegistry canonical_path collision)";
            ok = false;
          }
        });
      }

      auto valueParamNames = f->getAttrOfType<ArrayAttr>("pyc.value_params");
      auto valueParamTypes = f->getAttrOfType<ArrayAttr>("pyc.value_param_types");
      if (bool(valueParamNames) != bool(valueParamTypes)) {
        pyc::emitError(f, "PYC913") << "value-param metadata mismatch: both `pyc.value_params` and "
                         "`pyc.value_param_types` must be present together";
        ok = false;
      } else if (valueParamNames && valueParamTypes) {
        if (valueParamNames.size() != valueParamTypes.size()) {
          pyc::emitError(f, "PYC914") << "value-param metadata arity mismatch: `pyc.value_params` has "
                        << valueParamNames.size() << " entries but `pyc.value_param_types` has "
                        << valueParamTypes.size();
          ok = false;
        }

        llvm::StringSet<> argNameSet;
        if (argNames) {
          for (Attribute a : argNames) {
            if (auto s = dyn_cast<StringAttr>(a))
              argNameSet.insert(s.getValue());
          }
        }

        auto validValueType = [](StringRef ty) -> bool {
          if (ty == "!pyc.clock" || ty == "!pyc.reset")
            return true;
          if (!ty.starts_with("i"))
            return false;
          unsigned w = 0;
          return !ty.drop_front().getAsInteger(10, w) && w > 0;
        };

        for (unsigned idx = 0, e = static_cast<unsigned>(valueParamNames.size()); idx < e; ++idx) {
          auto nameAttr = dyn_cast<StringAttr>(valueParamNames[idx]);
          auto typeAttr = dyn_cast<StringAttr>(valueParamTypes[idx]);
          if (!nameAttr) {
            pyc::emitError(f, "PYC915") << "`pyc.value_params` entry #" << idx << " must be a string";
            ok = false;
            continue;
          }
          if (!typeAttr) {
            pyc::emitError(f, "PYC916") << "`pyc.value_param_types` entry #" << idx << " must be a string";
            ok = false;
            continue;
          }
          if (!argNameSet.contains(nameAttr.getValue())) {
            pyc::emitError(f, "PYC917") << "value-param `" << nameAttr.getValue()
                          << "` is not present in `arg_names`";
            ok = false;
          }
          if (!validValueType(typeAttr.getValue())) {
            pyc::emitError(f, "PYC918") << "invalid value-param type `" << typeAttr.getValue()
                          << "` for `" << nameAttr.getValue() << "` (expected iN/!pyc.clock/!pyc.reset)";
            ok = false;
          }
        }
      }

      if (structMetrics) {
        llvm::Expected<llvm::json::Value> parsed = llvm::json::parse(structMetrics.getValue());
        if (!parsed) {
          pyc::emitError(f, "PYC953") << "invalid JSON in `pyc.struct.metrics`";
          ok = false;
        } else {
          auto *obj = parsed->getAsObject();
          if (!obj) {
            pyc::emitError(f, "PYC954") << "`pyc.struct.metrics` must encode a JSON object";
            ok = false;
          } else {
            (void)jsonIntFieldNonNegative(f, *obj, "source_loc", "PYC955", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "ast_node_count", "PYC956", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "hardware_call_count", "PYC957", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "loop_count", "PYC958", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "module_call_count", "PYC959", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "state_call_count", "PYC960", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "estimated_inline_cost", "PYC961", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "instance_count", "PYC962", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "state_alloc_count", "PYC963", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "collection_count", "PYC964", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "collection_instance_count", "PYC965", ok);
            (void)jsonIntFieldNonNegative(f, *obj, "module_family_collection_count", "PYC966", ok);

            auto clusterIt = obj->find("repeated_body_clusters");
            if (clusterIt == obj->end() || !clusterIt->second.getAsArray()) {
              pyc::emitError(f, "PYC967") << "`pyc.struct.metrics` missing `repeated_body_clusters` array";
              ok = false;
            } else {
              auto *arr = clusterIt->second.getAsArray();
              for (std::size_t idx = 0; idx < arr->size(); ++idx) {
                auto *entry = (*arr)[idx].getAsObject();
                if (!entry) {
                  pyc::emitError(f, "PYC968") << "`pyc.struct.metrics.repeated_body_clusters[" << idx
                                << "]` must be an object";
                  ok = false;
                  continue;
                }
                auto fp = entry->getString("fingerprint");
                if (!fp || fp->empty()) {
                  pyc::emitError(f, "PYC969") << "repeated-body cluster #" << idx
                                << " must provide a non-empty `fingerprint`";
                  ok = false;
                }
                (void)jsonIntFieldNonNegative(f, *entry, "count", "PYC970", ok);
                (void)jsonIntFieldNonNegative(f, *entry, "node_count", "PYC971", ok);
                (void)jsonIntFieldNonNegative(f, *entry, "hardware_calls", "PYC972", ok);
                (void)jsonIntFieldNonNegative(f, *entry, "module_calls", "PYC973", ok);
                (void)jsonIntFieldNonNegative(f, *entry, "state_calls", "PYC974", ok);
                (void)jsonIntFieldNonNegative(f, *entry, "loop_extent_hint", "PYC975", ok);
              }
            }
          }
        }
      }

      if (structCollections) {
        llvm::Expected<llvm::json::Value> parsed = llvm::json::parse(structCollections.getValue());
        if (!parsed) {
          pyc::emitError(f, "PYC976") << "invalid JSON in `pyc.struct.collections`";
          ok = false;
        } else {
          auto *arr = parsed->getAsArray();
          if (!arr) {
            pyc::emitError(f, "PYC977") << "`pyc.struct.collections` must encode a JSON array";
            ok = false;
          } else {
            for (std::size_t idx = 0; idx < arr->size(); ++idx) {
              auto *entry = (*arr)[idx].getAsObject();
              if (!entry) {
                pyc::emitError(f, "PYC978") << "`pyc.struct.collections[" << idx << "]` must be an object";
                ok = false;
                continue;
              }
              auto kindStr = entry->getString("collection_kind");
              if (!kindStr || kindStr->empty()) {
                pyc::emitError(f, "PYC979") << "structural collection #" << idx
                              << " must provide `collection_kind`";
                ok = false;
              }
              (void)jsonIntFieldNonNegative(f, *entry, "key_count", "PYC980", ok);
              auto familyFlag = entry->getBoolean("from_module_family");
              if (!familyFlag.has_value()) {
                pyc::emitError(f, "PYC981") << "structural collection #" << idx
                              << " must provide boolean `from_module_family`";
                ok = false;
              }
            }
          }
        }
      }
    });

    if (!ok)
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<::mlir::Pass> createCheckFrontendContractPass() {
  return std::make_unique<CheckFrontendContractPass>();
}

static PassRegistration<CheckFrontendContractPass> pass;

} // namespace pyc
