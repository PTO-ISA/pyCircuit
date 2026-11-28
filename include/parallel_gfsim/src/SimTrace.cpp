#include "parallel_gfsim/SimTrace.h"

#include <iomanip>

namespace parallel_gfsim {

SimTrace::SimTrace(std::ostream& output, std::uint64_t sampleEvery)
    : output_(&output), sampleEvery_(sampleEvery)
{
    if (sampleEvery_ == 0) {
        throw std::invalid_argument("trace sample interval must be positive");
    }
    *output_ << "{\"traceEvents\":[";
    BeginEvent();
    *output_ << "{\"ph\":\"M\",\"name\":\"process_name\",\"pid\":1,"
             << "\"args\":{\"name\":\"Queues\"}}";
    BeginEvent();
    *output_ << "{\"ph\":\"M\",\"name\":\"process_name\",\"pid\":2,"
             << "\"args\":{\"name\":\"Modules\"}}";
}

SimTrace::~SimTrace()
{
    Close();
}

std::string SimTrace::Quote(const std::string& value)
{
    std::ostringstream escaped;
    escaped << '"';
    for (unsigned char ch : value) {
        switch (ch) {
        case '"': escaped << "\\\""; break;
        case '\\': escaped << "\\\\"; break;
        case '\b': escaped << "\\b"; break;
        case '\f': escaped << "\\f"; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default:
            if (ch < 0x20) {
                escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<unsigned>(ch) << std::dec;
            } else {
                escaped << static_cast<char>(ch);
            }
        }
    }
    escaped << '"';
    return escaped.str();
}

void SimTrace::EnsureOpen() const
{
    if (closed_) {
        throw std::logic_error("trace is closed");
    }
}

void SimTrace::BeginEvent()
{
    EnsureOpen();
    if (!firstEvent_) {
        *output_ << ',';
    }
    firstEvent_ = false;
}

void SimTrace::RegisterQueue(TraceableQueue& queue, const std::string& path)
{
    EnsureOpen();
    if (path.empty()) {
        throw std::invalid_argument("queue trace path must not be empty");
    }
    for (const auto& entry : queues_) {
        if (entry.queue == &queue || entry.path == path) {
            throw std::invalid_argument("queue trace registration must be unique");
        }
    }
    const std::uint64_t tid = queues_.size() + 1;
    queues_.push_back(RegisteredQueue{&queue, path, tid});
    BeginEvent();
    *output_ << "{\"ph\":\"M\",\"name\":\"thread_name\",\"pid\":1,"
             << "\"tid\":" << tid << ",\"args\":{\"name\":"
             << Quote(path) << "}}";
}

std::uint64_t SimTrace::ModuleLane(const std::string& module)
{
    EnsureOpen();
    if (module.empty()) {
        throw std::invalid_argument("module trace name must not be empty");
    }
    const auto found = moduleLanes_.find(module);
    if (found != moduleLanes_.end()) {
        return found->second;
    }
    const std::uint64_t tid = moduleLanes_.size() + 1;
    moduleLanes_.emplace(module, tid);
    BeginEvent();
    *output_ << "{\"ph\":\"M\",\"name\":\"thread_name\",\"pid\":2,"
             << "\"tid\":" << tid << ",\"args\":{\"name\":"
             << Quote(module) << "}}";
    return tid;
}

void SimTrace::Capture(std::uint64_t cycle)
{
    EnsureOpen();
    if (cycle % sampleEvery_ != 0) {
        return;
    }
    for (const auto& entry : queues_) {
        const QueueSnapshot snapshot = entry.queue->Snapshot();
        BeginEvent();
        *output_ << "{\"name\":\"queue_depth\",\"cat\":\"queue\","
                 << "\"ph\":\"C\",\"pid\":1,\"tid\":" << entry.tid
                 << ",\"ts\":" << cycle << ",\"args\":{\"cycle\":"
                 << cycle << ",\"visible\":" << snapshot.visible.size()
                 << ",\"staged\":" << snapshot.staged.size()
                 << ",\"total\":"
                 << snapshot.visible.size() + snapshot.staged.size() << "}}";

        BeginEvent();
        *output_ << "{\"name\":\"queue_snapshot\",\"cat\":\"queue\","
                 << "\"ph\":\"i\",\"s\":\"t\",\"pid\":1,\"tid\":"
                 << entry.tid << ",\"ts\":" << cycle << ",\"args\":{"
                 << "\"cycle\":" << cycle << ",\"queue\":" << Quote(entry.path)
                 << ",\"capacity\":" << snapshot.capacity
                 << ",\"stalled\":" << (snapshot.stalled ? "true" : "false")
                 << ",\"visible\":[";
        for (std::size_t index = 0; index < snapshot.visible.size(); ++index) {
            if (index != 0) *output_ << ',';
            *output_ << Quote(snapshot.visible[index]);
        }
        *output_ << "],\"staged\":[";
        for (std::size_t index = 0; index < snapshot.staged.size(); ++index) {
            if (index != 0) *output_ << ',';
            *output_ << Quote(snapshot.staged[index]);
        }
        *output_ << "],\"delays\":[";
        for (std::size_t index = 0; index < snapshot.delays.size(); ++index) {
            if (index != 0) *output_ << ',';
            *output_ << snapshot.delays[index];
        }
        *output_ << "]}}";
    }
}

void SimTrace::AddDuration(const std::string& module, const std::string& name,
                           std::uint64_t startCycle, std::uint64_t endCycle,
                           const std::string& detail)
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    if (endCycle < startCycle) {
        throw std::invalid_argument("trace duration ends before it starts");
    }
    const std::uint64_t tid = ModuleLane(module);
    BeginEvent();
    *output_ << "{\"name\":" << Quote(name) << ",\"cat\":\"module\","
             << "\"ph\":\"X\",\"pid\":2,\"tid\":" << tid
             << ",\"ts\":" << startCycle << ",\"dur\":"
             << endCycle - startCycle << ",\"args\":{\"detail\":"
             << Quote(detail) << "}}";
}

void SimTrace::AddFlow(const std::string& fromModule,
                       const std::string& toModule,
                       std::uint64_t startCycle, std::uint64_t endCycle,
                       const std::string& name)
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    if (endCycle < startCycle) {
        throw std::invalid_argument("trace flow ends before it starts");
    }
    const std::uint64_t fromTid = ModuleLane(fromModule);
    const std::uint64_t toTid = ModuleLane(toModule);
    const std::uint64_t id = nextFlowId_++;
    BeginEvent();
    *output_ << "{\"name\":" << Quote(name) << ",\"cat\":\"flow\","
             << "\"ph\":\"s\",\"pid\":2,\"tid\":" << fromTid
             << ",\"ts\":" << startCycle << ",\"id\":" << id << '}';
    BeginEvent();
    *output_ << "{\"name\":" << Quote(name) << ",\"cat\":\"flow\","
             << "\"ph\":\"f\",\"pid\":2,\"tid\":" << toTid
             << ",\"ts\":" << endCycle << ",\"id\":" << id << '}';
}

void SimTrace::Close()
{
    if (closed_) {
        return;
    }
    *output_ << "]}";
    output_->flush();
    closed_ = true;
}

} // namespace parallel_gfsim
