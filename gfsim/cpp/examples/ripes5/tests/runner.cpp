#ifdef ACPY_GENERATED_MODEL
#include <model.hpp>
#else
#include "../model.hpp"
#endif
#include <charconv>
#include <chrono>
#include <iostream>
#include <string>

#ifdef ACPY_GENERATED_MODEL
using namespace ac_generated;
using Word = std::uint32_t;
#else
using namespace ripes5;
#endif
namespace {
std::uint64_t argument(const char *text) {
    const std::string token(text);
    std::uint64_t value{};
    auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
    if (error != std::errc{} || end != token.data() + token.size())
        throw std::invalid_argument("invalid unsigned cycle argument");
    return value;
}
std::uint64_t number(std::uint64_t max = UINT64_MAX) {
    std::string token;
    if (!(std::cin >> token))
        throw std::invalid_argument("truncated numeric input");
    std::uint64_t value{};
    auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
    if (error != std::errc{} || end != token.data() + token.size() || value > max)
        throw std::invalid_argument("invalid unsigned input value");
    return value;
}
struct Input {
    gfsim::Tick max_cycles;
    Word end_pc, data_base;
    bool reverse;
    std::vector<Word> words, data;
    std::array<Word, 32> registers;
};
Input readInput() {
    if (number(2) != 2)
        throw std::invalid_argument("unsupported input protocol");
    Input i{};
    i.max_cycles = number();
    i.end_pc = number(UINT32_MAX);
    i.data_base = number(UINT32_MAX);
    i.reverse = number(1);
    // Bound allocations before reading untrusted counts. Python validates the ISA/schema.
    auto nw = number(1U << 20), nd = number(1U << 20);
    if (!nw || !nd || !i.max_cycles)
        throw std::invalid_argument("empty input dimensions");
    i.words.resize(nw);
    i.data.resize(nd);
    for (auto &x : i.words)
        x = number(UINT32_MAX);
    for (auto &x : i.registers)
        x = number(UINT32_MAX);
    for (auto &x : i.data)
        x = number(UINT32_MAX);
    std::string extra;
    if (std::cin >> extra)
        throw std::invalid_argument("trailing input");
    if (i.registers[0] || i.data_base % 4 || i.data_base < nw * 4 ||
        std::uint64_t(i.data_base) + nd * 4 > (std::uint64_t{1} << 32) || i.end_pc % 4 ||
        i.end_pc / 4 + 4 != nw || i.words[i.end_pc / 4] != 0x7ff00013 || i.words[nw - 3] != 0x6f ||
        i.words[nw - 2] != 0x13 || i.words[nw - 1] != 0x13 ||
        std::count(i.words.begin(), i.words.end(), 0x7ff00013U) != 1)
        throw std::invalid_argument("invalid data region, registers or marker");
    for (auto word : i.words)
        if (decode(word).op == INVALID || decode(word).op == HALT)
            throw std::invalid_argument("unsupported instruction");
    return i;
}
bool executable(const CPU &cpu, Word pc) {
    return pc % 4 == 0 && pc / 4 < cpu.words.size();
}
void stage(const CPU &cpu, bool valid, Word pc) {
    valid = valid && executable(cpu, pc);
    std::cout << "{\"valid\":" << valid << ",\"pc\":";
    if (valid)
        std::cout << pc;
    else
        std::cout << "null";
    std::cout << '}';
}
void snapshot(const CPU &cpu, const Event *retire = nullptr, const Store *store = nullptr) {
    // Testbench observation outside Work never creates dependencies. At tick zero
    // derive control directly, since runtime Signals initialize on the first step.
    const auto id = cpu.if_id.peek(), ex = cpu.id_ex.peek(), mem = cpu.ex_mem.peek(),
               wb = cpu.mem_wb.peek();
    const auto result = execute(ex, mem, wb);
    const bool stall = loadUseStall(id, ex);
    const Word pc = cpu.pc.peek(), next = result.redirect ? result.next_slot.result : pc + 4;
    std::cout << std::boolalpha << "{\"cycle\":" << cpu.sim.tick() << ",\"stages\":[";
    stage(cpu, true, pc);
    for (auto slot : {id, ex, mem, wb}) {
        std::cout << ',';
        stage(cpu, slot.valid, slot.pc);
    }
    std::cout << "],\"fetch_pc\":" << pc << ",\"next_fetch\":" << next
              << ",\"next_pc\":" << (stall ? pc : next) << ",\"control\":{\"stall\":" << stall
              << ",\"flush_ifid\":" << result.redirect
              << ",\"flush_idex\":" << (result.redirect || stall) << ",\"pc_enable\":" << !stall
              << ",\"idex_enable\":true,\"exmem_clear\":false,\"target\":";
    if (result.redirect)
        std::cout << result.next_slot.result;
    else
        std::cout << "null";
    std::cout << ",\"forward_a\":" << result.forward_a << ",\"forward_b\":" << result.forward_b
              << "},\"stalled\":[false,false," << ex.stalled << ',' << mem.stalled << ','
              << wb.stalled << ']';
    auto array = [](const auto &refs) {
        std::cout << '[';
        for (std::size_t i = 0; i < refs.size(); ++i) {
            if (i)
                std::cout << ',';
            std::cout << refs[i]->peek();
        }
        std::cout << ']';
    };
    std::cout << ",\"registers\":";
    array(cpu.registers.refs);
    std::cout << ",\"data\":";
    array(cpu.data.refs);
    std::cout << ",\"retired\":" << cpu.retirement.peek().sequence << ",\"retire\":";
    if (!retire)
        std::cout << "null";
    else {
        std::cout << "{\"pc\":" << retire->pc << ",\"word\":" << retire->word << ",\"write\":";
        if (retire->rd)
            std::cout << '[' << retire->rd << ',' << retire->value << ']';
        else
            std::cout << "null";
        std::cout << '}';
    }
    std::cout << ",\"store\":";
    if (store)
        std::cout << '[' << store->address << ',' << store->value << ']';
    else
        std::cout << "null";
    std::cout << "}\n";
}
void counters(const CPU &cpu, std::int64_t construct_ns, std::int64_t run_ns,
              std::uint64_t warmup = 0, std::uint64_t measured = 0, std::uint64_t before = 0) {
    const auto &s = cpu.sim.stats();
    std::cout << '{';
    if (measured) {
        std::cout << "\"warmup_cycles\":" << warmup << ",\"measured_cycles\":" << measured
                  << ",\"retired_before\":" << before
                  << ",\"retired\":" << cpu.retirement.peek().sequence
                  << ",\"retired_delta\":" << cpu.retirement.peek().sequence - before
                  << ",\"final_state\":";
        snapshot(cpu);
        std::cout << ',';
    }
    std::cout << "\"cycles\":" << cpu.sim.tick() << ",\"construct_ns\":" << construct_ns
              << ",\"run_ns\":" << run_ns << ",\"module_work\":" << s.moduleWork
              << ",\"rule_work\":" << s.ruleWork << ",\"signal_work\":" << s.signalWork
              << ",\"events\":" << s.events << ",\"due_events\":" << s.dueEvents
              << ",\"change_notifications\":" << s.changeNotifications
              << ",\"arbitration_attempts\":" << s.arbitrationAttempts
              << ",\"delta_rounds\":" << s.deltaRounds << ",\"queue_checks\":" << s.queueChecks
              << ",\"accepted\":" << s.accepted << ",\"signal_evaluations\":["
              << cpu.ex_result.evaluations() << ',' << cpu.load_use_stall.evaluations() << "]}\n";
}
} // namespace
int main(int argc, char **argv) {
    try {
        const bool benchmark = argc == 2 && std::string(argv[1]) == "--benchmark";
        const bool fixed = argc == 4 && std::string(argv[1]) == "--benchmark-fixed";
        if (argc != 1 && !benchmark && !fixed)
            throw std::invalid_argument(
                "usage: gfsim-ripes5 [--benchmark | --benchmark-fixed K N] < numeric-input");
        const auto warmup = fixed ? argument(argv[2]) : 0;
        const auto measured = fixed ? argument(argv[3]) : 0;
        const auto input = readInput();
        if (fixed &&
            (!measured || warmup > input.max_cycles || measured > input.max_cycles - warmup))
            throw std::invalid_argument("fixed window requires N > 0 and K + N <= max_cycles");
        using Clock = std::chrono::steady_clock;
        auto start = Clock::now();
        CPU cpu(input.words, input.data_base, input.registers, input.data, input.reverse);
        auto construct_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
        if (fixed) {
            for (std::uint64_t i = 0; i < warmup; ++i)
                cpu.sim.step();
            const auto before = cpu.retirement.peek().sequence;
            start = Clock::now();
            for (std::uint64_t i = 0; i < measured; ++i)
                cpu.sim.step();
            const auto stop = Clock::now();
            const auto ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            counters(cpu, construct_ns, ns, warmup, measured, before);
            return 0;
        }
        if (!benchmark)
            snapshot(cpu);
        start = Clock::now();
        bool done = false;
        for (gfsim::Tick i = 0; i < input.max_cycles; ++i) {
            const auto wb = cpu.mem_wb.peek();
            done = wb.valid && wb.pc == input.end_pc;
            if (benchmark)
                cpu.sim.step();
            else {
                const auto before = cpu.retirement.peek().sequence,
                           store_before = cpu.store.peek().sequence;
                cpu.sim.step();
                const auto event = cpu.retirement.peek();
                const auto store = cpu.store.peek();
                snapshot(cpu, event.sequence == before ? nullptr : &event,
                         store.sequence == store_before ? nullptr : &store);
            }
            if (done)
                break;
        }
        auto run_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
        if (!done)
            throw std::runtime_error("benchmark marker missing");
        if (benchmark)
            counters(cpu, construct_ns, run_ns);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
