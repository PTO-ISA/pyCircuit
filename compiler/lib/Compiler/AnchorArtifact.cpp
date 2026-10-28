#include "AnchorArtifact.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "Transforms/LowerTransactions.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Transforms/Passes.h"
#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/AttrTypeSubElements.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"

#include <cerrno>
#include <filesystem>
#include <map>
#include <set>
#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace mlir;
namespace acir::compiler {
namespace {
namespace json = llvm::json;
using Files = std::map<std::string, std::string>;

bool hexID(StringRef value) {
  return value.size() == 64 && llvm::all_of(value, [](char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
  });
}

bool decimal(StringRef value) {
  if (value == "0") return true;
  value.consume_front("-");
  return !value.empty() && value.front() >= '1' && value.front() <= '9' &&
         llvm::all_of(value, [](char c) { return c >= '0' && c <= '9'; });
}

std::string quote(StringRef value) {
  std::string result = "\"";
  static constexpr char digits[] = "0123456789abcdef";
  for (unsigned char c : value) {
    switch (c) {
    case '"': result += "\\\""; break;
    case '\\': result += "\\\\"; break;
    case '\b': result += "\\b"; break;
    case '\f': result += "\\f"; break;
    case '\n': result += "\\n"; break;
    case '\r': result += "\\r"; break;
    case '\t': result += "\\t"; break;
    default:
      if (c < 0x20) {
        result += "\\u00";
        result += digits[c >> 4]; result += digits[c & 15];
      } else result += c;
    }
  }
  return result + '"';
}

// JSON v1 is deliberately smaller than general JSON: no floating point,
// booleans or null. Canonical byte equality also rejects duplicate keys and
// alternate integer spellings before any fragment can be parsed.
FailureOr<std::string> canonical(const json::Value &value) {
  if (auto string = value.getAsString()) return quote(*string);
  if (auto integer = value.getAsInteger()) return std::to_string(*integer);
  if (auto array = value.getAsArray()) {
    std::string out = "[";
    for (const auto &item : *array) {
      auto text = canonical(item);
      if (failed(text)) return failure();
      if (out.size() > 1) out += ',';
      out += *text;
    }
    return out + ']';
  }
  if (auto object = value.getAsObject()) {
    std::map<std::string, const json::Value *> fields;
    for (const auto &field : *object) fields[field.first.str()] = &field.second;
    std::string out = "{";
    for (const auto &[name, item] : fields) {
      auto text = canonical(*item);
      if (failed(text)) return failure();
      if (out.size() > 1) out += ',';
      out += quote(name) + ':' + *text;
    }
    return out + '}';
  }
  return failure();
}

std::string canonicalObject(const json::Object &object) {
  return *canonical(json::Value(json::Object(object)));
}
std::string identity(const json::Object &object) {
  return anchorSha256(canonicalObject(object));
}

bool keys(const json::Object *object, std::initializer_list<StringRef> names) {
  return object && object->size() == names.size() &&
         llvm::all_of(names, [&](StringRef name) { return object->get(name); });
}
std::string string(const json::Object &object, StringRef name) {
  auto value = object.getString(name);
  return value ? value->str() : std::string();
}
bool ownerShape(const json::Object *owner) {
  return keys(owner, {"package", "path"}) && owner->getString("package") &&
         owner->getString("path");
}
json::Object ownerJSON(DictionaryAttr owner) {
  return json::Object{{"package", owner.getAs<StringAttr>("package").getValue()},
          {"path", owner.getAs<StringAttr>("path").getValue()}};
}
DictionaryAttr ownerAttr(const json::Object &owner, MLIRContext &context) {
  Builder builder(&context);
  return builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr(string(owner, "package"))),
      builder.getNamedAttr("path", builder.getStringAttr(string(owner, "path")))});
}
std::string symbol(StringRef name) { return "@\"" + name.str() + "\""; }
FailureOr<std::string> symbolName(StringRef name, MLIRContext &context) {
  auto reference = dyn_cast_or_null<FlatSymbolRefAttr>(parseAttribute(name, &context));
  if (!reference || symbol(reference.getValue()) != name) return failure();
  return reference.getValue().str();
}

class BoundedText : public llvm::raw_ostream {
public:
  explicit BoundedText(uint64_t limit) : llvm::raw_ostream(true), limit(limit) {}
  std::string text;
  bool overflow = false;
private:
  void write_impl(const char *bytes, size_t size) override {
    if (size > limit - text.size()) { overflow = true; return; }
    if (!overflow) text.append(bytes, size);
  }
  uint64_t current_pos() const override { return text.size(); }
  uint64_t limit;
};
std::string print(Operation *operation, uint64_t limit = AnchorFormatLimits::fileBytes) {
  BoundedText output(limit);
  operation->print(output, OpPrintingFlags().enableDebugInfo());
  output << '\n';
  return output.overflow ? std::string() : std::move(output.text);
}
std::string print(Attribute attribute, uint64_t limit = AnchorFormatLimits::fileBytes) {
  BoundedText output(limit);
  attribute.print(output); output << '\n';
  return output.overflow ? std::string() : std::move(output.text);
}
DictionaryAttr spec(ac::ModuleOp definition) {
  Builder builder(definition.getContext());
  return builder.getDictionaryAttr({
      builder.getNamedAttr("properties", definition->getPropertiesAsAttribute()),
      builder.getNamedAttr("attributes", definition->getDiscardableAttrDictionary())});
}

// Only actual callee and native type references are semantic dependencies.
// In particular Occurrence/SourceSpan symbols are never provider requests.
std::set<std::string> dependencies(Operation *declaration) {
  std::set<std::string> result;
  AttrTypeWalker walker;
  walker.addWalk([&](ac::StructType type) { result.insert(symbol(type.getName().getValue())); });
  walker.addWalk([&](ac::EnumType type) { result.insert(symbol(type.getName().getValue())); });
  auto types = [&](Operation *operation) {
    for (Type type : operation->getOperandTypes()) walker.walk(type);
    for (Type type : operation->getResultTypes()) walker.walk(type);
    for (Region &region : operation->getRegions())
      for (Block &block : region)
        for (Type type : block.getArgumentTypes()) walker.walk(type);
    // TypeAttr is the only route from declaration metadata to a type. The
    // walker does not treat FlatSymbolRefAttr as an executable reference.
    if (auto properties = operation->getPropertiesAsAttribute()) walker.walk(properties);
    walker.walk(operation->getDiscardableAttrDictionary());
  };
  declaration->walk([&](Operation *operation) {
    if (isa<ac::InstanceOp, ac::CollectionOp>(operation))
      if (auto callee = operation->getAttrOfType<FlatSymbolRefAttr>("callee"))
        result.insert(symbol(callee.getValue()));
    types(operation);
  });
  if (auto name = declaration->getAttrOfType<StringAttr>("sym_name"))
    result.erase(symbol(name.getValue()));
  return result;
}
json::Array dependencyJSON(const std::set<std::string> &values) {
  json::Array result;
  for (const auto &value : values) result.push_back(value);
  return result;
}

#ifndef _WIN32
struct TreeEntry { struct stat status; bool directory; };
using Tree = std::map<std::string, TreeEntry>;
bool sameStat(const struct stat &left, const struct stat &right) {
#ifdef __APPLE__
  auto lm = left.st_mtimespec, rm = right.st_mtimespec;
  auto lc = left.st_ctimespec, rc = right.st_ctimespec;
#else
  auto lm = left.st_mtim, rm = right.st_mtim;
  auto lc = left.st_ctim, rc = right.st_ctim;
#endif
  return left.st_dev == right.st_dev && left.st_ino == right.st_ino &&
         left.st_mode == right.st_mode && left.st_size == right.st_size &&
         lm.tv_sec == rm.tv_sec && lm.tv_nsec == rm.tv_nsec &&
         lc.tv_sec == rc.tv_sec && lc.tv_nsec == rc.tv_nsec;
}
struct FD {
  int value;
  explicit FD(int value) : value(value) {}
  ~FD() { if (value >= 0) ::close(value); }
  FD(const FD &) = delete;
};

int openDirectoryNoSymlinks(StringRef path) {
  FD current(::open(path.starts_with("/") ? "/" : ".", O_RDONLY | O_DIRECTORY | O_NOFOLLOW));
  SmallVector<StringRef> components;
  path.split(components, '/', /*MaxSplit=*/-1, /*KeepEmpty=*/false);
  for (StringRef component : components) {
    if (component == ".") continue;
    int next = ::openat(current.value, component.str().c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (next < 0) return -1;
    ::close(current.value); current.value = next;
  }
  int result = current.value; current.value = -1;
  return result;
}

LogicalResult enumerateTree(int descriptor, const std::string &prefix, Tree &tree,
                        uint64_t &bytes, uint64_t &files, uint64_t &directories,
                        ac::detail::EmitError emitError) {
  FD scan(::openat(descriptor, ".", O_RDONLY | O_DIRECTORY | O_NOFOLLOW));
  DIR *directory = scan.value >= 0 ? ::fdopendir(scan.value) : nullptr;
  if (!directory) return emitError() << "cannot enumerate anchor directory";
  scan.value = -1; // closedir owns this descriptor.
  bool okay = true;
  errno = 0;
  while (auto *entry = ::readdir(directory)) {
    StringRef name(entry->d_name);
    if (name == "." || name == "..") continue;
    std::string path = prefix.empty() ? name.str() : prefix + '/' + name.str();
    struct stat status;
    if (path.size() > AnchorFormatLimits::pathBytes ||
        std::count(path.begin(), path.end(), '/') + 1 > AnchorFormatLimits::depth ||
        ::fstatat(descriptor, entry->d_name, &status, AT_SYMLINK_NOFOLLOW) != 0 ||
        (!S_ISDIR(status.st_mode) && !S_ISREG(status.st_mode))) {
      emitError() << "anchor tree requires bounded paths and ordinary files/directories without symlinks";
      okay = false; break;
    }
    bool isDirectory = S_ISDIR(status.st_mode);
    if (isDirectory) {
      if (++directories > AnchorFormatLimits::directories) {
        emitError() << "anchor directory count exceeds format limit"; okay = false; break;
      }
    } else {
      uint64_t limit = path == "anchor.json" ? AnchorFormatLimits::manifestBytes
                                            : AnchorFormatLimits::fileBytes;
      if (status.st_size < 0 || uint64_t(status.st_size) > limit ||
          (path != "anchor.json" &&
           (++files > AnchorFormatLimits::payloadFiles ||
            uint64_t(status.st_size) > AnchorFormatLimits::totalBytes - bytes))) {
        emitError() << "anchor file/count/aggregate bytes exceed format limit"; okay = false; break;
      }
      if (path != "anchor.json") bytes += status.st_size;
    }
    tree.emplace(path, TreeEntry{status, isDirectory});
    if (isDirectory) {
      FD child(::openat(descriptor, entry->d_name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW));
      if (child.value < 0 || failed(enumerateTree(child.value, path, tree, bytes, files,
                                            directories, emitError))) {
        okay = false; break;
      }
    }
    errno = 0;
  }
  if (okay && errno) { emitError() << "anchor directory changed during enumeration"; okay = false; }
  ::closedir(directory);
  return success(okay);
}

FailureOr<std::string> readCaptured(int root, StringRef path,
                                   const TreeEntry &entry,
                                   ac::detail::EmitError emitError) {
  SmallVector<StringRef> components;
  path.split(components, '/');
  FD current(::dup(root));
  for (StringRef component : ArrayRef<StringRef>(components).drop_back()) {
    int next = ::openat(current.value, component.str().c_str(),
                        O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (next < 0) return emitError() << "anchor parent changed during capture";
    ::close(current.value); current.value = next;
  }
  // A regular file can be replaced by a FIFO after enumeration. Never block
  // waiting for a writer before fstat can reject that changed file type.
  FD file(::openat(current.value, components.back().str().c_str(),
                   O_RDONLY | O_NOFOLLOW | O_NONBLOCK));
  struct stat before, after;
  if (file.value < 0 || ::fstat(file.value, &before) || !sameStat(before, entry.status))
    return emitError() << "anchor file changed before capture: " << path;
  std::string result(static_cast<size_t>(before.st_size), '\0');
  size_t readBytes = 0;
  while (readBytes < result.size()) {
    auto count = ::read(file.value, result.data() + readBytes, result.size() - readBytes);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) return emitError() << "anchor file changed during capture: " << path;
    readBytes += count;
  }
  if (::fstat(file.value, &after) || !sameStat(before, after))
    return emitError() << "anchor file changed during capture: " << path;
  return result;
}

FailureOr<Files> capture(StringRef path, ac::detail::EmitError emitError) {
  FD root(openDirectoryNoSymlinks(path));
  if (root.value < 0) return emitError() << "anchor input must be an ordinary directory";
  struct stat rootBefore, rootAfter;
  if (::fstat(root.value, &rootBefore)) return emitError() << "cannot inspect anchor input root";
  Tree tree;
  uint64_t bytes = 0, files = 0, directories = 0;
  if (failed(enumerateTree(root.value, "", tree, bytes, files, directories, emitError))) return failure();
  Files contents;
  for (const auto &[name, entry] : tree) {
    if (entry.directory) continue;
    auto text = readCaptured(root.value, name, entry, emitError);
    if (failed(text)) return failure();
    contents.emplace(name, std::move(*text));
  }
  Tree after;
  bytes = files = directories = 0;
  if (failed(enumerateTree(root.value, "", after, bytes, files, directories, emitError)) ||
      tree.size() != after.size()) return emitError() << "anchor tree changed during capture";
  for (const auto &[name, entry] : tree) {
    auto found = after.find(name);
    if (found == after.end() || !sameStat(entry.status, found->second.status))
      return emitError() << "anchor tree changed during capture";
  }
  // All directories must be parents of an actual file. Empty extras are not a
  // cache extension and cannot evade the exact inventory below.
  std::set<std::string> requiredDirectories;
  for (const auto &[name, text] : contents)
    for (size_t slash = name.find('/'); slash != std::string::npos; slash = name.find('/', slash + 1))
      requiredDirectories.insert(name.substr(0, slash));
  for (const auto &[name, entry] : tree)
    if (entry.directory && !requiredDirectories.contains(name))
      return emitError() << "anchor contains an extra directory: " << name;
  FD reopened(openDirectoryNoSymlinks(path));
  if (reopened.value < 0 || ::fstat(reopened.value, &rootAfter) || !sameStat(rootBefore, rootAfter))
    return emitError() << "anchor root identity changed during capture";
  return contents;
}
#else
struct PortableEntry { llvm::sys::fs::file_status status; bool directory; };
using PortableTree = std::map<std::string, PortableEntry>;
bool noReparseChain(const std::filesystem::path &path) {
  std::error_code error;
  auto absolute = std::filesystem::absolute(path, error);
  if (error) return false;
  std::filesystem::path prefix;
  for (const auto &component : absolute) {
    prefix /= component;
    auto attributes = ::GetFileAttributesW(prefix.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
  }
  return true;
}
bool sameStatus(const llvm::sys::fs::file_status &left,
                const llvm::sys::fs::file_status &right) {
  return left.type() == right.type() && left.getUniqueID() == right.getUniqueID() &&
         left.getSize() == right.getSize() &&
         left.getLastModificationTime() == right.getLastModificationTime();
}
FailureOr<PortableTree> portableTree(StringRef path, ac::detail::EmitError emitError) {
  namespace fs = std::filesystem;
  std::error_code error;
  auto root = fs::path(path.str());
  if (!noReparseChain(root) || !fs::is_directory(fs::symlink_status(root, error)) || error)
    return emitError() << "anchor input must be an ordinary directory";
  PortableTree tree;
  uint64_t bytes = 0, files = 0, directories = 0;
  fs::recursive_directory_iterator iterator(root, fs::directory_options::none, error), end;
  if (error) return emitError() << "cannot enumerate anchor directory";
  for (; iterator != end; iterator.increment(error)) {
    if (error) return emitError() << "anchor directory changed during enumeration";
    auto relative = iterator->path().lexically_relative(root).generic_string();
    llvm::sys::fs::file_status status;
    if (relative.size() > AnchorFormatLimits::pathBytes ||
        std::count(relative.begin(), relative.end(), '/') + 1 > AnchorFormatLimits::depth ||
        !noReparseChain(iterator->path()) ||
        llvm::sys::fs::status(iterator->path().string(), status, false) ||
        (!llvm::sys::fs::is_directory(status) && !llvm::sys::fs::is_regular_file(status)))
      return emitError() << "anchor tree requires bounded ordinary files/directories without symlinks";
    bool directory = llvm::sys::fs::is_directory(status);
    if (directory) {
      if (++directories > AnchorFormatLimits::directories)
        return emitError() << "anchor directory count exceeds format limit";
    } else {
      uint64_t limit = relative == "anchor.json" ? AnchorFormatLimits::manifestBytes : AnchorFormatLimits::fileBytes;
      if (status.getSize() > limit || (relative != "anchor.json" &&
          (++files > AnchorFormatLimits::payloadFiles || status.getSize() > AnchorFormatLimits::totalBytes - bytes)))
        return emitError() << "anchor file/count/aggregate bytes exceed format limit";
      if (relative != "anchor.json") bytes += status.getSize();
    }
    tree.emplace(relative, PortableEntry{status, directory});
  }
  if (error) return emitError() << "anchor directory changed during enumeration";
  return tree;
}
FailureOr<Files> capture(StringRef path, ac::detail::EmitError emitError) {
  llvm::sys::fs::file_status rootBefore, rootAfter;
  if (!noReparseChain(std::filesystem::path(path.str())) || llvm::sys::fs::status(path, rootBefore, false))
    return emitError() << "anchor input root/ancestors must be ordinary directories";
  auto tree = portableTree(path, emitError);
  if (failed(tree)) return failure();
  Files contents;
  std::set<std::string> requiredDirectories;
  for (const auto &[relative, entry] : *tree) {
    if (entry.directory) continue;
    auto native = llvm::sys::fs::openNativeFileForRead((std::filesystem::path(path.str()) / relative).string());
    if (!native) { llvm::consumeError(native.takeError()); return emitError() << "cannot capture anchor file"; }
    auto handle = *native;
    llvm::sys::fs::file_status before, after;
    if (llvm::sys::fs::status(handle, before) || !sameStatus(before, entry.status)) {
      llvm::sys::fs::closeFile(handle); return emitError() << "anchor file changed before capture";
    }
    std::string text(before.getSize(), '\0');
    size_t offset = 0;
    while (offset < text.size()) {
      auto count = llvm::sys::fs::readNativeFile(handle,
          llvm::MutableArrayRef<char>(text.data() + offset, text.size() - offset));
      if (!count || !*count) {
        if (!count) llvm::consumeError(count.takeError());
        llvm::sys::fs::closeFile(handle); return emitError() << "anchor file changed during capture";
      }
      offset += *count;
    }
    bool unchanged = !llvm::sys::fs::status(handle, after) && sameStatus(before, after);
    llvm::sys::fs::closeFile(handle);
    if (!unchanged) return emitError() << "anchor file changed during capture";
    contents.emplace(relative, std::move(text));
    for (size_t slash = relative.find('/'); slash != std::string::npos; slash = relative.find('/', slash + 1))
      requiredDirectories.insert(relative.substr(0, slash));
  }
  auto after = portableTree(path, emitError);
  if (failed(after) || tree->size() != after->size()) return emitError() << "anchor tree changed during capture";
  for (const auto &[relative, entry] : *tree) {
    auto found = after->find(relative);
    if (found == after->end() || !sameStatus(entry.status, found->second.status) ||
        (entry.directory && !requiredDirectories.contains(relative)))
      return emitError() << "anchor tree changed or contains extra directories";
  }
  if (!noReparseChain(std::filesystem::path(path.str())) ||
      llvm::sys::fs::status(path, rootAfter, false) || !sameStatus(rootBefore, rootAfter))
    return emitError() << "anchor root identity changed during capture";
  return contents;
}
#endif

bool inventoryPath(StringRef path, StringRef role) {
  if (role == "graph") return path == "graph.ac";
  if (role == "source-interface")
    return path.consume_front("sources/") && path.consume_back(".interface.ac") && hexID(path);
  StringRef suffix = role == "spec" ? "/spec.ac" : "/body.ac";
  return (role == "spec" || role == "body") && path.consume_front("definitions/") &&
         path.consume_back(suffix) && hexID(path);
}

FailureOr<json::Object> manifest(const Files &files, ac::detail::EmitError emitError) {
  auto found = files.find("anchor.json");
  if (found == files.end()) return emitError() << "anchor.json is required";
  StringRef text(found->second);
  unsigned depth = 0;
  bool inString = false, escaped = false;
  for (char c : text) {
    if (inString) {
      if (escaped) escaped = false;
      else if (c == '\\') escaped = true;
      else if (c == '"') inString = false;
    } else if (c == '"') inString = true;
    else if (c == '{' || c == '[') {
      if (++depth > 32) return emitError() << "anchor JSON nesting exceeds format limit";
    } else if (c == '}' || c == ']') {
      if (!depth--) return emitError() << "anchor JSON is malformed";
    }
  }
  auto parsed = json::parse(text);
  if (!parsed) { llvm::consumeError(parsed.takeError()); return emitError() << "anchor JSON is malformed"; }
  auto normalized = canonical(*parsed);
  if (failed(normalized) || *normalized + '\n' != text)
    return emitError() << "anchor JSON must be canonical UTF8, without duplicate keys";
  auto object = parsed->getAsObject();
  if (!keys(object, {"kind", "schema", "routed_contract", "owner", "root", "configuration",
                     "configuration_identity", "content_identity", "files", "definitions", "sources", "producer"}) ||
      object->getString("kind") != "pycircuit-anchor" ||
      !((object->getInteger("schema") == 1 && object->getInteger("routed_contract") == 2) ||
        (object->getInteger("schema") == 2 && object->getInteger("routed_contract") == 3) ||
        (object->getInteger("schema") == 3 && object->getInteger("routed_contract") == 4)))
    return emitError() << "unsupported anchor schema/contract; regenerate from source units";
  auto owner = object->getObject("owner");
  auto producer = object->getObject("producer");
  if (!keys(owner, {"kind", "source", "definition"}) || owner->getString("kind") != "anchor" ||
      !ownerShape(owner->getObject("source")) || !owner->getString("definition") ||
      owner->getString("definition") != object->getString("root") ||
      !keys(producer, {"tool", "version", "pipeline"}) || !producer->getString("tool") ||
      !producer->getString("version") || !producer->getString("pipeline") ||
      !hexID(string(*object, "configuration_identity")) || !hexID(string(*object, "content_identity")))
    return emitError() << "anchor owner/producer/identity metadata is malformed";
  json::Object unsignedManifest(*object);
  unsignedManifest.erase("content_identity");
  if (identity(unsignedManifest) != string(*object, "content_identity"))
    return emitError() << "anchor content identity mismatch";
  auto inventory = object->getArray("files");
  if (!inventory || inventory->empty() || inventory->size() + 1 != files.size())
    return emitError() << "anchor tree must exactly match its payload inventory";
  std::string previous;
  for (const auto &raw : *inventory) {
    auto item = raw.getAsObject();
    if (!keys(item, {"path", "role", "sha256"})) return emitError() << "invalid anchor inventory entry";
    auto path = string(*item, "path"), role = string(*item, "role"), digest = string(*item, "sha256");
    auto contents = files.find(path);
    if (!inventoryPath(path, role) || !hexID(digest) || path <= previous ||
        contents == files.end() || anchorSha256(contents->second) != digest)
      return emitError() << "anchor inventory path/order/digest mismatch: " << path;
    previous = path;
  }
  return json::Object(*object);
}

// Parsing fragments has no authority to run owner/binding/layout analyses.
OwningOpRef<mlir::ModuleOp> fragment(StringRef bytes, MLIRContext &context) {
  ParserConfig config(&context, /*verifyAfterParse=*/false);
  auto parsed = parseSourceString<mlir::ModuleOp>(bytes, config);
  if (!parsed || parsed->getOperation()->getNumRegions() != 1 ||
      !parsed->getOperation()->getRegion(0).hasOneBlock())
    return {};
  return parsed;
}

FailureOr<json::Object> configuration(mlir::ModuleOp package, const json::Array &definitions,
                                      StringRef graphHash, ac::detail::EmitError emitError) {
  auto systems = package.getOps<ac::SystemOp>();
  if (!llvm::hasSingleElement(systems)) return emitError() << "anchor requires exactly one System selection";
  auto system = *systems.begin();
  ac::HardwareAnalysis analysis(package);
  auto bindings = ac::detail::resolveHardwareLogicalRootBindings(analysis);
  if (failed(bindings)) return failure();
  auto root = cast<ac::ModuleOp>(bindings->owner);
  auto physical = ac::detail::resolveHardwareRootBindings(analysis);
  if (failed(physical)) return failure();
  auto implementation = cast<ac::ModuleOp>(physical->owner);
  const json::Object *rootRow = nullptr, *implementationRow = nullptr;
  json::Array variants;
  for (const auto &raw : definitions) {
    const auto &row = *raw.getAsObject();
    if (row.getString("definition") == symbol(root.getSymName())) rootRow = &row;
    if (row.getString("definition") == symbol(implementation.getSymName())) implementationRow = &row;
    variants.push_back(json::Object{{"definition", string(row, "definition")},
        {"variant_id", string(row, "variant_id")}, {"spec_sha256", string(row, "spec_sha256")},
        {"body_sha256", string(row, "body_sha256")}});
  }
  if (!rootRow || !implementationRow) return emitError() << "anchor root definition is absent from its index";
  json::Array integers, types;
  for (Attribute raw : root.getParameters()) {
    StringRef name = cast<DictionaryAttr>(raw).getAs<StringAttr>("name").getValue();
    auto value = dyn_cast_or_null<ac::MathIntAttr>(bindings->integers.lookup(name));
    if (!value || !decimal(value.getCanonicalValue())) return emitError() << "invalid resolved anchor integer argument";
    integers.push_back(json::Object{{"name", name}, {"value", value.getCanonicalValue()}});
  }
  for (Attribute raw : root.getTypeParameters()) {
    StringRef name = cast<StringAttr>(raw).getValue();
    auto type = bindings->types.lookup(name);
    if (!type) return emitError() << "unresolved anchor type argument";
    std::string text; llvm::raw_string_ostream out(text); type.print(out);
    types.push_back(json::Object{{"name", name}, {"type", text}});
  }
  auto reset = ac::getHardwareResetSemantics(package);
  if (failed(reset)) return failure();
  json::Object result{{"root", json::Object{{"source", ownerJSON(root.getSourceOwner())},
       {"definition", symbol(root.getSymName())}, {"variant_id", string(*rootRow, "variant_id")}}},
      {"integer_arguments", std::move(integers)}, {"type_arguments", std::move(types)},
      {"domain", system.getDomain()}, {"reset_semantics", *reset == ac::HardwareResetSemantics::Strict ? "strict" : "if_one"},
      {"graph_sha256", graphHash.str()}, {"definitions", std::move(variants)}};
  if (system->hasAttr("ac.entry_boundary"))
    result["implementation_root"] = json::Object{{"source", ownerJSON(implementation.getSourceOwner())},
        {"definition", symbol(implementation.getSymName())}, {"variant_id", string(*implementationRow, "variant_id")}};
  return result;
}

LogicalResult writePrivate(StringRef directory, const Files &files, ac::detail::EmitError emitError) {
  std::error_code error;
  auto root = std::filesystem::path(directory.str());
  auto status = std::filesystem::symlink_status(root, error);
  if (error || !std::filesystem::is_directory(status) ||
      !std::filesystem::is_empty(root, error) || error)
    return emitError() << "anchor producer requires an existing empty private directory";
  for (const auto &[path, contents] : files) {
    auto destination = root / path;
    std::filesystem::create_directories(destination.parent_path(), error);
    if (error) return emitError() << "cannot create private anchor parent directory";
    int descriptor = -1;
    if (llvm::sys::fs::openFileForWrite(destination.string(), descriptor,
        llvm::sys::fs::CD_CreateNew, llvm::sys::fs::OF_None))
      return emitError() << "cannot exclusively create private anchor file";
    llvm::raw_fd_ostream output(descriptor, true);
    output << contents; output.flush();
    if (output.has_error()) return emitError() << "cannot write private anchor file";
  }
  return success();
}
} // namespace

std::string anchorSha256(StringRef bytes) {
  llvm::SHA256 hash; hash.update(bytes);
  auto digest = hash.final();
  static constexpr char digits[] = "0123456789abcdef";
  std::string result;
  for (uint8_t byte : digest) { result += digits[byte >> 4]; result += digits[byte & 15]; }
  return result;
}

LogicalResult writeAnchorArtifacts(
    mlir::ModuleOp routed, ArrayRef<AnchorSourceProvenance> sources,
    StringRef privateDirectory, ac::detail::EmitError emitError) {
  auto contract = routed->getAttrOfType<IntegerAttr>("ac.routed_contract");
  bool transactional = contract && contract.getInt() == 4;
  uint64_t interfaceBytes = 0;
  if (sources.size() >= AnchorFormatLimits::payloadFiles)
    return emitError() << "anchor source count exceeds format limit";
  for (const auto &source : sources) {
    if (source.interfaceText.size() > AnchorFormatLimits::fileBytes ||
        source.interfaceText.size() > AnchorFormatLimits::totalBytes - interfaceBytes)
      return emitError() << "anchor interfaces exceed format byte limits";
    interfaceBytes += source.interfaceText.size();
  }
  if (failed(ac::verifyRoutedPackage(routed))) return failure();
  SmallVector<SourceLinkUnit> units;
  for (const auto &source : sources) units.push_back(source.unit);
  if (!transactional && failed(admitSourceLinkUnits(units, emitError))) return failure();
  auto systems = routed.getOps<ac::SystemOp>();
  if (!llvm::hasSingleElement(systems)) return emitError() << "anchor requires one selected root";
  auto system = *systems.begin();
  std::string implementationRoot = symbol(system.getEntry().getAs<FlatSymbolRefAttr>("callee").getValue());
  ac::HardwareAnalysis rootAnalysis(routed);
  auto logicalBindings = ac::detail::resolveHardwareLogicalRootBindings(rootAnalysis);
  if (failed(logicalBindings)) return failure();
  std::string root = symbol(cast<ac::ModuleOp>(logicalBindings->owner).getSymName());
  bool boundary = system->hasAttr("ac.entry_boundary");
  std::map<std::string, Operation *> declarations;
  for (Operation &operation : routed.getBody()->getOperations())
    if (auto name = operation.getAttrOfType<StringAttr>("sym_name"))
      if (!declarations.emplace(symbol(name.getValue()), &operation).second)
        return emitError() << "duplicate anchor declaration";
  std::set<std::string> closure;
  std::vector<std::string> pending{implementationRoot};
  // Canonical physical leaf declarations are retained for subsequent state
  // refinement, even before logical state has a physical callee reference.
  for (auto imported : routed.getOps<ac::ModuleImportOp>())
    if (imported.getPrimitiveKindAttr()) pending.push_back(symbol(imported.getSymName()));
  if (transactional)
    for (Operation &operation : routed.getBody()->getOperations())
      if (isa<ac::EnumOp, ac::StructOp>(operation))
        pending.push_back(symbol(SymbolTable::getSymbolName(&operation).getValue()));
  while (!pending.empty()) {
    auto name = std::move(pending.back()); pending.pop_back();
    if (!closure.insert(name).second) continue;
    auto found = declarations.find(name);
    if (found == declarations.end()) return emitError() << "missing semantic anchor declaration: " << name;
    for (const auto &dependency : dependencies(found->second)) pending.push_back(dependency);
  }
  Files files;
  json::Array definitions, provenance;
  uint64_t payloadBytes = 0;
  auto add = [&](const std::string &path, std::string text) -> LogicalResult {
    if (text.empty() || text.size() > AnchorFormatLimits::fileBytes ||
        text.size() > AnchorFormatLimits::totalBytes - payloadBytes ||
        files.size() >= AnchorFormatLimits::payloadFiles || !files.emplace(path, std::move(text)).second)
      return emitError() << "anchor payload exceeds format bounds or duplicates a file";
    payloadBytes += files.at(path).size();
    return success();
  };
  OwningOpRef<mlir::ModuleOp> graph(mlir::ModuleOp::create(routed.getLoc()));
  graph->getOperation()->setAttrs(routed->getAttrs());
  // Transactional replay authenticates nominal declarations in source order.
  for (Operation &operation : routed.getBody()->getOperations())
    if (!isa<ac::ModuleOp, ac::SystemOp>(operation) &&
        closure.contains(symbol(SymbolTable::getSymbolName(&operation).getValue())))
      graph->getBody()->push_back(operation.clone());
  for (const auto &name : closure) {
    auto *declaration = declarations.at(name);
    auto definition = dyn_cast<ac::ModuleOp>(declaration);
    if (!definition) continue;
    uint64_t remaining = AnchorFormatLimits::totalBytes - payloadBytes;
    std::string specText = print(spec(definition), std::min(AnchorFormatLimits::fileBytes, remaining));
    std::string bodyText = print(declaration, std::min(AnchorFormatLimits::fileBytes, remaining - specText.size()));
    if (specText.empty() || bodyText.empty()) return emitError() << "anchor definition exceeds per-file limit";
    std::string specHash = anchorSha256(specText), bodyHash = anchorSha256(bodyText);
    json::Object variant{{"source", ownerJSON(definition.getSourceOwner())},
                         {"definition", name}, {"spec_sha256", specHash}, {"body_sha256", bodyHash}};
    std::string id = identity(variant), prefix = "definitions/" + id;
    if (failed(add(prefix + "/spec.ac", std::move(specText))) ||
        failed(add(prefix + "/body.ac", std::move(bodyText)))) return failure();
    definitions.push_back(json::Object{{"definition", name}, {"source", ownerJSON(definition.getSourceOwner())},
      {"variant_id", id}, {"spec", prefix + "/spec.ac"}, {"body", prefix + "/body.ac"},
      {"spec_sha256", specHash}, {"body_sha256", bodyHash}, {"dependencies", dependencyJSON(dependencies(declaration))}});
  }
  graph->getBody()->push_back(system->clone());
  if (failed(add("graph.ac", print(*graph)))) return failure();
  std::map<std::string, const AnchorSourceProvenance *> ordered;
  for (const auto &source : sources) {
    if (transactional) continue; // Linked source provenance is retained by routed replay.
    auto owner = source.unit.header->getAttrOfType<DictionaryAttr>("ac.source_owner");
    auto key = canonicalObject(ownerJSON(owner));
    if (!hexID(source.bodySha256) || source.interfaceText.size() > AnchorFormatLimits::fileBytes ||
        !ordered.emplace(key, &source).second)
      return emitError() << "invalid or duplicate anchor source provenance";
  }
  for (const auto &[key, source] : ordered) {
    auto owner = source->unit.header->getAttrOfType<DictionaryAttr>("ac.source_owner");
    // Provenance bytes must describe the same admitted original interface.
    auto original = fragment(source->interfaceText, *routed.getContext());
    auto originalText = original ? print(*original) : std::string();
    auto admittedText = print(source->unit.header);
    if (!original || originalText.empty() || admittedText.empty() || originalText != admittedText)
      return emitError() << "anchor provenance differs from its admitted interface";
    std::string id = anchorSha256(key), path = "sources/" + id + ".interface.ac";
    std::string digest = anchorSha256(source->interfaceText);
    if (source->interfaceText.size() > AnchorFormatLimits::totalBytes - payloadBytes)
      return emitError() << "anchor interfaces exceed aggregate payload byte limit";
    if (failed(add(path, source->interfaceText))) return failure();
    provenance.push_back(json::Object{{"source", ownerJSON(owner)}, {"owner_id", id},
      {"interface", path}, {"body_sha256", source->bodySha256}, {"interface_sha256", digest}});
  }
  // The pruned package is the exact standalone verification view reconstructed
  // by the loader. Verify it before using existing semantic binders.
  OwningOpRef<mlir::ModuleOp> view(cast<mlir::ModuleOp>(graph->clone()));
  for (const auto &raw : definitions) {
    auto body = fragment(files.at(string(*raw.getAsObject(), "body")), *routed.getContext());
    if (!body || !llvm::hasSingleElement(*body->getBody())) return emitError() << "cannot reconstruct anchor definition";
    Operation *operation = &body->getBody()->front(); operation->remove(); view->getBody()->push_back(operation);
  }
  if (failed(ac::verifyRoutedPackage(*view))) return failure();
  auto config = configuration(*view, definitions, anchorSha256(files.at("graph.ac")), emitError);
  if (failed(config)) return failure();
  auto rootSource = config->getObject("root")->getObject("source");
  json::Array inventory;
  for (const auto &[path, text] : files) {
    std::string role = path == "graph.ac" ? "graph" :
        StringRef(path).starts_with("sources/") ? "source-interface" :
        StringRef(path).ends_with("/spec.ac") ? "spec" : "body";
    inventory.push_back(json::Object{{"path", path}, {"role", role}, {"sha256", anchorSha256(text)}});
  }
  json::Object output{{"kind", "pycircuit-anchor"}, {"schema", transactional ? 3 : boundary ? 2 : 1}, {"routed_contract", transactional ? 4 : boundary ? 3 : 2},
    {"owner", json::Object{{"kind", "anchor"}, {"source", json::Object(*rootSource)}, {"definition", root}}},
    {"root", root}, {"configuration", json::Object(*config)}, {"configuration_identity", identity(*config)},
    {"files", std::move(inventory)}, {"definitions", std::move(definitions)}, {"sources", std::move(provenance)},
    {"producer", json::Object{{"tool", "pycircuit-link"}, {"version", "6.1.0"}, {"pipeline", "source-route-anchor-v1"}}}};
  output["content_identity"] = identity(output);
  std::string text = canonicalObject(output) + '\n';
  if (text.size() > AnchorFormatLimits::manifestBytes) return emitError() << "anchor manifest exceeds format limit";
  files.emplace("anchor.json", std::move(text));
  return writePrivate(privateDirectory, files, emitError);
}

FailureOr<LoadedAnchor> loadVerifiedAnchor(StringRef directory, MLIRContext &context,
                                          ac::detail::EmitError emitError) {
  auto captured = capture(directory, emitError);
  if (failed(captured)) return failure();
  const Files &files = *captured;
  auto parsed = manifest(files, emitError);
  if (failed(parsed)) return failure();
  auto &document = *parsed;
  auto config = document.getObject("configuration");
  bool boundary = document.getInteger("schema") == 2;
  bool transactional = document.getInteger("schema") == 3;
  auto definitions = document.getArray("definitions"), sources = document.getArray("sources");
  if (!definitions || definitions->empty() || !sources || (transactional ? !sources->empty() : sources->empty()) ||
      !(boundary ? keys(config, {"root", "implementation_root", "integer_arguments", "type_arguments", "domain", "reset_semantics", "graph_sha256", "definitions"})
                  : keys(config, {"root", "integer_arguments", "type_arguments", "domain", "reset_semantics", "graph_sha256", "definitions"})) ||
      !keys(config->getObject("root"), {"source", "definition", "variant_id"}) ||
      !ownerShape(config->getObject("root")->getObject("source")) ||
      !config->getObject("root")->getString("definition") ||
      !hexID(string(*config->getObject("root"), "variant_id")) ||
      !config->getArray("integer_arguments") || !config->getArray("type_arguments") ||
      !config->getArray("definitions") || !config->getString("domain") ||
      !hexID(string(*config, "graph_sha256")) ||
      (config->getString("reset_semantics") != "strict" && config->getString("reset_semantics") != "if_one") ||
      identity(*config) != string(document, "configuration_identity"))
    return emitError() << "anchor configuration schema/identity is invalid";
  if (boundary) {
    auto implementation = config->getObject("implementation_root");
    if (!keys(implementation, {"source", "definition", "variant_id"}) ||
        !ownerShape(implementation->getObject("source")) ||
        !implementation->getString("definition") || !hexID(string(*implementation, "variant_id")))
      return emitError() << "anchor implementation_root schema is invalid";
  }
  std::set<std::string> arguments;
  for (const auto &raw : *config->getArray("integer_arguments")) {
    auto row = raw.getAsObject();
    if (!keys(row, {"name", "value"}) || !row->getString("name") || string(*row, "name").empty() ||
        !decimal(string(*row, "value")) || !arguments.insert(string(*row, "name")).second)
      return emitError() << "invalid canonical anchor integer argument";
  }
  if (!config->getArray("type_arguments")->empty()) return emitError() << "saved anchor root type arguments are unsupported";
  std::map<std::string, const json::Object *> index;
  std::set<std::string> variants, usedFiles{"graph.ac"}, sourceOwners;
  std::map<std::string, std::pair<std::string, std::string>> inventory;
  for (const auto &raw : *document.getArray("files")) {
    const auto &item = *raw.getAsObject();
    inventory.emplace(string(item, "path"), std::make_pair(string(item, "role"), string(item, "sha256")));
  }
  auto pinned = [&](const std::string &path, StringRef role, const std::string &hash) {
    auto found = inventory.find(path);
    return found != inventory.end() && found->second.first == role && found->second.second == hash &&
           usedFiles.insert(path).second;
  };
  if (!inventory.contains("graph.ac") || inventory.at("graph.ac").first != "graph" ||
      inventory.at("graph.ac").second != string(*config, "graph_sha256"))
    return emitError() << "anchor graph identity mismatch";
  std::string previous;
  for (const auto &raw : *definitions) {
    auto row = raw.getAsObject();
    if (!keys(row, {"definition", "source", "variant_id", "spec", "body", "spec_sha256", "body_sha256", "dependencies"}) ||
        !ownerShape(row->getObject("source")) || !row->getArray("dependencies"))
      return emitError() << "anchor definition index schema is invalid";
    std::string name = string(*row, "definition"), id = string(*row, "variant_id");
    json::Object variant{{"source", json::Object(*row->getObject("source"))}, {"definition", name},
                         {"spec_sha256", string(*row, "spec_sha256")}, {"body_sha256", string(*row, "body_sha256")}};
    if (name <= previous || !hexID(id) || identity(variant) != id ||
        !hexID(string(*row, "spec_sha256")) || !hexID(string(*row, "body_sha256")) ||
        !variants.insert(id).second || !index.emplace(name, row).second ||
        string(*row, "spec") != "definitions/" + id + "/spec.ac" ||
        string(*row, "body") != "definitions/" + id + "/body.ac" ||
        !pinned(string(*row, "spec"), "spec", string(*row, "spec_sha256")) ||
        !pinned(string(*row, "body"), "body", string(*row, "body_sha256")))
      return emitError() << "anchor definition variant/spec/body identity mismatch";
    previous = name;
    std::string priorDependency;
    for (const auto &dependency : *row->getArray("dependencies")) {
      auto value = dependency.getAsString();
      if (!value || *value <= priorDependency) return emitError() << "anchor dependencies must be sorted unique canonical symbols";
      priorDependency = value->str();
    }
  }
  previous.clear();
  for (const auto &raw : *sources) {
    auto row = raw.getAsObject();
    if (!keys(row, {"source", "owner_id", "interface", "body_sha256", "interface_sha256"}) || !ownerShape(row->getObject("source")))
      return emitError() << "anchor source provenance schema is invalid";
    auto owner = canonicalObject(*row->getObject("source"));
    std::string id = string(*row, "owner_id");
    if (owner <= previous || !sourceOwners.insert(owner).second || !hexID(id) ||
        anchorSha256(owner) != id || !hexID(string(*row, "body_sha256")) ||
        string(*row, "interface") != "sources/" + id + ".interface.ac" ||
        !pinned(string(*row, "interface"), "source-interface", string(*row, "interface_sha256")))
      return emitError() << "anchor source provenance identity mismatch";
    previous = owner;
  }
  if (usedFiles.size() != inventory.size()) return emitError() << "anchor has unreferenced inventory files";
  std::string root = string(document, "root");
  if (!index.contains(root)) return emitError() << "anchor selected root is absent from its definition index";
  const auto &rootRow = *index.at(root);
  auto configRoot = config->getObject("root");
  if (configRoot->getString("definition") != root ||
      configRoot->getString("variant_id") != rootRow.getString("variant_id") ||
      *configRoot->getObject("source") != *rootRow.getObject("source") ||
      *configRoot->getObject("source") != *document.getObject("owner")->getObject("source"))
    return emitError() << "anchor root/configuration/source identity mismatch";
  std::string implementationRoot = root;
  if (boundary) {
    auto implementation = config->getObject("implementation_root");
    implementationRoot = string(*implementation, "definition");
    if (!index.contains(implementationRoot)) return emitError() << "anchor implementation root is absent from its index";
    auto expected = index.at(implementationRoot);
    if (implementation->getString("variant_id") != expected->getString("variant_id") ||
        *implementation->getObject("source") != *expected->getObject("source"))
      return emitError() << "anchor implementation root identity mismatch";
  }
  json::Array expectedVariants;
  for (const auto &raw : *definitions) {
    const auto &row = *raw.getAsObject();
    expectedVariants.push_back(json::Object{{"definition", string(row, "definition")},
      {"variant_id", string(row, "variant_id")}, {"spec_sha256", string(row, "spec_sha256")},
      {"body_sha256", string(row, "body_sha256")}});
  }
  if (expectedVariants != *config->getArray("definitions"))
    return emitError() << "anchor configuration definition index is not exact";
  auto graph = fragment(files.at("graph.ac"), context);
  if (!graph) return emitError() << "cannot parse native anchor graph fragment";
  if (transactional) {
    auto owners = (*graph)->getAttrOfType<ArrayAttr>("ac.source_units");
    if (!owners || owners.empty())
      return emitError() << "transactional anchor requires source owners";
    for (Attribute raw : owners) {
      auto owner = dyn_cast<DictionaryAttr>(raw);
      if (!owner || failed(ac::detail::verifySourceOwner(owner, emitError)) ||
          !sourceOwners.insert(canonicalObject(ownerJSON(owner))).second)
        return emitError() << "transactional anchor source owner inventory is invalid";
    }
  }
  std::map<std::string, Operation *> graphDeclarations;
  for (Operation &operation : graph->getBody()->getOperations()) {
    if (isa<ac::SystemOp>(operation)) continue;
    if (!isa<ac::StructOp, ac::EnumOp, ac::ModuleImportOp>(operation))
      return emitError() << "anchor graph fragment cannot contain executable module bodies";
    auto name = operation.getAttrOfType<StringAttr>("sym_name");
    if (!name || index.contains(symbol(name.getValue())) ||
        !graphDeclarations.emplace(symbol(name.getValue()), &operation).second)
      return emitError() << "duplicate/conflicting anchor graph declaration";
  }
  std::set<std::string> loaded, neededGraph;
  std::vector<std::string> pending{implementationRoot};
  for (const auto &[name, declaration] : graphDeclarations)
    if (auto imported = dyn_cast<ac::ModuleImportOp>(declaration);
        imported && imported->getAttrOfType<StringAttr>("primitive_kind"))
      pending.push_back(name);
  if (transactional)
    for (const auto &[name, declaration] : graphDeclarations)
      if (isa<ac::EnumOp, ac::StructOp>(declaration))
        pending.push_back(name);
  while (!pending.empty()) {
    auto name = std::move(pending.back()); pending.pop_back();
    if (loaded.contains(name) || neededGraph.contains(name)) continue;
    auto found = index.find(name);
    if (found == index.end()) {
      auto graphDeclaration = graphDeclarations.find(name);
      if (graphDeclaration == graphDeclarations.end()) return emitError() << "missing semantic anchor provider: " << name;
      neededGraph.insert(name);
      for (const auto &dependency : dependencies(graphDeclaration->second)) pending.push_back(dependency);
      continue;
    }
    const auto &row = *found->second;
    auto expectedName = symbolName(name, context);
    auto body = fragment(files.at(string(row, "body")), context);
    auto specification = dyn_cast_or_null<DictionaryAttr>(parseAttribute(files.at(string(row, "spec")), &context));
    if (failed(expectedName) || !body || !llvm::hasSingleElement(*body->getBody()) ||
        !isa<ac::ModuleOp>(body->getBody()->front()) || !specification || specification.size() != 2 ||
        !specification.getAs<DictionaryAttr>("properties") || !specification.getAs<DictionaryAttr>("attributes"))
      return emitError() << "invalid native anchor body/spec fragment";
    auto definition = cast<ac::ModuleOp>(body->getBody()->front());
    auto actualName = definition->getAttrOfType<StringAttr>("sym_name");
    auto owner = definition->getAttrOfType<DictionaryAttr>("source_owner");
    if (!actualName || actualName.getValue() != *expectedName || !owner ||
        failed(ac::detail::verifySourceOwner(owner, emitError)) ||
        ownerJSON(owner) != *row.getObject("source") || !sourceOwners.contains(canonicalObject(ownerJSON(owner))) ||
        definition->getPropertiesAsAttribute() != specification.get("properties") ||
        definition->getDiscardableAttrDictionary() != specification.get("attributes"))
      return emitError() << "anchor definition symbol/owner/exact saved spec mismatch";
    auto actualDependencies = dependencies(definition);
    if (dependencyJSON(actualDependencies) != *row.getArray("dependencies"))
      return emitError() << "anchor actual semantic dependencies differ from its index";
    loaded.insert(name);
    for (const auto &dependency : actualDependencies) pending.push_back(dependency);
    definition->remove(); graph->getBody()->push_back(definition);
  }
  if (loaded.size() != index.size() || neededGraph.size() != graphDeclarations.size())
    return emitError() << "anchor definition index/graph is not the selected root's exact semantic closure";
  // Do not call a binder, packed-layout evaluator or configuration analysis
  // until the complete assembled package has passed bounded native admission.
  if (failed(ac::verifyRoutedPackage(*graph))) return failure();
  auto contract = (*graph)->getAttrOfType<IntegerAttr>("ac.routed_contract");
  if (!contract || contract.getInt() != (transactional ? 4 : boundary ? 3 : 2))
    return emitError() << "anchor schema and native routed contract differ";

  for (const auto &raw : *sources) {
    const auto &row = *raw.getAsObject();
    auto interface = fragment(files.at(string(row, "interface")), context);
    if (!interface) return emitError() << "invalid retained anchor source interface";
    auto structure = ac::verifySourceInterfaceStructure(*interface, emitError);
    if (failed(structure) || ownerJSON(structure->owner) != *row.getObject("source"))
      return emitError() << "anchor retained source interface owner mismatch";
  }
  auto actual = configuration(*graph, *definitions, string(*config, "graph_sha256"), emitError);
  if (failed(actual) || *actual != *config ||
      config->getObject("root")->getString("definition") != root ||
      *config->getObject("root")->getObject("source") != *document.getObject("owner")->getObject("source"))
    return emitError() << "anchor saved configuration differs from verified native bindings";
  auto fingerprint = print(*graph, AnchorFormatLimits::totalBytes);
  if (fingerprint.empty()) return emitError() << "assembled anchor exceeds format bounds";
  LoadedAnchor result;
  result.package = std::move(graph);
  result.root = root;
  result.sourceOwner = ownerAttr(*document.getObject("owner")->getObject("source"), context);
  result.contentIdentity = string(document, "content_identity");
  result.configurationIdentity = string(document, "configuration_identity");
  result.verifiedPackageSha256 = anchorSha256(fingerprint);
  return result;
}

FailureOr<OwningOpRef<mlir::ModuleOp>> refineVerifiedAnchor(
    const LoadedAnchor &anchor, ac::detail::EmitError emitError) {
  if (!anchor.package || !hexID(anchor.contentIdentity) || !hexID(anchor.configurationIdentity) ||
      failed(ac::verifyRoutedPackage(*anchor.package))) return failure();
  auto fingerprint = print(*anchor.package, AnchorFormatLimits::totalBytes);
  if (fingerprint.empty() || anchorSha256(fingerprint) != anchor.verifiedPackageSha256)
    return emitError() << "loaded anchor changed after native verification";
  ac::HardwareSourceCheckLimits limits;
  if (ac::detail::findUnloweredTransaction((*anchor.package).getOperation())) limits.maxWorkUnits = 1024 * 1024;
  ac::detail::SourceCheckBudget budget(limits);
  auto contract = (*anchor.package)->getAttrOfType<IntegerAttr>("ac.routed_contract");
  FailureOr<OwningOpRef<mlir::ModuleOp>> candidate = failure();
  if (contract && contract.getInt() == 4) {
    candidate = OwningOpRef<mlir::ModuleOp>(cast<mlir::ModuleOp>((*anchor.package)->clone()));
    PassManager lowering((*anchor.package).getContext());
    lowering.addPass(acir::createLowerTransactionsPass());
    if (failed(lowering.run(**candidate))) return failure();
  } else {
    candidate = acir::detail::materializeHardwareState(*anchor.package, budget);
    if (failed(candidate) || failed(acir::detail::verifyHardwareStateMaterialization(
            *anchor.package, **candidate, budget))) return failure();
  }
  Builder builder((*anchor.package).getContext());
  (**candidate)->setAttr("ac.anchor_provenance", builder.getDictionaryAttr({
      builder.getNamedAttr("content_identity", builder.getStringAttr(anchor.contentIdentity)),
      builder.getNamedAttr("configuration_identity", builder.getStringAttr(anchor.configurationIdentity))}));
  if (failed(ac::verifyHardwarePackage(**candidate))) return failure();
  return std::move(*candidate);
}

} // namespace acir::compiler
