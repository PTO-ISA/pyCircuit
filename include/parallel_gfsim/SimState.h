#pragma once

#include <string>
#include <type_traits>
#include <utility>

#include "SimSystem.h"

namespace parallel_gfsim {

// One logical value, staged during Work and committed by its owner in Xfer.
// Different workers may write different instances, never the same instance.
template <typename T>
class SimState final : public SimObj {
    static_assert(std::is_copy_constructible<T>::value,
                  "SimState values must be copy constructible");
    static_assert(std::is_copy_assignable<T>::value,
                  "SimState values must be copy assignable");

public:
    SimState(std::string name, const T& initialValue)
        : SimObj(std::move(name)), initial_(initialValue), current_(initialValue),
          pending_(initialValue), written_(false) {}

    T Read() const { return current_; }
    void Write(const T& value) { pending_ = value; written_ = true; }

    void Build() override { SetObjectCapacity(0); }
    void Work() override {}
    void Xfer() override
    {
        if (written_) {
            current_ = pending_;
            written_ = false;
        }
    }
    void Reset() override
    {
        current_ = initial_;
        written_ = false;
    }
    void ReportStat() override {}

private:
    T initial_;
    T current_;
    T pending_;
    bool written_;
};

} // namespace parallel_gfsim
