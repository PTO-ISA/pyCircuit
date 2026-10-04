#pragma once
#include <gfsim/queue.hpp>
#include <gfsim/signal.hpp>
#include <algorithm>
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
    template<class Values, class Init> QueueArray(const Values& values, std::size_t capacity, Init init, bool initialized = true) {
        for (auto value : values) {
            storage.push_back(std::make_unique<gfsim::Queue<T>>(capacity, initialized ? std::vector<T>{init(value)} : std::vector<T>{}));
            refs.push_back(storage.back().get());
        }
    }
};
// Borrow stable Module resource tables; own only explicit temporary lists.
// Copies across CFG edges, Rule parameters keep the same identities
// without copying a potentially large table of Queue pointers.
template<class T> class QueueRefs {
    using Pointer = gfsim::Queue<T>*;
    std::shared_ptr<const std::vector<Pointer>> owner;
    std::span<Pointer const> view;
public:
    QueueRefs() = default;
    explicit QueueRefs(const std::vector<Pointer>& resources) : view(resources) {}
    QueueRefs(std::initializer_list<Pointer> resources)
        : owner(std::make_shared<const std::vector<Pointer>>(resources)), view(*owner) {}
    std::size_t size() const { return view.size(); }
    Pointer at(std::size_t index) const {
        if (index >= view.size()) throw std::out_of_range("ACPy Queue array index");
        return view[index];
    }
    bool operator==(const QueueRefs& other) const {
        return view.size() == other.view.size() &&
            (view.data() == other.view.data() || std::equal(view.begin(), view.end(), other.view.begin()));
    }
};
// Proposal bookkeeping only: scheduling, dirtiness and commits belong to GFSim.
// Comparing runtime identities also deduplicates aliased input parameters.
struct Pops {
    std::vector<gfsim::QueueBase*> queues;
    template<class T> void add(gfsim::Queue<T>* queue, gfsim::RuleId rule) {
        for (std::size_t i = 0; i < queues.size(); ++i) if (queues[i] == queue) return;
        queues.push_back(queue);
        queue->proposePop(rule);
    }
};

template<class T, class... Args> T make(Args... args) { return T{args...}; }
template<class T> auto tryPeek(T* q) { return q->tryPeek(); }
template<class T> T load(const T* p) { return *p; }
template<class T> bool present(const T* p) { return p != nullptr; }
template<class T> auto peek(T* q) { return q->peek(); }
template<class T> auto signalValue(T* q) { return q->value(); }
template<class T> bool nonempty(T* q) { return !q->empty(); }
template<class T> bool empty(T* q) { return q->empty(); }
template<class T> bool full(T* q) { return q->full(); }
template<class T> std::uint32_t size(T* q) { return cast<std::uint32_t>(q->size()); }
template<class T> std::uint32_t length(const T& v) { return cast<std::uint32_t>(v.size()); }
template<class T, class I> auto index(const T& v, I i) { return v.at(i); }
template<class T, class V> void push(T* q, V value, gfsim::RuleId id) { q->proposePush(id, value); }
template<class T, class V, class Accessor, class... I> void revise(T* q, V value, gfsim::RuleId id, Accessor access, I... indices) {
    q->proposeReviseWith(id, [=](auto& target) { access(target, indices...) = value; });
}
inline void check(bool value) { if (!value) throw std::invalid_argument("ACPy assertion failed"); }
[[noreturn]] inline void unreachable() { throw std::logic_error("ACPy function reached end without a value"); }
}
