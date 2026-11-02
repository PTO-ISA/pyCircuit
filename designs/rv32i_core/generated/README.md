# Generated processor snapshot

These files are real outputs of the main-based source and SDK at
`3ee29b077725638081b69dcb0b267979ade07f1f`, including the held-operand repair.
They are executable deliverables: [the experiment report](../EXPERIMENT.md)
shows how to compile and verify them without recapturing the processor.

| Path | Contents |
| --- | --- |
| `cpp/` | Complete emitted C++ headers/sources, CMake entry, port metadata and source maps |
| `rtl/` | Physical top, five hardware modules, port metadata, emitted build metadata and source maps |
| `profiles/` | Original functional execution descriptors; the payload directories retain original timing descriptors |
| `artifacts.json` | Generation/source/tool identities, shared-profile rule, original IR hashes, dependencies and exact payload file checksums |

C++ and RTL are different backends. Timing and functional are execution policies.
The emitted computation/state files are byte-identical across policies, so only
the small differing descriptors are duplicated. The consumer in
`verification/artifact_driver` compiles these exact files and supplies the
external memory/clock boundary; it has no instruction semantics.

The large generated C++ headers may exceed GitHub's inline viewer limit. Use
[core.hpp](cpp/sources/rv32i_core/core.hpp)'s Raw/download action, a repository
checkout, or the commit archive. No large-file service or private bundle is
required; the largest file is about 3.17 MB. The manifest provides SHA-256 for
byte verification after download. Runtime primitives and headers come from the
fixed public SDK and are not copied here.

Keep emitted files unformatted. Checksums and compilation/simulation establish
the identity of these snapshots; explanatory text and adapters have separate
owners. Full IR can be regenerated with the pinned source commands and checked
against the recorded hashes; it is not needed to build the included C++.
