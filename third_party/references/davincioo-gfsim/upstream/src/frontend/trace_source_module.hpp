#pragma once

#include <deque>
#include <vector>

#include "davincioo/model/framework.hpp"
#include "davincioo/model/pto_inst.hpp"

namespace davincioo::frontend {

class TraceSourceModule : public Module<TraceSourceModule, PTOInstRef> {
public:
  TraceSourceModule();

  void LoadTrace(std::vector<PTOInst> insts);
  bool Done() const;
  std::size_t SourceCount() const;

  void SetFetchWidth(std::size_t width);
  std::size_t FetchWidth() const noexcept { return fetch_width_; }

protected:
  void ResetSelf() override;
  void WorkSelf() override;

private:
  std::vector<PTOInst> source_insts_;
  std::deque<PTOInst> pending_insts_;
  std::size_t fetch_width_ = 1;
};

}  // namespace davincioo::frontend
