# Parallel GFSIM runtime

This library imports the cycle simulator from LinxISA/GFSIM's
`parallel-subtree` implementation at `e81f9632c31bc592f112453563d5d86bb9df5c68`.
It provides modules, staged queues and state, subtree scheduling, and the
associated memory, logging, tracing, topology and testbench support.

Use `#include <parallel_gfsim/SimModule.h>` and the `parallel_gfsim` C++
namespace. Macro names have the same prefix in uppercase, for example
`PARALLEL_GFSIM_ASSERT`. Installed SDK consumers can link the runtime with:

```cmake
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
target_link_libraries(my_model PRIVATE pycircuit::parallel_gfsim)
```

Register the root with `system.AddModule(root)`. For parallel execution,
call `system.SetWorkerCount(n)` and `system.SetParallelChildren(root.get())`
before `system.Build()`. Each `system.Step()` runs one Work phase and one
Xfer phase. The selected parent's child subtrees can execute on workers.
Cross-module methods run synchronously on the caller's thread. Designs must
assign concurrent writes to distinct state objects.
