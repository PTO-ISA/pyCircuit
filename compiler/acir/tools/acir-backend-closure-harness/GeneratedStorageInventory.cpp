#include "GeneratedStorageInventory.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include <limits>

namespace {
struct Family {
  uint64_t registers = 0;
  llvm::SmallVector<std::string> children;
};
} // namespace
mlir::FailureOr<uint64_t>
generatedStorageInventory(llvm::StringRef text, bool cpp,
                          acir::ac::detail::EmitError error) {
  llvm::StringMap<Family> families;
  llvm::StringRef remainder = text;
  const llvm::StringRef start = cpp ? "class FinalModuleDef" : "module ";
  const llvm::StringRef end = cpp ? "\n};" : "endmodule";
  while (true) {
    size_t position = remainder.find(start);
    if (position == llvm::StringRef::npos)
      break;
    remainder = remainder.drop_front(position);
    auto firstLine = remainder.take_front(remainder.find('\n'));
    if (cpp && !firstLine.contains(" final : public gfsim::SimModule {") &&
        !firstLine.contains(" final : public ::gfsim::SimModule {")) {
      remainder = remainder.drop_front(start.size());
      continue;
    }
    llvm::StringRef afterKeyword = remainder.drop_front(cpp ? 6 : 7);
    auto name = afterKeyword.take_front(afterKeyword.find_first_of(" (\n"));
    size_t close = remainder.find(end);
    if (name.empty() || close == llvm::StringRef::npos ||
        families.contains(name))
      return error() << "generated storage family is malformed or repeated";
    auto body = remainder.take_front(close);
    Family family;
    family.registers =
        cpp ? body.count("gfsim::SimDFFE<") + body.count("gfsim::SimDFF<")
            : body.count("always_ff @(posedge clk)");
    llvm::SmallVector<llvm::StringRef> lines;
    body.split(lines, '\n');
    for (auto line : lines) {
      line = line.trim();
      auto [type, tail] = line.split(' ');
      tail = tail.ltrim();
      bool rtlChild = false;
      if (!cpp && tail.starts_with("child_")) {
        auto suffix = tail.drop_front(6);
        auto ordinal =
            suffix.take_while([](char c) { return c >= '0' && c <= '9'; });
        rtlChild = !ordinal.empty() &&
                   suffix.drop_front(ordinal.size()).starts_with("(");
      }
      if ((cpp && type.starts_with("FinalModuleDef") &&
           tail.starts_with("child_")) ||
          (!cpp && (rtlChild || tail.starts_with("root_("))))
        family.children.push_back(type.str());
    }
    families.try_emplace(name, std::move(family));
    remainder = remainder.drop_front(close + end.size());
  }
  std::string root = "FinalModel";
  if (cpp) {
    constexpr llvm::StringLiteral alias = "using FinalModel = ";
    size_t position = text.find(alias);
    if (position == llvm::StringRef::npos)
      return error() << "generated C++ storage has no root alias";
    auto tail = text.drop_front(position + alias.size());
    root = tail.take_front(tail.find(';')).trim().str();
  }
  llvm::StringSet<> active;
  auto expand = [&](auto &&self,
                    llvm::StringRef name) -> mlir::FailureOr<uint64_t> {
    auto family = families.find(name);
    if (family == families.end() || !active.insert(name).second)
      return error() << "generated storage hierarchy is missing or recursive";
    uint64_t total = family->second.registers;
    for (const auto &child : family->second.children) {
      auto count = self(self, child);
      if (mlir::failed(count))
        return mlir::failure();
      if (*count > std::numeric_limits<uint64_t>::max() - total)
        return error() << "generated storage count overflow";
      total += *count;
    }
    active.erase(name);
    return total;
  };
  return expand(expand, root);
}
