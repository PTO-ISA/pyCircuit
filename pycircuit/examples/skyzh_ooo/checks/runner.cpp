// Replay externally supplied operands/expectations through the generated component.
// This is deliberately not a CPU runner: it does not fetch, rename or issue.
#include "model.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace ac_generated;

struct Expected {
    Completion result;
    bool value, memory;
};

void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

int main() try {
    std::size_t n;
    if (!(std::cin >> n)) throw std::runtime_error("missing request count");
    std::vector<Request> requests(n);
    std::vector<Expected> expected;
    for (auto& request : requests) {
        Expected entry{};
        std::cin >> request.tag.epoch >> request.tag.sequence >> request.pc >> request.word
                 >> request.left >> request.right >> request.predicted_next >> request.fault
                 >> entry.value >> entry.result.value >> entry.memory >> entry.result.address
                 >> entry.result.store_value >> entry.result.target >> entry.result.fault;
        check(bool(std::cin), "invalid request");
        entry.result.tag = request.tag;
        entry.result.redirect = entry.result.target != request.predicted_next;
        if (request.tag.epoch == 1) expected.push_back(entry);
    }
    std::vector<unsigned> baseline;
    unsigned held = 0, replacement = 0;
    for (bool cache : {false, true}) for (bool reverse : {false, true}) {
        ExecutionCheck model(requests, cache, reverse);
        std::vector<unsigned> observed;
        for (std::size_t cycle = 0; cycle < n * 10 + 30; ++cycle) {
            const auto before = model.count.peek();
            const bool had_output = !model.completed_execute_out0.empty();
            const auto old_tag = had_output ? model.completed_execute_out0.peek().tag : Tag{};
            model.sim.step();
            const bool has_output = !model.completed_execute_out0.empty();
            if (had_output && has_output) {
                if (old_tag == model.completed_execute_out0.peek().tag) ++held;
                else ++replacement;
            }
            if (model.count.peek() != before) {
                check(before < expected.size(), "extra output");
                const auto& wanted = expected[before];
                const auto got = model.last.peek();
                if (!(got.tag == wanted.result.tag && got.target == wanted.result.target &&
                      got.fault == wanted.result.fault && got.redirect == wanted.result.redirect &&
                      (!wanted.value || got.value == wanted.result.value) &&
                      (!wanted.memory || (got.address == wanted.result.address && got.store_value == wanted.result.store_value)))) {
                    std::cerr << "request " << got.tag.sequence << ": value " << got.value
                              << " != " << wanted.result.value << ", target " << got.target
                              << " != " << wanted.result.target << '\n';
                    throw std::runtime_error("execution differs from independent oracle");
                }
            }
            observed.push_back(model.count.peek());
        }
        check(model.count.peek() == expected.size(), "missing output");
        check(model.incoming_produce_out0.empty() && model.completed_execute_out0.empty(), "pipeline did not drain");
        if (baseline.empty()) baseline = observed;
        else check(baseline == observed, "cache/order changed component timing");
    }
    check(held > 0 && replacement > 0, "backpressure or same-tick replacement not exercised");
    std::cout << "{\"requests\":" << n << ",\"outputs\":" << expected.size()
              << ",\"configurations\":4,\"held_cycles\":" << held
              << ",\"same_tick_replacements\":" << replacement << "}\n";
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
