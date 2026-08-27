#include "acir/CodeGen/EmitCxx.h"

#include "acir/Dialect/ACSim/ACSimOps.h"
#include "acir/Dialect/ACSim/ACSimTypes.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <utility>

using namespace mlir;
using acir::acsim::ActivationIdType;
using acir::acsim::ActivateOp;
using acir::acsim::ArrayOp;
using acir::acsim::BindOp;
using acir::acsim::BindingOp;
using acir::acsim::ContinueOp;
using acir::acsim::DispatchOp;
using acir::acsim::ElementOp;
using acir::acsim::ExportOp;
using acir::acsim::ExprType;
using acir::acsim::InlineOp;
using acir::acsim::InstanceOp;
using acir::acsim::InvokeOp;
using acir::acsim::LiveLoadOp;
using acir::acsim::LiveStoreOp;
using acir::acsim::ModelOp;
using acir::acsim::ObjectIdType;
using acir::acsim::OwnerType;
using acir::acsim::PortOp;
using acir::acsim::ProcessOp;
using acir::acsim::RefType;
using acir::acsim::ResourceOp;
using acir::acsim::ReturnOp;
using acir::acsim::SuspendOp;
using acir::acsim::TerminateOp;
using acir::acsim::TypeOp;
using acir::acsim::ValueType;
using acir::acsim::WakeType;
namespace acsim = acir::acsim;

struct QueueMember {
  std::string name;
  std::string cppType = "std::uint32_t";
  unsigned capacity = 1;
  unsigned byteCapacity = 0;
};

struct DeviceMember {
  enum class Kind { Register, RegFile } kind = Kind::Register;
  std::string name;
  std::string cppType = "std::uint32_t";
  unsigned depth = 32;
};

struct ResourceMember {
  std::string name;
  unsigned capacity = 1;
};

struct StatMember {
  std::string name;
};

namespace acir::codegen {
namespace {

llvm::cl::opt<std::string>
    clOutputDir("acsim-output-dir",
                llvm::cl::desc("Staged directory for ACSim C++ emission"),
                llvm::cl::value_desc("dir"), llvm::cl::init(""));

llvm::cl::opt<bool> clCheckCxxContract(
    "acsim-check-cxx-contract",
    llvm::cl::desc(
        "Require generated kBuildFingerprint to match build-manifest.json"),
    llvm::cl::init(false));

llvm::StringRef hexFingerprint(StringRef fingerprint) {
  return fingerprint.starts_with("sha256:") ? fingerprint.drop_front(7)
                                            : fingerprint;
}

std::string withShaPrefix(StringRef hex) {
  if (hex.starts_with("sha256:"))
    return hex.str();
  return ("sha256:" + hex).str();
}

struct QualName {
  std::vector<std::string> namespaces;
  std::string ident;
};

std::vector<std::string> splitNamespaces(StringRef name) {
  llvm::SmallVector<StringRef> parts;
  name.split(parts, "::", -1, false);
  std::vector<std::string> result;
  result.reserve(parts.size());
  for (StringRef part : parts)
    if (!part.empty())
      result.push_back(part.str());
  return result;
}

QualName splitQualified(StringRef name) {
  QualName result;
  llvm::SmallVector<StringRef> parts;
  name.split(parts, "::", -1, false);
  if (parts.empty())
    return result;
  for (size_t index = 0; index + 1 < parts.size(); ++index)
    result.namespaces.push_back(parts[index].str());
  result.ident = parts.back().str();
  return result;
}

std::string joinQualified(const QualName &name) {
  std::string result;
  for (const std::string &ns : name.namespaces) {
    result += ns;
    result += "::";
  }
  result += name.ident;
  return result;
}

void emitOpenNamespaces(llvm::raw_ostream &os,
                        llvm::ArrayRef<std::string> namespaces) {
  for (const std::string &ns : namespaces)
    os << "namespace " << ns << " {\n";
  if (!namespaces.empty())
    os << '\n';
}

void emitCloseNamespaces(llvm::raw_ostream &os,
                         llvm::ArrayRef<std::string> namespaces) {
  for (size_t index = namespaces.size(); index > 0; --index)
    os << "} // namespace " << namespaces[index - 1] << '\n';
}

Operation *lookupSymbol(ModelOp model, SymbolRefAttr symbol) {
  return SymbolTable::lookupNearestSymbolFrom(model, symbol);
}

std::string ownerNamespace(acsim::ModuleOp module) {
  return ("acsim_generated::" + module.getSymName() + "::s" +
          hexFingerprint(module.getSpecializationFingerprint()))
      .str();
}

std::string ownerTypeName(acsim::ModuleOp module) {
  return ownerNamespace(module) + "::Owner";
}

std::string processNamespace(acsim::ModuleOp module, ProcessOp process) {
  return (ownerNamespace(module) + "::" + process.getSymName() + "::p" +
          hexFingerprint(process.getSpecializationFingerprint()))
      .str();
}

std::string processTypeName(acsim::ModuleOp module, ProcessOp process) {
  return processNamespace(module, process) + "::Process";
}

std::string pcIdent(Attribute attribute) {
  auto ref = dyn_cast<FlatSymbolRefAttr>(attribute);
  return ref ? ref.getValue().str() : "pc";
}

std::string cppOfTypeOp(TypeOp type) { return type.getCppName().str(); }

std::string bindingSymbol(BindingOp binding) {
  DictionaryAttr cpp = binding.getCppRecord();
  if (!cpp)
    return {};
  if (auto symbol = cpp.getAs<StringAttr>("symbol"))
    return symbol.getValue().str();
  return {};
}

std::string bindingHeader(BindingOp binding) {
  DictionaryAttr cpp = binding.getCppRecord();
  if (!cpp)
    return {};
  if (auto header = cpp.getAs<StringAttr>("header"))
    return header.getValue().str();
  return {};
}

std::string bindingEntry(BindingOp binding, StringRef kind) {
  DictionaryAttr cpp = binding.getCppRecord();
  if (!cpp)
    return {};
  auto entries = cpp.getAs<DictionaryAttr>("entry_points");
  if (!entries)
    return {};
  if (auto value = entries.getAs<StringAttr>(kind))
    return value.getValue().str();
  return {};
}

std::string realizationType(ModelOp model, SymbolRefAttr symbol) {
  Operation *resolved = lookupSymbol(model, symbol);
  if (auto type = dyn_cast_or_null<TypeOp>(resolved))
    return cppOfTypeOp(type);
  if (auto binding = dyn_cast_or_null<BindingOp>(resolved))
    return bindingSymbol(binding);
  if (auto module = dyn_cast_or_null<acsim::ModuleOp>(resolved))
    return ownerTypeName(module);
  if (symbol)
    return symbol.getRootReference().str();
  return "void";
}

unsigned integerBitWidth(Type type) {
  if (auto integer = dyn_cast<IntegerType>(type))
    return integer.getWidth();
  if (isa<IndexType>(type))
    return 64;
  return 32;
}

std::string signedCppType(unsigned width) {
  if (width == 1)
    return "bool";
  return "std::int" + std::to_string(width) + "_t";
}

std::string cppTypeName(ModelOp model, Type type) {
  if (auto integer = dyn_cast<IntegerType>(type)) {
    unsigned width = integer.getWidth();
    if (width == 1)
      return "bool";
    return "std::uint" + std::to_string(width) + "_t";
  }
  if (isa<IndexType>(type))
    return "std::size_t";
  if (auto floating = dyn_cast<FloatType>(type)) {
    if (floating.getWidth() <= 32)
      return "float";
    return "double";
  }
  if (auto value = dyn_cast<ValueType>(type))
    return realizationType(model, value.getSymbol());
  if (auto expr = dyn_cast<ExprType>(type))
    return realizationType(model, expr.getSymbol());
  if (auto wake = dyn_cast<WakeType>(type))
    return realizationType(model, wake.getSymbol());
  if (auto ref = dyn_cast<RefType>(type))
    return realizationType(model, ref.getRealization()) + " *";
  if (auto owner = dyn_cast<OwnerType>(type))
    return realizationType(model, owner.getRealization());
  if (isa<ObjectIdType>(type))
    return "gfsim::ObjectId";
  if (isa<ActivationIdType>(type))
    return "gfsim::ObjectId";
  return "void";
}

std::string memberAccessFromPath(StringRef path, StringRef rootName) {
  StringRef rest = path;
  if (rest.starts_with(rootName) && rest.size() > rootName.size() &&
      rest[rootName.size()] == '.')
    rest = rest.drop_front(rootName.size() + 1);
  else if (rest == rootName)
    return {};
  return rest.str();
}

std::string ownerAccessFromMemberAccess(StringRef memberAccess) {
  size_t separator = memberAccess.rfind('.');
  if (separator == StringRef::npos)
    return {};
  return memberAccess.take_front(separator).str();
}

struct DispatchInfo {
  DispatchOp op;
  int64_t objectId = 0;
  int64_t activationId = 0;
  std::string path;
  std::string work;
  std::string xfer;
  std::string reset;
  std::string validate;
  std::string memberAccess;
};

class Emitter {
public:
  Emitter(ModelOp model, const EmitCxxOptions &options)
      : model(model), options(options) {}

  FailureOr<BuildManifest> run() {
    if (options.outputDir.empty()) {
      model.emitError("ACSIM-EMIT: --acsim-output-dir is required");
      return failure();
    }
    if (failed(collect()))
      return failure();

    SourceFile header;
    if (failed(emitHeader(header)))
      return failure();
    auto sourceOrError = emitSource();
    if (failed(sourceOrError))
      return failure();
    SourceFile source = std::move(*sourceOrError);
    llvm::SmallVector<SourceFile, 4> files = {header, source};
    if (options.emitMain)
      files.push_back(emitMain());

    namespace fs = std::filesystem;
    fs::path finalDir = options.outputDir;
    fs::path staging = fs::path(options.outputDir + ".staging");
    std::error_code ec;
    fs::remove_all(staging, ec);
    fs::create_directories(staging);
    BuildManifest manifest;
    manifest.contractEpoch = "0.1";
    manifest.schema = "agentic-circuit-build-manifest";
    auto frozen = model.getFingerprints().getAs<StringAttr>("frozen_acir");
    manifest.inputFingerprint =
        frozen ? hexFingerprint(frozen.getValue()).str()
               : computeFingerprint(model.getSymName().str());
    manifest.profileFingerprint = options.profile;
    manifest.toolchainFingerprint = options.toolchainTarget;
    constexpr llvm::StringLiteral kPlaceholder =
        "sha256:0000000000000000000000000000000000000000000000000000000000000000";
    for (SourceFile &file : files)
      file.fingerprint = computeFingerprint(file.content);
    manifest.sources = std::vector<SourceFile>(files.begin(), files.end());
    manifest.finalize();
    std::string fingerprint = withShaPrefix(manifest.outputFingerprint);
    for (SourceFile &file : files) {
      if (file.content.find(kPlaceholder) != std::string::npos) {
        auto pos = file.content.find(kPlaceholder);
        file.content.replace(pos, kPlaceholder.size(), fingerprint);
        file.fingerprint = computeFingerprint(file.content);
      }
    }
    manifest.sources = std::vector<SourceFile>(files.begin(), files.end());
    for (SourceFile &file : files) {
      fs::path path = staging / file.relativePath;
      fs::create_directories(path.parent_path());
      std::ofstream out(path);
      if (!out) {
        fs::remove_all(staging, ec);
        model.emitError("ACSIM-EMIT: cannot write ") << file.relativePath;
        return failure();
      }
      out << file.content;
      if (!out) {
        fs::remove_all(staging, ec);
        model.emitError("ACSIM-EMIT: write failed for ") << file.relativePath;
        return failure();
      }
    }
    options.outputDir = staging.string();
    if (failed(writeManifest(manifest))) {
      fs::remove_all(staging, ec);
      return failure();
    }
    fs::path backup = fs::path(finalDir.string() + ".prev");
    if (fs::exists(finalDir)) {
      fs::remove_all(backup, ec);
      fs::rename(finalDir, backup, ec);
    }
    fs::rename(staging, finalDir, ec);
    if (ec) {
      if (fs::exists(backup)) {
        std::error_code restore;
        fs::rename(backup, finalDir, restore);
      }
      model.emitError("ACSIM-EMIT: cannot publish staging directory");
      return failure();
    }
    fs::remove_all(backup, ec);
    options.outputDir = finalDir.string();
    return manifest;
  }

private:
  LogicalResult collect() {
    rootName = model.getRoot().str();
    for (Operation &op : model.getBody().front()) {
      if (auto type = dyn_cast<TypeOp>(op)) {
        types.push_back(type);
        if (auto header = type->getAttrOfType<StringAttr>("header"))
          includes.insert(header.getValue().str());
      } else if (auto binding = dyn_cast<BindingOp>(op)) {
        bindings.push_back(binding);
        std::string header = bindingHeader(binding);
        if (!header.empty())
          includes.insert(header);
      } else if (auto module = dyn_cast<acsim::ModuleOp>(op)) {
        modules.push_back(module);
      } else if (auto dispatch = dyn_cast<DispatchOp>(op)) {
        DispatchInfo info;
        info.op = dispatch;
        info.objectId = dispatch.getObjectId();
        info.activationId = dispatch.getActivationId();
        info.path = dispatch.getPath().str();
        info.work = dispatch.getWork().str();
        info.xfer = dispatch.getXfer().str();
        info.reset = dispatch.getReset().str();
        info.validate = dispatch.getValidate().str();
        info.memberAccess = memberAccessFromPath(info.path, rootName);
        dispatches.push_back(info);
        objectCount = std::max(objectCount, info.objectId + 1);
        activationCount = std::max(activationCount, info.activationId + 1);
      } else if (auto activate = dyn_cast<ActivateOp>(op)) {
        auto sourceOp = activate.getSource().getDefiningOp<DispatchOp>();
        auto targetOp = activate.getTarget().getDefiningOp<DispatchOp>();
        if (!sourceOp || !targetOp)
          return activate.emitOpError(
              "ACSIM-EMIT: activation endpoints must be dispatch results");
        edges.emplace_back(sourceOp.getActivationId(), targetOp.getObjectId());
      }
    }
    for (TypeOp type : types) {
      StringRef cpp = type.getCppName();
      StringRef symbol = type.getSymName();
      auto noteTyped = [&](StringRef cppName, StringRef prefix, auto &sink,
                           auto make) {
        if (cpp != cppName || !symbol.starts_with(prefix))
          return;
        sink.push_back(make(symbol.drop_front(prefix.size())));
      };
      for (acsim::ModuleOp module : modules) {
        std::string moduleName = module.getSymName().str();
        noteTyped("acir.register.load",
                  "acir_register_load_" + moduleName + "_",
                  devicesByModule[moduleName], [&](StringRef name) {
                    DeviceMember member;
                    member.kind = DeviceMember::Kind::Register;
                    member.name = name.str();
                    return member;
                  });
        noteTyped("acir.regfile.read",
                  "acir_regfile_read_" + moduleName + "_",
                  devicesByModule[moduleName], [&](StringRef name) {
                    DeviceMember member;
                    member.kind = DeviceMember::Kind::RegFile;
                    member.name = name.str();
                    return member;
                  });
        noteTyped("gfsim::Resource",
                  "acir_resource_" + moduleName + "_",
                  resourcesByModule[moduleName], [&](StringRef rest) {
                    ResourceMember member;
                    StringRef name = rest;
                    if (auto cap = rest.rfind("_cap"); cap != StringRef::npos) {
                      unsigned parsed = 1;
                      if (!rest.drop_front(cap + 4).getAsInteger(10, parsed) &&
                          parsed > 0)
                        member.capacity = parsed;
                      name = rest.take_front(cap);
                    }
                    member.name = name.str();
                    return member;
                  });
        noteTyped("acir.stat.add",
                  "acir_stat_add_" + moduleName + "_",
                  statsByModule[moduleName], [&](StringRef name) {
                    StatMember member;
                    member.name = name.str();
                    return member;
                  });
        if (cpp != "gfsim::SimQueue")
          continue;
        std::string prefix = "acir_queue_" + moduleName + "_";
        if (!symbol.starts_with(prefix))
          continue;
        QueueMember member;
        StringRef rest = symbol.drop_front(prefix.size());
        if (auto cap = rest.rfind("_cap"); cap != StringRef::npos) {
          StringRef head = rest.take_front(cap);
          unsigned parsed = 1;
          if (!rest.drop_front(cap + 4).getAsInteger(10, parsed) && parsed > 0)
            member.capacity = parsed;
          if (auto bytesTag = head.rfind("_bytes");
              bytesTag != StringRef::npos) {
            unsigned bytes = 0;
            if (!head.drop_front(bytesTag + 6).getAsInteger(10, bytes))
              member.byteCapacity = bytes;
            head = head.take_front(bytesTag);
          }
          if (auto widthTag = head.rfind("_i"); widthTag != StringRef::npos) {
            unsigned width = 32;
            if (!head.drop_front(widthTag + 2).getAsInteger(10, width) &&
                (width == 8 || width == 16 || width == 32 || width == 64)) {
              member.cppType = "std::uint" + std::to_string(width) + "_t";
              member.name = head.take_front(widthTag).str();
            } else {
              member.name = head.str();
            }
          } else {
            member.name = head.str();
          }
        } else {
          member.name = rest.str();
        }
        queuesByModule[moduleName].push_back(member);
      }
    }
    auto uniqueByName = [](auto &members) {
      llvm::sort(members, [](const auto &left, const auto &right) {
        return left.name < right.name;
      });
      members.erase(std::unique(members.begin(), members.end(),
                                [](const auto &left, const auto &right) {
                                  return left.name == right.name;
                                }),
                    members.end());
    };
    for (auto &[_, members] : queuesByModule)
      uniqueByName(members);
    for (auto &[_, members] : devicesByModule)
      uniqueByName(members);
    for (auto &[_, members] : resourcesByModule)
      uniqueByName(members);
    for (auto &[_, members] : statsByModule)
      uniqueByName(members);
    // Device identities are shared by load/store realizations and historically
    // did not encode payload width.  Recover the exact generated C++ type from
    // invoke operands/results so i64 registers do not silently truncate.
    for (acsim::ModuleOp owner : modules) {
      owner.walk([&](InvokeOp invoke) {
        auto type =
            dyn_cast_or_null<TypeOp>(lookupSymbol(model, invoke.getCalleeAttr()));
        if (!type)
          return;
        StringRef stem;
        Type payload;
        if (type.getCppName() == "acir.register.load") {
          stem = "acir_register_load";
          if (!invoke.getResults().empty())
            payload = invoke.getResults().front().getType();
        } else if (type.getCppName() == "acir.register.store") {
          stem = "acir_register_store";
          if (!invoke.getArgs().empty())
            payload = invoke.getArgs().front().getType();
        } else if (type.getCppName() == "acir.regfile.read") {
          stem = "acir_regfile_read";
          if (!invoke.getResults().empty())
            payload = invoke.getResults().front().getType();
        } else if (type.getCppName() == "acir.regfile.write") {
          stem = "acir_regfile_write";
          if (invoke.getArgs().size() > 1)
            payload = invoke.getArgs()[1].getType();
        }
        if (!payload)
          return;
        StringRef symbol = invoke.getCalleeAttr().getValue();
        acsim::ModuleOp declaring;
        StringRef field;
        size_t best = 0;
        for (acsim::ModuleOp candidate : modules) {
          std::string prefix =
              (stem + "_" + candidate.getSymName() + "_").str();
          if (symbol.starts_with(prefix) && prefix.size() > best) {
            declaring = candidate;
            field = symbol.drop_front(prefix.size());
            best = prefix.size();
          }
        }
        if (!declaring)
          return;
        for (DeviceMember &member :
             devicesByModule[declaring.getSymName().str()])
          if (member.name == field)
            member.cppType = cppTypeName(model, payload);
      });
    }
    llvm::sort(edges);
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
    return success();
  }

  LogicalResult emitHeader(SourceFile &file) {
    std::string storage;
    llvm::raw_string_ostream os(storage);
    os << "#pragma once\n\n";
    os << "#include <array>\n";
    os << "#include <cstddef>\n";
    os << "#include <cstdint>\n";
    os << "#include <string>\n";
    os << "#include <tuple>\n";
    os << "#include <utility>\n\n";
    os << "#include \"gfsim/core.h\"\n";
    os << "#include \"gfsim/dispatch.h\"\n";
    os << "#include \"gfsim/object.h\"\n";
    if (!queuesByModule.empty())
      os << "#include \"gfsim/queue.h\"\n";
    if (!devicesByModule.empty())
      os << "#include \"gfsim/register.h\"\n";
    if (!resourcesByModule.empty())
      os << "#include \"gfsim/resource.h\"\n";
    for (const std::string &header : includes)
      os << "#include \"" << header << "\"\n";
    os << '\n';

    emitGeneratedTypes(os);
    llvm::SmallVector<acsim::ModuleOp> declarationOrder;
    llvm::DenseSet<Operation *> declared;
    std::function<void(acsim::ModuleOp)> appendModule =
        [&](acsim::ModuleOp module) {
          if (!declared.insert(module.getOperation()).second)
            return;
          for (Operation &op : module.getBody().front()) {
            SymbolRefAttr target;
            if (auto instance = dyn_cast<InstanceOp>(op))
              target = instance.getTarget();
            else if (auto array = dyn_cast<ArrayOp>(op))
              target = array.getTarget();
            if (target)
              if (auto child = dyn_cast_or_null<acsim::ModuleOp>(
                      lookupSymbol(model, target)))
              appendModule(child);
          }
          declarationOrder.push_back(module);
        };
    for (acsim::ModuleOp module : modules)
      appendModule(module);
    for (acsim::ModuleOp module : declarationOrder)
      if (failed(emitModuleClass(os, module)))
        return failure();

    os << "namespace acsim_generated {\n\n";
    os << "constexpr std::uint32_t kObjectCount = " << objectCount << ";\n";
    os << "constexpr std::uint32_t kActivationCount = " << activationCount
       << ";\n\n";
    os << "struct GeneratedModel {\n";
    os << "  gfsim::SimSystem system;\n";
    if (!modules.empty()) {
      acsim::ModuleOp root = nullptr;
      for (acsim::ModuleOp module : modules)
        if (module.getSymName() == rootName)
          root = module;
      if (root)
        os << "  " << ownerTypeName(root) << " root;\n";
    }
    os << "  std::array<gfsim::LegacyDispatchThunk, "
       << (objectCount == 0 ? 1 : objectCount) << "> dispatch{};\n";
    os << "  std::array<std::uint32_t, "
       << (activationCount == 0 ? 1 : activationCount + 1)
       << "> activationOffsets{};\n";
    os << "  std::array<std::uint32_t, " << std::max<int64_t>(edges.size(), 1)
       << "> activationTargets{};\n";
    os << "  explicit GeneratedModel(std::string name = \""
       << model.getSymName() << "\");\n";
    os << "  gfsim::TerminationResult run();\n";
    os << "};\n\n";
    os << "gfsim::TerminationResult simulate();\n\n";
    os << "constexpr char kBuildFingerprint[] = "
          "\"sha256:0000000000000000000000000000000000000000000000000000000000000000\";\n\n";
    os << "} // namespace acsim_generated\n";

    file.relativePath = "include/generated/model.h";
    file.content = os.str();
    return success();
  }

  void emitGeneratedTypes(llvm::raw_ostream &os) {
    for (TypeOp type : types) {
      if (type.getKind() != "wake" ||
          !type.getCppName().starts_with("acir::generated::"))
        continue;
      QualName name = splitQualified(type.getCppName());
      emitOpenNamespaces(os, name.namespaces);
      os << "struct " << name.ident << " {\n";
      os << "  gfsim::Epoch ready{};\n";
      os << "};\n";
      emitCloseNamespaces(os, name.namespaces);
      os << '\n';
    }
  }

  LogicalResult emitModuleClass(llvm::raw_ostream &os, acsim::ModuleOp module) {
    auto ns = splitNamespaces(ownerNamespace(module));
    for (Operation &op : module.getBody().front()) {
      if (auto process = dyn_cast<ProcessOp>(op))
        emitProcessClass(os, module, process);
    }

    emitOpenNamespaces(os, ns);
    os << "struct Owner {\n";
    os << "  void *parent_ = nullptr;\n";
    for (const QueueMember &queue :
         queuesByModule[module.getSymName().str()]) {
      os << "  gfsim::SimQueue<" << queue.cppType << "> " << queue.name
         << ";\n";
    }
    for (const DeviceMember &device :
         devicesByModule[module.getSymName().str()]) {
      if (device.kind == DeviceMember::Kind::RegFile)
        os << "  gfsim::RegFile<" << device.cppType << ", " << device.depth
           << "> " << device.name << ";\n";
      else
        os << "  gfsim::Register<" << device.cppType << "> " << device.name
           << ";\n";
    }
    for (const ResourceMember &resource :
         resourcesByModule[module.getSymName().str()]) {
      os << "  gfsim::Resource " << resource.name << ";\n";
    }
    for (const StatMember &stat : statsByModule[module.getSymName().str()]) {
      os << "  std::uint64_t " << stat.name << "_ = 0;\n";
    }
    for (Operation &op : module.getBody().front()) {
      if (auto instance = dyn_cast<InstanceOp>(op)) {
        os << "  " << realizationType(model, instance.getTarget()) << ' '
           << instance.getSymName() << ";\n";
      } else if (auto array = dyn_cast<ArrayOp>(op)) {
        int64_t volume = 1;
        for (int64_t dim : array.getShape())
          volume *= dim;
        os << "  std::array<" << realizationType(model, array.getTarget())
           << ", " << volume << "> " << array.getSymName() << ";\n";
      } else if (auto process = dyn_cast<ProcessOp>(op)) {
        os << "  " << processTypeName(module, process) << ' '
           << process.getSymName() << ";\n";
      } else if (isa<ReturnOp>(op)) {
        continue;
      } else {
        return op.emitOpError("ACSIM-EMIT: unsupported module operation");
      }
    }
    os << "  gfsim::Epoch queuesCommittedEpoch_{~0ull, 0};\n";
    os << "  Owner() = default;\n";
    os << "  Owner(gfsim::SimSystem &system, std::string path);\n";
    os << "  void commitQueues(gfsim::Epoch epoch);\n";
    os << "};\n";
    emitCloseNamespaces(os, ns);
    os << '\n';
    return success();
  }

  void emitProcessClass(llvm::raw_ostream &os, acsim::ModuleOp module,
                        ProcessOp process) {
    auto ns = splitNamespaces(processNamespace(module, process));
    emitOpenNamespaces(os, ns);
    os << "struct Process {\n";
    llvm::SmallVector<std::string> pcs;
    for (Attribute attribute : process.getPcs())
      pcs.push_back(pcIdent(attribute));
    os << "  enum class Pc : std::uint8_t {";
    for (size_t index = 0; index < pcs.size(); ++index) {
      if (index)
        os << ", ";
      os << pcs[index];
    }
    os << "};\n";
    os << "  gfsim::SimSystem *system = nullptr;\n";
    os << "  gfsim::ObjectId id = 0;\n";
    os << "  void *owner_ = nullptr;\n";
    os << "  Pc pc_ = Pc::" << (pcs.empty() ? "entry" : pcs.front()) << ";\n";
    os << "  Pc proposedPc_ = pc_;\n";
    os << "  gfsim::Epoch proposedWake_{};\n";
    os << "  bool suspended_ = false;\n";
    os << "  bool terminated_ = false;\n";
    os << "  static constexpr std::uint64_t fairness_cap_ = "
       << process.getFairnessCap() << ";\n";
    for (Attribute attribute : process.getLiveSlots()) {
      auto dict = dyn_cast<DictionaryAttr>(attribute);
      if (!dict)
        continue;
      auto name = dict.getAs<StringAttr>("name");
      auto typeAttr = dict.getAs<TypeAttr>("type");
      if (!name || !typeAttr)
        continue;
      std::string cpp = cppTypeName(model, typeAttr.getValue());
      os << "  " << cpp << " " << name.getValue() << "_{};\n";
      os << "  " << cpp << " proposed_" << name.getValue() << "_{};\n";
    }
    for (auto [index, capture] : llvm::enumerate(process.getCaptures())) {
      if (index >= process.getCaptureNames().size())
        break;
      auto name = dyn_cast<StringAttr>(process.getCaptureNames()[index]);
      if (!name)
        continue;
      os << "  " << cppTypeName(model, capture.getType()) << ' '
         << name.getValue() << "_{};\n";
    }
    os << "  Process() = default;\n";
    os << "  void bind(gfsim::SimSystem &sys, gfsim::ObjectId objectId, "
          "void *moduleOwner = nullptr);\n";
    os << "  void work(gfsim::Epoch epoch);\n";
    os << "  void xfer(gfsim::Epoch epoch);\n";
    os << "  void reset();\n";
    os << "  bool validate() const;\n";
    os << "  static void thunkWork(void *object, gfsim::Epoch epoch);\n";
    os << "  static void thunkXfer(void *object, gfsim::Epoch epoch);\n";
    os << "  static void thunkReset(void *object);\n";
    os << "  static bool thunkValidate(void *object);\n";
    os << "};\n";
    emitCloseNamespaces(os, ns);
    os << '\n';
  }

  FailureOr<SourceFile> emitSource() {
    std::string storage;
    llvm::raw_string_ostream os(storage);
    os << "#include \"generated/model.h\"\n\n";
    emitGeneratedImpls(os);
    for (acsim::ModuleOp module : modules) {
      emitOwnerCtor(os, module);
      for (Operation &op : module.getBody().front())
        if (auto process = dyn_cast<ProcessOp>(op))
          if (failed(emitProcessMethods(os, module, process)))
            return failure();
    }
    emitModelGlue(os);

    SourceFile file;
    file.relativePath = "src/generated/model.cpp";
    file.content = os.str();
    return file;
  }

  void emitGeneratedImpls(llvm::raw_ostream &os) {
    for (TypeOp type : types) {
      if (type.getKind() != "implementation" ||
          !type.getCppName().starts_with("acir::generated::"))
        continue;
      QualName qual = splitQualified(type.getCppName());
      std::string resultType = "void";
      for (TypeOp other : types) {
        if (other.getKind() == "wake" &&
            other.getCppName().starts_with("acir::generated::")) {
          resultType = other.getCppName().str();
          break;
        }
      }
      emitOpenNamespaces(os, qual.namespaces);
      os << resultType << ' ' << qual.ident << "(gfsim::Epoch epoch) {\n";
      if (llvm::StringRef(qual.ident).contains("next_tick"))
        os << "  return " << resultType << "{{epoch.time + 1, 0}};\n";
      else
        os << "  return " << resultType << "{epoch.nextDelta()};\n";
      os << "}\n";
      emitCloseNamespaces(os, qual.namespaces);
      os << '\n';
    }
  }

  void emitOwnerCtor(llvm::raw_ostream &os, acsim::ModuleOp module) {
    os << ownerTypeName(module)
       << "::Owner(gfsim::SimSystem &system, std::string path)";
    llvm::SmallVector<std::string> inits;
    for (const QueueMember &queue :
         queuesByModule[module.getSymName().str()]) {
      inits.push_back(queue.name + "(\"" + queue.name +
                      "\", 0, nullptr, " + std::to_string(queue.capacity) +
                      (queue.byteCapacity > 0
                           ? (", " + std::to_string(queue.byteCapacity))
                           : std::string()) +
                      ")");
    }
    for (const ResourceMember &resource :
         resourcesByModule[module.getSymName().str()]) {
      inits.push_back(resource.name + "(\"" + resource.name + "\", 0, nullptr, " +
                      std::to_string(resource.capacity) + ")");
    }
    for (Operation &op : module.getBody().front()) {
      if (auto instance = dyn_cast<InstanceOp>(op)) {
        Operation *target = lookupSymbol(model, instance.getTarget());
        std::string expr = instance.getSymName().str();
        expr += "(system, path + \".";
        expr += instance.getSymName();
        expr += "\")";
        if (isa_and_nonnull<acsim::ModuleOp>(target))
          inits.push_back(expr);
        else
          inits.push_back(instance.getSymName().str() + "{}");
      } else if (auto array = dyn_cast<ArrayOp>(op)) {
        inits.push_back(array.getSymName().str() + "{}");
      } else if (isa<ProcessOp>(op)) {
        inits.push_back(cast<ProcessOp>(op).getSymName().str() + "{}");
      }
    }
    if (!inits.empty()) {
      os << '\n';
      for (size_t index = 0; index < inits.size(); ++index) {
        os << (index == 0 ? "    : " : "      ");
        os << inits[index];
        os << (index + 1 == inits.size() ? "\n" : ",\n");
      }
    }
    os << "{\n";
    os << "  (void)system;\n";
    os << "  (void)path;\n";
    for (Operation &op : module.getBody().front()) {
      if (auto instance = dyn_cast<InstanceOp>(op)) {
        if (isa_and_nonnull<acsim::ModuleOp>(
                lookupSymbol(model, instance.getTarget())))
          os << "  " << instance.getSymName() << ".parent_ = this;\n";
      } else if (auto array = dyn_cast<ArrayOp>(op)) {
        Operation *target = lookupSymbol(model, array.getTarget());
        if (!isa_and_nonnull<acsim::ModuleOp>(target))
          continue;
        int64_t volume = 1;
        for (int64_t dim : array.getShape())
          volume *= dim;
        os << "  for (std::size_t index = 0; index < " << volume
           << "; ++index)\n";
        os << "    " << array.getSymName()
           << "[index] = " << realizationType(model, array.getTarget())
           << "(system, path + \"." << array.getSymName()
           << "[\" + std::to_string(index) + \"]\");\n";
        os << "  for (std::size_t index = 0; index < " << volume
           << "; ++index)\n";
        os << "    " << array.getSymName() << "[index].parent_ = this;\n";
      }
    }
    os << "}\n\n";

    os << "void " << ownerTypeName(module)
       << "::commitQueues(gfsim::Epoch epoch) {\n";
    os << "  if (queuesCommittedEpoch_.time == epoch.time && "
          "queuesCommittedEpoch_.delta == epoch.delta)\n";
    os << "    return;\n";
    os << "  queuesCommittedEpoch_ = epoch;\n";
    for (const QueueMember &queue :
         queuesByModule[module.getSymName().str()])
      os << "  " << queue.name << ".doXfer(epoch);\n";
    for (const ResourceMember &resource :
         resourcesByModule[module.getSymName().str()]) {
      os << "  " << resource.name << ".doArbitrate(epoch);\n";
      os << "  " << resource.name << ".doXfer(epoch);\n";
    }
    os << "}\n\n";
  }

  LogicalResult emitProcessMethods(llvm::raw_ostream &os, acsim::ModuleOp module,
                                   ProcessOp process) {
    std::string type = processTypeName(module, process);
    os << "void " << type
       << "::bind(gfsim::SimSystem &sys, gfsim::ObjectId objectId, "
          "void *moduleOwner) {\n";
    os << "  system = &sys;\n";
    os << "  id = objectId;\n";
    os << "  owner_ = moduleOwner;\n";
    os << "}\n\n";

    os << "void " << type << "::thunkWork(void *object, gfsim::Epoch epoch) {\n";
    os << "  static_cast<Process *>(object)->work(epoch);\n";
    os << "}\n\n";
    os << "void " << type << "::thunkXfer(void *object, gfsim::Epoch epoch) {\n";
    os << "  static_cast<Process *>(object)->xfer(epoch);\n";
    os << "}\n\n";
    os << "void " << type << "::thunkReset(void *object) {\n";
    os << "  static_cast<Process *>(object)->reset();\n";
    os << "}\n\n";
    os << "bool " << type << "::thunkValidate(void *object) {\n";
    os << "  return static_cast<Process *>(object)->validate();\n";
    os << "}\n\n";

    os << "void " << type << "::work(gfsim::Epoch epoch) {\n";
    os << "  suspended_ = false;\n";
    os << "  std::uint64_t steps = 0;\n";
    os << "  Pc running = pc_;\n";
    os << "  while (steps < fairness_cap_ && !suspended_ && !terminated_) {\n";
    os << "    ++steps;\n";
    os << "    switch (running) {\n";
    for (auto [pcAttr, state] :
         llvm::zip(process.getPcs(), process.getStates())) {
      os << "    case Pc::" << pcIdent(pcAttr) << ": {\n";
      if (failed(emitState(os, module, process, state, pcIdent(pcAttr))))
        return failure();
      os << "      break;\n";
      os << "    }\n";
    }
    os << "    default:\n";
    os << "      terminated_ = true;\n";
    os << "      if (system)\n";
    os << "        system->requestTerminate(gfsim::TerminationClass::Failed, "
          "\"invalid_pc\");\n";
    os << "      return;\n";
    os << "    }\n";
    os << "  }\n";
    os << "}\n\n";

    os << "void " << type << "::xfer(gfsim::Epoch epoch) {\n";
    os << "  pc_ = proposedPc_;\n";
    for (Attribute attribute : process.getLiveSlots()) {
      auto dict = dyn_cast<DictionaryAttr>(attribute);
      if (!dict)
        continue;
      auto name = dict.getAs<StringAttr>("name");
      if (!name)
        continue;
      os << "  " << name.getValue() << "_ = proposed_" << name.getValue()
         << "_;\n";
    }
    os << "  if (auto *owner = static_cast<" << ownerTypeName(module)
       << " *>(owner_))\n";
    os << "    owner->commitQueues(epoch);\n";
    llvm::SmallVector<acsim::ModuleOp> parentModules;
    for (acsim::ModuleOp candidate : modules) {
      for (Operation &op : candidate.getBody().front()) {
        SymbolRefAttr target;
        if (auto instance = dyn_cast<InstanceOp>(op))
          target = instance.getTarget();
        else if (auto array = dyn_cast<ArrayOp>(op))
          target = array.getTarget();
        if (target && lookupSymbol(model, target) == module.getOperation()) {
          if (!llvm::is_contained(parentModules, candidate))
            parentModules.push_back(candidate);
        }
      }
    }
    if (parentModules.size() == 1) {
      os << "  if (auto *owner = static_cast<" << ownerTypeName(module)
         << " *>(owner_))\n";
      os << "    if (auto *parent = static_cast<"
         << ownerTypeName(parentModules.front()) << " *>(owner->parent_))\n";
      os << "      parent->commitQueues(epoch);\n";
    }
    os << "  if (suspended_ && system)\n";
    os << "    system->scheduleWork(id, proposedWake_);\n";
    os << "  suspended_ = false;\n";
    os << "}\n\n";

    os << "void " << type << "::reset() {\n";
    llvm::SmallVector<std::string> pcs;
    for (Attribute attribute : process.getPcs())
      pcs.push_back(pcIdent(attribute));
    os << "  pc_ = Pc::" << (pcs.empty() ? "entry" : pcs.front()) << ";\n";
    os << "  proposedPc_ = pc_;\n";
    os << "  suspended_ = false;\n";
    os << "  terminated_ = false;\n";
    os << "}\n\n";

    os << "bool " << type << "::validate() const {\n";
    os << "  switch (pc_) {\n";
    for (Attribute attribute : process.getPcs())
      os << "  case Pc::" << pcIdent(attribute) << ":\n";
    os << "    return true;\n";
    os << "  }\n";
    os << "  return false;\n";
    os << "}\n\n";
    return success();
  }

  LogicalResult emitState(llvm::raw_ostream &os, acsim::ModuleOp module,
                          ProcessOp process, Region &state, StringRef pcName) {
    if (state.empty()) {
      os << "      terminated_ = true;\n";
      os << "      return;\n";
      return success();
    }
    llvm::DenseMap<Value, std::string> names;
    unsigned nextId = 0;
    auto bind = [&](Value value) -> std::string {
      auto found = names.find(value);
      if (found != names.end())
        return found->second;
      std::string name = "v" + std::to_string(nextId++);
      names[value] = name;
      return name;
    };

    Block &entry = state.front();
    if (!process.getCaptureNames().empty() &&
        entry.getNumArguments() == process.getCaptures().size()) {
      for (auto [arg, nameAttr] :
           llvm::zip_equal(entry.getArguments(), process.getCaptureNames())) {
        auto name = dyn_cast<StringAttr>(nameAttr);
        if (name)
          names[arg] = name.getValue().str() + "_";
      }
    }

    llvm::DenseMap<Block *, unsigned> blockIds;
    unsigned ordinal = 0;
    for (Block &block : state)
      blockIds[&block] = ordinal++;
    const bool cfg = blockIds.size() > 1;
    assignResults = cfg;
    std::string pc = pcName.str();
    auto label = [&](Block *block) {
      return pc + "_blk" + std::to_string(blockIds[block]);
    };

    if (cfg) {
      for (Block &block : state)
        for (BlockArgument argument : block.getArguments())
          if (!names.contains(argument))
            (void)bind(argument);
      for (Block &block : state)
        for (Operation &op : block)
          for (Value result : op.getResults())
            (void)bind(result);
      for (Block &block : state)
        for (BlockArgument argument : block.getArguments())
          if (argument.getOwner() != &entry ||
              process.getCaptureNames().empty())
            os << "      " << cppTypeName(model, argument.getType()) << ' '
               << bind(argument) << "{};\n";
      for (Block &block : state)
        for (Operation &op : block)
          for (Value result : op.getResults())
            os << "      " << cppTypeName(model, result.getType()) << ' '
               << bind(result) << "{};\n";
      os << "      goto " << label(&entry) << ";\n";
    }

    for (Block &block : state) {
      if (cfg) {
        os << "    " << label(&block) << ":\n";
        os << "    {\n";
      }
      for (Operation &op : block) {
        auto emitAssign = [&](Value result, const Twine &expr) {
          os << "      ";
          if (!cfg)
            os << "auto ";
          os << bind(result) << " = " << expr << ";\n";
        };
        auto emitBin = [&](Value result, Value lhs, Value rhs, const char *op) {
          emitAssign(result, Twine(bind(lhs)) + " " + op + " " + bind(rhs));
        };
        if (auto constant = dyn_cast<arith::ConstantOp>(op)) {
          std::string name = bind(constant.getResult());
          Attribute value = constant.getValue();
          if (cfg)
            os << "      " << name << " = ";
          else {
            os << "      " << cppTypeName(model, constant.getType()) << ' '
               << name << " = ";
          }
          if (auto integer = dyn_cast<IntegerAttr>(value)) {
            const llvm::APInt &bits = integer.getValue();
            Type ty = constant.getType();
            if (isa<IndexType>(ty)) {
              os << "static_cast<std::size_t>(" << bits.getZExtValue() << "ull)";
            } else if (auto intTy = dyn_cast<IntegerType>(ty);
                       intTy && intTy.getWidth() == 1) {
              os << (bits.getBoolValue() ? "true" : "false");
            } else if (bits.getBitWidth() > 32) {
              os << "UINT64_C(" << bits.getZExtValue() << ")";
            } else {
              os << bits.getZExtValue();
            }
          } else if (auto floating = dyn_cast<FloatAttr>(value)) {
            os << floating.getValueAsDouble();
          } else {
            os << "0";
          }
          os << ";\n";
          continue;
        }
        if (auto add = dyn_cast<arith::AddIOp>(op)) {
          emitBin(add.getResult(), add.getLhs(), add.getRhs(), "+");
          continue;
        }
        if (auto mul = dyn_cast<arith::MulIOp>(op)) {
          emitBin(mul.getResult(), mul.getLhs(), mul.getRhs(), "*");
          continue;
        }
        if (auto div = dyn_cast<arith::DivUIOp>(op)) {
          emitBin(div.getResult(), div.getLhs(), div.getRhs(), "/");
          continue;
        }
        if (auto sub = dyn_cast<arith::SubIOp>(op)) {
          emitBin(sub.getResult(), sub.getLhs(), sub.getRhs(), "-");
          continue;
        }
        if (auto band = dyn_cast<arith::AndIOp>(op)) {
          emitBin(band.getResult(), band.getLhs(), band.getRhs(), "&");
          continue;
        }
        if (auto bor = dyn_cast<arith::OrIOp>(op)) {
          emitBin(bor.getResult(), bor.getLhs(), bor.getRhs(), "|");
          continue;
        }
        if (auto bxor = dyn_cast<arith::XOrIOp>(op)) {
          emitBin(bxor.getResult(), bxor.getLhs(), bxor.getRhs(), "^");
          continue;
        }
        if (auto shl = dyn_cast<arith::ShLIOp>(op)) {
          emitAssign(shl.getResult(),
                     Twine(bind(shl.getLhs())) + " << (" + bind(shl.getRhs()) +
                         " & " +
                         std::to_string(integerBitWidth(shl.getType()) - 1) +
                         "u)");
          continue;
        }
        if (auto shr = dyn_cast<arith::ShRUIOp>(op)) {
          emitAssign(shr.getResult(),
                     Twine(bind(shr.getLhs())) + " >> (" + bind(shr.getRhs()) +
                         " & " +
                         std::to_string(integerBitWidth(shr.getType()) - 1) +
                         "u)");
          continue;
        }
        if (auto sra = dyn_cast<arith::ShRSIOp>(op)) {
          unsigned width = integerBitWidth(sra.getType());
          emitAssign(sra.getResult(),
                     Twine("static_cast<") + cppTypeName(model, sra.getType()) +
                         ">(static_cast<" + signedCppType(width) + ">(" +
                         bind(sra.getLhs()) + ") >> (" + bind(sra.getRhs()) +
                         " & " + std::to_string(width - 1) + "))");
          continue;
        }
        if (auto cmp = dyn_cast<arith::CmpIOp>(op)) {
          std::string lhs = bind(cmp.getLhs());
          std::string rhs = bind(cmp.getRhs());
          unsigned width = integerBitWidth(cmp.getLhs().getType());
          std::string signedTy = signedCppType(width);
          std::string expr;
          switch (cmp.getPredicate()) {
          case arith::CmpIPredicate::eq:
            expr = lhs + " == " + rhs;
            break;
          case arith::CmpIPredicate::ne:
            expr = lhs + " != " + rhs;
            break;
          case arith::CmpIPredicate::slt:
            expr = "static_cast<" + signedTy + ">(" + lhs +
                   ") < static_cast<" + signedTy + ">(" + rhs + ")";
            break;
          case arith::CmpIPredicate::sle:
            expr = "static_cast<" + signedTy + ">(" + lhs +
                   ") <= static_cast<" + signedTy + ">(" + rhs + ")";
            break;
          case arith::CmpIPredicate::sgt:
            expr = "static_cast<" + signedTy + ">(" + lhs +
                   ") > static_cast<" + signedTy + ">(" + rhs + ")";
            break;
          case arith::CmpIPredicate::sge:
            expr = "static_cast<" + signedTy + ">(" + lhs +
                   ") >= static_cast<" + signedTy + ">(" + rhs + ")";
            break;
          case arith::CmpIPredicate::ult:
            expr = lhs + " < " + rhs;
            break;
          case arith::CmpIPredicate::ule:
            expr = lhs + " <= " + rhs;
            break;
          case arith::CmpIPredicate::ugt:
            expr = lhs + " > " + rhs;
            break;
          case arith::CmpIPredicate::uge:
            expr = lhs + " >= " + rhs;
            break;
          }
          emitAssign(cmp.getResult(), expr);
          continue;
        }
        if (auto select = dyn_cast<arith::SelectOp>(op)) {
          emitAssign(select.getResult(),
                     Twine(bind(select.getCondition())) + " ? " +
                         bind(select.getTrueValue()) + " : " +
                         bind(select.getFalseValue()));
          continue;
        }
        if (auto cast = dyn_cast<arith::IndexCastOp>(op)) {
          emitAssign(cast.getResult(),
                     Twine("static_cast<") +
                         cppTypeName(model, cast.getResult().getType()) + ">(" +
                         bind(cast.getIn()) + ")");
          continue;
        }
        if (auto branch = dyn_cast<cf::BranchOp>(op)) {
          for (auto [argument, operand] :
               llvm::zip_equal(branch.getDest()->getArguments(),
                               branch.getDestOperands()))
            os << "      " << bind(argument) << " = " << bind(operand)
               << ";\n";
          os << "      goto " << label(branch.getDest()) << ";\n";
          continue;
        }
        if (auto cond = dyn_cast<cf::CondBranchOp>(op)) {
          os << "      if (" << bind(cond.getCondition()) << ") {\n";
          for (auto [argument, operand] :
               llvm::zip_equal(cond.getTrueDest()->getArguments(),
                               cond.getTrueDestOperands()))
            os << "        " << bind(argument) << " = " << bind(operand)
               << ";\n";
          os << "        goto " << label(cond.getTrueDest()) << ";\n";
          os << "      }\n";
          for (auto [argument, operand] :
               llvm::zip_equal(cond.getFalseDest()->getArguments(),
                               cond.getFalseDestOperands()))
            os << "      " << bind(argument) << " = " << bind(operand)
               << ";\n";
          os << "      goto " << label(cond.getFalseDest()) << ";\n";
          continue;
        }
        if (auto load = dyn_cast<LiveLoadOp>(op)) {
          std::string name = bind(load.getResult());
          os << "      ";
          if (!cfg)
            os << "auto ";
          os << name << " = " << load.getSlot() << "_;\n";
          continue;
        }
        if (auto store = dyn_cast<LiveStoreOp>(op)) {
          os << "      proposed_" << store.getSlot() << "_ = "
             << bind(store.getValue()) << ";\n";
          continue;
        }
        if (auto invoke = dyn_cast<InvokeOp>(op)) {
          if (failed(emitCall(os, module, invoke.getCalleeAttr(),
                              invoke.getArgs(), invoke.getResults(), names,
                              bind, true)))
            return failure();
          continue;
        }
        if (auto inlineOp = dyn_cast<InlineOp>(op)) {
          if (failed(emitCall(os, module, inlineOp.getCalleeAttr(),
                              inlineOp.getArgs(), ValueRange(inlineOp.getResult()),
                              names, bind, false)))
            return failure();
          continue;
        }
        if (auto cont = dyn_cast<ContinueOp>(op)) {
          std::string target = pcIdent(cont.getTargetPcAttr());
          os << "      proposedPc_ = Pc::" << target << ";\n";
          os << "      running = Pc::" << target << ";\n";
          os << "      break;\n";
          continue;
        }
        if (auto suspend = dyn_cast<SuspendOp>(op)) {
          std::string target = pcIdent(suspend.getTargetPcAttr());
          os << "      proposedPc_ = Pc::" << target << ";\n";
          os << "      proposedWake_ = " << bind(suspend.getWake())
             << ".ready;\n";
          os << "      suspended_ = true;\n";
          os << "      return;\n";
          continue;
        }
        if (auto term = dyn_cast<TerminateOp>(op)) {
          os << "      terminated_ = true;\n";
          os << "      if (system)\n";
          os << "        system->requestTerminate(";
          if (term.getStatus() == "success")
            os << "gfsim::TerminationClass::Completed, \"success\");\n";
          else
            os << "gfsim::TerminationClass::Failed, \"failure\");\n";
          os << "      return;\n";
          continue;
        }
        return op.emitOpError("ACSIM-EMIT: unsupported process operation");
      }
      if (cfg)
        os << "    }\n";
    }
    assignResults = false;
    return success();
  }

  LogicalResult emitCall(llvm::raw_ostream &os, acsim::ModuleOp module,
                         FlatSymbolRefAttr callee, ValueRange args,
                         ValueRange results,
                         llvm::DenseMap<Value, std::string> &names,
                         llvm::function_ref<std::string(Value)> bind,
                         bool stateful) {
    Operation *resolved = lookupSymbol(model, callee);
    std::string fn;
    bool implicitEpoch = false;
    std::string cppName;
    if (auto type = dyn_cast_or_null<TypeOp>(resolved)) {
      fn = type.getCppName().str();
      cppName = fn;
      implicitEpoch = stateful && args.empty();
    } else if (auto binding = dyn_cast_or_null<BindingOp>(resolved)) {
      fn = stateful ? bindingEntry(binding, "work")
                    : bindingEntry(binding, "pure");
      if (fn.empty())
        fn = bindingSymbol(binding);
    } else {
      fn = callee.getValue().str();
    }

    auto emitResult = [&](Value result, const Twine &expr) {
      os << "      ";
      if (!assignResults)
        os << cppTypeName(model, result.getType()) << ' ';
      os << bind(result) << " = " << expr << ";\n";
    };
    auto resolveMember = [&](StringRef stem)
        -> std::pair<acsim::ModuleOp, StringRef> {
      StringRef symbol = callee.getValue();
      acsim::ModuleOp declaring;
      StringRef field;
      size_t best = 0;
      for (acsim::ModuleOp candidate : modules) {
        std::string prefix =
            (stem + "_" + candidate.getSymName() + "_").str();
        if (symbol.starts_with(prefix) && prefix.size() > best) {
          declaring = candidate;
          field = symbol.drop_front(prefix.size());
          best = prefix.size();
        }
      }
      return {declaring, field};
    };
    auto ownerCast = [&](acsim::ModuleOp declaring) {
      if (!declaring || declaring == module)
        return "static_cast<" + ownerTypeName(module) + " *>(owner_)";
      return "static_cast<" + ownerTypeName(declaring) +
             " *>(static_cast<" + ownerTypeName(module) +
             " *>(owner_)->parent_)";
    };
    auto traceSource = [&](StringRef kind) {
      std::string prefix = ("acir_trace_" + kind + "_").str();
      StringRef symbol = callee.getValue();
      return symbol.starts_with(prefix)
                 ? symbol.drop_front(prefix.size()).str()
                 : std::string("pto");
    };

    if (cppName == "acir.trace.open") {
      std::string source = traceSource("open");
      emitResult(results.front(),
                 Twine("system ? system->traceOpen(\"") + source +
                     "\") : UINT64_C(0)");
      return success();
    }
    if (cppName == "acir.trace.next") {
      std::string source = traceSource("next");
      std::string pack = "trace_next_" + bind(results.front());
      os << "      gfsim::TraceNextResult " << pack << "{};\n";
      os << "      if (system)\n";
      os << "        " << pack << " = system->traceNext(\"" << source
         << "\", static_cast<std::uint64_t>(" << bind(args.front())
         << "));\n";
      emitResult(results[0],
                 Twine("static_cast<") +
                     cppTypeName(model, results[0].getType()) + ">(" + pack +
                     ".cursor)");
      emitResult(results[1],
                 Twine("static_cast<") +
                     cppTypeName(model, results[1].getType()) + ">(" + pack +
                     ".handle)");
      emitResult(results[2], Twine(pack) + ".advanced");
      return success();
    }
    if (cppName == "acir.trace.decode") {
      emitResult(results.front(),
                 Twine("static_cast<") +
                     cppTypeName(model, results.front().getType()) +
                     ">(system ? system->traceDecode(static_cast<std::uint64_t>(" +
                     bind(args.front()) + ")) : UINT64_C(0))");
      return success();
    }
    if (cppName == "acir.trace.eof") {
      std::string source = traceSource("eof");
      emitResult(results.front(),
                 Twine("system && system->traceEof(\"") + source +
                     "\", static_cast<std::uint64_t>(" + bind(args.front()) +
                     "))");
      return success();
    }
    if (cppName == "acir.trace.position") {
      std::string source = traceSource("position");
      emitResult(results.front(),
                 Twine("static_cast<") +
                     cppTypeName(model, results.front().getType()) +
                     ">(system ? system->tracePosition(\"" + source +
                     "\", static_cast<std::uint64_t>(" + bind(args.front()) +
                     ")) : UINT64_C(0))");
      return success();
    }
    if (cppName == "acir.register.load") {
      auto [declaring, field] = resolveMember("acir_register_load");
      std::string owner = ownerCast(declaring);
      emitResult(results.front(),
                 Twine(owner) + " ? " + owner + "->" + field +
                     ".load() : " + cppTypeName(model, results.front().getType()) +
                     "{}");
      emitResult(results[1], "true");
      return success();
    }
    if (cppName == "acir.register.store") {
      auto [declaring, field] = resolveMember("acir_register_store");
      os << "      if (auto *owner = " << ownerCast(declaring) << ")\n";
      os << "        owner->" << field << ".store(" << bind(args.front())
         << ");\n";
      emitResult(results.front(), "true");
      return success();
    }
    if (cppName == "acir.regfile.read") {
      auto [declaring, field] = resolveMember("acir_regfile_read");
      std::string owner = ownerCast(declaring);
      emitResult(results.front(),
                 Twine(owner) + " ? " + owner + "->" + field +
                     ".read(static_cast<std::uint32_t>(" + bind(args.front()) +
                     ")) : " + cppTypeName(model, results.front().getType()) +
                     "{}");
      emitResult(results[1], "true");
      return success();
    }
    if (cppName == "acir.regfile.write") {
      auto [declaring, field] = resolveMember("acir_regfile_write");
      os << "      if (auto *owner = " << ownerCast(declaring) << ")\n";
      os << "        owner->" << field << ".write(static_cast<std::uint32_t>("
         << bind(args.front()) << "), " << bind(args[1]) << ");\n";
      emitResult(results.front(), "true");
      return success();
    }
    if (cppName == "acir.queue.push") {
      auto [declaring, queue] = resolveMember("acir_queue_push");
      std::string accepted = bind(results.front());
      os << "      ";
      if (!assignResults)
        os << "bool ";
      os << accepted << " = " << ownerCast(declaring) << "->" << queue
         << ".proposePush(" << bind(args.front()) << ");\n";
      return success();
    }
    if (cppName == "acir.queue.pop") {
      auto [declaring, queue] = resolveMember("acir_queue_pop");
      std::string valueName = bind(results.front());
      std::string validName = bind(results[1]);
      os << "      auto popped_" << valueName << " = "
         << ownerCast(declaring) << "->" << queue << ".proposePop();\n";
      os << "      ";
      if (!assignResults)
        os << cppTypeName(model, results.front().getType()) << ' ';
      os << valueName << " = popped_" << valueName << ".value_or(0);\n";
      os << "      ";
      if (!assignResults)
        os << "bool ";
      os << validName << " = popped_" << valueName << ".has_value();\n";
      return success();
    }
    if (cppName == "acir.complete") {
      StringRef prefix = "value";
      StringRef symbol = callee.getValue();
      if (symbol.starts_with("acir_complete_") && symbol.size() > 14)
        prefix = symbol.drop_front(14);
      os << "      terminated_ = true;\n";
      os << "      if (system)\n";
      os << "        system->requestTerminate(gfsim::TerminationClass::"
            "Completed, \""
         << prefix
         << "=\" + std::to_string(static_cast<unsigned long "
            "long>("
         << bind(args.front()) << ")));\n";
      os << "      return;\n";
      return success();
    }
    if (cppName == "acir.fail") {
      os << "      if (!" << bind(args.front()) << ") {\n";
      os << "        terminated_ = true;\n";
      os << "        if (system)\n";
      os << "          system->requestTerminate(gfsim::TerminationClass::"
            "Failed, \"failure\");\n";
      os << "        return;\n";
      os << "      }\n";
      return success();
    }
    if (cppName == "acir.stat.add") {
      auto [declaring, stat] = resolveMember("acir_stat_add");
      os << "      if (auto *owner = " << ownerCast(declaring) << ") {\n";
      os << "        owner->" << stat << "_ += static_cast<std::uint64_t>("
         << bind(args.front()) << ");\n";
      os << "        if (system)\n";
      os << "          system->recordStat(\"" << stat << "\", owner->" << stat
         << "_);\n";
      os << "      }\n";
      return success();
    }
    if (cppName == "acir.schedule") {
      auto [declaring, target] = resolveMember("acir_schedule");
      (void)declaring;
      int64_t objectId = -1;
      for (const DispatchInfo &info : dispatches) {
        if (info.memberAccess == target ||
            StringRef(info.path).ends_with(("." + target).str())) {
          objectId = info.objectId;
          break;
        }
      }
      os << "      if (system)\n";
      os << "        system->scheduleEvent({{epoch.time + static_cast<"
            "gfsim::Tick>("
         << bind(args[1]) << "), 0}, static_cast<gfsim::ObjectId>("
         << (objectId < 0 ? 0 : objectId) << "), 0, static_cast<std::uint64_t>("
         << bind(args[0]) << ")});\n";
      return success();
    }
    if (cppName == "acir.resource.acquire") {
      auto [declaring, resource] = resolveMember("acir_resource_acquire");
      std::string owner = ownerCast(declaring);
      emitResult(results.front(),
                 Twine(owner) + " ? " + owner + "->" + resource +
                     ".proposeReserve(id, 1, epoch, 0) : false");
      return success();
    }
    if (cppName == "acir.resource.release") {
      auto [declaring, resource] = resolveMember("acir_resource_release");
      os << "      if (auto *owner = " << ownerCast(declaring) << ")\n";
      os << "        owner->" << resource << ".proposeRelease(id, 1);\n";
      if (!results.empty())
        emitResult(results.front(), "true");
      return success();
    }
    if (cppName == "acir.probe") {
      StringRef symbol = callee.getValue();
      std::string kindPrefix = "acir_probe_";
      StringRef rest = symbol.starts_with(kindPrefix)
                           ? symbol.drop_front(kindPrefix.size())
                           : StringRef();
      std::string moduleTag = (module.getSymName() + "_").str();
      auto pos = rest.find(moduleTag);
      StringRef target;
      StringRef kind;
      if (pos != StringRef::npos) {
        kind = rest.take_front(pos);
        if (kind.ends_with("_"))
          kind = kind.drop_back();
        target = rest.drop_front(pos + moduleTag.size());
      }
      emitResult(results.front(),
                 Twine("static_cast<") +
                     cppTypeName(model, results.front().getType()) + ">(" +
                     ownerCast(module) + " ? " + ownerCast(module) + "->" +
                     target +
                     ".committedSize() : 0)");
      return success();
    }

    std::string lhs;
    if (results.size() == 1) {
      if (assignResults)
        lhs = bind(results.front()) + " = ";
      else
        lhs = "auto " + bind(results.front()) + " = ";
    } else if (results.size() > 1) {
      os << "      auto invoke_pack_" << bind(results.front()) << " = " << fn
         << '(';
      if (implicitEpoch)
        os << "epoch";
      for (size_t index = 0; index < args.size(); ++index) {
        if (index || implicitEpoch)
          os << ", ";
        os << bind(args[index]);
      }
      os << ");\n";
      for (auto [index, result] : llvm::enumerate(results)) {
        os << "      ";
        if (!assignResults)
          os << cppTypeName(model, result.getType()) << ' ';
        os << bind(result) << " = std::get<" << index << ">(invoke_pack_"
           << bind(results.front()) << ");\n";
      }
      (void)names;
      return success();
    }

    os << "      " << lhs << fn << '(';
    if (implicitEpoch)
      os << "epoch";
    for (size_t index = 0; index < args.size(); ++index) {
      if (index || implicitEpoch)
        os << ", ";
      os << bind(args[index]);
    }
    os << ");\n";
    (void)names;
    return success();
  }

  void emitModelGlue(llvm::raw_ostream &os) {
    acsim::ModuleOp root = nullptr;
    for (acsim::ModuleOp module : modules)
      if (module.getSymName() == rootName)
        root = module;

    os << "namespace acsim_generated {\n\n";
    os << "GeneratedModel::GeneratedModel(std::string name)\n";
    os << "    : system(std::move(name))";
    if (root)
      os << ",\n      root(system, \"" << rootName << "\")";
    os << " {\n";

    llvm::SmallVector<std::string, 8> offsetInit(activationCount + 1, "0");
    llvm::SmallVector<std::string, 8> targets;
    std::map<int64_t, llvm::SmallVector<int64_t, 4>> bySource;
    for (auto [source, target] : edges)
      bySource[source].push_back(target);
    uint32_t cursor = 0;
    for (int64_t source = 0; source < activationCount; ++source) {
      offsetInit[source] = std::to_string(cursor);
      auto found = bySource.find(source);
      if (found != bySource.end()) {
        llvm::sort(found->second);
        for (int64_t target : found->second) {
          targets.push_back(std::to_string(target));
          ++cursor;
        }
      }
    }
    if (activationCount >= 0)
      offsetInit[activationCount] = std::to_string(cursor);

    os << "  activationOffsets = {";
    for (size_t index = 0; index < offsetInit.size(); ++index) {
      if (index)
        os << ", ";
      os << offsetInit[index];
    }
    os << "};\n";
    os << "  activationTargets = {";
    if (targets.empty())
      os << "0";
    else {
      for (size_t index = 0; index < targets.size(); ++index) {
        if (index)
          os << ", ";
        os << targets[index];
      }
    }
    os << "};\n";

    for (const DispatchInfo &info : dispatches) {
      os << "  dispatch[" << info.objectId << "] = {";
      if (root && !info.memberAccess.empty())
        os << "&root." << info.memberAccess << ", ";
      else
        os << "nullptr, ";
      os << '&' << thunkFunction(info.work, "thunkWork") << ", ";
      os << '&' << thunkFunction(info.xfer, "thunkXfer") << ", ";
      os << '&' << thunkFunction(info.reset, "thunkReset") << ", ";
      os << '&' << thunkFunction(info.validate, "thunkValidate");
      os << "};\n";
      if (root && !info.memberAccess.empty()) {
        std::string ownerAccess =
            ownerAccessFromMemberAccess(info.memberAccess);
        os << "  root." << info.memberAccess << ".bind(system, static_cast<"
           << "gfsim::ObjectId>(" << info.objectId << "), &root";
        if (!ownerAccess.empty())
          os << "." << ownerAccess;
        os << ");\n";
      }
    }

    os << "  system.setLegacyDispatchTable({dispatch.data(), static_cast<"
          "std::uint32_t>(kObjectCount)});\n";
    os << "  system.setLegacyActivationGraph({activationOffsets.data(), "
          "activationTargets.data(), static_cast<std::uint32_t>("
          "kActivationCount)});\n";
    os << "  system.setBuildProfile(";
    if (options.profile == "validated")
      os << "gfsim::BuildProfile::Validated";
    else if (options.profile == "custom")
      os << "gfsim::BuildProfile::Custom";
    else
      os << "gfsim::BuildProfile::Fast";
    os << ");\n";
    os << "}\n\n";

    os << "gfsim::TerminationResult GeneratedModel::run() {\n";
    os << "  return system.run();\n";
    os << "}\n\n";
    os << "gfsim::TerminationResult simulate() {\n";
    os << "  GeneratedModel model;\n";
    os << "  return model.run();\n";
    os << "}\n\n";
    os << "} // namespace acsim_generated\n";
  }

  std::string thunkFunction(StringRef declared, StringRef local) {
    if (declared.starts_with("acsim_generated::")) {
      QualName qual = splitQualified(declared);
      qual.ident = "Process::" + local.str();
      return joinQualified(qual);
    }
    return declared.str();
  }

  SourceFile emitMain() {
    std::string storage;
    llvm::raw_string_ostream os(storage);
    os << "#include \"generated/model.h\"\n\n";
    os << "#include <cstddef>\n";
    os << "#include <cstdint>\n";
    os << "#include <cstdlib>\n";
    os << "#include <cstring>\n";
    os << "#include <exception>\n";
    os << "#include <iostream>\n";
    os << "#include <string>\n\n";
    os << "int main(int argc, char **argv) {\n";
    os << "  std::uint64_t maxTicks = ~0ull;\n";
    os << "  std::uint64_t maxEvents = ~0ull;\n";
    os << "  std::string tracePath;\n";
    os << "  for (int index = 1; index < argc; ++index) {\n";
    os << "    if (std::strncmp(argv[index], \"--max-ticks=\", 12) == 0)\n";
    os << "      maxTicks = std::strtoull(argv[index] + 12, nullptr, 10);\n";
    os << "    else if (std::strcmp(argv[index], \"--max-ticks\") == 0 && "
          "index + 1 < argc)\n";
    os << "      maxTicks = std::strtoull(argv[++index], nullptr, 10);\n";
    os << "    else if (std::strncmp(argv[index], \"--max-events=\", 13) == 0)\n";
    os << "      maxEvents = std::strtoull(argv[index] + 13, nullptr, 10);\n";
    os << "    else if (std::strcmp(argv[index], \"--max-events\") == 0 && "
          "index + 1 < argc)\n";
    os << "      maxEvents = std::strtoull(argv[++index], nullptr, 10);\n";
    os << "    else if (std::strncmp(argv[index], \"--trace=\", 8) == 0)\n";
    os << "      tracePath = argv[index] + 8;\n";
    os << "    else if (std::strcmp(argv[index], \"--trace\") == 0 && "
          "index + 1 < argc)\n";
    os << "      tracePath = argv[++index];\n";
    os << "  }\n";
    os << "  acsim_generated::GeneratedModel model;\n";
    os << "  model.system.setMaxTicks(maxTicks);\n";
    os << "  model.system.setMaxEvents(maxEvents);\n";
    os << "  gfsim::TerminationResult result;\n";
    os << "  try {\n";
    os << "    if (!tracePath.empty())\n";
    os << "      model.system.loadPtoTrace(\"pto\", tracePath);\n";
    os << "    result = model.run();\n";
    os << "  } catch (const std::exception &error) {\n";
    os << "    result.classification = gfsim::TerminationClass::Failed;\n";
    os << "    result.diagnosticCode = error.what();\n";
    os << "  }\n";
    os << "  const char *cls = \"incomplete\";\n";
    os << "  switch (result.classification) {\n";
    os << "  case gfsim::TerminationClass::Completed:\n";
    os << "    cls = \"completed\";\n";
    os << "    break;\n";
    os << "  case gfsim::TerminationClass::Failed:\n";
    os << "    cls = \"failed\";\n";
    os << "    break;\n";
    os << "  case gfsim::TerminationClass::Incomplete:\n";
    os << "    cls = \"incomplete\";\n";
    os << "    break;\n";
    os << "  }\n";
    os << "  std::cout << \"{\\\"classification\\\":\\\"\" << cls << \"\\\",\""
          "\n";
    os << "            << \"\\\"time\\\":\" << result.finalEpoch.time << \","
          "\"\n";
    os << "            << \"\\\"delta\\\":\" << result.finalEpoch.delta << \","
          "\"\n";
    os << "            << \"\\\"work\\\":\" << "
          "model.system.workInvocationCount() << \",\"\n";
    os << "            << \"\\\"activations\\\":\" << "
          "model.system.activationTraversalCount() << \",\"\n";
    os << "            << \"\\\"stats\\\":[\";\n";
    os << "  for (size_t index = 0; index < result.stats.size(); ++index) {\n";
    os << "    if (index)\n";
    os << "      std::cout << \",\";\n";
    os << "    std::cout << \"{\\\"name\\\":\\\"\" << result.stats[index].name "
          "<< \"\\\",\\\"value\\\":\" << result.stats[index].value << \"}\";\n";
    os << "  }\n";
    os << "  std::cout << \"],\"\n";
    os << "            << \"\\\"diagnostic\\\":\\\"\" << result.diagnosticCode "
          "<< \"\\\"}\\n\";\n";
    os << "  return result.classification == "
          "gfsim::TerminationClass::Failed ? 1 : 0;\n";
    os << "}\n";
    SourceFile file;
    file.relativePath = "src/generated/main.cpp";
    file.content = os.str();
    return file;
  }

  LogicalResult writeManifest(const BuildManifest &manifest) {
    llvm::json::Array sourceFiles;
    llvm::json::Array artifacts;
    for (const SourceFile &file : manifest.sources) {
      llvm::json::Object entry;
      entry["path"] = file.relativePath;
      entry["sha256"] = withShaPrefix(file.fingerprint);
      sourceFiles.push_back(llvm::json::Value(std::move(entry)));
      llvm::json::Object artifact;
      artifact["path"] = file.relativePath;
      artifact["kind"] = StringRef(file.relativePath).ends_with(".h")
                             ? "cpp_header"
                             : "cpp_source";
      artifact["sha256"] = withShaPrefix(file.fingerprint);
      artifacts.push_back(llvm::json::Value(std::move(artifact)));
    }

    llvm::json::Array specializations;
    for (acsim::ModuleOp module : modules) {
      llvm::json::Object spec;
      spec["canonical_name"] = module.getSymName().str();
      spec["schema_fingerprint"] =
          withShaPrefix(hexFingerprint(module.getSpecializationFingerprint()));
      spec["specialization_fingerprint"] =
          module.getSpecializationFingerprint().str();
      specializations.push_back(llvm::json::Value(std::move(spec)));
    }

    llvm::json::Object compiler;
    compiler["name"] = "acsim-emit-cxx";
    compiler["build_id"] = "0.1";
    compiler["toolchain_target"] = options.toolchainTarget;

    llvm::json::Object project;
    project["name"] = "agentic-circuit";
    project["identity"] = model.getSymName().str();
    llvm::json::Object system;
    system["name"] = model.getSymName().str();
    system["identity"] = model.getSymName().str();

    llvm::json::Array gates;
    llvm::json::Object gate;
    gate["name"] = "acsim-emit-cxx";
    gate["status"] = "passed";
    gate["report_sha256"] = nullptr;
    gates.push_back(llvm::json::Value(std::move(gate)));

    std::string profile = options.profile;
    if (profile != "fast" && profile != "validated" && profile != "custom")
      profile = "fast";

    llvm::json::Array providers;
    llvm::SmallSet<std::string, 4> seenProviders;
    for (BindingOp binding : bindings) {
      DictionaryAttr record = binding.getRecord();
      std::string identity = binding.getSymName().str();
      if (record) {
        if (auto provider = record.getAs<FlatSymbolRefAttr>("provider"))
          identity = provider.getValue().str();
        else if (auto cpp = record.getAs<DictionaryAttr>("cpp")) {
          if (auto target = cpp.getAs<StringAttr>("target"))
            identity = target.getValue().str();
        }
      }
      if (!seenProviders.insert(identity).second)
        continue;
      llvm::json::Object provider;
      provider["identity"] = identity;
      provider["binding"] = binding.getSymName().str();
      providers.push_back(llvm::json::Value(std::move(provider)));
    }

    llvm::json::Array specializationInputs;
    {
      llvm::json::Object profileInput;
      profileInput["kind"] = "profile";
      profileInput["value"] = options.profile;
      specializationInputs.push_back(llvm::json::Value(std::move(profileInput)));
      llvm::json::Object targetInput;
      targetInput["kind"] = "toolchain_target";
      targetInput["value"] = options.toolchainTarget;
      specializationInputs.push_back(llvm::json::Value(std::move(targetInput)));
    }

    llvm::json::Array instrumentation;
    if (auto frozen = model.getFingerprints().getAs<StringAttr>("frozen_acir")) {
      llvm::json::Object layer;
      layer["kind"] = "frozen_acir";
      layer["fingerprint"] = frozen.getValue().str();
      instrumentation.push_back(llvm::json::Value(std::move(layer)));
    }

    llvm::json::Object root;
    root["schema"] = "agentic-circuit-build-manifest";
    root["version"] = "0.1";
    root["contract_epoch"] = "0.1";
    root["project"] = std::move(project);
    root["system"] = std::move(system);
    root["source_files"] = std::move(sourceFiles);
    root["normalized_acir_sha256"] = withShaPrefix(manifest.inputFingerprint);
    root["compiler"] = std::move(compiler);
    root["pass_pipeline"] = llvm::json::Array{"acsim-emit-cxx"};
    root["providers"] = std::move(providers);
    root["component_specializations"] = std::move(specializations);
    root["protocol_identities"] = llvm::json::Array{};
    root["artifacts"] = std::move(artifacts);
    root["validation_gates"] = std::move(gates);
    root["build_profile"] = profile;
    root["instrumentation_layers"] = std::move(instrumentation);
    root["specialization_inputs"] = std::move(specializationInputs);
    root["build_fingerprint"] = withShaPrefix(manifest.outputFingerprint);

    std::string serialized;
    llvm::raw_string_ostream json(serialized);
    json << llvm::json::Value(std::move(root)) << '\n';

    namespace fs = std::filesystem;
    fs::path path = fs::path(options.outputDir) / "build-manifest.json";
    std::ofstream out(path);
    if (!out)
      return model.emitError("ACSIM-EMIT: cannot write build-manifest.json");
    out << serialized;
    return success();
  }

  ModelOp model;
  EmitCxxOptions options;
  std::string rootName;
  std::vector<TypeOp> types;
  std::vector<BindingOp> bindings;
  std::vector<acsim::ModuleOp> modules;
  std::vector<DispatchInfo> dispatches;
  std::vector<std::pair<int64_t, int64_t>> edges;
  std::set<std::string> includes;
  std::map<std::string, std::vector<QueueMember>> queuesByModule;
  std::map<std::string, std::vector<DeviceMember>> devicesByModule;
  std::map<std::string, std::vector<ResourceMember>> resourcesByModule;
  std::map<std::string, std::vector<StatMember>> statsByModule;
  bool assignResults = false;
  int64_t objectCount = 0;
  int64_t activationCount = 0;
};

class EmitCxxPass final : public PassWrapper<EmitCxxPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(EmitCxxPass)

  EmitCxxPass() = default;
  explicit EmitCxxPass(EmitCxxOptions opts) : options(std::move(opts)) {}

  StringRef getArgument() const override { return "acsim-emit-cxx"; }
  StringRef getDescription() const override {
    return "Emit deterministic C++20 sources from canonical ACSim";
  }

  void runOnOperation() override {
    EmitCxxOptions opts = options;
    if (opts.outputDir.empty())
      opts.outputDir = clOutputDir;
    if (failed(emitCxxFile(getOperation(), opts)))
      signalPassFailure();
  }

  EmitCxxOptions options;
};

LogicalResult checkCxxContractDir(Operation *reporter, StringRef directory) {
  if (directory.empty())
    return reporter->emitError(
        "ACSIM-CHECK-CXX-CONTRACT: --acsim-output-dir is required");
  namespace fs = std::filesystem;
  fs::path headerPath =
      fs::path(directory.str()) / "include" / "generated" / "model.h";
  fs::path manifestPath = fs::path(directory.str()) / "build-manifest.json";
  auto headerBuf = llvm::MemoryBuffer::getFile(headerPath.string());
  auto manifestBuf = llvm::MemoryBuffer::getFile(manifestPath.string());
  if (!headerBuf)
    return reporter->emitError(
        "ACSIM-CHECK-CXX-CONTRACT: missing include/generated/model.h");
  if (!manifestBuf)
    return reporter->emitError(
        "ACSIM-CHECK-CXX-CONTRACT: missing build-manifest.json");
  StringRef header = headerBuf.get()->getBuffer();
  constexpr StringRef kMarker = "kBuildFingerprint[] = \"";
  auto marker = header.find(kMarker);
  if (marker == StringRef::npos)
    return reporter->emitError(
        "ACSIM-CHECK-CXX-CONTRACT: kBuildFingerprint is missing");
  StringRef rest = header.drop_front(marker + kMarker.size());
  auto end = rest.find('"');
  if (end == StringRef::npos)
    return reporter->emitError(
        "ACSIM-CHECK-CXX-CONTRACT: kBuildFingerprint is malformed");
  std::string embedded = rest.take_front(end).str();
  auto parsed = llvm::json::parse(manifestBuf.get()->getBuffer());
  if (!parsed)
    return reporter->emitError(
        "ACSIM-CHECK-CXX-CONTRACT: build-manifest.json is not JSON");
  auto *object = parsed->getAsObject();
  auto fingerprint = object ? object->getString("build_fingerprint")
                            : std::optional<StringRef>();
  if (!fingerprint)
    return reporter->emitError(
        "ACSIM-CHECK-CXX-CONTRACT: build_fingerprint is missing");
  if (embedded != fingerprint->str())
    return reporter->emitError(
        "ACSIM-CHECK-CXX-CONTRACT: embedded fingerprint does not match "
        "build-manifest.json");
  return success();
}

class CheckCxxContractPass final
    : public PassWrapper<CheckCxxContractPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CheckCxxContractPass)

  StringRef getArgument() const override { return "acsim-verify-cxx-fingerprint"; }
  StringRef getDescription() const override {
    return "Check generated C++ fingerprint against the build manifest";
  }

  void runOnOperation() override {
    if (failed(checkCxxContractDir(getOperation(), clOutputDir)))
      signalPassFailure();
  }
};

} // namespace

bool emitCxxRequested() { return !clOutputDir.empty(); }

bool checkCxxContractRequested() { return clCheckCxxContract; }

std::string emitCxxOutputDir() { return clOutputDir; }

FailureOr<BuildManifest> emitCxx(ModelOp model, const EmitCxxOptions &options) {
  return Emitter(model, options).run();
}

LogicalResult emitCxxFile(ModuleOp file, const EmitCxxOptions &options) {
  ModelOp model;
  for (Operation &op : *file.getBody()) {
    if (auto candidate = dyn_cast<ModelOp>(op)) {
      if (model)
        return file.emitError("ACSIM-EMIT: expected exactly one acsim.model");
      model = candidate;
    }
  }
  if (!model)
    return file.emitError(
        "ACSIM-EMIT: canonical ACSim input is required (run --ac-lower-to-acsim "
        "first)");
  return emitCxx(model, options);
}

std::unique_ptr<Pass> createEmitCxxPass(EmitCxxOptions options) {
  return std::make_unique<EmitCxxPass>(std::move(options));
}

std::unique_ptr<Pass> createCheckCxxContractPass() {
  return std::make_unique<CheckCxxContractPass>();
}

} // namespace acir::codegen
