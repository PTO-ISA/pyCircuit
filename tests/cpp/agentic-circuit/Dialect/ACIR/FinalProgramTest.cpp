#if __has_include("Compiler/FinalProgram.h")
#include "Compiler/FinalProgram.h"
#define ACIR_HAS_FINAL_PROGRAM_API 1
#else
#define ACIR_HAS_FINAL_PROGRAM_API 0
#endif

#include "gtest/gtest.h"

#if ACIR_HAS_FINAL_PROGRAM_API
#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include <array>
#include <concepts>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace acir::compiler {
namespace {

struct TemporaryDirectory {
  llvm::SmallString<256> path;
  TemporaryDirectory() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-final-program", path));
  }
  ~TemporaryDirectory() { llvm::sys::fs::remove_directories(path); }
  std::string child(llvm::StringRef name) const {
    llvm::SmallString<256> result(path);
    llvm::sys::path::append(result, name);
    return result.str().str();
  }
};

int run(llvm::StringRef program, const std::vector<std::string> &owned,
        llvm::StringRef log) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : owned)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {std::nullopt,
                                                                   log, log};
  return llvm::sys::ExecuteAndWait(program, arguments, std::nullopt, redirects);
}

class FinalProgramTest : public ::testing::Test {
protected:
  FinalProgramTest() : context(dialects) {
    dialects.insert<ac::ACIRDialect, mlir::arith::ArithDialect>();
    context.appendDialectRegistry(dialects);
    context.loadAllAvailableDialects();
  }

  void SetUp() override {
    sourceRoot = temporary.child("src");
    ASSERT_FALSE(llvm::sys::fs::create_directories(sourceRoot));
    const std::string source = sourceRoot + "/main.py";
    write(source, R"py(from pycircuit import module, rule, log
@module
def Main():
    source: bool = True
    sink: bool = False
    state: bool = False
    @rule
    def transfer():
        nonlocal state, sink
        assert source, "source is set"
        log("info", "source", source)
        state = source
        sink = state
        return
    transfer()
)py");
    const std::string capture = temporary.child("capture.mlir");
    const std::string body = temporary.child("body.mlir");
    const std::string header = temporary.child("header.mlir");
    static constexpr llvm::StringLiteral captureScript = R"py(
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
                  {ACIR_TEST_PYTHON, "-c", captureScript.str(),
                   ACIR_TEST_REPO_ROOT, source, sourceRoot, capture},
                  temporary.child("capture.log")),
              0);
    ASSERT_EQ(run(ACIR_TEST_SOURCE_UNIT_HARNESS,
                  {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", capture,
                   "--package", "verify", "--path", "main.py", "--body-out",
                   body, "--interface-out", header},
                  temporary.child("compile.log")),
              0);
    bodyModule = mlir::parseSourceFile<mlir::ModuleOp>(body, &context);
    headerModule = mlir::parseSourceFile<mlir::ModuleOp>(header, &context);
    ASSERT_TRUE(bodyModule && headerModule);
    ASSERT_FALSE(llvm::sys::fs::remove_directories(sourceRoot));
  }

  void write(llvm::StringRef path, llvm::StringRef contents) {
    std::error_code error;
    llvm::raw_fd_ostream output(path, error);
    ASSERT_FALSE(error);
    output << contents;
  }

  auto emitError() {
    return [&]() -> mlir::InFlightDiagnostic {
      return mlir::emitError(mlir::UnknownLoc::get(&context));
    };
  }

  mlir::FailureOr<FinalProgram> emitReadyProgram() {
    SourceLinkUnit unit{*bodyModule, *headerModule};
    auto analysis =
        buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
    if (mlir::failed(analysis))
      return mlir::failure();
    return materializeFinalProgram(std::move(*analysis), emitError());
  }

  mlir::DialectRegistry dialects;
  mlir::MLIRContext context;
  TemporaryDirectory temporary;
  std::string sourceRoot;
  mlir::OwningOpRef<mlir::ModuleOp> bodyModule, headerModule;
};

template <typename Program, typename Emit>
concept HasFinalProgramMaterializer = requires(Program &&program, Emit emit) {
  {
    materializeFinalProgram(std::forward<Program>(program), emit)
  } -> std::same_as<mlir::FailureOr<FinalProgram>>;
};

template <typename Program, typename Emit, typename Callback>
void withEmitReadyProgram(Program &&program, Emit emit, Callback callback) {
  if constexpr (!HasFinalProgramMaterializer<Program, Emit>) {
    ADD_FAILURE() << "W10 RED: missing private API `FailureOr<FinalProgram> "
                     "materializeFinalProgram(FinalProgram&&, EmitError)`";
  } else {
    auto materialized =
        materializeFinalProgram(std::forward<Program>(program), emit);
    ASSERT_TRUE(mlir::succeeded(materialized));
    EXPECT_EQ(materialized->state(), FinalProgramState::EmitReady);
    EXPECT_TRUE(materialized->isEmitReady());
    EXPECT_TRUE(mlir::succeeded(verifyFinalProgram(*materialized, emit)));
    callback(*materialized);
  }
}

bool containsResidualFinalCarrier(const FinalProgram &program) {
  llvm::DenseSet<mlir::Operation *> definitions;
  bool residual = false;
  for (InstanceView *view : program.modules().views) {
    if (!view || !definitions.insert(view->module.getOperation()).second)
      continue;
    view->module->walk([&](mlir::Operation *operation) {
      residual |=
          mlir::isa<ac::SourceReadOp, ac::SourceUseOp, ac::ModuleImportOp>(
              operation);
      residual |= operation->getName().getStringRef() == "func.func";
      operation->walk([&](mlir::Operation *nested) {
        for (mlir::Type type : nested->getOperandTypes())
          residual |= mlir::isa<ac::MathIntType>(type);
        for (mlir::Type type : nested->getResultTypes())
          residual |= mlir::isa<ac::MathIntType>(type);
        for (mlir::NamedAttribute attribute : nested->getAttrs()) {
          std::string text;
          llvm::raw_string_ostream(text) << attribute.getValue();
          residual |= text.find("#ac.static_expr") != std::string::npos;
        }
      });
    });
  }
  return residual;
}

TEST_F(FinalProgramTest,
       CompleteSourceLinkUnitClosesAnalysisButIsNotEmitReady) {
  SourceLinkUnit unit{*bodyModule, *headerModule};
  auto program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(program));
  EXPECT_EQ(program->state(), FinalProgramState::AnalysisClosed);
  EXPECT_FALSE(program->isEmitReady());
  ASSERT_NE(program->modules().root, nullptr);
  ASSERT_FALSE(program->modules().views.empty());
  EXPECT_EQ(program->checks().modules, &program->modules());
  EXPECT_EQ(program->proposals().modules, &program->modules());
  EXPECT_EQ(program->proposals().checks, &program->checks());
  EXPECT_EQ(program->observations().modules, &program->modules());
  EXPECT_TRUE(mlir::succeeded(verifyFinalProgram(*program, emitError())));
}

TEST_F(FinalProgramTest, VerificationRejectsGraphMutationAndStageSpoof) {
  SourceLinkUnit unit{*bodyModule, *headerModule};
  auto program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(program));
  auto &modules = const_cast<ModuleGraph &>(program->modules());
  modules.root = nullptr;
  EXPECT_TRUE(mlir::failed(verifyFinalProgram(*program, emitError())));

  program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(program));
  program->modules().views.front()->module->setAttr(
      "ac.stage", mlir::StringAttr::get(&context, "final"));
  EXPECT_TRUE(mlir::failed(verifyFinalProgram(*program, emitError())));
}

TEST_F(FinalProgramTest,
       VerificationRejectsMissingOwnerCommitCheckAndObservation) {
  SourceLinkUnit unit{*bodyModule, *headerModule};
  auto program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(program));
  ASSERT_FALSE(program->modules().views.empty());
  const_cast<InstanceView *>(program->modules().views.front())->owner = {};
  EXPECT_TRUE(mlir::failed(verifyFinalProgram(*program, emitError())))
      << "owner omission must invalidate the frozen closure";

  program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(program));
  auto &proposals = const_cast<ProposalGraph &>(program->proposals());
  ASSERT_FALSE(proposals.states.empty());
  proposals.states.front().commit.stateID = {};
  EXPECT_TRUE(mlir::failed(verifyFinalProgram(*program, emitError())))
      << "missing commit must invalidate the frozen closure";

  program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(program));
  auto &checks = const_cast<CheckGraph &>(program->checks());
  ASSERT_FALSE(checks.bindings.empty());
  checks.bindings.front().owner = nullptr;
  EXPECT_TRUE(mlir::failed(verifyFinalProgram(*program, emitError())))
      << "missing check owner must invalidate the frozen closure";

  program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(program));
  auto &observations = const_cast<ObservationGraph &>(program->observations());
  ASSERT_FALSE(observations.bindings.empty());
  observations.bindings.front().owner = nullptr;
  EXPECT_TRUE(mlir::failed(verifyFinalProgram(*program, emitError())))
      << "missing observation owner must invalidate the frozen closure";
}

TEST_F(FinalProgramTest, MissingRootUnitIsRejected) {
  auto program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>{}, emitError());
  EXPECT_TRUE(mlir::failed(program));
}

TEST_F(FinalProgramTest, MaterializerProducesVerifiedEmitReadyCarrier) {
  SourceLinkUnit unit{*bodyModule, *headerModule};
  auto program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(program));
  withEmitReadyProgram(std::move(*program), emitError(),
                       [&](FinalProgram &ready) {
                         EXPECT_FALSE(containsResidualFinalCarrier(ready));
                       });
}

TEST_F(FinalProgramTest,
       MaterializerPreservesStateCommitPermitCheckAndObservationClosure) {
  SourceLinkUnit unit{*bodyModule, *headerModule};
  auto program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(program));
  llvm::SmallVector<mlir::Attribute> stateIDs;
  for (const StateProposals &state : program->proposals().states)
    stateIDs.push_back(state.stateID);
  const size_t checkCount = program->checks().bindings.size();
  const size_t observationCount = program->observations().bindings.size();
  const bool globalPermit = program->proposals().globalPermitAlways;

  withEmitReadyProgram(
      std::move(*program), emitError(), [&](FinalProgram &ready) {
        ASSERT_EQ(ready.proposals().states.size(), stateIDs.size());
        for (auto [index, state] : llvm::enumerate(ready.proposals().states)) {
          EXPECT_EQ(state.stateID, stateIDs[index]);
          EXPECT_EQ(state.commit.stateID, stateIDs[index]);
          EXPECT_EQ(state.commit.permitAlways, globalPermit);
          if (state.commit.kind == CommitPairKind::Forward) {
            EXPECT_TRUE(state.commit.data);
            EXPECT_TRUE(state.commit.enable);
          }
        }
        EXPECT_EQ(ready.proposals().globalPermitAlways, globalPermit);
        EXPECT_EQ(ready.checks().bindings.size(), checkCount);
        EXPECT_EQ(ready.observations().bindings.size(), observationCount);
      });
}

TEST_F(FinalProgramTest,
       EmitReadyVerifierRejectsTargetSwapPermitDisconnectAndStageSpoof) {
  auto exercise = [&](llvm::StringRef mutation) {
    SourceLinkUnit unit{*bodyModule, *headerModule};
    auto program =
        buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
    ASSERT_TRUE(mlir::succeeded(program));
    withEmitReadyProgram(
        std::move(*program), emitError(), [&](FinalProgram &ready) {
          if (mutation == "target-swap") {
            auto &proposals = const_cast<ProposalGraph &>(ready.proposals());
            ASSERT_GE(proposals.states.size(), 2u);
            proposals.states[0].commit.stateID = proposals.states[1].stateID;
          } else if (mutation == "permit-disconnect") {
            auto &proposals = const_cast<ProposalGraph &>(ready.proposals());
            proposals.globalPermitAlways = !proposals.globalPermitAlways;
          } else {
            ready.modules().views.front()->module->setAttr(
                "ac.stage", mlir::StringAttr::get(&context, "final"));
          }
          EXPECT_TRUE(mlir::failed(verifyFinalProgram(ready, emitError())));
        });
  };
  for (llvm::StringRef mutation :
       {"target-swap", "permit-disconnect", "stage-spoof"}) {
    SCOPED_TRACE(mutation.str());
    exercise(mutation);
  }
}

TEST_F(FinalProgramTest, EmitReadyVerifierRejectsResidualSourceOperation) {
  SourceLinkUnit unit{*bodyModule, *headerModule};
  auto program =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(program));
  mlir::Operation *sourceRead = nullptr;
  bodyModule->walk([&](ac::SourceReadOp operation) {
    if (!sourceRead)
      sourceRead = operation->clone();
  });
  ASSERT_NE(sourceRead, nullptr);
  withEmitReadyProgram(
      std::move(*program), emitError(), [&](FinalProgram &ready) {
        auto module = ready.modules().views.front()->module;
        mlir::Block &block = module.getBody().front();
        block.getOperations().insert(block.getTerminator()->getIterator(),
                                     sourceRead);
        EXPECT_TRUE(mlir::failed(verifyFinalProgram(ready, emitError())));
      });
}

TEST_F(FinalProgramTest, EmitReadyVerifierRejectsCommitDataAndEnableSwap) {
  for (llvm::StringRef mutation : {"data", "enable"}) {
    SCOPED_TRACE(mutation.str());
    auto ready = emitReadyProgram();
    ASSERT_TRUE(mlir::succeeded(ready));
    auto &proposals = const_cast<ProposalGraph &>(ready->proposals());
    llvm::SmallVector<StateProposals *> forwards;
    for (StateProposals &state : proposals.states)
      if (state.commit.kind == CommitPairKind::Forward)
        forwards.push_back(&state);
    ASSERT_GE(forwards.size(), 2u);
    if (mutation == "data")
      forwards[0]->commit.data = forwards[1]->commit.data;
    else
      forwards[0]->commit.enable = forwards[1]->commit.enable;
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())))
        << "same-typed commit " << mutation.str() << " swap was accepted";
  }
}

TEST_F(FinalProgramTest, EmitReadyVerifierRejectsStateCarrierFactMutations) {
  for (llvm::StringRef mutation : {"initializer", "physical-type"}) {
    SCOPED_TRACE(mutation.str());
    auto ready = emitReadyProgram();
    ASSERT_TRUE(mlir::succeeded(ready));
    ASSERT_FALSE(ready->stateCarriers().empty());
    const FinalProgram::StateCarrierSnapshot *ownedCarrier = nullptr;
    for (const auto &candidate : ready->stateCarriers())
      if (candidate.declaration) {
        ownedCarrier = &candidate;
        break;
      }
    ASSERT_NE(ownedCarrier, nullptr);
    auto reg = mlir::dyn_cast_or_null<ac::RegOp>(ownedCarrier->declaration);
    ASSERT_TRUE(reg);
    if (mutation == "initializer") {
      auto old =
          mlir::cast<mlir::IntegerAttr>(reg->getAttr("ac.initial_value"));
      auto value = old.getValue();
      value.flipBit(0);
      reg->setAttr("ac.initial_value",
                   mlir::IntegerAttr::get(old.getType(), value));
    } else {
      auto regType = mlir::cast<ac::RegType>(ownedCarrier->handle.getType());
      auto integer = mlir::cast<mlir::IntegerType>(regType.getElementType());
      auto replacement =
          mlir::IntegerType::get(&context, integer.getWidth() == 1 ? 2 : 1);
      mlir::Value handle = ownedCarrier->handle;
      handle.setType(ac::RegType::get(&context, replacement));
    }
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())))
        << mutation.str() << " mutation was accepted";
    EXPECT_TRUE(mlir::failed(emitFinalCpp(*ready, emitError())))
        << "emission must fail closed after " << mutation.str();
  }
}

TEST_F(FinalProgramTest, EmitReadyStateAliasTableResolvesEveryFrozenHandle) {
  auto ready = emitReadyProgram();
  ASSERT_TRUE(mlir::succeeded(ready));
  ASSERT_FALSE(ready->stateAliases().empty());
  for (const auto &alias : ready->stateAliases()) {
    ASSERT_NE(alias.view, nullptr);
    ASSERT_TRUE(alias.handle);
    ASSERT_TRUE(alias.stateID);
    EXPECT_EQ(ready->resolveStateID(alias.view, alias.handle), alias.stateID);
  }
  EXPECT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
}

TEST_F(FinalProgramTest, EmitReadyVerifierRejectsStateAliasMutations) {
  for (llvm::StringRef mutation : {"view", "handle"}) {
    SCOPED_TRACE(mutation.str());
    auto ready = emitReadyProgram();
    ASSERT_TRUE(mlir::succeeded(ready));
    ASSERT_GE(ready->stateAliases().size(), 2u);
    auto &snapshot = const_cast<FinalProgram::StateAliasSnapshot &>(
        ready->stateAliases().front());
    if (mutation == "view")
      snapshot.view = nullptr;
    else
      snapshot.handle = ready->stateAliases().back().handle;
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())))
        << mutation.str() << " state alias mutation was accepted";
  }
}

TEST_F(FinalProgramTest, EmitReadyVerifierRejectsEveryCheckBindingMutation) {
  for (llvm::StringRef mutation :
       {"condition", "path", "kind", "location", "registration",
        "stable-ordinal", "required-index"}) {
    SCOPED_TRACE(mutation.str());
    auto ready = emitReadyProgram();
    ASSERT_TRUE(mlir::succeeded(ready));
    auto &checks = const_cast<CheckGraph &>(ready->checks());
    ASSERT_FALSE(checks.bindings.empty());
    CheckBinding &binding = checks.bindings.front();
    mlir::Builder builder(&context);
    if (mutation == "condition")
      binding.condition = binding.path;
    else if (mutation == "path")
      binding.path = binding.condition;
    else if (mutation == "kind")
      binding.kind = builder.getStringAttr("range");
    else if (mutation == "location")
      binding.location = builder.getDictionaryAttr({});
    else if (mutation == "registration")
      binding.registration =
          binding.checkID.getAs<mlir::DictionaryAttr>("check");
    else if (mutation == "stable-ordinal")
      ++binding.stableOrdinal;
    else
      ++binding.requiredIndex;
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())))
        << "check " << mutation.str() << " mutation was accepted";
  }
}

TEST_F(FinalProgramTest,
       EmitReadyVerifierRejectsEveryObservationBindingMutation) {
  for (llvm::StringRef mutation :
       {"path", "value", "kind", "value-ids", "constraints", "registration",
        "stable-ordinal", "required-index"}) {
    SCOPED_TRACE(mutation.str());
    auto ready = emitReadyProgram();
    ASSERT_TRUE(mlir::succeeded(ready));
    auto &observations = const_cast<ObservationGraph &>(ready->observations());
    ASSERT_FALSE(observations.bindings.empty());
    ObservationBinding &binding = observations.bindings.front();
    ASSERT_FALSE(binding.values.empty());
    mlir::Builder builder(&context);
    if (mutation == "path")
      binding.path = binding.values.front();
    else if (mutation == "value")
      binding.values.front() = binding.path;
    else if (mutation == "kind")
      binding.kind = builder.getStringAttr("print");
    else if (mutation == "value-ids")
      binding.valueIDs = builder.getArrayAttr({});
    else if (mutation == "constraints")
      binding.valueConstraints = builder.getArrayAttr({});
    else if (mutation == "registration")
      binding.registration =
          binding.observationID.getAs<mlir::DictionaryAttr>("site");
    else if (mutation == "stable-ordinal")
      ++binding.stableOrdinal;
    else
      ++binding.requiredIndex;
    EXPECT_TRUE(mlir::failed(verifyFinalProgram(*ready, emitError())))
        << "observation " << mutation.str() << " mutation was accepted";
  }
}

TEST_F(FinalProgramTest,
       HierarchicalMaterializationRemovesImportsAndPreservesCallerIR) {
  std::string hierarchyRoot = temporary.child("hierarchy");
  ASSERT_FALSE(llvm::sys::fs::create_directories(hierarchyRoot));
  std::string leafSource = hierarchyRoot + "/leaf.py";
  std::string parentSource = hierarchyRoot + "/parent.py";
  write(leafSource, R"py(from pycircuit import module, rule, log

@module
def Leaf(source: bool, sink: bool):
    @rule
    def forward():
        nonlocal sink
        log("info", "leaf", source)
        sink = source
        return
    forward()
)py");
  write(parentSource, R"py(from pycircuit import module
from .leaf import Leaf

@module
def Parent():
    source: bool = True
    sink: bool = False
    relay: bool = False
    left = Leaf(source, relay)
    right = Leaf(relay, sink)
)py");
  static constexpr llvm::StringLiteral captureScript = R"py(
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
  auto capture = [&](llvm::StringRef source, llvm::StringRef output,
                     llvm::StringRef log) {
    return run(ACIR_TEST_PYTHON,
               {ACIR_TEST_PYTHON, "-c", captureScript.str(),
                ACIR_TEST_REPO_ROOT, source.str(), hierarchyRoot, output.str()},
               log);
  };
  std::string leafCapture = temporary.child("leaf.transport.mlir");
  std::string parentCapture = temporary.child("parent.transport.mlir");
  std::string leafBodyPath = temporary.child("leaf.body.mlir");
  std::string leafHeaderPath = temporary.child("leaf.interface.mlir");
  std::string parentBodyPath = temporary.child("parent.body.mlir");
  std::string parentHeaderPath = temporary.child("parent.interface.mlir");
  ASSERT_EQ(capture(leafSource, leafCapture, temporary.child("leaf-py.log")),
            0);
  ASSERT_EQ(
      capture(parentSource, parentCapture, temporary.child("parent-py.log")),
      0);
  ASSERT_EQ(run(ACIR_TEST_SOURCE_UNIT_HARNESS,
                {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", leafCapture,
                 "--package", "verify", "--path", "leaf.py", "--body-out",
                 leafBodyPath, "--interface-out", leafHeaderPath},
                temporary.child("leaf-compile.log")),
            0);
  ASSERT_EQ(run(ACIR_TEST_SOURCE_UNIT_HARNESS,
                {ACIR_TEST_SOURCE_UNIT_HARNESS, "--capture", parentCapture,
                 "--package", "verify", "--path", "parent.py", "--header",
                 leafHeaderPath, "--body-out", parentBodyPath,
                 "--interface-out", parentHeaderPath},
                temporary.child("parent-compile.log")),
            0);

  auto leafBody = mlir::parseSourceFile<mlir::ModuleOp>(leafBodyPath, &context);
  auto leafHeader =
      mlir::parseSourceFile<mlir::ModuleOp>(leafHeaderPath, &context);
  auto parentBody =
      mlir::parseSourceFile<mlir::ModuleOp>(parentBodyPath, &context);
  auto parentHeader =
      mlir::parseSourceFile<mlir::ModuleOp>(parentHeaderPath, &context);
  ASSERT_TRUE(leafBody && leafHeader && parentBody && parentHeader);
  std::string callerBefore;
  llvm::raw_string_ostream before(callerBefore);
  before << *leafBody << *leafHeader << *parentBody << *parentHeader;
  before.flush();
  EXPECT_NE(callerBefore.find("ac.module.import"), std::string::npos);
  EXPECT_NE(callerBefore.find("ac.source.read"), std::string::npos);

  llvm::SmallVector<SourceLinkUnit> units{{*leafBody, *leafHeader},
                                          {*parentBody, *parentHeader}};
  auto analysis = buildFinalProgram(units, emitError());
  ASSERT_TRUE(mlir::succeeded(analysis));
  auto ready = materializeFinalProgram(std::move(*analysis), emitError());

  std::string callerAfter;
  llvm::raw_string_ostream after(callerAfter);
  after << *leafBody << *leafHeader << *parentBody << *parentHeader;
  after.flush();
  EXPECT_EQ(callerAfter, callerBefore);

  ASSERT_TRUE(mlir::succeeded(ready));
  EXPECT_TRUE(ready->isEmitReady());
  EXPECT_TRUE(mlir::succeeded(verifyFinalProgram(*ready, emitError())));
  EXPECT_FALSE(containsResidualFinalCarrier(*ready));
  ASSERT_GT(ready->instances().size(), 1u);
  auto cpp = emitFinalCpp(*ready, emitError());
  ASSERT_TRUE(mlir::succeeded(cpp));
  EXPECT_NE(cpp->find("class FinalSystem"), std::string::npos);
}

TEST_F(FinalProgramTest, FinalEmittersAreDeterministicAndToolAccepted) {
  SourceLinkUnit unit{*bodyModule, *headerModule};
  auto analysis =
      buildFinalProgram(llvm::ArrayRef<SourceLinkUnit>(unit), emitError());
  ASSERT_TRUE(mlir::succeeded(analysis));
  EXPECT_TRUE(mlir::failed(emitFinalCpp(*analysis, emitError())));
  EXPECT_TRUE(mlir::failed(emitFinalVerilog(*analysis, emitError())));

  auto ready = materializeFinalProgram(std::move(*analysis), emitError());
  ASSERT_TRUE(mlir::succeeded(ready));
  auto cpp = emitFinalCpp(*ready, emitError());
  auto verilog = emitFinalVerilog(*ready, emitError());
  ASSERT_TRUE(mlir::succeeded(cpp));
  ASSERT_TRUE(mlir::succeeded(verilog));
  EXPECT_EQ(*cpp, *emitFinalCpp(*ready, emitError()));
  EXPECT_EQ(*verilog, *emitFinalVerilog(*ready, emitError()));
  EXPECT_NE(cpp->find("gfsim/SimDFF.h"), std::string::npos);
  EXPECT_NE(cpp->find("void Work("), std::string::npos);
  EXPECT_EQ(cpp->find("bool Check()"), std::string::npos);
  EXPECT_EQ(cpp->find("bool Drive("), std::string::npos);
  EXPECT_NE(cpp->find("void Xfer() noexcept"), std::string::npos);
  EXPECT_NE(cpp->find("class FinalSystem"), std::string::npos);
  EXPECT_NE(verilog->find("module FinalModel"), std::string::npos);
  EXPECT_NE(verilog->find("input logic clk"), std::string::npos);
  EXPECT_NE(verilog->find("input logic reset"), std::string::npos);
  EXPECT_NE(verilog->find("always_ff @(posedge clk)"), std::string::npos);
  EXPECT_NE(verilog->find("global_permit"), std::string::npos);
  for (llvm::StringRef forbidden : {"Queue", "SimQueue", "ac.source", "pyc."}) {
    EXPECT_EQ(cpp->find(forbidden.str()), std::string::npos) << forbidden.str();
    EXPECT_EQ(verilog->find(forbidden.str()), std::string::npos)
        << forbidden.str();
  }

  const std::string cppPath = temporary.child("final.cpp");
  const std::string verilogPath = temporary.child("final.sv");
  write(cppPath, *cpp);
  write(verilogPath, *verilog);
  EXPECT_EQ(run("/usr/bin/clang++",
                {"/usr/bin/clang++", "-std=c++20", "-fsyntax-only", "-I",
                 std::string(ACIR_TEST_REPO_ROOT) + "/simulator/gfsim/include",
                 cppPath},
                temporary.child("cpp.log")),
            0);
  EXPECT_EQ(run("/opt/homebrew/bin/verilator",
                {"/opt/homebrew/bin/verilator", "--lint-only", "--language",
                 "1800-2017", "--top-module", "FinalModel", verilogPath},
                temporary.child("verilog.log")),
            0);
  EXPECT_EQ(run("/opt/homebrew/bin/verilator",
                {"/opt/homebrew/bin/verilator", "--lint-only", "--language",
                 "1800-2017", "--top-module", "FinalModelSim", verilogPath},
                temporary.child("verilog-sim.log")),
            0);
}

} // namespace
} // namespace acir::compiler
#else
TEST(FinalProgramApiAvailability, PrivateCompilerFinalProgramContractExists) {
  FAIL() << "RED: compiler/acir/lib/Compiler/FinalProgram.h is missing; "
            "FinalProgram cannot own and verify the source graph closure";
}
#endif
