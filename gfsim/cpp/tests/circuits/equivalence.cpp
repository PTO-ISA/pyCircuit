// Untimed equivalence of generated circuits with forward/reverse Module registration.
#include "../check.hpp"
#include "memory/model.hpp"
#include "pipeline/model.hpp"
using namespace circuits;
void compareState(const Netlist &a, const Netlist &b) {
    CHECK(a.snapshot() == b.snapshot());
    auto events = [](const Netlist &n, bool reverse) {
        auto result = n.sim->events();
        if (reverse)
            for (auto &[tick, mid] : result)
                mid = n.modules.size() - 1 - mid;
        std::sort(result.begin(), result.end());
        return result;
    };
    CHECK(events(a, false) == events(b, true));
    for (std::size_t q = 0; q < a.queues.size(); ++q)
        CHECK(a.queues[q]->stateVersion() == b.queues[q]->stateVersion());
}
using Builder = std::function<std::unique_ptr<Netlist>(bool)>;
void compare(const Builder &build, std::size_t ticks) {
    auto forward = build(false), reverse = build(true);
    for (std::size_t t = 0; t <= ticks; ++t) {
        auto a = forward->sim->step(), b = reverse->sim->step();
        std::vector<RuleId> aa(a.begin(), a.end()), bb(b.begin(), b.end());
        std::sort(aa.begin(), aa.end());
        std::sort(bb.begin(), bb.end());
        CHECK(aa == bb);
        compareState(*forward, *reverse);
    }
}
int main() {
    try {
        constexpr std::size_t ticks = 80, depth = 16;
        std::vector<Word> values(ticks + 10), control(ticks + 2);
        for (std::size_t i = 0; i < values.size(); ++i)
            values[i] = static_cast<Word>(i);
        for (std::size_t i = 0; i < control.size(); ++i)
            control[i] = static_cast<Word>(i);
        std::vector<Request> requests;
        for (std::size_t i = 0; i < ticks; ++i)
            requests.push_back({static_cast<std::int64_t>(i),
                                static_cast<Word>((i * 17) % (2 * depth)), i % 2 == 0,
                                static_cast<Word>(i)});
        compare([&](bool reverse) { return pipeline(values, 16, 1, 0, {}, true, 1, reverse); },
                ticks);
        compare(
            [&](bool reverse) { return pipeline(values, 2, 20, 20, control, false, 1, reverse); },
            ticks);
        compare([&](bool reverse) { return memory(requests, 2, depth, 4, 3, reverse); }, ticks);
        std::cout << "FIFO, parameterized backpressure and banked memory Module-order equivalence "
                     "passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
