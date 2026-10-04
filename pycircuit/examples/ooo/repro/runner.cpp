#include <model.hpp>
#include <iostream>
int main() {
    ac_generated::Probe probe;
    probe.sim.step();
    std::cout << "natural=" << probe.original.value()
              << " hoisted=" << probe.workaround.value() << " expected=2\n";
    // Both the natural spelling and historical workaround must agree.
    return probe.original.value() == 2 && probe.workaround.value() == 2 ? 0 : 1;
}
