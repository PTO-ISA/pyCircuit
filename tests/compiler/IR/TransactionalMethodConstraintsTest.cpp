#include "mlir/AsmParser/AsmParser.h"
#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "gtest/gtest.h"

using namespace mlir;
namespace acir::ac {
namespace {
TEST(TransactionalMethodConstraints, ResultDemandSurvivesUntilProviderBinding) {
  MLIRContext context;
  context.getOrLoadDialect<ACIRDialect>();
  OpBuilder b(&context);
  auto package = mlir::ModuleOp::create(b.getUnknownLoc());
  b.setInsertionPointToEnd(package.getBody());
  OperationState declaration(b.getUnknownLoc(), "ac.module.import");
  declaration.addAttribute("sym_name", b.getStringAttr("Provider"));
  declaration.addAttribute("ac.reference_requirement", b.getBoolAttr(true));
  declaration.addAttribute("ac.methods", b.getArrayAttr({}));
  auto *provider = b.create(declaration);
  auto ref = ModuleRefType::get(&context,
                                FlatSymbolRefAttr::get(&context, "Provider"));
  auto receiver = package.getBody()->addArgument(ref, b.getUnknownLoc());
  auto addCall = [&](StringRef demand, unsigned slots) {
    OperationState state(b.getUnknownLoc(), "ac.method.pending");
    state.addOperands(receiver);
    state.addTypes(SmallVector<Type>(slots, InferType::get(&context)));
    state.addAttribute(
        "callee",
        FlatSymbolRefAttr::get(&context, demand == "tuple" ? "pair" : "read"));
    state.addAttribute("read_only", b.getBoolAttr(false));
    state.addAttribute("result_requirement", b.getStringAttr(demand));
    return b.create(state);
  };
  addCall("ignored", 0);
  addCall("scalar", 1);
  addCall("tuple", 2);
  ASSERT_TRUE(succeeded(resolveTransactionalMethodConstraints(package)));
  auto methods = getTransactionalMethodInterfaces(provider);
  ASSERT_EQ(methods.size(), 2u);
  auto entry = cast<DictionaryAttr>(methods[0]);
  EXPECT_FALSE(entry.get("function_type"));
  auto calls = entry.getAs<ArrayAttr>("calls");
  ASSERT_EQ(calls.size(), 2u);
  EXPECT_EQ(cast<DictionaryAttr>(calls[0])
                .getAs<StringAttr>("result_requirement")
                .getValue(),
            "ignored");
  EXPECT_EQ(cast<DictionaryAttr>(calls[1]).getAs<ArrayAttr>("results").size(),
            1u);
  auto tupleCall = cast<DictionaryAttr>(
      cast<DictionaryAttr>(methods[1]).getAs<ArrayAttr>("calls")[0]);
  EXPECT_EQ(tupleCall.getAs<ArrayAttr>("results").size(), 2u);
  // Binding an ignored call preserves every provider result on the real call.
  for (auto call :
       llvm::make_early_inc_range(package.getOps<TransactionMethodPendingOp>()))
    if (call.getResultRequirement() != "ignored")
      call.erase();
  provider->removeAttr("ac.reference_requirement");
  auto payload = parseType(
      R"mlir(!ac.bits<#ac.static_expr<{kind = "literal", location = {path = "methods.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Provider, ast_path = []}, expansion = []}, value = {kind = "integer", value = #ac.math_int<8>}}>>)mlir",
      &context);
  ASSERT_TRUE(payload);
  provider->setAttr(
      "ac.methods",
      b.getArrayAttr({b.getDictionaryAttr(
          {b.getNamedAttr("name", b.getStringAttr("read")),
           b.getNamedAttr("read_only", b.getBoolAttr(false)),
           b.getNamedAttr("function_type", TypeAttr::get(b.getFunctionType(
                                               {}, {payload, payload})))})}));
  ASSERT_TRUE(succeeded(resolveTransactionalMethodConstraints(package, true)));
  auto call = *package.getOps<TransactionMethodCallOp>().begin();
  EXPECT_EQ(call.getNumResults(), 2u);
  EXPECT_TRUE(call->use_empty());
  package.erase();
}
} // namespace
} // namespace acir::ac
