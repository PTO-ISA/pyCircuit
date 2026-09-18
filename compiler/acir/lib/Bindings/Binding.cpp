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
    if (token.getAsDouble(value) || !std::isfinite(value))
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

} // namespace acir::bindings
