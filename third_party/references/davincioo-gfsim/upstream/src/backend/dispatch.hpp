#pragma once

#include <cstddef>

#include "davincioo/model/framework.hpp"
#include "davincioo/model/pto_inst.hpp"

namespace davincioo::backend {

class Dispatch : public Module<Dispatch, PTOInstRef> {
public:
  Dispatch(std::size_t scalar_count, std::size_t vec_count, std::size_t cube_count, std::size_t tma_count);

  void SetDispatchWidth(std::size_t width);
  std::size_t DispatchWidth() const noexcept { return dispatch_width_; }

protected:
  void BuildSelf() override;
  void ResetSelf() override;
  void WorkSelf() override;

private:
  bool DispatchOne();
  std::size_t SelectOutputIndex(PTOEngineKind kind);

  std::size_t scalar_count_ = 0;
  std::size_t vec_count_ = 0;
  std::size_t cube_count_ = 0;
  std::size_t tma_count_ = 0;
  std::size_t vec_rr_ = 0;
  std::size_t cube_rr_ = 0;
  std::size_t tma_rr_ = 0;
  std::size_t dispatch_width_ = 1;
};

}  // namespace davincioo::backend
