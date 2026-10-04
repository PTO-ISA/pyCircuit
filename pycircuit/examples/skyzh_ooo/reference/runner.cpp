// Input, termination, final-state observation and timing only. No CPU behavior.
#include "Common/Session.h"
#include "Pipeline/OoOExecute.h"
#include <chrono>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char** argv) try {
    if (argc != 3 && argc != 5)
        throw std::runtime_error("usage: skyzh-reference IMAGE MAX_CYCLES [--fixed CYCLES]");
    const std::string path = argv[1];
    const auto limit = std::stoull(argv[2]);
    const bool fixed = argc == 5;
    if (fixed && std::string(argv[3]) != "--fixed")
        throw std::runtime_error("expected --fixed");
    const auto count = fixed ? std::stoull(argv[4]) : limit;
    if (!count || count > limit) throw std::runtime_error("invalid cycle limit");
    if (!std::ifstream(path)) throw std::runtime_error("cannot open image");
    auto session = std::make_unique<Session>(false);
    if (path.ends_with(".hex")) session->load_hex(path.c_str());
    else session->load_memory(path.c_str());
    const std::vector<unsigned char> initial(session->memory.mem, session->memory.mem + MEMORY_SIZE);
    const auto begin = std::chrono::steady_clock::now();
    if (fixed) {
        for (unsigned long long i = 0; i < count; ++i) session->tick();
    } else {
        for (unsigned long long i = 0; i < count && !session->memory[0x30004]; ++i) session->tick();
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - begin).count();
    const bool stopped = session->memory[0x30004] != 0;
    std::cout << "{\"stopped\":" << (stopped ? "true" : "false")
              << ",\"cycles\":" << session->stat.cycle
              << ",\"elapsed_ns\":" << elapsed
              << ",\"timing\":\"" << (fixed ? "fixed" : "termination_checked")
              << "\",\"rob_peak\":" << session->e->stat.rob_usage_max
              << ",\"branches\":" << session->e->stat.total_branch
              << ",\"correct_branches\":" << session->e->stat.correct_branch
              << ",\"registers\":[";
    for (unsigned i = 0; i < 32; ++i) std::cout << (i ? "," : "") << session->rf.read(i);
    std::cout << "],\"memory_changes\":[";
    bool first = true;
    for (unsigned i = 0; i < MEMORY_SIZE; ++i) {
        if (initial[i] == session->memory[i]) continue;
        std::cout << (first ? "" : ",") << '[' << i << ',' << unsigned(session->memory[i]) << ']';
        first = false;
    }
    std::cout << "]}\n";
    return stopped ? 0 : 2;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
