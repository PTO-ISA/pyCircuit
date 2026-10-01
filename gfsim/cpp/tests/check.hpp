#pragma once
#include <stdexcept>
#include <string>
#define CHECK(...)                                                                                 \
    do {                                                                                           \
        if (!(__VA_ARGS__))                                                                        \
            throw std::runtime_error(std::string("check failed: ") + #__VA_ARGS__ + " at " +       \
                                     __FILE__ + ":" + std::to_string(__LINE__));                   \
    } while (false)
template <class Exception, class F> void throws(F &&f) {
    bool caught = false;
    try {
        f();
    } catch (const Exception &) {
        caught = true;
    }
    CHECK(caught);
}
