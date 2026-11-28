#pragma once

#include <cstdint>

namespace parallel_gfsim {

struct RID {
    std::uint32_t value = 0;
    bool wrap = false;
    bool valid = false;

    RID() = default;
    RID(std::uint32_t index, bool wrapped)
        : value(index), wrap(wrapped), valid(true) {}
};

inline bool operator==(const RID& left, const RID& right)
{
    return left.value == right.value && left.wrap == right.wrap &&
           left.valid == right.valid;
}

inline bool operator!=(const RID& left, const RID& right)
{
    return !(left == right);
}

} // namespace parallel_gfsim
