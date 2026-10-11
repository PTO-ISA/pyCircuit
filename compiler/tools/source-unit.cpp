#include "Compiler/SourceUnit.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Support/FileUtilities.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
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
llvm::cl::opt<std::string> sourceImportOutput(
    "source-import-out", llvm::cl::init(""),
    llvm::cl::desc("Retain reproduction post-Lower, pre-Simplify IR text"));

void printNonfatalDiagnostic(const mlir::Diagnostic &diagnostic) {
  llvm::errs() << diagnostic.getLocation() << ": ";
  switch (diagnostic.getSeverity()) {
  case mlir::DiagnosticSeverity::Warning:
    llvm::errs() << "warning: ";
    break;
  case mlir::DiagnosticSeverity::Remark:
    llvm::errs() << "remark: ";
    break;
  default:
    llvm::errs() << "note: ";
    break;
  }
  diagnostic.print(llvm::errs());
  llvm::errs() << '\n';
  for (const mlir::Diagnostic &note : diagnostic.getNotes())
    printNonfatalDiagnostic(note);
}

mlir::OwningOpRef<mlir::ModuleOp> parseModule(llvm::StringRef path,
                                              mlir::MLIRContext &context) {
  return mlir::parseSourceFile<mlir::ModuleOp>(path, &context);
}

bool closeOutput(llvm::ToolOutputFile &output) {
  output.os().flush();
  // Existing ordinary outputs may use stdout; it is not owned by this file.
  if (output.getFilename() != "-")
    output.os().close();
  if (!output.os().has_error())
    return true;
  llvm::errs() << output.getFilename() << ": " << output.os().error().message()
               << '\n';
  output.os().clear_error();
  return false;
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
  if (!closeOutput(output))
    return false;
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
  if (!closeOutput(output))
    return false;
  output.keep();
  return true;
}

std::unique_ptr<llvm::ToolOutputFile>
openSourceImport(llvm::ArrayRef<llvm::StringRef> otherPaths) {
  if (sourceImportOutput.empty() || sourceImportOutput == "-") {
    llvm::errs() << "source-import-out requires a fresh ordinary file path\n";
    return nullptr;
  }
  auto normalized = [](llvm::StringRef path,
                       llvm::SmallVectorImpl<char> &absolute) {
    absolute.assign(path.begin(), path.end());
    if (auto error = llvm::sys::fs::make_absolute(absolute)) {
      llvm::errs() << path << ": " << error.message() << '\n';
      return false;
    }
    llvm::sys::path::remove_dots(absolute, true);
    return true;
  };
  llvm::SmallString<256> destination;
  if (!normalized(sourceImportOutput, destination))
    return nullptr;
  for (llvm::StringRef path : otherPaths) {
    llvm::SmallString<256> other;
    if (!normalized(path, other))
      return nullptr;
    if (destination == other) {
      llvm::errs() << "source-import-out collides with " << path << '\n';
      return nullptr;
    }
  }
  int descriptor = -1;
  if (auto error = llvm::sys::fs::openFileForWrite(
          sourceImportOutput, descriptor, llvm::sys::fs::CD_CreateNew,
          llvm::sys::fs::OF_Text)) {
    llvm::errs() << sourceImportOutput << ": " << error.message() << '\n';
    return nullptr;
  }
  auto output =
      std::make_unique<llvm::ToolOutputFile>(sourceImportOutput, descriptor);
  llvm::sys::fs::file_status destinationStatus;
  if (auto error = llvm::sys::fs::status(descriptor, destinationStatus)) {
    llvm::errs() << sourceImportOutput << ": " << error.message() << '\n';
    return nullptr;
  }
  if (!llvm::sys::fs::is_regular_file(destinationStatus)) {
    llvm::errs() << "source-import-out requires an ordinary file\n";
    return nullptr;
  }
  // Creation can make a formerly dangling output symlink resolve. Check real
  // identities now, before any ordinary output can truncate this diagnostic.
  for (llvm::StringRef path : otherPaths) {
    llvm::sys::fs::file_status otherStatus;
    auto error = llvm::sys::fs::status(path, otherStatus);
    if (error == std::errc::no_such_file_or_directory)
      continue;
    if (error) {
      llvm::errs() << path << ": " << error.message() << '\n';
      return nullptr;
    }
    if (otherStatus.type() == llvm::sys::fs::file_type::file_not_found)
      continue;
    if (llvm::sys::fs::equivalent(destinationStatus, otherStatus)) {
      llvm::errs() << "source-import-out aliases " << path << '\n';
      return nullptr;
    }
  }
  return output;
}

} // namespace

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv,
                                    "isolated pyCircuit source-unit harness\n");
  mlir::DialectRegistry dialects;
  dialects.insert<acir::ac::ACIRDialect, mlir::arith::ArithDialect,
                  mlir::func::FuncDialect>();
  mlir::MLIRContext context(dialects);
  mlir::ScopedDiagnosticHandler nonfatalDiagnostics(
      &context, [](mlir::Diagnostic &diagnostic) {
        if (diagnostic.getSeverity() == mlir::DiagnosticSeverity::Error)
          return mlir::failure();
        printNonfatalDiagnostic(diagnostic);
        return mlir::success();
      });
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
      acir::compiler::SourceHeaderRegistry::create(&context, headers, emitError);
  if (mlir::failed(registry))
    return 1;
  mlir::Builder builder(&context);
  auto owner = builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr(packageName)),
      builder.getNamedAttr("path", builder.getStringAttr(sourcePath)),
  });
  bool retainSourceImport = sourceImportOutput.getNumOccurrences() != 0;
  auto result = acir::compiler::compilePythonSourceUnit(
      *capture, owner, *registry, emitError, retainSourceImport);
  if (mlir::failed(result) || mlir::failed(mlir::verify(*result->body)) ||
      mlir::failed(mlir::verify(*result->interface)))
    return 1;
  if ((*result->body).getOperation()->getAttr("ac.source_owner") != owner ||
      (*result->interface).getOperation()->getAttr("ac.source_owner") != owner) {
    llvm::errs() << "compiled source unit does not carry the requested owner\n";
    return 1;
  }
  std::unique_ptr<llvm::ToolOutputFile> sourceImportFile;
  if (retainSourceImport) {
    if (!result->sourceImport) {
      llvm::errs() << "compiled source unit is missing source-import text\n";
      return 1;
    }
    llvm::SmallVector<llvm::StringRef> otherPaths{capturePath, bodyOutput,
                                                  interfaceOutput};
    for (const std::string &path : headerPaths)
      otherPaths.push_back(path);
    if (!depsOutput.empty())
      otherPaths.push_back(depsOutput);
    sourceImportFile = openSourceImport(otherPaths);
    if (!sourceImportFile)
      return 1;
    sourceImportFile->os() << *result->sourceImport;
    if (!closeOutput(*sourceImportFile))
      return 1;
  }
  if (!depsOutput.empty() &&
      !writeConsumedDependencies(*result->body, owner, depsOutput))
    return 1;
  if (!writeModule(*result->body, bodyOutput) ||
      !writeModule(*result->interface, interfaceOutput))
    return 1;
  if (sourceImportFile)
    sourceImportFile->keep();
  return 0;
}
