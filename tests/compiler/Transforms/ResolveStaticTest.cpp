#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Transforms/Passes.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>

namespace acir::ac {
namespace {

const char *metadata = R"ir(
#owner = {package = "resolve", path = "resolve.py"}
#def = {source_owner = #owner, qualified_name = "Test"}
#occ = {site = {definition = @Test, ast_path = []}, expansion = []}
#aocc = {site = {definition = @Test, ast_path = [{kind = "index", value = 1 : i64}]}, expansion = []}
#bocc = {site = {definition = @Test, ast_path = [{kind = "index", value = 2 : i64}]}, expansion = []}
#astate = {definition = #def, declaration = #aocc, element = []}
#bstate = {definition = #def, declaration = #bocc, element = []}
#span = {path = "resolve.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}
#site = {ast_path = [{kind = "field", name = "body"}], location = #span}
#diagnostic = {kind = "range", message = "bound", location = #span}
#formal = {kind = "formal", parameter = "input", ordinal = 0 : i64}
)ir";

std::string literal(llvm::StringRef value, bool boolean = false) {
  return "#ac.static_expr<{kind = \"literal\", value = {kind = \"" +
         std::string(boolean ? "bool" : "integer") + "\", value = " +
         (boolean ? value.str() : "#ac.math_int<" + value.str() + ">") +
         "}, origin = #occ, location = #span}>";
}
std::string unary(llvm::StringRef opcode, llvm::StringRef value) {
  return "#ac.static_expr<{kind = \"unary\", operator = \"" + opcode.str() +
         "\", operand = " + value.str() + ", origin = #occ, location = #span}>";
}
std::string binary(llvm::StringRef opcode, llvm::StringRef lhs,
                   llvm::StringRef rhs) {
  return "#ac.static_expr<{kind = \"binary\", operator = \"" + opcode.str() +
         "\", lhs = " + lhs.str() + ", rhs = " + rhs.str() +
         ", origin = #occ, location = #span}>";
}
std::string annotation(llvm::StringRef lower, llvm::StringRef upper) {
  return "#ac.source_type_expr<{kind = \"integer\", lower = " + lower.str() +
         ", upper = " + upper.str() + "}>";
}
std::string signedAnnotation() {
  return annotation(unary("neg", literal("3")),
                    binary("mul", literal("2"), literal("4")));
}
std::string signedInit() { return binary("sub", literal("2"), literal("5")); }

std::string design(llvm::StringRef domain, llvm::StringRef first,
                   llvm::StringRef second, bool boolDesign = false,
                   bool registered = true) {
  const std::string bound =
      boolDesign ? ""
                 : "%bound = \"ac.src.bound\"(%outer) {domain = #annotation, "
                   "diagnostic = #diagnostic, occurrence = #occ} : "
                   "(!ac.unresolved) -> !ac.math_int\n";
  const std::string proposal =
      boolDesign ? "\"ac.src.propose\"(%ca, %outer) {occurrence = #occ} : "
                   "(!ac.unresolved, !ac.unresolved) -> ()\n"
                 : "\"ac.src.propose\"(%ca, %bound) {occurrence = #occ} : "
                   "(!ac.unresolved, !ac.math_int) -> ()\n";
  return std::string(metadata) + "#annotation = " + domain.str() +
         "\n#first = " + first.str() + "\n#second = " + second.str() + R"ir(
module {
"ac.src.unit"() ({
"ac.src.module"() ({
^bb0(%clock: !ac.clock, %reset: !ac.bool, %input: !ac.unresolved):
%a = "ac.src.reg"(%clock, %reset) {state_decl = #astate, annotation = #annotation, initializer = #first} : (!ac.clock, !ac.bool) -> !ac.unresolved
%b = "ac.src.reg"(%clock, %reset) {state_decl = #bstate, annotation = #annotation, initializer = #second} : (!ac.clock, !ac.bool) -> !ac.unresolved
"ac.src.rule"(%reset, %input, %a, %b) ({
^bb0(%rr: !ac.bool, %formal: !ac.unresolved, %ca: !ac.unresolved, %cb: !ac.unresolved):
%condition = "ac.src.bool"() {value = true} : () -> !ac.bool
%formal_value = "ac.src.read"(%formal) {occurrence = #occ} : (!ac.unresolved) -> !ac.unresolved
%av = "ac.src.read"(%ca) {occurrence = #occ} : (!ac.unresolved) -> !ac.unresolved
%bv = "ac.src.read"(%cb) {occurrence = #occ} : (!ac.unresolved) -> !ac.unresolved
%inner = "ac.src.if"(%condition) ({
  "ac.src.yield"(%av) : (!ac.unresolved) -> ()
}, {
  "ac.src.yield"(%formal_value) : (!ac.unresolved) -> ()
}) : (!ac.bool) -> !ac.unresolved
%outer = "ac.src.if"(%condition) ({
  "ac.src.yield"(%inner) : (!ac.unresolved) -> ()
}, {
  "ac.src.yield"(%bv) : (!ac.unresolved) -> ()
}) : (!ac.bool) -> !ac.unresolved
)ir" + bound +
         proposal + R"ir(
"ac.src.yield"() : () -> ()
}) {sym_name = "step", occurrence = #occ} : (!ac.bool, !ac.unresolved, !ac.unresolved, !ac.unresolved) -> ()
)ir" +
         (registered ? "\"ac.src.register\"() {rule = @step, occurrence = "
                       "#aocc} : () -> ()\n"
                     : "") +
         R"ir(
"ac.src.yield"() : () -> ()
}) {sym_name = "Test", definition = #def, domain = "default", static_parameters = [],
    parameters = [{name = "input", binding = "positional_or_keyword", domain = #annotation, formal_ref = #formal}],
    function_type = (!ac.clock, !ac.bool, !ac.unresolved) -> ()} : () -> ()
}) {source_owner = #owner, kind = "body", interfaces = [#owner], exports = []} : () -> ()
}
)ir";
}
std::string print(mlir::Operation *op) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  op->print(stream);
  return text;
}

class ResolveStaticTest : public ::testing::Test {
protected:
  ResolveStaticTest() { context.loadDialect<ACIRDialect>(); }
  mlir::OwningOpRef<mlir::ModuleOp> parse(llvm::StringRef text) {
    auto file = mlir::parseSourceString<mlir::ModuleOp>(
        text, mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
    EXPECT_TRUE(file);
    return file;
  }
  bool resolve(mlir::ModuleOp file, bool registered = false) {
    diagnostics.clear();
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    mlir::PassManager manager(&context);
    if (registered) {
      acir::registerACIRPasses();
      if (mlir::failed(
              mlir::parsePassPipeline("pycircuit-resolve-static", manager)))
        return false;
    } else
      manager.addPass(acir::createResolveStaticPass());
    return mlir::succeeded(manager.run(file));
  }
  template <typename Op> Op find(mlir::ModuleOp file) {
    Op result;
    file.walk([&](Op op) {
      if (!result)
        result = op;
    });
    EXPECT_TRUE(result);
    return result;
  }
  llvm::SmallVector<SrcRegOp> regs(mlir::ModuleOp file) {
    llvm::SmallVector<SrcRegOp> result;
    file.walk([&](SrcRegOp reg) { result.push_back(reg); });
    return result;
  }
  void rejectsUnchanged(mlir::ModuleOp file) {
    const std::string before = print(file);
    EXPECT_FALSE(resolve(file));
    EXPECT_FALSE(diagnostics.empty());
    EXPECT_EQ(print(file), before) << "failed resolution published partial IR";
  }
  void expectNoPending(mlir::ModuleOp file) {
    file.walk([&](mlir::Operation *op) {
      for (auto type : op->getOperandTypes())
        EXPECT_FALSE(mlir::isa<UnresolvedType>(type));
      for (auto type : op->getResultTypes())
        EXPECT_FALSE(mlir::isa<UnresolvedType>(type));
      for (auto &region : op->getRegions())
        for (auto &block : region)
          for (auto argument : block.getArguments())
            EXPECT_FALSE(mlir::isa<UnresolvedType>(argument.getType()));
      if (auto reg = mlir::dyn_cast<SrcRegOp>(op)) {
        EXPECT_FALSE(reg->hasAttr("annotation"));
        EXPECT_FALSE(reg->hasAttr("initializer"));
        EXPECT_TRUE(reg->hasAttr("initial_value"));
      }
      if (auto bound = mlir::dyn_cast<SrcBoundOp>(op))
        EXPECT_TRUE(mlir::isa<SourceDomainAttr>(bound->getAttr("domain")));
    });
  }
  void expectDomain(SourceDomainAttr domain, llvm::StringRef kind,
                    llvm::StringRef lower = {}, llvm::StringRef upper = {}) {
    ASSERT_TRUE(domain);
    auto fields = domain.getValue();
    EXPECT_EQ(fields.getAs<mlir::StringAttr>("kind").getValue(), kind);
    EXPECT_FALSE(fields.get("storage"));
    EXPECT_FALSE(fields.get("interpretation"));
    EXPECT_FALSE(fields.get("signedness"));
    if (kind == "integer") {
      EXPECT_EQ(fields.getAs<MathIntAttr>("lower").getCanonicalValue(), lower);
      EXPECT_EQ(fields.getAs<MathIntAttr>("upper").getCanonicalValue(), upper);
    }
  }
  mlir::MLIRContext context;
  std::string diagnostics;
};

TEST_F(ResolveStaticTest,
       ComputedDomainsUpdateFormalsCapturesReadsAndNestedJoins) {
  auto file = parse(design(signedAnnotation(), signedInit(), literal("7")));
  ASSERT_TRUE(file);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)));
  ASSERT_TRUE(resolve(*file)) << diagnostics;
  expectNoPending(*file);
  auto module = find<SrcModuleOp>(*file);
  auto formalType =
      mlir::cast<RefType>(module.getBody().front().getArgument(2).getType());
  expectDomain(formalType.getDomain(), "integer", "-3", "8");
  auto parameters = module->getAttrOfType<mlir::ArrayAttr>("parameters");
  EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(parameters[0]).get("domain"),
            formalType.getDomain());
  auto rule = find<SrcRuleOp>(*file);
  for (unsigned index = 0; index < rule->getNumOperands(); ++index)
    EXPECT_EQ(rule->getOperand(index).getType(),
              rule.getBody().front().getArgument(index).getType());
  file->walk([&](SrcReadOp read) {
    EXPECT_TRUE(mlir::isa<MathIntType>(read.getResult().getType()));
  });
  file->walk([&](SrcIfOp branch) {
    EXPECT_TRUE(mlir::isa<MathIntType>(branch.getResult(0).getType()));
  });
  auto state = regs(*file);
  ASSERT_GE(state.size(), 2u);
  EXPECT_EQ(
      state[0]->getAttrOfType<MathIntAttr>("initial_value").getCanonicalValue(),
      "-3");
  EXPECT_EQ(
      state[1]->getAttrOfType<MathIntAttr>("initial_value").getCanonicalValue(),
      "7");
  auto saved = print(*file);
  auto reparsed = mlir::parseSourceString<mlir::ModuleOp>(saved, &context);
  ASSERT_TRUE(reparsed);
  mlir::PassManager inference(&context);
  inference.addPass(acir::createInferEffectsPass());
  EXPECT_TRUE(mlir::succeeded(inference.run(*reparsed)));
  ASSERT_TRUE(resolve(*file, true)) << diagnostics;
  EXPECT_EQ(print(*file), saved);
}

TEST_F(ResolveStaticTest, BooleanAndRangeTwoResolveToDistinctSourceKinds) {
  auto boolFile =
      parse(design("#ac.source_type_expr<{kind = \"bool\"}>",
                   literal("true", true), literal("false", true), true));
  ASSERT_TRUE(boolFile);
  ASSERT_TRUE(resolve(*boolFile)) << diagnostics;
  expectNoPending(*boolFile);
  auto boolType =
      mlir::cast<RefType>(regs(*boolFile).front().getResult().getType());
  expectDomain(boolType.getDomain(), "bool");
  boolFile->walk([&](SrcReadOp read) {
    EXPECT_TRUE(mlir::isa<BoolType>(read.getResult().getType()));
  });
  auto integerFile = parse(design(annotation(literal("0"), literal("2")),
                                  literal("1"), literal("0")));
  ASSERT_TRUE(integerFile);
  ASSERT_TRUE(resolve(*integerFile)) << diagnostics;
  auto integerType =
      mlir::cast<RefType>(regs(*integerFile).front().getResult().getType());
  expectDomain(integerType.getDomain(), "integer", "0", "2");
  EXPECT_NE(boolType, integerType);
}

TEST_F(ResolveStaticTest, HalfOpenMembershipAndLateFailuresAreTransactional) {
  for (llvm::StringRef value : {"-3", "7"}) {
    auto file = parse(design(signedAnnotation(), literal(value), literal("0")));
    ASSERT_TRUE(file);
    ASSERT_TRUE(resolve(*file)) << diagnostics;
  }
  for (llvm::StringRef value : {"-4", "8"}) {
    auto file = parse(design(signedAnnotation(), literal("0"), literal(value)));
    ASSERT_TRUE(file);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)));
    rejectsUnchanged(
        *file); // Second register fails after earlier facts resolve.
  }
  auto wrongKind =
      parse(design(signedAnnotation(), literal("false", true), literal("0")));
  ASSERT_TRUE(wrongKind);
  rejectsUnchanged(*wrongKind);
}

TEST_F(ResolveStaticTest, WideSourceDomainsAndInitializersDoNotSelectStorage) {
  const auto wide =
      annotation(literal("0"), binary("shl", literal("1"), literal("100")));
  auto file = parse(
      design(wide, binary("shl", literal("1"), literal("80")), literal("0")));
  ASSERT_TRUE(file);
  ASSERT_TRUE(resolve(*file)) << diagnostics;
  expectNoPending(*file);
  auto reg = regs(*file).front();
  expectDomain(mlir::cast<RefType>(reg.getResult().getType()).getDomain(),
               "integer", "0", "1267650600228229401496703205376");
  EXPECT_EQ(
      reg->getAttrOfType<MathIntAttr>("initial_value").getCanonicalValue(),
      "1208925819614629174706176");
  auto invalid = parse(
      design(wide, literal("0"), binary("shl", literal("1"), literal("100"))));
  ASSERT_TRUE(invalid);
  rejectsUnchanged(*invalid);
}

TEST_F(ResolveStaticTest,
       UnregisteredTypedBodiesResolveAndRemainInactiveForEffects) {
  auto file = parse(
      design(signedAnnotation(), signedInit(), literal("0"), false, false));
  ASSERT_TRUE(file);
  ASSERT_TRUE(resolve(*file)) << diagnostics;
  expectNoPending(*file);
  mlir::PassManager inference(&context);
  inference.addPass(acir::createInferEffectsPass());
  ASSERT_TRUE(mlir::succeeded(inference.run(*file)));
  auto effects =
      find<SrcModuleOp>(*file)->getAttrOfType<mlir::DictionaryAttr>("effects");
  ASSERT_TRUE(effects);
  auto entry = mlir::cast<mlir::DictionaryAttr>(
      effects.getAs<mlir::ArrayAttr>("refs")[0]);
  EXPECT_FALSE(entry.getAs<mlir::BoolAttr>("read").getValue());
  EXPECT_FALSE(entry.getAs<mlir::BoolAttr>("write").getValue());
  EXPECT_FALSE(effects.getAs<mlir::BoolAttr>("may_check").getValue());
}

TEST_F(ResolveStaticTest,
       SameOwnerCaptureSubstitutionPreservesTheActualReference) {
  auto file = parse(design(signedAnnotation(), signedInit(), literal("0")));
  ASSERT_TRUE(file);
  auto rule = find<SrcRuleOp>(*file);
  auto states = regs(*file);
  ASSERT_GE(states.size(), 2u);
  auto expectedIdentity = states[1]->getAttr("state_decl");
  rule->setOperand(
      2, states[1].getResult()); // Uref A→same-type Uref B is valid IR.
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)));
  ASSERT_TRUE(resolve(*file)) << diagnostics;
  auto resolvedRule = find<SrcRuleOp>(*file);
  auto actualReg = resolvedRule->getOperand(2).getDefiningOp<SrcRegOp>();
  ASSERT_TRUE(actualReg);
  EXPECT_EQ(actualReg->getAttr("state_decl"), expectedIdentity);
  bool readsActualCapture = false;
  file->walk([&](SrcReadOp read) {
    if (read->getOperand(0) == resolvedRule.getBody().front().getArgument(2))
      readsActualCapture = true;
  });
  EXPECT_TRUE(readsActualCapture);
}

TEST_F(ResolveStaticTest,
       InvalidCaptureTypesControlsAndInactiveBodiesRejectWithoutMutation) {
  auto make = [&] {
    return parse(
        design(signedAnnotation(), signedInit(), literal("0"), false, false));
  };
  auto types = make();
  ASSERT_TRUE(types);
  find<SrcRuleOp>(*types).getBody().front().getArgument(2).setType(
      MathIntType::get(&context));
  rejectsUnchanged(*types);
  auto arity = make();
  ASSERT_TRUE(arity);
  find<SrcRuleOp>(*arity)->eraseOperand(3);
  rejectsUnchanged(*arity);
  auto isolated = make();
  ASSERT_TRUE(isolated);
  auto originalModule = find<SrcModuleOp>(*isolated);
  auto other = mlir::cast<SrcModuleOp>(originalModule->clone());
  other->setAttr("sym_name", mlir::StringAttr::get(&context, "Other"));
  mlir::NamedAttrList definition(
      other->getAttrOfType<mlir::DictionaryAttr>("definition"));
  definition.set("qualified_name", mlir::StringAttr::get(&context, "Other"));
  other->setAttr("definition", definition.getDictionary(&context));
  auto &block = other.getBody().front();
  while (!block.empty())
    block.back().erase();
  mlir::OpBuilder builder(&context);
  builder.setInsertionPointToEnd(&block);
  builder.create(
      mlir::OperationState(other.getLoc(), SrcYieldOp::getOperationName()));
  originalModule->getBlock()->getOperations().push_back(other);
  find<SrcRuleOp>(*isolated)->setOperand(2, block.getArgument(2));
  rejectsUnchanged(
      *isolated); // Foreign isolated module ref, same unresolved type.
  auto reset = make();
  ASSERT_TRUE(reset);
  auto sourceRule = find<SrcRuleOp>(*reset);
  SrcBoolOp data;
  reset->walk([&](SrcBoolOp value) { data = value; });
  ASSERT_TRUE(data);
  sourceRule->setOperand(
      0,
      data.getResult()); // Same bool type, foreign implicit control identity.
  rejectsUnchanged(*reset);
  auto malformed = make();
  ASSERT_TRUE(malformed);
  auto read = find<SrcReadOp>(*malformed);
  read->setOperand(0, find<SrcBoolOp>(*malformed).getResult());
  rejectsUnchanged(*malformed);
}

TEST_F(ResolveStaticTest,
       UnsupportedAliasReferenceAndGeneralHelperNeverFallback) {
  const std::string alias = "#ac.source_type_expr<{kind = \"alias\", name = "
                            "[\"Word\"], site = #site}>";
  const std::string reference =
      "#ac.static_expr<{kind = \"reference\", ref = {kind = \"export\", symbol "
      "= @Unopened}, origin = #occ, location = #span}>";
  const std::string helper =
      "#ac.static_expr<{kind = \"call\", callee = @Unopened, arguments = [], "
      "origin = #occ, location = #span}>";
  for (const auto &domain : {alias, annotation(literal("0"), reference),
                             annotation(literal("0"), helper)}) {
    auto file = parse(design(domain, literal("0"), literal("0")));
    ASSERT_TRUE(file);
    rejectsUnchanged(*file);
  }
}

} // namespace
} // namespace acir::ac
