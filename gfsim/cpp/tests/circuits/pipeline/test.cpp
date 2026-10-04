#include "../../check.hpp"
#include "model.hpp"
void testLongPipeline() {
    using namespace circuits;
    constexpr int length = 1100;
    for (bool reverse : {false, true}) {
        auto c = pipeline({7}, length, 1, 0, {}, true, 1, reverse);
        for (int i = 0; i < length + 5; ++i)
            c->sim->step();
        auto &output = dynamic_cast<Queue<Receipt<Value, 1>> &>(*c->output);
        CHECK(output.size() == length + 2);
        for (int i = length; i >= 0; --i)
            CHECK(output.at(length - i).values[0] == Value{-i - 1, 100 + length});
        CHECK(output.at(length + 1).values[0] == Value{0, 7 + length});
        if (!reverse)
            CHECK(c->sim->stats().deltaRounds > 1000);
        CHECK(c->sim->events().empty());
    }
}
