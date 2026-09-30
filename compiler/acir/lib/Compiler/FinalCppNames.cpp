#include "FinalCppNames.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/Twine.h"

#include <cstdint>

using namespace mlir;

namespace acir::compiler {

bool isAsciiAlpha(char value) {
  return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z');
}

bool isAsciiDigit(char value) { return value >= '0' && value <= '9'; }

bool isCppKeyword(StringRef value) {
  static const llvm::StringSet<> keywords = [] {
    llvm::StringSet<> result;
    SmallVector<StringRef> spellings;
    StringRef(
        "alignas alignof and and_eq asm atomic_cancel atomic_commit "
        "atomic_noexcept auto bitand bitor bool break case catch char char8_t "
        "char16_t char32_t class compl concept const consteval constexpr "
        "constinit const_cast continue co_await co_return co_yield decltype "
        "default delete do double dynamic_cast else enum explicit export "
        "extern false float for friend goto if import inline int long module "
        "mutable namespace new noexcept not not_eq nullptr operator or or_eq "
        "private protected public reflexpr register reinterpret_cast requires "
        "return short signed sizeof static static_assert static_cast struct "
        "switch synchronized template this thread_local throw true try "
        "typedef typeid typename union unsigned using virtual void volatile "
        "wchar_t while xor xor_eq")
        .split(spellings, ' ');
    for (StringRef keyword : spellings)
      result.insert(keyword);
    return result;
  }();
  return keywords.contains(value);
}

FailureOr<uint32_t> readCodePoint(StringRef text, size_t &offset,
                                  ac::detail::EmitError emitError) {
  const auto *bytes = reinterpret_cast<const uint8_t *>(text.data());
  uint8_t first = bytes[offset];
  unsigned length = first < 0x80             ? 1
                    : (first & 0xe0) == 0xc0 ? 2
                    : (first & 0xf0) == 0xe0 ? 3
                    : (first & 0xf8) == 0xf0 ? 4
                                             : 0;
  if (!length || offset + length > text.size())
    return emitError() << "source name contains invalid UTF-8";
  uint32_t value = first & (length == 1   ? 0x7f
                            : length == 2 ? 0x1f
                            : length == 3 ? 0x0f
                                          : 0x07);
  for (unsigned index = 1; index < length; ++index) {
    uint8_t byte = bytes[offset + index];
    if ((byte & 0xc0) != 0x80)
      return emitError() << "source name contains invalid UTF-8";
    value = (value << 6) | (byte & 0x3f);
  }
  if ((length == 2 && value < 0x80) || (length == 3 && value < 0x800) ||
      (length == 4 && value < 0x10000) || value > 0x10ffff ||
      (value >= 0xd800 && value <= 0xdfff))
    return emitError() << "source name contains invalid UTF-8";
  offset += length;
  return value;
}

FailureOr<std::string> legalizeIdentifier(StringRef source,
                                          ac::detail::EmitError emitError) {
  if (source.empty())
    return emitError() << "C++ source identifier is empty";
  static constexpr char hex[] = "0123456789abcdef";
  std::string result;
  bool pendingSeparator = false;
  bool previousLowerOrDigit = false;
  size_t offset = 0;
  while (offset < source.size()) {
    unsigned char byte = static_cast<unsigned char>(source[offset]);
    if (byte >= 0x80) {
      auto codePoint = readCodePoint(source, offset, emitError);
      if (failed(codePoint))
        return failure();
      if (!result.empty())
        result.push_back('_');
      std::string token = "u";
      uint32_t value = *codePoint;
      SmallVector<char, 8> digits;
      do {
        digits.push_back(hex[value & 0xf]);
        value >>= 4;
      } while (value);
      for (auto it = digits.rbegin(); it != digits.rend(); ++it)
        token.push_back(*it);
      result.append(token);
      pendingSeparator = true;
      previousLowerOrDigit = false;
      continue;
    }

    char value = static_cast<char>(byte);
    ++offset;
    if (!isAsciiAlpha(value) && !isAsciiDigit(value)) {
      pendingSeparator = true;
      previousLowerOrDigit = false;
      continue;
    }
    bool uppercase = value >= 'A' && value <= 'Z';
    bool originalLowerOrDigit =
        (value >= 'a' && value <= 'z') || isAsciiDigit(value);
    if (uppercase && previousLowerOrDigit)
      pendingSeparator = true;
    if (pendingSeparator && !result.empty() && result.back() != '_')
      result.push_back('_');
    pendingSeparator = false;
    if (uppercase)
      value = static_cast<char>(value - 'A' + 'a');
    result.push_back(value);
    previousLowerOrDigit = originalLowerOrDigit;
  }
  while (!result.empty() && result.back() == '_')
    result.pop_back();
  if (result.empty())
    return emitError() << "source name does not form a C++ identifier";
  if (isAsciiDigit(result.front()))
    result.insert(0, "pyc_");
  if (isCppKeyword(result))
    result.insert(0, "pyc_");
  return result;
}

FailureOr<SmallVector<std::string>>
splitComponents(StringRef value, char separator,
                ac::detail::EmitError emitError) {
  SmallVector<std::string> result;
  if (value.empty())
    return result;
  SmallVector<StringRef> parts;
  value.split(parts, separator, /*MaxSplit=*/-1, /*KeepEmpty=*/true);
  for (StringRef part : parts) {
    if (part.empty())
      return emitError() << "SourceOwner contains an empty qualified component";
    result.push_back(part.str());
  }
  return result;
}

std::string join(ArrayRef<std::string> parts, StringRef separator) {
  std::string result;
  for (StringRef part : parts) {
    if (!result.empty())
      result.append(separator);
    result.append(part);
  }
  return result;
}

FailureOr<CppOwnerComponents>
sourceOwnerComponents(DictionaryAttr owner, ac::detail::EmitError emitError) {
  if (failed(ac::detail::verifySourceOwner(owner, emitError)))
    return failure();
  auto packageAttr = owner.getAs<StringAttr>("package");
  auto pathAttr = owner.getAs<StringAttr>("path");
  StringRef path = pathAttr.getValue();
  if (!path.ends_with(".py"))
    return emitError() << "C++ source group owner path must end in .py";

  CppOwnerComponents result;
  auto packageParts = splitComponents(packageAttr.getValue(), '.', emitError);
  if (failed(packageParts))
    return failure();
  auto fileParts = splitComponents(path.drop_back(3), '/', emitError);
  if (failed(fileParts))
    return failure();
  SmallVector<std::string> moduleParts = *fileParts;
  if (!moduleParts.empty() && moduleParts.back() == "__init__")
    moduleParts.pop_back();

  result.namespaces.append(packageParts->begin(), packageParts->end());
  result.namespaces.append(moduleParts.begin(), moduleParts.end());
  result.filePath.append(packageParts->begin(), packageParts->end());
  result.filePath.append(fileParts->begin(), fileParts->end());
  if (result.filePath.empty())
    result.filePath.push_back("__init__");
  result.moduleName = join(result.namespaces, ".");
  return result;
}

FailureOr<std::string> sourceDefinitionName(FlatSymbolRefAttr definition,
                                            StringRef moduleName,
                                            ac::detail::EmitError emitError) {
  if (!definition)
    return emitError() << "C++ source group has no qualified definition";
  StringRef symbol = definition.getValue();
  StringRef leaf = symbol;
  if (!moduleName.empty()) {
    std::string prefix = (Twine(moduleName) + ".").str();
    if (!symbol.starts_with(prefix))
      return emitError()
             << "C++ definition does not belong to its module SourceOwner";
    leaf = symbol.drop_front(prefix.size());
  } else if (symbol.contains('.')) {
    return emitError()
           << "unpackaged C++ definition has a qualified module prefix";
  }
  if (leaf.empty() || leaf.contains('.'))
    return emitError() << "C++ definition has an invalid source class name";
  return leaf.str();
}

FailureOr<std::string> namespaceCppName(ArrayRef<std::string> components,
                                        ac::detail::EmitError emitError) {
  SmallVector<std::string> legalized;
  for (StringRef component : components) {
    auto name = legalizeIdentifier(component, emitError);
    if (failed(name))
      return failure();
    legalized.push_back(std::move(*name));
  }
  return join(legalized, "::");
}

FailureOr<std::string> sourcePathName(ArrayRef<std::string> components,
                                      StringRef extension,
                                      ac::detail::EmitError emitError) {
  SmallVector<std::string> legalized;
  for (StringRef component : components) {
    auto name = legalizeIdentifier(component, emitError);
    if (failed(name))
      return failure();
    legalized.push_back(std::move(*name));
  }
  return (Twine("sources/") + join(legalized, "/") + extension).str();
}

} // namespace acir::compiler
