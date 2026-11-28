#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace parallel_gfsim {

struct QueueSnapshot {
    std::vector<std::string> visible;
    std::vector<std::string> staged;
    std::vector<std::uint32_t> delays;
    std::size_t capacity = 0;
    bool stalled = false;
};

class TraceableQueue {
public:
    virtual ~TraceableQueue() = default;
    virtual QueueSnapshot Snapshot() const = 0;
};

namespace detail {

template <typename T>
auto StreamPayload(const T& value, int)
    -> decltype(std::declval<std::ostream&>() << value, std::string())
{
    std::ostringstream formatted;
    formatted << value;
    return formatted.str();
}

template <typename T>
std::string StreamPayload(const T&, long)
{
    throw std::logic_error("queue payload needs a trace formatter");
}

template <typename T>
typename std::enable_if<!std::is_pointer<T>::value, std::string>::type
DefaultPayload(const T& value)
{
    return StreamPayload(value, 0);
}

template <typename T>
typename std::enable_if<std::is_pointer<T>::value, std::string>::type
DefaultPayload(const T&)
{
    throw std::logic_error("pointer payload needs a trace formatter");
}

} // namespace detail

// Streams Perfetto-compatible JSON trace events without retaining old cycles.
class SimTrace {
public:
    explicit SimTrace(std::ostream& output, std::uint64_t sampleEvery = 1);
    ~SimTrace();

    SimTrace(const SimTrace&) = delete;
    SimTrace& operator=(const SimTrace&) = delete;

    void RegisterQueue(TraceableQueue& queue, const std::string& path);
    void Capture(std::uint64_t cycle);
    void AddDuration(const std::string& module, const std::string& name,
                     std::uint64_t startCycle, std::uint64_t endCycle,
                     const std::string& detail = "");
    void AddFlow(const std::string& fromModule, const std::string& toModule,
                 std::uint64_t startCycle, std::uint64_t endCycle,
                 const std::string& name = "flow");
    void Close();
    bool Closed() const { return closed_; }

private:
    struct RegisteredQueue {
        TraceableQueue* queue;
        std::string path;
        std::uint64_t tid;
    };

    static std::string Quote(const std::string& value);
    void BeginEvent();
    void EnsureOpen() const;
    std::uint64_t ModuleLane(const std::string& module);

    std::ostream* output_; // non-owning
    std::mutex eventMutex_;
    std::uint64_t sampleEvery_;
    std::uint64_t nextFlowId_ = 1;
    bool firstEvent_ = true;
    bool closed_ = false;
    std::vector<RegisteredQueue> queues_;
    std::unordered_map<std::string, std::uint64_t> moduleLanes_;
};

} // namespace parallel_gfsim
