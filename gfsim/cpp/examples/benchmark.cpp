#include "../tests/check.hpp"
#include "memory/model.hpp"
#include "pipeline/model.hpp"
#include <chrono>
#include <iomanip>
using namespace circuits;
using Builder = std::function<std::unique_ptr<Netlist>(bool)>;
void compareState(const Netlist &a, const Netlist &b) {
    CHECK(a.snapshot() == b.snapshot());
    CHECK(a.sim->events() == b.sim->events());
    for (std::size_t q = 0; q < a.queues.size(); ++q) {
        CHECK(a.queues[q]->stateVersion() == b.queues[q]->stateVersion());
        for (ModuleId m = 0; m < a.modules.size(); ++m) {
            auto ga = a.queues[q]->readers()[m], gb = b.queues[q]->readers()[m];
            CHECK((ga && ga == a.sim->module(m).readGen) == (gb && gb == b.sim->module(m).readGen));
        }
    }
    for (ModuleId m = 0; m < a.modules.size(); ++m)
        CHECK(a.sim->module(m).calls == b.sim->module(m).calls);
}
struct Measurement {
    double seconds{};
    Stats stats;
    std::size_t outputs{};
};
Measurement measure(const Builder &build, std::size_t ticks, unsigned repeat,
                    const std::string &expected, bool cache) {
    std::vector<double> times;
    Measurement result;
    for (unsigned i = 0; i < repeat; ++i) {
        auto c = build(cache);
        c->sim->step();
        auto before = c->sim->stats();
        auto start = std::chrono::steady_clock::now();
        for (std::size_t t = 0; t < ticks; ++t)
            c->sim->step();
        auto end = std::chrono::steady_clock::now();
        times.push_back(std::chrono::duration<double>(end - start).count());
        CHECK(c->snapshot() == expected);
        result.stats = c->sim->stats();
        result.stats.moduleWork -= before.moduleWork;
        result.stats.ruleWork -= before.ruleWork;
        result.stats.cacheHits -= before.cacheHits;
        result.stats.versionChecks -= before.versionChecks;
        result.stats.readerChecks -= before.readerChecks;
        result.stats.dfsVisits -= before.dfsVisits;
        result.stats.queueChecks -= before.queueChecks;
        result.stats.accepted -= before.accepted;
        result.stats.events -= before.events;
        result.outputs = c->output->size();
    }
    std::sort(times.begin(), times.end());
    result.seconds = times[times.size() / 2];
    return result;
}
void print(const Measurement &m) {
    const auto &s = m.stats;
    std::cout << "{\"seconds_median\":" << m.seconds << ",\"completed_outputs\":" << m.outputs
              << ",\"counters\":{\"module_work\":" << s.moduleWork
              << ",\"rule_work\":" << s.ruleWork << ",\"cache_hits\":" << s.cacheHits
              << ",\"version_checks\":" << s.versionChecks
              << ",\"reader_checks\":" << s.readerChecks << ",\"dfs_visits\":" << s.dfsVisits
              << ",\"queue_checks\":" << s.queueChecks << ",\"accepted\":" << s.accepted
              << ",\"events\":" << s.events << "}}";
}
void workload(const char *name, const Builder &build, std::size_t ticks, unsigned repeat) {
    auto cached = build(true), uncached = build(false);
    // Full semantic comparison is a separate untimed run.
    for (std::size_t t = 0; t <= ticks; ++t) {
        auto a = cached->sim->step(), b = uncached->sim->step();
        std::vector<RuleId> aa(a.begin(), a.end()), bb(b.begin(), b.end());
        std::sort(aa.begin(), aa.end());
        std::sort(bb.begin(), bb.end());
        CHECK(aa == bb);
        compareState(*cached, *uncached);
    }
    auto expected = cached->snapshot();
    auto yes = measure(build, ticks, repeat, expected, true),
         no = measure(build, ticks, repeat, expected, false);
    std::cout << '"' << name << "\":{\"cached\":";
    print(yes);
    std::cout << ",\"uncached\":";
    print(no);
    std::cout << ",\"uncached_over_cached\":" << no.seconds / yes.seconds << ",\"reader_bytes\":"
              << cached->queues.size() * cached->modules.size() * sizeof(Tick) << '}';
}
int main(int argc, char **argv) {
    try {
        std::size_t ticks = argc > 1 ? std::stoull(argv[1]) : 1000;
        unsigned repeat = argc > 2 ? static_cast<unsigned>(std::stoul(argv[2])) : 5;
        unsigned iterations = argc > 3 ? static_cast<unsigned>(std::stoul(argv[3])) : 20000;
        std::size_t depth = argc > 4 ? std::stoull(argv[4]) : 512;
        CHECK(ticks > 0 && repeat > 0 && depth > 0);
        std::vector<Word> values(ticks + 10), control(ticks + 2);
        for (std::size_t i = 0; i < values.size(); ++i)
            values[i] = static_cast<Word>(i);
        for (std::size_t i = 0; i < control.size(); ++i)
            control[i] = static_cast<Word>(i);
        std::vector<Request> requests;
        for (std::size_t i = 0; i < ticks; ++i)
            requests.push_back({static_cast<std::int64_t>(i), (i * 17) % (2 * depth), i % 2 == 0,
                                static_cast<Word>(i)});
        std::cout << std::setprecision(8) << "{\"ticks\":" << ticks << ",\"repeat\":" << repeat
                  << ",\"iterations\":" << iterations << ",\"depth\":" << depth
                  << ",\"workloads\":{";
        workload(
            "full_pipeline",
            [&](bool cache) { return pipeline(values, 16, 1, 0, {}, true, 1, cache); }, ticks,
            repeat);
        std::cout << ',';
        workload(
            "backpressure_compute",
            [&](bool cache) {
                return pipeline(values, 2, 20, iterations, control, false, 1, cache);
            },
            ticks, repeat);
        std::cout << ',';
        workload(
            "sparse_banked_table",
            [&](bool cache) { return memory(requests, 2, depth, 4, 3, cache); }, ticks, repeat);
        std::cout << "}}\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
