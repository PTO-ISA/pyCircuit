#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"
#include "gtest/gtest.h"

#include <initializer_list>
#include <map>
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
  unsigned width = static_cast<unsigned>(spelling.size() * 4 + 2);
  return MathIntAttr::get(&context, signedInteger(width, spelling));
}

mlir::IntegerAttr
integerAttr(mlir::OpBuilder &builder, unsigned width, const llvm::APInt &value,
            mlir::IntegerType::SignednessSemantics signedness =
                mlir::IntegerType::Signless) {
  return builder.getIntegerAttr(
      mlir::IntegerType::get(builder.getContext(), width, signedness), value);
}

mlir::IntegerAttr u64(mlir::OpBuilder &builder, uint64_t value) {
  return integerAttr(builder, 64, llvm::APInt(64, value));
}

mlir::TypeAttr bits(mlir::OpBuilder &builder, unsigned width) {
  return mlir::TypeAttr::get(
      mlir::IntegerType::get(builder.getContext(), width));
}

mlir::FlatSymbolRefAttr symbol(mlir::OpBuilder &builder, llvm::StringRef name) {
  return mlir::FlatSymbolRefAttr::get(builder.getContext(), name);
}

mlir::DictionaryAttr logicalBool(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"kind", builder.getStringAttr("bool")},
                              {"storage", bits(builder, 1)}});
}

mlir::DictionaryAttr logicalInteger(mlir::OpBuilder &builder,
                                    llvm::StringRef lower,
                                    llvm::StringRef upper, unsigned width,
                                    llvm::StringRef interpretation) {
  return dictionary(
      builder, {{"kind", builder.getStringAttr("integer")},
                {"storage", bits(builder, width)},
                {"lower", math(*builder.getContext(), lower)},
                {"upper", math(*builder.getContext(), upper)},
                {"interpretation", builder.getStringAttr(interpretation)}});
}

mlir::DictionaryAttr logicalRecord(mlir::OpBuilder &builder,
                                   llvm::StringRef name) {
  return dictionary(builder, {{"kind", builder.getStringAttr("record")},
                              {"symbol", symbol(builder, name)}});
}

mlir::DictionaryAttr logicalList(mlir::OpBuilder &builder,
                                 mlir::DictionaryAttr element,
                                 mlir::Attribute length) {
  return dictionary(builder, {{"kind", builder.getStringAttr("list")},
                              {"element", element},
                              {"length", length}});
}

mlir::DictionaryAttr staticBool(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"kind", builder.getStringAttr("bool")}});
}

mlir::DictionaryAttr staticInteger(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"kind", builder.getStringAttr("integer")}});
}

mlir::DictionaryAttr staticInteger(mlir::OpBuilder &builder,
                                   llvm::StringRef lower,
                                   llvm::StringRef upper) {
  return dictionary(builder, {{"kind", builder.getStringAttr("integer")},
                              {"lower", math(*builder.getContext(), lower)},
                              {"upper", math(*builder.getContext(), upper)}});
}

mlir::DictionaryAttr staticRecord(mlir::OpBuilder &builder,
                                  llvm::StringRef name) {
  return dictionary(builder, {{"kind", builder.getStringAttr("record")},
                              {"symbol", symbol(builder, name)}});
}

mlir::DictionaryAttr staticList(mlir::OpBuilder &builder,
                                mlir::DictionaryAttr element,
                                mlir::Attribute length) {
  return dictionary(builder, {{"kind", builder.getStringAttr("list")},
                              {"element", element},
                              {"length", length}});
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

mlir::DictionaryAttr recordValue(mlir::OpBuilder &builder, llvm::StringRef name,
                                 mlir::ArrayAttr fields) {
  return dictionary(builder, {{"kind", builder.getStringAttr("record")},
                              {"symbol", symbol(builder, name)},
                              {"fields", fields}});
}

mlir::DictionaryAttr listValue(mlir::OpBuilder &builder,
                               mlir::ArrayAttr values) {
  return dictionary(
      builder, {{"kind", builder.getStringAttr("list")}, {"values", values}});
}

mlir::DictionaryAttr absentDefault(mlir::OpBuilder &builder) {
  return dictionary(builder, {{"present", builder.getBoolAttr(false)}});
}

mlir::DictionaryAttr presentDefault(mlir::OpBuilder &builder,
                                    mlir::DictionaryAttr value) {
  return dictionary(builder,
                    {{"present", builder.getBoolAttr(true)}, {"value", value}});
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
  bool passed = mlir::succeeded(callback(emitError));
  return {passed, diagnostic};
}

void expectRejected(const CheckResult &result) {
  EXPECT_FALSE(result.passed);
  EXPECT_FALSE(result.diagnostic.empty());
}

using RecordTable =
    std::map<std::string, detail::ResolvedRecordView, std::less<>>;

auto resolver(const RecordTable &records) {
  return [&records](mlir::FlatSymbolRefAttr requested)
             -> mlir::FailureOr<detail::ResolvedRecordView> {
    auto found = records.find(requested.getValue().str());
    if (found == records.end())
      return mlir::failure();
    return found->second;
  };
}

class SourceTypeContractsTest : public ::testing::Test {
protected:
  SourceTypeContractsTest() : builder(&context) {
    context.loadDialect<ACIRDialect>();
  }

  mlir::MLIRContext context;
  mlir::OpBuilder builder;
};

TEST_F(SourceTypeContractsTest,
       AcceptsEveryLogicalVariantAndMinimalIntegerDomains) {
  auto verify = [&](mlir::DictionaryAttr value) {
    return check(context, [&](EmitError emit) {
      return detail::verifyLogicalTypeStructure(value, emit);
    });
  };
  EXPECT_TRUE(verify(logicalBool(builder)).passed);
  EXPECT_TRUE(verify(logicalRecord(builder, "demo.Packet")).passed);

  struct IntegerCase {
    llvm::StringRef lower;
    llvm::StringRef upper;
    unsigned width;
    llvm::StringRef interpretation;
  };
  for (const IntegerCase &testCase : {
           IntegerCase{"0", "1", 1, "unsigned"},
           {"1", "2", 1, "unsigned"},
           {"-1", "0", 1, "signed"},
           {"-1", "1", 1, "signed"},
           {"0", "3", 2, "unsigned"},
           {"100", "101", 7, "unsigned"},
           {"-128", "128", 8, "signed"},
           {"-128", "129", 9, "signed"},
           {"0", "18446744073709551616", 64, "unsigned"},
           {"-9223372036854775808", "9223372036854775808", 64, "signed"},
       })
    EXPECT_TRUE(verify(logicalInteger(builder, testCase.lower, testCase.upper,
                                      testCase.width, testCase.interpretation))
                    .passed)
        << testCase.lower.str() << ".." << testCase.upper.str();

  EXPECT_TRUE(
      verify(logicalList(builder, logicalBool(builder), u64(builder, 1)))
          .passed);
  EXPECT_TRUE(
      verify(logicalList(builder,
                         logicalInteger(builder, "0", "3", 2, "unsigned"),
                         u64(builder, 2)))
          .passed);
  EXPECT_TRUE(verify(logicalList(builder, logicalRecord(builder, "demo.Packet"),
                                 u64(builder, 3)))
                  .passed);
}

TEST_F(SourceTypeContractsTest, RejectsNonMinimalAndMalformedLogicalVariants) {
  auto reject = [&](mlir::DictionaryAttr value) {
    expectRejected(check(context, [&](EmitError emit) {
      return detail::verifyLogicalTypeStructure(value, emit);
    }));
  };

  reject(dictionary(builder, {}));
  reject(dictionary(builder, {{"kind", builder.getStringAttr("other")}}));
  reject(dictionary(builder, {{"kind", builder.getStringAttr("bool")}}));
  reject(dictionary(builder, {{"kind", builder.getStringAttr("bool")},
                              {"storage", bits(builder, 2)}}));
  reject(dictionary(builder, {{"kind", builder.getStringAttr("bool")},
                              {"storage", bits(builder, 1)},
                              {"unknown", builder.getUnitAttr()}}));
  reject(logicalInteger(builder, "0", "1", 2, "unsigned"));
  reject(logicalInteger(builder, "0", "3", 2, "signed"));
  reject(logicalInteger(builder, "-1", "1", 1, "unsigned"));
  reject(logicalInteger(builder, "1", "1", 1, "unsigned"));
  reject(logicalInteger(builder, "2", "1", 1, "unsigned"));
  reject(logicalInteger(builder, "18446744073709551616", "18446744073709551617",
                        64, "unsigned"));
  reject(dictionary(builder,
                    {{"kind", builder.getStringAttr("integer")},
                     {"storage", bits(builder, 1)},
                     {"lower", math(context, "0")},
                     {"interpretation", builder.getStringAttr("unsigned")}}));
  reject(
      dictionary(builder, {{"kind", builder.getStringAttr("record")},
                           {"symbol", builder.getStringAttr("demo.Packet")}}));
  reject(logicalList(builder, logicalBool(builder), u64(builder, 0)));
  reject(logicalList(
      builder, logicalList(builder, logicalBool(builder), u64(builder, 1)),
      u64(builder, 1)));
  reject(dictionary(builder, {{"kind", builder.getStringAttr("list")},
                              {"element", logicalBool(builder)}}));
}

TEST_F(SourceTypeContractsTest, U64FieldsUseValueRangeInsteadOfContainerWidth) {
  auto logical = [&](mlir::Attribute length) {
    return check(context, [&](EmitError emit) {
      return detail::verifyLogicalTypeStructure(
          logicalList(builder, logicalBool(builder), length), emit);
    });
  };
  auto staticType = [&](mlir::Attribute length) {
    return check(context, [&](EmitError emit) {
      return detail::verifyStaticTypeStructure(
          staticList(builder, staticBool(builder), length), emit);
    });
  };
  auto wide = [&](mlir::IntegerType::SignednessSemantics signedness,
                  const llvm::APInt &value) {
    return integerAttr(builder, 128, value, signedness);
  };

  auto maximum = integerAttr(builder, 64, llvm::APInt::getAllOnes(64),
                             mlir::IntegerType::Unsigned);
  auto signedSmall = wide(mlir::IntegerType::Signed, llvm::APInt(128, 7));
  auto unsignedSmall = wide(mlir::IntegerType::Unsigned, llvm::APInt(128, 7));
  EXPECT_TRUE(logical(maximum).passed);
  EXPECT_TRUE(logical(signedSmall).passed);
  EXPECT_TRUE(staticType(unsignedSmall).passed);

  expectRejected(logical(builder.getBoolAttr(true)));
  expectRejected(
      logical(integerAttr(builder, 64, llvm::APInt::getAllOnes(64))));
  expectRejected(logical(integerAttr(builder, 64, llvm::APInt::getAllOnes(64),
                                     mlir::IntegerType::Signed)));
  auto tooLarge = llvm::APInt::getOneBitSet(128, 64);
  expectRejected(logical(wide(mlir::IntegerType::Signed, tooLarge)));
  expectRejected(staticType(wide(mlir::IntegerType::Unsigned, tooLarge)));
}

TEST_F(SourceTypeContractsTest, AcceptsUnboundedBoundedAndNestedStaticTypes) {
  auto verify = [&](mlir::DictionaryAttr value) {
    return check(context, [&](EmitError emit) {
      return detail::verifyStaticTypeStructure(value, emit);
    });
  };

  EXPECT_TRUE(verify(staticBool(builder)).passed);
  EXPECT_TRUE(verify(staticInteger(builder)).passed);
  EXPECT_TRUE(
      verify(staticInteger(builder, "-1234567890123456789012345678901234567890",
                           "1234567890123456789012345678901234567891"))
          .passed);
  EXPECT_TRUE(verify(staticRecord(builder, "demo.Packet")).passed);
  EXPECT_TRUE(verify(staticList(builder,
                                staticList(builder, staticInteger(builder),
                                           u64(builder, 2)),
                                u64(builder, 3)))
                  .passed);
}

TEST_F(SourceTypeContractsTest,
       RejectsMalformedStaticTypesAndNonPositiveLists) {
  auto reject = [&](mlir::DictionaryAttr value) {
    expectRejected(check(context, [&](EmitError emit) {
      return detail::verifyStaticTypeStructure(value, emit);
    }));
  };

  reject(dictionary(builder, {}));
  reject(dictionary(builder, {{"kind", builder.getStringAttr("other")}}));
  reject(dictionary(builder, {{"kind", builder.getStringAttr("bool")},
                              {"unknown", builder.getUnitAttr()}}));
  reject(dictionary(builder, {{"kind", builder.getStringAttr("integer")},
                              {"lower", math(context, "0")}}));
  reject(staticInteger(builder, "1", "1"));
  reject(dictionary(builder, {{"kind", builder.getStringAttr("record")}}));
  reject(staticList(builder, staticBool(builder), u64(builder, 0)));
  reject(dictionary(builder, {{"kind", builder.getStringAttr("list")},
                              {"length", u64(builder, 1)}}));
}

TEST_F(SourceTypeContractsTest,
       EnforcesClosedStaticValueVariantsAndBoolIntegerDistinction) {
  auto verify = [&](mlir::DictionaryAttr value) {
    return check(context, [&](EmitError emit) {
      return detail::verifyStaticValueStructure(value, emit);
    });
  };

  EXPECT_TRUE(verify(boolValue(builder, true)).passed);
  EXPECT_TRUE(verify(integerValue(builder, "1")).passed);
  EXPECT_TRUE(
      verify(recordValue(builder, "demo.Empty", builder.getArrayAttr({})))
          .passed);
  EXPECT_TRUE(verify(listValue(builder, builder.getArrayAttr(
                                            {boolValue(builder, false)})))
                  .passed);
  EXPECT_TRUE(
      verify(listValue(builder, builder.getArrayAttr({listValue(
                                    builder, builder.getArrayAttr({integerValue(
                                                 builder, "1")}))})))
          .passed);

  expectRejected(verify(dictionary(builder, {})));
  expectRejected(
      verify(dictionary(builder, {{"kind", builder.getStringAttr("other")}})));
  expectRejected(
      verify(dictionary(builder, {{"kind", builder.getStringAttr("list")}})));
  expectRejected(
      verify(dictionary(builder, {{"kind", builder.getStringAttr("bool")},
                                  {"value", math(context, "1")}})));
  expectRejected(
      verify(dictionary(builder, {{"kind", builder.getStringAttr("integer")},
                                  {"value", builder.getBoolAttr(true)}})));
  expectRejected(
      verify(dictionary(builder, {{"kind", builder.getStringAttr("record")},
                                  {"symbol", symbol(builder, "demo.Empty")}})));
  expectRejected(verify(listValue(builder, builder.getArrayAttr({}))));
  expectRejected(verify(dictionary(
      builder, {{"kind", builder.getStringAttr("list")},
                {"values", builder.getArrayAttr({boolValue(builder, true)})},
                {"unknown", builder.getUnitAttr()}})));
}

TEST_F(SourceTypeContractsTest, EnforcesDefaultPresentDiscriminator) {
  auto verify = [&](mlir::DictionaryAttr value) {
    return check(context, [&](EmitError emit) {
      return detail::verifyDefaultStructure(value, emit);
    });
  };

  EXPECT_TRUE(verify(absentDefault(builder)).passed);
  EXPECT_TRUE(
      verify(presentDefault(builder, integerValue(builder, "1"))).passed);
  expectRejected(verify(dictionary(builder, {})));
  expectRejected(
      verify(dictionary(builder, {{"present", builder.getBoolAttr(true)}})));
  expectRejected(
      verify(dictionary(builder, {{"present", builder.getBoolAttr(false)},
                                  {"value", integerValue(builder, "1")}})));
  expectRejected(verify(dictionary(builder, {{"present", u64(builder, 0)}})));
}

TEST_F(SourceTypeContractsTest, AcceptsNestedRecordsAndSharedDeclarationDags) {
  RecordTable records;
  auto leaf = symbol(builder, "demo.Leaf");
  auto pair = symbol(builder, "demo.Pair");
  auto outer = symbol(builder, "demo.Outer");
  records.emplace(
      "demo.Leaf",
      detail::ResolvedRecordView{
          leaf, {logicalInteger(builder, "0", "3", 2, "unsigned")}});
  records.emplace("demo.Pair", detail::ResolvedRecordView{
                                   pair,
                                   {logicalRecord(builder, "demo.Leaf"),
                                    logicalRecord(builder, "demo.Leaf")}});
  records.emplace(
      "demo.Outer",
      detail::ResolvedRecordView{outer, {logicalRecord(builder, "demo.Pair")}});

  auto result = check(context, [&](EmitError emit) {
    return detail::verifyTypeResolved(logicalRecord(builder, "demo.Outer"),
                                      detail::ExpectedTypeKind::Logical,
                                      resolver(records), emit);
  });
  EXPECT_TRUE(result.passed) << result.diagnostic;
}

TEST_F(SourceTypeContractsTest, RejectsUnknownWrongAuthorityAndCycles) {
  auto reject = [&](mlir::DictionaryAttr type, const RecordTable &records) {
    expectRejected(check(context, [&](EmitError emit) {
      return detail::verifyTypeResolved(type, detail::ExpectedTypeKind::Logical,
                                        resolver(records), emit);
    }));
  };

  reject(logicalRecord(builder, "i32"), {});
  RecordTable wrong{
      {"demo.Packet", {symbol(builder, "demo.Other"), {logicalBool(builder)}}}};
  reject(logicalRecord(builder, "demo.Packet"), wrong);
  RecordTable direct{{"demo.Direct",
                      {symbol(builder, "demo.Direct"),
                       {logicalRecord(builder, "demo.Direct")}}}};
  reject(logicalRecord(builder, "demo.Direct"), direct);
  RecordTable indirect{
      {"demo.A",
       {symbol(builder, "demo.A"), {logicalRecord(builder, "demo.B")}}},
      {"demo.B",
       {symbol(builder, "demo.B"), {logicalRecord(builder, "demo.A")}}},
  };
  reject(logicalRecord(builder, "demo.A"), indirect);
}

TEST_F(SourceTypeContractsTest, EnforcesNominalIdentityArityKindRangeAndOrder) {
  auto packet = symbol(builder, "demo.Packet");
  RecordTable records{{"demo.Packet",
                       {packet,
                        {logicalBool(builder),
                         logicalInteger(builder, "0", "2", 1, "unsigned")}}},
                      {"demo.Other",
                       {symbol(builder, "demo.Other"),
                        {logicalBool(builder),
                         logicalInteger(builder, "0", "2", 1, "unsigned")}}}};
  auto expected = logicalRecord(builder, "demo.Packet");
  auto match = [&](mlir::DictionaryAttr value) {
    return check(context, [&](EmitError emit) {
      return detail::verifyStaticValueMatchesType(
          value, expected, detail::ExpectedTypeKind::Logical, resolver(records),
          emit);
    });
  };

  EXPECT_TRUE(
      match(recordValue(builder, "demo.Packet",
                        builder.getArrayAttr({boolValue(builder, true),
                                              integerValue(builder, "1")})))
          .passed);
  expectRejected(
      match(recordValue(builder, "demo.Other",
                        builder.getArrayAttr({boolValue(builder, true),
                                              integerValue(builder, "1")}))));
  expectRejected(
      match(recordValue(builder, "demo.Packet",
                        builder.getArrayAttr({boolValue(builder, true)}))));
  expectRejected(
      match(recordValue(builder, "demo.Packet",
                        builder.getArrayAttr({integerValue(builder, "1"),
                                              boolValue(builder, true)}))));
  expectRejected(
      match(recordValue(builder, "demo.Packet",
                        builder.getArrayAttr({boolValue(builder, true),
                                              integerValue(builder, "2")}))));

  RecordTable ranges{{"demo.Ranges",
                      {symbol(builder, "demo.Ranges"),
                       {logicalInteger(builder, "0", "2", 1, "unsigned"),
                        logicalInteger(builder, "10", "12", 4, "unsigned")}}}};
  auto rangeOrder = check(context, [&](EmitError emit) {
    return detail::verifyStaticValueMatchesType(
        recordValue(builder, "demo.Ranges",
                    builder.getArrayAttr({integerValue(builder, "10"),
                                          integerValue(builder, "1")})),
        logicalRecord(builder, "demo.Ranges"),
        detail::ExpectedTypeKind::Logical, resolver(ranges), emit);
  });
  expectRejected(rangeOrder);
}

TEST_F(SourceTypeContractsTest,
       NestedListFailureReportsFieldAndElementContext) {
  auto container = symbol(builder, "demo.Container");
  RecordTable records{{
      "demo.Container",
      {container,
       {logicalList(builder, logicalInteger(builder, "0", "2", 1, "unsigned"),
                    u64(builder, 2))}},
  }};
  auto value = recordValue(
      builder, "demo.Container",
      builder.getArrayAttr({listValue(
          builder, builder.getArrayAttr({integerValue(builder, "0"),
                                         integerValue(builder, "2")}))}));

  auto result = check(context, [&](EmitError emit) {
    return detail::verifyStaticValueMatchesType(
        value, logicalRecord(builder, "demo.Container"),
        detail::ExpectedTypeKind::Logical, resolver(records), emit);
  });
  expectRejected(result);
  EXPECT_NE(result.diagnostic.find("field[0]"), std::string::npos)
      << result.diagnostic;
  EXPECT_NE(result.diagnostic.find("element[1]"), std::string::npos)
      << result.diagnostic;
}

TEST_F(SourceTypeContractsTest, AbsentDefaultResolvesTypeAndPresentMustMatch) {
  RecordTable records{{"demo.Packet",
                       {symbol(builder, "demo.Packet"),
                        {logicalInteger(builder, "0", "2", 1, "unsigned")}}}};
  auto expected = staticRecord(builder, "demo.Packet");
  auto match = [&](mlir::DictionaryAttr value, const RecordTable &table) {
    return check(context, [&](EmitError emit) {
      return detail::verifyDefaultMatchesType(value, expected,
                                              detail::ExpectedTypeKind::Static,
                                              resolver(table), emit);
    });
  };

  EXPECT_TRUE(match(absentDefault(builder), records).passed);
  EXPECT_TRUE(match(presentDefault(
                        builder, recordValue(builder, "demo.Packet",
                                             builder.getArrayAttr({integerValue(
                                                 builder, "1")}))),
                    records)
                  .passed);
  expectRejected(match(absentDefault(builder), {}));
  expectRejected(
      match(presentDefault(builder, boolValue(builder, true)), records));
}

} // namespace
} // namespace acir::ac
