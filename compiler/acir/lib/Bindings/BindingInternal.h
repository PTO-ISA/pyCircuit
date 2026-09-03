#ifndef ACIR_LIB_BINDINGS_BINDINGINTERNAL_H
#define ACIR_LIB_BINDINGS_BINDINGINTERNAL_H

#include "acir/Bindings/Binding.h"

namespace acir::bindings::detail {

llvm::Expected<size_t> preflightConstructedJson(const llvm::json::Value &value,
                                                const JsonParseLimits &limits);
llvm::Expected<size_t>
preflightConstructedJson(const llvm::json::Object &object,
                         const JsonParseLimits &limits);

} // namespace acir::bindings::detail

#endif // ACIR_LIB_BINDINGS_BINDINGINTERNAL_H
