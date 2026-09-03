#include "backend/engine.hpp"

#include <algorithm>

namespace davincioo::backend {

Engine::Engine(PTOEngineKind kind, std::size_t engine_count, std::string name)
    : SimObject(std::move(name)),
      kind_(kind),
      engine_count_(engine_count),
      execute_q_(1, 1, Name() + "_execute_q") {
  const auto pos = Name().find_last_not_of("0123456789");
  if (pos != std::string::npos && pos + 1 < Name().size()) {
    engine_id_ = static_cast<std::uint64_t>(std::stoull(Name().substr(pos + 1)));
  }
}

void Engine::BindIssuedInput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  issued_in_ = queue;
}

void Engine::BindCompletedOutput(SimQueue<PTOInstRef>* queue) {
  GFSIM_ASSERT(queue != nullptr);
  completed_out_ = queue;
}

PTOEngineKind Engine::Kind() const {
  return kind_;
}

std::size_t Engine::Capacity() const {
  return 1;
}

std::size_t Engine::Count() const {
  return execute_q_.Occupancy();
}

std::size_t Engine::DTypeBits(PTODType dtype) {
  switch (dtype) {
    case PTODType::kBool:
      return 8;
    case PTODType::kInt8:
      return 8;
    case PTODType::kInt32:
      return 32;
    case PTODType::kInt64:
      return 64;
    case PTODType::kUInt64:
      return 64;
    case PTODType::kFloat16:
    case PTODType::kBFloat16:
      return 16;
    case PTODType::kFloat32:
      return 32;
    case PTODType::kFloat8:
      return 8;
    case PTODType::kFloat4:
      return 4;
    case PTODType::kUnknown:
    default:
      return 32;
  }
}

std::size_t Engine::CeilDiv(std::size_t num, std::size_t den) {
  GFSIM_ASSERT(den != 0);
  return (num + den - 1) / den;
}

std::size_t Engine::TileElementCount(const PTOTileReg& reg) {
  if (reg.shape.empty()) {
    return 0;
  }
  std::size_t elements = 1;
  for (const std::int64_t dim : reg.shape) {
    if (dim <= 0) {
      return 0;
    }
    elements *= static_cast<std::size_t>(dim);
  }
  return elements;
}

std::size_t Engine::TileByteSizeCeil(const PTOTileReg& reg) {
  const std::size_t elements = TileElementCount(reg);
  const std::size_t bits = DTypeBits(reg.dtype);
  return CeilDiv(elements * bits, 8);
}

void Engine::Build() {
  GFSIM_ASSERT(engine_count_ > 0);
  GFSIM_ASSERT(issued_in_ != nullptr);
  GFSIM_ASSERT(completed_out_ != nullptr);
  execute_q_.Build();
}

void Engine::Reset() {
  execute_q_.Reset();
}

void Engine::Work() {
  GFSIM_ASSERT(issued_in_ != nullptr);
  GFSIM_ASSERT(completed_out_ != nullptr);
  execute_q_.SetCurrentCycle(this->CurrentCycle());
  execute_q_.Work();
  this->MarkProgress(execute_q_.ConsumeProgress());

  if (!execute_q_.Empty()) {
    if (completed_out_->Full()) {
      return;
    }
    PTOInstRef completed = execute_q_.Read();
    GFSIM_ASSERT(completed != nullptr);
    completed->runtime_latency = 0;
    completed->timestamps.engine_complete_cycle = this->CurrentCycle();
    completed_out_->Write(completed);
    this->MarkProgress();
  }

  if (issued_in_->Empty()) {
    return;
  }

  const PTOInstRef& head = issued_in_->Front();
  GFSIM_ASSERT(head != nullptr);
  if (execute_q_.Full()) {
    return;
  }

  PTOInstRef inst = issued_in_->Read();
  GFSIM_ASSERT(inst != nullptr);
  // Clamp zero-byte tile latencies (e.g. valid_row=0 after batch padding)
  // to one cycle so the engine still progresses through the SimQueue. The
  // cost-model returns 0 when bytes / bandwidth + overhead is 0; that is
  // a legal pypto codegen pattern, not a config bug.
  const std::size_t latency = std::max<std::size_t>(LookupLatency(*inst), 1);
  inst->engine_id = engine_id_;
  inst->runtime_latency = latency;
  inst->timestamps.engine_pop_cycle = this->CurrentCycle();
  execute_q_.SetLatency(static_cast<std::uint32_t>(latency));
  execute_q_.Write(inst);
  this->MarkProgress();
}

void Engine::Xfer() {
  execute_q_.Xfer();
}

}  // namespace davincioo::backend
