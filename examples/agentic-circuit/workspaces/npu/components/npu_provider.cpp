#include "npu_provider.h"

#include "gfsim/components.h"

static_assert(gfsim::Component<workspace::npu::provider::NpuProvider>);
static_assert(gfsim::Component<workspace::npu::provider::NpuNodeProvider>);
