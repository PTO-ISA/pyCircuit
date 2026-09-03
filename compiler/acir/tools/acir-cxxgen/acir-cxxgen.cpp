#include "BuildInternal.h"

#include "acir/CodeGen/Build.h"
#include "acir/CodeGen/Generator.h"
#include "acir/CodeGen/ModelPlan.h"
#include "acir/Dialect/ACSim/ACSimDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Index/IR/IndexDialect.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <array>
#include <exception>
#include <optional>
#include <set>
#include <string>
#include <system_error>
#include <vector>

namespace {

llvm::cl::opt<std::string> inputFile(llvm::cl::Positional, llvm::cl::Required,
                                     llvm::cl::desc("<canonical-acsim>"));
llvm::cl::opt<std::string> stopAfter("stop-after", llvm::cl::Required);
llvm::cl::opt<std::string> outputRoot("output-root");
llvm::cl::opt<std::string> frozenAcir("frozen-acir");
llvm::cl::opt<std::string> bindingLock("binding-lock");
llvm::cl::opt<std::string> projectName("project-name");
llvm::cl::opt<std::string> projectIdentity("project-identity");
llvm::cl::opt<std::string> systemName("system-name");
llvm::cl::opt<std::string> systemIdentity("system-identity");
llvm::cl::opt<std::string> profile("profile");
llvm::cl::opt<std::string> compiler("compiler");
llvm::cl::opt<std::string> standardLibrary("standard-library");
llvm::cl::opt<std::string> abiMode("abi-mode");
llvm::cl::opt<std::string> objectFormat("object-format");
llvm::cl::list<std::string> contractFlags("contract-flag");
llvm::cl::list<std::string> includeRoots("include-root");
llvm::cl::list<std::string> definitions("definition");
llvm::cl::list<std::string> compilerFlags("compiler-flag");
llvm::cl::list<std::string> linkerFlags("linker-flag");
llvm::cl::list<std::string> linkInputs("link-input");
llvm::cl::list<std::string> providerInputs("provider-input");
llvm::cl::list<std::string> instrumentation("instrumentation");

constexpr std::array<llvm::StringLiteral, 6> stages = {
    "model-plan", "acsim-emit-cxx", "acsim-check-cxx-contract",
    "compile",    "link",           "publish"};

int fail(llvm::StringRef stage, llvm::Error error) {
  llvm::errs() << "stage=" << stage
               << " status=failed error=" << llvm::toString(std::move(error))
               << '\n';
  return EXIT_FAILURE;
}

llvm::Error driverError(llvm::StringRef message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      "ACLOWER-FINGERPRINT: " + message);
}

llvm::Expected<std::string> readFile(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  if (!buffer)
    return llvm::createStringError(buffer.getError(), "cannot read input file");
  return buffer.get()->getBuffer().str();
}

llvm::Error requireOutputRoot() {
  if (outputRoot.empty())
    return driverError("--output-root is required for this stage");
  if (std::error_code error = llvm::sys::fs::create_directories(outputRoot))
    return llvm::createStringError(error, "cannot create output root");
  return llvm::Error::success();
}

llvm::Error writeBundle(const acir::codegen::SourceBundle &bundle) {
  if (auto error = requireOutputRoot())
    return error;
  for (const acir::codegen::GeneratedFile &file : bundle.files)
    if (auto error = acir::codegen::writeFileExclusive(
            outputRoot, file.relativePath, file.content))
      return error;
  return llvm::Error::success();
}

llvm::Expected<acir::codegen::BuildRequest> makeCompileRequest() {
  if (compiler.empty() || projectName.empty() || projectIdentity.empty() ||
      systemName.empty() || systemIdentity.empty() || profile.empty() ||
      standardLibrary.empty() || abiMode.empty() || objectFormat.empty() ||
      contractFlags.empty())
    return driverError(
        "compile stages require explicit identity and toolchain flags");
  auto toolchain = acir::codegen::identifyToolchain(
      compiler, standardLibrary, abiMode, objectFormat,
      std::vector<std::string>(contractFlags.begin(), contractFlags.end()));
  if (!toolchain)
    return toolchain.takeError();
  acir::codegen::BuildRequest request;
  request.project = {projectName, projectIdentity};
  request.system = {systemName, systemIdentity};
  request.profile = profile;
  request.passPipeline = {"acsim-emit-cxx", "acsim-check-cxx-contract",
                          "compile", "link"};
  request.toolchain = std::move(*toolchain);
  request.includeRoots.assign(includeRoots.begin(), includeRoots.end());
  request.definitions.assign(definitions.begin(), definitions.end());
  request.compilerFlags.assign(compilerFlags.begin(), compilerFlags.end());
  request.linkerFlags.assign(linkerFlags.begin(), linkerFlags.end());
  request.linkInputs.assign(linkInputs.begin(), linkInputs.end());
  request.providerInputs.assign(providerInputs.begin(), providerInputs.end());
  request.instrumentationLayers.assign(instrumentation.begin(),
                                       instrumentation.end());
  request.outputRoot = outputRoot;
  return request;
}

std::string resolvePath(llvm::StringRef path) {
  if (llvm::sys::path::is_absolute(path))
    return path.str();
  llvm::SmallString<256> result(outputRoot);
  llvm::sys::path::append(result, path);
  return result.str().str();
}

std::vector<std::string> resolveCommand(const acir::codegen::CompilePlan &plan,
                                        llvm::ArrayRef<std::string> arguments) {
  std::set<std::string> generated(plan.sourceUnits.begin(),
                                  plan.sourceUnits.end());
  generated.insert(plan.objectOutputs.begin(), plan.objectOutputs.end());
  generated.insert(plan.executablePath);
  std::vector<std::string> result;
  for (const std::string &argument : arguments) {
    if (generated.contains(argument)) {
      result.push_back(resolvePath(argument));
      continue;
    }
    llvm::StringRef value(argument);
    if (value.starts_with("-I") &&
        !llvm::sys::path::is_absolute(value.drop_front(2))) {
      result.push_back("-I" + resolvePath(value.drop_front(2)));
      continue;
    }
    result.push_back(argument);
  }
  return result;
}

llvm::Error runCommand(llvm::ArrayRef<std::string> ownedArguments) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  const int status = llvm::sys::ExecuteAndWait(arguments.front(), arguments,
                                               std::nullopt, {}, 120);
  return status == 0 ? llvm::Error::success()
                     : driverError("compiler or linker command failed");
}

llvm::Error compileOrLink(const acir::codegen::SourceBundle &bundle,
                          bool link) {
  auto request = makeCompileRequest();
  if (!request)
    return request.takeError();
  auto plan = acir::codegen::createCompilePlan(*request, bundle);
  if (!plan)
    return plan.takeError();
  if (auto error = writeBundle(bundle))
    return error;
  auto planBytes = plan->canonicalJson();
  if (!planBytes)
    return planBytes.takeError();
  if (auto error = acir::codegen::writeFileExclusive(
          outputRoot, "compile-plan.json", *planBytes))
    return error;
  for (const std::string &output : plan->objectOutputs) {
    llvm::SmallString<256> parent(resolvePath(output));
    llvm::sys::path::remove_filename(parent);
    if (std::error_code error = llvm::sys::fs::create_directories(parent))
      return llvm::createStringError(error, "cannot create object directory");
  }
  for (const acir::codegen::CompileCommand &command : plan->compileCommands)
    if (auto error = runCommand(resolveCommand(*plan, command.arguments)))
      return error;
  if (!link)
    return llvm::Error::success();
  llvm::SmallString<256> executableParent(resolvePath(plan->executablePath));
  llvm::sys::path::remove_filename(executableParent);
  if (std::error_code error =
          llvm::sys::fs::create_directories(executableParent))
    return llvm::createStringError(error, "cannot create executable directory");
  return runCommand(resolveCommand(*plan, plan->linkCommand.arguments));
}

int runDriver() {
  if (std::find(stages.begin(), stages.end(), stopAfter) == stages.end()) {
    llvm::errs() << "unknown --stop-after stage '" << stopAfter << "'\n";
    return EXIT_FAILURE;
  }

  mlir::MLIRContext context;
  context
      .loadDialect<acir::acsim::ACSimDialect, mlir::arith::ArithDialect,
                   mlir::cf::ControlFlowDialect, mlir::index::IndexDialect>();
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(inputFile, &context);
  if (!module)
    return fail(stopAfter, driverError("canonical ACSim parsing failed"));
  auto model = acir::codegen::buildModelPlan(*module);
  if (!model)
    return fail(stopAfter, model.takeError());
  if (stopAfter == "model-plan") {
    llvm::outs() << "stage=model-plan status=passed model="
                 << model->modelSymbol << " modules=" << model->modules.size()
                 << " runtime_objects=" << model->runtimeObjects.size() << '\n';
    return EXIT_SUCCESS;
  }

  auto bundle = acir::codegen::generateModelSources(*model);
  if (!bundle)
    return fail(stopAfter, bundle.takeError());
  if (stopAfter == "acsim-emit-cxx") {
    if (auto error = writeBundle(*bundle))
      return fail(stopAfter, std::move(error));
    llvm::outs() << "stage=acsim-emit-cxx status=passed files="
                 << bundle->files.size() << '\n';
    return EXIT_SUCCESS;
  }
  if (stopAfter == "acsim-check-cxx-contract") {
    if (auto error = writeBundle(*bundle))
      return fail(stopAfter, std::move(error));
    if (auto error = acir::codegen::validateSourceBundle(*model, *bundle))
      return fail(stopAfter, std::move(error));
    if (auto error = acir::codegen::writeFileExclusive(
            outputRoot, "reports/source-contract.txt", "passed\n"))
      return fail(stopAfter, std::move(error));
    llvm::outs() << "stage=acsim-check-cxx-contract status=passed\n";
    return EXIT_SUCCESS;
  }
  if (stopAfter == "compile" || stopAfter == "link") {
    if (auto error = compileOrLink(*bundle, stopAfter == "link"))
      return fail(stopAfter, std::move(error));
    llvm::outs() << "stage=" << stopAfter << " status=passed\n";
    return EXIT_SUCCESS;
  }

  auto request = makeCompileRequest();
  if (!request)
    return fail(stopAfter, request.takeError());
  auto frozen = readFile(frozenAcir);
  if (!frozen)
    return fail(stopAfter, frozen.takeError());
  auto lock = readFile(bindingLock);
  if (!lock)
    return fail(stopAfter, lock.takeError());
  std::string acsimBytes;
  llvm::raw_string_ostream acsimStream(acsimBytes);
  module->print(acsimStream);
  acsimStream.flush();
  request->canonicalACSim = *module;
  request->frozenAcirBytes = std::move(*frozen);
  request->bindingLockBytes = std::move(*lock);
  request->canonicalACSimBytes = std::move(acsimBytes);
  auto result = acir::codegen::buildGeneratedModel(*request);
  if (!result)
    return fail(stopAfter, result.takeError());
  llvm::outs() << "stage=publish status=passed build="
               << result->buildFingerprint << '\n';
  return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char **argv) {
  try {
    llvm::cl::ParseCommandLineOptions(
        argc, argv, "Agentic Circuit internal C++ generator\n");
    return runDriver();
  } catch (const std::exception &exception) {
    llvm::errs() << "ACLOWER-FINGERPRINT: internal driver exception: "
                 << exception.what() << '\n';
    return EXIT_FAILURE;
  }
}
