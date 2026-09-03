#include "acir/Dialect/ACSim/ACSimDialect.h"
#include "acir/Dialect/ACSim/ACSimTypes.h"

#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "gtest/gtest.h"

#include <array>

namespace acir::acsim {
namespace {

TEST(ACSimTypesTest, ExactPublicInventoryConstructsPrintsAndRoundTrips) {
  mlir::MLIRContext context;
  context.loadDialect<ACSimDialect>();

  struct TypeCase {
    llvm::StringLiteral spelling;
    mlir::TypeID typeID;
  };
  const std::array<TypeCase, 11> cases = {{
      {"!acsim.value<@cpp_i32>", ValueType::getTypeID()},
      {"!acsim.expr<@cpp_i32>", ExprType::getTypeID()},
      {"!acsim.owner<@fifo_binding>", OwnerType::getTypeID()},
      {"!acsim.ref<@fifo_binding>", RefType::getTypeID()},
      {"!acsim.port<@stream, @producer, @packet, @ready_valid>",
       PortType::getTypeID()},
      {"!acsim.resource<@memory, @initiator>", ResourceType::getTypeID()},
      {"!acsim.array<[2, 3], !acsim.owner<@fifo_binding>>",
       ArrayType::getTypeID()},
      {"!acsim.object_id", ObjectIdType::getTypeID()},
      {"!acsim.activation_id", ActivationIdType::getTypeID()},
      {"!acsim.pc<@tick>", PcType::getTypeID()},
      {"!acsim.wake<@event>", WakeType::getTypeID()},
  }};

  for (const TypeCase &testCase : cases) {
    mlir::Type type = mlir::parseType(testCase.spelling, &context);
    ASSERT_TRUE(type) << testCase.spelling.str();
    EXPECT_EQ(type.getTypeID(), testCase.typeID) << testCase.spelling.str();

    std::string printed;
    llvm::raw_string_ostream(printed) << type;
    EXPECT_EQ(printed, testCase.spelling) << testCase.spelling.str();
    EXPECT_EQ(type, mlir::parseType(printed, &context));
  }
}

TEST(ACSimTypesTest,
     CheckedArrayBuilderRejectsDynamicNegativeAndExcessiveShape) {
  mlir::MLIRContext context;
  context.loadDialect<ACSimDialect>();
  mlir::ScopedDiagnosticHandler suppressExpectedDiagnostics(
      &context, [](mlir::Diagnostic &) { return mlir::success(); });
  auto location = mlir::UnknownLoc::get(&context);
  auto emitError = [location] { return mlir::emitError(location); };
  auto owner = OwnerType::get(
      &context, mlir::FlatSymbolRefAttr::get(&context, "fifo_binding"));

  EXPECT_TRUE(ArrayType::getChecked(
      emitError, &context, mlir::DenseI64ArrayAttr::get(&context, {2, 3}),
      mlir::Type(owner)));
  EXPECT_FALSE(ArrayType::getChecked(emitError, &context,
                                     mlir::DenseI64ArrayAttr::get(&context, {}),
                                     mlir::Type(owner)));
  EXPECT_FALSE(ArrayType::getChecked(
      emitError, &context, mlir::DenseI64ArrayAttr::get(&context, {2, -1}),
      mlir::Type(owner)));
  EXPECT_FALSE(ArrayType::getChecked(
      emitError, &context,
      mlir::DenseI64ArrayAttr::get(&context, {1048576, 1048576}),
      mlir::Type(owner)));
}

TEST(ACSimTypesTest, PublicTypesAreUniquedByExactTypedMetadata) {
  mlir::MLIRContext context;
  context.loadDialect<ACSimDialect>();
  auto left = mlir::FlatSymbolRefAttr::get(&context, "left");
  auto right = mlir::FlatSymbolRefAttr::get(&context, "right");
  EXPECT_EQ(ValueType::get(&context, left), ValueType::get(&context, left));
  EXPECT_NE(ValueType::get(&context, left), ValueType::get(&context, right));
  EXPECT_NE(OwnerType::get(&context, left), RefType::get(&context, left));
}

} // namespace
} // namespace acir::acsim
