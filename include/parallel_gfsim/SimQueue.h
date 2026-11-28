#pragma once
#ifndef PARALLEL_GFSIM_SIM_QUEUE_H
#define PARALLEL_GFSIM_SIM_QUEUE_H

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "SimSystem.h"
#include "SimAssert.h"
#include "SimTrace.h"
#include "RID.h"

namespace parallel_gfsim {
namespace detail {
template <typename T> struct IsSharedPtr : std::false_type {};
template <typename T> struct IsSharedPtr<std::shared_ptr<T>> : std::true_type {};

template <typename T>
auto PacketRID(const T& value, int) -> decltype(RID(value.rid))
{
    return RID(value.rid);
}

template <typename T>
RID PacketRID(const T&, long)
{
    throw std::logic_error("queue payload needs a RID extractor");
}

template <typename T, bool Direct> struct QueuePayloadPolicy;

template <typename T> struct QueuePayloadPolicy<T, true> {
    using Packet = T;
    static const T& Object(const Packet& packet, const std::string&) { return packet; }
    static void Validate(const Packet&, const std::string&) {}
};

template <typename T> struct QueuePayloadPolicy<T, false> {
    using Packet = std::shared_ptr<T>;
    static const T& Object(const Packet& packet, const std::string& name)
    {
        PARALLEL_GFSIM_ASSERT(packet != nullptr) << name << " contains a null packet";
        return *packet;
    }
    static void Validate(const Packet& packet, const std::string& name)
    {
        PARALLEL_GFSIM_ASSERT(packet != nullptr) << name << " cannot accept a null packet";
    }
};
}

template <class T>
class SimQueue : public SimObj, public TraceableQueue {
    static constexpr bool kDirect = std::is_arithmetic<T>::value ||
                                    std::is_enum<T>::value;
    static_assert(kDirect || std::is_class<T>::value,
                  "SimQueue<T> requires a scalar, enum, class, or struct payload");
    static_assert(!detail::IsSharedPtr<T>::value,
                  "Use SimQueue<T>, not SimQueue<std::shared_ptr<T>>");
    using Policy = detail::QueuePayloadPolicy<T, kDirect>;

public:
    using Value = T;
    using Packet = typename Policy::Packet;

private:
    static constexpr uint32_t kCapacityLimit = 512;
    uint32_t maxQSize = kCapacityLimit;
    // Occupied slots at the last boundary plus successful writes since then.
    uint32_t writeOccupancy = 0;
    std::deque<Packet> writeQ;
    std::deque<Packet> readQ;
    // Only the consumer writes this during Work; readers inspect readQ only.
    bool popPending_ = false;
    std::deque<uint32_t> delayQ;
    uint32_t latency = 1;
    bool stall;
    std::function<std::string(const T&)> traceFormatter_;
    std::function<RID(const T&)> ridExtractor_;

public:
    explicit SimQueue(std::string name = "SimQueue")
        : SimObj(std::move(name)), stall(false) {};
    virtual ~SimQueue() {};

    void Build() override {};

    void SetTraceFormatter(std::function<std::string(const T&)> formatter)
    {
        if (!formatter) {
            throw std::invalid_argument("queue trace formatter must not be empty");
        }
        traceFormatter_ = std::move(formatter);
    }

    void SetRidExtractor(std::function<RID(const T&)> extractor)
    {
        if (!extractor) {
            throw std::invalid_argument("queue RID extractor must not be empty");
        }
        ridExtractor_ = std::move(extractor);
    }

    QueueSnapshot Snapshot() const override
    {
        QueueSnapshot snapshot;
        snapshot.capacity = maxQSize;
        snapshot.stalled = stall;
        for (const auto& item : readQ) {
            snapshot.visible.push_back(FormatTraceValue(Policy::Object(item, Name())));
        }
        for (const auto& item : writeQ) {
            snapshot.staged.push_back(FormatTraceValue(Policy::Object(item, Name())));
        }
        snapshot.delays.assign(delayQ.begin(), delayQ.end());
        return snapshot;
    }

    // Requests are issued by modules during Work; the queue itself has no work.
    void Work() override {}

    void Xfer() override
    {
        // Commit and publish only after all module Work has completed.
        if (popPending_) {
            PARALLEL_GFSIM_ASSERT(!readQ.empty()) << Name() << " pending pop from empty queue";
            readQ.pop_front();
            popPending_ = false;
        }
        // Publishing entries changes their visibility, not total occupancy.
        PARALLEL_GFSIM_ASSERT(readQ.size() + writeQ.size() <= maxQSize)
            << Name() << " capacity exceeded";
        writeOccupancy = Size() + SizeW();
        if (stall) {
            return;
        }
        PARALLEL_GFSIM_ASSERT(writeQ.size() == delayQ.size())
            << Name() << " staged data/delay metadata are misaligned";
        for (auto it = delayQ.begin(); it != delayQ.end(); ++it) {
            if (*it != 0) {
                --(*it);
            }
        }
        while (!writeQ.empty() && readQ.size() < maxQSize && delayQ.front() == 0) {
            readQ.push_back(std::move(writeQ.front()));
            writeQ.pop_front();
            if (!delayQ.empty()) {
                delayQ.pop_front();
            }
        }
    };

    void Reset() override
    {
        writeQ.clear();
        readQ.clear();
        delayQ.clear();
        writeOccupancy = 0;
        popPending_ = false;
        stall = false;
    }
    void Setlatency(uint32_t d)
    {
        latency = d;
    }
    void ReportStat() override {}

    bool toBeOverflow(uint32_t reserve) const { return Full(reserve); }
    bool getStall() const { return stall; }
    void setStall() { stall = true; }
    void unsetStall() { stall = false; }

    SimSystem* GetSim() override
    {
        PARALLEL_GFSIM_ASSERT(false) << "SimQueue has no direct SimSystem owner";
        return nullptr;
    };

    void Write(Packet packet)
    {
        PARALLEL_GFSIM_ASSERT(!Full()) << Name() << " write to full queue";
        Policy::Validate(packet, Name());
        writeQ.push_back(std::move(packet));
        try {
            delayQ.emplace_back(latency);
        } catch (...) {
            writeQ.pop_back();
            throw;
        }
        ++writeOccupancy;
    };

    Packet Read()
    {
        PARALLEL_GFSIM_ASSERT(!readQ.empty()) << Name() << " read from empty queue";
        PARALLEL_GFSIM_ASSERT(!popPending_) << Name() << " multiple consumes in one cycle";
        Packet result = readQ.front();
        popPending_ = true;
        return result;
    };

    const Packet& Front() const
    {
        PARALLEL_GFSIM_ASSERT(!readQ.empty()) << Name() << " peek from empty queue";
        return readQ.front();
    };

    const T& Peek() const
    {
        return Policy::Object(Front(), Name());
    }

    void Pop()
    {
        PARALLEL_GFSIM_ASSERT(!readQ.empty()) << Name() << " pop from empty queue";
        PARALLEL_GFSIM_ASSERT(!popPending_) << Name() << " multiple consumes in one cycle";
        popPending_ = true;
    }

    bool Empty() const
    {
        return readQ.empty();
    };

    bool WRBothEmpty() const
    {
        return readQ.empty() && writeQ.empty();
    }

    bool Full() const
    {
        return Full(0);
    };
    bool Full(uint32_t reserve) const
    {
        return uint64_t{writeOccupancy} + reserve >= maxQSize;
    };

    uint32_t Size() const
    {
        return readQ.size();
    };

    uint32_t SizeW() const
    {
        return writeQ.size();
    };

    int MaxSize() const
    {
        return maxQSize;
    }

    std::deque<Packet> &GetRawWriteData()
    {
        return writeQ;
    };
    std::deque<Packet> &GetRawReadData()
    {
        return readQ;
    };
    std::deque<uint32_t> &GetRawDelayCycle()
    {
        return delayQ;
    };

    template <class Function>
    typename std::enable_if<
        !std::is_same<typename std::decay<Function>::type, RID>::value,
        std::size_t>::type
    FlushIf(Function&& match)
    {
        PARALLEL_GFSIM_ASSERT(!popPending_) << Name() << " flush with pending consumption";
        PARALLEL_GFSIM_ASSERT(writeQ.size() == delayQ.size())
            << Name() << " staged data/delay metadata are misaligned";
        std::size_t removed = 0;
        auto delay = delayQ.begin();
        for (auto it = writeQ.begin(); it != writeQ.end();) {
            if (match(*it)) {
                delay = delayQ.erase(delay);
                it = writeQ.erase(it);
                ++removed;
            } else {
                ++it;
                ++delay;
            }
        }

        for (auto it = readQ.begin(); it != readQ.end();) {
            if (match(*it)) {
                it = readQ.erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
        return removed;
    }

    std::size_t FlushIf(const RID& target)
    {
        if (!target.valid) {
            throw std::invalid_argument("flush RID must be valid");
        }
        return FlushIf([&](const Packet& packet) {
            const T& value = Policy::Object(packet, Name());
            const RID packetRID = ridExtractor_
                ? ridExtractor_(value) : detail::PacketRID(value, 0);
            return packetRID == target;
        });
    }

    template <class Compare>
    std::size_t FlushIf(const RID& target, Compare shouldFlush)
    {
        if (!target.valid) {
            throw std::invalid_argument("flush RID must be valid");
        }
        return FlushIf([&](const Packet& packet) {
            const T& value = Policy::Object(packet, Name());
            const RID packetRID = ridExtractor_
                ? ridExtractor_(value) : detail::PacketRID(value, 0);
            return packetRID.valid && shouldFlush(packetRID, target);
        });
    }
    template <class Function>
    bool MatchIf(Function &match)
    {
        for (auto it = writeQ.cbegin(); it != writeQ.cend(); ++it) {
            if (match(*it)) {
                return true;
            }
        }
        for (auto it = readQ.cbegin(); it != readQ.cend(); ++it) {
            if (match(*it)) {
                return true;
            }
        }
        return false;
    }
    template <class Function>
    uint32_t MatchCount(const Function &match)
    {
        uint32_t cnt = 0;
        for (auto it = writeQ.cbegin(); it != writeQ.cend(); ++it) {
            if (match(*it)) {
                ++cnt;
            }
        }
        for (auto it = readQ.cbegin(); it != readQ.cend(); ++it) {
            if (match(*it)) {
                ++cnt;
            }
        }
        return cnt;
    }

    void InitMaxSize(uint32_t size)
    {
        PARALLEL_GFSIM_ASSERT(size > 0 && size <= kCapacityLimit)
            << Name() << " capacity must be in [1, 512]";
        PARALLEL_GFSIM_ASSERT(size >= writeQ.size() + readQ.size())
            << Name() << " capacity below current occupancy";
        maxQSize = size;
    }

private:
    std::string FormatTraceValue(const T& item) const
    {
        return traceFormatter_ ? traceFormatter_(item) : detail::DefaultPayload(item);
    }
};
} // namespace parallel_gfsim
#endif
