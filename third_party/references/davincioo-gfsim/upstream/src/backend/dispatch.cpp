#include "backend/dispatch.hpp"

namespace davincioo::backend {

Dispatch::Dispatch(std::size_t scalar_count, std::size_t vec_count, std::size_t cube_count, std::size_t tma_count)
    : Module<Dispatch, PTOInstRef>("dispatch"),
      scalar_count_(scalar_count),
      vec_count_(vec_count),
      cube_count_(cube_count),
      tma_count_(tma_count) {}

void Dispatch::BuildSelf() {
  GFSIM_ASSERT(scalar_count_ + vec_count_ + cube_count_ + tma_count_ > 0);
}

void Dispatch::ResetSelf() {
  vec_rr_ = 0;
  cube_rr_ = 0;
  tma_rr_ = 0;
}

std::size_t Dispatch::SelectOutputIndex(PTOEngineKind kind) {
  switch (kind) {
    case PTOEngineKind::Scalar: {
      if (scalar_count_ == 0) {
        return static_cast<std::size_t>(-1);
      }
      return 0;
    }
    case PTOEngineKind::Vec: {
      if (vec_count_ == 0) {
        return static_cast<std::size_t>(-1);
      }
      const std::size_t output = 1 + (vec_rr_ % vec_count_);
      vec_rr_ = (vec_rr_ + 1) % vec_count_;
      return output;
    }
    case PTOEngineKind::Cube: {
      if (cube_count_ == 0) {
        return static_cast<std::size_t>(-1);
      }
      const std::size_t output = 1 + vec_count_ + (cube_rr_ % cube_count_);
      cube_rr_ = (cube_rr_ + 1) % cube_count_;
      return output;
    }
    case PTOEngineKind::Tma: {
      if (tma_count_ == 0) {
        return static_cast<std::size_t>(-1);
      }
      const std::size_t output = 1 + vec_count_ + cube_count_ + (tma_rr_ % tma_count_);
      tma_rr_ = (tma_rr_ + 1) % tma_count_;
      return output;
    }
    default:
      return 0;
  }
}

void Dispatch::SetDispatchWidth(std::size_t width) {
  GFSIM_ASSERT(width > 0);
  dispatch_width_ = width;
}

bool Dispatch::DispatchOne() {
  INPUT(inst_in, 0);
  if (inst_in->Empty()) {
    return false;
  }
  PTOInstRef inst = inst_in->Front();
  GFSIM_ASSERT(inst != nullptr);
  const std::size_t output_index = SelectOutputIndex(inst->engine_kind);
  if (output_index == static_cast<std::size_t>(-1)) {
    inst->dispatched = true;
    inst->runtime_latency = 0;
    inst->timestamps.dispatch_cycle = CurrentCycle();
    inst_in->Pop();
    MarkProgress();
    return true;
  }

  OUTPUT(out0, output_index);
  if (out0->Full()) {
    return false;
  }

  PTOInstRef dispatched = inst_in->Read();
  dispatched->dispatched = true;
  dispatched->timestamps.dispatch_cycle = CurrentCycle();
  out0->Write(dispatched);
  MarkProgress();
  return true;
}

void Dispatch::WorkSelf() {
  for (std::size_t i = 0; i < dispatch_width_; ++i) {
    if (!DispatchOne()) {
      break;
    }
  }
}

}  // namespace davincioo::backend
