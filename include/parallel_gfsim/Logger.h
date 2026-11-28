#pragma once

#include <cstdint>
#include <iostream>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

namespace parallel_gfsim {

enum class LogLevel { Debug, Info, Warn, Error, Off };

class Logger {
public:
    explicit Logger(std::ostream& output = std::clog) : output_(&output) {}

    void SetOutput(std::ostream& output) { output_ = &output; }
    void SetLevel(LogLevel level) { level_ = level; }
    LogLevel Level() const { return level_; }
    void SetCycle(std::uint64_t cycle) { cycle_ = cycle; }
    std::uint64_t Cycle() const { return cycle_; }

    class Line {
    public:
        Line(Logger& logger, LogLevel level, std::string unit, std::string stage);
        Line(const Line&) = delete;
        Line& operator=(const Line&) = delete;
        Line(Line&& other);
        ~Line();

        template <typename T>
        Line& operator<<(const T& value)
        {
            if (owner_ != nullptr) {
                message_ << value;
            }
            return *this;
        }

        Line& operator<<(std::ostream& (*manipulator)(std::ostream&))
        {
            if (owner_ != nullptr) {
                manipulator(message_);
            }
            return *this;
        }

    private:
        Logger* owner_;
        LogLevel level_;
        std::string unit_;
        std::string stage_;
        std::ostringstream message_;
    };

    Line Start(LogLevel level, std::string unit, std::string stage);
    Line Debug(std::string unit, std::string stage);
    Line Info(std::string unit, std::string stage);
    Line Warn(std::string unit, std::string stage);
    Line Error(std::string unit, std::string stage);

private:
    friend class Line;
    void Emit(LogLevel level, const std::string& unit,
              const std::string& stage, const std::string& message);

    std::ostream* output_; // non-owning
    std::mutex emitMutex_;
    LogLevel level_ = LogLevel::Warn;
    std::uint64_t cycle_ = 0;
};

} // namespace parallel_gfsim
