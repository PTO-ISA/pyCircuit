#ifndef GFSIM_SIMMODULE_H
#define GFSIM_SIMMODULE_H

#include "gfsim/SimObj.h"

namespace gfsim {

// Concrete empty module. A generated definition adds typed ports, Work rule
// methods and owned child instances. Storage leaf definitions override the
// same lifecycle and own their state. Parents explicitly visit their children;
// the base adds no connection registry, scheduler or implicit storage.
class SimModule : public SimObj {
public:
  using SimObj::SimObj;

  void Build() override {}
  void Work() override {}
  void Xfer() noexcept override {}
  void DiscardNext() noexcept override {}
  void Reset() noexcept override {}
  void ReportStat() override {}
  bool HasWork() const noexcept override { return false; }
};

} // namespace gfsim

#endif // GFSIM_SIMMODULE_H
