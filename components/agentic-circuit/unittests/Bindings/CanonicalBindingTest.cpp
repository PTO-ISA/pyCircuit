#include "Bindings/BindingTestHooks.h"
#include "acir/Bindings/Binding.h"
#include "acir/Bindings/Registry.h"
#include "acir/Dialect/ACIR/GraphRegion.h"
#include "acir/InitAllDialects.h"
#include "acir/Transforms/Passes.h"
#include "acir/Transforms/ResolveBindings.h"

#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <bit>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace acir::bindings {
namespace {

constexpr llvm::StringLiteral kRecordFingerprint =
    "sha256:c6b77b47fe20236bd16946179bcf8109e3c6f6bab8029cd9bb1abbd29eb7a70e";

std::string takeError(llvm::Error error) {
  return llvm::toString(std::move(error));
}

testing::AssertionResult containsText(llvm::StringRef actual,
                                      llvm::StringRef expected) {
  if (actual.contains(expected))
    return testing::AssertionSuccess();
  return testing::AssertionFailure() << "expected substring '" << expected.str()
                                     << "' in '" << actual.str() << "'";
}

testing::AssertionResult startsWithText(llvm::StringRef actual,
                                        llvm::StringRef expected) {
  if (actual.starts_with(expected))
    return testing::AssertionSuccess();
  return testing::AssertionFailure() << "expected prefix '" << expected.str()
                                     << "' in '" << actual.str() << "'";
}

std::string recordJson(llvm::StringRef fingerprint = kRecordFingerprint,
                       llvm::StringRef symbol = "gfsim::Leaf") {
  return (llvm::Twine(R"json({
    "activation_sources": [],
    "availability": "available",
    "binding": "Leaf",
    "binding_schema": "acsim-binding-0.1",
    "component_schema": "ac.Leaf",
    "component_schema_fingerprint": "sha256:1111111111111111111111111111111111111111111111111111111111111111",
    "construction": {"arguments": [8], "kind": "constructor"},
    "contract_epoch": "0.4",
    "cpp": {
      "concept": "gfsim::PureModel",
      "entry_points": {"pure": "gfsim::leaf", "reset": "", "validate": "", "work": "", "xfer": ""},
      "header": "gfsim/leaf.hpp",
      "symbol": ")json") +
          symbol + R"json(",
      "target": "gfsim"
    },
    "cpp_type": "cpp_i32",
    "effect": "pure",
    "fingerprint": ")json" +
          fingerprint + R"json(",
    "implementation": "gfsim.Leaf",
    "ownership": {"kind": "none", "placement": "inline"},
    "parameters": [{
      "acir_type": "i64",
      "cpp_type": "std::int64_t",
      "mapping": "constructor_constant",
      "name": "width",
      "ordinal": 0,
      "value": 8
    }],
    "ports": [{
      "accessor": "input",
      "cardinality": "exclusive",
      "delegation": "forbidden",
      "direction": "input",
      "interface": "ac.Stream",
      "ownership": "borrowed",
      "payload": "ac.Packet",
      "protocol": "ac.ReadyValid",
      "role": "consumer",
      "time_domain": "ac.cycle"
    }],
    "provider": "gfsim",
    "provider_implementation_fingerprint": "sha256:2222222222222222222222222222222222222222222222222222222222222222",
    "resources": [],
    "results": [{"cpp_type": "cpp_i32", "name": "result"}]
  })json")
      .str();
}

std::string candidateJson(llvm::StringRef profile = "fast",
                          llvm::StringRef target = "arm64-apple-darwin",
                          bool available = true, llvm::StringRef record = {}) {
  std::string selectedRecord = record.empty() ? recordJson() : record.str();
  return (llvm::Twine(R"json({"available":)json") +
          (available ? "true" : "false") + R"json(,"profile":")json" + profile +
          R"json(","record":)json" + selectedRecord + R"json(,"target":")json" +
          target + R"json("})json")
      .str();
}

std::string requestJson(
    llvm::StringRef componentSchema = "ac.Leaf",
    llvm::StringRef providerFingerprint =
        "sha256:"
        "2222222222222222222222222222222222222222222222222222222222222222",
    llvm::StringRef functionType = "() -> i32",
    llvm::StringRef parameterValue = "8") {
  return (llvm::Twine(R"json({
    "activation_sources": [],
    "binding": "Leaf",
    "binding_schema": "acsim-binding-0.1",
    "component_schema": ")json") +
          componentSchema + R"json(",
    "component_schema_fingerprint": "sha256:1111111111111111111111111111111111111111111111111111111111111111",
    "contract_epoch": "0.4",
    "effect": "pure",
    "function_type": ")json" +
          functionType + R"json(",
    "parameters": [{"acir_type":"i64","name":"width","ordinal":0,"value":)json" +
          parameterValue + R"json(}],
    "ports": [{
      "cardinality": "exclusive",
      "delegation": "forbidden",
      "direction": "input",
      "interface": "ac.Stream",
      "ownership": "borrowed",
      "payload": "ac.Packet",
      "protocol": "ac.ReadyValid",
      "role": "consumer",
      "time_domain": "ac.cycle"
    }],
    "provider": "gfsim",
    "provider_implementation_fingerprint": ")json" +
          providerFingerprint + R"json(",
    "resolution_key": "@Leaf",
    "resources": [],
    "results": [{"acir_type":"i32","name":"result"}]
  })json")
      .str();
}

std::string registryJson(std::vector<std::string> candidates,
                         std::vector<std::string> requests) {
  std::string result = R"({"candidates":[)";
  for (size_t index = 0; index < candidates.size(); ++index) {
    if (index)
      result.push_back(',');
    result.append(candidates[index]);
  }
  result.append(R"(],"requests":[)");
  for (size_t index = 0; index < requests.size(); ++index) {
    if (index)
      result.push_back(',');
    result.append(requests[index]);
  }
  result.append("]}");
  return result;
}

std::string registryJson(std::vector<std::string> candidates) {
  return registryJson(std::move(candidates), {requestJson()});
}

std::vector<BindingCandidate> parseCandidates(llvm::StringRef text) {
  auto parsed = parseBindingRegistry(text);
  EXPECT_TRUE(static_cast<bool>(parsed))
      << (parsed ? "" : takeError(parsed.takeError()));
  return parsed ? std::move(parsed->candidates)
                : std::vector<BindingCandidate>();
}

std::vector<BindingRequest> parseRequests(llvm::StringRef text) {
  auto parsed = parseBindingRegistry(text);
  EXPECT_TRUE(static_cast<bool>(parsed))
      << (parsed ? "" : takeError(parsed.takeError()));
  return parsed ? std::move(parsed->requests) : std::vector<BindingRequest>();
}

std::string withComponentSchema(llvm::StringRef text,
                                llvm::StringRef componentSchema) {
  auto parsed = parseIJson(text);
  EXPECT_TRUE(static_cast<bool>(parsed));
  if (!parsed)
    return {};
  auto *object = parsed->getAsObject();
  EXPECT_NE(nullptr, object);
  if (!object)
    return {};
  (*object)["component_schema"] = componentSchema;
  auto record = BindingRecord::parse(*object);
  EXPECT_TRUE(static_cast<bool>(record));
  if (!record)
    return {};
  auto fingerprint = computeBindingRecordFingerprint(*record);
  EXPECT_TRUE(static_cast<bool>(fingerprint));
  if (!fingerprint)
    return {};
  (*object)["fingerprint"] = *fingerprint;
  auto canonical = canonicalizeJson(*parsed);
  EXPECT_TRUE(static_cast<bool>(canonical));
  return canonical ? std::move(*canonical) : std::string();
}

std::string withUnitParameter(llvm::StringRef text) {
  auto parsed = parseIJson(text);
  EXPECT_TRUE(static_cast<bool>(parsed));
  if (!parsed)
    return {};
  auto *object = parsed->getAsObject();
  auto *parameters = object ? object->getArray("parameters") : nullptr;
  auto *construction = object ? object->getObject("construction") : nullptr;
  auto *arguments =
      construction ? construction->getArray("arguments") : nullptr;
  auto *parameter = parameters && parameters->size() == 1
                        ? (*parameters)[0].getAsObject()
                        : nullptr;
  EXPECT_NE(nullptr, parameter);
  EXPECT_NE(nullptr, arguments);
  if (!parameter || !arguments || arguments->size() != 1)
    return {};
  llvm::json::Object parameterValue;
  parameterValue["unit"] = "cycles";
  parameterValue["value"] = 4;
  (*parameter)["value"] = std::move(parameterValue);
  llvm::json::Object constructionValue;
  constructionValue["unit"] = "cycles";
  constructionValue["value"] = 4;
  (*arguments)[0] = std::move(constructionValue);
  auto record = BindingRecord::parse(*object);
  EXPECT_TRUE(static_cast<bool>(record));
  if (!record)
    return {};
  auto fingerprint = computeBindingRecordFingerprint(*record);
  EXPECT_TRUE(static_cast<bool>(fingerprint));
  if (!fingerprint)
    return {};
  (*object)["fingerprint"] = *fingerprint;
  auto canonical = canonicalizeJson(*parsed);
  EXPECT_TRUE(static_cast<bool>(canonical));
  return canonical ? std::move(*canonical) : std::string();
}

BindingRequest exactRequest() {
  auto requests = parseRequests(registryJson({candidateJson()}));
  EXPECT_EQ(1U, requests.size());
  return requests.empty() ? BindingRequest() : std::move(requests.front());
}

void writeTestFile(llvm::StringRef path, llvm::StringRef bytes) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error) << error.message();
  output << bytes;
  output.flush();
  ASSERT_FALSE(output.has_error());
}

std::string readTestFile(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path, false, false);
  EXPECT_TRUE(static_cast<bool>(buffer))
      << (buffer ? "" : buffer.getError().message());
  return buffer ? (*buffer)->getBuffer().str() : std::string();
}

void expectNoBindingTemporaries(llvm::StringRef directory) {
  std::error_code error;
  for (llvm::sys::fs::directory_iterator iterator(directory, error), end;
       iterator != end && !error; iterator.increment(error))
    EXPECT_EQ(llvm::StringRef::npos,
              llvm::sys::path::filename(iterator->path()).find(".tmp-"));
  EXPECT_FALSE(error) << error.message();
}

void expectCanonicalOutputBoundary(const llvm::json::Value &value,
                                   llvm::StringRef expected) {
  JsonParseLimits belowBoundary;
  belowBoundary.maxInputBytes = expected.size() - 1;
  {
    detail::ScopedCanonicalEmissionFailure denyEmission;
    auto rejected = canonicalizeJson(value, belowBoundary);
    ASSERT_FALSE(static_cast<bool>(rejected));
    EXPECT_TRUE(containsText(takeError(rejected.takeError()),
                             "canonical output byte limit exceeded"));
  }

  JsonParseLimits atBoundary;
  atBoundary.maxInputBytes = expected.size();
  {
    detail::ScopedCanonicalEmissionFailure denyEmission;
    auto attempted = canonicalizeJson(value, atBoundary);
    ASSERT_FALSE(static_cast<bool>(attempted));
    EXPECT_TRUE(containsText(takeError(attempted.takeError()),
                             "canonical emission failure injected"));
  }
  auto accepted = canonicalizeJson(value, atBoundary);
  ASSERT_TRUE(static_cast<bool>(accepted)) << takeError(accepted.takeError());
  EXPECT_EQ(expected, *accepted);
}

TEST(CanonicalJsonTest, ImplementsTheAuthoritativeRfc8785Vector) {
  constexpr llvm::StringLiteral input = R"json({
    "numbers": [333333333.33333329, 1E30, 4.50, 2e-3, 0.000000000000000000000000001],
    "string": "€$\u000f\nA'B\"\\\\\"/",
    "literals": [null, true, false]
  })json";
  constexpr llvm::StringLiteral expected =
      R"json({"literals":[null,true,false],"numbers":[333333333.3333333,1e+30,4.5,0.002,1e-27],"string":"€$\u000f\nA'B\"\\\\\"/"})json";

  auto canonical = canonicalizeJsonText(input);
  ASSERT_TRUE(static_cast<bool>(canonical)) << takeError(canonical.takeError());
  EXPECT_EQ(expected, *canonical);
  EXPECT_EQ(
      "sha256:2d5e01a318d0f0879ab568c4be289c8b1f64ef8921a53c6277d5e069978baacb",
      sha256Fingerprint(*canonical));
}

TEST(CanonicalJsonTest, ImplementsAuthoritativeRfc8785AppendixBNumbers) {
  struct Vector {
    uint64_t bits;
    llvm::StringLiteral expected;
  };
  constexpr Vector vectors[] = {
      {0x0000000000000000ULL, "0"},
      {0x0000000000000001ULL, "5e-324"},
      {0x8000000000000001ULL, "-5e-324"},
      {0x7fefffffffffffffULL, "1.7976931348623157e+308"},
      {0xffefffffffffffffULL, "-1.7976931348623157e+308"},
      {0x4340000000000000ULL, "9007199254740992"},
      {0xc340000000000000ULL, "-9007199254740992"},
      {0x4430000000000000ULL, "295147905179352830000"},
      {0x44b52d02c7e14af5ULL, "9.999999999999997e+22"},
      {0x44b52d02c7e14af6ULL, "1e+23"},
      {0x44b52d02c7e14af7ULL, "1.0000000000000001e+23"},
      {0x444b1ae4d6e2ef4eULL, "999999999999999700000"},
      {0x444b1ae4d6e2ef4fULL, "999999999999999900000"},
      {0x444b1ae4d6e2ef50ULL, "1e+21"},
      {0x3eb0c6f7a0b5ed8cULL, "9.999999999999997e-7"},
      {0x3eb0c6f7a0b5ed8dULL, "0.000001"},
      {0x41b3de4355555553ULL, "333333333.3333332"},
      {0x41b3de4355555554ULL, "333333333.33333325"},
      {0x41b3de4355555555ULL, "333333333.3333333"},
      {0x41b3de4355555556ULL, "333333333.3333334"},
      {0x41b3de4355555557ULL, "333333333.33333343"},
      {0xbecbf647612f3696ULL, "-0.0000033333333333333333"},
      {0x43143ff3c1cb0959ULL, "1424953923781206.2"},
  };
  for (const Vector &vector : vectors) {
    double value = std::bit_cast<double>(vector.bits);
    auto canonical = canonicalizeJson(llvm::json::Value(value));
    ASSERT_TRUE(static_cast<bool>(canonical))
        << takeError(canonical.takeError());
    EXPECT_EQ(vector.expected, *canonical) << vector.bits;
  }

  for (uint64_t bits :
       {0x8000000000000000ULL, 0x7fffffffffffffffULL, 0x7ff0000000000000ULL}) {
    auto rejected =
        canonicalizeJson(llvm::json::Value(std::bit_cast<double>(bits)));
    ASSERT_FALSE(static_cast<bool>(rejected)) << bits;
    EXPECT_TRUE(
        containsText(takeError(rejected.takeError()), "ACLOWER-BINDING-JSON"));
  }

  for (const auto &[input, expected] :
       std::array<std::pair<llvm::StringLiteral, llvm::StringLiteral>, 2>{
           std::pair{llvm::StringLiteral("9007199254740992"),
                     llvm::StringLiteral("9007199254740992")},
           std::pair{llvm::StringLiteral("9007199254740993"),
                     llvm::StringLiteral("9007199254740992")}}) {
    auto text = canonicalizeJsonText(input);
    ASSERT_TRUE(static_cast<bool>(text)) << takeError(text.takeError());
    EXPECT_EQ(expected, *text);
  }
}

TEST(CanonicalJsonTest, SortsObjectPropertiesByUtf16CodeUnitsRecursively) {
  constexpr llvm::StringLiteral input =
      R"json({"\ud83d\ude00":"emoji","\u20ac":"euro","\r":"control","1":"one","\u00f6":"latin","\ufb33":"hebrew","nested":{"b":0,"a":1}})json";
  constexpr llvm::StringLiteral expected =
      R"json({"\r":"control","1":"one","nested":{"a":1,"b":0},"ö":"latin","€":"euro","😀":"emoji","דּ":"hebrew"})json";

  auto canonical = canonicalizeJsonText(input);
  ASSERT_TRUE(static_cast<bool>(canonical)) << takeError(canonical.takeError());
  EXPECT_EQ(expected, *canonical);
}

TEST(CanonicalJsonTest, RejectsDuplicateInvalidUnsafeAndNegativeZeroInputs) {
  for (llvm::StringRef input :
       {R"({"x":1,"x":2})", R"("\udead")", "-0", "-0.0", "1e999"}) {
    auto parsed = parseIJson(input);
    EXPECT_FALSE(static_cast<bool>(parsed)) << input.str();
    EXPECT_TRUE(
        containsText(takeError(parsed.takeError()), "ACLOWER-BINDING-JSON"));
  }
}

TEST(CanonicalJsonTest, RejectsConstructedNonFiniteAndNegativeZeroValues) {
  for (double value : {std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::quiet_NaN(), -0.0}) {
    auto canonical = canonicalizeJson(llvm::json::Value(value));
    EXPECT_FALSE(static_cast<bool>(canonical));
    EXPECT_TRUE(
        containsText(takeError(canonical.takeError()), "ACLOWER-BINDING-JSON"));
  }
}

TEST(CanonicalJsonTest, AppliesEveryDeterministicResourceCap) {
  JsonParseLimits bytes;
  bytes.maxInputBytes = 2;
  auto tooManyBytes = parseIJson("null", bytes);
  ASSERT_FALSE(static_cast<bool>(tooManyBytes));
  EXPECT_TRUE(
      containsText(takeError(tooManyBytes.takeError()), "input byte limit"));

  JsonParseLimits shallow;
  shallow.maxDepth = 4;
  auto deep = parseIJson("[[[[[0]]]]]", shallow);
  ASSERT_FALSE(static_cast<bool>(deep));
  EXPECT_TRUE(containsText(takeError(deep.takeError()), "maximum depth"));

  JsonParseLimits small;
  small.maxStructuralWork = 8;
  auto large = parseIJson("[0,1,2,3,4,5,6,7,8]", small);
  ASSERT_FALSE(static_cast<bool>(large));
  EXPECT_TRUE(
      containsText(takeError(large.takeError()), "structural work limit"));

  JsonParseLimits longString;
  longString.maxStringBytes = 1;
  auto oversizedString = parseIJson(R"("ab")", longString);
  ASSERT_FALSE(static_cast<bool>(oversizedString));
  EXPECT_TRUE(containsText(takeError(oversizedString.takeError()),
                           "string byte limit"));

  JsonParseLimits totalStrings;
  totalStrings.maxStringBytes = 8;
  totalStrings.maxTotalStringBytes = 3;
  auto oversizedTotal = parseIJson(R"(["ab","cd"])", totalStrings);
  ASSERT_FALSE(static_cast<bool>(oversizedTotal));
  EXPECT_TRUE(containsText(takeError(oversizedTotal.takeError()),
                           "total string byte limit"));

  JsonParseLimits arrayElements;
  arrayElements.maxArrayElements = 2;
  auto oversizedArray = parseIJson("[0,1,2]", arrayElements);
  ASSERT_FALSE(static_cast<bool>(oversizedArray));
  EXPECT_TRUE(containsText(takeError(oversizedArray.takeError()),
                           "array element limit"));

  JsonParseLimits objectMembers;
  objectMembers.maxObjectMembers = 1;
  auto oversizedObject = parseIJson(R"({"a":0,"b":1})", objectMembers);
  ASSERT_FALSE(static_cast<bool>(oversizedObject));
  EXPECT_TRUE(containsText(takeError(oversizedObject.takeError()),
                           "object member limit"));
}

TEST(CanonicalJsonTest, BoundsEveryConstructedDomBeforeCanonicalization) {
  auto expectLimit = [](llvm::json::Value value, JsonParseLimits limits,
                        llvm::StringRef message) {
    auto canonical = canonicalizeJson(value, limits);
    ASSERT_FALSE(static_cast<bool>(canonical));
    EXPECT_TRUE(containsText(takeError(canonical.takeError()), message.str()));
  };

  JsonParseLimits depth;
  depth.maxDepth = 3;
  llvm::json::Value nested = nullptr;
  for (size_t index = 0; index < 3; ++index) {
    llvm::json::Array wrapper;
    wrapper.push_back(std::move(nested));
    nested = llvm::json::Value(std::move(wrapper));
  }
  expectLimit(std::move(nested), depth, "maximum depth");

  JsonParseLimits work;
  work.maxStructuralWork = 3;
  expectLimit(llvm::json::Array({nullptr, nullptr, nullptr}), work,
              "structural work");

  JsonParseLimits string;
  string.maxStringBytes = 1;
  expectLimit("ab", string, "string byte limit");

  JsonParseLimits totalStrings;
  totalStrings.maxStringBytes = 8;
  totalStrings.maxTotalStringBytes = 3;
  expectLimit(llvm::json::Array({"ab", "cd"}), totalStrings,
              "total string byte limit");

  JsonParseLimits array;
  array.maxArrayElements = 2;
  expectLimit(llvm::json::Array({0, 1, 2}), array, "array element limit");

  JsonParseLimits object;
  object.maxObjectMembers = 1;
  llvm::json::Object members;
  members["a"] = 0;
  members["b"] = 1;
  expectLimit(std::move(members), object, "object member limit");

  JsonParseLimits output;
  output.maxInputBytes = 3;
  expectLimit("ab", output, "canonical output byte limit");
}

TEST(CanonicalJsonTest, CountsContainerPunctuationBeforeRecursiveEmission) {
  llvm::json::Object object;
  object["a"] = llvm::json::Array({nullptr, true, false, 1});
  expectCanonicalOutputBoundary(llvm::json::Value(std::move(object)),
                                R"json({"a":[null,true,false,1]})json");
}

TEST(CanonicalJsonTest, CountsEscapedStringBytesBeforeRecursiveEmission) {
  expectCanonicalOutputBoundary(std::string("\"\\\n"), R"json("\"\\\n")json");
}

TEST(BindingRecordTest, BoundsConstructedStaticValuesBeforeSemanticRecursion) {
  auto parsed = parseIJson(recordJson());
  ASSERT_TRUE(static_cast<bool>(parsed)) << takeError(parsed.takeError());
  auto *object = parsed->getAsObject();
  ASSERT_NE(nullptr, object);
  auto *parameters = object->getArray("parameters");
  ASSERT_NE(nullptr, parameters);
  auto *parameter = (*parameters)[0].getAsObject();
  ASSERT_NE(nullptr, parameter);
  (*parameter)["value"] = llvm::json::Array({0, 1, 2});

  JsonParseLimits limits;
  limits.maxArrayElements = 2;
  auto record = BindingRecord::parse(*object, limits);
  ASSERT_FALSE(static_cast<bool>(record));
  EXPECT_TRUE(
      containsText(takeError(record.takeError()), "array element limit"));
}

TEST(BindingRecordTest, RestrictsSafeIntegerRangeOnlyForStaticMetadata) {
  auto parsed = parseIJson(recordJson());
  ASSERT_TRUE(static_cast<bool>(parsed)) << takeError(parsed.takeError());
  auto *object = parsed->getAsObject();
  ASSERT_NE(nullptr, object);
  auto *parameters = object->getArray("parameters");
  ASSERT_NE(nullptr, parameters);
  auto *parameter = (*parameters)[0].getAsObject();
  ASSERT_NE(nullptr, parameter);
  (*parameter)["value"] = INT64_C(9007199254740992);

  auto record = BindingRecord::parse(*object);
  ASSERT_FALSE(static_cast<bool>(record));
  EXPECT_TRUE(containsText(takeError(record.takeError()), "safe exact range"));
}

TEST(BindingRecordTest, AppliesCanonicalOutputLimitBeforeSemanticRecursion) {
  auto parsed = parseIJson(recordJson());
  ASSERT_TRUE(static_cast<bool>(parsed)) << takeError(parsed.takeError());
  auto *object = parsed->getAsObject();
  ASSERT_NE(nullptr, object);

  JsonParseLimits limits;
  limits.maxInputBytes = 0;
  auto record = BindingRecord::parse(*object, limits);
  ASSERT_FALSE(static_cast<bool>(record));
  EXPECT_TRUE(containsText(takeError(record.takeError()),
                           "canonical output byte limit exceeded"));
}

TEST(BindingRecordTest, ParsesTheClosedTypedMetadataRecord) {
  auto candidates = parseCandidates(registryJson({candidateJson()}));
  ASSERT_EQ(1U, candidates.size());
  const BindingRecord &record = candidates.front().record();
  EXPECT_EQ("acsim-binding-0.1", record.bindingSchema());
  EXPECT_EQ("0.4", record.contractEpoch());
  EXPECT_EQ("Leaf", record.binding());
  EXPECT_EQ("pure", record.effect());
  EXPECT_EQ("gfsim::Leaf", record.cpp().symbol);
  ASSERT_EQ(1U, record.parameters().size());
  EXPECT_EQ("constructor_constant", record.parameters().front().mapping);
  ASSERT_EQ(1U, record.ports().size());
  EXPECT_EQ("exclusive", record.ports().front().cardinality);
  EXPECT_EQ(kRecordFingerprint, record.fingerprint());

  auto unitCandidates = parseCandidates(registryJson({candidateJson(
      "fast", "arm64-apple-darwin", true, withUnitParameter(recordJson()))}));
  ASSERT_EQ(1U, unitCandidates.size());
  ASSERT_EQ(1U, unitCandidates.front().record().parameters().size());
  EXPECT_NE(
      nullptr,
      unitCandidates.front().record().parameters().front().value.getAsObject());
}

TEST(BindingRecordTest, RejectsUnknownFieldsAndBehavioralCppFragments) {
  std::string unknown = recordJson();
  unknown.insert(unknown.find("\"activation_sources\""),
                 "\"emitter_callback\":\"emitLeaf\",");
  auto parsedUnknown = parseBindingRegistry(registryJson(
      {candidateJson("fast", "arm64-apple-darwin", true, unknown)}));
  ASSERT_FALSE(static_cast<bool>(parsedUnknown));
  EXPECT_TRUE(containsText(takeError(parsedUnknown.takeError()),
                           "exactly the acsim-binding-0.1 fields"));

  for (llvm::StringRef raw : {"gfsim::Leaf()", "Leaf{x}", "#define Leaf X",
                              "leaf = callback", "emit(%s)"}) {
    auto parsed = parseBindingRegistry(
        registryJson({candidateJson("fast", "arm64-apple-darwin", true,
                                    recordJson(kRecordFingerprint, raw))}));
    ASSERT_FALSE(static_cast<bool>(parsed)) << raw.str();
    EXPECT_TRUE(containsText(takeError(parsed.takeError()),
                             "raw C++ or emitter behavior"));
  }

  std::string traversal = recordJson();
  traversal.replace(traversal.find("gfsim/leaf.hpp"),
                    llvm::StringRef("gfsim/leaf.hpp").size(), "include/..");
  auto parsedTraversal = parseBindingRegistry(registryJson(
      {candidateJson("fast", "arm64-apple-darwin", true, traversal)}));
  ASSERT_FALSE(static_cast<bool>(parsedTraversal));
  EXPECT_TRUE(
      containsText(takeError(parsedTraversal.takeError()), "header path"));

  std::string rawParameterType = recordJson();
  rawParameterType.replace(rawParameterType.find("std::int64_t"),
                           llvm::StringRef("std::int64_t").size(),
                           "std::int64_t;emit()");
  auto parsedParameterType = parseBindingRegistry(registryJson(
      {candidateJson("fast", "arm64-apple-darwin", true, rawParameterType)}));
  ASSERT_FALSE(static_cast<bool>(parsedParameterType));
  EXPECT_TRUE(containsText(takeError(parsedParameterType.takeError()),
                           "raw C++ or emitter behavior"));
}

TEST(BindingRecordTest, RejectsDuplicateStatefulActivationSourceNames) {
  auto parsed = parseIJson(recordJson());
  ASSERT_TRUE(static_cast<bool>(parsed)) << takeError(parsed.takeError());
  auto *object = parsed->getAsObject();
  ASSERT_NE(nullptr, object);
  (*object)["effect"] = "stateful";
  auto *cpp = object->getObject("cpp");
  auto *entryPoints = cpp ? cpp->getObject("entry_points") : nullptr;
  auto *ownership = object->getObject("ownership");
  ASSERT_NE(nullptr, entryPoints);
  ASSERT_NE(nullptr, ownership);
  (*entryPoints)["pure"] = "";
  (*entryPoints)["work"] = "gfsim::work";
  (*entryPoints)["xfer"] = "gfsim::xfer";
  (*ownership)["kind"] = "unique";
  (*ownership)["placement"] = "member_or_array";
  llvm::json::Array activations;
  activations.push_back(
      llvm::json::Object({{"kind", "ac.Clock"}, {"name", "wake"}}));
  activations.push_back(
      llvm::json::Object({{"kind", "ac.Reset"}, {"name", "wake"}}));
  (*object)["activation_sources"] = std::move(activations);

  auto duplicate = BindingRecord::parse(*object);
  ASSERT_FALSE(static_cast<bool>(duplicate));
  EXPECT_TRUE(containsText(takeError(duplicate.takeError()),
                           "activation-source names must be unique"));

  auto *sources = object->getArray("activation_sources");
  ASSERT_NE(nullptr, sources);
  auto *reset = (*sources)[1].getAsObject();
  ASSERT_NE(nullptr, reset);
  (*reset)["name"] = "reset";
  auto unique = BindingRecord::parse(*object);
  ASSERT_TRUE(static_cast<bool>(unique)) << takeError(unique.takeError());
}

TEST(BindingRecordTest, RejectsMalformedRegistryEnvelopeAndResourceCaps) {
  auto objectInsteadOfArray = parseBindingRegistry(candidateJson());
  ASSERT_FALSE(static_cast<bool>(objectInsteadOfArray));
  EXPECT_TRUE(
      containsText(takeError(objectInsteadOfArray.takeError()),
                   "registry must contain exactly candidates and requests"));

  std::string requestWithCppOutput = requestJson();
  requestWithCppOutput.insert(requestWithCppOutput.find("\"binding\""),
                              "\"cpp_type\":\"cpp_i32\",");
  auto cppAuthoredRequest =
      parseBindingRegistry(registryJson({}, {std::move(requestWithCppOutput)}));
  ASSERT_FALSE(static_cast<bool>(cppAuthoredRequest));
  EXPECT_TRUE(containsText(takeError(cppAuthoredRequest.takeError()),
                           "exactly the frozen architecture fields"));

  std::string requestWithAccessor = requestJson();
  requestWithAccessor.insert(requestWithAccessor.find("\"cardinality\""),
                             "\"accessor\":\"input\",");
  auto accessorAuthoredRequest =
      parseBindingRegistry(registryJson({}, {std::move(requestWithAccessor)}));
  ASSERT_FALSE(static_cast<bool>(accessorAuthoredRequest));
  EXPECT_TRUE(
      containsText(takeError(accessorAuthoredRequest.takeError()),
                   "request port must contain exact architecture fields"));

  JsonParseLimits limits;
  limits.maxInputBytes = 32;
  auto tooLarge = parseBindingRegistry(registryJson({candidateJson()}), limits);
  ASSERT_FALSE(static_cast<bool>(tooLarge));
  EXPECT_TRUE(
      containsText(takeError(tooLarge.takeError()), "input byte limit"));
}

TEST(BindingCandidateTest, RejectsOversizedConstructedProfileAndTarget) {
  auto parsed = parseIJson(candidateJson());
  ASSERT_TRUE(static_cast<bool>(parsed)) << takeError(parsed.takeError());
  auto *object = parsed->getAsObject();
  ASSERT_NE(nullptr, object);

  JsonParseLimits limits;
  limits.maxStringBytes = 71;
  for (llvm::StringRef key : {"profile", "target"}) {
    (*object)[key] = std::string(72, key.front());
    auto candidate = BindingCandidate::parse(*object, limits);
    ASSERT_FALSE(static_cast<bool>(candidate)) << key.str();
    EXPECT_TRUE(containsText(takeError(candidate.takeError()),
                             "string byte limit exceeded"));
    (*object)[key] = key == "profile" ? "fast" : "arm64-apple-darwin";
  }
}

TEST(BindingCandidateTest, AppliesDepthAndOutputLimitsToCompleteEnvelope) {
  auto parsed = parseIJson(candidateJson());
  ASSERT_TRUE(static_cast<bool>(parsed)) << takeError(parsed.takeError());
  auto *object = parsed->getAsObject();
  ASSERT_NE(nullptr, object);

  JsonParseLimits depth;
  depth.maxDepth = 4;
  auto tooDeep = BindingCandidate::parse(*object, depth);
  ASSERT_FALSE(static_cast<bool>(tooDeep));
  EXPECT_TRUE(
      containsText(takeError(tooDeep.takeError()), "maximum depth exceeded"));

  JsonParseLimits output;
  output.maxInputBytes = 0;
  auto tooLarge = BindingCandidate::parse(*object, output);
  ASSERT_FALSE(static_cast<bool>(tooLarge));
  EXPECT_TRUE(containsText(takeError(tooLarge.takeError()),
                           "canonical output byte limit exceeded"));
}

TEST(BindingRegistryTest, ExactSelectionIsIndependentOfProviderOrder) {
  std::string fast = candidateJson();
  std::string validated = candidateJson("validated");
  auto forward = parseCandidates(registryJson({validated, fast}));
  auto reverse = parseCandidates(registryJson({fast, validated}));
  BindingRequest request = exactRequest();

  auto first =
      resolveBindings(forward, {request}, "fast", "arm64-apple-darwin");
  auto second =
      resolveBindings(reverse, {request}, "fast", "arm64-apple-darwin");
  ASSERT_TRUE(static_cast<bool>(first)) << takeError(first.takeError());
  ASSERT_TRUE(static_cast<bool>(second)) << takeError(second.takeError());
  EXPECT_EQ(first->canonicalLock(), second->canonicalLock());
  EXPECT_EQ(first->lockFingerprint(), second->lockFingerprint());
  ASSERT_EQ(1U, first->selections().size());
  EXPECT_EQ("Leaf", first->selections().front().record().binding());
}

TEST(BindingResolutionResultTest, LooksUpSelectionsOnlyByExactResolutionKey) {
  auto candidates = parseCandidates(registryJson({candidateJson()}));
  auto result = resolveBindings(candidates, {exactRequest()}, "fast",
                                "arm64-apple-darwin");
  ASSERT_TRUE(static_cast<bool>(result)) << takeError(result.takeError());

  const ResolvedBinding *selection = result->selectionForResolutionKey("@Leaf");
  ASSERT_NE(nullptr, selection);
  EXPECT_EQ("Leaf", selection->record().binding());
  EXPECT_EQ(nullptr, result->selectionForResolutionKey("Leaf"));
  EXPECT_EQ(nullptr, result->selectionForResolutionKey("@Top::@Leaf"));
  EXPECT_EQ(nullptr, result->selectionForResolutionKey("@Missing"));
}

TEST(BindingResolutionResultTest, EmptyResultIsCanonicalAndLookupIsMissing) {
  auto first = resolveBindings({}, {}, "fast", "arm64-apple-darwin");
  auto second = resolveBindings({}, {}, "fast", "arm64-apple-darwin");
  ASSERT_TRUE(static_cast<bool>(first)) << takeError(first.takeError());
  ASSERT_TRUE(static_cast<bool>(second)) << takeError(second.takeError());

  EXPECT_TRUE(first->selections().empty());
  EXPECT_EQ(nullptr, first->selectionForResolutionKey("@Anything"));
  EXPECT_EQ("[]", first->canonicalLock());
  EXPECT_EQ("sha256:4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11b"
            "a873c2f11161202b945",
            first->lockFingerprint().str());
  EXPECT_EQ(first->canonicalLock(), second->canonicalLock());
  EXPECT_EQ(first->lockFingerprint(), second->lockFingerprint());
}

TEST(BindingRegistryTest, MissingUnavailableAndAmbiguousAreHardFailures) {
  auto candidates = parseCandidates(registryJson({candidateJson()}));
  BindingRequest request = exactRequest();

  auto missing = resolveBindings({}, {request}, "fast", "arm64-apple-darwin");
  ASSERT_FALSE(static_cast<bool>(missing));
  EXPECT_TRUE(
      containsText(takeError(missing.takeError()), "ACLOWER-BINDING-MISSING"));

  auto unavailable = parseCandidates(
      registryJson({candidateJson("fast", "arm64-apple-darwin", false)}));
  auto noAvailable =
      resolveBindings(unavailable, {request}, "fast", "arm64-apple-darwin");
  ASSERT_FALSE(static_cast<bool>(noAvailable));
  EXPECT_TRUE(containsText(takeError(noAvailable.takeError()),
                           "ACLOWER-BINDING-MISSING"));

  auto ambiguous =
      parseCandidates(registryJson({candidateJson(), candidateJson()}));
  auto multiple =
      resolveBindings(ambiguous, {request}, "fast", "arm64-apple-darwin");
  ASSERT_FALSE(static_cast<bool>(multiple));
  EXPECT_TRUE(containsText(takeError(multiple.takeError()),
                           "ACLOWER-BINDING-AMBIGUOUS"));
}

TEST(BindingRegistryTest,
     ClassifiesMalformedInputsBeforeExactMatchCardinality) {
  auto candidates = parseCandidates(registryJson({candidateJson()}));
  BindingRequest exact = exactRequest();
  struct Malformed {
    llvm::StringLiteral code;
    void (*mutate)(BindingRequest &);
  };
  const Malformed malformed[] = {
      {"ACLOWER-EPOCH-MISMATCH",
       [](BindingRequest &request) { request.contractEpoch = "0.1"; }},
      {"ACLOWER-SCHEMA-MISMATCH",
       [](BindingRequest &request) {
         request.bindingSchema = "acsim-binding-0.2";
       }},
      {"ACLOWER-INLINE-EFFECT",
       [](BindingRequest &request) { request.effect = "invalid"; }},
      {"ACLOWER-FINGERPRINT",
       [](BindingRequest &request) {
         request.providerImplementationFingerprint = "not-a-fingerprint";
       }},
  };

  for (const Malformed &input : malformed) {
    BindingRequest request = exact;
    input.mutate(request);
    auto result =
        resolveBindings(candidates, {request}, "fast", "arm64-apple-darwin");
    ASSERT_FALSE(static_cast<bool>(result)) << input.code.str();
    EXPECT_TRUE(
        startsWithText(takeError(result.takeError()), input.code.str()));
  }

  auto invalidProfile =
      resolveBindings(candidates, {exact}, "fast debug", "arm64-apple-darwin");
  ASSERT_FALSE(static_cast<bool>(invalidProfile));
  EXPECT_TRUE(
      startsWithText(takeError(invalidProfile.takeError()), "ACLOWER-PROFILE"));
}

TEST(BindingRegistryTest, ZeroExactMatchesAreMissingWithSubordinateReason) {
  auto candidates = parseCandidates(registryJson({candidateJson()}));
  BindingRequest exact = exactRequest();
  struct Mismatch {
    llvm::StringLiteral reason;
    void (*mutate)(BindingRequest &);
  };
  const Mismatch mismatches[] = {
      {"ACLOWER-SCHEMA-MISMATCH",
       [](BindingRequest &request) { request.componentSchema = "ac.Other"; }},
      {"ACLOWER-INLINE-EFFECT",
       [](BindingRequest &request) { request.effect = "stateful"; }},
      {"ACLOWER-PARAM-PHASE",
       [](BindingRequest &request) { request.parameters.clear(); }},
      {"ACLOWER-TYPE-MISMATCH",
       [](BindingRequest &request) { request.results.front().name = "other"; }},
      {"ACLOWER-TYPE-MISMATCH",
       [](BindingRequest &request) { request.ports.clear(); }},
      {"ACLOWER-FINGERPRINT",
       [](BindingRequest &request) {
         request.providerImplementationFingerprint =
             "sha256:"
             "3333333333333333333333333333333333333333333333333333333333333333";
       }},
  };

  for (const Mismatch &mismatch : mismatches) {
    BindingRequest request = exact;
    mismatch.mutate(request);
    auto result =
        resolveBindings(candidates, {request}, "fast", "arm64-apple-darwin");
    ASSERT_FALSE(static_cast<bool>(result)) << mismatch.reason.str();
    std::string message = takeError(result.takeError());
    EXPECT_TRUE(startsWithText(message, "ACLOWER-BINDING-MISSING"));
    EXPECT_TRUE(containsText(message,
                             (llvm::Twine("reason=") + mismatch.reason).str()));
    EXPECT_TRUE(containsText(message, "key=@Leaf"));
  }

  for (auto [profile, target] : {std::pair<llvm::StringRef, llvm::StringRef>(
                                     "validated", "arm64-apple-darwin"),
                                 {"fast", "x86_64-linux-gnu"}}) {
    auto result = resolveBindings(candidates, {exact}, profile, target);
    ASSERT_FALSE(static_cast<bool>(result));
    std::string message = takeError(result.takeError());
    EXPECT_TRUE(startsWithText(message, "ACLOWER-BINDING-MISSING"));
    EXPECT_TRUE(containsText(message, "reason=ACLOWER-PROFILE"));
  }
}

TEST(BindingRegistryTest, NarrowsOneCandidateSetAcrossEveryExactField) {
  BindingRequest request = exactRequest();
  std::string otherSchema = withComponentSchema(recordJson(), "ac.OtherLeaf");
  auto candidates = parseCandidates(registryJson(
      {candidateJson("validated"),
       candidateJson("fast", "arm64-apple-darwin", true, otherSchema)}));

  auto result =
      resolveBindings(candidates, {request}, "fast", "arm64-apple-darwin");
  ASSERT_FALSE(static_cast<bool>(result));
  EXPECT_TRUE(containsText(takeError(result.takeError()), "ACLOWER-PROFILE"));
}

TEST(BindingRegistryTest, RejectsIncorrectRecordFingerprintBeforeSelection) {
  auto candidates = parseCandidates(registryJson(
      {candidateJson("fast", "arm64-apple-darwin", true,
                     recordJson("sha256:"
                                "ffffffffffffffffffffffffffffffffffffffffffffff"
                                "ffffffffffffffffff"))}));
  BindingRequest request = exactRequest();
  auto result =
      resolveBindings(candidates, {request}, "fast", "arm64-apple-darwin");
  ASSERT_FALSE(static_cast<bool>(result));
  EXPECT_TRUE(
      containsText(takeError(result.takeError()), "ACLOWER-FINGERPRINT"));
}

TEST(BindingLockTest, EmitsStableCanonicalBytesAndProjectHashVector) {
  auto candidates = parseCandidates(registryJson({candidateJson()}));
  auto result = resolveBindings(candidates, {exactRequest()}, "fast",
                                "arm64-apple-darwin");
  ASSERT_TRUE(static_cast<bool>(result)) << takeError(result.takeError());
  EXPECT_EQ(
      "sha256:0ee9be5714c9d07718ab3ee9971bfc9d1f549350155bd2076b200e1532b3537b",
      result->lockFingerprint());
  EXPECT_EQ('[', result->canonicalLock().front());
  EXPECT_EQ(']', result->canonicalLock().back());

  std::string streamed;
  llvm::raw_string_ostream output(streamed);
  EXPECT_FALSE(static_cast<bool>(emitBindingLock(*result, output)));
  output.flush();
  EXPECT_EQ(result->canonicalLock(), streamed);
}

TEST(BindingLockTest, PublishesAtomicallyOnlyAfterCompleteResolution) {
  llvm::SmallString<128> directory;
  ASSERT_FALSE(
      llvm::sys::fs::createUniqueDirectory("acir-bindings", directory));
  llvm::SmallString<160> successPath(directory);
  llvm::sys::path::append(successPath, "acsim-bindings.lock.json");
  llvm::SmallString<160> failurePath(directory);
  llvm::sys::path::append(failurePath, "failed.lock.json");
  llvm::SmallString<160> existingPath(directory);
  llvm::sys::path::append(existingPath, "existing.lock.json");

  auto candidates = parseCandidates(registryJson({candidateJson()}));
  BindingRequest request = exactRequest();
  EXPECT_FALSE(static_cast<bool>(resolveAndWriteBindingLock(
      candidates, {request}, "fast", "arm64-apple-darwin", successPath)));
  EXPECT_TRUE(llvm::sys::fs::exists(successPath));

  BindingRequest missing = request;
  missing.binding = "Missing";
  llvm::Error failure = resolveAndWriteBindingLock(
      candidates, {missing}, "fast", "arm64-apple-darwin", failurePath);
  ASSERT_TRUE(static_cast<bool>(failure));
  EXPECT_TRUE(
      containsText(takeError(std::move(failure)), "ACLOWER-BINDING-MISSING"));
  EXPECT_FALSE(llvm::sys::fs::exists(failurePath));

  constexpr llvm::StringLiteral sentinel = "existing-lock-sentinel\n";
  writeTestFile(existingPath, sentinel);
  llvm::Error existingResolutionFailure = resolveAndWriteBindingLock(
      candidates, {missing}, "fast", "arm64-apple-darwin", existingPath);
  ASSERT_TRUE(static_cast<bool>(existingResolutionFailure));
  EXPECT_TRUE(containsText(takeError(std::move(existingResolutionFailure)),
                           "ACLOWER-BINDING-MISSING"));
  EXPECT_EQ(sentinel, readTestFile(existingPath));
  expectNoBindingTemporaries(directory);

  llvm::SmallString<160> invalidParent(directory);
  llvm::sys::path::append(invalidParent, "missing", "lock.json");
  auto result =
      resolveBindings(candidates, {request}, "fast", "arm64-apple-darwin");
  ASSERT_TRUE(static_cast<bool>(result)) << takeError(result.takeError());
  llvm::Error writeFailure = emitBindingLockAtomically(*result, invalidParent);
  ASSERT_TRUE(static_cast<bool>(writeFailure));
  EXPECT_TRUE(containsText(takeError(std::move(writeFailure)),
                           "ACLOWER-BINDING-OUTPUT"));

  {
    detail::ScopedBindingPublishFailure failure;
    llvm::Error publishFailure =
        emitBindingLockAtomically(*result, existingPath);
    ASSERT_TRUE(static_cast<bool>(publishFailure));
    EXPECT_TRUE(containsText(takeError(std::move(publishFailure)),
                             "ACLOWER-BINDING-OUTPUT"));
  }
  EXPECT_EQ(sentinel, readTestFile(existingPath));
  expectNoBindingTemporaries(directory);
}

TEST(ResolveBindingsApiTest, ReturnsTypedResultWithoutMutatingFrozenTopology) {
  mlir::DialectRegistry dialects;
  acir::registerAllDialects(dialects);
  mlir::MLIRContext context(dialects);
  context.loadAllAvailableDialects();
  acir::ac::getStructuralProviderRegistry(&context).registerExternal("Leaf");
  constexpr llvm::StringLiteral source = R"mlir(
    builtin.module attributes {ac.contract_epoch = "0.4"} {
      ac.system @soc root @Top as "root" tick 0 "cycle"
          workload @Top::@workload seed {kind = "fixed", value = 0 : i64}
          instrumentation [] results {id = "default", format = "json"}
          selected true
      ac.module.extern @Leaf : () -> i32 parameters {width = 8 : i64}
          implementation {registry = "cpp", name = "Leaf"}
      ac.module @Top() parameters {} graph {
        %leaf = ac.instance @leaf of @Leaf() static {width = 8 : i64}
            id "leaf" path "leaf" : () -> i32
        ac.process @workload kind "workload" { ac.yield_sim }
        ac.return
      }
    }
  )mlir";
  auto module = mlir::parseSourceString<mlir::ModuleOp>(source, &context);
  ASSERT_TRUE(module);

  ResolveBindingsPassOptions options;
  options.candidates = parseCandidates(registryJson({candidateJson()}));
  options.requests = parseRequests(registryJson({candidateJson()}));
  options.profile = "fast";
  options.target = "arm64-apple-darwin";

  auto beforeFreeze = resolveModuleBindings(*module, options);
  ASSERT_FALSE(static_cast<bool>(beforeFreeze));
  EXPECT_TRUE(containsText(takeError(beforeFreeze.takeError()),
                           "ACLOWER-BINDING-MISSING"));

  mlir::PassManager manager(&context);
  manager.enableVerifier(false);
  manager.addPass(createFreezeTopologyPass());
  ASSERT_TRUE(mlir::succeeded(manager.run(*module)));
  mlir::Attribute digest = (*module)->getAttr("ac.topology_digest");

  auto result = resolveModuleBindings(*module, options);
  ASSERT_TRUE(static_cast<bool>(result)) << takeError(result.takeError());
  ASSERT_EQ(1U, result->selections().size());
  EXPECT_EQ("Leaf", result->selections().front().record().binding());
  EXPECT_EQ(digest, (*module)->getAttr("ac.topology_digest"));
  bool containsACSimOperation = false;
  module->walk([&](mlir::Operation *operation) {
    if (operation->getName().getDialectNamespace() == "acsim")
      containsACSimOperation = true;
  });
  EXPECT_FALSE(containsACSimOperation);

  ResolveBindingsPassOptions ambiguousOptions = options;
  ambiguousOptions.candidates =
      parseCandidates(registryJson({candidateJson(), candidateJson()}));
  auto ambiguous = resolveModuleBindings(*module, ambiguousOptions);
  ASSERT_FALSE(static_cast<bool>(ambiguous));
  EXPECT_TRUE(containsText(takeError(ambiguous.takeError()),
                           "ACLOWER-BINDING-AMBIGUOUS"));

  ResolveBindingsPassOptions absentRequest = options;
  absentRequest.requests.clear();
  auto absent = resolveModuleBindings(*module, absentRequest);
  ASSERT_FALSE(static_cast<bool>(absent));
  EXPECT_TRUE(containsText(takeError(absent.takeError()),
                           "exact frozen architecture request is absent"));

  ResolveBindingsPassOptions schemaMismatch = options;
  schemaMismatch.candidates = parseCandidates(registryJson(
      {candidateJson("fast", "arm64-apple-darwin", true,
                     withComponentSchema(recordJson(), "ac.OtherLeaf"))}));
  auto wrongSchema = resolveModuleBindings(*module, schemaMismatch);
  ASSERT_FALSE(static_cast<bool>(wrongSchema));
  EXPECT_TRUE(containsText(takeError(wrongSchema.takeError()),
                           "ACLOWER-SCHEMA-MISMATCH"));

  ResolveBindingsPassOptions providerMismatch = options;
  providerMismatch.requests.front().providerImplementationFingerprint =
      "sha256:3333333333333333333333333333333333333333333333333333333333333333";
  auto wrongProvider = resolveModuleBindings(*module, providerMismatch);
  ASSERT_FALSE(static_cast<bool>(wrongProvider));
  EXPECT_TRUE(containsText(takeError(wrongProvider.takeError()),
                           "ACLOWER-FINGERPRINT"));

  ResolveBindingsPassOptions signatureMismatch = options;
  signatureMismatch.requests.front().functionType = "() -> i64";
  auto wrongSignature = resolveModuleBindings(*module, signatureMismatch);
  ASSERT_FALSE(static_cast<bool>(wrongSignature));
  EXPECT_TRUE(containsText(takeError(wrongSignature.takeError()),
                           "ACLOWER-TYPE-MISMATCH"));

  std::string unitSource = source.str();
  unitSource.replace(unitSource.find("width = 8 : i64"),
                     llvm::StringRef("width = 8 : i64").size(),
                     "width = {unit = \"cycles\", value = 4 : i64}");
  unitSource.replace(unitSource.find("width = 8 : i64"),
                     llvm::StringRef("width = 8 : i64").size(),
                     "width = {unit = \"cycles\", value = 4 : i64}");
  auto unitModule =
      mlir::parseSourceString<mlir::ModuleOp>(unitSource, &context);
  ASSERT_TRUE(unitModule);
  mlir::PassManager unitManager(&context);
  unitManager.enableVerifier(false);
  unitManager.addPass(createFreezeTopologyPass());
  ASSERT_TRUE(mlir::succeeded(unitManager.run(*unitModule)));
  ResolveBindingsPassOptions unitOptions = options;
  std::string unitRegistry = registryJson(
      {candidateJson("fast", "arm64-apple-darwin", true,
                     withUnitParameter(recordJson()))},
      {requestJson(
          "ac.Leaf",
          "sha256:"
          "2222222222222222222222222222222222222222222222222222222222222222",
          "() -> i32", R"({"unit":"cycles","value":4})")});
  unitOptions.candidates = parseCandidates(unitRegistry);
  unitOptions.requests = parseRequests(unitRegistry);
  auto unitResult = resolveModuleBindings(*unitModule, unitOptions);
  ASSERT_TRUE(static_cast<bool>(unitResult))
      << takeError(unitResult.takeError());
}

} // namespace
} // namespace acir::bindings
