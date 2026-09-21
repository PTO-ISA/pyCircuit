#pragma once

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/SystemUtils.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "llvm/Support/ConvertUTF.h"
#endif

#include <system_error>

namespace acir {

/// Move a staged directory into its final location.
///
/// `llvm::sys::fs::rename` opens the source without `FILE_FLAG_BACKUP_SEMANTICS`,
/// which Windows requires in order to open a directory, so it cannot move a
/// directory there and fails with permission denied. `MoveFileExW` moves
/// directories, and the caller has already established that the destination
/// does not exist.
inline std::error_code publishDirectory(llvm::StringRef staged,
                                        llvm::StringRef root) {
#if defined(_WIN32)
  llvm::SmallVector<wchar_t, 0> source;
  llvm::SmallVector<wchar_t, 0> target;
  if (std::error_code error = llvm::sys::windows::UTF8ToUTF16(staged, source))
    return error;
  if (std::error_code error = llvm::sys::windows::UTF8ToUTF16(root, target))
    return error;
  source.push_back(L'\0');
  target.push_back(L'\0');
  // A scanner or indexer can still hold a handle on a file written moments
  // ago, which blocks the move; retry briefly before reporting failure.
  for (unsigned attempt = 0; attempt != 20; ++attempt) {
    if (::MoveFileExW(source.data(), target.data(), MOVEFILE_REPLACE_EXISTING))
      return std::error_code();
    ::Sleep(50);
  }
  return std::error_code(static_cast<int>(::GetLastError()),
                         std::system_category());
#else
  return llvm::sys::fs::rename(staged, root);
#endif
}

} // namespace acir
