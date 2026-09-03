#ifndef ACIR_DIALECT_ACIR_GRAPHREGION_H
#define ACIR_DIALECT_ACIR_GRAPHREGION_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/DialectInterface.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"

#include <string>

namespace mlir {
class Operation;
}

namespace acir::ac {

class ACIRDialect;

/// Absolute identity assigned to a Task 7/8 state owner by static hierarchy
/// elaboration. This is intentionally distinct from the pre-freeze,
/// definition-qualified identity exposed by the declaration's effect.
struct ElaboratedStateOwner {
  mlir::Operation *declaration;
  std::string path;
  std::string stableId;
  /// Logical harness-bound trace sources owned by this process instance.
  llvm::SmallVector<std::string> traceSources;
};

/// Every owning structural object expanded in the selected hierarchy. Unlike
/// ElaboratedStateOwner this also includes instances, arrays, ordered
/// collections, and their elements, so topology freeze can persist one stable
/// machine-checkable ownership manifest.
struct ElaboratedTopologyOwner {
  mlir::Operation *declaration;
  std::string path;
  std::string stableId;
};

/// Context-owned registry of exact build-time structural providers. The
/// registry is populated by dialect-registry extensions before parsing and is
/// never model/runtime configuration.
class StructuralProviderRegistry {
public:
  void registerExternal(llvm::StringRef name);
  void registerGenerator(llvm::StringRef name);
  bool hasExternal(llvm::StringRef name) const;
  bool hasGenerator(llvm::StringRef name) const;

private:
  llvm::StringSet<> externalProviders;
  llvm::StringSet<> generatorProviders;
};

class StructuralProviderDialectInterface
    : public mlir::DialectInterface::Base<StructuralProviderDialectInterface> {
public:
  explicit StructuralProviderDialectInterface(mlir::Dialect *dialect)
      : Base(dialect) {}
  StructuralProviderRegistry &getRegistry() { return registry; }

private:
  StructuralProviderRegistry registry;
};

StructuralProviderRegistry &
getStructuralProviderRegistry(mlir::MLIRContext *context);

/// Verifies whole-file hierarchy selection, stable ownership identities and
/// the statically-resolved subset of topology freeze implemented by ACIR.
mlir::LogicalResult verifyGraphStructure(mlir::Operation *topLevel);

/// Verifies the graph and returns every elaborated queue/event/resource,
/// address-space, process, and statistics owner in deterministic hierarchy
/// order.
mlir::LogicalResult collectElaboratedStateOwners(
    mlir::Operation *topLevel,
    llvm::SmallVectorImpl<ElaboratedStateOwner> &owners);

/// Verifies the graph and returns every elaborated structural owner in
/// deterministic hierarchy order.
mlir::LogicalResult collectElaboratedTopologyOwners(
    mlir::Operation *topLevel,
    llvm::SmallVectorImpl<ElaboratedTopologyOwner> &owners);

/// Returns true for concrete builtin static parameter values admitted by the
/// public v0.1 graph contract.
bool isConcreteStaticValue(mlir::Attribute value);

/// Builds the canonical lexicographic element path for a static N-D array.
std::string buildArrayElementPath(llvm::StringRef base,
                                  llvm::ArrayRef<int64_t> indices);

} // namespace acir::ac

#endif
