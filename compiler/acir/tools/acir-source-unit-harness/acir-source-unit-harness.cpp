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

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

llvm::cl::opt<std::string> capturePath("capture", llvm::cl::Required);
llvm::cl::opt<std::string> packageName("package", llvm::cl::init(""));
llvm::cl::opt<std::string> sourcePath("path", llvm::cl::Required);
llvm::cl::list<std::string> headerPaths("header", llvm::cl::ZeroOrMore);
llvm::cl::opt<std::string> bodyOutput("body-out", llvm::cl::Required);
llvm::cl::opt<std::string> interfaceOutput("interface-out", llvm::cl::Required);
llvm::cl::opt<std::string> depsOutput("deps-out", llvm::cl::init(""));
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

std::string jsonString(llvm::StringRef value) {
  std::string out = "\"";
  for (char raw : value) {
    const unsigned char byte = static_cast<unsigned char>(raw);
    switch (byte) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (byte < 0x20) {
        char buffer[8];
        std::snprintf(buffer, sizeof(buffer), "\\u%04x", byte);
        out += buffer;
      } else {
        out += static_cast<char>(byte);
      }
      break;
    }
  }
  out += "\"";
  return out;
}

// Writes the consumed interface closure as a strict JSON array of
// {"package","path"} objects, excluding the unit's own owner.
bool writeConsumedDependencies(mlir::ModuleOp body,
                               mlir::DictionaryAttr owner,
                               llvm::StringRef path) {
  auto interfaces = body->getAttrOfType<mlir::ArrayAttr>("ac.interfaces");
  auto ownerPackage = owner.getAs<mlir::StringAttr>("package");
  auto ownerPath = owner.getAs<mlir::StringAttr>("path");
  if (!interfaces || !ownerPackage || !ownerPath)
    return false;
  std::vector<std::pair<llvm::StringRef, llvm::StringRef>> consumed;
  for (mlir::Attribute raw : interfaces) {
    auto entry = llvm::dyn_cast<mlir::DictionaryAttr>(raw);
    auto package = entry ? entry.getAs<mlir::StringAttr>("package")
                         : mlir::StringAttr();
    auto sourcePath =
        entry ? entry.getAs<mlir::StringAttr>("path") : mlir::StringAttr();
    if (!package || !sourcePath)
      return false;
    if (package.getValue() == ownerPackage.getValue() &&
        sourcePath.getValue() == ownerPath.getValue())
      continue;
    consumed.emplace_back(package.getValue(), sourcePath.getValue());
  }
  llvm::sort(consumed);
  std::string text = "[";
  for (size_t index = 0; index < consumed.size(); ++index) {
    if (index)
      text += ",";
    text += "{\"package\":" + jsonString(consumed[index].first) +
            ",\"path\":" + jsonString(consumed[index].second) + "}";
  }
  text += "]\n";
  std::error_code error;
  llvm::ToolOutputFile output(path, error, llvm::sys::fs::OF_Text);
  if (error) {
    llvm::errs() << error.message() << '\n';
    return false;
  }
  output.os() << text;
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
  if ((*result->body).getOperation()->getAttr("ac.source_owner") != owner ||
      (*result->interface).getOperation()->getAttr("ac.source_owner") != owner) {
    llvm::errs() << "compiled source unit does not carry the requested owner\n";
    return 1;
  }
  if (!depsOutput.empty() &&
      !writeConsumedDependencies(*result->body, owner, depsOutput))
    return 1;
  if (!writeModule(*result->body, bodyOutput) ||
      !writeModule(*result->interface, interfaceOutput))
    return 1;
  return 0;
}
