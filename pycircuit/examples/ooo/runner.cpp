// Host duties: bounded input, observation, termination and wall-clock timing.
#include <model.hpp>
#include <charconv>
#include <chrono>
#include <iostream>
#include <string>
using namespace ac_generated;
namespace {
std::uint64_t number(std::uint64_t max = UINT64_MAX) {
    std::string token;
    if (!(std::cin >> token)) throw std::invalid_argument("truncated numeric input");
    std::uint64_t value{};
    auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
    if (error != std::errc{} || end != token.data() + token.size() || value > max)
        throw std::invalid_argument("invalid unsigned input");
    return value;
}
struct Input {
    std::uint64_t cycles;
    std::uint32_t base, period, closed;
    bool reverse;
    std::vector<std::uint32_t> words, data;
    std::array<std::uint32_t, 32> regs;
};
Input readInput() {
    if (number(2) != 2) throw std::invalid_argument("unsupported protocol");
    Input i{};
    i.cycles = number(1000000); i.base = number(UINT32_MAX);
    i.reverse = number(1);
    i.period = number(10000); i.closed = number(10000);
    auto nw = number(1 << 20), nd = number(1 << 20);
    if (!nw || !nd || !i.cycles) throw std::invalid_argument("empty dimensions");
    i.words.resize(nw); i.data.resize(nd);
    for (auto &x : i.words) x = number(UINT32_MAX);
    for (auto &x : i.regs) x = number(UINT32_MAX);
    for (auto &x : i.data) x = number(UINT32_MAX);
    std::string extra;
    if (std::cin >> extra) throw std::invalid_argument("trailing input");
    if (i.regs[0] || i.base % 4 || i.base < nw * 4 ||
        std::uint64_t(i.base) + nd * 4 > (std::uint64_t{1} << 32) ||
        (i.period == 0 ? i.closed != 0 : i.closed >= i.period))
        throw std::invalid_argument("invalid initial state or writeback schedule");
    return i;
}
void tag(Tag t) { std::cout << '[' << t.epoch << ',' << t.sequence << ']'; }
void identity(Tag t, std::uint32_t pc, std::uint32_t word) {
    std::cout << "{\"tag\":"; tag(t);
    std::cout << ",\"pc\":" << pc << ",\"word\":" << word;
}
struct Previous {
    std::array<Tag, 8> alloc{}, ii{}, mi{}, ic{}, mc{};
    std::uint64_t retired{}, flushed{};
};
void snapshot(const CPU &cpu, Previous &before) {
    auto ctl = cpu.control.peek();
    std::cout << "{\"cycle\":" << cpu.sim.tick() << ",\"epoch\":" << ctl.epoch
              << ",\"head\":" << ctl.head << ",\"stopped\":" << ctl.stopped;
    unsigned occupancy = 0;
    std::cout << ",\"window\":[";
    for (unsigned i = 0; i < 8; ++i) {
        if (i) std::cout << ',';
        auto e = cpu.entries.refs[i]->peek();
        auto o = cpu.operands.refs[i]->peek();
        bool active = e.tag.epoch == ctl.epoch && e.tag.sequence >= ctl.head && !ctl.stopped;
        occupancy += active;
        identity(e.tag, e.pc, e.word);
        std::cout << ",\"active\":" << active << ",\"operands_tag\":"; tag(o.tag);
        std::cout << ",\"left\":[" << o.left.ready << ',' << o.left.value << ',';
        tag(o.left.producer);
        std::cout << "],\"right\":[" << o.right.ready << ',' << o.right.value << ',';
        tag(o.right.producer); std::cout << "]}";
    }
    std::cout << "],\"occupancy\":" << occupancy;
    auto changes = [&](const char *name, auto &previous, const auto &queues) {
        std::cout << ",\"" << name << "\":[";
        bool comma = false;
        for (unsigned i = 0; i < 8; ++i) {
            const auto &value = queues.refs[i]->peek();
            Tag t;
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Tag>) t = value;
            else t = value.tag;
            if (t == previous[i]) continue;
            previous[i] = t;
            if (comma) std::cout << ',';
            comma = true;
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Completion>) {
                identity(t, value.pc, value.word);
                std::cout << ",\"value\":" << value.value << ",\"fault\":" << value.fault;
            } else {
                auto e = cpu.entries.refs[i]->peek();
                identity(t, e.pc, e.word);
            }
            std::cout << '}';
        }
        std::cout << ']';
    };
    changes("allocate", before.alloc, cpu.entries);
    changes("issue_int", before.ii, cpu.int_issued);
    changes("issue_mem", before.mi, cpu.mem_issued);
    changes("complete_int", before.ic, cpu.int_results);
    changes("complete_mem", before.mc, cpu.mem_results);
    auto r = cpu.retirement.peek();
    std::cout << ",\"retire\":";
    if (r.count == before.retired) std::cout << "null";
    else {
        before.retired = r.count;
        identity(r.tag, r.pc, r.word);
        std::cout << ",\"write\":";
        if (r.rd) std::cout << '[' << r.rd << ',' << r.value << ']';
        else std::cout << "null";
        std::cout << ",\"store\":";
        if (r.store) std::cout << '[' << r.address << ',' << r.store_value << ']';
        else std::cout << "null";
        std::cout << ",\"fault\":" << r.fault << ",\"halt\":" << r.halt << '}';
    }
    auto f = cpu.flush.peek();
    std::cout << ",\"flush\":";
    if (f.count == before.flushed) std::cout << "null";
    else {
        before.flushed = f.count;
        std::cout << "{\"tag\":"; tag(f.tag);
        std::cout << ",\"target\":" << f.target << '}';
    }
    auto array = [](const auto &queues) {
        std::cout << '[';
        for (std::size_t i = 0; i < queues.refs.size(); ++i) {
            if (i) std::cout << ',';
            std::cout << queues.refs[i]->peek();
        }
        std::cout << ']';
    };
    std::cout << ",\"registers\":"; array(cpu.registers);
    std::cout << ",\"data\":"; array(cpu.data);
    std::cout << ",\"queues\":[" << cpu.fetched_fetch_out0.size() << ','
              << cpu.int_requests_issue_out0.size() << ',' << cpu.mem_requests_issue_out0.size() << ','
              << cpu.int_completed_execute_out0.size() << ',' << cpu.mem_completed_execute_out0.size() << ']';
    auto p = cpu.pending.peek();
    std::cout << ",\"memory_remaining\":" << p.remaining << ",\"memory_tag\":"; tag(p.result.tag);
    // Queue identities make recovery with in-flight messages directly observable.
    auto queue_tag = [](const auto &q) {
        if (q.empty()) std::cout << "null";
        else tag(q.peek().tag);
    };
    std::cout << ",\"inflight\":[";
    queue_tag(cpu.int_requests_issue_out0); std::cout << ',';
    queue_tag(cpu.mem_requests_issue_out0); std::cout << ',';
    queue_tag(cpu.int_completed_execute_out0); std::cout << ',';
    queue_tag(cpu.mem_completed_execute_out0); std::cout << "]}\n";
}
}
int main(int argc, char **argv) {
    try {
        bool benchmark = argc == 2 && std::string(argv[1]) == "--benchmark";
        if (argc != 1 && !benchmark) throw std::invalid_argument("usage: acpy-ooo [--benchmark]");
        const auto input = readInput();
        using Clock = std::chrono::steady_clock;
        auto start = Clock::now();
        CPU cpu(input.words, input.base, input.regs, input.data, input.period, input.closed,
                input.reverse);
        auto construct = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
        Previous previous;
        if (!benchmark) snapshot(cpu, previous);
        start = Clock::now();
        for (std::uint64_t i = 0; i < input.cycles && !cpu.control.peek().stopped; ++i) {
            cpu.sim.step();
            if (!benchmark) snapshot(cpu, previous);
        }
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
        if (!cpu.control.peek().stopped) throw std::runtime_error("cycle limit exceeded");
        if (benchmark) {
            const auto &s = cpu.sim.stats();
            std::cout << "{\"cycles\":" << cpu.sim.tick() << ",\"retired\":" << cpu.retirement.peek().count
                      << ",\"construct_ns\":" << construct << ",\"run_ns\":" << ns
                      << ",\"rule_work\":" << s.ruleWork << "}\n";
        }
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
