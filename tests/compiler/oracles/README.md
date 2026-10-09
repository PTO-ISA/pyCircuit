# Compiler test oracles

This directory owns independent reference models, artifact checks and their
contract notes. It is test-only: product capture, passes, backends and Runtime
must not import it or use it to compute DUT results.

`queue_source/models.py` was moved from the existing queue vector generator;
`Source/Inputs/queue-source-vectors.py` remains the serializer for the existing
C++/RTL drivers. The three existing independent checkers live beside the model.
The existing `Source/queue-source.test` nightly owner runs them with
`--oracle-checks`, reusing the same public compile/link/emit artifacts. Reorder
mutation probes also use that public CLI, and failure must fail the owner.
Credit's reference-model self-check is supplemental; the queue owner still
builds/runs the generated DUT separately. No host reference result replaces it.

`source_observation_native.cpp` is the unchanged driver formerly embedded in
`Source/source-observation-native.test`. The same lit owner compiles/runs it;
that generated-model coverage is selected by the existing nightly tier. Cheap
Runtime and source-diagnostic gate tests remain separate.

The queue migration docstrings were preserved in
`queue_source/MIGRATION-NOTES.md`. Their recorded limitations are not erased by
moving the text. Large model files are inherited code, not a new oracle framework;
this extraction deliberately does not redesign or duplicate their algorithms.

Run through `flows/scripts/run_api_tests.sh --tier nightly` when validation is
scheduled. The 2026-10-07 user-directed review/extraction does **not** run these
oracles, generated DUT coverage or nightly. Syntax/AST checks do not establish
behavioral acceptance of the moved files or changed checker assertions.
