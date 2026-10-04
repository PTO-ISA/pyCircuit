#pragma once
#include "simulator.hpp"

namespace gfsim {
// The helper entry and instance are immutable wiring. Queue/Signal inputs are
// declared before freeze(); runtime evaluates the static DAG after Queue Xfer.
template <class T> class Signal final : public SignalBase {
    using Helper = T (*)(void *);
    void *object_;
    Helper helper_;
    std::optional<T> value_;
    bool evaluate() override {
        T next = helper_(object_);
        bool changed = !value_ || !(*value_ == next);
        value_ = std::move(next);
        return changed;
    }

  public:
    Signal(void *object, Helper helper) : object_(object), helper_(helper) {
        if (!object || !helper)
            throw std::invalid_argument("null Signal helper");
    }
    const T &value() const {
        if (!value_)
            throw std::logic_error("Signal read before initialization");
        return *value_;
    }
};
} // namespace gfsim
