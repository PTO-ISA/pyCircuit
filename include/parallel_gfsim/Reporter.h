#pragma once

#include <cstdint>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>

namespace parallel_gfsim {

class Reporter {
public:
    explicit Reporter(std::ostream& output = std::cout) : output_(&output) {}

    void SetOutput(std::ostream& output) { output_ = &output; }

    void ReportTitle(const std::string& title);

    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, void>::type
    ReportVal(const std::string& name, T value)
    {
        std::ostringstream formatted;
        formatted << value;
        WriteValue(name, formatted.str());
    }

    void ReportVal(const std::string& name, double value, int precision = 2);
    void ReportAvg(const std::string& name, std::uint64_t numerator,
                   std::uint64_t denominator);
    void ReportPct(const std::string& name, std::uint64_t numerator,
                   std::uint64_t denominator);
    void ReportValAndPct(const std::string& name, std::uint64_t numerator,
                         std::uint64_t denominator);

private:
    void WriteValue(const std::string& name, const std::string& value);
    static std::string FormatRatio(std::uint64_t numerator,
                                   std::uint64_t denominator, bool percent);

    std::ostream* output_; // non-owning
};

} // namespace parallel_gfsim
