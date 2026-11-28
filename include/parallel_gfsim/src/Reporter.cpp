#include "parallel_gfsim/Reporter.h"

#include <iomanip>

namespace parallel_gfsim {

namespace {
const std::size_t kTitleWidth = 60;
const std::size_t kNameWidth = 49;
const std::size_t kValueWidth = 10;
}

void Reporter::ReportTitle(const std::string& title)
{
    const std::size_t fill = title.size() < kTitleWidth ? kTitleWidth - title.size() : 0;
    *output_ << std::string(fill / 2, '=') << title
             << std::string(fill - fill / 2, '=') << '\n';
}

void Reporter::WriteValue(const std::string& name, const std::string& value)
{
    std::ostringstream line;
    line << name;
    if (name.size() < kNameWidth) {
        line << std::string(kNameWidth - name.size(), '.');
    }
    line << ':' << std::right << std::setw(kValueWidth) << value << '\n';
    *output_ << line.str();
}

void Reporter::ReportVal(const std::string& name, double value, int precision)
{
    std::ostringstream formatted;
    formatted << std::fixed << std::setprecision(precision < 0 ? 0 : precision)
              << value;
    WriteValue(name, formatted.str());
}

std::string Reporter::FormatRatio(std::uint64_t numerator,
                                  std::uint64_t denominator, bool percent)
{
    if (denominator == 0) {
        return "N/A";
    }
    std::ostringstream formatted;
    const double scale = percent ? 100.0 : 1.0;
    formatted << std::fixed << std::setprecision(2)
              << (static_cast<double>(numerator) /
                  static_cast<double>(denominator)) * scale;
    if (percent) {
        formatted << '%';
    }
    return formatted.str();
}

void Reporter::ReportAvg(const std::string& name, std::uint64_t numerator,
                         std::uint64_t denominator)
{
    WriteValue(name, FormatRatio(numerator, denominator, false));
}

void Reporter::ReportPct(const std::string& name, std::uint64_t numerator,
                         std::uint64_t denominator)
{
    WriteValue(name, FormatRatio(numerator, denominator, true));
}

void Reporter::ReportValAndPct(const std::string& name, std::uint64_t numerator,
                               std::uint64_t denominator)
{
    WriteValue(name, std::to_string(numerator) + " (" +
                     FormatRatio(numerator, denominator, true) + ")");
}

} // namespace parallel_gfsim
