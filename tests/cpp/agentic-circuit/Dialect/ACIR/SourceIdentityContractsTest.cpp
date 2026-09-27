#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "gtest/gtest.h"

#include <initializer_list>
#include <string>
#include <utility>

namespace acir::ac {
namespace {

using Field = std::pair<llvm::StringRef, mlir::Attribute>;
using EmitError = llvm::function_ref<mlir::InFlightDiagnostic()>;

mlir::DictionaryAttr dictionary(mlir::OpBuilder &builder,
                                std::initializer_list<Field> fields) {
  llvm::SmallVector<mlir::NamedAttribute> attributes;
  for (const auto &[name, value] : fields)
    attributes.push_back(builder.getNamedAttr(name, value));
  return builder.getDictionaryAttr(attributes);
}

llvm::APSInt signedInteger(unsigned width, llvm::StringRef spelling) {
  bool negative = spelling.consume_front("-");
  llvm::APInt bits(width, spelling, 10);
  if (negative)
    bits = -bits;
  return llvm::APSInt(std::move(bits), /*isUnsigned=*/false);
}

MathIntAttr math(mlir::MLIRContext &context, llvm::StringRef spelling) {
  return MathIntAttr::get(
      &context,
      signedInteger(static_cast<unsigned>(spelling.size() * 4 + 2), spelling));
}

mlir::IntegerAttr integer(mlir::OpBuilder &builder, unsigned width,
                          const llvm::APInt &value,
                          mlir::IntegerType::SignednessSemantics signedness =
                              mlir::IntegerType::Signless) {
  return builder.getIntegerAttr(
      mlir::IntegerType::get(builder.getContext(), width, signedness), value);
}

mlir::IntegerAttr u64(mlir::OpBuilder &builder, uint64_t value) {
  return integer(builder, 64, llvm::APInt(64, value));
}

mlir::IntegerAttr u32(mlir::OpBuilder &builder, uint32_t value) {
  return integer(builder, 32, llvm::APInt(32, value));
}

mlir::FlatSymbolRefAttr symbol(mlir::OpBuilder &builder,
                               llvm::StringRef value) {
  return mlir::FlatSymbolRefAttr::get(builder.getContext(), value);
}

mlir::DictionaryAttr pathField(mlir::OpBuilder &builder,
                               llvm::StringRef name = "body") {
  return dictionary(builder, {{"kind", builder.getStringAttr("field")},
                              {"name", builder.getStringAttr(name)}});
}

mlir::DictionaryAttr site(mlir::OpBuilder &builder,
                          llvm::StringRef definition = "demo.Model") {
  return dictionary(builder,
                    {{"definition", symbol(builder, definition)},
                     {"ast_path", builder.getArrayAttr({pathField(builder)})}});
}

mlir::DictionaryAttr boolValue(mlir::OpBuilder &builder, bool value) {
  return dictionary(builder, {{"kind", builder.getStringAttr("bool")},
                              {"value", builder.getBoolAttr(value)}});
}

mlir::DictionaryAttr integerValue(mlir::OpBuilder &builder,
                                  llvm::StringRef value) {
  return dictionary(builder, {{"kind", builder.getStringAttr("integer")},
                              {"value", math(*builder.getContext(), value)}});
}

mlir::DictionaryAttr callFrame(mlir::OpBuilder &builder,
                               llvm::StringRef callee = "demo.Helper") {
  return dictionary(builder, {{"kind", builder.getStringAttr("call")},
                              {"site", site(builder)},
                              {"callee", symbol(builder, callee)}});
}

mlir::DictionaryAttr iterationFrame(mlir::OpBuilder &builder,
                                    mlir::Attribute ordinal,
                                    mlir::DictionaryAttr value) {
  return dictionary(builder, {{"kind", builder.getStringAttr("iteration")},
                              {"site", site(builder)},
                              {"ordinal", ordinal},
                              {"value", value}});
}

mlir::DictionaryAttr occurrence(mlir::OpBuilder &builder,
                                mlir::ArrayAttr expansion = {}) {
  if (!expansion)
    expansion = builder.getArrayAttr({});
  return dictionary(builder,
                    {{"site", site(builder)}, {"expansion", expansion}});
}

mlir::DictionaryAttr specKey(mlir::OpBuilder &builder,
                             mlir::ArrayAttr arguments,
                             llvm::StringRef definition = "demo.Model") {
  return dictionary(builder, {{"definition", symbol(builder, definition)},
                              {"arguments", arguments}});
}

mlir::DictionaryAttr valueID(mlir::OpBuilder &builder, mlir::Attribute slot) {
  return dictionary(builder, {{"origin", occurrence(builder)}, {"slot", slot}});
}

mlir::DictionaryAttr checkID(mlir::OpBuilder &builder,
                             mlir::Attribute obligation) {
  return dictionary(builder, {{"registration", occurrence(builder)},
                              {"check", occurrence(builder)},
                              {"obligation", obligation}});
}

mlir::DictionaryAttr ownerRef(mlir::OpBuilder &builder, mlir::ArrayAttr path) {
  return dictionary(builder, {{"instance_path", path}});
}

struct CheckResult {
  bool passed;
  std::string diagnostic;
};

template <typename Callback>
CheckResult check(mlir::MLIRContext &context, Callback callback) {
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream stream(diagnostic);
    value.print(stream);
    return mlir::success();
  });
  auto location = mlir::UnknownLoc::get(&context);
  auto emitError = [location] { return mlir::emitError(location); };
  return {mlir::succeeded(callback(emitError)), diagnostic};
}

void expectRejected(const CheckResult &result) {
  EXPECT_FALSE(result.passed);
  EXPECT_FALSE(result.diagnostic.empty());
}

class SourceIdentityContractsTest : public ::testing::Test {
protected:
  SourceIdentityContractsTest() : builder(&context) {
    context.loadDialect<ACIRDialect>();
  }

  template <typename Verifier>
  CheckResult verify(mlir::DictionaryAttr value, Verifier verifier) {
    return check(context,
                 [&](EmitError emit) { return verifier(value, emit); });
  }

  mlir::MLIRContext context;
  mlir::OpBuilder builder;
};

TEST_F(SourceIdentityContractsTest,
       SourceOwnerEnforcesOnlyApprovedPosixPathRules) {
  auto verifyOwner = [&](llvm::StringRef package, llvm::StringRef path) {
    return verify(
        dictionary(builder, {{"package", builder.getStringAttr(package)},
                             {"path", builder.getStringAttr(path)}}),
        detail::verifySourceOwner);
  };

  EXPECT_TRUE(verifyOwner("", "model.py").passed);
  EXPECT_TRUE(verifyOwner("demo.pkg", "nested/model.py").passed);
  EXPECT_TRUE(verifyOwner("demo", "pkg/__init__.py").passed);
  EXPECT_TRUE(verifyOwner("demo", "pkg:variant/model.py").passed);
  for (llvm::StringRef path :
       {"", "/model.py", "./model.py", "pkg/./model.py", "../model.py",
        "pkg/../model.py", "pkg//model.py", "pkg\\model.py", "C:/model.py",
        "C:model.py", "model.txt", "model.PY", "pkg/"})
    expectRejected(verifyOwner("demo", path));
  std::string nulPath("pkg/model\0.py", 13);
  expectRejected(verifyOwner("demo", nulPath));

  expectRejected(
      verify(dictionary(builder, {{"package", builder.getStringAttr("demo")}}),
             detail::verifySourceOwner));
  expectRejected(
      verify(dictionary(builder, {{"package", builder.getStringAttr("demo")},
                                  {"path", builder.getStringAttr("model.py")},
                                  {"unknown", builder.getUnitAttr()}}),
             detail::verifySourceOwner));
  expectRejected(
      verify(dictionary(builder, {{"package", u64(builder, 0)},
                                  {"path", builder.getStringAttr("model.py")}}),
             detail::verifySourceOwner));
}

TEST_F(SourceIdentityContractsTest, ExpansionFramesAcceptBothClosedVariants) {
  EXPECT_TRUE(
      verify(callFrame(builder, ""), detail::verifyExpansionFrame).passed);
  EXPECT_TRUE(verify(iterationFrame(builder, u64(builder, 0),
                                    integerValue(builder, "-1")),
                     detail::verifyExpansionFrame)
                  .passed);
  for (auto invalid : {
           dictionary(builder, {{"kind", builder.getStringAttr("other")},
                                {"site", site(builder)},
                                {"callee", symbol(builder, "demo.Helper")}}),
           dictionary(builder, {{"kind", builder.getStringAttr("call")},
                                {"site", site(builder)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("call")},
                                {"site", builder.getStringAttr("not-a-site")},
                                {"callee", symbol(builder, "demo.Helper")}}),
           dictionary(builder, {{"kind", builder.getStringAttr("call")},
                                {"site", site(builder)},
                                {"callee", symbol(builder, "demo.Helper")},
                                {"unknown", builder.getUnitAttr()}}),
           dictionary(builder, {{"kind", builder.getStringAttr("iteration")},
                                {"site", site(builder)},
                                {"ordinal", builder.getBoolAttr(true)},
                                {"value", boolValue(builder, true)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("iteration")},
                                {"site", site(builder)},
                                {"ordinal", u64(builder, 0)},
                                {"value", builder.getStringAttr("bad")}}),
       })
    expectRejected(verify(invalid, detail::verifyExpansionFrame));
}

TEST_F(SourceIdentityContractsTest,
       OccurrencePreservesEmptyAndOrderedDuplicateExpansionFrames) {
  auto first = callFrame(builder, "demo.A");
  auto second = callFrame(builder, "demo.B");
  EXPECT_TRUE(verify(occurrence(builder), detail::verifyOccurrence).passed);
  EXPECT_TRUE(
      verify(occurrence(builder, builder.getArrayAttr({second, first, second})),
             detail::verifyOccurrence)
          .passed);
  expectRejected(verify(dictionary(builder, {{"site", site(builder)}}),
                        detail::verifyOccurrence));
  expectRejected(
      verify(dictionary(builder, {{"site", site(builder)},
                                  {"expansion", builder.getStringAttr("bad")}}),
             detail::verifyOccurrence));
  expectRejected(
      verify(dictionary(builder, {{"site", site(builder)},
                                  {"expansion", builder.getArrayAttr({})},
                                  {"unknown", builder.getUnitAttr()}}),
             detail::verifyOccurrence));
  auto nested = verify(
      occurrence(builder, builder.getArrayAttr({builder.getStringAttr("bad")})),
      detail::verifyOccurrence);
  expectRejected(nested);
  EXPECT_NE(nested.diagnostic.find("expansion[0]"), std::string::npos)
      << nested.diagnostic;
}

TEST_F(SourceIdentityContractsTest,
       SpecKeyAcceptsEmptySymbolsEmptyArgumentsAndOrderedDuplicates) {
  auto zero = integerValue(builder, "0");
  auto one = integerValue(builder, "1");
  EXPECT_TRUE(verify(specKey(builder, builder.getArrayAttr({}), ""),
                     detail::verifySpecKey)
                  .passed);
  EXPECT_TRUE(verify(specKey(builder, builder.getArrayAttr({one, zero, one})),
                     detail::verifySpecKey)
                  .passed);
  expectRejected(verify(
      dictionary(builder, {{"definition", symbol(builder, "demo.Model")}}),
      detail::verifySpecKey));
  expectRejected(
      verify(dictionary(builder, {{"definition", builder.getStringAttr("bad")},
                                  {"arguments", builder.getArrayAttr({})}}),
             detail::verifySpecKey));
  expectRejected(verify(
      specKey(builder, builder.getArrayAttr({builder.getStringAttr("bad")})),
      detail::verifySpecKey));
  expectRejected(
      verify(dictionary(builder, {{"definition", symbol(builder, "demo.Model")},
                                  {"arguments", builder.getArrayAttr({})},
                                  {"unknown", builder.getUnitAttr()}}),
             detail::verifySpecKey));
}

TEST_F(SourceIdentityContractsTest,
       U32AndU64FieldsHonorMathematicalValueRange) {
  auto wideSmall = integer(builder, 128, llvm::APInt(128, 7));
  auto maximumU32 = integer(builder, 32, llvm::APInt::getAllOnes(32),
                            mlir::IntegerType::Unsigned);
  auto maximumU64 = integer(builder, 64, llvm::APInt::getAllOnes(64),
                            mlir::IntegerType::Unsigned);
  EXPECT_TRUE(
      verify(valueID(builder, maximumU32), detail::verifyValueID).passed);
  EXPECT_TRUE(
      verify(valueID(builder, wideSmall), detail::verifyValueID).passed);
  EXPECT_TRUE(
      verify(checkID(builder, maximumU64), detail::verifyCheckID).passed);
  EXPECT_TRUE(
      verify(checkID(builder, wideSmall), detail::verifyCheckID).passed);

  for (mlir::Attribute invalid : {
           mlir::Attribute(builder.getBoolAttr(true)),
           mlir::Attribute(integer(builder, 64, llvm::APInt::getAllOnes(64))),
           mlir::Attribute(integer(builder, 64, llvm::APInt::getAllOnes(64),
                                   mlir::IntegerType::Signed)),
           mlir::Attribute(
               integer(builder, 128, llvm::APInt::getOneBitSet(128, 32))),
       })
    expectRejected(verify(valueID(builder, invalid), detail::verifyValueID));
  expectRejected(
      verify(checkID(builder,
                     integer(builder, 128, llvm::APInt::getOneBitSet(128, 64))),
             detail::verifyCheckID));
  for (mlir::Attribute invalid : {
           mlir::Attribute(builder.getBoolAttr(true)),
           mlir::Attribute(integer(builder, 64, llvm::APInt::getAllOnes(64))),
           mlir::Attribute(integer(builder, 64, llvm::APInt::getAllOnes(64),
                                   mlir::IntegerType::Signed)),
       })
    expectRejected(verify(checkID(builder, invalid), detail::verifyCheckID));
}

TEST_F(SourceIdentityContractsTest,
       CheckValueAndProofIdentitiesRejectMalformedNestedChildren) {
  auto value = valueID(builder, u32(builder, 0));
  auto checkIdentity = checkID(builder, u64(builder, 0));
  auto key = specKey(builder, builder.getArrayAttr({}));
  auto scope = dictionary(builder, {{"specialization", key},
                                    {"registration", occurrence(builder)}});
  EXPECT_TRUE(verify(value, detail::verifyValueID).passed);
  EXPECT_TRUE(verify(checkIdentity, detail::verifyCheckID).passed);
  EXPECT_TRUE(verify(scope, detail::verifyProofScope).passed);

  expectRejected(verify(dictionary(builder, {{"origin", occurrence(builder)}}),
                        detail::verifyValueID));
  expectRejected(
      verify(dictionary(builder, {{"origin", occurrence(builder)},
                                  {"slot", u32(builder, 0)},
                                  {"unknown", builder.getUnitAttr()}}),
             detail::verifyValueID));
  expectRejected(
      verify(dictionary(builder, {{"registration", occurrence(builder)},
                                  {"check", builder.getStringAttr("bad")},
                                  {"obligation", u64(builder, 0)}}),
             detail::verifyCheckID));
  expectRejected(
      verify(dictionary(builder, {{"registration", occurrence(builder)},
                                  {"check", occurrence(builder)}}),
             detail::verifyCheckID));
  auto nested = verify(
      dictionary(builder, {{"specialization", builder.getStringAttr("bad")},
                           {"registration", occurrence(builder)}}),
      detail::verifyProofScope);
  expectRejected(nested);
  EXPECT_NE(nested.diagnostic.find("specialization"), std::string::npos)
      << nested.diagnostic;
  expectRejected(verify(dictionary(builder, {{"specialization", key}}),
                        detail::verifyProofScope));
}

TEST_F(SourceIdentityContractsTest,
       OwnerAndStateIdentitiesAllowRootAndScalarEmptyPaths) {
  auto declaration = occurrence(builder);
  auto root = ownerRef(builder, builder.getArrayAttr({}));
  auto repeatedPath = ownerRef(
      builder,
      builder.getArrayAttr({occurrence(builder), occurrence(builder)}));
  auto state = dictionary(builder, {{"owner", repeatedPath},
                                    {"declaration", declaration},
                                    {"element", builder.getArrayAttr({})}});
  EXPECT_TRUE(verify(root, detail::verifyOwnerRef).passed);
  EXPECT_TRUE(verify(repeatedPath, detail::verifyOwnerRef).passed);
  EXPECT_TRUE(verify(state, detail::verifyStateID).passed);
  EXPECT_TRUE(
      verify(dictionary(builder,
                        {{"owner", root},
                         {"declaration", declaration},
                         {"element", builder.getArrayAttr(
                                         {u64(builder, 2), u64(builder, 0)})}}),
             detail::verifyStateID)
          .passed);
  expectRejected(verify(
      ownerRef(builder, builder.getArrayAttr({builder.getStringAttr("bad")})),
      detail::verifyOwnerRef));
  expectRejected(verify(
      dictionary(builder, {{"owner", root}, {"declaration", declaration}}),
      detail::verifyStateID));
  expectRejected(
      verify(dictionary(builder, {{"owner", builder.getStringAttr("bad")},
                                  {"declaration", declaration},
                                  {"element", builder.getArrayAttr({})}}),
             detail::verifyStateID));
}

TEST_F(SourceIdentityContractsTest,
       StateRefDistinguishesOwnedAndFormalOrdinals) {
  auto declaration = occurrence(builder);
  auto ownedScalar =
      dictionary(builder, {{"kind", builder.getStringAttr("owned")},
                           {"declaration", declaration},
                           {"element", builder.getArrayAttr({})}});
  auto formalScalar =
      dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                           {"parameter", builder.getStringAttr("")},
                           {"ordinal", builder.getUnitAttr()}});
  auto formalElement =
      dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                           {"parameter", builder.getStringAttr("items")},
                           {"ordinal", u64(builder, 0)}});
  EXPECT_TRUE(verify(ownedScalar, detail::verifyStateRef).passed);
  EXPECT_TRUE(verify(formalScalar, detail::verifyStateRef).passed);
  EXPECT_TRUE(verify(formalElement, detail::verifyStateRef).passed);

  for (auto invalid : {
           dictionary(builder, {{"kind", builder.getStringAttr("other")},
                                {"parameter", builder.getStringAttr("items")},
                                {"ordinal", u64(builder, 0)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("owned")},
                                {"declaration", declaration},
                                {"element", builder.getArrayAttr(
                                                {builder.getUnitAttr()})}}),
           dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                                {"parameter", builder.getStringAttr("items")},
                                {"ordinal", builder.getBoolAttr(true)}}),
           dictionary(builder, {{"kind", builder.getStringAttr("formal")},
                                {"parameter", builder.getStringAttr("items")}}),
           dictionary(builder, {{"kind", builder.getStringAttr("owned")},
                                {"declaration", declaration},
                                {"element", builder.getArrayAttr({})},
                                {"unknown", builder.getUnitAttr()}}),
       })
    expectRejected(verify(invalid, detail::verifyStateRef));
}

} // namespace
} // namespace acir::ac
