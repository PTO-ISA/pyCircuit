#include "Compiler/ScalarNumericLowering.h"
#include "Compiler/SourceUnit.h"
#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Support/FileUtilities.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ToolOutputFile.h"

#include <memory>

namespace {

llvm::cl::opt<std::string> capturePath("capture", llvm::cl::Required);
llvm::cl::opt<std::string> packageName("package", llvm::cl::init(""));
llvm::cl::opt<std::string> sourcePath("path", llvm::cl::Required);
llvm::cl::list<std::string> headerPaths("header", llvm::cl::ZeroOrMore);
llvm::cl::opt<std::string> bodyOutput("body-out", llvm::cl::Required);
llvm::cl::opt<std::string> interfaceOutput("interface-out", llvm::cl::Required);
llvm::cl::opt<bool> lowerNumeric("lower-numeric", llvm::cl::init(false));

mlir::OwningOpRef<mlir::ModuleOp> parseModule(llvm::StringRef path,
                                              mlir::MLIRContext &context) {
  return mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
}

bool writeModule(mlir::ModuleOp module, llvm::StringRef path) {
  std::error_code error;
  llvm::ToolOutputFile output(path, error, llvm::sys::fs::OF_Text);
  if (error) {
    llvm::errs() << error.message() << '\n';
    return false;
  }
  module.print(output.os(), mlir::OpPrintingFlags().enableDebugInfo());
  output.os() << '\n';
  output.keep();
  return true;
}

} // namespace

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv,
                                    "isolated pyCircuit source-unit harness\n");
  mlir::DialectRegistry dialects;
  dialects.insert<acir::ac::ACIRDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect>();
  mlir::MLIRContext context(dialects);
  context.loadAllAvailableDialects();

  auto capture = parseModule(capturePath, context);
  if (!capture)
    return 1;
  llvm::SmallVector<mlir::OwningOpRef<mlir::ModuleOp>> ownedHeaders;
  llvm::SmallVector<mlir::ModuleOp> headers;
  for (const std::string &path : headerPaths) {
    auto header = parseModule(path, context);
    if (!header)
      return 1;
    headers.push_back(*header);
    ownedHeaders.push_back(std::move(header));
  }
  auto emitError = [&] {
    return mlir::emitError(mlir::UnknownLoc::get(&context));
  };
  auto registry =
      acir::compiler::SourceHeaderRegistry::create(headers, emitError);
  if (mlir::failed(registry))
    return 1;
  mlir::Builder builder(&context);
  auto owner = builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr(packageName)),
      builder.getNamedAttr("path", builder.getStringAttr(sourcePath)),
  });
  auto result = acir::compiler::compilePythonSourceUnit(*capture, owner,
                                                        *registry, emitError);
  if (mlir::failed(result) || mlir::failed(mlir::verify(*result->body)) ||
      mlir::failed(mlir::verify(*result->interface)))
    return 1;
  if (lowerNumeric &&
      mlir::failed(
          acir::compiler::lowerExactInputAddTransactional(*result->body)))
    return 1;
  if (!writeModule(*result->body, bodyOutput) ||
      !writeModule(*result->interface, interfaceOutput))
    return 1;
  return 0;
}
