#include "Dialect/ACIR/ACIRNumericComposition.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace acir::compiler {
namespace {
using namespace acir::ac;

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(llvm::sys::fs::createUniqueDirectory(
        "numeric-composition-adversarial", path));
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
  ADD_FAILURE() << "missing field " << name.str();
  return dictionary;
}

class NumericCompositionAdversarialTest : public ::testing::Test {
protected:
  NumericCompositionAdversarialTest() : context(dialects) {
    dialects.insert<ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("source");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    writeFile(sourceRoot + "/composition.py", R"py(from typing import Annotated
from pycircuit import rule, system
Word = Annotated[int, range(256)]
@system
def CompositionAuthority():
    guard: bool = True
    left: Word = 1
    right: Word = 2
    sink: Word = 0
    @rule
    def choose():
        nonlocal sink
        if guard and left < 4:
            assert left != 0, "left is live"
            sink = (left + 1) & 255
        else:
            assert right != 0, "right is live"
            sink = (right + 1) & 255
    choose()
)py");
  }

  mlir::OwningOpRef<mlir::ModuleOp> compile() {
    const std::string source = sourceRoot + "/composition.py";
    const std::string capture = temporary.child("capture.mlir");
    const std::string body = temporary.child("lowered.mlir");
    const std::string header = temporary.child("interface.mlir");
    const std::string log = temporary.child("compile.log");
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
    if (run(ACIR_TEST_PYTHON,
            {ACIR_TEST_PYTHON, "-c", script.str(), ACIR_TEST_REPO_ROOT, source,
             sourceRoot, capture},
            log) != 0)
      return {};
    if (run(ACIR_TEST_SOURCE_UNIT_HARNESS,
            {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", capture, "--package",
             "composition", "--path", "composition.py", "--body-out", body,
             "--interface-out", header, "--lower-numeric"},
            log) != 0) {
      ADD_FAILURE() << readFile(log);
      return {};
    }
    return mlir::parseSourceFile<mlir::ModuleOp>(body, &context);
  }

  RuleOp rule(mlir::ModuleOp file) {
    RuleOp result;
    file.walk([&](RuleOp candidate) { result = candidate; });
    return result;
  }

  void reject(mlir::ModuleOp original, llvm::StringRef label,
              llvm::function_ref<void(mlir::ModuleOp)> mutate) {
    SCOPED_TRACE(label.str());
    auto copy = mlir::OwningOpRef<mlir::ModuleOp>(
        mlir::cast<mlir::ModuleOp>(original->clone()));
    mutate(*copy);
    EXPECT_TRUE(mlir::failed(mlir::verify(*copy)));
  }

  void replaceObligation(RuleOp target, mlir::Attribute oldNode,
                         mlir::Attribute newNode) {
    for (NumericProofOp proof :
         target.getBody().front().getOps<NumericProofOp>()) {
      auto obligations = proof.getObligationsAttr();
      llvm::SmallVector<mlir::Attribute> values(obligations.begin(),
                                                obligations.end());
      bool changed = false;
      for (mlir::Attribute &value : values)
        if (value == oldNode) {
          value = newNode;
          changed = true;
        }
      if (changed)
        proof->setAttr("obligations",
                       mlir::Builder(&context).getArrayAttr(values));
    }
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot;
};

TEST_F(NumericCompositionAdversarialTest,
       RejectsCoherentRecipeSsaAndProofCorruption) {
  auto original = compile();
  ASSERT_TRUE(original);
  ASSERT_TRUE(mlir::succeeded(mlir::verify(*original)));

  reject(*original, "unknown opcode with matching obligation",
         [&](mlir::ModuleOp file) {
           RuleOp target = rule(file);
           auto required =
               target->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
           llvm::SmallVector<mlir::Attribute> nodes(required.begin(),
                                                    required.end());
           mlir::Builder builder(&context);
           for (mlir::Attribute &raw : nodes) {
             auto node = mlir::cast<mlir::DictionaryAttr>(raw);
             auto opcode = node.getAs<mlir::StringAttr>("operator");
             if (!opcode || opcode.getValue() != "add")
               continue;
             auto changed = replaceField(builder, node, "operator",
                                         builder.getStringAttr("mul"));
             replaceObligation(target, raw, changed);
             raw = changed;
             break;
           }
           target->setAttr("ac.required_numeric", builder.getArrayAttr(nodes));
         });
  reject(*original, "forged addi attribute", [&](mlir::ModuleOp file) {
    RuleOp add = rule(file);
    auto operation =
        *add.getBody().front().getOps<mlir::arith::AddIOp>().begin();
    operation->setAttr("forged", mlir::UnitAttr::get(&context));
  });
  reject(*original, "forged cmpi attribute", [&](mlir::ModuleOp file) {
    mlir::arith::CmpIOp compare;
    file.walk([&](mlir::arith::CmpIOp candidate) {
      if (!compare)
        compare = candidate;
    });
    ASSERT_TRUE(compare);
    compare->setAttr("forged", mlir::UnitAttr::get(&context));
  });
  reject(*original, "proof obligation reorder", [&](mlir::ModuleOp file) {
    for (NumericProofOp proof :
         rule(file).getBody().front().getOps<NumericProofOp>()) {
      auto obligations = proof.getObligationsAttr();
      if (obligations.size() < 2)
        continue;
      llvm::SmallVector<mlir::Attribute> values(obligations.begin(),
                                                obligations.end());
      std::reverse(values.begin(), values.end());
      proof->setAttr("obligations",
                     mlir::Builder(&context).getArrayAttr(values));
      return;
    }
    ADD_FAILURE() << "no multi-node proof";
  });
  reject(*original, "signed boundary target", [&](mlir::ModuleOp file) {
    RuleOp target = rule(file);
    auto required =
        target->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
    llvm::SmallVector<mlir::Attribute> nodes(required.begin(), required.end());
    mlir::Builder builder(&context);
    for (mlir::Attribute &raw : nodes) {
      auto node = mlir::cast<mlir::DictionaryAttr>(raw);
      auto opcode = node.getAs<mlir::StringAttr>("operator");
      if (!opcode || opcode.getValue() != "to_bits")
        continue;
      auto boundary = node.getAs<mlir::DictionaryAttr>("target");
      auto domain = boundary.getAs<mlir::DictionaryAttr>("domain");
      domain = replaceField(builder, domain, "interpretation",
                            builder.getStringAttr("signed"));
      boundary = replaceField(builder, boundary, "domain", domain);
      auto changed = replaceField(builder, node, "target", boundary);
      replaceObligation(target, raw, changed);
      raw = changed;
      break;
    }
    target->setAttr("ac.required_numeric", builder.getArrayAttr(nodes));
  });
}

TEST_F(NumericCompositionAdversarialTest,
       RejectsConstantRightAndBoundaryAuthorityBypass) {
  auto original = compile();
  ASSERT_TRUE(original);

  reject(*original, "constant-right changed to input",
         [&](mlir::ModuleOp file) {
           RuleOp target = rule(file);
           auto required =
               target->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
           llvm::SmallVector<mlir::Attribute> nodes(required.begin(),
                                                    required.end());
           mlir::Builder builder(&context);
           size_t from = 0;
           for (size_t index = 0; index < nodes.size(); ++index) {
             auto node = mlir::cast<mlir::DictionaryAttr>(nodes[index]);
             auto opcode = node.getAs<mlir::StringAttr>("operator");
             if (opcode && opcode.getValue() == "from_bits") {
               from = index;
               break;
             }
           }
           for (mlir::Attribute &raw : nodes) {
             auto node = mlir::cast<mlir::DictionaryAttr>(raw);
             auto opcode = node.getAs<mlir::StringAttr>("operator");
             if (!opcode || opcode.getValue() != "add")
               continue;
             auto operands = node.getAs<mlir::ArrayAttr>("operands");
             llvm::SmallVector<mlir::Attribute> refs(operands.begin(),
                                                     operands.end());
             refs[1] = builder.getDictionaryAttr({
                 builder.getNamedAttr("kind", builder.getStringAttr("node")),
                 builder.getNamedAttr("index", builder.getI32IntegerAttr(from)),
             });
             auto changed = replaceField(builder, node, "operands",
                                         builder.getArrayAttr(refs));
             replaceObligation(target, raw, changed);
             raw = changed;
             break;
           }
           auto add =
               *target.getBody().front().getOps<mlir::arith::AddIOp>().begin();
           add.getRhsMutable().assign(add.getLhs());
           target->setAttr("ac.required_numeric", builder.getArrayAttr(nodes));
         });

  reject(
      *original, "boundary use redirected to K255", [&](mlir::ModuleOp file) {
        RuleOp target = rule(file);
        auto required =
            target->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
        auto uses = target->getAttrOfType<mlir::ArrayAttr>("ac.required_uses");
        auto selectedUse = mlir::cast<mlir::DictionaryAttr>(uses[0]);
        mlir::DictionaryAttr maskID;
        auto boundaryID = selectedUse.getAs<mlir::DictionaryAttr>("value");
        std::optional<size_t> boundaryIndex;
        for (auto [index, raw] : llvm::enumerate(required)) {
          auto node = mlir::cast<mlir::DictionaryAttr>(raw);
          if (node.getAs<mlir::DictionaryAttr>("id") == boundaryID)
            boundaryIndex = index;
        }
        ASSERT_TRUE(boundaryIndex);
        for (size_t index = *boundaryIndex; index-- > 0;) {
          auto node = mlir::cast<mlir::DictionaryAttr>(required[index]);
          auto opcode = node.getAs<mlir::StringAttr>("operator");
          if (!opcode || opcode.getValue() != "constant")
            continue;
          auto operands = node.getAs<mlir::ArrayAttr>("operands");
          auto ref = mlir::cast<mlir::DictionaryAttr>(operands[0]);
          auto value = ref.getAs<MathIntAttr>("value");
          if (value && value.getCanonicalValue() == "255") {
            maskID = node.getAs<mlir::DictionaryAttr>("id");
            break;
          }
        }
        ASSERT_TRUE(maskID && boundaryID);
        ValueBindingOp mask, boundary;
        for (ValueBindingOp binding :
             target.getBody().front().getOps<ValueBindingOp>()) {
          if (binding.getIdAttr() == maskID)
            mask = binding;
          if (binding.getIdAttr() == boundaryID)
            boundary = binding;
        }
        ASSERT_TRUE(mask && boundary);
        mlir::Builder builder(&context);
        llvm::SmallVector<mlir::Attribute> rows(uses.begin(), uses.end());
        auto row = mlir::cast<mlir::DictionaryAttr>(rows[0]);
        rows[0] = replaceField(builder, row, "value", maskID);
        target->setAttr("ac.required_uses", builder.getArrayAttr(rows));
        auto use = *target.getBody().front().getOps<ValueUseOp>().begin();
        use->setAttr("source", maskID);
        use.getValueMutable().assign(mask.getValue());
        use.getValidMutable().assign(mask.getValid());
        for (mlir::arith::SelectOp select :
             target.getBody().front().getOps<mlir::arith::SelectOp>())
          if (select.getTrueValue() == boundary.getValue())
            select.getTrueValueMutable().assign(mask.getValue());
      });
}

} // namespace
} // namespace acir::compiler
