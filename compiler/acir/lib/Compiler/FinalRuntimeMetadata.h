#ifndef ACIR_LIB_COMPILER_FINALRUNTIMEMETADATA_H
#define ACIR_LIB_COMPILER_FINALRUNTIMEMETADATA_H
#include "SourceUnit.h"
namespace acir::compiler {
mlir::FailureOr<std::string> runtimeMetadataJson(mlir::Attribute value,
                                                 ac::detail::EmitError error);
}
#endif
