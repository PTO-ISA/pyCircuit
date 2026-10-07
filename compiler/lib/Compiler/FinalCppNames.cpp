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
  bool simple = !isAsciiDigit(source.front());
  for (char byte : source)
    simple &= isAsciiAlpha(byte) || isAsciiDigit(byte) || byte == '_';
  simple &= !source.starts_with("pyc_") && !source.starts_with("__") &&
            !(source.size() > 1 && source[0] == '_' && source[1] >= 'A' &&
              source[1] <= 'Z');
  static const llvm::StringSet<> reserved = [] {
    llvm::StringSet<> names;
    for (StringRef name :
         {"Work",        "Xfer",      "Reset",      "DiscardNext",
          "Build",       "HasWork",   "ReportStat", "name",
          "input",       "output",    "wire",       "logic",
          "always",      "assign",    "end",        "begin",
          "initial",     "parameter", "localparam", "function",
          "endfunction", "task",      "endtask",    "package",
          "endpackage"})
      names.insert(name);
    // Use one injective spelling across C++ and SystemVerilog. In particular,
    // primitive strength/net and assertion keywords are reserved even when a
    // design uses none of those constructs.
    SmallVector<StringRef> verilog;
    StringRef(
        "accept_on alias always_comb always_ff always_latch assert assume "
        "automatic before bind bins binsof bit break buf bufif0 bufif1 byte "
        "case casex casez cell chandle checker class clocking cmos config "
        "const constraint context continue cover covergroup coverpoint cross "
        "deassign default defparam design disable dist do edge else endcase "
        "endchecker endclass endclocking endconfig endgenerate endgroup "
        "endinterface endmodule endprimitive endprogram endproperty "
        "endsequence "
        "endspecify endtable enum event eventually expect export extends "
        "extern "
        "final first_match for force foreach forever fork forkjoin genvar "
        "generate global highz0 highz1 if iff ifnone ignore_bins illegal_bins "
        "implements implies import incdir include inside instance int integer "
        "interconnect interface intersect join join_any join_none large let "
        "liblist library local localparam longint macromodule matches medium "
        "modport nand negedge nettype new nexttime nmos nor noshowcancelled "
        "not notif0 notif1 null or packed pmos posedge primitive priority "
        "program property protected pull0 pull1 pulldown pullup "
        "pulsestyle_onevent "
        "pulsestyle_ondetect pure rand randc randcase randsequence rcmos real "
        "realtime ref reg reject_on release repeat restrict return rnmos rpmos "
        "rtran rtranif0 rtranif1 s_always s_eventually s_nexttime s_until "
        "s_until_with scalared sequence shortint shortreal showcancelled "
        "signed "
        "small soft solve specify specparam static string strong strong0 "
        "strong1 "
        "struct super supply0 supply1 sync_accept_on sync_reject_on table "
        "tagged "
        "this throughout time timeprecision timeunit tran tranif0 tranif1 tri "
        "tri0 tri1 triand trior trireg type typedef union unique unique0 "
        "unsigned "
        "until until_with untyped use uwire var vectored virtual void wait "
        "wait_order wand weak weak0 weak1 while wildcard with within wor xnor "
        "xor")
        .split(verilog, ' ');
    for (StringRef keyword : verilog)
      names.insert(keyword);
    return names;
  }();
  if (simple && !isCppKeyword(source) && !reserved.contains(source))
    return source.str();
  static constexpr char hex[] = "0123456789abcdef";
  std::string result = "pyc_";
  for (unsigned char byte : source.bytes()) {
    result.push_back(hex[byte >> 4]);
    result.push_back(hex[byte & 15]);
  }
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
  (void)moduleName;
  StringRef leaf = definition.getValue().rsplit('.').second;
  if (leaf.empty())
    return emitError() << "hardware definition has an empty class name";
  return legalizeIdentifier(leaf, emitError);
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
  if (components.empty())
    return emitError() << "source path has no components";
  for (StringRef component : components) {
    if (component.empty() || component == "." || component == ".." ||
        component.contains('/') || component.contains('\\') ||
        component.contains('\0'))
      return emitError() << "source path has an invalid component";
  }
  return (Twine("sources/") + join(components, "/") + extension).str();
}

} // namespace acir::compiler
