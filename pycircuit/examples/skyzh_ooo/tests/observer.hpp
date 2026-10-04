#pragma once
// Shared image/JSON utilities only; the two adapters observe independent cores.
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace observe {
using Clock = std::chrono::steady_clock;
constexpr unsigned memorySize = 0x400000, stopAddress = 0x30004;
struct Options {
    std::string image;
    std::uint64_t limit{}, fixed{};
    bool benchmark{}, reverse{};
    Options(int argc, char **argv) {
        if (argc < 3) throw std::invalid_argument("IMAGE LIMIT [--fixed N] [--benchmark] [--reverse]");
        image = argv[1];
        limit = std::stoull(argv[2]);
        for (int i = 3; i < argc; ++i) {
            std::string option = argv[i];
            if (option == "--fixed" && i + 1 < argc) fixed = std::stoull(argv[++i]);
            else if (option == "--benchmark") benchmark = true;
            else if (option == "--reverse") reverse = true;
            else throw std::invalid_argument("unknown option: " + option);
        }
        if (!limit || fixed > limit || (benchmark && !fixed))
            throw std::invalid_argument("benchmark requires 0 < fixed <= limit");
    }
};
struct Image {
    std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(memorySize);
    std::size_t extent{};
    explicit Image(const std::string &path) {
        std::ifstream input(path);
        if (!input) throw std::invalid_argument("cannot open image: " + path);
        const unsigned width = path.ends_with(".hex") ? 4 : 1;
        std::size_t address = 0;
        std::string token;
        while (input >> token) {
            if (token[0] == '@') { address = std::stoull(token.substr(1), nullptr, 16); continue; }
            auto value = std::stoull(token, nullptr, 16);
            if (address + width > bytes.size() || (width == 1 && value > 255) || value > 0xffffffffULL)
                throw std::invalid_argument("image address/value out of range");
            for (unsigned i = 0; i < width; ++i) bytes[address++] = value >> (8 * i);
            extent = std::max(extent, address);
        }
    }
};
template<class Values> void array(const Values &values) {
    std::cout << '[';
    bool first = true;
    for (auto value : values) { std::cout << (first ? "" : ",") << +value; first = false; }
    std::cout << ']';
}
struct Snapshot {
    std::vector<std::uint32_t> values;
    std::vector<std::string> names;
    void add(std::string name, std::uint32_t value) {
        names.push_back(std::move(name)); values.push_back(value);
    }
    void group(const std::string &name, std::initializer_list<std::uint32_t> row) {
        unsigned i = 0;
        for (auto v : row) add(name + "." + std::to_string(i++), v);
    }
};
// [ROB id, PC, instruction bits, decoded opcode, destination, value, predicted PC, immediate].
using Commit = std::vector<std::uint32_t>;
inline void trace(std::uint64_t tick, const Snapshot &state, const Commit &commit) {
    std::cout << "{\"tick\":" << tick;
    if (!tick) {
        std::cout << ",\"fields\":[";
        for (std::size_t i=0; i<state.names.size(); ++i)
            std::cout << (i ? "," : "") << '"' << state.names[i] << '"';
        std::cout << ']';
    }
    std::cout << ",\"state\":"; array(state.values);
    std::cout << ",\"commit\":"; array(commit);
    std::cout << "}\n";
}
template<class Byte> std::uint64_t hash(unsigned size, Byte byte) {
    std::uint64_t value=1469598103934665603ULL;
    for(unsigned i=0;i<size;++i) value=(value ^ byte(i))*1099511628211ULL;
    return value;
}
template<class Reg, class Byte, class History, class Counter>
void final(const Image &image, std::uint64_t cycles, std::int64_t run,
           std::int64_t construct, Reg reg, Byte byte, History history, Counter counter) {
    std::cout << "{\"final\":true,\"stopped\":" << (byte(stopAddress) ? "true" : "false")
              << ",\"cycles\":" << cycles << ",\"run_ns\":" << run
              << ",\"construct_ns\":" << construct << ",\"registers\":[";
    for(unsigned i=0;i<32;++i) std::cout << (i ? "," : "") << reg(i);
    std::cout << "],\"memory_changes\":[";
    bool first=true;
    for(unsigned i=0;i<memorySize;++i)
        if(byte(i)!=image.bytes[i]) {
            std::cout << (first ? "" : ",") << '[' << i << ',' << +byte(i) << ']'; first=false;
        }
    std::cout << "],\"predictor_history_hash\":" << hash(memorySize, history)
              << ",\"predictor_counters_hash\":" << hash(memorySize, counter) << "}\n";
}
inline std::int64_t elapsed(Clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-start).count();
}
}
