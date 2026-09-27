# U02-A0 behavior-preserving extraction verification

Fresh current-checkout U02 binaries passed the unchanged accepted behavior suite: 36/36 foundation contracts, 14/14 SourceUnit/header GTests, and 51/51 configured Packet/source-admission system cases with zero skips. CTest passed both isolated targets. No test source was changed for U02-A0.

The U02 harness generated Packet and consumer transport, body, and interface artifacts from the current checkout. All six files are byte-for-byte identical to the accepted raw U01-E artifacts; no EOF or other normalization was needed. Their SHA-256 values also match the U01-E manifest.

The build cache names this checkout as `CMAKE_HOME_DIRECTORY` and uses LLVM 22.1.8. U02 binaries have their own hashes and were built under `.pycircuit_out/u02-a0/build`; no U01 binary was copied or reused.

This evidence supports only behavior preservation across the Python importer Context/Signature responsibility extraction. It does not add or validate N1 attributes, driver/publication, installed SDK behavior, final link/backend closure, U03 proofs, or complete framework behavior.
