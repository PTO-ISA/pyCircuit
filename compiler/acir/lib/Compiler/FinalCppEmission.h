#ifndef ACIR_LIB_COMPILER_FINALCPPEMISSION_H
#define ACIR_LIB_COMPILER_FINALCPPEMISSION_H

#include "FinalEmitCppHierarchy.h"
#include "FinalProgram.h"
#include "llvm/ADT/DenseMap.h"

#include <string>

namespace acir::compiler {

struct CppSpecNames {
  mlir::DictionaryAttr sourceOwner;
  std::string familyName;
  std::string methodTypeName;
  std::string qualifiedTypeName;
  std::string rawNameSpace;
  std::string headerPath;
  std::string sourcePath;
  std::string nameSpace;
  llvm::SmallVector<std::string> stateNames;
  llvm::DenseMap<unsigned, std::string> inputNames;
  llvm::SmallVector<std::string> childNames;
};

struct CppSourceOwnerNames {
  mlir::DictionaryAttr sourceOwner;
  std::string headerPath;
  std::string sourcePath;
  llvm::SmallVector<size_t> definitions;
  llvm::SmallVector<std::string> childHeaders;
};

struct CppEmissionNames {
  bool sourceOwned = false;
  llvm::SmallVector<CppSpecNames, 0> specs;
  llvm::SmallVector<size_t> defByInstance;
  llvm::SmallVector<CppSourceOwnerNames> sourceGroups;

  llvm::StringRef methodType(size_t definition) const;
  llvm::StringRef qualifiedType(size_t definition) const;
  llvm::StringRef family(size_t definition) const;
  std::string input(size_t definition, unsigned portIndex) const;
  std::string inputParameter(size_t definition, unsigned portIndex) const;
  std::string child(size_t definition, size_t childPosition) const;
  std::string state(size_t definition, llvm::StringRef role,
                    size_t localState) const;
};

struct CppDefinitionEmission {
  std::string declaration;
  std::string constructor;
  std::string methods;
};

struct CppEmissionPlan {
  CppEmissionNames names;
  size_t rootDefinition = 0;
  std::string support;
  std::string legacyForwardDeclarations;
  llvm::SmallVector<CppDefinitionEmission> definitions;
  llvm::SmallVector<size_t> definitionPostOrder;
  std::string system;
};

mlir::FailureOr<CppEmissionNames>
buildCppEmissionNames(const FinalProgram &program,
                      llvm::ArrayRef<SpecGroup> groups,
                      llvm::ArrayRef<size_t> defByInstance, bool sourceOwned,
                      ac::detail::EmitError emitError);

mlir::FailureOr<CppEmissionPlan>
buildCppEmissionPlan(const FinalProgram &program, bool sourceOwned,
                     ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALCPPEMISSION_H
