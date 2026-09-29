#include "Compiler/FinalProgram.h"
#include "Dialect/ACIR/ACIRFinalUses.h"
#include "Dialect/ACIR/ACIRHardwareClosure.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("final-generic-uses", path));
  }
  ~TemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};

void writeFile(llvm::StringRef path, llvm::StringRef contents) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error);
  output << contents;
}

std::string readFile(llvm::StringRef path) {
  auto contents = llvm::MemoryBuffer::getFile(path);
  return contents ? contents.get()->getBuffer().str() : std::string{};
}

int run(llvm::StringRef program, const std::vector<std::string> &arguments,
        llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> refs;
  for (const std::string &argument : arguments)
    refs.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, refs, std::nullopt, redirects);
}

std::string print(mlir::Attribute value) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  value.print(stream);
  return text;
}

mlir::DictionaryAttr replaceField(mlir::Builder &builder,
                                  mlir::DictionaryAttr dictionary,
                                  llvm::StringRef name,
                                  mlir::Attribute replacement) {
  llvm::SmallVector<mlir::NamedAttribute> fields(dictionary.begin(),
                                                 dictionary.end());
  for (mlir::NamedAttribute &field : fields)
    if (field.getName() == name) {
      field = builder.getNamedAttr(name, replacement);
      return builder.getDictionaryAttr(fields);
    }
  ADD_FAILURE() << "missing dictionary field " << name.str();
  return dictionary;
}

void rewriteUseID(ac::RuleOp rule, mlir::DictionaryAttr replacement,
                  mlir::Builder &builder) {
  auto required = rule->getAttrOfType<mlir::ArrayAttr>("ac.required_uses");
  auto item = mlir::cast<mlir::DictionaryAttr>(required[0]);
  rule->setAttr(
      "ac.required_uses",
      builder.getArrayAttr({replaceField(builder, item, "id", replacement)}));
  auto use = *rule.getBody().front().getOps<ac::ValueUseOp>().begin();
  use->setAttr("id", replacement);
  auto bindings = rule->getAttrOfType<mlir::ArrayAttr>("ac.yield_bindings");
  auto binding = mlir::cast<mlir::DictionaryAttr>(bindings[0]);
  auto contributions = binding.getAs<mlir::ArrayAttr>("contributions");
  auto contribution = mlir::cast<mlir::DictionaryAttr>(contributions[0]);
  contribution = replaceField(builder, contribution, "use", replacement);
  binding = replaceField(builder, binding, "contributions",
                         builder.getArrayAttr({contribution}));
  rule->setAttr("ac.yield_bindings", builder.getArrayAttr({binding}));
}

struct SourceUseFact {
  std::string id;
  std::string source;
  std::string target;
  bool operator==(const SourceUseFact &) const = default;
};

class FinalGenericUsesTest : public ::testing::Test {
protected:
  FinalGenericUsesTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    const std::string source = sourceRoot + "/uses.py";
    writeFile(source, R"py(from typing import Annotated
from pycircuit import rule, system
Word = Annotated[int, range(256)]
@system
def GenericUses():
    source: Word = 3
    ready: bool = True
    direct_out: Word = 0
    literal_out: Word = 0
    shared_left: Word = 0
    shared_right: Word = 0
    conditional_out: Word = 0
    inverse_out: Word = 0
    @rule
    def direct():
        nonlocal direct_out
        direct_out = source
        return
    @rule
    def literal():
        nonlocal literal_out
        literal_out = 7
        return
    @rule
    def shared():
        nonlocal shared_left, shared_right
        cached = source
        shared_left = cached
        shared_right = cached
        return
    @rule
    def conditional():
        nonlocal conditional_out
        if ready:
            conditional_out = source
            return
        return
    @rule
    def inverse():
        nonlocal inverse_out
        if ready:
            return
        else:
            inverse_out = source
            return
    @rule
    def observe_only():
        print("source", source)
        return
    direct()
    literal()
    shared()
    conditional()
    inverse()
    observe_only()
)py");
    const std::string capture = temporary.child("uses.transport.mlir");
    const std::string bodyPath = temporary.child("uses.body.mlir");
    const std::string headerPath = temporary.child("uses.interface.mlir");
    static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
root = Path(sys.argv[1])
sys.path[:0] = [str(root / "python/semantic-core/src"),
                str(root / "python/pycircuit/src"), str(root)]
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
source = Path(sys.argv[2])
capture = _capture_source_file(source, source_root=Path(sys.argv[3]))
Path(sys.argv[4]).write_text(_emit_source_transport(capture), encoding="utf-8")
)py";
    ASSERT_EQ(run(ACIR_TEST_PYTHON,
                  {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
                   source, sourceRoot, capture},
                  temporary.child("capture.log")),
              0);
    const std::string compileLog = temporary.child("compile.log");
    ASSERT_EQ(run(ACIR_TEST_SOURCE_UNIT_HARNESS,
                  {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", capture,
                   "--package", "generic_final", "--path", "uses.py",
                   "--body-out", bodyPath, "--interface-out", headerPath},
                  compileLog),
              0)
        << readFile(compileLog);
    body = mlir::parseSourceFile<mlir::ModuleOp>(bodyPath, &context);
    header = mlir::parseSourceFile<mlir::ModuleOp>(headerPath, &context);
    ASSERT_TRUE(body && header);
    body->walk([&](ac::SourceUseOp use) {
      sourceFacts.push_back({print(use.getIdAttr()), print(use.getSourceAttr()),
                             print(use.getTargetAttr())});
    });
    std::sort(
        sourceFacts.begin(), sourceFacts.end(),
        [](const auto &left, const auto &right) { return left.id < right.id; });
    ASSERT_EQ(sourceFacts.size(), 6u);
  }

  auto emitError() {
    return [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
  }

  std::string materializeText() {
    SourceLinkUnit unit{*body, *header};
    auto analysis =
        buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
    EXPECT_TRUE(mlir::succeeded(analysis));
    if (mlir::failed(analysis))
      return {};
    auto ready = materializeFinalProgram(std::move(*analysis), emitError());
    EXPECT_TRUE(mlir::succeeded(ready));
    if (mlir::failed(ready) || !ready->hardware())
      return {};
    std::string text;
    llvm::raw_string_ostream stream(text);
    ready->hardware().print(stream, mlir::OpPrintingFlags().enableDebugInfo());
    return text;
  }

  bool verify(mlir::ModuleOp package) {
    return mlir::succeeded(mlir::verify(package)) &&
           mlir::succeeded(ac::verifyFinalHardware(package));
  }

  ac::RuleOp rule(mlir::ModuleOp package, llvm::StringRef name) {
    ac::RuleOp result;
    package.walk([&](ac::RuleOp candidate) {
      if (candidate.getName() == name)
        result = candidate;
    });
    return result;
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot;
  mlir::OwningOpRef<mlir::ModuleOp> body, header;
  std::vector<SourceUseFact> sourceFacts;
};

TEST_F(FinalGenericUsesTest,
       FreshPackageRetainsOriginalUseLineageAndAllControlForms) {
  std::string text = materializeText();
  ASSERT_FALSE(text.empty());
  mlir::MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect>();
  auto package = mlir::parseSourceString<mlir::ModuleOp>(text, &fresh);
  ASSERT_TRUE(package);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*package)));
  ASSERT_TRUE(mlir::succeeded(ac::verifyFinalHardware(*package)));

  std::vector<SourceUseFact> retained;
  package->walk([&](ac::RuleOp finalRule) {
    if (!finalRule.getTargets().empty()) {
      ASSERT_TRUE(ac::hasGenericFinalUses(finalRule));
      ASSERT_TRUE(mlir::succeeded(ac::verifyGenericFinalUses(finalRule)));
      EXPECT_FALSE(finalRule->hasAttr("ac.required_numeric"));
      auto uses = finalRule->getAttrOfType<mlir::ArrayAttr>("ac.required_uses");
      for (mlir::Attribute raw : uses) {
        auto item = mlir::cast<mlir::DictionaryAttr>(raw);
        retained.push_back({print(item.get("id")), print(item.get("value")),
                            print(item.get("target"))});
      }
      return;
    }
    EXPECT_FALSE(finalRule->hasAttr("ac.required_uses"));
    EXPECT_FALSE(finalRule->hasAttr("ac.yield_bindings"));
  });
  std::sort(
      retained.begin(), retained.end(),
      [](const auto &left, const auto &right) { return left.id < right.id; });
  EXPECT_EQ(retained, sourceFacts);

  ac::RuleOp shared = rule(*package, "shared");
  ASSERT_TRUE(shared);
  EXPECT_EQ(
      llvm::range_size(shared.getBody().front().getOps<ac::ValueBindingOp>()),
      1u);
  EXPECT_EQ(llvm::range_size(shared.getBody().front().getOps<ac::ValueUseOp>()),
            2u);
  auto sharedUses = shared->getAttrOfType<mlir::ArrayAttr>("ac.required_uses");
  EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(sharedUses[0]).get("value"),
            mlir::cast<mlir::DictionaryAttr>(sharedUses[1]).get("value"));
  EXPECT_TRUE(rule(*package, "literal")
                  .getBody()
                  .front()
                  .getOps<mlir::arith::ConstantOp>()
                  .begin() != rule(*package, "literal")
                                  .getBody()
                                  .front()
                                  .getOps<mlir::arith::ConstantOp>()
                                  .end());
  EXPECT_EQ(llvm::range_size(rule(*package, "observe_only")
                                 .getBody()
                                 .front()
                                 .getOps<ac::SourceObserveOp>()),
            1u);
}

TEST_F(FinalGenericUsesTest, RejectsMissingDuplicateRedirectedAndHiddenFacts) {
  std::string text = materializeText();
  ASSERT_FALSE(text.empty());
  mlir::MLIRContext fresh;
  fresh.loadDialect<ac::ACIRDialect, mlir::arith::ArithDialect>();
  auto original = mlir::parseSourceString<mlir::ModuleOp>(text, &fresh);
  ASSERT_TRUE(original);
  ASSERT_TRUE(verify(*original));
  auto expectRejected = [&](llvm::StringRef label, auto mutate) {
    SCOPED_TRACE(label.str());
    auto copy = mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>((*original)->clone()));
    mutate(*copy);
    EXPECT_FALSE(verify(*copy));
  };
  for (llvm::StringRef attribute :
       {"ac.required_uses", "ac.yield_bindings", "ac.proof_scope"})
    expectRejected(attribute, [&](mlir::ModuleOp package) {
      rule(package, "direct")->removeAttr(attribute);
    });
  expectRejected("all stripped", [&](mlir::ModuleOp package) {
    ac::RuleOp direct = rule(package, "direct");
    for (llvm::StringRef name :
         {"ac.required_uses", "ac.yield_bindings", "ac.proof_scope"})
      direct->removeAttr(name);
    llvm::SmallVector<mlir::Operation *> erase;
    for (mlir::Operation &operation : direct.getBody().front())
      if (mlir::isa<ac::ValueBindingOp, ac::ValueUseOp>(operation))
        erase.push_back(&operation);
    for (mlir::Operation *operation : llvm::reverse(erase))
      operation->erase();
  });
  for (llvm::StringRef kind : {"binding", "use"}) {
    std::string label = (llvm::Twine("duplicate ") + kind).str();
    expectRejected(label, [&](mlir::ModuleOp package) {
      ac::RuleOp direct = rule(package, "direct");
      mlir::Operation *operation;
      if (kind == "binding") {
        ac::ValueBindingOp binding =
            *direct.getBody().front().getOps<ac::ValueBindingOp>().begin();
        operation = binding;
      } else {
        ac::ValueUseOp use =
            *direct.getBody().front().getOps<ac::ValueUseOp>().begin();
        operation = use;
      }
      operation->getBlock()->getOperations().insert(operation->getIterator(),
                                                    operation->clone());
    });
  }
  expectRejected("redirect data", [&](mlir::ModuleOp package) {
    ac::RuleOp shared = rule(package, "shared");
    auto yield =
        mlir::cast<ac::YieldOp>(shared.getBody().front().getTerminator());
    mlir::OpBuilder builder(yield);
    auto wrong = mlir::arith::ConstantOp::create(
        builder, yield.getLoc(), builder.getI8Type(),
        builder.getIntegerAttr(builder.getI8Type(), 9));
    yield->setOperand(0, wrong);
  });
  expectRejected("redirect enable", [&](mlir::ModuleOp package) {
    ac::RuleOp shared = rule(package, "shared");
    auto yield =
        mlir::cast<ac::YieldOp>(shared.getBody().front().getTerminator());
    yield->setOperand(1, yield->getOperand(3));
  });
  expectRejected("redirect target", [&](mlir::ModuleOp package) {
    ac::RuleOp shared = rule(package, "shared");
    auto uses = shared->getAttrOfType<mlir::ArrayAttr>("ac.required_uses");
    auto first = mlir::cast<mlir::DictionaryAttr>(uses[0]);
    auto second = mlir::cast<mlir::DictionaryAttr>(uses[1]);
    mlir::Builder builder(&fresh);
    shared->setAttr("ac.required_uses",
                    builder.getArrayAttr({replaceField(builder, first, "target",
                                                       second.get("target")),
                                          second}));
  });
  expectRejected("hidden arithmetic", [&](mlir::ModuleOp package) {
    ac::RuleOp direct = rule(package, "direct");
    mlir::OpBuilder builder(direct.getBody().front().getTerminator());
    mlir::Value input = direct.getBody().front().getArgument(0);
    (void)mlir::arith::AddIOp::create(builder, direct.getLoc(), input, input);
  });
  expectRejected("narrowing", [&](mlir::ModuleOp package) {
    ac::RuleOp direct = rule(package, "direct");
    auto binding =
        *direct.getBody().front().getOps<ac::ValueBindingOp>().begin();
    mlir::OpBuilder builder(binding);
    auto narrow = mlir::arith::TruncIOp::create(
        builder, binding.getLoc(), builder.getI4Type(), binding.getValue());
    binding.getValueMutable().assign(narrow);
  });
  expectRejected("wrong domain", [&](mlir::ModuleOp package) {
    ac::RuleOp direct = rule(package, "direct");
    auto binding =
        *direct.getBody().front().getOps<ac::ValueBindingOp>().begin();
    mlir::DictionaryAttr boolType;
    for (mlir::Attribute raw :
         rule(package, "conditional")
             ->getAttrOfType<mlir::ArrayAttr>("ac.input_types")) {
      auto logical = mlir::cast<mlir::DictionaryAttr>(raw);
      auto kind = logical.getAs<mlir::StringAttr>("kind");
      if (kind && kind.getValue() == "bool")
        boolType = logical;
    }
    ASSERT_TRUE(boolType);
    binding->setAttr("domain", boolType);
  });
  expectRejected("malformed scope", [&](mlir::ModuleOp package) {
    ac::RuleOp direct = rule(package, "direct");
    auto scope = direct->getAttrOfType<mlir::DictionaryAttr>("ac.proof_scope");
    mlir::Builder builder(&fresh);
    direct->setAttr("ac.proof_scope",
                    replaceField(builder, scope, "registration",
                                 builder.getDictionaryAttr({})));
  });
  expectRejected("forged provenance", [&](mlir::ModuleOp package) {
    ac::RuleOp direct = rule(package, "direct");
    auto binding =
        *direct.getBody().front().getOps<ac::ValueBindingOp>().begin();
    auto id = binding.getIdAttr();
    mlir::Builder builder(&fresh);
    binding->setAttr(
        "id", replaceField(builder, id, "slot", builder.getI32IntegerAttr(9)));
  });
  expectRejected("coherent foreign UseID", [&](mlir::ModuleOp package) {
    ac::RuleOp direct = rule(package, "direct");
    auto use = *direct.getBody().front().getOps<ac::ValueUseOp>().begin();
    auto id = use.getIdAttr();
    auto origin = id.getAs<mlir::DictionaryAttr>("origin");
    auto site = origin.getAs<mlir::DictionaryAttr>("site");
    mlir::Builder builder(&fresh);
    site = replaceField(builder, site, "definition",
                        mlir::FlatSymbolRefAttr::get(&fresh, "foreign.Unit"));
    origin = replaceField(builder, origin, "site", site);
    rewriteUseID(direct, replaceField(builder, id, "origin", origin), builder);
  });
  expectRejected("wrong ValueUse filename", [&](mlir::ModuleOp package) {
    ac::RuleOp direct = rule(package, "direct");
    auto use = *direct.getBody().front().getOps<ac::ValueUseOp>().begin();
    use->setLoc(mlir::FileLineColLoc::get(&fresh, "wrong.py", 1, 1));
  });
  expectRejected("nonempty UseID expansion", [&](mlir::ModuleOp package) {
    ac::RuleOp direct = rule(package, "direct");
    auto use = *direct.getBody().front().getOps<ac::ValueUseOp>().begin();
    auto id = use.getIdAttr();
    auto origin = id.getAs<mlir::DictionaryAttr>("origin");
    auto site = origin.getAs<mlir::DictionaryAttr>("site");
    mlir::Builder builder(&fresh);
    auto frame = builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("call")),
        builder.getNamedAttr("site", site),
        builder.getNamedAttr("callee",
                             site.getAs<mlir::FlatSymbolRefAttr>("definition")),
    });
    origin = replaceField(builder, origin, "expansion",
                          builder.getArrayAttr({frame}));
    rewriteUseID(direct, replaceField(builder, id, "origin", origin), builder);
  });
  expectRejected("unknown retained constant attribute", [&](mlir::ModuleOp
                                                                package) {
    ac::RuleOp literal = rule(package, "literal");
    auto constant =
        *literal.getBody().front().getOps<mlir::arith::ConstantOp>().begin();
    constant->setAttr("forged", mlir::UnitAttr::get(&fresh));
  });
  expectRejected("unknown yield AND attribute", [&](mlir::ModuleOp package) {
    ac::RuleOp direct = rule(package, "direct");
    auto yield =
        mlir::cast<ac::YieldOp>(direct.getBody().front().getTerminator());
    auto enable = yield.getValues()[1].getDefiningOp<mlir::arith::AndIOp>();
    ASSERT_TRUE(enable);
    enable->setAttr("forged", mlir::UnitAttr::get(&fresh));
  });
}

} // namespace
} // namespace acir::compiler
