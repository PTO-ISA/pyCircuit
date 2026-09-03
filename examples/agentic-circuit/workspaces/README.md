# Complete workspaces

Each child directory is a self-contained public CLI workspace. A workspace owns
its component schemas, bindings, canonical input trace, and expected build/run
artifacts so it can be copied and replayed independently.

The workspace contract intentionally duplicates a small fixture set. Shared
generated artifacts that are not part of workspace portability belong under
`tests/goldens` instead.
