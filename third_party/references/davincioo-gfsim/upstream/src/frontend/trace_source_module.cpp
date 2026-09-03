#include "frontend/trace_source_module.hpp"

#include "davincioo/model/framework.hpp"

namespace davincioo::frontend {

TraceSourceModule::TraceSourceModule() : Module<TraceSourceModule, PTOInstRef>("trace_source") {}

void TraceSourceModule::LoadTrace(std::vector<PTOInst> insts) {
  source_insts_ = std::move(insts);
}

bool TraceSourceModule::Done() const {
  return pending_insts_.empty();
}

std::size_t TraceSourceModule::SourceCount() const {
  return source_insts_.size();
}

void TraceSourceModule::SetFetchWidth(std::size_t width) {
  GFSIM_ASSERT(width > 0);
  fetch_width_ = width;
}

void TraceSourceModule::ResetSelf() {
  pending_insts_.clear();
  for (const PTOInst& inst : source_insts_) {
    pending_insts_.push_back(inst);
  }
}

void TraceSourceModule::WorkSelf() {
  OUTPUT(out0, 0);
  std::size_t emitted = 0;
  while (emitted < fetch_width_ && !pending_insts_.empty() && !out0->Full()) {
    out0->Write(std::make_shared<PTOInst>(pending_insts_.front()));
    pending_insts_.pop_front();
    ++emitted;
  }
}

}  // namespace davincioo::frontend
