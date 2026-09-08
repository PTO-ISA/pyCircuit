# Issue #7 readable generated C++ symbols

Generated C++ previously embedded complete module/process specialization
fingerprints in ACSim namespaces and dispatch thunks, complete fingerprints in
QueueGraph specialization classes, and 16-hex fragments in every structured
ModelPlan class/file. Process helper and scalar-storage symbols exposed another
full-digest path.

Decision 0228 separates presentation from identity. ACSim lowering,
verification, and direct emission now share one readable module/process thunk
helper. ModelPlan and QueueGraph use semantic class names and add a local
16-hex suffix only inside an actual collision group; a suffix collision fails
closed. Process-state symbols use the closed helper role and scalar type and
reject duplicate readable identities. Category prefixes shield C++ keywords and
generator-owned root names. Full SHA-256 values remain in canonical operations,
plans, manifests, providers, artifacts, and cache keys.

Review-driven coverage checks C++ keywords, exact four-thunk agreement, full
fingerprint retention, readable module/process classes and files, deterministic
generation, local collision suffixes, post-suffix rejection, QueueGraph module
reuse, process-state descriptor identity, generated compile/run behavior, and
the full queue-codegen integration matrix.

Independent final review covered 42 changed files plus evidence artifacts,
found no remaining P0, P1, or P2 issues, and recommended approval. Review fixes
added exact process-level manifest mapping in both emitters, retained unrelated
integration shape assertions, and completed readable ProcessState scalar and
negative coverage.
