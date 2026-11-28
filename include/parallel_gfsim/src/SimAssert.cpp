#include "parallel_gfsim/SimAssert.h"

namespace parallel_gfsim {

namespace {
std::string FormatAssertion(const char* expression, const char* file,
                            int line, const char* function,
                            const std::string& detail)
{
    std::ostringstream message;
    message << "assertion failed: " << expression << " at " << file << ':'
            << line << " in " << function;
    if (!detail.empty()) {
        message << ": " << detail;
    }
    return message.str();
}
}

AssertionError::AssertionError(const char* expression, const char* file,
                               int line, const char* function,
                               const std::string& detail)
    : std::runtime_error(FormatAssertion(expression, file, line, function, detail))
{
}

AssertStream::~AssertStream() noexcept(false)
{
    if (!passed_) {
        throw AssertionError(expression_, file_, line_, function_, detail_.str());
    }
}

} // namespace parallel_gfsim
