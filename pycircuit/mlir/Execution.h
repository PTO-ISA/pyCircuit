#pragma once
#include "mlir/IR/BuiltinOps.h"
#include "llvm/Support/JSON.h"
#include <map>
#include <string>
namespace acir {
mlir::Attribute evaluateFunction(mlir::ModuleOp module, llvm::StringRef name,
                                mlir::ArrayAttr arguments);
mlir::Attribute zero(mlir::Type type, mlir::ModuleOp module);
mlir::Attribute calculate(mlir::Operation *op, llvm::ArrayRef<mlir::Attribute> args,
                          mlir::ModuleOp module);
llvm::json::Value jsonAttribute(mlir::Attribute value);
llvm::json::Object execute(mlir::ModuleOp module, unsigned cycles);
}
