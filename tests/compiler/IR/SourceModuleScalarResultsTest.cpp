// Production source capture and owning interfaces authenticate scalar results.
#include "Compiler/FinalCppSourceParts.h"
#include "Compiler/FinalVerilogSourceParts.h"
#include "Compiler/LiteralPayloadWires.h"
#include "Compiler/SourceLink.h"
#include "Compiler/SourceUnit.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {
namespace ac = acir::ac;
namespace compiler = acir::compiler;
using namespace mlir;
class SourceModuleScalarResultsTest : public ::testing::Test {
protected:
  MLIRContext context;
  Builder b{&context};
  llvm::SmallString<256> directory;
  std::string diagnostics;
  std::unique_ptr<ScopedDiagnosticHandler> handler;
  void SetUp() override {
    context.loadDialect<ac::ACIRDialect>();
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("module-scalar-results",
                                                      directory));
    handler =
        std::make_unique<ScopedDiagnosticHandler>(&context, [&](Diagnostic &d) {
          llvm::raw_string_ostream out(diagnostics);
          d.print(out);
          diagnostics += '\n';
          return success();
        });
  }
  void TearDown() override { llvm::sys::fs::remove_directories(directory); }
  std::string path(StringRef name) {
    llvm::SmallString<256> p(directory);
    llvm::sys::path::append(p, name);
    return p.str().str();
  }
  auto error() {
    return [&] { return emitError(b.getUnknownLoc()); };
  }
  FailureOr<compiler::SourceUnitArtifacts>
  compile(StringRef source, StringRef name = "scalars.py",
          ArrayRef<ModuleOp> interfaces = {}) {
    std::error_code ec;
    {
      llvm::raw_fd_ostream out(path(name), ec);
      if (ec)
        return failure();
      out << source;
    }
    static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
repo, source, root, output = map(Path, sys.argv[1:])
sys.path.insert(0, str(repo / "python"))
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
output.write_text(_emit_source_transport(_capture_source_file(source, source_root=root)), encoding="utf-8")
)py";
    std::vector<std::string> owned{
        PYCIRCUIT_TEST_PYTHON,    "-c",       script.str(),
        PYCIRCUIT_TEST_REPO_ROOT, path(name), directory.str().str(),
        path("capture.mlir")};
    SmallVector<StringRef> args;
    for (auto &arg : owned)
      args.push_back(arg);
    auto log = path("capture.log");
    std::array<std::optional<StringRef>, 3> redirects{std::nullopt, log, log};
    int status =
        llvm::sys::ExecuteAndWait(owned[0], args, std::nullopt, redirects);
    auto output = llvm::MemoryBuffer::getFile(log);
    EXPECT_EQ(status, 0) << (output ? output.get()->getBuffer().str() : log);
    if (status)
      return failure();
    auto captured = parseSourceFile<ModuleOp>(path("capture.mlir"), &context);
    if (!captured)
      return failure();
    auto registry =
        compiler::SourceHeaderRegistry::create(&context, interfaces, error());
    if (failed(registry))
      return failure();
    auto owner =
        b.getDictionaryAttr({b.getNamedAttr("package", b.getStringAttr("")),
                             b.getNamedAttr("path", b.getStringAttr(name))});
    return compiler::compilePythonSourceUnit(*captured, owner, *registry,
                                             error());
  }
  ac::ModuleImportOp declaration(ModuleOp file, StringRef symbol) {
    return dyn_cast_or_null<ac::ModuleImportOp>(
        SymbolTable::lookupSymbolIn(file, symbol));
  }
  ac::ModuleOp definition(ModuleOp file, StringRef symbol) {
    return dyn_cast_or_null<ac::ModuleOp>(
        SymbolTable::lookupSymbolIn(file, symbol));
  }
  void dualEmit(ModuleOp file) {
    ac::HardwareAnalysis analysis(file);
    ASSERT_TRUE(succeeded(analysis.verify())) << diagnostics;
    auto cpp = compiler::emitCppSourceParts(file, analysis);
    ASSERT_TRUE(succeeded(cpp)) << diagnostics;
    EXPECT_FALSE(cpp->sourceGroups.empty());
    EXPECT_FALSE(cpp->systemHeader.empty());
    auto rtl = compiler::emitVerilogSourceParts(file, analysis);
    ASSERT_TRUE(succeeded(rtl)) << diagnostics;
    EXPECT_FALSE(rtl->sourceGroups.empty());
    EXPECT_FALSE(rtl->core.empty());
  }
  std::string text(Operation *operation) {
    std::string result;
    llvm::raw_string_ostream output(result);
    operation->print(output, OpPrintingFlags().enableDebugInfo());
    return result;
  }
  ac::StaticExprAttr withInteger(ac::StaticExprAttr expression, uint64_t value,
                                 std::optional<int64_t> line = std::nullopt) {
    NamedAttrList tree(expression.getTree());
    NamedAttrList literal(cast<DictionaryAttr>(tree.get("value")));
    literal.set("value", ac::MathIntAttr::get(
                             &context, llvm::APSInt(std::to_string(value))));
    tree.set("value", literal.getDictionary(&context));
    if (line) {
      NamedAttrList location(cast<DictionaryAttr>(tree.get("location")));
      location.set("line", b.getI64IntegerAttr(*line));
      location.set("end_line", b.getI64IntegerAttr(*line));
      tree.set("location", location.getDictionary(&context));
    }
    return ac::StaticExprAttr::get(&context, tree.getDictionary(&context));
  }
  ac::StaticExprAttr nonliteralZero(ac::StaticExprAttr zero) {
    NamedAttrList tree(zero.getTree());
    tree.set("kind", b.getStringAttr("binary"));
    tree.erase("value");
    tree.set("operator", b.getStringAttr("add"));
    tree.set("lhs", zero);
    tree.set("rhs", zero);
    return ac::StaticExprAttr::get(&context, tree.getDictionary(&context));
  }
  static bool isLiteralConstructorWire(ac::BitsExtractOp extract) {
    if (!isa<ac::ModuleOp>(extract->getParentOp()) ||
        !extract.getInput().getDefiningOp<ac::BitsConstantOp>() ||
        extract.getInput().getType() != extract.getResult().getType() ||
        !extract->getDiscardableAttrDictionary().empty() ||
        extract.getResult().use_empty())
      return false;
    auto tree = extract.getLow().getTree();
    auto kind = tree.getAs<StringAttr>("kind");
    auto value = tree.getAs<DictionaryAttr>("value");
    auto valueKind = value ? value.getAs<StringAttr>("kind") : StringAttr();
    auto integer =
        value ? value.getAs<ac::MathIntAttr>("value") : ac::MathIntAttr();
    if (!kind || kind.getValue() != "literal" || !valueKind ||
        valueKind.getValue() != "integer" || !integer ||
        integer.getCanonicalValue() != "0")
      return false;
    return llvm::all_of(extract->getUsers(), [&](Operation *user) {
      return user->getBlock() == extract->getBlock() &&
             isa<ac::StructCreateOp, ac::TableCreateOp>(user);
    });
  }
  SmallVector<ac::BitsExtractOp> literalConstructorWires(ac::ModuleOp module) {
    SmallVector<ac::BitsExtractOp> result;
    for (auto extract : module.getBody().front().getOps<ac::BitsExtractOp>())
      if (isLiteralConstructorWire(extract))
        result.push_back(extract);
    return result;
  }
  static bool isLiteralZero(ac::BitsConstantOp constant) {
    auto tree = constant.getValue().getTree();
    auto kind = tree.getAs<StringAttr>("kind");
    auto value = tree.getAs<DictionaryAttr>("value");
    auto valueKind = value ? value.getAs<StringAttr>("kind") : StringAttr();
    auto integer =
        value ? value.getAs<ac::MathIntAttr>("value") : ac::MathIntAttr();
    return kind && kind.getValue() == "literal" && valueKind &&
           valueKind.getValue() == "integer" && integer &&
           integer.getCanonicalValue() == "0";
  }
  SmallVector<ac::BitsConstantOp>
  literalConstructorConstants(ac::ModuleOp module) {
    SmallVector<ac::BitsConstantOp> result;
    auto &block = module.getBody().front();
    for (auto constant : block.getOps<ac::BitsConstantOp>()) {
      if (!isLiteralZero(constant) ||
          !constant->getDiscardableAttrDictionary().empty() ||
          constant.getResult().use_empty())
        continue;
      if (llvm::all_of(constant->getUsers(), [&](Operation *user) {
            return user->getBlock() == &block &&
                   isa<ac::StructCreateOp, ac::TableCreateOp>(user);
          }))
        result.push_back(constant);
    }
    return result;
  }
  bool injectLiteralConstructorWire(ac::ModuleOp module) {
    auto existing = literalConstructorWires(module);
    for (auto representative : existing) {
      bool hasStruct = false, hasTable = false;
      SmallVector<OpOperand *> uses;
      for (auto extract : existing) {
        if (extract.getResult().getType() !=
            representative.getResult().getType())
          continue;
        for (OpOperand &use : extract.getResult().getUses()) {
          hasStruct |= isa<ac::StructCreateOp>(use.getOwner());
          hasTable |= isa<ac::TableCreateOp>(use.getOwner());
          uses.push_back(&use);
        }
      }
      if (!hasStruct || !hasTable || uses.size() < 2)
        continue;
      for (OpOperand *use : uses)
        use->set(representative.getResult());
      return true;
    }
    for (auto representative : existing) {
      if (!llvm::any_of(representative->getUsers(), [](Operation *user) {
            return isa<ac::StructCreateOp>(user);
          }))
        continue;
      for (auto table : existing) {
        if (!llvm::any_of(
                table->getUsers(),
                [](Operation *user) { return isa<ac::TableCreateOp>(user); }) ||
            !ac::areEquivalentHardwareTypes(
                representative.getResult().getType(),
                table.getResult().getType()))
          continue;
        auto constant = table.getInput().getDefiningOp<ac::BitsConstantOp>();
        if (!constant)
          continue;
        constant.getResult().setType(representative.getResult().getType());
        table.getResult().setType(representative.getResult().getType());
        table.getResult().replaceAllUsesWith(representative.getResult());
        return true;
      }
    }
    struct Candidate {
      ac::BitsConstantOp constant;
      SmallVector<OpOperand *> uses;
      bool hasStruct = false;
      bool hasTable = false;
    } selected;
    for (auto constant :
         module.getBody().front().getOps<ac::BitsConstantOp>()) {
      if (!isLiteralZero(constant))
        continue;
      Candidate candidate{constant};
      auto collect = [&](Operation *operation, bool table) {
        for (OpOperand &operand : operation->getOpOperands()) {
          auto input = operand.get().getDefiningOp<ac::BitsConstantOp>();
          if (!input || !isLiteralZero(input) ||
              input.getResult().getType() != constant.getResult().getType())
            continue;
          candidate.uses.push_back(&operand);
          candidate.hasStruct |= !table;
          candidate.hasTable |= table;
        }
      };
      for (auto create : module.getBody().front().getOps<ac::StructCreateOp>())
        collect(create, false);
      for (auto create : module.getBody().front().getOps<ac::TableCreateOp>())
        collect(create, true);
      if (candidate.hasStruct && candidate.hasTable &&
          candidate.uses.size() > 1) {
        selected = std::move(candidate);
        break;
      }
    }
    if (!selected.constant)
      return false;
    OpBuilder builder(selected.uses.front()->getOwner());
    OperationState state(selected.constant.getLoc(),
                         ac::BitsExtractOp::getOperationName());
    state.addOperands(selected.constant.getResult());
    state.addTypes(selected.constant.getResult().getType());
    state.addAttribute("low", selected.constant.getValue());
    auto extract = cast<ac::BitsExtractOp>(builder.create(state));
    for (OpOperand *use : selected.uses)
      use->set(extract.getResult());
    return true;
  }
  bool injectDuplicateConstructorConstant(ac::ModuleOp module,
                                          int64_t line = 202) {
    auto constants = literalConstructorConstants(module);
    if (constants.empty())
      return false;
    auto representative = constants.front();
    OpOperand *target = nullptr;
    auto choose = [&](Operation *operation) {
      for (OpOperand &operand : operation->getOpOperands()) {
        auto constant = operand.get().getDefiningOp<ac::BitsConstantOp>();
        if (constant && constant != representative && isLiteralZero(constant) &&
            constant.getResult().getType() !=
                representative.getResult().getType() &&
            ac::areEquivalentHardwareTypes(
                constant.getResult().getType(),
                representative.getResult().getType())) {
          target = &operand;
          return;
        }
      }
    };
    for (auto create : module.getBody().front().getOps<ac::StructCreateOp>()) {
      choose(create);
      if (target)
        break;
    }
    if (!target)
      for (auto create : module.getBody().front().getOps<ac::TableCreateOp>()) {
        choose(create);
        if (target)
          break;
      }
    if (!target)
      return false;
    OpBuilder builder(target->getOwner());
    OperationState state(representative.getLoc(),
                         ac::BitsConstantOp::getOperationName());
    state.addTypes(representative.getResult().getType());
    state.addAttribute("value",
                       withInteger(representative.getValue(), 0, line));
    auto duplicate = cast<ac::BitsConstantOp>(builder.create(state));
    target->set(duplicate.getResult());
    return true;
  }
  void exposeLiteralConstructorConstants(ac::ModuleOp module) {
    for (auto extract : llvm::make_early_inc_range(
             module.getBody().front().getOps<ac::BitsExtractOp>())) {
      if (!isLiteralConstructorWire(extract))
        continue;
      extract.getResult().replaceAllUsesWith(extract.getInput());
      extract.erase();
    }
  }
  bool injectLiteralIdentity(OpOperand &use) {
    if (use.get().getDefiningOp<ac::BitsExtractOp>())
      return true;
    auto constant = use.get().getDefiningOp<ac::BitsConstantOp>();
    if (!constant)
      return false;
    OpBuilder builder(use.getOwner());
    OperationState state(constant.getLoc(),
                         ac::BitsExtractOp::getOperationName());
    state.addOperands(constant.getResult());
    state.addTypes(constant.getResult().getType());
    state.addAttribute("low", withInteger(constant.getValue(), 0));
    auto extract = cast<ac::BitsExtractOp>(builder.create(state));
    use.set(extract.getResult());
    return true;
  }
  static StringRef literalWireSource() {
    return R"py(import pycircuit as ac

@ac.struct
class Bundle:
    head: ac.u8
    tail: ac.u8
    lanes: ac.table[2, ac.u8]

@ac.module
def Top(seed: ac.u8) -> Bundle:
    return Bundle(head=0, tail=0, lanes=(0, 0))

@ac.module
def Direct() -> ac.u8:
    return 0

@ac.module
def Stateful() -> ac.u8:
    value: ac.u8 = 0
    return value

@ac.module
def NestedRule() -> ac.u8:
    @ac.rule
    def literal() -> ac.u8:
        return 0
    return literal
)py";
  }
  static StringRef source() {
    return R"py(import pycircuit as ac
from enum import Enum
from typing import Annotated
@ac.encoding(width=2)
class Tag(Enum):
    ZERO = 0
    TWO = 2
@ac.module
def Bits(value: ac.u3) -> ac.u8:
    return value
@ac.module
def Flag(value: bool) -> bool:
    return value
@ac.module
def Number(value: Annotated[int, range(1 << 3)]) -> Annotated[int, range(1 << 3)]:
    return value
@ac.module
def Tagged(value: Tag) -> Tag:
    return value
@ac.module
def TableValue(value: ac.table[2, ac.u8]) -> ac.table[2, ac.u8]:
    return value
@ac.module
def TableState() -> ac.table[2, ac.u8]:
    entries = ac.table[2, ac.u8](init=3)
    return entries
@ac.module
def TupleOne(value: ac.u3) -> tuple[ac.u8]:
    return (value,)
@ac.module
def Collision(result: ac.u3, result_1: ac.u3) -> ac.u8:
    return result
@ac.module
def Top(value: ac.u3, flag: bool, number: Annotated[int, range(1 << 3)], tag: Tag,
        entries: ac.table[2, ac.u8]) -> tuple[ac.u8, bool, Annotated[int, range(1 << 3)], Tag, ac.table[2, ac.u8], ac.table[2, ac.u8], ac.u8, ac.u8]:
    word = Bits(value)
    truth = Flag(flag)
    integer = Number(number)
    nominal = Tagged(tag)
    table = TableValue(entries)
    state = TableState()
    singleton, = TupleOne(value)
    renamed = Collision(value, value)
    return word, truth, integer, nominal, table, state, singleton, renamed
)py";
  }
};

TEST_F(SourceModuleScalarResultsTest,
       ExistingKindsPublishExactSingleConstraintsAndDualEmit) {
  auto unit = compile(source());
  ASSERT_TRUE(succeeded(unit)) << diagnostics;
  for (auto [name, kind] : {std::pair{"Bits", "fixed_bits"},
                            {"Flag", "boolean"},
                            {"Number", "integer"},
                            {"Tagged", "nominal"},
                            {"TableValue", "table"},
                            {"TableState", "table"}}) {
    auto header = declaration(*unit->interface, std::string("scalars.") + name);
    ASSERT_TRUE(header);
    EXPECT_EQ(header->getAttrOfType<StringAttr>("ac.return_form").getValue(),
              "single");
    auto results = header->getAttrOfType<ArrayAttr>("ac.result_constraints");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(cast<DictionaryAttr>(results[0])
                  .getAs<StringAttr>("source_kind")
                  .getValue(),
              kind);
    EXPECT_EQ(
        cast<DictionaryAttr>(results[0]).getAs<TypeAttr>("type").getValue(),
        header.getFunctionType().getResult(0));
  }
  auto number = declaration(*unit->interface, "scalars.Number");
  auto constraint = cast<DictionaryAttr>(
      number->getAttrOfType<ArrayAttr>("ac.result_constraints")[0]);
  auto domain = constraint.getAs<ac::SourceDomainAttr>("domain").getValue();
  EXPECT_EQ(domain.getAs<ac::MathIntAttr>("lower").getCanonicalValue(), "0");
  EXPECT_EQ(domain.getAs<ac::MathIntAttr>("upper").getCanonicalValue(), "8");
  auto collision = declaration(*unit->interface, "scalars.Collision");
  ASSERT_TRUE(collision);
  EXPECT_EQ(cast<StringAttr>(collision.getOutputNames()[0]).getValue(),
            "result_2");
  auto singleton = declaration(*unit->interface, "scalars.TupleOne");
  ASSERT_TRUE(singleton);
  EXPECT_EQ(singleton->getAttrOfType<StringAttr>("ac.return_form").getValue(),
            "tuple");
  auto final = compiler::linkSourceUnits({{*unit->body, *unit->interface}},
                                           "scalars.Top", error());
  ASSERT_TRUE(succeeded(final)) << diagnostics;
  dualEmit(**final);
}

TEST_F(SourceModuleScalarResultsTest,
       ScalarFixedBitsUseTheSameWideningBoundaryAsSingletonTuples) {
  auto unit = compile(source());
  ASSERT_TRUE(succeeded(unit)) << diagnostics;
  for (StringRef name :
       {"scalars.Bits", "scalars.TupleOne", "scalars.Collision"}) {
    auto module = definition(*unit->body, name);
    ASSERT_TRUE(module);
    bool widened = false;
    module.walk([&](ac::BitsResizeOp resize) {
      if (resize.getMode() == "zext")
        widened = true;
    });
    EXPECT_TRUE(widened) << name.str();
  }
}

TEST_F(SourceModuleScalarResultsTest,
       LogicalKindNarrowingAndLiteralRangeDiagnosticsRemainEnforced) {
  struct Case {
    StringRef input, output, value, diagnostic;
  };
  for (const auto &test :
       {Case{"bool", "Annotated[int, range(1 << 1)]", "value", "kind"},
        Case{"Annotated[int, range(1 << 1)]", "bool", "value", "kind"},
        Case{"ac.u8", "ac.u3", "value", "narrow"},
        Case{"ac.u3", "ac.u3", "8", "bounds"}}) {
    SCOPED_TRACE(test.input.str() + " -> " + test.output.str() + " return " +
                 test.value.str());
    diagnostics.clear();
    std::string text = "import pycircuit as ac\nfrom typing import "
                       "Annotated\n@ac.module\ndef Top(value: " +
                       test.input.str() + ") -> " + test.output.str() +
                       ":\n    return " + test.value.str() + "\n";
    auto unit = compile(text);
    EXPECT_TRUE(failed(unit));
    EXPECT_NE(diagnostics.find(test.diagnostic.str()), std::string::npos)
        << diagnostics;
  }
}

TEST_F(SourceModuleScalarResultsTest,
       ImportedScalarContractNeedsOnlyOwningHeaderAndPreservesKinds) {
  auto provider = compile(source(), "provider.py");
  ASSERT_TRUE(succeeded(provider)) << diagnostics;
  ASSERT_FALSE(llvm::sys::fs::remove(path("provider.py")));
  auto consumer = compile(R"py(import pycircuit as ac
from provider import Bits, Flag
@ac.module
def Top(value: ac.u3, flag: bool) -> tuple[ac.u8, bool]:
    word = Bits(value)
    truth = Flag(flag)
    return word, truth
)py",
                          "consumer.py", {*provider->interface});
  ASSERT_TRUE(succeeded(consumer)) << diagnostics;
  auto final =
      compiler::linkSourceUnits({{*provider->body, *provider->interface},
                                   {*consumer->body, *consumer->interface}},
                                  "consumer.Top", error());
  ASSERT_TRUE(succeeded(final)) << diagnostics;
  dualEmit(**final);
}

TEST_F(SourceModuleScalarResultsTest,
       SingleAndSingletonTupleCannotBypassOwningPublication) {
  auto unit = compile(source());
  ASSERT_TRUE(succeeded(unit)) << diagnostics;
  for (StringRef name : {"scalars.Bits", "scalars.TupleOne"}) {
    OwningOpRef<ModuleOp> body(cast<ModuleOp>(unit->body->clone()));
    auto module = definition(*body, name);
    ASSERT_TRUE(module);
    auto old = module->getAttrOfType<StringAttr>("ac.return_form").getValue();
    module->setAttr("ac.return_form",
                    b.getStringAttr(old == "single" ? "tuple" : "single"));
    diagnostics.clear();
    ASSERT_TRUE(succeeded(verify(*body))) << diagnostics;
    auto registry =
        compiler::SourceHeaderRegistry::create({*unit->interface}, error());
    ASSERT_TRUE(succeeded(registry)) << diagnostics;
    diagnostics.clear();
    EXPECT_TRUE(failed(
        registry->verifyBodySnapshots(*body, *unit->interface, error())));
    EXPECT_NE(diagnostics.find("published interface"), std::string::npos)
        << diagnostics;
    auto final = compiler::linkSourceUnits({{*body, *unit->interface}},
                                             "scalars.Top", error());
    EXPECT_TRUE(failed(final));
  }
}

TEST_F(SourceModuleScalarResultsTest,
       EqualWidthLogicalResultKindTamperStillFailsPublication) {
  auto unit = compile(source());
  ASSERT_TRUE(succeeded(unit)) << diagnostics;
  OwningOpRef<ModuleOp> header(cast<ModuleOp>(unit->interface->clone()));
  auto flag = declaration(*header, "scalars.Flag");
  ASSERT_TRUE(flag);
  auto result = cast<DictionaryAttr>(
      flag->getAttrOfType<ArrayAttr>("ac.result_constraints")[0]);
  NamedAttrList fields(result);
  fields.set("source_kind", b.getStringAttr("integer"));
  fields.set(
      "domain",
      ac::SourceDomainAttr::get(
          &context,
          b.getDictionaryAttr(
              {b.getNamedAttr("kind", b.getStringAttr("integer")),
               b.getNamedAttr(
                   "lower", ac::MathIntAttr::get(&context, llvm::APSInt("0"))),
               b.getNamedAttr("upper", ac::MathIntAttr::get(
                                           &context, llvm::APSInt("2")))})));
  flag->setAttr("ac.result_constraints",
                b.getArrayAttr({fields.getDictionary(&context)}));
  diagnostics.clear();
  ASSERT_TRUE(succeeded(verify(*header))) << diagnostics;
  auto registry = compiler::SourceHeaderRegistry::create({*header}, error());
  ASSERT_TRUE(succeeded(registry)) << diagnostics;
  diagnostics.clear();
  EXPECT_TRUE(
      failed(registry->verifyBodySnapshots(*unit->body, *header, error())));
  EXPECT_NE(diagnostics.find("published interface"), std::string::npos)
      << diagnostics;
}
TEST_F(SourceModuleScalarResultsTest, RuleStateAndMemoryTypeBindingsSurvivePublication) {
  auto provider = compile(R"py(import pycircuit as ac
@ac.module
def Read(index: ac.u2, data: ac.u8) -> ac.u8:
    state: ac.u8 = 0
    raw = ac.sync_mem[ac.u8](True, index, False, index, data, 1, depth=4)
    @ac.rule
    def update() -> None:
        state = raw
    return state
)py", "memory.py");
  ASSERT_TRUE(succeeded(provider)) << diagnostics;
  auto consumer = compile(R"py(import pycircuit as ac
from memory import Read
@ac.module
def Top(index: ac.u2, data: ac.u8) -> ac.u8:
    return Read(index, data)
)py", "consumer.py", {*provider->interface});
  ASSERT_TRUE(succeeded(consumer)) << diagnostics;
  auto linked =
      compiler::linkSourceUnits({{*provider->body, *provider->interface},
                                   {*consumer->body, *consumer->interface}},
                                  "consumer.Top", error());
  ASSERT_TRUE(succeeded(linked)) << diagnostics;
  dualEmit(**linked);
}

TEST_F(SourceModuleScalarResultsTest,
       LinkCompactsOnlyDirectLiteralConstructorWiresAndPreservesInputs) {
  auto unit = compile(literalWireSource(), "literal_wires.py");
  ASSERT_TRUE(succeeded(unit)) << diagnostics;
  auto top = definition(*unit->body, "literal_wires.Top");
  ASSERT_TRUE(top);
  ASSERT_TRUE(injectLiteralConstructorWire(top));
  auto candidates = literalConstructorWires(top);
  ASSERT_FALSE(candidates.empty());
  bool structUser = false, tableUser = false, fanout = false;
  for (auto extract : candidates) {
    unsigned uses = 0;
    for (Operation *user : extract->getUsers()) {
      structUser |= isa<ac::StructCreateOp>(user);
      tableUser |= isa<ac::TableCreateOp>(user);
      ++uses;
    }
    fanout |= uses > 1;
  }
  ASSERT_TRUE(structUser);
  ASSERT_TRUE(tableUser);
  ASSERT_TRUE(fanout);

  auto sourceDirect = definition(*unit->body, "literal_wires.Direct");
  auto sourceStateful = definition(*unit->body, "literal_wires.Stateful");
  auto sourceNested = definition(*unit->body, "literal_wires.NestedRule");
  ASSERT_TRUE(sourceDirect && sourceStateful && sourceNested);
  auto directYield =
      cast<ac::YieldOp>(sourceDirect.getBody().front().getTerminator());
  ASSERT_TRUE(injectLiteralIdentity(directYield->getOpOperand(0)));
  ac::InstanceOp storage;
  for (auto instance :
       sourceStateful.getBody().front().getOps<ac::InstanceOp>())
    if (instance.getInstanceName() == "value")
      storage = instance;
  ASSERT_TRUE(storage);
  ASSERT_TRUE(injectLiteralIdentity(
      storage->getOpOperand(storage->getNumOperands() - 1)));
  OpOperand *nestedUse = nullptr;
  sourceNested.walk([&](ac::YieldOp yield) {
    if (!yield->getParentOfType<ac::SourceMethodOp>())
      return;
    if (nestedUse)
      return;
    for (OpOperand &operand : yield->getOpOperands())
      if (operand.get().getDefiningOp<ac::BitsConstantOp>() ||
          operand.get().getDefiningOp<ac::BitsExtractOp>()) {
        nestedUse = &operand;
        break;
      }
  });
  ASSERT_NE(nestedUse, nullptr);
  ASSERT_TRUE(injectLiteralIdentity(*nestedUse));

  auto bodyBefore = text(unit->body->getOperation());
  auto interfaceBefore = text(unit->interface->getOperation());
  compiler::SourceLinkStages stages;
  auto linked = compiler::linkSourceUnits(
      {{*unit->body, *unit->interface}}, "literal_wires.Top", error(), compiler::SourceLinkTarget::Hardware, &stages);
  ASSERT_TRUE(succeeded(linked)) << diagnostics;
  auto linkedTop = definition(**linked, "literal_wires.Top");
  ASSERT_TRUE(linkedTop);
  EXPECT_TRUE(literalConstructorWires(linkedTop).empty());
  for (auto create : linkedTop.getBody().front().getOps<ac::StructCreateOp>())
    for (Value input : create.getValues())
      if (isa<ac::BitsType>(input.getType()))
        EXPECT_TRUE(input.getDefiningOp<ac::BitsConstantOp>());
  for (auto create : linkedTop.getBody().front().getOps<ac::TableCreateOp>())
    for (Value input : create.getInputs())
      EXPECT_TRUE(input.getDefiningOp<ac::BitsConstantOp>());
  EXPECT_EQ(text(unit->body->getOperation()), bodyBefore);
  EXPECT_EQ(text(unit->interface->getOperation()), interfaceBefore);

  auto before = parseSourceString<ModuleOp>(stages.beforeTransactions,
                                            ParserConfig(&context, false));
  ASSERT_TRUE(before);
  auto direct = definition(*before, "literal_wires.Direct");
  auto stateful = definition(*before, "literal_wires.Stateful");
  auto nested = definition(*before, "literal_wires.NestedRule");
  ASSERT_TRUE(direct && stateful && nested);
  EXPECT_TRUE(cast<ac::YieldOp>(direct.getBody().front().getTerminator())
                  .getValues()[0]
                  .getDefiningOp<ac::BitsExtractOp>());
  unsigned retainedStateExtracts = 0;
  stateful.walk([&](ac::BitsExtractOp) { ++retainedStateExtracts; });
  EXPECT_GT(retainedStateExtracts, 0u);
  unsigned retainedNestedExtracts = 0;
  nested.walk([&](ac::BitsExtractOp extract) {
    if (!isa<ac::ModuleOp>(extract->getParentOp()))
      ++retainedNestedExtracts;
  });
  EXPECT_GT(retainedNestedExtracts, 0u);
}

TEST_F(SourceModuleScalarResultsTest,
       LinkKeepsLiteralExtractsOutsideEveryCompactionBoundary) {
  auto unit = compile(literalWireSource(), "literal_wires.py");
  ASSERT_TRUE(succeeded(unit)) << diagnostics;

  auto check = [&](auto mutate, auto retained) {
    OwningOpRef<ModuleOp> body(cast<ModuleOp>(unit->body->clone()));
    auto top = definition(*body, "literal_wires.Top");
    ASSERT_TRUE(top);
    ASSERT_TRUE(injectLiteralConstructorWire(top));
    auto candidates = literalConstructorWires(top);
    ASSERT_FALSE(candidates.empty());
    mutate(top, candidates.front());
    auto bodyBefore = text(body->getOperation());
    auto interfaceBefore = text(unit->interface->getOperation());
    diagnostics.clear();
    auto linked = compiler::linkSourceUnits({{*body, *unit->interface}},
                                              "literal_wires.Top", error());
    ASSERT_TRUE(succeeded(linked)) << diagnostics;
    auto finalTop = definition(**linked, "literal_wires.Top");
    ASSERT_TRUE(finalTop);
    EXPECT_TRUE(retained(finalTop));
    EXPECT_EQ(text(body->getOperation()), bodyBefore);
    EXPECT_EQ(text(unit->interface->getOperation()), interfaceBefore);
  };

  check(
      [&](ac::ModuleOp, ac::BitsExtractOp extract) {
        auto input = extract.getInput().getDefiningOp<ac::BitsConstantOp>();
        ASSERT_TRUE(input);
        auto width = cast<ac::BitsType>(input.getType()).getWidth();
        input.getResult().setType(
            ac::BitsType::get(&context, withInteger(width, 8, 99)));
      },
      [](ac::ModuleOp top) {
        return llvm::any_of(top.getBody().front().getOps<ac::BitsExtractOp>(),
                            [](ac::BitsExtractOp extract) {
                              return extract.getInput().getType() !=
                                     extract.getResult().getType();
                            });
      });

  check(
      [&](ac::ModuleOp, ac::BitsExtractOp extract) {
        auto input = extract.getInput().getDefiningOp<ac::BitsConstantOp>();
        ASSERT_TRUE(input);
        auto width = cast<ac::BitsType>(input.getType()).getWidth();
        input.getResult().setType(
            ac::BitsType::get(&context, withInteger(width, 9)));
        extract->setAttr("low", withInteger(extract.getLow(), 1));
      },
      [](ac::ModuleOp top) {
        return llvm::any_of(
            top.getBody().front().getOps<ac::BitsExtractOp>(),
            [](ac::BitsExtractOp extract) {
              auto value =
                  extract.getLow().getTree().getAs<DictionaryAttr>("value");
              auto integer = value ? value.getAs<ac::MathIntAttr>("value")
                                   : ac::MathIntAttr();
              return integer && integer.getCanonicalValue() == "1";
            });
      });

  check(
      [&](ac::ModuleOp, ac::BitsExtractOp extract) {
        extract->setAttr("low", nonliteralZero(extract.getLow()));
      },
      [](ac::ModuleOp top) {
        return llvm::any_of(
            top.getBody().front().getOps<ac::BitsExtractOp>(),
            [](ac::BitsExtractOp extract) {
              auto kind = extract.getLow().getTree().getAs<StringAttr>("kind");
              return kind && kind.getValue() == "binary";
            });
      });

  check(
      [&](ac::ModuleOp, ac::BitsExtractOp extract) {
        auto constant = extract.getInput().getDefiningOp<ac::BitsConstantOp>();
        ASSERT_TRUE(constant);
        OpBuilder builder(extract);
        OperationState state(extract.getLoc(),
                             ac::BitsUnaryOp::getOperationName());
        state.addOperands(constant.getResult());
        state.addTypes(constant.getResult().getType());
        state.addAttribute("opcode", b.getStringAttr("not"));
        auto *nonconstant = builder.create(state);
        extract->setOperand(0, nonconstant->getResult(0));
      },
      [](ac::ModuleOp top) {
        return llvm::any_of(
            top.getBody().front().getOps<ac::BitsExtractOp>(),
            [](ac::BitsExtractOp extract) {
              return extract.getInput().getDefiningOp<ac::BitsUnaryOp>();
            });
      });

  check(
      [&](ac::ModuleOp, ac::BitsExtractOp extract) {
        extract->setAttr("literal_wire_test", UnitAttr::get(&context));
      },
      [](ac::ModuleOp top) {
        return llvm::any_of(top.getBody().front().getOps<ac::BitsExtractOp>(),
                            [](ac::BitsExtractOp extract) {
                              return extract->hasAttr("literal_wire_test");
                            });
      });

  check(
      [&](ac::ModuleOp, ac::BitsExtractOp extract) {
        OpBuilder builder(extract);
        builder.setInsertionPointAfter(extract);
        OperationState state(extract.getLoc(),
                             ac::BitsUnaryOp::getOperationName());
        state.addOperands(extract.getResult());
        state.addTypes(extract.getResult().getType());
        state.addAttribute("opcode", b.getStringAttr("not"));
        builder.create(state);
      },
      [](ac::ModuleOp top) {
        return llvm::any_of(
            top.getBody().front().getOps<ac::BitsExtractOp>(),
            [](ac::BitsExtractOp extract) {
              bool constructor = false, other = false;
              for (Operation *user : extract->getUsers()) {
                constructor |= isa<ac::StructCreateOp, ac::TableCreateOp>(user);
                other |= isa<ac::BitsUnaryOp>(user);
              }
              return constructor && other;
            });
      });
}

TEST_F(SourceModuleScalarResultsTest,
       RejectedSourceTamperCannotBeHiddenAndPreservesBothInputs) {
  auto unit = compile(literalWireSource(), "literal_wires.py");
  ASSERT_TRUE(succeeded(unit)) << diagnostics;
  OwningOpRef<ModuleOp> body(cast<ModuleOp>(unit->body->clone()));
  auto top = definition(*body, "literal_wires.Top");
  ASSERT_TRUE(top);
  ASSERT_TRUE(injectLiteralConstructorWire(top));
  ASSERT_FALSE(literalConstructorWires(top).empty());
  top->setAttr("ac.return_form", b.getStringAttr("tuple"));
  ASSERT_TRUE(succeeded(verify(*body))) << diagnostics;
  auto bodyBefore = text(body->getOperation());
  auto interfaceBefore = text(unit->interface->getOperation());
  compiler::SourceLinkStages stages{"before sentinel", "routed sentinel", "after sentinel"};
  diagnostics.clear();
  auto linked = compiler::linkSourceUnits(
      {{*body, *unit->interface}}, "literal_wires.Top", error(), compiler::SourceLinkTarget::Hardware, &stages);
  EXPECT_TRUE(failed(linked));
  EXPECT_NE(diagnostics.find("published interface"), std::string::npos)
      << diagnostics;
  EXPECT_EQ(stages.beforeTransactions, "before sentinel");
  EXPECT_EQ(stages.afterTransactions, "after sentinel");
  EXPECT_EQ(text(body->getOperation()), bodyBefore);
  EXPECT_EQ(text(unit->interface->getOperation()), interfaceBefore);
}

TEST_F(SourceModuleScalarResultsTest,
       LinkPoolsOnlyExactConstructorConstantsAndKeepsFirstMetadata) {
  auto unit = compile(literalWireSource(), "literal_wires.py");
  ASSERT_TRUE(succeeded(unit)) << diagnostics;
  auto top = definition(*unit->body, "literal_wires.Top");
  ASSERT_TRUE(top);
  exposeLiteralConstructorConstants(top);
  ASSERT_TRUE(injectDuplicateConstructorConstant(top));
  auto constants = literalConstructorConstants(top);
  ASSERT_GE(constants.size(), 2u);
  auto type = constants.front().getResult().getType();
  SmallVector<ac::BitsConstantOp> same;
  for (auto constant : constants)
    if (constant.getResult().getType() == type)
      same.push_back(constant);
  ASSERT_GE(same.size(), 2u);
  auto representativeValue = same.front().getValue();
  auto representativeLocation = same.front().getLoc();
  auto bodyBefore = text(unit->body->getOperation());
  auto interfaceBefore = text(unit->interface->getOperation());
  auto linked = compiler::linkSourceUnits({{*unit->body, *unit->interface}},
                                            "literal_wires.Top", error());
  ASSERT_TRUE(succeeded(linked)) << diagnostics;
  auto finalTop = definition(**linked, "literal_wires.Top");
  ASSERT_TRUE(finalTop);
  SmallVector<ac::BitsConstantOp> used;
  auto collect = [&](Operation *operation) {
    for (Value operand : operation->getOperands())
      if (auto constant = operand.getDefiningOp<ac::BitsConstantOp>();
          constant && constant.getResult().getType() == type &&
          !llvm::is_contained(used, constant))
        used.push_back(constant);
  };
  for (auto create : finalTop.getBody().front().getOps<ac::StructCreateOp>())
    collect(create);
  for (auto create : finalTop.getBody().front().getOps<ac::TableCreateOp>())
    collect(create);
  ASSERT_EQ(used.size(), 1u);
  EXPECT_EQ(used.front().getValue(), representativeValue);
  EXPECT_EQ(used.front().getLoc(), representativeLocation);
  EXPECT_EQ(text(unit->body->getOperation()), bodyBefore);
  EXPECT_EQ(text(unit->interface->getOperation()), interfaceBefore);

  auto check = [&](auto mutate, auto retained) {
    OwningOpRef<ModuleOp> body(cast<ModuleOp>(unit->body->clone()));
    auto candidate = definition(*body, "literal_wires.Top");
    ASSERT_TRUE(candidate);
    auto values = literalConstructorConstants(candidate);
    ASSERT_GE(values.size(), 2u);
    mutate(values.front());
    auto candidateBefore = text(body->getOperation());
    diagnostics.clear();
    auto result = compiler::linkSourceUnits({{*body, *unit->interface}},
                                              "literal_wires.Top", error());
    ASSERT_TRUE(succeeded(result)) << diagnostics;
    auto linkedCandidate = definition(**result, "literal_wires.Top");
    ASSERT_TRUE(linkedCandidate);
    EXPECT_TRUE(retained(linkedCandidate));
    EXPECT_EQ(text(body->getOperation()), candidateBefore);
  };

  check(
      [&](ac::BitsConstantOp constant) {
        auto width =
            cast<ac::BitsType>(constant.getResult().getType()).getWidth();
        constant.getResult().setType(
            ac::BitsType::get(&context, withInteger(width, 8, 101)));
      },
      [](ac::ModuleOp candidate) {
        SmallVector<Type> types;
        candidate.walk([&](ac::BitsConstantOp constant) {
          if (llvm::any_of(constant->getUsers(),
                           [](Operation *user) {
                             return isa<ac::StructCreateOp, ac::TableCreateOp>(
                                 user);
                           }) &&
              !llvm::is_contained(types, constant.getResult().getType()))
            types.push_back(constant.getResult().getType());
        });
        return types.size() > 1;
      });

  check(
      [&](ac::BitsConstantOp constant) {
        constant->setAttr("literal_pool_test", UnitAttr::get(&context));
      },
      [](ac::ModuleOp candidate) {
        bool marked = false, other = false;
        candidate.walk([&](ac::BitsConstantOp constant) {
          if (!llvm::any_of(constant->getUsers(), [](Operation *user) {
                return isa<ac::StructCreateOp, ac::TableCreateOp>(user);
              }))
            return;
          marked |= constant->hasAttr("literal_pool_test");
          other |= !constant->hasAttr("literal_pool_test");
        });
        return marked && other;
      });

  check(
      [&](ac::BitsConstantOp constant) {
        OpBuilder builder(constant->getNextNode());
        OperationState state(constant.getLoc(),
                             ac::BitsUnaryOp::getOperationName());
        state.addOperands(constant.getResult());
        state.addTypes(constant.getResult().getType());
        state.addAttribute("opcode", b.getStringAttr("not"));
        builder.create(state);
      },
      [](ac::ModuleOp candidate) {
        return llvm::any_of(
            candidate.getBody().front().getOps<ac::BitsConstantOp>(),
            [](ac::BitsConstantOp constant) {
              bool constructor = false, scalar = false;
              for (Operation *user : constant->getUsers()) {
                constructor |= isa<ac::StructCreateOp, ac::TableCreateOp>(user);
                scalar |= isa<ac::BitsUnaryOp>(user);
              }
              return constructor && scalar;
            });
      });
}

TEST_F(SourceModuleScalarResultsTest,
       LiteralPoolPlanGrowthFailureLeavesAllConstructorOperandsUnchanged) {
  auto unit = compile(literalWireSource(), "literal_wires.py");
  ASSERT_TRUE(succeeded(unit)) << diagnostics;
  auto top = definition(*unit->body, "literal_wires.Top");
  ASSERT_TRUE(top);
  exposeLiteralConstructorConstants(top);
  ASSERT_TRUE(injectDuplicateConstructorConstant(top, 202));
  ASSERT_TRUE(injectDuplicateConstructorConstant(top, 203));
  auto constants = literalConstructorConstants(top);
  ASSERT_FALSE(constants.empty());
  auto type = constants.front().getResult().getType();
  EXPECT_GE(llvm::count_if(constants,
                           [&](ac::BitsConstantOp constant) {
                             return constant.getResult().getType() == type;
                           }),
            3u);
  auto before = text(unit->body->getOperation());
  constexpr uint64_t elementBytes =
      sizeof(compiler::detail::LiteralPayloadPlan::value_type);
  uint64_t available = 2 * elementBytes + 1;
  unsigned growthPayments = 0;
  auto plan = compiler::detail::planLiteralPayloadConstants(
      top.getBody().front(), [&](uint64_t, uint64_t bytes) {
        if (bytes != elementBytes && bytes != 2 * elementBytes)
          return success();
        ++growthPayments;
        if (bytes > available)
          return failure();
        available -= bytes;
        return success();
      });
  EXPECT_TRUE(failed(plan));
  EXPECT_EQ(growthPayments, 2u);
  EXPECT_EQ(text(unit->body->getOperation()), before);
}

} // namespace
