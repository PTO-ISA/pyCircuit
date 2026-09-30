#ifndef ACIR_LIB_COMPILER_FINALEMITCPPHIERARCHY_H
#define ACIR_LIB_COMPILER_FINALEMITCPPHIERARCHY_H

#include "FinalProgram.h"
#include "llvm/ADT/DenseMap.h"

namespace acir::compiler {

struct CppEmissionNames;

struct SpecGroup {
  mlir::FlatSymbolRefAttr definition;
  mlir::ArrayAttr staticArguments;
  std::string orderKey;
  llvm::SmallVector<size_t> instances;
};

struct InputPort {
  size_t portIndex = 0;
  size_t inputIndex = 0;
  unsigned width = 0;
  mlir::DictionaryAttr formalState;
  mlir::DictionaryAttr stateID;
};

struct ChildReadBinding {
  std::string constructorExpression;
  std::string freezeExpression;
};

bool sameFormal(mlir::DictionaryAttr left, mlir::DictionaryAttr right);
mlir::FailureOr<std::string>
actualReadBindingKey(const FinalProgram &program, size_t instanceOrdinal,
                     mlir::Value handle, ac::detail::EmitError emitError);
mlir::FailureOr<std::string>
frozenPlacementName(const FinalProgram::FinalInstanceSnapshot &instance,
                    ac::detail::EmitError emitError);
std::string cppStringLiteral(llvm::StringRef value);
mlir::LogicalResult verifySpecGroupLayouts(const FinalProgram &program,
                                           llvm::ArrayRef<SpecGroup> groups,
                                           llvm::ArrayRef<size_t> defByInstance,
                                           ac::detail::EmitError emitError);
mlir::FailureOr<ChildReadBinding>
childReadBinding(const FinalProgram &program, size_t parentOrdinal,
                 size_t childPosition, size_t childOrdinal,
                 const InputPort &childInput, mlir::Value actualHandle,
                 llvm::ArrayRef<InputPort> parentInputs,
                 llvm::DenseMap<mlir::Attribute, size_t> parentOwned,
                 const CppEmissionNames &names, size_t parentDefinition,
                 ac::detail::EmitError emitError);
mlir::FailureOr<llvm::SmallVector<ChildReadBinding>>
childReadBindings(const FinalProgram &program, size_t parentOrdinal,
                  size_t childPosition, size_t childOrdinal,
                  llvm::ArrayRef<InputPort> childInputs,
                  llvm::ArrayRef<mlir::Value> actualHandles,
                  llvm::ArrayRef<InputPort> parentInputs,
                  llvm::DenseMap<mlir::Attribute, size_t> parentOwned,
                  const CppEmissionNames &names, size_t parentDefinition,
                  ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALEMITCPPHIERARCHY_H
