#include "acir/Bindings/Binding.h"

#include "BindingInternal.h"
#include "BindingTestHooks.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/ConvertUTF.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <string_view>
#include <system_error>

namespace acir::bindings {
namespace {

constexpr int64_t MaxSafeInteger = 9007199254740991LL;

llvm::Error jsonError(const llvm::Twine &message) {
  return llvm::createStringError(llvm::errc::invalid_argument,
                                 "ACLOWER-BINDING-JSON: %s",
                                 message.str().c_str());
}

llvm::Error metadataError(const llvm::Twine &message) {
  return llvm::createStringError(llvm::errc::invalid_argument,
                                 "ACLOWER-BINDING-METADATA: %s",
                                 message.str().c_str());
}

class IJsonPreflight {
public:
  IJsonPreflight(llvm::StringRef input, const JsonParseLimits &limits)
      : input(input), limits(limits) {}

  llvm::Error run() {
    if (input.size() > limits.maxInputBytes)
      return jsonError("input byte limit exceeded");
    skipWhitespace();
    if (llvm::Error error = scanValue(1))
      return error;
    skipWhitespace();
    if (position != input.size())
      return jsonError(llvm::Twine("trailing input at byte ") +
                       llvm::Twine(position));
    return llvm::Error::success();
  }

private:
  llvm::Error spendWork() {
    if (++structuralWork > limits.maxStructuralWork)
      return jsonError("structural work limit exceeded");
    return llvm::Error::success();
  }

  void skipWhitespace() {
    while (position < input.size() &&
           (input[position] == ' ' || input[position] == '\t' ||
            input[position] == '\n' || input[position] == '\r'))
      ++position;
  }

  llvm::Error scanValue(size_t depth) {
    if (depth > limits.maxDepth)
      return jsonError("maximum depth exceeded");
    if (llvm::Error error = spendWork())
      return error;
    skipWhitespace();
    if (position == input.size())
      return jsonError("unexpected end of input");
    switch (input[position]) {
    case 'n':
      return scanLiteral("null");
    case 't':
      return scanLiteral("true");
    case 'f':
      return scanLiteral("false");
    case '"': {
      auto string = scanString();
      if (!string)
        return string.takeError();
      return llvm::Error::success();
    }
    case '[':
      return scanArray(depth);
    case '{':
      return scanObject(depth);
    default:
      if (input[position] == '-' || llvm::isDigit(input[position]))
        return scanNumber();
      return jsonError(llvm::Twine("unexpected token at byte ") +
                       llvm::Twine(position));
    }
  }

  llvm::Error scanLiteral(llvm::StringRef literal) {
    if (!input.substr(position).starts_with(literal))
      return jsonError(llvm::Twine("invalid literal at byte ") +
                       llvm::Twine(position));
    position += literal.size();
    return llvm::Error::success();
  }

  llvm::Expected<std::string> scanString() {
    size_t start = position++;
    while (position < input.size()) {
      unsigned char byte = static_cast<unsigned char>(input[position++]);
      if (byte == '"') {
        llvm::StringRef token = input.slice(start, position);
        auto parsed = llvm::json::parse(token);
        if (!parsed)
          return jsonError(llvm::Twine("invalid Unicode string at byte ") +
                           llvm::Twine(start));
        auto value = parsed->getAsString();
        if (!value)
          return jsonError("internal string parse failure");
        if (value->size() > limits.maxStringBytes)
          return jsonError("string byte limit exceeded");
        totalStringBytes += value->size();
        if (totalStringBytes > limits.maxTotalStringBytes)
          return jsonError("total string byte limit exceeded");
        return value->str();
      }
      if (byte < 0x20)
        return jsonError(llvm::Twine("unescaped control character at byte ") +
                         llvm::Twine(position - 1));
      if (byte != '\\')
        continue;
      if (position == input.size())
        return jsonError("unterminated escape sequence");
      char escape = input[position++];
      if (escape == '"' || escape == '\\' || escape == '/' || escape == 'b' ||
          escape == 'f' || escape == 'n' || escape == 'r' || escape == 't')
        continue;
      if (escape != 'u' || position + 4 > input.size())
        return jsonError(llvm::Twine("invalid string escape at byte ") +
                         llvm::Twine(position - 1));
      uint16_t codeUnit = 0;
      for (size_t index = 0; index < 4; ++index)
        if (!llvm::isHexDigit(input[position + index])) {
          return jsonError(llvm::Twine("invalid Unicode escape at byte ") +
                           llvm::Twine(position));
        } else {
          codeUnit = static_cast<uint16_t>(
              (codeUnit << 4) | llvm::hexDigitValue(input[position + index]));
        }
      position += 4;
      if (codeUnit >= 0xdc00 && codeUnit <= 0xdfff)
        return jsonError("lone low UTF-16 surrogate is invalid Unicode");
      if (codeUnit < 0xd800 || codeUnit > 0xdbff)
        continue;
      if (position + 6 > input.size() || input[position] != '\\' ||
          input[position + 1] != 'u')
        return jsonError("lone high UTF-16 surrogate is invalid Unicode");
      uint16_t lowSurrogate = 0;
      for (size_t index = 0; index < 4; ++index) {
        char digit = input[position + 2 + index];
        if (!llvm::isHexDigit(digit))
          return jsonError("invalid low UTF-16 surrogate escape");
        lowSurrogate = static_cast<uint16_t>((lowSurrogate << 4) |
                                             llvm::hexDigitValue(digit));
      }
      if (lowSurrogate < 0xdc00 || lowSurrogate > 0xdfff)
        return jsonError("high UTF-16 surrogate requires a low surrogate");
      position += 6;
    }
    return jsonError("unterminated string");
  }

  llvm::Error scanArray(size_t depth) {
    ++position;
    skipWhitespace();
    if (position < input.size() && input[position] == ']') {
      ++position;
      return llvm::Error::success();
    }
    size_t elements = 0;
    while (true) {
      if (++elements > limits.maxArrayElements)
        return jsonError("array element limit exceeded");
      if (llvm::Error error = scanValue(depth + 1))
        return error;
      skipWhitespace();
      if (position == input.size())
        return jsonError("unterminated array");
      if (input[position] == ']') {
        ++position;
        return llvm::Error::success();
      }
      if (input[position++] != ',')
        return jsonError("array entries must be comma-separated");
      skipWhitespace();
    }
  }

  llvm::Error scanObject(size_t depth) {
    ++position;
    skipWhitespace();
    if (position < input.size() && input[position] == '}') {
      ++position;
      return llvm::Error::success();
    }
    llvm::StringSet<> names;
    size_t members = 0;
    while (true) {
      if (++members > limits.maxObjectMembers)
        return jsonError("object member limit exceeded");
      if (position == input.size() || input[position] != '"')
        return jsonError("object property name must be a string");
      auto name = scanString();
      if (!name)
        return name.takeError();
      if (!names.insert(*name).second)
        return jsonError(llvm::Twine("duplicate object property '") + *name +
                         "'");
      skipWhitespace();
      if (position == input.size())
        return jsonError("object property requires ':'");
      const char separator = input[position];
      ++position;
      if (separator != ':')
        return jsonError("object property requires ':'");
      if (llvm::Error error = scanValue(depth + 1))
        return error;
      skipWhitespace();
      if (position == input.size())
        return jsonError("unterminated object");
      if (input[position] == '}') {
        ++position;
        return llvm::Error::success();
      }
      if (input[position++] != ',')
        return jsonError("object entries must be comma-separated");
      skipWhitespace();
    }
  }

  llvm::Error scanNumber() {
    size_t start = position;
    bool negative = false;
    if (input[position] == '-') {
      negative = true;
      ++position;
      if (position == input.size())
        return jsonError("incomplete number");
    }
    if (input[position] == '0') {
      ++position;
      if (position < input.size() && llvm::isDigit(input[position]))
        return jsonError("number has a leading zero");
    } else {
      if (!llvm::isDigit(input[position]) || input[position] == '0')
        return jsonError("number requires an integer part");
      while (position < input.size() && llvm::isDigit(input[position]))
        ++position;
    }
    if (position < input.size() && input[position] == '.') {
      ++position;
      size_t fraction = position;
      while (position < input.size() && llvm::isDigit(input[position]))
        ++position;
      if (fraction == position)
        return jsonError("number fraction requires a digit");
    }
    if (position < input.size() &&
        (input[position] == 'e' || input[position] == 'E')) {
      ++position;
      if (position < input.size() &&
          (input[position] == '+' || input[position] == '-'))
        ++position;
      size_t exponent = position;
      while (position < input.size() && llvm::isDigit(input[position]))
        ++position;
      if (exponent == position)
        return jsonError("number exponent requires a digit");
    }
    llvm::StringRef token = input.slice(start, position);
    double value = 0.0;
    auto converted = std::from_chars(token.begin(), token.end(), value,
                                     std::chars_format::general);
    if (converted.ec != std::errc() || converted.ptr != token.end() ||
        !std::isfinite(value))
      return jsonError("number is not a finite IEEE-754 binary64 value");
    if (negative && value == 0.0)
      return jsonError("negative zero is forbidden by RFC 8785 errata 7920");
    return llvm::Error::success();
  }

  llvm::StringRef input;
  const JsonParseLimits &limits;
  size_t position = 0;
  size_t structuralWork = 0;
  size_t totalStringBytes = 0;
};

llvm::Expected<std::vector<uint16_t>> utf16Units(llvm::StringRef string);
llvm::Expected<std::string> ecmascriptNumber(double value);

llvm::Expected<size_t> canonicalStringSize(llvm::StringRef string) {
  if (auto units = utf16Units(string); !units)
    return units.takeError();
  size_t size = 2;
  for (unsigned char byte : string.bytes()) {
    if (byte == '"' || byte == '\\' || byte == '\b' || byte == '\t' ||
        byte == '\n' || byte == '\f' || byte == '\r') {
      size += 2;
    } else if (byte < 0x20) {
      size += 6;
    } else {
      ++size;
    }
  }
  return size;
}

class ConstructedJsonPreflight {
public:
  explicit ConstructedJsonPreflight(const JsonParseLimits &limits)
      : limits(limits) {}

  llvm::Expected<size_t> run(const llvm::json::Value &value) {
    pending.push_back({&value, 1});
    if (llvm::Error error = drain())
      return std::move(error);
    return canonicalBytes;
  }

  llvm::Expected<size_t> run(const llvm::json::Object &object) {
    if (llvm::Error error = enter(1))
      return std::move(error);
    if (llvm::Error error = visitObject(object, 1))
      return std::move(error);
    if (llvm::Error error = drain())
      return std::move(error);
    return canonicalBytes;
  }

private:
  struct Frame {
    const llvm::json::Value *value;
    size_t depth;
  };

  llvm::Error enter(size_t depth) {
    if (depth > limits.maxDepth)
      return jsonError("maximum depth exceeded");
    if (++structuralWork > limits.maxStructuralWork)
      return jsonError("structural work limit exceeded");
    return llvm::Error::success();
  }

  llvm::Error accountCanonicalBytes(size_t bytes) {
    if (bytes > limits.maxInputBytes - canonicalBytes)
      return jsonError("canonical output byte limit exceeded");
    canonicalBytes += bytes;
    return llvm::Error::success();
  }

  llvm::Error accountRawString(llvm::StringRef string) {
    if (string.size() > limits.maxStringBytes)
      return jsonError("string byte limit exceeded");
    if (string.size() > limits.maxTotalStringBytes - totalStringBytes)
      return jsonError("total string byte limit exceeded");
    totalStringBytes += string.size();
    return llvm::Error::success();
  }

  llvm::Error accountCanonicalString(llvm::StringRef string) {
    auto size = canonicalStringSize(string);
    if (!size)
      return size.takeError();
    if (llvm::Error error = accountCanonicalBytes(*size))
      return error;
    return llvm::Error::success();
  }

  llvm::Error visitObject(const llvm::json::Object &object, size_t depth) {
    if (object.size() > limits.maxObjectMembers)
      return jsonError("object member limit exceeded");
    if (llvm::Error error = accountCanonicalBytes(2))
      return error;
    if (!object.empty()) {
      if (llvm::Error error = accountCanonicalBytes(object.size() - 1))
        return error;
      if (llvm::Error error = accountCanonicalBytes(object.size()))
        return error;
    }
    for (const auto &entry : object) {
      if (llvm::Error error = accountRawString(entry.first))
        return error;
      if (llvm::Error error = accountCanonicalString(entry.first))
        return error;
      pending.push_back({&entry.second, depth + 1});
    }
    return llvm::Error::success();
  }

  llvm::Error drain() {
    while (!pending.empty()) {
      Frame frame = pending.pop_back_val();
      if (llvm::Error error = enter(frame.depth))
        return error;
      const llvm::json::Value &value = *frame.value;
      if (value.getAsNull()) {
        if (llvm::Error error = accountCanonicalBytes(4))
          return error;
        continue;
      }
      if (auto boolean = value.getAsBoolean()) {
        if (llvm::Error error = accountCanonicalBytes(*boolean ? 4 : 5))
          return error;
        continue;
      }
      if (auto string = value.getAsString()) {
        if (llvm::Error error = accountRawString(*string))
          return error;
        if (llvm::Error error = accountCanonicalString(*string))
          return error;
        continue;
      }
      if (const auto *array = value.getAsArray()) {
        if (array->size() > limits.maxArrayElements)
          return jsonError("array element limit exceeded");
        if (llvm::Error error = accountCanonicalBytes(2))
          return error;
        if (!array->empty())
          if (llvm::Error error = accountCanonicalBytes(array->size() - 1))
            return error;
        for (const llvm::json::Value &element : *array)
          pending.push_back({&element, frame.depth + 1});
        continue;
      }
      if (const auto *object = value.getAsObject()) {
        if (llvm::Error error = visitObject(*object, frame.depth))
          return error;
        continue;
      }
      auto number = value.getAsNumber();
      if (!number || !std::isfinite(*number) ||
          (std::signbit(*number) && *number == 0.0))
        return jsonError("constructed number is not canonical I-JSON");
      auto serialized = ecmascriptNumber(*number);
      if (!serialized)
        return serialized.takeError();
      if (llvm::Error error = accountCanonicalBytes(serialized->size()))
        return error;
    }
    return llvm::Error::success();
  }

  const JsonParseLimits &limits;
  llvm::SmallVector<Frame, 64> pending;
  size_t structuralWork = 0;
  size_t totalStringBytes = 0;
  size_t canonicalBytes = 0;
};

llvm::Expected<std::vector<uint16_t>> utf16Units(llvm::StringRef string) {
  std::vector<uint16_t> units;
  for (size_t index = 0; index < string.size();) {
    unsigned char first = static_cast<unsigned char>(string[index]);
    uint32_t codePoint = 0;
    size_t count = 0;
    if (first < 0x80) {
      codePoint = first;
      count = 1;
    } else if ((first & 0xe0) == 0xc0) {
      codePoint = first & 0x1f;
      count = 2;
    } else if ((first & 0xf0) == 0xe0) {
      codePoint = first & 0x0f;
      count = 3;
    } else if ((first & 0xf8) == 0xf0) {
      codePoint = first & 0x07;
      count = 4;
    } else {
      return jsonError("invalid UTF-8 lead byte");
    }
    if (index + count > string.size())
      return jsonError("truncated UTF-8 sequence");
    for (size_t offset = 1; offset < count; ++offset) {
      unsigned char continuation =
          static_cast<unsigned char>(string[index + offset]);
      if ((continuation & 0xc0) != 0x80)
        return jsonError("invalid UTF-8 continuation byte");
      codePoint = (codePoint << 6) | (continuation & 0x3f);
    }
    if ((count == 2 && codePoint < 0x80) || (count == 3 && codePoint < 0x800) ||
        (count == 4 && codePoint < 0x10000) || codePoint > 0x10ffff ||
        (codePoint >= 0xd800 && codePoint <= 0xdfff))
      return jsonError("invalid Unicode scalar value");
    if (codePoint <= 0xffff) {
      units.push_back(static_cast<uint16_t>(codePoint));
    } else {
      codePoint -= 0x10000;
      units.push_back(static_cast<uint16_t>(0xd800 + (codePoint >> 10)));
      units.push_back(static_cast<uint16_t>(0xdc00 + (codePoint & 0x3ff)));
    }
    index += count;
  }
  return units;
}

llvm::Error writeEscapedString(llvm::StringRef string,
                               llvm::raw_ostream &output) {
  if (auto units = utf16Units(string); !units)
    return units.takeError();
  output << '"';
  static constexpr char Hex[] = "0123456789abcdef";
  for (unsigned char byte : string.bytes()) {
    switch (byte) {
    case '"':
      output << "\\\"";
      break;
    case '\\':
      output << "\\\\";
      break;
    case '\b':
      output << "\\b";
      break;
    case '\t':
      output << "\\t";
      break;
    case '\n':
      output << "\\n";
      break;
    case '\f':
      output << "\\f";
      break;
    case '\r':
      output << "\\r";
      break;
    default:
      if (byte < 0x20)
        output << "\\u00" << Hex[byte >> 4] << Hex[byte & 0xf];
      else
        output << static_cast<char>(byte);
      break;
    }
  }
  output << '"';
  return llvm::Error::success();
}

llvm::Expected<std::string> ecmascriptNumber(double value) {
  if (!std::isfinite(value))
    return jsonError("non-finite number cannot be canonicalized");
  if (value == 0.0) {
    if (std::signbit(value))
      return jsonError("negative zero cannot be canonicalized");
    return std::string("0");
  }

  std::array<char, 128> buffer{};
  auto converted = std::to_chars(buffer.data(), buffer.data() + buffer.size(),
                                 value, std::chars_format::general);
  if (converted.ec != std::errc())
    return jsonError("binary64 conversion failed");
  std::string shortest(buffer.data(), converted.ptr);
  bool negative = shortest.front() == '-';
  llvm::StringRef magnitude(shortest);
  if (negative)
    magnitude = magnitude.drop_front();

  auto [mantissa, exponentText] = magnitude.split('e');
  if (exponentText.empty()) {
    auto split = magnitude.split('E');
    mantissa = split.first;
    exponentText = split.second;
  }
  int explicitExponent = 0;
  if (!exponentText.empty()) {
    llvm::StringRef digits = exponentText;
    bool exponentNegative = digits.consume_front("-");
    digits.consume_front("+");
    if (digits.getAsInteger(10, explicitExponent))
      return jsonError("binary64 exponent conversion failed");
    if (exponentNegative)
      explicitExponent = -explicitExponent;
  }

  size_t decimal = mantissa.find('.');
  if (decimal == llvm::StringRef::npos)
    decimal = mantissa.size();
  std::string digits;
  digits.reserve(mantissa.size());
  for (char character : mantissa)
    if (character != '.')
      digits.push_back(character);
  size_t firstNonZero = digits.find_first_not_of('0');
  if (firstNonZero == std::string::npos)
    return std::string("0");
  int scientificExponent = explicitExponent + static_cast<int>(decimal) -
                           static_cast<int>(firstNonZero) - 1;
  digits.erase(0, firstNonZero);
  while (digits.size() > 1 && digits.back() == '0')
    digits.pop_back();

  std::string result;
  if (negative)
    result.push_back('-');
  if (scientificExponent >= 0 && scientificExponent < 21) {
    size_t integerDigits = static_cast<size_t>(scientificExponent) + 1;
    if (digits.size() <= integerDigits) {
      result.append(digits);
      result.append(integerDigits - digits.size(), '0');
    } else {
      result.append(digits.substr(0, integerDigits));
      result.push_back('.');
      result.append(digits.substr(integerDigits));
    }
  } else if (scientificExponent >= -6 && scientificExponent < 0) {
    result.append("0.");
    result.append(static_cast<size_t>(-scientificExponent - 1), '0');
    result.append(digits);
  } else {
    result.push_back(digits.front());
    if (digits.size() > 1) {
      result.push_back('.');
      result.append(digits.substr(1));
    }
    result.push_back('e');
    if (scientificExponent >= 0)
      result.push_back('+');
    result.append(std::to_string(scientificExponent));
  }
  return result;
}

llvm::Error writeCanonical(const llvm::json::Value &value,
                           llvm::raw_ostream &output) {
  if (value.getAsNull()) {
    output << "null";
    return llvm::Error::success();
  }
  if (auto boolean = value.getAsBoolean()) {
    output << (*boolean ? "true" : "false");
    return llvm::Error::success();
  }
  if (auto string = value.getAsString())
    return writeEscapedString(*string, output);
  if (const auto *array = value.getAsArray()) {
    output << '[';
    for (size_t index = 0; index < array->size(); ++index) {
      if (index)
        output << ',';
      if (llvm::Error error = writeCanonical((*array)[index], output))
        return error;
    }
    output << ']';
    return llvm::Error::success();
  }
  if (const auto *object = value.getAsObject()) {
    struct Property {
      llvm::StringRef name;
      const llvm::json::Value *value;
      std::vector<uint16_t> sortKey;
    };
    std::vector<Property> properties;
    properties.reserve(object->size());
    for (const auto &entry : *object) {
      llvm::StringRef name = entry.first;
      auto key = utf16Units(name);
      if (!key)
        return key.takeError();
      properties.push_back({name, &entry.second, std::move(*key)});
    }
    llvm::sort(properties, [](const Property &left, const Property &right) {
      return std::lexicographical_compare(
          left.sortKey.begin(), left.sortKey.end(), right.sortKey.begin(),
          right.sortKey.end());
    });
    output << '{';
    for (size_t index = 0; index < properties.size(); ++index) {
      if (index)
        output << ',';
      if (llvm::Error error =
              writeEscapedString(properties[index].name, output))
        return error;
      output << ':';
      if (llvm::Error error = writeCanonical(*properties[index].value, output))
        return error;
    }
    output << '}';
    return llvm::Error::success();
  }
  auto number = value.getAsNumber();
  if (!number)
    return jsonError("unsupported JSON value kind");
  if (std::signbit(*number) && *number == 0.0)
    return jsonError("negative zero cannot be canonicalized");
  auto serialized = ecmascriptNumber(*number);
  if (!serialized)
    return serialized.takeError();
  output << *serialized;
  return llvm::Error::success();
}

bool hasExactKeys(const llvm::json::Object &object,
                  llvm::ArrayRef<llvm::StringRef> expected) {
  if (object.size() != expected.size())
    return false;
  return llvm::all_of(expected,
                      [&](llvm::StringRef key) { return object.get(key); });
}

llvm::Expected<std::string> requireString(const llvm::json::Object &object,
                                          llvm::StringRef key) {
  auto value = object.getString(key);
  if (!value)
    return metadataError(llvm::Twine("field '") + key + "' must be a string");
  return value->str();
}

bool isName(llvm::StringRef value) {
  if (value.empty() || !(llvm::isAlpha(value.front()) || value.front() == '_'))
    return false;
  return llvm::all_of(value.drop_front(), [](char character) {
    return llvm::isAlnum(character) || character == '_';
  });
}

bool isIdentity(llvm::StringRef value) {
  if (value.empty() || !isName(value.split('.').first))
    return false;
  while (value.contains('.')) {
    value = value.split('.').second;
    if (!isName(value.split('.').first))
      return false;
  }
  return true;
}

bool isCppSymbol(llvm::StringRef value) {
  if (value.empty() || value.starts_with("::") || value.ends_with("::"))
    return false;
  while (!value.empty()) {
    auto [segment, remainder] = value.split("::");
    if (!isName(segment))
      return false;
    value = remainder;
  }
  return true;
}

bool hasRawCppFragment(llvm::StringRef value) {
  return value.contains(';') || value.contains('{') || value.contains('}') ||
         value.contains('(') || value.contains(')') || value.contains('=') ||
         value.contains('%') || value.contains('#') || value.contains('\n') ||
         value.contains('\r') || value.contains('`');
}

bool isSha256(llvm::StringRef value) {
  if (!value.starts_with("sha256:") || value.size() != 71)
    return false;
  return llvm::all_of(value.drop_front(7), [](char character) {
    return llvm::isDigit(character) || (character >= 'a' && character <= 'f');
  });
}

llvm::Error validateStaticValue(const llvm::json::Value &value) {
  if (value.getAsNull() || value.getAsBoolean())
    return llvm::Error::success();
  if (auto string = value.getAsString()) {
    if (hasRawCppFragment(*string))
      return metadataError("static metadata contains raw C++ fragments");
    return llvm::Error::success();
  }
  if (auto integer = value.getAsInteger()) {
    if (*integer < -MaxSafeInteger || *integer > MaxSafeInteger)
      return metadataError("static integer is outside the safe exact range");
    return llvm::Error::success();
  }
  if (auto number = value.getAsNumber()) {
    if (!std::isfinite(*number) || (std::signbit(*number) && *number == 0.0))
      return metadataError("static number is not canonical I-JSON");
    return llvm::Error::success();
  }
  if (const auto *array = value.getAsArray()) {
    for (const llvm::json::Value &element : *array)
      if (llvm::Error error = validateStaticValue(element))
        return error;
    return llvm::Error::success();
  }
  if (const auto *object = value.getAsObject()) {
    for (const auto &entry : *object) {
      if (!isName(entry.first))
        return metadataError(
            "static metadata object keys must be canonical identifiers");
      if (llvm::Error error = validateStaticValue(entry.second))
        return error;
    }
    return llvm::Error::success();
  }
  return metadataError("static metadata is not canonical I-JSON data");
}

template <typename Record, typename Parser>
llvm::Expected<std::vector<Record>>
parseRecordArray(const llvm::json::Object &object, llvm::StringRef key,
                 Parser parse) {
  const auto *array = object.getArray(key);
  if (!array)
    return metadataError(llvm::Twine("field '") + key + "' must be an array");
  std::vector<Record> records;
  records.reserve(array->size());
  for (const llvm::json::Value &value : *array) {
    const auto *record = value.getAsObject();
    if (!record)
      return metadataError(llvm::Twine("field '") + key +
                           "' must contain records");
    auto parsed = parse(*record);
    if (!parsed)
      return parsed.takeError();
    records.push_back(std::move(*parsed));
  }
  return records;
}

} // namespace

namespace detail {

llvm::Expected<size_t> preflightConstructedJson(const llvm::json::Value &value,
                                                const JsonParseLimits &limits) {
  ConstructedJsonPreflight preflight(limits);
  return preflight.run(value);
}

llvm::Expected<size_t>
preflightConstructedJson(const llvm::json::Object &object,
                         const JsonParseLimits &limits) {
  ConstructedJsonPreflight preflight(limits);
  return preflight.run(object);
}

} // namespace detail

struct BindingRecord::Storage {
  llvm::json::Object object;
  std::string bindingSchema;
  std::string contractEpoch;
  std::string binding;
  std::string componentSchema;
  std::string componentSchemaFingerprint;
  std::string availability;
  std::string effect;
  std::string cppType;
  std::string implementation;
  std::string provider;
  std::string providerImplementationFingerprint;
  std::string fingerprint;
  CppBinding cpp;
  ConstructionBinding construction;
  OwnershipBinding ownership;
  std::vector<ParameterBinding> parameters;
  std::vector<PortBinding> ports;
  std::vector<ResourceBinding> resources;
  std::vector<ResultBinding> results;
  std::vector<ActivationSourceBinding> activationSources;
};

llvm::Expected<llvm::json::Value> parseIJson(llvm::StringRef input,
                                             const JsonParseLimits &limits) {
  IJsonPreflight preflight(input, limits);
  if (llvm::Error error = preflight.run())
    return std::move(error);
  auto parsed = llvm::json::parse(input);
  if (!parsed)
    return jsonError(llvm::Twine("invalid JSON: ") +
                     llvm::toString(parsed.takeError()));
  return std::move(*parsed);
}

llvm::Expected<std::string> canonicalizeJson(const llvm::json::Value &value,
                                             const JsonParseLimits &limits) {
  auto canonicalSize = detail::preflightConstructedJson(value, limits);
  if (!canonicalSize)
    return canonicalSize.takeError();
  if (detail::shouldFailCanonicalEmission())
    return jsonError("canonical emission failure injected");
  std::string storage;
  storage.reserve(*canonicalSize);
  llvm::raw_string_ostream output(storage);
  if (llvm::Error error = writeCanonical(value, output))
    return std::move(error);
  output.flush();
  return storage;
}

llvm::Expected<std::string>
canonicalizeJsonText(llvm::StringRef input, const JsonParseLimits &limits) {
  auto parsed = parseIJson(input, limits);
  if (!parsed)
    return parsed.takeError();
  return canonicalizeJson(*parsed, limits);
}

std::string sha256Fingerprint(llvm::StringRef canonicalBytes) {
  llvm::SHA256 hasher;
  hasher.update(canonicalBytes);
  return "sha256:" + llvm::toHex(hasher.final(), true);
}

BindingRecord::BindingRecord(std::shared_ptr<const Storage> storage)
    : storage(std::move(storage)) {}

BindingRecord::~BindingRecord() = default;

llvm::Expected<BindingRecord>
BindingRecord::parse(const llvm::json::Object &object,
                     const JsonParseLimits &limits) {
  auto canonicalSize = detail::preflightConstructedJson(object, limits);
  if (!canonicalSize)
    return canonicalSize.takeError();
  static constexpr std::array<llvm::StringRef, 20> TopKeys = {
      "activation_sources",
      "availability",
      "binding",
      "binding_schema",
      "component_schema",
      "component_schema_fingerprint",
      "construction",
      "contract_epoch",
      "cpp",
      "cpp_type",
      "effect",
      "fingerprint",
      "implementation",
      "ownership",
      "parameters",
      "ports",
      "provider",
      "provider_implementation_fingerprint",
      "resources",
      "results",
  };
  if (!hasExactKeys(object, TopKeys))
    return metadataError(
        "binding lock must contain exactly the acsim-binding-0.1 fields");

  auto result = std::make_shared<Storage>();
  result->object = object;
#define ACIR_REQUIRE_STRING(member, key)                                       \
  do {                                                                         \
    auto value = requireString(object, key);                                   \
    if (!value)                                                                \
      return value.takeError();                                                \
    result->member = std::move(*value);                                        \
  } while (false)
  ACIR_REQUIRE_STRING(bindingSchema, "binding_schema");
  ACIR_REQUIRE_STRING(contractEpoch, "contract_epoch");
  ACIR_REQUIRE_STRING(binding, "binding");
  ACIR_REQUIRE_STRING(componentSchema, "component_schema");
  ACIR_REQUIRE_STRING(componentSchemaFingerprint,
                      "component_schema_fingerprint");
  ACIR_REQUIRE_STRING(availability, "availability");
  ACIR_REQUIRE_STRING(effect, "effect");
  ACIR_REQUIRE_STRING(cppType, "cpp_type");
  ACIR_REQUIRE_STRING(implementation, "implementation");
  ACIR_REQUIRE_STRING(provider, "provider");
  ACIR_REQUIRE_STRING(providerImplementationFingerprint,
                      "provider_implementation_fingerprint");
  ACIR_REQUIRE_STRING(fingerprint, "fingerprint");
#undef ACIR_REQUIRE_STRING

  if (result->bindingSchema != BindingSchema ||
      result->contractEpoch != ContractEpoch ||
      result->availability != "available" || !isName(result->binding) ||
      !isIdentity(result->componentSchema) ||
      !isIdentity(result->implementation) || !isIdentity(result->provider) ||
      !isIdentity(result->cppType) ||
      (result->effect != "pure" && result->effect != "stateful"))
    return metadataError(
        "binding lock identity, epoch, availability, and effect are invalid");
  for (llvm::StringRef fingerprint :
       {llvm::StringRef(result->componentSchemaFingerprint),
        llvm::StringRef(result->providerImplementationFingerprint),
        llvm::StringRef(result->fingerprint)})
    if (!isSha256(fingerprint))
      return metadataError(
          "binding lock requires lowercase sha256 fingerprints");

  const auto *cpp = object.getObject("cpp");
  static constexpr std::array<llvm::StringRef, 5> CppKeys = {
      "concept", "entry_points", "header", "symbol", "target"};
  static constexpr std::array<llvm::StringRef, 5> EntryKeys = {
      "pure", "reset", "validate", "work", "xfer"};
  if (!cpp || !hasExactKeys(*cpp, CppKeys))
    return metadataError("binding lock C++ record must have exact fields");
  const auto *entries = cpp->getObject("entry_points");
  if (!entries || !hasExactKeys(*entries, EntryKeys))
    return metadataError("binding lock entry points must have exact fields");
  auto assignCpp = [&](std::string &destination, const llvm::json::Object &from,
                       llvm::StringRef key) -> llvm::Error {
    auto value = requireString(from, key);
    if (!value)
      return value.takeError();
    destination = std::move(*value);
    return llvm::Error::success();
  };
  if (llvm::Error error = assignCpp(result->cpp.conceptName, *cpp, "concept"))
    return std::move(error);
  if (llvm::Error error = assignCpp(result->cpp.header, *cpp, "header"))
    return std::move(error);
  if (llvm::Error error = assignCpp(result->cpp.symbol, *cpp, "symbol"))
    return std::move(error);
  if (llvm::Error error = assignCpp(result->cpp.target, *cpp, "target"))
    return std::move(error);
  if (llvm::Error error =
          assignCpp(result->cpp.entryPoints.pure, *entries, "pure"))
    return std::move(error);
  if (llvm::Error error =
          assignCpp(result->cpp.entryPoints.reset, *entries, "reset"))
    return std::move(error);
  if (llvm::Error error =
          assignCpp(result->cpp.entryPoints.validate, *entries, "validate"))
    return std::move(error);
  if (llvm::Error error =
          assignCpp(result->cpp.entryPoints.work, *entries, "work"))
    return std::move(error);
  if (llvm::Error error =
          assignCpp(result->cpp.entryPoints.xfer, *entries, "xfer"))
    return std::move(error);
  for (llvm::StringRef value :
       {llvm::StringRef(result->cpp.conceptName),
        llvm::StringRef(result->cpp.header),
        llvm::StringRef(result->cpp.symbol),
        llvm::StringRef(result->cpp.target),
        llvm::StringRef(result->cpp.entryPoints.pure),
        llvm::StringRef(result->cpp.entryPoints.reset),
        llvm::StringRef(result->cpp.entryPoints.validate),
        llvm::StringRef(result->cpp.entryPoints.work),
        llvm::StringRef(result->cpp.entryPoints.xfer)})
    if (hasRawCppFragment(value))
      return metadataError(
          "binding metadata cannot contain raw C++ or emitter behavior");
  llvm::StringRef header = result->cpp.header;
  if (!isCppSymbol(result->cpp.conceptName) ||
      !isCppSymbol(result->cpp.symbol) || !isName(result->cpp.target) ||
      header.empty() || header.starts_with('/') || header.contains('\\') ||
      header == ".." || header.starts_with("../") || header.contains("/../") ||
      header.ends_with("/.."))
    return metadataError("binding C++ names and header path are invalid");
  for (llvm::StringRef entry :
       {llvm::StringRef(result->cpp.entryPoints.pure),
        llvm::StringRef(result->cpp.entryPoints.reset),
        llvm::StringRef(result->cpp.entryPoints.validate),
        llvm::StringRef(result->cpp.entryPoints.work),
        llvm::StringRef(result->cpp.entryPoints.xfer)})
    if (!entry.empty() && !isCppSymbol(entry))
      return metadataError(
          "binding entry points must be empty or qualified C++ symbols");
  if ((result->effect == "pure" && (result->cpp.entryPoints.pure.empty() ||
                                    !result->cpp.entryPoints.reset.empty() ||
                                    !result->cpp.entryPoints.validate.empty() ||
                                    !result->cpp.entryPoints.work.empty() ||
                                    !result->cpp.entryPoints.xfer.empty())) ||
      (result->effect == "stateful" && (!result->cpp.entryPoints.pure.empty() ||
                                        result->cpp.entryPoints.work.empty() ||
                                        result->cpp.entryPoints.xfer.empty())))
    return metadataError(
        "binding effect requires exact executable entry points");

  const auto *construction = object.getObject("construction");
  const auto *ownership = object.getObject("ownership");
  static constexpr std::array<llvm::StringRef, 2> ConstructionKeys = {
      "arguments", "kind"};
  static constexpr std::array<llvm::StringRef, 2> OwnershipKeys = {"kind",
                                                                   "placement"};
  if (!construction || !hasExactKeys(*construction, ConstructionKeys) ||
      !ownership || !hasExactKeys(*ownership, OwnershipKeys))
    return metadataError(
        "binding construction and ownership records are exact");
  auto constructionKind = requireString(*construction, "kind");
  auto ownershipKind = requireString(*ownership, "kind");
  auto ownershipPlacement = requireString(*ownership, "placement");
  const auto *arguments = construction->getArray("arguments");
  if (!constructionKind || !ownershipKind || !ownershipPlacement || !arguments)
    return metadataError(
        "binding construction and ownership fields are invalid");
  result->construction.kind = std::move(*constructionKind);
  result->construction.arguments.assign(arguments->begin(), arguments->end());
  result->ownership.kind = std::move(*ownershipKind);
  result->ownership.placement = std::move(*ownershipPlacement);
  if (result->construction.kind != "constructor")
    return metadataError("construction kind must be exactly constructor");
  for (const llvm::json::Value &argument : result->construction.arguments)
    if (llvm::Error error = validateStaticValue(argument))
      return std::move(error);
  if ((result->effect == "pure" && (result->ownership.kind != "none" ||
                                    result->ownership.placement != "inline")) ||
      (result->effect == "stateful" &&
       (result->ownership.kind != "unique" ||
        (result->ownership.placement != "member_or_array" &&
         result->ownership.placement != "root_or_process"))))
    return metadataError("binding ownership does not match its effect");

  auto parameters = parseRecordArray<ParameterBinding>(
      object, "parameters",
      [](const llvm::json::Object &parameter)
          -> llvm::Expected<ParameterBinding> {
        static constexpr std::array<llvm::StringRef, 6> Keys = {
            "acir_type", "cpp_type", "mapping", "name", "ordinal", "value"};
        if (!hasExactKeys(parameter, Keys))
          return metadataError(
              "binding parameter must have exact static fields");
        ParameterBinding result;
        auto acirType = requireString(parameter, "acir_type");
        auto cppType = requireString(parameter, "cpp_type");
        auto mapping = requireString(parameter, "mapping");
        auto name = requireString(parameter, "name");
        auto ordinal = parameter.getInteger("ordinal");
        const llvm::json::Value *value = parameter.get("value");
        if (!acirType || !cppType || !mapping || !name || !ordinal || !value)
          return metadataError("binding parameter fields have invalid types");
        result.acirType = std::move(*acirType);
        result.cppType = std::move(*cppType);
        result.mapping = std::move(*mapping);
        result.name = std::move(*name);
        result.ordinal = *ordinal;
        result.value = *value;
        if (hasRawCppFragment(result.acirType) ||
            hasRawCppFragment(result.cppType))
          return metadataError(
              "binding metadata cannot contain raw C++ or emitter behavior");
        if (result.acirType.empty() || result.cppType.empty() ||
            !isName(result.name) || result.ordinal < 0 ||
            (result.mapping != "template_argument" &&
             result.mapping != "constexpr_argument" &&
             result.mapping != "constructor_constant"))
          return metadataError("binding parameter domain is invalid");
        if (llvm::Error error = validateStaticValue(result.value))
          return std::move(error);
        return result;
      });
  if (!parameters)
    return parameters.takeError();
  llvm::StringSet<> parameterNames;
  for (size_t index = 0; index < parameters->size(); ++index)
    if ((*parameters)[index].ordinal != static_cast<int64_t>(index) ||
        !parameterNames.insert((*parameters)[index].name).second)
      return metadataError(
          "binding parameters require contiguous ordinals and unique names");
  result->parameters = std::move(*parameters);

  auto ports = parseRecordArray<PortBinding>(
      object, "ports",
      [](const llvm::json::Object &port) -> llvm::Expected<PortBinding> {
        static constexpr std::array<llvm::StringRef, 10> Keys = {
            "accessor",  "cardinality", "delegation", "direction",
            "interface", "ownership",   "payload",    "protocol",
            "role",      "time_domain"};
        if (!hasExactKeys(port, Keys))
          return metadataError(
              "binding port records require exact closed fields");
        PortBinding result;
#define ACIR_PORT_STRING(member, key)                                          \
  do {                                                                         \
    auto value = requireString(port, key);                                     \
    if (!value)                                                                \
      return value.takeError();                                                \
    result.member = std::move(*value);                                         \
  } while (false)
        ACIR_PORT_STRING(accessor, "accessor");
        ACIR_PORT_STRING(cardinality, "cardinality");
        ACIR_PORT_STRING(delegation, "delegation");
        ACIR_PORT_STRING(direction, "direction");
        ACIR_PORT_STRING(interface, "interface");
        ACIR_PORT_STRING(ownership, "ownership");
        ACIR_PORT_STRING(payload, "payload");
        ACIR_PORT_STRING(protocol, "protocol");
        ACIR_PORT_STRING(role, "role");
        ACIR_PORT_STRING(timeDomain, "time_domain");
#undef ACIR_PORT_STRING
        if (!isName(result.accessor) || !isIdentity(result.interface) ||
            !isIdentity(result.payload) || !isIdentity(result.protocol) ||
            !isIdentity(result.role) || !isIdentity(result.timeDomain) ||
            (result.cardinality != "exclusive" &&
             result.cardinality != "shared") ||
            (result.direction != "input" && result.direction != "output") ||
            (result.delegation != "forbidden" &&
             result.delegation != "allowed" &&
             result.delegation != "required") ||
            (result.ownership != "owned" && result.ownership != "borrowed" &&
             result.ownership != "shared"))
          return metadataError("binding port domain is invalid");
        return result;
      });
  if (!ports)
    return ports.takeError();
  llvm::StringSet<> portAccessors;
  for (const PortBinding &port : *ports)
    if (!portAccessors.insert(port.accessor).second)
      return metadataError("binding port accessors must be unique");
  result->ports = std::move(*ports);

  auto resources = parseRecordArray<ResourceBinding>(
      object, "resources",
      [](const llvm::json::Object &resource)
          -> llvm::Expected<ResourceBinding> {
        static constexpr std::array<llvm::StringRef, 7> Keys = {
            "accessor", "delegation", "mode",       "ownership",
            "resource", "role",       "time_domain"};
        if (!hasExactKeys(resource, Keys))
          return metadataError(
              "binding resource records require exact closed fields");
        ResourceBinding result;
#define ACIR_RESOURCE_STRING(member, key)                                      \
  do {                                                                         \
    auto value = requireString(resource, key);                                 \
    if (!value)                                                                \
      return value.takeError();                                                \
    result.member = std::move(*value);                                         \
  } while (false)
        ACIR_RESOURCE_STRING(accessor, "accessor");
        ACIR_RESOURCE_STRING(delegation, "delegation");
        ACIR_RESOURCE_STRING(mode, "mode");
        ACIR_RESOURCE_STRING(ownership, "ownership");
        ACIR_RESOURCE_STRING(resource, "resource");
        ACIR_RESOURCE_STRING(role, "role");
        ACIR_RESOURCE_STRING(timeDomain, "time_domain");
#undef ACIR_RESOURCE_STRING
        if (!isName(result.accessor) || !isIdentity(result.resource) ||
            !isIdentity(result.role) || !isIdentity(result.timeDomain) ||
            (result.mode != "initiator" && result.mode != "target") ||
            (result.delegation != "forbidden" &&
             result.delegation != "allowed" &&
             result.delegation != "required") ||
            (result.ownership != "owned" && result.ownership != "borrowed" &&
             result.ownership != "shared"))
          return metadataError("binding resource domain is invalid");
        return result;
      });
  if (!resources)
    return resources.takeError();
  llvm::StringSet<> resourceAccessors;
  for (const ResourceBinding &resource : *resources)
    if (!resourceAccessors.insert(resource.accessor).second)
      return metadataError("binding resource accessors must be unique");
  result->resources = std::move(*resources);

  auto results = parseRecordArray<ResultBinding>(
      object, "results",
      [](const llvm::json::Object &entry) -> llvm::Expected<ResultBinding> {
        static constexpr std::array<llvm::StringRef, 2> Keys = {"cpp_type",
                                                                "name"};
        if (!hasExactKeys(entry, Keys))
          return metadataError("binding result records require exact fields");
        auto cppType = requireString(entry, "cpp_type");
        auto name = requireString(entry, "name");
        if (!cppType || !name)
          return metadataError("binding result fields have invalid types");
        if (!isIdentity(*cppType) || !isName(*name))
          return metadataError("binding result domain is invalid");
        return ResultBinding{std::move(*cppType), std::move(*name)};
      });
  if (!results)
    return results.takeError();
  llvm::StringSet<> resultNames;
  for (const ResultBinding &entry : *results)
    if (!resultNames.insert(entry.name).second)
      return metadataError("binding result names must be unique");
  result->results = std::move(*results);

  auto activations = parseRecordArray<ActivationSourceBinding>(
      object, "activation_sources",
      [](const llvm::json::Object &entry)
          -> llvm::Expected<ActivationSourceBinding> {
        static constexpr std::array<llvm::StringRef, 2> Keys = {"kind", "name"};
        if (!hasExactKeys(entry, Keys))
          return metadataError(
              "binding activation source records require exact fields");
        auto kind = requireString(entry, "kind");
        auto name = requireString(entry, "name");
        if (!kind || !name)
          return metadataError(
              "binding activation source fields have invalid types");
        if (!isIdentity(*kind) || !isName(*name))
          return metadataError("binding activation source domain is invalid");
        return ActivationSourceBinding{std::move(*kind), std::move(*name)};
      });
  if (!activations)
    return activations.takeError();
  if (result->effect == "pure" && !activations->empty())
    return metadataError(
        "pure binding cannot contain activation or wakeup metadata");
  llvm::StringSet<> activationNames;
  for (const ActivationSourceBinding &activation : *activations)
    if (!activationNames.insert(activation.name).second)
      return metadataError("binding activation-source names must be unique");
  result->activationSources = std::move(*activations);

  return BindingRecord(std::move(result));
}

llvm::StringRef BindingRecord::bindingSchema() const {
  return storage->bindingSchema;
}
llvm::StringRef BindingRecord::contractEpoch() const {
  return storage->contractEpoch;
}
llvm::StringRef BindingRecord::binding() const { return storage->binding; }
llvm::StringRef BindingRecord::componentSchema() const {
  return storage->componentSchema;
}
llvm::StringRef BindingRecord::componentSchemaFingerprint() const {
  return storage->componentSchemaFingerprint;
}
llvm::StringRef BindingRecord::availability() const {
  return storage->availability;
}
llvm::StringRef BindingRecord::effect() const { return storage->effect; }
llvm::StringRef BindingRecord::cppType() const { return storage->cppType; }
llvm::StringRef BindingRecord::implementation() const {
  return storage->implementation;
}
llvm::StringRef BindingRecord::provider() const { return storage->provider; }
llvm::StringRef BindingRecord::providerImplementationFingerprint() const {
  return storage->providerImplementationFingerprint;
}
llvm::StringRef BindingRecord::fingerprint() const {
  return storage->fingerprint;
}
const CppBinding &BindingRecord::cpp() const { return storage->cpp; }
const ConstructionBinding &BindingRecord::construction() const {
  return storage->construction;
}
const OwnershipBinding &BindingRecord::ownership() const {
  return storage->ownership;
}
llvm::ArrayRef<ParameterBinding> BindingRecord::parameters() const {
  return storage->parameters;
}
llvm::ArrayRef<PortBinding> BindingRecord::ports() const {
  return storage->ports;
}
llvm::ArrayRef<ResourceBinding> BindingRecord::resources() const {
  return storage->resources;
}
llvm::ArrayRef<ResultBinding> BindingRecord::results() const {
  return storage->results;
}
llvm::ArrayRef<ActivationSourceBinding>
BindingRecord::activationSources() const {
  return storage->activationSources;
}
const llvm::json::Object &BindingRecord::json() const {
  return storage->object;
}

llvm::Expected<std::string> BindingRecord::canonicalJson() const {
  return canonicalizeJson(
      llvm::json::Value(llvm::json::Object(storage->object)));
}

llvm::Expected<std::string> BindingRecord::canonicalJsonForFingerprint() const {
  llvm::json::Object object(storage->object);
  object.erase("fingerprint");
  return canonicalizeJson(llvm::json::Value(std::move(object)));
}

llvm::Error BindingRecord::validateFingerprint() const {
  auto expected = computeBindingRecordFingerprint(*this);
  if (!expected)
    return expected.takeError();
  if (*expected != fingerprint())
    return llvm::createStringError(
        llvm::errc::invalid_argument,
        "ACLOWER-FINGERPRINT: binding=%s expected=%s actual=%s",
        binding().str().c_str(), expected->c_str(),
        fingerprint().str().c_str());
  return llvm::Error::success();
}

llvm::Expected<std::string>
computeBindingRecordFingerprint(const BindingRecord &record) {
  auto canonical = record.canonicalJsonForFingerprint();
  if (!canonical)
    return canonical.takeError();
  return sha256Fingerprint(*canonical);
}

} // namespace acir::bindings
