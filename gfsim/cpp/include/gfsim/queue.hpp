#pragma once
#include "simulator.hpp"
#include <type_traits>

namespace gfsim {
template <auto Member, auto... Rest, class Object> decltype(auto) fieldAt(Object &object) {
    if constexpr (sizeof...(Rest) == 0)
        return (object.*Member);
    else
        return fieldAt<Rest...>(object.*Member);
}
template <class T> class Queue final : public QueueBase {
    friend struct TestAccess;
    struct Slot {
        RuleId source{};
        unsigned allowed{};
        enum Status { Empty, Pending, Accepted } status{Empty};
        bool pop{};
        std::optional<T> push;
        std::vector<std::function<bool(T &)>> revises;
        void clear() {
            pop = false;
            push.reset();
            revises.clear();
            status = Empty;
        }
    };
    std::vector<std::optional<T>> data_;
    std::size_t head_{}, count_{};
    std::vector<Slot> slots_;
    std::vector<std::size_t> accepted_;
    bool acceptedPop_{}, frozen_{}, registerOnly_{};
    Slot &slot(RuleId r) { return const_cast<Slot &>(std::as_const(*this).slot(r)); }
    const Slot &slot(RuleId r) const {
        auto it = std::lower_bound(slots_.begin(), slots_.end(), r,
                                   [](const Slot &s, RuleId id) { return s.source < id; });
        if (it == slots_.end() || it->source != r)
            throw std::logic_error("undeclared Queue source");
        return *it;
    }
    Slot &proposal(RuleId r, unsigned op) {
        auto &s = slot(r);
        if (!(s.allowed & op))
            throw std::logic_error("undeclared Queue operation");
        prepare(r, op != Push);
        s.status = Slot::Pending;
        return s;
    }
    void registerSource(RuleId r, unsigned op) override {
        if (frozen_)
            throw std::logic_error("Queue is frozen");
        if (registerOnly_ && (op & (Pop | Push)))
            throw std::logic_error("register only permits revise");
        for (auto &s : slots_)
            if (s.source == r) {
                s.allowed |= op;
                return;
            }
        Slot s;
        s.source = r;
        s.allowed = op;
        slots_.push_back(std::move(s));
    }
    void freeze() override {
        registerSource(0, 0);
        std::sort(slots_.begin(), slots_.end(),
                  [](const Slot &a, const Slot &b) { return a.source < b.source; });
        accepted_.reserve(slots_.size());
        frozen_ = true;
    }
    bool acceptedPopFor(RuleId r) const override {
        const auto &s = slot(r);
        return s.status == Slot::Accepted && s.pop;
    }
    bool pendingPop(RuleId r) const override {
        const auto &s = slot(r);
        return s.status == Slot::Pending && s.pop;
    }
    bool pendingPush(RuleId r) const override {
        const auto &s = slot(r);
        return s.status == Slot::Pending && s.push.has_value();
    }
    bool pushSpace(RuleId r) const override {
        return !full() || acceptedPop_ || (slot(r).pop && !empty());
    }
    bool canAccept(RuleId r) const override {
        const auto &s = slot(r);
        return s.status == Slot::Pending && (!(s.pop || !s.revises.empty()) || !empty()) &&
               (!s.push || pushSpace(r));
    }
    void accept(RuleId r) override {
        auto &s = slot(r);
        s.status = Slot::Accepted;
        acceptedPop_ |= s.pop;
        accepted_.push_back(static_cast<std::size_t>(&s - slots_.data()));
    }
    void cancel(RuleId r) override {
        auto &s = slot(r);
        if (s.status == Slot::Accepted)
            throw std::logic_error("cannot cancel accepted proposal");
        s.clear();
    }
    bool xfer() override {
        bool modified = false;
        for (auto index : accepted_)
            for (const auto &revise : slots_[index].revises)
                modified |= revise(*data_[(head_ + count_ - 1) % capacity()]);
        if (acceptedPop_) {
            data_[head_].reset();
            head_ = (head_ + 1) % capacity();
            --count_;
            modified = true;
        }
        for (auto index : accepted_) {
            auto &s = slots_[index];
            if (s.push) {
                data_[(head_ + count_) % capacity()] = std::move(s.push);
                ++count_;
                modified = true;
            }
            s.clear();
        }
        if (modified)
            changed();
        accepted_.clear();
        acceptedPop_ = false;
        return modified;
    }

  public:
    explicit Queue(std::size_t capacity = 1, std::vector<T> initial = {}, bool registerOnly = false)
        : data_(capacity), count_(initial.size()), registerOnly_(registerOnly) {
        if (!capacity || initial.size() > capacity ||
            (registerOnly && (capacity != 1 || initial.size() != 1)))
            throw std::invalid_argument("invalid Queue capacity or register initial state");
        for (std::size_t i = 0; i < initial.size(); ++i)
            data_[i] = std::move(initial[i]);
    }
    std::size_t capacity() const override { return data_.size(); }
    std::size_t size() const override { return count_; }
    std::size_t sourceCount() const override { return slots_.size(); }
    const T *tryPeek() const { return empty() ? nullptr : &*data_[head_]; }
    const T &peek() const {
        if (empty())
            throw std::logic_error("peek on empty Queue; generated code must abort explicitly");
        return *data_[head_];
    }
    const T &at(std::size_t i) const {
        if (i >= count_)
            throw std::out_of_range("Queue current index");
        return *data_[(head_ + i) % capacity()];
    }
    void proposePop(RuleId r) {
        auto &s = proposal(r, Pop);
        if (s.pop)
            throw std::logic_error("duplicate pop");
        s.pop = true;
    }
    void proposePush(RuleId r, T value) {
        auto &s = proposal(r, Push);
        if (s.push)
            throw std::logic_error("duplicate push");
        s.push = std::move(value);
    }
    template <auto... Path, class Value> void proposeRevise(RuleId r, Value value) {
        auto &s = proposal(r, Revise);
        auto save = [&]() {
            if constexpr (sizeof...(Path) == 0)
                return T(std::move(value));
            else
                return std::remove_cvref_t<decltype(fieldAt<Path...>(std::declval<T &>()))>(
                    std::move(value));
        }();
        s.revises.emplace_back([saved = std::move(save)](T &target) {
            if constexpr (sizeof...(Path) == 0) {
                bool changed = !(target == saved);
                target = saved;
                return changed;
            } else {
                auto &field = fieldAt<Path...>(target);
                bool changed = !(field == saved);
                field = saved;
                return changed;
            }
        });
    }
};
} // namespace gfsim
