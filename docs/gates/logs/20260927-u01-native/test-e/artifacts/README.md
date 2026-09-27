# Archived U01 IR text

These reviewable copies normalize extra blank lines at EOF to one final newline
so repository whitespace checks pass. No MLIR content changed. The original
run artifacts remain in the candidate checkout at
`.pycircuit_out/u01/native-test-e/artifacts/`.

The native test manifest binds the original bytes. `archive-binding.json` records
both raw and archived hashes. Appending the recorded number of LF bytes to each
archived file exactly reconstructs its raw original; this was checked while
archiving. No compiler, library or executable was copied between checkouts.
