#include <model.hpp>
#include <iostream>
int main() {
    ac_generated::Probe probe;
    probe.sim.step();
    std::cout << "natural=" << probe.original.value()
              << " hoisted=" << probe.workaround.value() << " expected=2\n";
    // The model workaround is a gate; the diagnostic remains useful after a fix.
    return probe.workaround.value() == 2 ? 0 : 1;
}
