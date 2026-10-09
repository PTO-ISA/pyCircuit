#include "pycircuit/Dialect/ACIR/SourceEffects.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/STLExtras.h"
#include "gtest/gtest.h"

#include <string>

namespace acir::ac {
namespace {

std::string design(llvm::StringRef body) {
  return R"ir(
#owner = {package = "effects", path = "effects.py"}
#definition = {source_owner = #owner, qualified_name = "Effects"}
#occ = {site = {definition = @Effects, ast_path = []}, expansion = []}
#span = {path = "effects.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#diagnostic = {kind = "assert", message = "check", location = #span}
#integer = #ac.source_domain<{kind = "integer", lower = #ac.math_int<-8>, upper = #ac.math_int<8>}>
#formal = {kind = "formal", parameter = "state", ordinal = 0 : i64}
#parameter = {name = "state", binding = "positional_or_keyword", domain = #integer, formal_ref = #formal}
module {
"ac.src.unit"() ({
"ac.src.module"() ({
^bb0(%clock: !ac.clock, %reset: !ac.bool, %state: !ac.ref<#integer>):
"ac.src.rule"(%reset, %state) ({
^bb0(%rule_reset: !ac.bool, %captured: !ac.ref<#integer>):
%b = "ac.src.bool"() {value = true} : () -> !ac.bool
%i = "ac.src.int"() {value = #ac.math_int<7>} : () -> !ac.math_int
%read = "ac.src.read"(%captured) {occurrence = #occ} : (!ac.ref<#integer>) -> !ac.math_int
)ir" + body.str() +
         R"ir(
"ac.src.yield"() : () -> ()
}) {sym_name = "step", occurrence = #occ} : (!ac.bool, !ac.ref<#integer>) -> ()
"ac.src.register"() {rule = @step, occurrence = {site = {definition = @Effects, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}} : () -> ()
"ac.src.yield"() : () -> ()
}) {sym_name = "Effects", definition = #definition, domain = "default", parameters = [#parameter], static_parameters = [], function_type = (!ac.clock, !ac.bool, !ac.ref<#integer>) -> ()} : () -> ()
}) {source_owner = #owner, kind = "body", interfaces = [#owner], exports = []} : () -> ()
}
)ir";
}

class SourceEffectsTest : public ::testing::Test {
protected:
  SourceEffectsTest() { context.loadDialect<ACIRDialect>(); }
  mlir::OwningOpRef<mlir::ModuleOp> parse(llvm::StringRef body) {
    auto file = mlir::parseSourceString<mlir::ModuleOp>(design(body), &context);
    EXPECT_TRUE(file);
    if (file)
      EXPECT_TRUE(mlir::succeeded(mlir::verify(*file)));
    return file;
  }
  template <typename Op> Op find(mlir::ModuleOp module) {
    Op result;
    module.walk([&](Op candidate) { result = candidate; });
    EXPECT_TRUE(result);
    return result;
  }
  template <typename Effect, typename Resource> bool has(mlir::Operation *op) {
    auto effects = mlir::getEffectsRecursively(op);
    EXPECT_TRUE(effects.has_value());
    return effects && llvm::any_of(*effects, [](const auto &effect) {
             return mlir::isa<Effect>(effect.getEffect()) &&
                    effect.getResource() == Resource::get();
           });
  }
  void orderedCheck(mlir::Operation *operation) {
    EXPECT_TRUE(
        (has<mlir::MemoryEffects::Write, SourceCheckResource>(operation)));
    EXPECT_TRUE((has<mlir::MemoryEffects::Write, SourceEvaluationOrderResource>(
        operation)));
    EXPECT_FALSE(mlir::isSpeculatable(operation));
    EXPECT_FALSE(mlir::isOpTriviallyDead(operation));
  }
  mlir::MLIRContext context;
};

TEST_F(SourceEffectsTest, LiteralsArePureAndTotalArithmeticHasNoFailureEffect) {
  auto file =
      parse("%sum = \"ac.src.binary\"(%read, %i) {opcode = \"add\"} : "
            "(!ac.math_int, !ac.math_int) -> !ac.math_int\n"
            "%neg = \"ac.src.unary\"(%i) {opcode = \"neg\"} : (!ac.math_int) "
            "-> !ac.math_int\n"
            "%cmp = \"ac.src.compare\"(%read, %i) {predicate = \"lt\"} : "
            "(!ac.math_int, !ac.math_int) -> !ac.bool\n"
            "%cast = \"ac.src.int_cast\"(%b) : (!ac.bool) -> !ac.math_int");
  ASSERT_TRUE(file);
  auto integer = find<SrcIntOp>(*file);
  auto boolean = find<SrcBoolOp>(*file);
  EXPECT_TRUE(mlir::isMemoryEffectFree(integer));
  EXPECT_TRUE(mlir::isMemoryEffectFree(boolean));
  EXPECT_TRUE(mlir::isSpeculatable(integer));
  EXPECT_TRUE(mlir::isSpeculatable(boolean));
  for (mlir::Operation *op : {find<SrcUnaryOp>(*file).getOperation(),
                              find<SrcBinaryOp>(*file).getOperation(),
                              find<SrcCompareOp>(*file).getOperation(),
                              find<SrcIntCastOp>(*file).getOperation()})
    EXPECT_TRUE(mlir::isMemoryEffectFree(op));
}

TEST_F(SourceEffectsTest, CurrentReadEffectBindsTheActualReference) {
  auto file = parse("");
  ASSERT_TRUE(file);
  auto read = find<SrcReadOp>(*file);
  auto interface =
      mlir::dyn_cast<mlir::MemoryEffectOpInterface>(read.getOperation());
  ASSERT_TRUE(interface);
  llvm::SmallVector<mlir::MemoryEffects::EffectInstance> effects;
  interface.getEffects(effects);
  ASSERT_FALSE(effects.empty());
  bool matched = false;
  for (const auto &effect : effects)
    if (mlir::isa<mlir::MemoryEffects::Read>(effect.getEffect()) &&
        effect.getResource() == SourceCurrentStateResource::get()) {
      matched = true;
      EXPECT_EQ(effect.getValue(), read->getOperand(0));
    }
  EXPECT_TRUE(matched);
  EXPECT_FALSE(
      (has<mlir::MemoryEffects::Write, SourceCurrentStateResource>(read)));
}

TEST_F(SourceEffectsTest,
       PotentiallyFailingArithmeticCannotBeDeletedOrSpeculated) {
  for (llvm::StringRef opcode : {"floordiv", "mod", "shl", "shr"}) {
    auto file = parse("%unused = \"ac.src.binary\"(%read, %i) {opcode = \"" +
                      opcode.str() +
                      "\"} : (!ac.math_int, !ac.math_int) -> !ac.math_int");
    ASSERT_TRUE(file);
    auto binary = find<SrcBinaryOp>(*file);
    ASSERT_TRUE(binary.getResult().use_empty());
    orderedCheck(binary);
  }
  for (llvm::StringRef opcode :
       {"add", "sub", "mul", "and_bits", "or_bits", "xor_bits"}) {
    auto file = parse("%unused = \"ac.src.binary\"(%read, %i) {opcode = \"" +
                      opcode.str() +
                      "\"} : (!ac.math_int, !ac.math_int) -> !ac.math_int");
    ASSERT_TRUE(file);
    auto binary = find<SrcBinaryOp>(*file);
    EXPECT_TRUE(mlir::isMemoryEffectFree(binary));
    EXPECT_TRUE(mlir::isOpTriviallyDead(binary));
  }
}

TEST_F(SourceEffectsTest,
       ChecksProposalsAndPublicationShareAConflictingOrderResource) {
  auto file =
      parse("%bound = \"ac.src.bound\"(%read) {domain = #integer, diagnostic = "
            "#diagnostic, occurrence = #occ} : (!ac.math_int) -> !ac.math_int\n"
            "\"ac.src.assert\"(%b) {diagnostic = #diagnostic, occurrence = "
            "#occ} : (!ac.bool) -> ()\n"
            "\"ac.src.propose\"(%captured, %i) {occurrence = #occ} : "
            "(!ac.ref<#integer>, !ac.math_int) -> ()\n"
            "\"ac.src.observe\"(%i) {kind = \"report\", spec = {name = "
            "\"count\"}, occurrence = #occ} : (!ac.math_int) -> ()");
  ASSERT_TRUE(file);
  orderedCheck(find<SrcBoundOp>(*file));
  orderedCheck(find<SrcAssertOp>(*file));
  auto proposal = find<SrcProposeOp>(*file);
  auto observation = find<SrcObserveOp>(*file);
  EXPECT_TRUE(
      (has<mlir::MemoryEffects::Write, SourceProposalResource>(proposal)));
  EXPECT_TRUE((
      has<mlir::MemoryEffects::Write, SourcePublicationResource>(observation)));
  for (mlir::Operation *operation :
       {proposal.getOperation(), observation.getOperation()}) {
    EXPECT_TRUE((has<mlir::MemoryEffects::Write, SourceEvaluationOrderResource>(
        operation)));
    EXPECT_FALSE(mlir::isSpeculatable(operation));
    EXPECT_FALSE(mlir::isOpTriviallyDead(operation));
  }
  auto effects = mlir::getEffectsRecursively(proposal);
  ASSERT_TRUE(effects);
  bool bindsReference = false;
  for (const auto &effect : *effects) {
    if (effect.getResource() == SourceProposalResource::get())
      bindsReference |= effect.getValue() == proposal->getOperand(0);
    if (effect.getResource() == SourceEvaluationOrderResource::get())
      EXPECT_FALSE(effect.getValue())
          << "evaluation order is shared, not per-ref";
  }
  EXPECT_TRUE(bindsReference);
}

TEST_F(SourceEffectsTest,
       RecursiveIfRetainsNestedFailureAndEvaluationOrdering) {
  auto file = parse(R"ir(
"ac.src.if"(%b) ({
  "ac.src.if"(%b) ({
    %unused = "ac.src.binary"(%read, %i) {opcode = "floordiv"} : (!ac.math_int, !ac.math_int) -> !ac.math_int
    "ac.src.yield"() : () -> ()
  }, {
    "ac.src.yield"() : () -> ()
  }) : (!ac.bool) -> ()
  "ac.src.yield"() : () -> ()
}, {
  "ac.src.yield"() : () -> ()
}) : (!ac.bool) -> ()
)ir");
  ASSERT_TRUE(file);
  unsigned visited = 0;
  file->walk([&](SrcIfOp branch) {
    ++visited;
    EXPECT_TRUE(branch->hasTrait<mlir::OpTrait::HasRecursiveMemoryEffects>());
    orderedCheck(branch);
  });
  EXPECT_GT(visited, 0u);
}

} // namespace
} // namespace acir::ac
