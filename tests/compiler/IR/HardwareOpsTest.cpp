#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <string>

namespace acir::ac {
namespace {

// These tests deliberately verify local operation contracts. Complete error
// closure, dependency cycles and runtime transitions need their separate gates.
const char *metadata = R"ir(
#owner = {package = "test", path = "hardware.py"}
#root = {source_owner = #owner, qualified_name = "Root"}
#child = {source_owner = #owner, qualified_name = "Child"}
#occ = {site = {definition = @Root, ast_path = []}, expansion = []}
#span = {path = "hardware.py", line = 1 : i64, column = 1 : i64,
         end_line = 1 : i64, end_column = 2 : i64}
#state = {definition = #root, declaration = #occ, element = []}
#a = {kind = "formal", parameter = "a", ordinal = 3 : i64}
#b = {kind = "formal", parameter = "b", ordinal = 8 : i64}
#x = {kind = "formal", parameter = "x", ordinal = 17 : i64}
#y = {kind = "formal", parameter = "y", ordinal = 29 : i64}
#clock = {name = "clk", role = "clock", type = !ac.clock}
#reset = {name = "rst", role = "reset", type = i1}
#permit = {name = "commit", role = "permit", type = !ac.commit_permit}
#error = {name = "failure", role = "error", type = !ac.error}
)ir";

std::string integerDomain(unsigned width, llvm::StringRef lower,
                          llvm::StringRef upper, bool isSigned = false) {
  return "{kind = \"integer\", storage = i" + std::to_string(width) +
         ", lower = #ac.math_int<" + lower.str() + ">, upper = #ac.math_int<" +
         upper.str() + ">, interpretation = \"" +
         (isSigned ? "signed" : "unsigned") + "\"}";
}

std::string module(llvm::StringRef body) {
  return std::string(metadata) + R"ir(
module {
  "ac.hw.module"() ({
  ^bb0(%clk: !ac.clock, %rst: i1, %permit: !ac.commit_permit):
)ir" + body.str() +
         R"ir(
  }) {sym_name = "Root", source_owner = #owner, definition = #root,
      domain = "default", ports = [#clock, #reset, #permit, #error],
      function_type = (!ac.clock, i1, !ac.commit_permit) -> !ac.error} : () -> ()
})ir";
}

std::string registerModule(unsigned width, llvm::StringRef logical,
                           llvm::StringRef initial) {
  std::string type = "i" + std::to_string(width);
  return module("%on = arith.constant true\n"
                "%q = \"ac.seq.reg\"(%d, %on, %clk, %rst, %permit) "
                "{state_decl = #state, logical_domain = " +
                logical.str() + ", initial_bits = " + initial.str() + " : " +
                type + "} : (" + type +
                ", i1, !ac.clock, i1, !ac.commit_permit) -> " + type +
                "\n%one = arith.constant 1 : " + type +
                "\n%d = arith.addi %q, %one : " + type +
                "\n%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n"
                "\"ac.hw.output\"(%e) : (!ac.error) -> ()");
}

std::string port(llvm::StringRef name, llvm::StringRef role,
                 llvm::StringRef ref, llvm::StringRef domain) {
  return "{name = \"" + name.str() + "\", role = \"" + role.str() +
         "\", type = " + (role == "enable" ? "i1" : "i8") + ", formal_ref = #" +
         ref.str() + (role == "enable" ? "" : ", domain = " + domain.str()) +
         "}";
}

// Both child formals receive one actual when alias=true. Output presence,
// rather than names or enable constants, determines whether that formal is
// writable.
std::string hierarchy(bool writesA, bool writesB, bool alias = false) {
  const std::string domain = integerDomain(8, "0", "256");
  std::string outputs, outputTypes, resultNames;
  std::string ports = "[#clock, #reset, #permit, " +
                      port("a", "current", "a", domain) + ", " +
                      port("b", "current", "b", domain);
  for (auto [writes, ref] :
       {std::pair{writesA, "a"}, std::pair{writesB, "b"}}) {
    if (!writes)
      continue;
    ports += ", " + port(std::string(ref) + "_next", "next", ref, domain) +
             ", " + port(std::string(ref) + "_enable", "enable", ref, domain);
    outputs += "%" + std::string(ref) + ", %on, ";
    outputTypes += "i8, i1, ";
    resultNames += "%" + std::string(ref) + "_next, %" + ref + "_enable, ";
  }
  ports += ", #error]";
  outputTypes += "!ac.error";
  resultNames += "%child_error";
  return std::string(metadata) +
         "module {\n"
         "\"ac.hw.module\"() ({\n"
         "^bb0(%clk: !ac.clock, %rst: i1, %permit: !ac.commit_permit, %a: i8, "
         "%b: i8):\n"
         "%on = arith.constant true\n"
         "%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n"
         "\"ac.hw.output\"(" +
         outputs + "%e) : (" + outputTypes +
         ") -> ()\n"
         "}) {sym_name = \"Child\", source_owner = #owner, definition = "
         "#child, "
         "domain = \"default\", ports = " +
         ports +
         ", function_type = (!ac.clock, i1, !ac.commit_permit, i8, i8) -> (" +
         outputTypes +
         ")} : () -> ()\n"
         "\"ac.hw.module\"() ({\n"
         "^bb0(%clk: !ac.clock, %rst: i1, %permit: !ac.commit_permit, %x: i8, "
         "%y: i8):\n" +
         resultNames + " = \"ac.hw.instance\"(%clk, %rst, %permit, %x, " +
         (alias ? "%x" : "%y") +
         ") {callee = @Child, occurrence = #occ, "
         "actual_ref_bindings = [{formal_ref = #a, actual = #x}, "
         "{formal_ref = #b, actual = #" +
         (alias ? "x" : "y") +
         "}]} : (!ac.clock, i1, !ac.commit_permit, i8, i8) -> (" + outputTypes +
         ")\n"
         "\"ac.hw.output\"(%child_error) : (!ac.error) -> ()\n"
         "}) {sym_name = \"Root\", source_owner = #owner, definition = #root, "
         "domain = \"default\", ports = [#clock, #reset, #permit, " +
         port("x", "current", "x", domain) + ", " +
         port("y", "current", "y", domain) +
         ", #error], function_type = (!ac.clock, i1, !ac.commit_permit, i8, "
         "i8) -> !ac.error} "
         ": () -> ()\n}";
}

class HardwareOpsTest : public ::testing::Test {
protected:
  HardwareOpsTest() {
    context.loadDialect<ACIRDialect, mlir::arith::ArithDialect>();
  }

  mlir::OwningOpRef<mlir::ModuleOp> parse(llvm::StringRef text) {
    return mlir::parseSourceString<mlir::ModuleOp>(
        text, mlir::ParserConfig(&context, /*verifyAfterParse=*/false));
  }

  void accepts(llvm::StringRef text, bool roundtrip = false) {
    auto file = parse(text);
    ASSERT_TRUE(file);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*file)));
    if (roundtrip) {
      std::string printed;
      llvm::raw_string_ostream stream(printed);
      file->print(stream);
      auto copy = parse(printed);
      ASSERT_TRUE(copy);
      EXPECT_TRUE(mlir::succeeded(mlir::verify(*copy)));
    }
  }

  void rejects(llvm::StringRef text, llvm::StringRef reason) {
    auto file = parse(text);
    ASSERT_TRUE(file) << "negative oracle must reach verification";
    std::string diagnostics;
    mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &d) {
      llvm::raw_string_ostream stream(diagnostics);
      d.print(stream);
      return mlir::success();
    });
    EXPECT_TRUE(mlir::failed(mlir::verify(*file)));
    EXPECT_NE(diagnostics.find(reason.str()), std::string::npos) << diagnostics;
  }

  std::string replace(std::string text, llvm::StringRef old,
                      llvm::StringRef replacement) {
    size_t offset = text.find(old.str());
    EXPECT_NE(offset, std::string::npos);
    if (offset != std::string::npos)
      text.replace(offset, old.size(), replacement.str());
    return text;
  }

  mlir::MLIRContext context;
};

TEST_F(HardwareOpsTest, ZeroStateZeroCheckModuleAndSystemRoundtrip) {
  std::string text = module("%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n"
                            "\"ac.hw.output\"(%e) : (!ac.error) -> ()");
  text.insert(text.rfind('}'), R"ir(
  "ac.hw.system"() ({
  ^bb0(%clk: !ac.clock, %rst: i1):
    %e = "ac.hw.instance"(%clk, %rst, %permit)
      {callee = @Root, occurrence = #occ, actual_ref_bindings = []}
      : (!ac.clock, i1, !ac.commit_permit) -> !ac.error
    %permit = "ac.hw.commit_permit"(%e) : (!ac.error) -> !ac.commit_permit
    "ac.hw.output"() : () -> ()
  }) {sym_name = "Test", source_owner = #owner, domain = "default"} : () -> ()
)ir");
  accepts(text, true);
  rejects(replace(text, "callee = @Root", "callee = @Missing"),
          "callee must resolve");
  rejects(
      replace(text, "qualified_name = \"Root\"", "qualified_name = \"Other\""),
      "qualified_name must match");
}

TEST_F(HardwareOpsTest, SequentialFeedbackAndFiniteInitializationBoundaries) {
  accepts(registerModule(1, "{kind = \"bool\", storage = i1}", "1"), true);
  accepts(registerModule(7, integerDomain(7, "-64", "64", true), "-64"));
  accepts(registerModule(7, integerDomain(7, "-64", "64", true), "63"));
  accepts(registerModule(64, integerDomain(64, "0", "18446744073709551616"),
                         "18446744073709551615"));
  const std::string limited = integerDomain(7, "-50", "50", true);
  accepts(registerModule(7, limited, "-50"));
  accepts(registerModule(7, limited, "49"));
  rejects(registerModule(7, limited, "-51"), "outside declared logical domain");
  rejects(registerModule(7, limited, "50"), "outside declared logical domain");
  rejects(registerModule(7, integerDomain(7, "5", "100"), "4"),
          "outside declared logical domain");
  rejects(registerModule(7, integerDomain(7, "5", "100"), "100"),
          "outside declared logical domain");
  rejects(
      registerModule(65, integerDomain(65, "0", "36893488147419103232"), "0"),
      "finite bool/integer storage");
}

TEST_F(HardwareOpsTest, RegisterControlIdentityAndUniqueOwnership) {
  const std::string text =
      registerModule(7, integerDomain(7, "0", "128"), "37");
  rejects(replace(text, "%d, %on, %clk, %rst, %permit",
                  "%d, %on, %clk, %on, %permit"),
          "identity-forward owning controls");
  rejects(replace(text, "initial_bits = 37 : i7", "initial_bits = 37 : i8"),
          "exactly the register finite type");
  rejects(replace(text, "qualified_name = \"Root\"}",
                  "qualified_name = \"Root\", extra = 0 : i64}"),
          "definition requires source_owner");
  auto file = parse(text);
  ASSERT_TRUE(file);
  SeqRegOp reg;
  file->walk([&](SeqRegOp candidate) { reg = candidate; });
  ASSERT_TRUE(reg);
  reg->getBlock()->getOperations().insert(reg->getIterator(), reg->clone());
  EXPECT_TRUE(mlir::failed(mlir::verify(*file)));
}

TEST_F(HardwareOpsTest, InstanceBindingsUsePhysicalAndLogicalIdentity) {
  const std::string text = hierarchy(true, false);
  accepts(text, true);
  const std::string resetSubstitution = replace(
      replace(
          text, "%a_next, %a_enable, %child_error =",
          "%false = arith.constant false\n%a_next, %a_enable, %child_error ="),
      "\"ac.hw.instance\"(%clk, %rst, %permit",
      "\"ac.hw.instance\"(%clk, %false, %permit");
  rejects(resetSubstitution, "identity-forward clock/reset/permit");
  rejects(replace(text, "%permit, %x, %y)", "%permit, %y, %x)"),
          "matching owner current SSA/domain");
  rejects(replace(text, "actual = #y", "actual = #x"),
          "matching owner current SSA/domain");
  rejects(replace(text, "formal_ref = #b, actual", "formal_ref = #a, actual"),
          "ordered callee formal_ref");
  rejects(replace(text, "callee = @Child", "callee = @Missing"),
          "callee must resolve");
  auto file = parse(text);
  ASSERT_TRUE(file);
  HwInstanceOp instance;
  file->walk([&](HwInstanceOp candidate) { instance = candidate; });
  ASSERT_TRUE(instance);
  instance->getResult(0).setType(mlir::IntegerType::get(&context, 7));
  EXPECT_TRUE(mlir::failed(mlir::verify(*file)));
  // Equal physical storage still cannot substitute another logical domain.
  std::string different =
      replace(text, port("x", "current", "x", integerDomain(8, "0", "256")),
              port("x", "current", "x", integerDomain(8, "1", "256")));
  rejects(different, "matching owner current SSA/domain");
}

TEST_F(HardwareOpsTest, WritableAliasesRejectButReadAliasesRemainLegal) {
  accepts(hierarchy(false, false, true));
  accepts(hierarchy(true, false, true));
  accepts(hierarchy(false, true, true));
  accepts(hierarchy(true, true, false));
  rejects(hierarchy(true, true, true),
          "distinct writable formals cannot alias");
}

TEST_F(HardwareOpsTest, SiblingOccurrencesIdentifyInstancesAcrossCallees) {
  auto file = parse(hierarchy(false, false));
  ASSERT_TRUE(file);
  HwModuleOp child;
  HwInstanceOp first;
  file->walk([&](HwModuleOp candidate) {
    if (candidate.getSymName() == "Child")
      child = candidate;
  });
  file->walk([&](HwInstanceOp candidate) { first = candidate; });
  ASSERT_TRUE(child && first);
  mlir::Builder builder(&context);
  auto other = mlir::cast<HwModuleOp>(child->clone());
  other->setAttr("sym_name", builder.getStringAttr("OtherChild"));
  auto definition = child->getAttrOfType<mlir::DictionaryAttr>("definition");
  other->setAttr(
      "definition",
      builder.getDictionaryAttr(
          {builder.getNamedAttr("source_owner", definition.get("source_owner")),
           builder.getNamedAttr("qualified_name",
                                builder.getStringAttr("OtherChild"))}));
  child->getBlock()->getOperations().insert(child->getIterator(), other);
  auto second = mlir::cast<HwInstanceOp>(first->clone());
  first->getBlock()->getOperations().insert(first->getIterator(), second);

  auto print = [&]() {
    std::string text;
    llvm::raw_string_ostream stream(text);
    file->print(stream);
    return text;
  };
  // Physical identity belongs to the owner occurrence, independent of callee.
  rejects(print(), "instance occurrence must be unique");
  second->setAttr("callee",
                  mlir::FlatSymbolRefAttr::get(&context, "OtherChild"));
  rejects(print(), "instance occurrence must be unique");

  auto occurrence = first->getAttrOfType<mlir::DictionaryAttr>("occurrence");
  auto expanded = [&](uint64_t ordinal) {
    auto frame = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("iteration")),
         builder.getNamedAttr("site", occurrence.get("site")),
         builder.getNamedAttr("ordinal", builder.getI64IntegerAttr(ordinal)),
         builder.getNamedAttr(
             "value",
             builder.getDictionaryAttr(
                 {builder.getNamedAttr("kind",
                                       builder.getStringAttr("integer")),
                  builder.getNamedAttr(
                      "value", MathIntAttr::get(&context,
                                                llvm::APSInt(llvm::APInt(8, 19),
                                                             true)))}))});
    return builder.getDictionaryAttr(
        {builder.getNamedAttr("site", occurrence.get("site")),
         builder.getNamedAttr("expansion", builder.getArrayAttr({frame}))});
  };
  for (auto [left, right] : {std::pair<uint64_t, uint64_t>{0, 1}, {13, 4099}}) {
    first->setAttr("occurrence", expanded(left));
    second->setAttr("occurrence", expanded(right));
    accepts(print(), true);
    second->setAttr("callee", first->getAttr("callee"));
    accepts(print());
    second->setAttr("occurrence", expanded(left));
    rejects(print(), "instance occurrence must be unique");
    second->setAttr("callee",
                    mlir::FlatSymbolRefAttr::get(&context, "OtherChild"));
  }
}

TEST_F(HardwareOpsTest, LocalChecksReductionAndObservationsRoundtrip) {
  const std::string domain = integerDomain(7, "0", "128");
  const std::string text =
      module("%on = arith.constant true\n%value = arith.constant 11 : i7\n"
             "%check = \"ac.hw.check\"(%on, %on) {source_owner = #owner, "
             "occurrence = #occ, "
             "diagnostic = {kind = \"assert\", message = \"bounded\", location "
             "= #span}} "
             ": (i1, i1) -> !ac.error\n"
             "%e = \"ac.hw.error_reduce\"(%check) : (!ac.error) -> !ac.error\n"
             "\"ac.hw.observe\"(%on, %permit, %value) {kind = \"print\", "
             "spec = {sep = \" \", end = \"\\n\", items = [{kind = "
             "\"literal\", text = \"value\"}, "
             "{kind = \"value\", ordinal = 0 : i32}]}, domains = [" +
             domain +
             "], source_owner = #owner, occurrence = #occ} : (i1, "
             "!ac.commit_permit, i7) -> ()\n"
             "\"ac.hw.output\"(%e) : (!ac.error) -> ()");
  accepts(text, true);
  rejects(replace(text, "(%check) : (!ac.error)",
                  "(%check, %check) : (!ac.error, !ac.error)"),
          "duplicate input");
  rejects(replace(text, "ordinal = 0 : i32", "ordinal = 1 : i32"),
          "cover values exactly in order");
  rejects(replace(text, "kind = \"print\"", "kind = \"trace\""),
          "observation kind must be");
  rejects(replace(text, "kind = \"assert\", message", "kind = \"\", message"),
          "diagnostic requires");
  rejects(replace(text, "definition = @Root, ast_path",
                  "definition = @Other, ast_path"),
          "occurrence must match");
  const std::string log = replace(
      replace(text, "kind = \"print\"", "kind = \"log\""),
      "sep = \" \", end = \"\\n\"", "level = \"info\", event = \"sample\"");
  accepts(log);
  rejects(replace(log, "level = \"info\"", "level = \"verbose\""),
          "log requires supported level");
  accepts(module("%on = arith.constant true\n%value = arith.constant 11 : i7\n"
                 "%e = \"ac.hw.error_reduce\"() : () -> !ac.error\n"
                 "\"ac.hw.observe\"(%on, %permit, %value) {kind = \"report\", "
                 "spec = {name = \"sample\"}, domains = [" +
                 domain +
                 "], source_owner = #owner, occurrence = #occ} "
                 ": (i1, !ac.commit_permit, i7) -> ()\n"
                 "\"ac.hw.output\"(%e) : (!ac.error) -> ()"));
}

} // namespace
} // namespace acir::ac
