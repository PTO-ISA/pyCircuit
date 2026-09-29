#include "BackendPublication.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
namespace acir::testing {
bool publishAtomically(llvm::MutableArrayRef<Publication> publications) {
  llvm::DenseSet<llvm::StringRef> paths;
  auto removeTemporaries = [&] {
    for (Publication &publication : publications)
      if (!publication.temporary.empty())
        llvm::sys::fs::remove(publication.temporary);
  };
  auto restoreBackups = [&] {
    for (Publication &publication : llvm::reverse(publications)) {
      if (publication.installed)
        llvm::sys::fs::remove(publication.path);
      if (!publication.backup.empty())
        llvm::sys::fs::rename(publication.backup, publication.path);
    }
  };
  for (Publication &publication : publications) {
    if (publication.path.empty() || !paths.insert(publication.path).second) {
      removeTemporaries();
      return false;
    }
    llvm::SmallString<256> model(publication.path);
    model += ".tmp-%%%%%%";
    int descriptor = -1;
    llvm::SmallString<256> temporary;
    if (llvm::sys::fs::createUniqueFile(model, descriptor, temporary)) {
      removeTemporaries();
      return false;
    }
    llvm::raw_fd_ostream output(descriptor, true);
    output << publication.contents;
    output.close();
    if (output.has_error()) {
      llvm::sys::fs::remove(temporary);
      removeTemporaries();
      return false;
    }
    publication.temporary = temporary.str().str();
  }

  for (Publication &publication : publications) {
    if (!llvm::sys::fs::exists(publication.path))
      continue;
    llvm::SmallString<256> model(publication.path);
    model += ".backup-%%%%%%";
    int descriptor = -1;
    llvm::SmallString<256> backup;
    if (llvm::sys::fs::createUniqueFile(model, descriptor, backup)) {
      restoreBackups();
      removeTemporaries();
      return false;
    }
    llvm::raw_fd_ostream placeholder(descriptor, true);
    placeholder.close();
    llvm::sys::fs::remove(backup);
    if (llvm::sys::fs::rename(publication.path, backup)) {
      restoreBackups();
      removeTemporaries();
      return false;
    }
    publication.backup = backup.str().str();
  }

  for (Publication &publication : publications) {
    if (llvm::sys::fs::rename(publication.temporary, publication.path)) {
      restoreBackups();
      removeTemporaries();
      return false;
    }
    publication.installed = true;
    publication.temporary.clear();
  }
  for (Publication &publication : publications)
    if (!publication.backup.empty())
      llvm::sys::fs::remove(publication.backup);
  return true;
}

} // namespace acir::testing
