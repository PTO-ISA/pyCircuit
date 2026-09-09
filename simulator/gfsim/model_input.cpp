#include "gfsim/model_input.h"

#include <charconv>
#include <cstdint>
#include <limits>
#include <sstream>

namespace gfsim {
namespace {

bool isValidUtf8(std::string_view input) {
  for (size_t index = 0; index < input.size();) {
    const unsigned char lead = input[index++];
    if (lead < 0x80)
      continue;
    unsigned continuationCount = 0;
    uint32_t codePoint = 0;
    uint32_t minimum = 0;
    if ((lead & 0xe0) == 0xc0) {
      continuationCount = 1;
      codePoint = lead & 0x1f;
      minimum = 0x80;
    } else if ((lead & 0xf0) == 0xe0) {
      continuationCount = 2;
      codePoint = lead & 0x0f;
      minimum = 0x800;
    } else if ((lead & 0xf8) == 0xf0) {
      continuationCount = 3;
      codePoint = lead & 0x07;
      minimum = 0x10000;
    } else {
      return false;
    }
    if (index + continuationCount > input.size())
      return false;
    for (unsigned count = 0; count < continuationCount; ++count) {
      const unsigned char continuation = input[index++];
      if ((continuation & 0xc0) != 0x80)
        return false;
      codePoint = (codePoint << 6) | (continuation & 0x3f);
    }
    if (codePoint < minimum || codePoint > 0x10ffff ||
        (codePoint >= 0xd800 && codePoint <= 0xdfff))
      return false;
  }
  return true;
}

class Parser {
public:
  explicit Parser(std::string_view input) : input_(input) {}

  bool take(std::string_view token) {
    if (!input_.substr(cursor_).starts_with(token))
      return false;
    cursor_ += token.size();
    return true;
  }

  bool finish() const { return cursor_ == input_.size(); }

  bool null() { return take("null"); }

  bool unsignedInteger(uint64_t &result) {
    const size_t start = cursor_;
    while (cursor_ < input_.size() && input_[cursor_] >= '0' &&
           input_[cursor_] <= '9')
      ++cursor_;
    if (start == cursor_ ||
        (cursor_ - start > 1 && input_[start] == '0'))
      return false;
    const char *begin = input_.data() + start;
    const char *end = input_.data() + cursor_;
    auto [parsed, error] = std::from_chars(begin, end, result);
    return error == std::errc{} && parsed == end;
  }

  bool string(std::string &result) {
    if (!take("\""))
      return false;
    result.clear();
    while (cursor_ < input_.size()) {
      const unsigned char value = input_[cursor_++];
      if (value == '"')
        return true;
      if (value < 0x20)
        return false;
      if (value != '\\') {
        result.push_back(static_cast<char>(value));
        continue;
      }
      if (cursor_ == input_.size())
        return false;
      switch (input_[cursor_++]) {
      case '"':
        result.push_back('"');
        break;
      case '\\':
        result.push_back('\\');
        break;
      case '/':
        result.push_back('/');
        break;
      case 'b':
        result.push_back('\b');
        break;
      case 'f':
        result.push_back('\f');
        break;
      case 'n':
        result.push_back('\n');
        break;
      case 'r':
        result.push_back('\r');
        break;
      case 't':
        result.push_back('\t');
        break;
      default:
        return false;
      }
    }
    return false;
  }

private:
  std::string_view input_;
  size_t cursor_ = 0;
};

std::string quote(std::string_view value) {
  std::ostringstream output;
  output << '"';
  for (const unsigned char character : value) {
    switch (character) {
    case '"':
      output << "\\\"";
      break;
    case '\\':
      output << "\\\\";
      break;
    case '\b':
      output << "\\b";
      break;
    case '\f':
      output << "\\f";
      break;
    case '\n':
      output << "\\n";
      break;
    case '\r':
      output << "\\r";
      break;
    case '\t':
      output << "\\t";
      break;
    default:
      if (character < 0x20)
        return {};
      output << static_cast<char>(character);
    }
  }
  output << '"';
  return output.str();
}

bool optionalUnsigned(Parser &parser, std::optional<uint64_t> &result) {
  if (parser.null()) {
    result.reset();
    return true;
  }
  uint64_t value = 0;
  if (!parser.unsignedInteger(value))
    return false;
  result = value;
  return true;
}

std::string canonicalConfig(const RuntimeLimits &limits) {
  std::ostringstream output;
  output << "{\"deadlock_window\":";
  if (limits.deadlockWindow)
    output << *limits.deadlockWindow;
  else
    output << "null";
  output << ",\"max_domain_cycles\":{";
  bool first = true;
  for (const auto &[name, value] : limits.maxDomainCycles) {
    if (!first)
      output << ',';
    first = false;
    output << quote(name) << ':' << value;
  }
  output << "},\"max_ticks\":";
  if (limits.maxTicks)
    output << *limits.maxTicks;
  else
    output << "null";
  output << ",\"schema\":\"agentic-model-config\",\"version\":\"1\"}";
  return output.str();
}

} // namespace

bool parseModelConfigJson(std::string_view input, RuntimeLimits &limits,
                          std::string &error) {
  error.clear();
  limits = {};
  if (!isValidUtf8(input)) {
    error = "model config is not valid UTF-8";
    return false;
  }
  if (input == "{}")
    return true;
  Parser parser(input);
  if (!parser.take("{\"deadlock_window\":") ||
      !optionalUnsigned(parser, limits.deadlockWindow) ||
      !parser.take(",\"max_domain_cycles\":{")) {
    error = "model config is malformed or has unknown fields";
    return false;
  }
  if (!parser.take("}")) {
    std::string prior;
    while (true) {
      std::string name;
      uint64_t value = 0;
      if (!parser.string(name) || name.empty() || name <= prior ||
          !parser.take(":") || !parser.unsignedInteger(value)) {
        error = "model config max_domain_cycles is not canonical";
        return false;
      }
      limits.maxDomainCycles.emplace(name, value);
      prior = std::move(name);
      if (parser.take("}"))
        break;
      if (!parser.take(",")) {
        error = "model config max_domain_cycles is malformed";
        return false;
      }
    }
  }
  if (!parser.take(",\"max_ticks\":") ||
      !optionalUnsigned(parser, limits.maxTicks) ||
      !parser.take(",\"schema\":\"agentic-model-config\",\"version\":\"1\"}") ||
      !parser.finish() || canonicalConfig(limits) != input) {
    error = "model config is not canonical v1 JSON";
    return false;
  }
  return true;
}

} // namespace gfsim
