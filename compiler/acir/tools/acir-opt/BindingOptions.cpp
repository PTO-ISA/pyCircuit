#include "BindingOptions.h"

#include "acir/Bindings/Registry.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/MemoryBuffer.h"

#include <string>
#include <vector>

namespace acir::opt {
namespace {

llvm::cl::OptionCategory BindingCategory("Exact binding resolution options");

llvm::cl::opt<bool> ResolveBindings(
    "ac-resolve-gfsim-bindings",
    llvm::cl::desc("Run exact frozen-ACIR gfsim binding resolution"),
    llvm::cl::init(false), llvm::cl::cat(BindingCategory));

llvm::cl::opt<bool>
    LowerToACSim("ac-lower-to-acsim",
                 llvm::cl::desc("Atomically lower the frozen ACIR model to "
                                "canonical ACSim"),
                 llvm::cl::init(false), llvm::cl::cat(BindingCategory));

llvm::cl::list<std::string> BindingRegistries(
    "ac-binding-registry",
    llvm::cl::desc(
        "Closed binding candidate/request registry JSON file (repeatable)"),
    llvm::cl::ZeroOrMore, llvm::cl::value_desc("file"),
    llvm::cl::cat(BindingCategory));

llvm::cl::opt<std::string> BindingLockOutput(
    "ac-binding-lock-output",
    llvm::cl::desc("Required atomic output path for acsim-bindings.lock.json"),
    llvm::cl::value_desc("file"), llvm::cl::init(""),
    llvm::cl::cat(BindingCategory));

llvm::cl::opt<std::string>
    BindingProfile("ac-binding-profile",
                   llvm::cl::desc("Exact static build profile identity"),
                   llvm::cl::value_desc("profile"), llvm::cl::init(""),
                   llvm::cl::cat(BindingCategory));

llvm::cl::opt<std::string>
    BindingTarget("ac-binding-target",
                  llvm::cl::desc("Exact toolchain target identity"),
                  llvm::cl::value_desc("target"), llvm::cl::init(""),
                  llvm::cl::cat(BindingCategory));

llvm::Error optionError(const llvm::Twine &message) {
  return llvm::createStringError(llvm::errc::invalid_argument,
                                 "ACLOWER-BINDING-OPTIONS: %s",
                                 message.str().c_str());
}

bool hasRelatedOption() {
  return !BindingRegistries.empty() || !BindingLockOutput.empty() ||
         !BindingProfile.empty() || !BindingTarget.empty();
}

struct RegistryInputs {
  std::vector<bindings::BindingCandidate> candidates;
  std::vector<bindings::BindingRequest> requests;
};

llvm::Expected<RegistryInputs> loadRegistryInputs() {
  std::vector<std::string> registryPaths(BindingRegistries.begin(),
                                         BindingRegistries.end());
  llvm::sort(registryPaths);
  RegistryInputs inputs;
  for (const std::string &path : registryPaths) {
    auto buffer = llvm::MemoryBuffer::getFile(path, false, false);
    if (!buffer)
      return optionError(llvm::Twine("cannot read registry '") + path +
                         "': " + buffer.getError().message());
    auto document = bindings::parseBindingRegistry((*buffer)->getBuffer());
    if (!document)
      return document.takeError();
    inputs.candidates.insert(
        inputs.candidates.end(),
        std::make_move_iterator(document->candidates.begin()),
        std::make_move_iterator(document->candidates.end()));
    inputs.requests.insert(inputs.requests.end(),
                           std::make_move_iterator(document->requests.begin()),
                           std::make_move_iterator(document->requests.end()));
  }
  return inputs;
}

} // namespace

llvm::Expected<std::optional<ResolveBindingsPassOptions>>
loadBindingCommandLineOptions() {
  if (!ResolveBindings) {
    if (hasRelatedOption() && !LowerToACSim)
      return optionError("binding options require --ac-resolve-gfsim-bindings "
                         "or --ac-lower-to-acsim");
    return std::optional<ResolveBindingsPassOptions>();
  }
  if (BindingLockOutput.empty())
    return optionError("--ac-binding-lock-output is required");
  if (BindingProfile.empty())
    return optionError("--ac-binding-profile is required");
  if (BindingTarget.empty())
    return optionError("--ac-binding-target is required");

  auto inputs = loadRegistryInputs();
  if (!inputs)
    return inputs.takeError();
  ResolveBindingsPassOptions options;
  options.profile = BindingProfile;
  options.target = BindingTarget;
  options.lockOutputPath = BindingLockOutput;
  options.candidates = std::move(inputs->candidates);
  options.requests = std::move(inputs->requests);
  return std::optional<ResolveBindingsPassOptions>(std::move(options));
}

llvm::Expected<std::optional<ACIRToACSimPassOptions>>
loadLoweringCommandLineOptions() {
  if (!LowerToACSim)
    return std::optional<ACIRToACSimPassOptions>();
  if (BindingProfile.empty())
    return optionError("--ac-lower-to-acsim requires --ac-binding-profile");
  if (BindingTarget.empty())
    return optionError("--ac-lower-to-acsim requires --ac-binding-target");

  auto inputs = loadRegistryInputs();
  if (!inputs)
    return inputs.takeError();
  ACIRToACSimPassOptions options;
  options.profile = BindingProfile;
  options.target = BindingTarget;
  options.candidates = std::move(inputs->candidates);
  options.requests = std::move(inputs->requests);
  return std::optional<ACIRToACSimPassOptions>(std::move(options));
}

std::string selectedBindingProfile() { return BindingProfile; }

std::string selectedBindingTarget() { return BindingTarget; }

} // namespace acir::opt
