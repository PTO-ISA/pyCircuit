// SPDX-License-Identifier: BSD-3-Clause
#ifndef GFSIM_SIMOBJ_H
#define GFSIM_SIMOBJ_H

#include <string>
#include <string_view>
#include <utility>

namespace gfsim {

// Common lifecycle for modules and hardware primitives. No scheduling or state
// storage is added here; the owning parent directly calls its child objects.
class SimObj {
public:
  explicit SimObj(std::string name) : name_(std::move(name)) {}
  virtual ~SimObj() = default;

  std::string_view name() const noexcept { return name_; }

  virtual void Build() = 0;
  virtual void Work() = 0;
  virtual void Xfer() noexcept = 0;
  virtual void DiscardNext() noexcept = 0;
  virtual void Reset() noexcept = 0;
  virtual void ReportStat() = 0;
  virtual bool HasWork() const noexcept = 0;

private:
  std::string name_;
};

} // namespace gfsim

#endif // GFSIM_SIMOBJ_H
