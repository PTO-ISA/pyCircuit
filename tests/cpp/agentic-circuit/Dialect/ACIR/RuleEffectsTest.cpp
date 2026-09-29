#include "Compiler/RuleEffectView.h"
#include "Compiler/SourceUnit.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-rule-effects", path));
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

int run(llvm::StringRef program, const std::vector<std::string> &owned,
        llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

int emitCapture(llvm::StringRef source, llvm::StringRef root,
                llvm::StringRef output, llvm::StringRef log) {
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
  return run(ACIR_TEST_PYTHON,
             {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT,
              source.str(), root.str(), output.str()},
             log);
}

int compileCapture(llvm::StringRef capture, llvm::StringRef body,
                   llvm::StringRef interface, llvm::StringRef log) {
  return run(ACIR_TEST_SOURCE_UNIT_HARNESS,
             {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", capture.str(),
              "--package", "verify", "--path", "effects.py", "--body-out",
              body.str(), "--interface-out", interface.str()},
             log);
}

std::string printed(mlir::Attribute value) {
  std::string result;
  llvm::raw_string_ostream(result) << value;
  return result;
}

struct Effect {
  bool read = false;
  bool write = false;
  std::vector<std::string> origins;
};

using Effects = std::map<std::string, Effect>;

void finish(Effect &effect) {
  llvm::sort(effect.origins);
  effect.origins.erase(
      std::unique(effect.origins.begin(), effect.origins.end()),
      effect.origins.end());
}

Effects recomputeBodyEffects(mlir::ModuleOp body) {
  Effects effects;
  body.walk([&](ac::RuleOp rule) {
    auto bindings = rule->getAttrOfType<mlir::ArrayAttr>("ac.input_bindings");
    mlir::Block &block = rule.getBody().front();
    rule.walk([&](ac::SourceReadOp read) {
      auto argument = mlir::dyn_cast<mlir::BlockArgument>(read.getCurrent());
      EXPECT_TRUE(argument && argument.getOwner() == &block);
      if (!argument || argument.getOwner() != &block ||
          argument.getArgNumber() >= bindings.size())
        return;
      auto binding =
          mlir::cast<mlir::DictionaryAttr>(bindings[argument.getArgNumber()]);
      auto parameter = binding.getAs<mlir::StringAttr>("parameter");
      auto origin = read->getAttrOfType<mlir::DictionaryAttr>("ac.origin");
      ASSERT_TRUE(parameter && origin);
      Effect &effect = effects[parameter.getValue().str()];
      effect.read = true;
      effect.origins.push_back(printed(origin));
    });
    rule.walk([&](ac::SourceUseOp use) {
      auto target = use->getAttrOfType<mlir::DictionaryAttr>("target");
      auto state = target ? target.getAs<mlir::DictionaryAttr>("state")
                          : mlir::DictionaryAttr();
      auto parameter = state ? state.getAs<mlir::StringAttr>("parameter")
                             : mlir::StringAttr();
      auto id = use->getAttrOfType<mlir::DictionaryAttr>("id");
      auto origin = id ? id.getAs<mlir::DictionaryAttr>("origin")
                       : mlir::DictionaryAttr();
      ASSERT_TRUE(target && target.getAs<mlir::StringAttr>("kind") &&
                  target.getAs<mlir::StringAttr>("kind").getValue() ==
                      "next_scalar" &&
                  parameter && origin);
      if (!parameter || !origin)
        return;
      Effect &effect = effects[parameter.getValue().str()];
      effect.write = true;
      effect.origins.push_back(printed(origin));
    });
  });
  for (auto &[_, effect] : effects)
    finish(effect);
  return effects;
}

Effects headerEffects(mlir::ModuleOp header) {
  Effects effects;
  auto declaration = *header.getOps<ac::ModuleImportOp>().begin();
  auto contract =
      declaration->getAttrOfType<mlir::DictionaryAttr>("ac.contract");
  for (mlir::Attribute rawConnection :
       contract.getAs<mlir::ArrayAttr>("connections")) {
    auto connection = mlir::cast<mlir::DictionaryAttr>(rawConnection);
    std::string parameter =
        connection.getAs<mlir::StringAttr>("parameter").getValue().str();
    auto elements = connection.getAs<mlir::ArrayAttr>("elements");
    EXPECT_EQ(elements.size(), 1u);
    if (elements.size() != 1)
      continue;
    auto element = mlir::cast<mlir::DictionaryAttr>(elements[0]);
    Effect &effect = effects[parameter];
    effect.read = element.getAs<mlir::BoolAttr>("read").getValue();
    effect.write = element.getAs<mlir::BoolAttr>("write").getValue();
    for (mlir::Attribute origin : element.getAs<mlir::ArrayAttr>("origins"))
      effect.origins.push_back(printed(origin));
  }
  return effects;
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

mlir::DictionaryAttr indexedOccurrence(mlir::Builder &builder,
                                       mlir::FlatSymbolRefAttr definition,
                                       uint64_t index) {
  auto component = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("index")),
      builder.getNamedAttr("value", builder.getI64IntegerAttr(index)),
  });
  auto site = builder.getDictionaryAttr({
      builder.getNamedAttr("definition", definition),
      builder.getNamedAttr("ast_path", builder.getArrayAttr({component})),
  });
  return builder.getDictionaryAttr({
      builder.getNamedAttr("site", site),
      builder.getNamedAttr("expansion", builder.getArrayAttr({})),
  });
}

bool inferRule(mlir::MLIRContext &context, ac::RuleOp rule,
               detail::RuleEffectView *view = nullptr) {
  std::string diagnostic;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &value) {
    llvm::raw_string_ostream(diagnostic) << value;
    return mlir::success();
  });
  auto emit = [&]() -> mlir::InFlightDiagnostic {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  auto inferred = detail::inferSourceRuleEffects(rule, emit);
  if (mlir::succeeded(inferred) && view)
    *view = std::move(*inferred);
  return mlir::succeeded(inferred);
}

class RuleEffectsTest : public ::testing::Test {
protected:
  RuleEffectsTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    root = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(root));
    std::string source = root + "/effects.py";
    writeFile(source, R"py(from pycircuit import module, rule

@module
def Effects(source: bool, sink: bool, ready: bool):
    @rule
    def inactive():
        nonlocal sink
        sink = source
        return

    @rule
    def active():
        nonlocal sink
        first = source
        cached = first
        second = source
        if ready:
            sink = second
            return
        return

    active()
)py");
    std::string capture = temporary.child("effects.transport.mlir");
    std::string bodyPath = temporary.child("effects.body.mlir");
    std::string headerPath = temporary.child("effects.interface.mlir");
    ASSERT_EQ(emitCapture(source, root, capture, temporary.child("python.log")),
              0);
    ASSERT_EQ(compileCapture(capture, bodyPath, headerPath,
                             temporary.child("compile.log")),
              0);
    body = mlir::parseSourceFile<mlir::ModuleOp>(bodyPath, &context);
    header = mlir::parseSourceFile<mlir::ModuleOp>(headerPath, &context);
    ASSERT_TRUE(body && header);
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*body)));
    ASSERT_TRUE(mlir::succeeded(mlir::verify(*header)));
  }

  mlir::OwningOpRef<mlir::ModuleOp> clone(mlir::ModuleOp module) {
    return mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(module->clone()));
  }

  bool snapshotsAccept(mlir::ModuleOp candidateBody,
                       mlir::ModuleOp candidateHeader) {
    std::string diagnostic;
    mlir::ScopedDiagnosticHandler capture(
        &context, [&](mlir::Diagnostic &value) {
          llvm::raw_string_ostream(diagnostic) << value;
          return mlir::success();
        });
    auto emit = [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
    auto registry = SourceHeaderRegistry::create({}, emit);
    EXPECT_TRUE(mlir::succeeded(registry)) << diagnostic;
    return mlir::succeeded(registry) &&
           mlir::succeeded(registry->verifyBodySnapshots(
               candidateBody, candidateHeader, emit));
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string root;
  mlir::OwningOpRef<mlir::ModuleOp> body, header;
};

TEST_F(RuleEffectsTest, HeaderMatchesEffectsRecomputedFromActiveSourceFacts) {
  Effects expected = recomputeBodyEffects(*body);
  Effects actual = headerEffects(*header);
  ASSERT_EQ(expected.size(), 3u);
  EXPECT_EQ(expected.size(), actual.size());
  for (const auto &[parameter, effect] : expected) {
    SCOPED_TRACE(parameter);
    auto found = actual.find(parameter);
    ASSERT_NE(found, actual.end());
    EXPECT_EQ(effect.read, found->second.read);
    EXPECT_EQ(effect.write, found->second.write);
    EXPECT_EQ(effect.origins, found->second.origins);
  }
  EXPECT_EQ(expected["source"].origins.size(), 2u);
  EXPECT_EQ(expected["ready"].origins.size(), 1u);
  EXPECT_EQ(expected["sink"].origins.size(), 1u);
  EXPECT_TRUE(snapshotsAccept(*body, *header));
}

TEST_F(RuleEffectsTest, DeletedReadMarkerInvalidatesOwningHeader) {
  auto candidate = clone(*body);
  ac::SourceReadOp unused;
  candidate->walk([&](ac::SourceReadOp read) {
    if (read.getResult().use_empty() && !unused)
      unused = read;
  });
  ASSERT_TRUE(unused);
  unused.erase();
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*candidate)));
  EXPECT_FALSE(snapshotsAccept(*candidate, *header));
}

TEST_F(RuleEffectsTest, ForgedReadOriginInvalidatesOwningHeader) {
  auto candidate = clone(*body);
  llvm::SmallVector<ac::SourceReadOp> reads;
  candidate->walk([&](ac::SourceReadOp read) { reads.push_back(read); });
  ASSERT_GE(reads.size(), 2u);
  reads[0]->setAttr("ac.origin", reads[1]->getAttr("ac.origin"));
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*candidate)));
  EXPECT_FALSE(snapshotsAccept(*candidate, *header));
}

TEST_F(RuleEffectsTest, HeaderCannotDropOneRecomputedOrigin) {
  auto candidate = clone(*header);
  auto declaration = *candidate->getOps<ac::ModuleImportOp>().begin();
  mlir::Builder builder(&context);
  auto contract =
      declaration->getAttrOfType<mlir::DictionaryAttr>("ac.contract");
  auto connections = contract.getAs<mlir::ArrayAttr>("connections");
  llvm::SmallVector<mlir::Attribute> changed(connections.begin(),
                                             connections.end());
  for (mlir::Attribute &raw : changed) {
    auto connection = mlir::cast<mlir::DictionaryAttr>(raw);
    if (connection.getAs<mlir::StringAttr>("parameter").getValue() != "source")
      continue;
    auto elements = connection.getAs<mlir::ArrayAttr>("elements");
    auto effect = mlir::cast<mlir::DictionaryAttr>(elements[0]);
    auto origins = effect.getAs<mlir::ArrayAttr>("origins");
    ASSERT_EQ(origins.size(), 2u);
    effect = replaceField(builder, effect, "origins",
                          builder.getArrayAttr({origins[0]}));
    raw = replaceField(builder, connection, "elements",
                       builder.getArrayAttr({effect}));
  }
  declaration->setAttr("ac.contract",
                       replaceField(builder, contract, "connections",
                                    builder.getArrayAttr(changed)));
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*candidate)));
  EXPECT_FALSE(snapshotsAccept(*body, *candidate));
}

TEST_F(RuleEffectsTest, OneRuleRejectsDuplicateSourceReadOccurrence) {
  auto candidate = clone(*body);
  auto module = *candidate->getOps<ac::ModuleOp>().begin();
  auto rule = *module.getBody().front().getOps<ac::RuleOp>().begin();
  auto reads =
      llvm::to_vector(rule.getBody().front().getOps<ac::SourceReadOp>());
  ASSERT_FALSE(reads.empty());
  mlir::OpBuilder builder(reads.front());
  mlir::OperationState duplicate(reads.front().getLoc(),
                                 ac::SourceReadOp::getOperationName());
  duplicate.addOperands(reads.front().getCurrent());
  duplicate.addTypes(reads.front().getResult().getType());
  duplicate.addAttribute("ac.origin", reads.front()->getAttr("ac.origin"));
  builder.create(duplicate);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*candidate)));
  EXPECT_FALSE(inferRule(context, rule));
}

TEST_F(RuleEffectsTest, ScalarStateAndNumericAstPathUseStructuralOrdering) {
  auto candidate = clone(*body);
  auto module = *candidate->getOps<ac::ModuleOp>().begin();
  auto rule = *module.getBody().front().getOps<ac::RuleOp>().begin();
  auto bindings = rule->getAttrOfType<mlir::ArrayAttr>("ac.input_bindings");
  std::optional<unsigned> sourceIndex;
  for (auto [index, raw] : llvm::enumerate(bindings)) {
    auto binding = mlir::cast<mlir::DictionaryAttr>(raw);
    auto parameter = binding.getAs<mlir::StringAttr>("parameter");
    if (parameter && parameter.getValue() == "source")
      sourceIndex = static_cast<unsigned>(index);
  }
  ASSERT_TRUE(sourceIndex.has_value());
  llvm::SmallVector<ac::SourceReadOp> sourceReads;
  rule.walk([&](ac::SourceReadOp read) {
    auto argument = mlir::dyn_cast<mlir::BlockArgument>(read.getCurrent());
    if (argument && argument.getArgNumber() == *sourceIndex)
      sourceReads.push_back(read);
  });
  ASSERT_EQ(sourceReads.size(), 2u);
  mlir::Builder builder(&context);
  auto definition =
      mlir::FlatSymbolRefAttr::get(&context, "verify.effects.Effects");
  sourceReads[0]->setAttr("ac.origin",
                          indexedOccurrence(builder, definition, 10));
  sourceReads[1]->setAttr("ac.origin",
                          indexedOccurrence(builder, definition, 2));

  detail::RuleEffectView view;
  ASSERT_TRUE(inferRule(context, rule, &view));
  auto sourceEffect =
      llvm::find_if(view.effects, [](const detail::RuleEffect &effect) {
        auto parameter = effect.state.getAs<mlir::StringAttr>("parameter");
        return parameter && parameter.getValue() == "source";
      });
  ASSERT_NE(sourceEffect, view.effects.end());
  EXPECT_TRUE(mlir::isa<mlir::UnitAttr>(sourceEffect->state.get("ordinal")));
  ASSERT_EQ(sourceEffect->origins.size(), 2u);
  auto firstPath = sourceEffect->origins[0]
                       .getAs<mlir::DictionaryAttr>("site")
                       .getAs<mlir::ArrayAttr>("ast_path");
  auto secondPath = sourceEffect->origins[1]
                        .getAs<mlir::DictionaryAttr>("site")
                        .getAs<mlir::ArrayAttr>("ast_path");
  EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(firstPath[0])
                .getAs<mlir::IntegerAttr>("value")
                .getInt(),
            2);
  EXPECT_EQ(mlir::cast<mlir::DictionaryAttr>(secondPath[0])
                .getAs<mlir::IntegerAttr>("value")
                .getInt(),
            10);
}

TEST_F(RuleEffectsTest, UnsupportedOriginAttributeFailsBeforeEffectSorting) {
  auto candidate = clone(*body);
  auto module = *candidate->getOps<ac::ModuleOp>().begin();
  auto rule = *module.getBody().front().getOps<ac::RuleOp>().begin();
  auto read = *rule.getBody().front().getOps<ac::SourceReadOp>().begin();
  mlir::Builder builder(&context);
  auto origin = read->getAttrOfType<mlir::DictionaryAttr>("ac.origin");
  read->setAttr("ac.origin",
                replaceField(builder, origin, "expansion",
                             builder.getArrayAttr({builder.getUnitAttr()})));
  EXPECT_FALSE(inferRule(context, rule));
}

} // namespace
} // namespace acir::compiler
