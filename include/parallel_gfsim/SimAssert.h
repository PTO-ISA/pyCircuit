#pragma once

#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace parallel_gfsim {

class AssertionError : public std::runtime_error {
public:
    AssertionError(const char* expression, const char* file, int line,
                   const char* function, const std::string& detail);
};

class AssertStream {
public:
    AssertStream(bool passed, const char* expression, const char* file,
                 int line, const char* function)
        : passed_(passed), expression_(expression), file_(file),
          line_(line), function_(function) {}

    AssertStream(const AssertStream&) = delete;
    AssertStream& operator=(const AssertStream&) = delete;
    ~AssertStream() noexcept(false);

    template <typename T>
    AssertStream& operator<<(const T& value)
    {
        if (!passed_) {
            detail_ << value;
        }
        return *this;
    }

    AssertStream& operator<<(std::ostream& (*manipulator)(std::ostream&))
    {
        if (!passed_) {
            manipulator(detail_);
        }
        return *this;
    }

private:
    bool passed_;
    const char* expression_;
    const char* file_;
    int line_;
    const char* function_;
    std::ostringstream detail_;
};

} // namespace parallel_gfsim

#define PARALLEL_GFSIM_ASSERT(condition) \
    ::parallel_gfsim::AssertStream(static_cast<bool>(condition), #condition, \
                          __FILE__, __LINE__, __func__)
