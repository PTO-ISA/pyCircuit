#include "BuildInternal.h"

#include "acir/Bindings/Binding.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <system_error>

namespace acir::codegen {
namespace {

llvm::Error stageError(const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      "ACLOWER-FINGERPRINT: " + message);
}

llvm::SmallString<256> joined(llvm::StringRef root, llvm::StringRef relative) {
  llvm::SmallString<256> result(root);
  llvm::sys::path::append(result, relative);
  return result;
}

llvm::Expected<Fingerprint> fileFingerprint(llvm::StringRef path) {
  auto bytes = readFileBytes(path);
  if (!bytes)
    return bytes.takeError();
  return computeFingerprint(*bytes);
}

} // namespace

llvm::Expected<std::string> normalizeArtifactPath(llvm::StringRef path) {
  const llvm::StringRef original = path;
  if (path.empty() || llvm::sys::path::is_absolute(path) ||
      llvm::sys::path::has_root_name(path) || path.ends_with('/') ||
      path.contains('\\') || path.contains('\0'))
    return stageError("artifact path must be normalized and relative");
  llvm::SmallString<256> normalized;
  while (!path.empty()) {
    auto [component, remainder] = path.split('/');
    if (component.empty() || component == "." || component == "..")
      return stageError("artifact path escapes its private stage");
    llvm::sys::path::append(normalized, component);
    path = remainder;
  }
  if (normalized.empty() || normalized != original)
    return stageError("artifact path is not canonical");
  return normalized.str().str();
}

llvm::Expected<std::string> readFileBytes(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path, false, false);
  if (!buffer)
    return llvm::createStringError(buffer.getError(), "cannot read build file");
  return buffer.get()->getBuffer().str();
}

llvm::Error writeFileExclusive(llvm::StringRef stageRoot,
                               llvm::StringRef relativePath,
                               llvm::StringRef bytes) {
  auto normalized = normalizeArtifactPath(relativePath);
  if (!normalized)
    return normalized.takeError();
  llvm::SmallString<256> destination = joined(stageRoot, *normalized);
  llvm::SmallString<256> parent(destination);
  llvm::sys::path::remove_filename(parent);
  if (std::error_code error = llvm::sys::fs::create_directories(parent))
    return llvm::createStringError(error, "cannot create staged directory");

  int descriptor = -1;
  if (std::error_code error = llvm::sys::fs::openFileForWrite(
          destination, descriptor, llvm::sys::fs::CD_CreateNew,
          llvm::sys::fs::OF_None))
    return llvm::createStringError(error, "cannot create staged file");
  {
    llvm::raw_fd_ostream output(descriptor, true);
    output.write(bytes.data(), bytes.size());
    output.flush();
    if (output.has_error())
      return stageError("staged file write failed");
  }
  auto stored = readFileBytes(destination);
  if (!stored)
    return stored.takeError();
  if (stored->size() != bytes.size() ||
      computeFingerprint(*stored) != computeFingerprint(bytes))
    return stageError("staged file verification failed");
  return llvm::Error::success();
}

llvm::Expected<PublishedStage>
publishImmutableStage(llvm::StringRef stageRoot, llvm::StringRef outputRoot,
                      llvm::StringRef buildFingerprint,
                      llvm::ArrayRef<Artifact> artifacts,
                      llvm::StringRef manifestBytes) {
  if (!isValidFingerprint(buildFingerprint))
    return stageError("immutable build fingerprint is invalid");
  llvm::SmallString<256> builds(outputRoot);
  llvm::sys::path::append(builds, "builds");
  if (std::error_code error = llvm::sys::fs::create_directories(builds))
    return llvm::createStringError(error, "cannot create immutable build root");
  llvm::SmallString<256> destination(builds);
  llvm::sys::path::append(destination, buildFingerprint);

  if (llvm::sys::fs::exists(destination)) {
    auto existingManifest =
        readFileBytes(joined(destination, "build-manifest.json"));
    if (!existingManifest || *existingManifest != manifestBytes)
      return stageError("immutable build manifest differs for one fingerprint");
    for (const Artifact &artifact : artifacts) {
      auto normalized = normalizeArtifactPath(artifact.path);
      if (!normalized)
        return normalized.takeError();
      auto existing = fileFingerprint(joined(destination, *normalized));
      if (!existing || *existing != artifact.sha256)
        return stageError(
            "immutable build artifact differs for one fingerprint");
    }
    if (std::error_code error = llvm::sys::fs::remove_directories(stageRoot))
      return llvm::createStringError(error,
                                     "cannot clean exact cache-hit stage");
    return PublishedStage{destination.str().str(), true};
  }

  std::error_code error;
  std::filesystem::rename(std::filesystem::path(stageRoot.str()),
                          std::filesystem::path(destination.str().str()),
                          error);
  if (error)
    return llvm::createStringError(error, "cannot publish immutable build");
  return PublishedStage{destination.str().str(), false};
}

llvm::Error writeCurrentPointer(llvm::StringRef outputRoot,
                                llvm::StringRef buildFingerprint) {
  llvm::json::Object pointer{
      {"build_fingerprint", buildFingerprint},
      {"path", (llvm::Twine("builds/") + buildFingerprint).str()}};
  auto bytes =
      bindings::canonicalizeJson(llvm::json::Value(std::move(pointer)));
  if (!bytes)
    return bytes.takeError();

  llvm::SmallString<256> temporary;
  llvm::SmallString<256> prefix(outputRoot);
  llvm::sys::path::append(prefix, ".current");
  int descriptor = -1;
  if (std::error_code error = llvm::sys::fs::createUniqueFile(
          llvm::Twine(prefix) + "-%%%%%%.json", descriptor, temporary))
    return llvm::createStringError(error,
                                   "cannot create current pointer stage");
  {
    llvm::raw_fd_ostream output(descriptor, true);
    output << *bytes;
    output.flush();
    if (output.has_error()) {
      llvm::sys::fs::remove(temporary);
      return stageError("cannot flush current pointer");
    }
  }
  llvm::SmallString<256> destination(outputRoot);
  llvm::sys::path::append(destination, "current.json");
  std::error_code error;
  std::filesystem::rename(std::filesystem::path(temporary.str().str()),
                          std::filesystem::path(destination.str().str()),
                          error);
  if (error) {
    llvm::sys::fs::remove(temporary);
    return llvm::createStringError(error, "cannot atomically select build");
  }
  return llvm::Error::success();
}

} // namespace acir::codegen
