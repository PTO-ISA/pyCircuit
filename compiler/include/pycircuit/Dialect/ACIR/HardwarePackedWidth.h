#ifndef PYCIRCUIT_DIALECT_ACIR_HARDWAREPACKEDWIDTH_H
#define PYCIRCUIT_DIALECT_ACIR_HARDWAREPACKEDWIDTH_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysisContext.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/StringRef.h"
#include <cstdint>
#include <utility>

namespace acir::ac {
namespace detail {

/// The nominal-declaration and static-evaluation queries the shared
/// packed-width core needs (r8 review): HardwareAnalysis supplies the
/// binding-aware occurrence resolution, and the transactional closed-payload
/// verifier supplies the same package under an empty binding context. One
/// core, two resolvers; the concrete resolvers live in their TUs.
class PackedWidthResolver {
public:
  virtual ~PackedWidthResolver() = default;
  virtual mlir::ModuleOp getPackage() const = 0;
  virtual mlir::Operation *lookupDefinition(llvm::StringRef name) const = 0;
  virtual StructOp lookupStruct(StructType type) const = 0;
  virtual mlir::FailureOr<EnumDefinitionView>
  resolveEnum(EnumType type, mlir::Operation *site) const = 0;
  virtual mlir::FailureOr<mlir::Attribute>
  evaluateStatic(StaticExprAttr expression, const HardwareBindings &bindings,
                 mlir::Operation *site) const = 0;
  virtual mlir::FailureOr<mlir::Type>
  resolveType(mlir::Type type, const HardwareBindings &bindings,
              mlir::Operation *site) const = 0;
  virtual mlir::FailureOr<uint64_t>
  getPackedWidth(mlir::Type type, const HardwareBindings &bindings,
                 mlir::Operation *site) const = 0;
  /// The in-flight demanded-expression guard shared between the quiet
  /// evaluable probe and demanded evaluation; resolver-owned so both entry
  /// paths see the same cycle state.
  virtual llvm::SmallVector<std::pair<mlir::Attribute, mlir::Operation *>> &
  activeStatic() const = 0;
};

// The shared packed-width core, moved from HardwareAnalysis.cpp and
// CollectionAnalysis.cpp (r8 review): the transactional closed-payload
// verification runs these entries directly from the dialect layer, and the
// HardwareAnalysis members delegate to the same definitions.

/// Substitute static actuals without changing immutable source provenance.
mlir::Attribute
substituteHardwareAttribute(mlir::Attribute attribute,
                            const HardwareBindings &bindings);
/// A mathematical nonnegative (positive when requested) u64 hardware static.
mlir::FailureOr<uint64_t>
naturalHardwareStatic(mlir::Attribute attribute, mlir::Operation *site,
                      bool positive);
/// The quiet evaluable probe over one binding context; select and
/// short-circuit demands probe the closed branch through the resolver.
bool hardwareStaticEvaluable(mlir::Attribute attribute,
                             const HardwareBindings &bindings,
                             const PackedWidthResolver &resolver);
/// The demanded static evaluator over one binding context (references
/// resolve only through the supplied bindings).
mlir::FailureOr<mlir::Attribute>
evaluateHardwareStatic(const PackedWidthResolver &resolver,
                       StaticExprAttr expression,
                       const HardwareBindings &bindings,
                       mlir::Operation *site);
/// Nominal enum resolution shared by both resolvers: package lookup plus the
/// declaration grammar homed with the EnumOp verifier.
mlir::FailureOr<EnumDefinitionView>
resolveNominalEnum(EnumType type, mlir::Operation *site,
                   llvm::function_ref<mlir::Operation *(llvm::StringRef)>
                       lookupDefinition);
mlir::FailureOr<mlir::Type>
resolveHardwareType(const PackedWidthResolver &resolver, mlir::Type type,
                    const HardwareBindings &bindings, mlir::Operation *site);
mlir::FailureOr<llvm::SmallVector<uint64_t>>
resolveHardwareShape(const PackedWidthResolver &resolver, mlir::ArrayAttr shape,
                     const HardwareBindings &bindings, mlir::Operation *site);
uint64_t shapeProduct(llvm::ArrayRef<uint64_t> shape);
mlir::FailureOr<llvm::SmallVector<uint64_t>>
hardwareTableShape(const PackedWidthResolver &resolver, TableType table,
                   const HardwareBindings &bindings, mlir::Operation *site);
mlir::FailureOr<uint64_t>
hardwareTableSize(const PackedWidthResolver &resolver, TableType table,
                  const HardwareBindings &bindings, mlir::Operation *site);
/// The packed layout walk with cycle detection; leaves return in packed
/// order (field zero most significant last).
mlir::FailureOr<llvm::SmallVector<PackedLeaf>>
collectPackedLeaves(const PackedWidthResolver &resolver, mlir::Type type,
                    const HardwareBindings &bindings, mlir::Operation *site);
mlir::FailureOr<uint64_t>
sumPackedLeaves(mlir::Operation *site, llvm::ArrayRef<PackedLeaf> leaves);
mlir::FailureOr<uint64_t>
packedHardwareWidth(const PackedWidthResolver &resolver, mlir::Type type,
                    const HardwareBindings &bindings, mlir::Operation *site);

} // namespace detail
} // namespace acir::ac
#endif
