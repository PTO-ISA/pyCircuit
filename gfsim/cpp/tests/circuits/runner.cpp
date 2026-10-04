#include "feedback/model.hpp"
#include "lookup/model.hpp"
#include "memory/model.hpp"
#include "packets/model.hpp"
#include "pairs/model.hpp"
#include "pipeline/model.hpp"
#include "retry/model.hpp"
using namespace circuits;
std::vector<Timed<Word>> readChanges() {
    std::size_t n;
    std::cin >> n;
    std::vector<Timed<Word>> v(n);
    for (auto &x : v)
        std::cin >> x.due >> x.value;
    return v;
}
int main() {
    try {
        std::string kind;
        std::size_t ticks;
        bool reverse;
        std::cin >> kind >> ticks >> reverse;
        std::unique_ptr<Netlist> c;
        if (kind == "pipeline") {
            int length;
            Tick period;
            unsigned iterations;
            bool prefill;
            std::size_t capacity;
            std::cin >> length >> period >> iterations >> prefill >> capacity;
            auto values = readWords(std::cin), control = readWords(std::cin);
            c = pipeline(values, length, period, iterations, control, prefill, capacity, reverse);
        } else if (kind == "packets") {
            Tick period;
            std::cin >> period;
            std::array lanes{readWords(std::cin), readWords(std::cin)};
            std::size_t n;
            std::cin >> n;
            std::vector<Timed<Mode>> changes(n);
            for (auto &x : changes)
                std::cin >> x.due >> x.value.mode >> x.value.bias >> x.value.epoch;
            c = packets(lanes, changes, period, reverse);
        } else if (kind == "pairs") {
            Tick period;
            std::cin >> period;
            auto left = readWords(std::cin), right = readWords(std::cin);
            c = pairs(left, right, period, reverse);
        } else if (kind == "memory") {
            std::size_t banks, depth, n;
            Tick latency, period;
            std::cin >> banks >> depth >> latency >> period >> n;
            std::vector<Request> requests(n);
            for (auto &r : requests)
                std::cin >> r.seq >> r.address >> r.write >> r.data;
            c = memory(requests, banks, depth, latency, period, reverse);
        } else if (kind == "feedback") {
            bool loop;
            std::size_t n;
            std::cin >> loop >> n;
            std::vector<Value> tokens(n);
            for (auto &x : tokens)
                std::cin >> x.seq >> x.value;
            c = feedback(tokens, loop, reverse);
        } else if (kind == "lookup") {
            Tick period;
            std::size_t depth;
            std::cin >> period >> depth;
            auto values = readWords(std::cin);
            auto indices = readChanges();
            std::size_t count;
            std::cin >> count;
            std::vector<std::pair<std::size_t, std::vector<Timed<Word>>>> updates;
            for (std::size_t i = 0; i < count; ++i) {
                std::size_t index;
                std::cin >> index;
                updates.emplace_back(index, readChanges());
            }
            c = lookup(values, indices, updates, period, depth, reverse);
        } else if (kind == "retry") {
            Word attempts;
            std::cin >> attempts;
            c = retry(attempts, reverse);
        } else
            throw std::invalid_argument("unknown scenario");
        if (!std::cin)
            throw std::invalid_argument("invalid stimulus");
        for (std::size_t t = 0; t < ticks; ++t) {
            auto accepted = c->sim->step();
            std::cout << c->trace(accepted) << '\n';
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
