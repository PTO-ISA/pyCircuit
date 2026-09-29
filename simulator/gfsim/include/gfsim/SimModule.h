#ifndef GFSIM_SIMMODULE_H
#define GFSIM_SIMMODULE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace gfsim {

class SimSystem;

class SimModule {
public:
  explicit SimModule(std::string name) : name_(std::move(name)) {}
  virtual ~SimModule() = default;

  std::string_view name() const noexcept { return name_; }

private:
  friend class SimSystem;

  virtual void Build() = 0;
  virtual void Work(std::uint64_t epoch) = 0;
  virtual void Xfer() noexcept = 0;
  virtual void DiscardNext() noexcept = 0;
  virtual void Reset() noexcept = 0;
  virtual void ReportStat() = 0;
  virtual bool HasWork() const noexcept = 0;

  std::string name_;
};

} // namespace gfsim

#endif // GFSIM_SIMMODULE_H
