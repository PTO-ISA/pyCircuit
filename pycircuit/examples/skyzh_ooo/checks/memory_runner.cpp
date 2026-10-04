#include "model.hpp"
#include <cstdint>
#include <iostream>
using namespace ac_generated;
int main() {
    unsigned cases = 0;
    // Byte slicing is checked independently using unsigned host arithmetic.
    for (auto word : {0U, 0x80ff1234U, 0xffffffffU})
        for (auto width : {1U, 2U, 4U}) for (unsigned address = 0; address < 4; address += width)
            for (bool is_unsigned : {false, true}) {
                MemoryHelpers model(word, address, width, 0x9876abcdU, is_unsigned);
                model.sim.step();
                const auto mask = width == 4 ? UINT32_MAX : (1U << (width * 8)) - 1;
                auto value = (word >> (address * 8)) & mask;
                if (!is_unsigned && width != 4 && (value & (1U << (width * 8 - 1)))) value |= ~mask;
                const auto shifted = mask << (address * 8);
                const auto stored = (word & ~shifted) | ((0x9876abcdU << (address * 8)) & shifted);
                if (model.loaded.peek() != value || model.stored.peek() != stored) return 1;
                ++cases;
            }
    std::cout << "{\"memory_helper_cases\":" << cases << "}\n";
}
