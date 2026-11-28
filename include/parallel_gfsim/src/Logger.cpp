#include "parallel_gfsim/Logger.h"

namespace parallel_gfsim {

namespace {
const char* LevelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warn: return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Off: return "OFF";
    }
    return "UNKNOWN";
}
}

Logger::Line::Line(Logger& logger, LogLevel level, std::string unit,
                   std::string stage)
    : owner_(level != LogLevel::Off && level >= logger.level_ &&
                 logger.level_ != LogLevel::Off
                 ? &logger : nullptr),
      level_(level), unit_(std::move(unit)), stage_(std::move(stage))
{
}

Logger::Line::Line(Line&& other)
    : owner_(other.owner_), level_(other.level_),
      unit_(std::move(other.unit_)), stage_(std::move(other.stage_)),
      message_(std::move(other.message_))
{
    other.owner_ = nullptr;
}

Logger::Line::~Line()
{
    if (owner_ != nullptr) {
        owner_->Emit(level_, unit_, stage_, message_.str());
    }
}

Logger::Line Logger::Start(LogLevel level, std::string unit, std::string stage)
{
    return Line(*this, level, std::move(unit), std::move(stage));
}

Logger::Line Logger::Debug(std::string unit, std::string stage)
{
    return Start(LogLevel::Debug, std::move(unit), std::move(stage));
}

Logger::Line Logger::Info(std::string unit, std::string stage)
{
    return Start(LogLevel::Info, std::move(unit), std::move(stage));
}

Logger::Line Logger::Warn(std::string unit, std::string stage)
{
    return Start(LogLevel::Warn, std::move(unit), std::move(stage));
}

Logger::Line Logger::Error(std::string unit, std::string stage)
{
    return Start(LogLevel::Error, std::move(unit), std::move(stage));
}

void Logger::Emit(LogLevel level, const std::string& unit,
                  const std::string& stage, const std::string& message)
{
    std::lock_guard<std::mutex> lock(emitMutex_);
    *output_ << "[C:" << cycle_ << "][" << unit << '.' << stage << "]["
             << LevelName(level) << "] " << message << '\n';
}

} // namespace parallel_gfsim
