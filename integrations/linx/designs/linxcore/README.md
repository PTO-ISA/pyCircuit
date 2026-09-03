# LinxCore Integration (Root-only Topology)

This repository no longer nests `designs/linxcore` as a Git submodule.

For LinxISA superproject workflows, use the root-level LinxCore checkout:

- `/Users/zhoubot/linx-isa/rtl/LinxCore`

If you run pyCircuit flows that require LinxCore assets, point those flows to the
superproject LinxCore path via environment/config expected by the calling script.
