#pragma once
#include <gfsim/queue.hpp>
#include <gfsim/signal.hpp>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

namespace ac_detail {
template<class T, class U> constexpr T cast(U value) {
    if constexpr (std::is_same_v<T, bool>) return value != 0;
    else if constexpr (std::is_signed_v<T>) {
        const auto bits = static_cast<std::make_unsigned_t<T>>(value);
        T result;
        std::memcpy(&result, &bits, sizeof(T));
        return result;
    }
    else return static_cast<T>(value);
}
template<class T> using Unsigned = std::make_unsigned_t<T>;
template<class T> std::uint64_t bits(T value) { return static_cast<Unsigned<T>>(value); }
template<class T> T add(T a, T b) { return cast<T>(bits(a) + bits(b)); }
template<class T> T sub(T a, T b) { return cast<T>(bits(a) - bits(b)); }
template<class T> T mul(T a, T b) { return cast<T>(bits(a) * bits(b)); }
template<class T> T neg(T a) { return cast<T>(std::uint64_t{0} - bits(a)); }
template<class T> T shl(T a, T b) {
    const auto count = bits(b);
    return count >= sizeof(T) * 8 ? T{0} : cast<T>(bits(a) << count);
}
template<class T> T shr(T a, T b) {
    const auto count = bits(b);
    return count >= sizeof(T) * 8 ? (a < 0 ? cast<T>(-1) : T{0}) : cast<T>(a >> count);
}
template<class T> T div(T a, T b) {
    if (!b) throw std::domain_error("ACPy division by zero");
    if constexpr (std::is_signed_v<T>) {
        if (a == std::numeric_limits<T>::min() && b == -1) return a;
        const T q = a / b, r = a % b;
        return r && ((a < 0) != (b < 0)) ? sub(q, T{1}) : q;
    } else return a / b;
}
template<class T> T mod(T a, T b) {
    if (!b) throw std::domain_error("ACPy modulo by zero");
    if constexpr (std::is_signed_v<T>) {
        if (a == std::numeric_limits<T>::min() && b == -1) return 0;
        const T r = a % b;
        return r && ((a < 0) != (b < 0)) ? add(r, b) : r;
    } else return a % b;
}
inline std::vector<std::uint32_t> range(std::uint32_t n) {
    std::vector<std::uint32_t> values(n);
    for (std::uint32_t i = 0; i < n; ++i) values[i] = i;
    return values;
}
template<class T> struct QueueArray {
    std::vector<std::unique_ptr<gfsim::Queue<T>>> storage;
    std::vector<gfsim::Queue<T>*> refs;
    template<class Values, class Init> QueueArray(const Values& values, std::size_t capacity, Init init) {
        for (auto value : values) {
            storage.push_back(std::make_unique<gfsim::Queue<T>>(capacity, std::vector<T>{init(value)}));
            refs.push_back(storage.back().get());
        }
    }
};
// Proposal bookkeeping only: scheduling, dirtiness and commits belong to GFSim.
// Comparing runtime identities also deduplicates aliased input parameters.
template<std::size_t N> struct Pops {
    std::array<gfsim::QueueBase*, N> queues{};
    std::size_t count{};
    template<class T> void add(gfsim::Queue<T>* queue, gfsim::RuleId rule) {
        for (std::size_t i = 0; i < count; ++i) if (queues[i] == queue) return;
        queues[count++] = queue;
        queue->proposePop(rule);
    }
};
}
