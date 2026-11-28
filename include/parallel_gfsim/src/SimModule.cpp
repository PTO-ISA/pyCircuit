#include "parallel_gfsim/SimModule.h"

namespace parallel_gfsim {

void SimModule::AdvanceInnerQueues()
{
    SimSystem* const system = GetSim();
    for (const auto& queue : innerQueues_) {
        if (!system || !system->DelegatesQueue(queue.get())) queue->Xfer();
    }
}

void SimModule::WorkChildren()
{
    if (GetSim() && GetSim()->ExecuteChildren(this, true)) return;
    for (const auto& child : submodules_) child->Work();
}

void SimModule::XferChildren()
{
    if (GetSim() && GetSim()->ExecuteChildren(this, false)) return;
    for (const auto& child : submodules_) child->Xfer();
}

} // namespace parallel_gfsim
