#include <cstdint>
#include <gfsim/queue.hpp>

// Ordinary generated class: business methods and scheduling records are separate.
class Increment {
  public:
    gfsim::Simulator &sim;
    gfsim::Queue<std::uint32_t> &input;
    gfsim::Queue<std::uint32_t> &output;
    gfsim::ModuleId mid{};
    gfsim::RuleId rid{};
    gfsim::ParameterCache<std::uint32_t> arguments;

    void Work() { workIncrement(7); }
    void workIncrement(std::uint32_t bias) {
        if (!sim.beginRule(rid, arguments, bias))
            return;
        sim.recordRead(mid, input, rid);
        const auto *value = input.tryPeek();
        if (!value) {
            sim.abortRule(rid);
            return;
        }
        input.proposePop(rid);
        output.proposePush(rid, *value + bias);
        sim.completeRule(rid);
    }
    bool arbitrate() { return sim.arbitrateRule(rid); }
};
int main() {
    gfsim::Queue<std::uint32_t> input(2, {10, 20}), output(2);
    gfsim::Simulator sim;
    Increment module{sim, input, output};
    module.mid = sim.addModule<&Increment::Work>(module);
    module.rid = sim.addRule(module.mid, [](void *object, gfsim::Simulator &, gfsim::RuleId) {
        return static_cast<Increment *>(object)->arbitrate();
    });
    sim.addQueue(input);
    sim.addQueue(output);
    sim.bind(module.rid, input, gfsim::Pop);
    sim.bind(module.rid, output, gfsim::Push);
    sim.freeze();
    sim.step();
    sim.step();
    return output.size() == 2 && output.at(0) == 17 && output.at(1) == 27 ? 0 : 1;
}
