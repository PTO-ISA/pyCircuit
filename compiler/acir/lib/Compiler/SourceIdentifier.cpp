#include "SourceIdentifier.h"

#include "SourceIdentifierUnicodeData.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

using namespace llvm;
using namespace mlir;

namespace acir::compiler::detail {
namespace {

using namespace unicode_data;

bool decodeUtf8(StringRef input, std::vector<uint32_t> &codepoints) {
  size_t offset = 0;
  while (offset < input.size()) {
    uint8_t lead = static_cast<uint8_t>(input[offset++]);
    uint32_t value = 0;
    size_t continuationCount = 0;
    if (lead < 0x80) {
      value = lead;
    } else if (lead >= 0xC2 && lead <= 0xDF) {
      value = lead & 0x1F;
      continuationCount = 1;
    } else if (lead >= 0xE0 && lead <= 0xEF) {
      value = lead & 0x0F;
      continuationCount = 2;
    } else if (lead >= 0xF0 && lead <= 0xF4) {
      value = lead & 0x07;
      continuationCount = 3;
    } else {
      return false;
    }
    if (offset + continuationCount > input.size())
      return false;
    for (size_t index = 0; index < continuationCount; ++index) {
      uint8_t next = static_cast<uint8_t>(input[offset++]);
      if ((next & 0xC0) != 0x80)
        return false;
      value = (value << 6) | (next & 0x3F);
    }
    if ((continuationCount == 1 && value < 0x80) ||
        (continuationCount == 2 && value < 0x800) ||
        (continuationCount == 3 && value < 0x10000) || value > 0x10FFFF ||
        (value >= 0xD800 && value <= 0xDFFF))
      return false;
    codepoints.push_back(value);
  }
  return true;
}

template <size_t Size>
bool containsCodepoint(const Range (&table)[Size], uint32_t codepoint) {
  auto found = std::lower_bound(
      std::begin(table), std::end(table), codepoint,
      [](const Range &range, uint32_t value) { return range.last < value; });
  return found != std::end(table) && found->first <= codepoint;
}

uint8_t combiningClass(uint32_t codepoint) {
  auto found = std::lower_bound(
      std::begin(combiningClasses), std::end(combiningClasses), codepoint,
      [](const CombiningClass &entry, uint32_t value) {
        return entry.codepoint < value;
      });
  if (found == std::end(combiningClasses) || found->codepoint != codepoint)
    return 0;
  return found->value;
}

const Decomposition *decomposition(uint32_t codepoint) {
  auto found = std::lower_bound(
      std::begin(decompositions), std::end(decompositions), codepoint,
      [](const Decomposition &entry, uint32_t value) {
        return entry.codepoint < value;
      });
  if (found == std::end(decompositions) || found->codepoint != codepoint)
    return nullptr;
  return found;
}

uint32_t composePair(uint32_t first, uint32_t second) {
  constexpr uint32_t HangulSBase = 0xAC00;
  constexpr uint32_t HangulLBase = 0x1100;
  constexpr uint32_t HangulVBase = 0x1161;
  constexpr uint32_t HangulTBase = 0x11A7;
  constexpr uint32_t HangulLCount = 19;
  constexpr uint32_t HangulVCount = 21;
  constexpr uint32_t HangulTCount = 28;
  constexpr uint32_t HangulNCount = HangulVCount * HangulTCount;
  constexpr uint32_t HangulSCount = HangulLCount * HangulNCount;

  if (first >= HangulLBase && first < HangulLBase + HangulLCount &&
      second >= HangulVBase && second < HangulVBase + HangulVCount)
    return HangulSBase +
           ((first - HangulLBase) * HangulVCount + (second - HangulVBase)) *
               HangulTCount;
  if (first >= HangulSBase && first < HangulSBase + HangulSCount &&
      (first - HangulSBase) % HangulTCount == 0 && second > HangulTBase &&
      second < HangulTBase + HangulTCount)
    return first + second - HangulTBase;

  auto found = std::lower_bound(
      std::begin(compositions), std::end(compositions),
      std::pair<uint32_t, uint32_t>{first, second},
      [](const Composition &entry, const std::pair<uint32_t, uint32_t> &key) {
        return entry.first < key.first ||
               (entry.first == key.first && entry.second < key.second);
      });
  if (found == std::end(compositions) || found->first != first ||
      found->second != second)
    return 0;
  return found->result;
}

void appendDecomposed(uint32_t codepoint, std::vector<uint32_t> &output) {
  constexpr uint32_t HangulSBase = 0xAC00;
  constexpr uint32_t HangulLBase = 0x1100;
  constexpr uint32_t HangulVBase = 0x1161;
  constexpr uint32_t HangulTBase = 0x11A7;
  constexpr uint32_t HangulVCount = 21;
  constexpr uint32_t HangulTCount = 28;
  constexpr uint32_t HangulNCount = HangulVCount * HangulTCount;
  constexpr uint32_t HangulSCount = 19 * HangulNCount;

  std::vector<uint32_t> pending{codepoint};
  while (!pending.empty()) {
    uint32_t current = pending.back();
    pending.pop_back();
    if (current >= HangulSBase && current < HangulSBase + HangulSCount) {
      uint32_t index = current - HangulSBase;
      uint32_t trailing = index % HangulTCount;
      if (trailing)
        pending.push_back(HangulTBase + trailing);
      pending.push_back(HangulVBase + (index % HangulNCount) / HangulTCount);
      pending.push_back(HangulLBase + index / HangulNCount);
      continue;
    }
    const Decomposition *mapping = decomposition(current);
    if (!mapping) {
      output.push_back(current);
      continue;
    }
    for (size_t index = mapping->length; index > 0; --index)
      pending.push_back(decompositionValues[mapping->offset + index - 1]);
  }
}

std::vector<uint32_t> nfkc(const std::vector<uint32_t> &input) {
  std::vector<uint32_t> decomposed;
  for (uint32_t codepoint : input)
    appendDecomposed(codepoint, decomposed);

  size_t segment = 0;
  while (segment < decomposed.size()) {
    size_t marks = segment + (combiningClass(decomposed[segment]) == 0);
    size_t end = marks;
    while (end < decomposed.size() && combiningClass(decomposed[end]) != 0)
      ++end;
    std::stable_sort(decomposed.begin() + marks, decomposed.begin() + end,
                     [](uint32_t left, uint32_t right) {
                       return combiningClass(left) < combiningClass(right);
                     });
    segment = end;
  }

  std::vector<uint32_t> composed;
  composed.reserve(decomposed.size());
  size_t starterPosition = static_cast<size_t>(-1);
  uint8_t lastClass = 0;
  for (uint32_t codepoint : decomposed) {
    uint8_t currentClass = combiningClass(codepoint);
    if (starterPosition != static_cast<size_t>(-1) &&
        (lastClass == 0 || lastClass < currentClass)) {
      uint32_t composite = composePair(composed[starterPosition], codepoint);
      if (composite) {
        composed[starterPosition] = composite;
        continue;
      }
    }
    if (currentClass == 0)
      starterPosition = composed.size();
    composed.push_back(codepoint);
    lastClass = currentClass;
  }
  return composed;
}

} // namespace

LogicalResult verifyPythonAstIdentifier(StringRef name, bool bindingName,
                                       ac::detail::EmitError emitError) {
  std::vector<uint32_t> codepoints;
  codepoints.reserve(name.size());
  if (name.empty() || !decodeUtf8(name, codepoints))
    return emitError() << "Python AST identifier must be nonempty valid UTF-8";
  if (!containsCodepoint(xidStartRanges, codepoints.front()))
    return emitError() << "Python AST identifier does not start with XID_Start";
  for (size_t index = 1; index < codepoints.size(); ++index) {
    if (!containsCodepoint(xidContinueRanges, codepoints[index]))
      return emitError() << "Python AST identifier contains a non-XID_Continue character";
  }
  if (nfkc(codepoints) != codepoints)
    return emitError() << "Python AST identifier is not in Unicode NFKC form";

  StringRef normalized = name;
  if (normalized == "None" || normalized == "True" || normalized == "False")
    return emitError() << "Python AST identifier cannot be a constant keyword";
  if (bindingName && normalized == "__debug__")
    return emitError() << "Python AST binding cannot use '__debug__'";
  return success();
}

} // namespace acir::compiler::detail
