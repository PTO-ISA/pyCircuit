// Independent final-IR tests start from the existing production-captured
// address-grant fixture. No generated candidate supplies a numerical oracle.
#include "Compiler/SourceLink.h"
#include "Compiler/SourceUnit.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
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

class HardwareWriteDomainsTest : public ::testing::Test {
protected:
  MLIRContext context;
  Builder attrs{&context};
  llvm::SmallString<256> directory;
  std::string diagnostics;
  std::unique_ptr<ScopedDiagnosticHandler> handler;
  void SetUp() override {
    context.loadDialect<ac::ACIRDialect>();
    ASSERT_FALSE(
        llvm::sys::fs::createUniqueDirectory("write-domain-proof", directory));
    handler =
        std::make_unique<ScopedDiagnosticHandler>(&context, [&](Diagnostic &d) {
          llvm::raw_string_ostream out(diagnostics);
          d.print(out);
          diagnostics += '\n';
          return success();
        });
  }
  void TearDown() override { llvm::sys::fs::remove_directories(directory); }
  std::string path(StringRef file) {
    llvm::SmallString<256> result(directory);
    llvm::sys::path::append(result, file);
    return result.str().str();
  }
  auto error() {
    return [&] { return emitError(attrs.getUnknownLoc()); };
  }
  OwningOpRef<ModuleOp> compileCaptured(StringRef sourceName, StringRef top) {
    auto captured = parseSourceFile<ModuleOp>(path("capture.mlir"), &context);
    if (!captured)
      return {};
    auto headers =
        compiler::SourceHeaderRegistry::create(&context, {}, error());
    if (failed(headers))
      return {};
    auto owner = attrs.getDictionaryAttr(
        {attrs.getNamedAttr("package", attrs.getStringAttr("execution")),
         attrs.getNamedAttr("path", attrs.getStringAttr(sourceName))});
    auto unit =
        compiler::compilePythonSourceUnit(*captured, owner, *headers, error());
    EXPECT_TRUE(succeeded(unit)) << diagnostics;
    if (failed(unit))
      return {};
    auto final = compiler::linkHardwareUnits({{*unit->body, *unit->interface}},
                                             top, error());
    EXPECT_TRUE(succeeded(final)) << diagnostics;
    return succeeded(final) ? std::move(*final) : OwningOpRef<ModuleOp>{};
  }
  bool capture(StringRef script, StringRef option = "") {
    std::vector<std::string> owned{
        PYCIRCUIT_TEST_PYTHON, "-c",
        script.str(),          PYCIRCUIT_TEST_REPO_ROOT,
        path("capture.mlir"),  option.str()};
    SmallVector<StringRef> arguments;
    for (auto &value : owned)
      arguments.push_back(value);
    auto log = path("capture.log");
    std::array<std::optional<StringRef>, 3> redirects{std::nullopt, log, log};
    int status =
        llvm::sys::ExecuteAndWait(owned[0], arguments, std::nullopt, redirects);
    auto output = llvm::MemoryBuffer::getFile(log);
    EXPECT_EQ(status, 0) << (output ? output.get()->getBuffer().str() : log);
    return status == 0;
  }
  OwningOpRef<ModuleOp> fixture(bool wideIndex = false) {
    static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
repo, output = map(Path, sys.argv[1:3])
wide = sys.argv[3] == "wide"
sys.path.insert(0, str(repo / "python"))
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
original = repo / "tests/compiler/lit/Source/Inputs/source_check_execution/address_grants.py"
text = original.read_text()
if wide:
    assert "ac.table[2, Entry]" in text and "i: ac.u1, j: ac.u1" in text
    text = text.replace("ac.table[2, Entry]", "ac.table[16, Entry]").replace("i: ac.u1, j: ac.u1", "i: ac.u4, j: ac.u4")
source = output.with_name("address_grants.py")
source.write_text(text)
output.write_text(_emit_source_transport(_capture_source_file(source, source_root=source.parent)), encoding="utf-8")
)py";
    if (!capture(script, wideIndex ? "wide" : "narrow"))
      return {};
    return compileCaptured("address_grants.py", "execution.address_grants.Top");
  }
  OwningOpRef<ModuleOp> hierarchyFixture(bool orderedParent = false) {
    static constexpr llvm::StringLiteral script = R"py(
import sys
from pathlib import Path
repo, output = map(Path, sys.argv[1:3])
sys.path.insert(0, str(repo / "python"))
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport
text = (repo / "tests/system/fixtures/source_system/hierarchical_state_provider.py").read_text()
text += '''

@ac.module
def Top(selector: ac.u1):
    direct = StateCell(selector)
    relayed = PayloadRelay(selector)
    standalone = PayloadRelay(not selector)

    @ac.rule
    def drive() -> None:
        if selector:
            before, after = direct.update(9)
            relayed.update(11)
            assert after == before + 9

    drive()
'''
if sys.argv[3] == "ordered":
    text = text.replace(
        "    direct = StateCell(selector)",
        "    ordered = OrderedStateCell()\n    direct = StateCell(selector)",
    ).replace("    drive()\n", '''
    @ac.rule
    def first() -> None:
        before, after = ordered.update(3)
        assert after == before + 3

    @ac.rule(after=(first,))
    def second() -> None:
        before, after = ordered.update(5)
        assert after == before + 5

    first()
    second()
    drive()
''')
source = output.with_name("hierarchy.py")
source.write_text(text)
output.write_text(_emit_source_transport(_capture_source_file(source, source_root=source.parent)), encoding="utf-8")
)py";
    if (!capture(script, orderedParent ? "ordered" : "unordered"))
      return {};
    return compileCaptured("hierarchy.py", "execution.hierarchy.Top");
  }
  LogicalResult verifyFinal(ModuleOp file) {
    ac::HardwareAnalysis analysis(file);
    auto plan = analysis.getSourceCheckPlan();
    if (failed(plan))
      return failure();
    EXPECT_FALSE(plan->checks.empty());
    EXPECT_GE(plan->commits.size(), 3u);
    return analysis.verifySourceCheckPlan(*plan);
  }
  void rejects(ModuleOp file, StringRef expected = {}) {
    diagnostics.clear();
    ASSERT_TRUE(succeeded(verify(file))) << diagnostics;
    diagnostics.clear();
    EXPECT_TRUE(failed(verifyFinal(file)));
    EXPECT_FALSE(diagnostics.empty());
    if (!expected.empty())
      EXPECT_NE(diagnostics.find(expected.str()), std::string::npos)
          << diagnostics;
  }
  struct Domain {
    ac::RuleOp rule;
    unsigned ordinal = 0;
    DictionaryAttr row;
    Value mask() const {
      return rule->getResult(cast<IntegerAttr>(row.get("mask")).getInt());
    }
    Value rawMask() const {
      auto yield =
          cast<ac::YieldOp>(rule->getRegion(0).front().getTerminator());
      return yield.getOperand(cast<IntegerAttr>(row.get("mask")).getInt());
    }
  };
  Domain indexedDomain(ModuleOp file) {
    Domain result;
    file.walk([&](ac::RuleOp rule) {
      if (result.rule)
        return;
      auto sampled =
          rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      auto domains =
          sampled ? sampled.getAs<ArrayAttr>("write_domains") : ArrayAttr();
      if (!domains)
        return;
      for (auto [index, raw] : llvm::enumerate(domains)) {
        auto row = cast<DictionaryAttr>(raw);
        auto selectors = row.getAs<ArrayAttr>("selectors");
        if (llvm::any_of(selectors, [](Attribute raw) {
              return bool(cast<DictionaryAttr>(raw).get("index"));
            })) {
          result = {rule, unsigned(index), row};
          return;
        }
      }
    });
    return result;
  }
  ac::CollectionOp tableStorage(ModuleOp file) {
    ac::HardwareAnalysis hardware(file);
    ac::CollectionOp result;
    file.walk([&](ac::CollectionOp collection) {
      if (hardware.getPrimitiveKind(hardware.resolveCallee(collection)) ==
              "dffe" &&
          collection->getNumOperands() == 5 &&
          isa<ac::TableType>(collection->getOperand(3).getType()))
        result = collection;
    });
    return result;
  }
  ac::TableMapOp firstComposition(ac::CollectionOp storage) {
    auto latest = storage->getOperand(3).getDefiningOp<ac::TableMapOp>();
    if (!latest || latest.getTables().empty())
      return {};
    return latest.getTables()[0].getDefiningOp<ac::TableMapOp>();
  }
  DictionaryAttr changed(DictionaryAttr row, StringRef key, Attribute value) {
    NamedAttrList fields(row);
    fields.set(key, value);
    return fields.getDictionary(&context);
  }
  void changeDomain(Domain domain, StringRef key, Attribute value) {
    auto sampled =
        domain.rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    auto rows = sampled.getAs<ArrayAttr>("write_domains");
    SmallVector<Attribute> updated(rows.begin(), rows.end());
    updated[domain.ordinal] = changed(domain.row, key, value);
    domain.rule->setAttr(
        "ac.sampled_admission",
        changed(sampled, "write_domains", attrs.getArrayAttr(updated)));
  }
  Value constant(OpBuilder &b, Type type, unsigned value) {
    auto bits = cast<ac::BitsType>(type);
    NamedAttrList tree(bits.getWidth().getTree());
    auto number =
        ac::MathIntAttr::get(&context, llvm::APSInt(std::to_string(value)));
    tree.set("value",
             attrs.getDictionaryAttr(
                 {attrs.getNamedAttr("kind", attrs.getStringAttr("integer")),
                  attrs.getNamedAttr("value", number)}));
    OperationState state(b.getUnknownLoc(),
                         ac::BitsConstantOp::getOperationName());
    state.addTypes(type);
    state.addAttribute("value", ac::StaticExprAttr::get(
                                    &context, tree.getDictionary(&context)));
    return b.create(state)->getResult(0);
  }
  struct Hierarchy {
    ac::ModuleOp module;
    ac::InstanceOp storage;
    Value data;
    Value enable;
    ac::ValueMergeOp incomingMerge;
    ac::ValueMergeOp localMerge;
  };
  Hierarchy directHierarchy(ModuleOp file) {
    ac::HardwareAnalysis hardware(file);
    for (auto module : file.getOps<ac::ModuleOp>()) {
      auto wiring =
          module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
      auto incoming =
          wiring ? wiring.getAs<ArrayAttr>("incoming") : ArrayAttr();
      if (!incoming || incoming.size() != 1)
        continue;
      for (auto instance : module.getBody().front().getOps<ac::InstanceOp>()) {
        if (hardware.getPrimitiveKind(hardware.resolveCallee(instance)) !=
                "dffe" ||
            !isa<ac::BitsType>(instance->getResult(0).getType()))
          continue;
        auto row = cast<DictionaryAttr>(incoming[0]);
        Value data = module.getBody().front().getArgument(
            row.getAs<IntegerAttr>("data").getInt());
        Value enable = module.getBody().front().getArgument(
            row.getAs<IntegerAttr>("enable").getInt());
        ac::ValueMergeOp prefix;
        for (auto merge : module.getBody().front().getOps<ac::ValueMergeOp>())
          if (merge.getValues().size() == 1 && merge.getValues()[0] == data)
            prefix = merge;
        return {
            module, instance,
            data,   enable,
            prefix, instance->getOperand(3).getDefiningOp<ac::ValueMergeOp>()};
      }
    }
    return {};
  }
};

TEST_F(HardwareWriteDomainsTest,
       HierarchicalWholeOwnerCompositionHasVerifiedNonemptyPlan) {
  auto file = hierarchyFixture();
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  auto owner = directHierarchy(*file);
  ASSERT_TRUE(owner.module);
  ASSERT_TRUE(owner.incomingMerge);
  ASSERT_TRUE(owner.localMerge);
  EXPECT_NE(owner.incomingMerge, owner.localMerge);
  EXPECT_EQ(owner.incomingMerge.getBase(), owner.storage->getResult(0));
  EXPECT_EQ(owner.localMerge.getBase(), owner.incomingMerge.getNext());
}

TEST_F(HardwareWriteDomainsTest,
       HierarchicalIncomingFromOrderedParentHasVerifiedNonemptyPlan) {
  auto file = hierarchyFixture(true);
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
}

TEST_F(HardwareWriteDomainsTest,
       HierarchicalWholeOwnerPrefixRetainsIncomingDataOldQAndFire) {
  auto original = hierarchyFixture();
  ASSERT_TRUE(original) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*original))) << diagnostics;
  for (unsigned mutation = 0; mutation != 5; ++mutation) {
    SCOPED_TRACE(mutation);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto owner = directHierarchy(*file);
    ASSERT_TRUE(owner.incomingMerge);
    ASSERT_TRUE(owner.localMerge);
    OpBuilder b(owner.localMerge);
    if (mutation == 0)
      owner.incomingMerge->setOperand(2, owner.storage->getResult(0));
    if (mutation == 1)
      owner.localMerge->setOperand(0, owner.storage->getResult(0));
    if (mutation == 2) {
      b.setInsertionPoint(owner.incomingMerge);
      owner.incomingMerge->setOperand(0, constant(b, owner.data.getType(), 0));
    }
    if (mutation == 3) {
      b.setInsertionPoint(owner.incomingMerge);
      owner.incomingMerge->setOperand(1,
                                      constant(b, owner.enable.getType(), 1));
    }
    if (mutation == 4)
      owner.localMerge->setOperand(1, constant(b, owner.enable.getType(), 1));
    rejects(*file);
  }
}

TEST_F(HardwareWriteDomainsTest,
       HierarchicalWholeOwnerCommitRetainsIncomingAndLocalPoison) {
  auto original = hierarchyFixture();
  ASSERT_TRUE(original) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*original))) << diagnostics;
  for (unsigned mutation = 0; mutation != 3; ++mutation) {
    SCOPED_TRACE(mutation);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto owner = directHierarchy(*file);
    ASSERT_TRUE(owner.storage);
    auto joined =
        owner.storage->getOperand(2).getDefiningOp<ac::BitsBinaryOp>();
    ASSERT_TRUE(joined);
    ASSERT_EQ(joined.getOpcode(), "xor");
    if (mutation == 0)
      joined->setAttr("opcode", attrs.getStringAttr("or"));
    else {
      auto poison = joined.getRhs().getDefiningOp<ac::BitsBinaryOp>();
      ASSERT_TRUE(poison);
      auto leaf = (mutation == 1 ? poison.getLhs() : poison.getRhs())
                      .getDefiningOp<ac::BitsBinaryOp>();
      ASSERT_TRUE(leaf);
      ASSERT_EQ(leaf.getOpcode(), "xor");
      OpBuilder b(leaf);
      leaf->setOperand(1, constant(b, leaf.getRhs().getType(), 0));
    }
    rejects(*file);
  }
}

TEST_F(HardwareWriteDomainsTest,
       HierarchicalIncomingPairCannotBeMissingOrRetargeted) {
  auto original = hierarchyFixture();
  ASSERT_TRUE(original) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*original))) << diagnostics;
  for (unsigned mutation = 0; mutation != 5; ++mutation) {
    SCOPED_TRACE(mutation);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto owner = directHierarchy(*file);
    ASSERT_TRUE(owner.module);
    auto wiring =
        owner.module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
    auto rows = wiring.getAs<ArrayAttr>("incoming");
    auto row = cast<DictionaryAttr>(rows[0]);
    if (mutation == 0)
      owner.module->setAttr(
          "ac.transaction_wiring",
          changed(wiring, "incoming", attrs.getArrayAttr({})));
    else if (mutation < 3) {
      auto field = mutation == 1 ? "data" : "enable";
      auto replacement = changed(row, field, attrs.getI64IntegerAttr(0));
      owner.module->setAttr(
          "ac.transaction_wiring",
          changed(wiring, "incoming", attrs.getArrayAttr({replacement})));
    } else {
      ac::HardwareAnalysis hardware(*file);
      ac::InstanceOp allocation;
      file->walk([&](ac::InstanceOp instance) {
        if (hardware.resolveCallee(instance) == owner.module)
          allocation = instance;
      });
      ASSERT_TRUE(allocation);
      auto field = mutation == 3 ? "data" : "enable";
      unsigned index = row.getAs<IntegerAttr>(field).getInt();
      OpBuilder b(allocation);
      allocation->setOperand(
          index, constant(b, allocation->getOperand(index).getType(),
                          mutation == 3 ? 0 : 1));
    }
    if (mutation < 3) {
      diagnostics.clear();
      EXPECT_TRUE(failed(verify(*file)));
      EXPECT_NE(diagnostics.find(mutation == 0 ? "exactly cover appended inputs"
                                              : "exact appended pair"),
                std::string::npos)
          << diagnostics;
    } else {
      rejects(*file);
    }
  }
}

TEST_F(HardwareWriteDomainsTest,
       HierarchicalIncomingEnableCannotLeakToUnrelatedComputation) {
  auto file = hierarchyFixture();
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  auto owner = directHierarchy(*file);
  ASSERT_TRUE(owner.module);
  OpBuilder b(owner.module.getBody().front().getTerminator());
  OperationState state(b.getUnknownLoc(), ac::BitsBinaryOp::getOperationName());
  state.addOperands({owner.enable, owner.enable});
  state.addTypes(owner.enable.getType());
  state.addAttribute("opcode", attrs.getStringAttr("and"));
  b.create(state);
  rejects(*file);
}

TEST_F(HardwareWriteDomainsTest,
       HierarchicalTransitForwardsClosedPrefixInsteadOfRawIncomingData) {
  auto file = hierarchyFixture();
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  ac::HardwareAnalysis hardware(*file);
  bool mutated = false;
  for (auto module : file->getOps<ac::ModuleOp>()) {
    auto wiring =
        module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
    auto incoming = wiring ? wiring.getAs<ArrayAttr>("incoming") : ArrayAttr();
    if (!incoming || incoming.size() != 1 ||
        !wiring.getAs<ArrayAttr>("states").empty() ||
        !wiring.getAs<ArrayAttr>("writers").empty())
      continue;
    auto row = cast<DictionaryAttr>(incoming[0]);
    Value rawData = module.getBody().front().getArgument(
        row.getAs<IntegerAttr>("data").getInt());
    for (auto instance : module.getBody().front().getOps<ac::InstanceOp>()) {
      auto child =
          dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(instance));
      auto childWiring =
          child ? child->getAttrOfType<DictionaryAttr>("ac.transaction_wiring")
                : DictionaryAttr();
      auto routes =
          childWiring ? childWiring.getAs<ArrayAttr>("incoming") : ArrayAttr();
      if (!routes || routes.size() != 1)
        continue;
      auto childRow = cast<DictionaryAttr>(routes[0]);
      unsigned index = childRow.getAs<IntegerAttr>("data").getInt();
      ASSERT_NE(instance->getOperand(index), rawData);
      instance->setOperand(index, rawData);
      mutated = true;
      break;
    }
    if (mutated)
      break;
  }
  ASSERT_TRUE(mutated);
  rejects(*file);
}

TEST_F(HardwareWriteDomainsTest,
       ProductionIndexedFieldDomainsHaveNonemptyVerifiedPlan) {
  auto file = fixture();
  ASSERT_TRUE(file) << diagnostics;
  EXPECT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  auto domain = indexedDomain(*file);
  ASSERT_TRUE(domain.rule);
  EXPECT_EQ(domain.row.getAs<ArrayAttr>("selectors").size(), 2u);
  auto map = domain.rawMask().getDefiningOp<ac::TableMapOp>();
  ASSERT_TRUE(map);
  EXPECT_EQ(map->getNumResults(), 2u);
  auto storage = tableStorage(*file);
  ASSERT_TRUE(storage);
  ASSERT_TRUE(firstComposition(storage));
}

TEST_F(HardwareWriteDomainsTest,
       FirstUnorderedCandidateRetainsOldQAndExactGating) {
  auto original = fixture();
  ASSERT_TRUE(original) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*original))) << diagnostics;
  for (unsigned mutation = 0; mutation != 3; ++mutation) {
    SCOPED_TRACE(mutation);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto storage = tableStorage(*file);
    ASSERT_TRUE(storage);
    auto first = firstComposition(storage);
    ASSERT_TRUE(first);
    if (mutation == 0)
      first->setOperand(0, first.getTables()[1]);
    else {
      ac::ValueMergeOp leaf;
      first.getBody().walk([&](ac::ValueMergeOp merge) {
        if (!leaf && isa<ac::BitsType>(merge.getNext().getType()))
          leaf = merge;
      });
      ASSERT_TRUE(leaf);
      if (mutation == 1)
        leaf->setOperand(0, leaf.getValues()[0]);
      else {
        auto guard = leaf.getGuards()[0].getDefiningOp<ac::BitsBinaryOp>();
        ASSERT_TRUE(guard);
        guard->setAttr("opcode", attrs.getStringAttr("or"));
      }
    }
    rejects(*file, "write-domain");
  }
}

TEST_F(HardwareWriteDomainsTest,
       MaskUsesActualRawPathFullIndexAndSelectedField) {
  auto original = fixture();
  ASSERT_TRUE(original) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*original))) << diagnostics;
  for (unsigned mutation = 0; mutation != 5; ++mutation) {
    SCOPED_TRACE(mutation);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto domain = indexedDomain(*file);
    ASSERT_TRUE(domain.rule);
    auto map = domain.rawMask().getDefiningOp<ac::TableMapOp>();
    ASSERT_TRUE(map);
    auto &body = map.getBody().front();
    auto yield = cast<ac::YieldOp>(body.getTerminator());
    auto mask = yield.getOperand(1).getDefiningOp<ac::BitsBinaryOp>();
    ASSERT_TRUE(mask);
    auto equal = mask.getRhs().getDefiningOp<ac::BitsCompareOp>();
    ASSERT_TRUE(equal);
    if (mutation == 0) {
      OpBuilder b(mask);
      mask->setOperand(0, constant(b, mask.getLhs().getType(), 1));
    }
    if (mutation == 1)
      equal->setAttr("predicate", attrs.getStringAttr("case_eq"));
    if (mutation == 2)
      equal->setOperand(1, equal.getLhs());
    if (mutation == 3)
      mask->setAttr("opcode", attrs.getStringAttr("or"));
    if (mutation == 4) {
      auto merge = yield.getOperand(0).getDefiningOp<ac::ValueMergeOp>();
      ASSERT_TRUE(merge);
      merge->setAttr(
          "paths",
          attrs.getArrayAttr({attrs.getArrayAttr({attrs.getStringAttr("b")})}));
    }
    rejects(*file);
  }
}

TEST_F(HardwareWriteDomainsTest,
       FullWidthAddressEqualityCannotTruncateHighBits) {
  auto file = fixture(true);
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  auto domain = indexedDomain(*file);
  ASSERT_TRUE(domain.rule);
  auto map = domain.rawMask().getDefiningOp<ac::TableMapOp>();
  ASSERT_TRUE(map);
  auto &body = map.getBody().front();
  auto mask = cast<ac::YieldOp>(body.getTerminator())
                  .getOperand(1)
                  .getDefiningOp<ac::BitsBinaryOp>();
  ASSERT_TRUE(mask);
  auto equal = mask.getRhs().getDefiningOp<ac::BitsCompareOp>();
  ASSERT_TRUE(equal);
  OpBuilder b(equal);
  auto carrier = cast<ac::BitsType>(equal.getLhs().getType());
  NamedAttrList tree(carrier.getWidth().getTree());
  auto literal = [&](unsigned value) {
    tree.set("value",
             attrs.getDictionaryAttr(
                 {attrs.getNamedAttr("kind", attrs.getStringAttr("integer")),
                  attrs.getNamedAttr(
                      "value",
                      ac::MathIntAttr::get(
                          &context, llvm::APSInt(std::to_string(value))))}));
    return ac::StaticExprAttr::get(&context, tree.getDictionary(&context));
  };
  Type narrow = ac::BitsType::get(&context, literal(1));
  auto low = literal(0);
  auto extract = [&](Value input) {
    OperationState state(equal.getLoc(), ac::BitsExtractOp::getOperationName());
    state.addOperands(input);
    state.addTypes(narrow);
    state.addAttribute("low", low);
    return b.create(state)->getResult(0);
  };
  equal->setOperands({extract(equal.getLhs()), extract(equal.getRhs())});
  rejects(*file, "truncates its address");
}

TEST_F(HardwareWriteDomainsTest,
       SerializedSelectorsCannotRetargetAnotherEqualWidthField) {
  auto file = fixture();
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  auto domain = indexedDomain(*file);
  ASSERT_TRUE(domain.rule);
  auto selectors = domain.row.getAs<ArrayAttr>("selectors");
  SmallVector<Attribute> changedSelectors(selectors.begin(), selectors.end());
  changedSelectors.back() = attrs.getDictionaryAttr(
      {attrs.getNamedAttr("field", attrs.getStringAttr("b"))});
  changeDomain(domain, "selectors", attrs.getArrayAttr(changedSelectors));
  rejects(*file, "selected field");
}

TEST_F(HardwareWriteDomainsTest,
       AddressGrantCannotInvertItsActualCollisionTest) {
  auto file = fixture();
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  ac::BitsCompareOp grant;
  for (auto module : file->getOps<ac::ModuleOp>())
    for (auto compare : module.getBody().front().getOps<ac::BitsCompareOp>())
      if (compare.getPredicate() == "ne")
        grant = compare;
  ASSERT_TRUE(grant);
  grant->setAttr("predicate", attrs.getStringAttr("eq"));
  rejects(*file, "collision");
}

TEST_F(HardwareWriteDomainsTest, UnorderedCommitRetainsPoisonFromEveryEnable) {
  auto original = fixture();
  ASSERT_TRUE(original) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*original))) << diagnostics;
  for (bool inner : {false, true}) {
    SCOPED_TRACE(inner);
    OwningOpRef<ModuleOp> file(cast<ModuleOp>(original->clone()));
    auto storage = tableStorage(*file);
    ASSERT_TRUE(storage);
    auto enables = storage->getOperand(2).getDefiningOp<ac::TableSplatOp>();
    ASSERT_TRUE(enables);
    auto outer = enables.getInput().getDefiningOp<ac::BitsBinaryOp>();
    ASSERT_TRUE(outer);
    ASSERT_EQ(outer.getOpcode(), "xor");
    if (!inner)
      outer->setAttr("opcode", attrs.getStringAttr("or"));
    else {
      auto poison = outer.getRhs().getDefiningOp<ac::BitsBinaryOp>();
      ASSERT_TRUE(poison);
      auto oldPoison = poison.getLhs().getDefiningOp<ac::BitsBinaryOp>();
      ASSERT_TRUE(oldPoison);
      auto newPoison = poison.getRhs().getDefiningOp<ac::BitsBinaryOp>();
      ASSERT_TRUE(newPoison);
      newPoison->setOperand(1, oldPoison.getLhs());
    }
    rejects(*file);
  }
}

TEST_F(HardwareWriteDomainsTest, PrivateMaskCannotGainAnUnrelatedPayloadUse) {
  auto file = fixture();
  ASSERT_TRUE(file) << diagnostics;
  ASSERT_TRUE(succeeded(verifyFinal(*file))) << diagnostics;
  auto domain = indexedDomain(*file);
  ASSERT_TRUE(domain.rule);
  auto parent = domain.rule->getParentOfType<ac::ModuleOp>();
  auto table = cast<ac::TableType>(domain.mask().getType());
  OpBuilder b(parent.getBody().front().getTerminator());
  auto index = constant(b, table.getElementType(), 0);
  OperationState state(domain.rule.getLoc(),
                       ac::TableGetOp::getOperationName());
  state.addOperands({domain.mask(), index});
  state.addTypes({table.getElementType(), table.getElementType()});
  b.create(state);
  rejects(*file, "private write-domain mask");
}
} // namespace
