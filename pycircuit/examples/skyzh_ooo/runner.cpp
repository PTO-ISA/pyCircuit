// Program loading, driving, observation and timing only; no CPU behavior.
#include <charconv>
#include <chrono>
#include <fstream>
#include <iostream>
#include <model.hpp>
#include <string>
#include <sys/resource.h>
using namespace ac_generated;
static std::uint64_t number(const std::string &s, int base = 10) {
  std::uint64_t n{};
  auto [p, e] = std::from_chars(s.data(), s.data() + s.size(), n, base);
  if (e != std::errc{} || p != s.data() + s.size())
    throw std::invalid_argument("invalid number");
  return n;
}
int main(int argc, char **argv) {
  try {
    if (argc < 3)
      throw std::invalid_argument(
          "usage: acpy-skyzh IMAGE.hex MAX_CYCLES [--cache-off] [--reverse] "
          "[--benchmark] [--fixed N] [--warmup K] [--wb-period N] [--wb-closed "
          "N]");
    const auto limit = number(argv[2]);
    if (!limit || limit > 100000000)
      throw std::invalid_argument("cycle bound out of range");
    bool cache = true, reverse = false, benchmark = false;
    std::uint64_t fixed = 0, warmup = 0, period = 0, closed = 0;
    for (int i = 3; i < argc; ++i) {
      std::string opt = argv[i];
      if (opt == "--cache-off")
        cache = false;
      else if (opt == "--reverse")
        reverse = true;
      else if (opt == "--benchmark")
        benchmark = true;
      else if (i + 1 < argc && (opt == "--fixed" || opt == "--warmup" ||
                                opt == "--wb-period" || opt == "--wb-closed")) {
        auto n = number(argv[++i]);
        if (opt == "--fixed")
          fixed = n;
        if (opt == "--warmup")
          warmup = n;
        if (opt == "--wb-period")
          period = n;
        if (opt == "--wb-closed")
          closed = n;
      } else
        throw std::invalid_argument("unknown or incomplete option");
    }
    if ((period == 0 ? closed != 0 : closed >= period) || warmup > limit ||
        fixed > limit - warmup || (fixed && !benchmark))
      throw std::invalid_argument("invalid timing/writeback range");
    std::ifstream file(argv[1]);
    if (!file)
      throw std::invalid_argument("cannot open program");
    std::vector<std::uint32_t> words, data(0x40000 / 4, 0);
    std::string token;
    while (file >> token) {
      auto n = number(token, 16);
      if (n > UINT32_MAX || words.size() == data.size())
        throw std::invalid_argument("program word/size out of range");
      words.push_back(n);
    }
    if (words.empty())
      throw std::invalid_argument("empty program");
    std::copy(words.begin(), words.end(), data.begin());
    using Clock = std::chrono::steady_clock;
    auto start = Clock::now();
    CPU cpu(words, data, period, closed, cache, reverse);
    for (unsigned i = 0; i < 10; ++i) {
      auto expected = i < 4   ? cpu.int_requests.rid_issue
                      : i < 7 ? cpu.load_requests.rid_issue
                              : cpu.store_requests.rid_issue;
      if (cpu.stations.refs[i]->popRule() != expected ||
          cpu.stations.refs[i]->pushRule() != cpu.dispatch.rid_allocate)
        throw std::logic_error(
            "incorrect static reservation-station source binding");
    }
    auto construct = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         Clock::now() - start)
                         .count();
    std::array<Tag, 3> previousIssue{};
    std::array<Tag, 12> previousResult{};
    std::uint64_t retired = 0;
    auto snapshot = [&] {
      std::cout << "{\"cycle\":" << cpu.sim.tick()
                << ",\"rob\":" << cpu.rob.size() << ",\"stations\":";
      unsigned occupied = 0;
      for (auto *q : cpu.stations.refs)
        occupied += !q->empty();
      std::cout << occupied;
      std::cout << ",\"issued\":[";
      std::array<gfsim::Queue<Request> *, 3> requests{
          &cpu.int_requests_issue_out0, &cpu.load_requests_issue_out0,
          &cpu.store_requests_issue_out0};
      bool comma = false;
      unsigned inflight = 0;
      for (unsigned i = 0; i < 3; ++i)
        if (!requests[i]->empty()) {
          ++inflight;
          auto t = requests[i]->peek().tag;
          if (t != previousIssue[i]) {
            if (comma)
              std::cout << ',';
            comma = true;
            std::cout << '[' << i << ',' << t.sequence << ',' << t.epoch << ']';
            previousIssue[i] = t;
          }
        }
      inflight += !cpu.int_completed_execute_out0.empty();
      inflight += !cpu.store_completed_execute_out0.empty();
      inflight += !cpu.addressed_address_out0.empty();
      inflight += !cpu.loaded_read_out0.empty();
      inflight += !cpu.load_completed_respond_out0.empty();
      std::cout << "],\"completed\":[";
      comma = false;
      for (unsigned i = 0; i < 12; ++i) {
        auto t = cpu.results.refs[i]->peek().tag;
        if (t != previousResult[i]) {
          if (comma)
            std::cout << ',';
          comma = true;
          std::cout << '[' << t.sequence << ',' << t.epoch << ']';
          previousResult[i] = t;
        }
      }
      std::cout << "],\"inflight\":" << inflight << ",\"retire\":";
      auto r = cpu.retirement.peek();
      if (r.count == retired)
        std::cout << "null";
      else {
        retired = r.count;
        std::cout << "{\"sequence\":" << r.tag.sequence
                  << ",\"epoch\":" << r.tag.epoch << ",\"pc\":" << r.pc
                  << ",\"word\":" << r.word << ",\"write\":";
        if (r.rd)
          std::cout << '[' << r.rd << ',' << r.value << ']';
        else
          std::cout << "null";
        std::cout << ",\"store\":";
        if (r.width)
          std::cout << '[' << r.address << ',' << r.width << ','
                    << r.store_value << ']';
        else
          std::cout << "null";
        std::cout << ",\"next_pc\":" << r.target << ",\"fault\":" << r.fault
                  << ",\"redirect\":" << r.redirect
                  << ",\"halted\":" << r.halted << '}';
      }
      std::cout << "}\n";
    };
    for (std::uint64_t i = 0; i < warmup; ++i)
      cpu.sim.step();
    auto instructionsBefore = cpu.retirement.peek().count;
    start = Clock::now();
    if (fixed) {
      for (std::uint64_t i = 0; i < fixed; ++i)
        cpu.sim.step();
    } else {
      while (cpu.sim.tick() < limit && !cpu.control.peek().stopped) {
        cpu.sim.step();
        if (!benchmark)
          snapshot();
      }
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                       Clock::now() - start)
                       .count();
    if (!fixed && !cpu.control.peek().stopped)
      throw std::runtime_error("cycle bound exceeded");
    struct rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    std::cout << "{\"final\":true,\"cycles\":" << cpu.sim.tick()
              << ",\"instructions\":" << cpu.retirement.peek().count
              << ",\"stopped\":" << cpu.control.peek().stopped;
    if (benchmark) {
      std::cout << ",\"measured_cycles\":" << cpu.sim.tick() - warmup
                << ",\"measured_instructions\":"
                << cpu.retirement.peek().count - instructionsBefore
                << ",\"run_ns\":" << elapsed
                << ",\"construct_ns\":" << construct
                << ",\"max_rss_kib\":" << usage.ru_maxrss;
    }
    {
      std::cout << ",\"registers\":[";
      for (unsigned i = 0; i < 32; ++i) {
        if (i)
          std::cout << ',';
        std::cout << cpu.registers.refs[i]->peek();
      }
      std::cout << "],\"memory_changes\":[";
      bool comma = false;
      for (std::size_t i = 0; i < data.size(); ++i) {
        auto now = cpu.data.refs[i]->peek();
        for (unsigned j = 0; j < 4; ++j) {
          auto a = (data[i] >> (8 * j)) & 255, b = (now >> (8 * j)) & 255;
          if (a != b) {
            if (comma)
              std::cout << ',';
            comma = true;
            std::cout << '[' << i * 4 + j << ',' << b << ']';
          }
        }
      }
      std::cout << ']';
    }
    std::cout << "}\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
