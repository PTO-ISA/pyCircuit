#include "CompilerInternal.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Errc.h"

#include <iterator>

namespace acir::compiler::detail {

llvm::Expected<std::vector<CompilerStage>>
selectPipeline(const CompilerRequest &request) {
  if (request.profile == CompilerProfile::Custom &&
      (!request.customPipeline || request.customPipeline->empty()))
    return llvm::createStringError(llvm::errc::invalid_argument,
                                   "custom profile requires a pipeline");
  if (request.profile != CompilerProfile::Custom && request.customPipeline)
    return llvm::createStringError(
        llvm::errc::invalid_argument,
        "custom pipeline is legal only for the custom profile");

  std::vector<CompilerStage> stages{
      CompilerStage::AcirParse,     CompilerStage::AcirVerify,
      CompilerStage::AcirNormalize, CompilerStage::AcirFreeze,
      CompilerStage::AcsimLower,    CompilerStage::AcsimVerify,
      CompilerStage::CxxEmit,       CompilerStage::CxxContract,
      CompilerStage::Compile,       CompilerStage::Link,
      CompilerStage::Publish,
  };
  if (!request.stopAfter)
    return stages;
  auto found = llvm::find(stages, *request.stopAfter);
  if (found == stages.end())
    return llvm::createStringError(llvm::errc::invalid_argument,
                                   "stop stage is outside the pipeline");
  stages.erase(std::next(found), stages.end());
  return stages;
}

} // namespace acir::compiler::detail
