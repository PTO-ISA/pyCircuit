#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIRTypes.h"
#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"

#include <cstdint>

#include "gtest/gtest.h"

#if defined(PYCIRCUIT_CORE_PROBE)
// Core probe: the executable links the ACIRCore target as WHOLE_ARCHIVE,
// so every core member object is pulled regardless of use, and any
// reverse dependency inside ACIRCore fails here at link time. This TU
// references one symbol per enumerated core TU so the probe also detects
// a member that silently stopped being part of the closure (r9 §4).
#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"
#include "pycircuit/Dialect/ACIR/ACIRTypes.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "Dialect/ACIR/SourceSyntaxSiteContracts.h"
#include "Dialect/ACIR/SourceValueSemantics.h"
#include "pycircuit/Dialect/ACIR/SourceIdentifier.h"
#include "IR/StateDeclarationContracts.h"
namespace {
// Force the linker to see a reference to every pinned symbol: the pinned
// addresses feed an unprovable runtime branch, so no reference can be
// folded away regardless of optimization level (a bare `(void)&f;` can).
std::uintptr_t anchorSum(std::uintptr_t seed) { return seed; }
template <typename T, typename... Ts>
std::uintptr_t anchorSum(std::uintptr_t seed, T *head, Ts *...rest) {
  return anchorSum(seed + reinterpret_cast<std::uintptr_t>(head), rest...);
}
TEST(ACIRCoreStandalone, ReferencesEveryCoreUnit) {
  // One pinned symbol per core member TU (r9 §4); the anchorSum call
  // materializes every reference (r8 review fix — a bare `(void)&f;` can
  // be folded away, so it could not guarantee the reference).
  auto anchored = anchorSum(
      0u,
      // ACIRCoreDefs: generated attribute classes and their registration
      // (the generated typedef `get` anchors here too).
      &acir::ac::MathIntAttr::get,
      &acir::ac::BitsType::get,
      // ACIRPacketTypes: out-of-line custom type verifies and the shape
      // schema (not re-emitted by the generated classes).
      &acir::ac::detail::shapeSchema,
      // ACIRIdentityContracts: occurrence grammar and its memo.
      &acir::ac::detail::verifyOccurrence,
      // ACIRSourceContracts: source site/occurrence contract leaves.
      &acir::ac::detail::verifySite,
      // SourceStaticValueContracts: closed static-value grammar.
      &acir::ac::detail::verifyStaticValueStructure,
      // ACIRStaticContracts: static expression tree verification.
      &acir::ac::StaticExprAttr::verify,
      // SourceAttributeContracts: source domain/type-expr attr verifiers.
      &acir::ac::SourceDomainAttr::verify,
      // SourceSyntaxSiteContracts: lexical path grammar.
      &acir::ac::detail::verifyLexicalPath,
      // SourceValueSemantics: value opcode grammar.
      &acir::ac::detail::parseValueOpcode,
      // StateDeclarationContracts: declaration placement contracts.
      &acir::ac::verifyRoutedLogicalStatePlacement,
      // SourceIdentifier: Python identifier grammar leaf.
      &acir::ac::detail::verifyPythonAstIdentifier);
  // The sum feeds an unprovable runtime branch, so no reference can be
  // folded away regardless of optimization level.
  ASSERT_NE(anchored, 42u) << "unreachable: pinned symbol anchors";
  SUCCEED();
}
} // namespace
#elif defined(PYCIRCUIT_ANALYSIS_PROBE)
// Analysis-layer containment probe: this executable links ONLY the
// ACIRAnalysis target. Constructing the analysis machinery and running the
// staged transactional acceptance must pull no symbol from a target above
// the analysis layer (B2 step-2 design r9 §4); the configure-time
// pycircuit_verify_layering walk asserts the transitive closure. The ACIR
// dialect resolves through ACIRAnalysis's own operations dependency, which
// is below this layer in the directed topology.
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
namespace {
TEST(ACIRAnalysisStandalone, RunsTheStagedPackageAcceptance) {
  mlir::DialectRegistry registry;
  registry.insert<acir::ac::ACIRDialect>();
  mlir::MLIRContext context(registry);
  ASSERT_TRUE(context.getOrLoadDialect<acir::ac::ACIRDialect>() != nullptr);
  auto package = mlir::ModuleOp::create(mlir::UnknownLoc::get(&context));
  package->setAttr("ac.transactional",
                   mlir::IntegerAttr::get(mlir::IntegerType::get(&context, 64),
                                          2));
  package->setAttr("ac.stage", mlir::StringAttr::get(&context, "linked"));
  // The staged width-closure entry constructs HardwareAnalysis over the
  // package and walks its payload types (B2 step-2 design r3 row; invoked
  // from the analysis-side preflight acceptance at IX).
  EXPECT_TRUE(mlir::succeeded(
      acir::ac::verifyTransactionalLinkedPackage(package)));
  acir::ac::HardwareAnalysis analysis(package);
  EXPECT_EQ(analysis.getPackage(), package);
  // The legacy routed/final stage still rejects transactional packages with
  // the exact boundary text; the preflight build reaches that check through
  // the staged width closure first.
  std::string diagnostics;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &diagnostic) {
    diagnostics += diagnostic.str();
    return mlir::success();
  });
  EXPECT_TRUE(mlir::failed(analysis.verify()));
  EXPECT_NE(diagnostics.find(
                "transactional resource packages cannot enter legacy routed "
                "or final hardware analysis"),
            std::string::npos)
      << diagnostics;
}
} // namespace
#elif defined(PYCIRCUIT_GFIR_VALIDATION_PROBE)
// GFIR validation containment probe: this executable links ONLY the
// GFIRValidation target (C-package design §5). Running the routed package
// entrypoint must pull no symbol from HWIRValidation, GFIRDialect,
// ACIRTransforms or ACIRCodegen; the configure-time pycircuit_verify_layering
// walk asserts the transitive closure. The ACIR dialect resolves through
// GFIRValidation's own ACIRAnalysis dependency, below this layer.
#include "pycircuit/Dialect/ACIR/GFIRPackageValidation.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
namespace {
TEST(GFIRValidationStandalone, RunsTheRoutedPackageEntrypoint) {
  mlir::DialectRegistry registry;
  registry.insert<acir::ac::ACIRDialect>();
  mlir::MLIRContext context(registry);
  ASSERT_TRUE(context.getOrLoadDialect<acir::ac::ACIRDialect>() != nullptr);
  std::string diagnostics;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &diagnostic) {
    diagnostics += diagnostic.str();
    return mlir::success();
  });
  // The routed stage policy rejects a package without ac.stage with the
  // exact boundary text; the shared bounded pipeline runs to that check.
  auto unstaged = mlir::ModuleOp::create(mlir::UnknownLoc::get(&context));
  EXPECT_TRUE(mlir::failed(acir::ac::verifyRoutedPackage(unstaged)));
  EXPECT_NE(diagnostics.find("routed verification requires ac.stage = routed"),
            std::string::npos)
      << diagnostics;
  // Transactional packages never enter the legacy routed pipeline.
  diagnostics.clear();
  auto transactional = mlir::ModuleOp::create(mlir::UnknownLoc::get(&context));
  transactional->setAttr("ac.transactional",
                         mlir::IntegerAttr::get(mlir::IntegerType::get(&context, 64),
                                                2));
  EXPECT_TRUE(mlir::failed(acir::ac::verifyRoutedPackage(transactional)));
  EXPECT_NE(diagnostics.find(
                "transactional resource packages cannot enter legacy routed "
                "or final hardware analysis"),
            std::string::npos)
      << diagnostics;
}
} // namespace
#elif defined(PYCIRCUIT_HWIR_VALIDATION_PROBE)
// HWIR validation containment probe: this executable links ONLY the
// HWIRValidation target (C-package design §5). Running the final hardware
// package entrypoint must pull no symbol from GFIRValidation, GFIRDialect,
// ACIRTransforms or ACIRCodegen; the configure-time pycircuit_verify_layering
// walk asserts the transitive closure.
#include "pycircuit/Dialect/ACIR/HWIRPackageValidation.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
namespace {
TEST(HWIRValidationStandalone, RunsTheFinalPackageEntrypoint) {
  mlir::DialectRegistry registry;
  registry.insert<acir::ac::ACIRDialect>();
  mlir::MLIRContext context(registry);
  ASSERT_TRUE(context.getOrLoadDialect<acir::ac::ACIRDialect>() != nullptr);
  std::string diagnostics;
  mlir::ScopedDiagnosticHandler capture(&context, [&](mlir::Diagnostic &diagnostic) {
    diagnostics += diagnostic.str();
    return mlir::success();
  });
  // The final hardware stage policy rejects a retained routed stage with the
  // exact boundary text; routed packages require state refinement first.
  auto routed = mlir::ModuleOp::create(mlir::UnknownLoc::get(&context));
  routed->setAttr("ac.stage", mlir::StringAttr::get(&context, "routed"));
  EXPECT_TRUE(mlir::failed(acir::ac::verifyHardwarePackage(routed)));
  EXPECT_NE(diagnostics.find("executable hardware verification requires final "
                             "hardware; routed packages require state refinement"),
            std::string::npos)
      << diagnostics;
  // Transactional packages never enter the legacy final pipeline.
  diagnostics.clear();
  auto transactional = mlir::ModuleOp::create(mlir::UnknownLoc::get(&context));
  transactional->setAttr("ac.transactional",
                         mlir::IntegerAttr::get(mlir::IntegerType::get(&context, 64),
                                                2));
  EXPECT_TRUE(mlir::failed(acir::ac::verifyHardwarePackage(transactional)));
  EXPECT_NE(diagnostics.find(
                "transactional resource packages cannot enter legacy routed "
                "or final hardware analysis"),
            std::string::npos)
      << diagnostics;
}
} // namespace
#else
// Ops-layer containment probe: this executable links ONLY the ACIRDialect
// target. Loading the dialect and resolving registered operations must
// pull no symbol from a target above the operations layer that is not on
// its link line (B2 step-2 design r9 §4); the configure-time
// pycircuit_verify_layering walk asserts the transitive closure.
namespace {
TEST(ACIROpsStandalone, LoadsAndResolvesItsOperations) {
  mlir::DialectRegistry registry;
  registry.insert<acir::ac::ACIRDialect>();
  mlir::MLIRContext context(registry);
  auto *dialect = context.getOrLoadDialect<acir::ac::ACIRDialect>();
  ASSERT_TRUE(dialect != nullptr);
  EXPECT_EQ(dialect->getNamespace(), "ac");
  EXPECT_TRUE(mlir::RegisteredOperationName::lookup("ac.module", &context));
  EXPECT_TRUE(mlir::RegisteredOperationName::lookup("ac.instance", &context));
  EXPECT_TRUE(mlir::RegisteredOperationName::lookup("ac.state.read", &context));
}
} // namespace
#endif
