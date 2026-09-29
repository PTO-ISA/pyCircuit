#ifndef ACIR_LIB_DIALECT_ACIR_ACIRHARDWARECLOSUREDETAIL_H
#define ACIR_LIB_DIALECT_ACIR_ACIRHARDWARECLOSUREDETAIL_H

#include "ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

#include <memory>

namespace acir::ac::hardware_detail {

struct Definition {
  acir::ac::ModuleOp module;
  mlir::DictionaryAttr key;
};

struct View {
  mlir::DictionaryAttr owner;
  mlir::DictionaryAttr key;
  acir::ac::ModuleOp module;
  InstanceOp placement;
  View *parent = nullptr;
  llvm::SmallVector<View *> children;
  llvm::DenseMap<mlir::Attribute, mlir::DictionaryAttr> states;
  llvm::SmallVector<mlir::DictionaryAttr> portStates;
};

struct Closure {
  mlir::ModuleOp package;
  mlir::Operation *system = nullptr;
  mlir::DictionaryAttr entry;
  mlir::ArrayAttr rows;
  llvm::DenseMap<mlir::Attribute, Definition> definitions;
  llvm::SmallVector<std::unique_ptr<View>> storage;
  llvm::SmallVector<View *> views;
  llvm::DenseSet<mlir::Attribute> activeDefinitions;
  llvm::DenseSet<mlir::Attribute> reachableDefinitions;
};

mlir::LogicalResult inspectEnvelope(Closure &closure,
                                    ac::detail::EmitError emitError);
mlir::LogicalResult rebuildInstances(Closure &closure,
                                     ac::detail::EmitError emitError);
mlir::LogicalResult verifyInstanceRows(Closure &closure,
                                       ac::detail::EmitError emitError);
mlir::LogicalResult verifyCommits(Closure &closure,
                                  ac::detail::EmitError emitError);

mlir::FailureOr<mlir::DictionaryAttr> specKey(mlir::Attribute raw,
                                              mlir::Operation *owner);
mlir::DictionaryAttr ownedRef(mlir::Builder &builder, RegOp reg);
mlir::DictionaryAttr formalRef(mlir::Builder &builder,
                               mlir::DictionaryAttr port);
mlir::DictionaryAttr stateID(mlir::Builder &builder, mlir::DictionaryAttr owner,
                             mlir::DictionaryAttr state);
mlir::FailureOr<mlir::DictionaryAttr>
resolveHandle(View &view, mlir::Value handle, ac::detail::EmitError emitError);

} // namespace acir::ac::hardware_detail

#endif // ACIR_LIB_DIALECT_ACIR_ACIRHARDWARECLOSUREDETAIL_H
