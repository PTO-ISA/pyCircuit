#pragma once

#include <string>

#include "davincioo/model/framework.hpp"
#include "davincioo/model/pto_inst.hpp"
namespace davincioo::backend {

class Engine : public SimObject {
public:
  Engine(PTOEngineKind kind, std::size_t engine_count, std::string name);
  virtual ~Engine() = default;

  void BindIssuedInput(SimQueue<PTOInstRef>* queue);
  void BindCompletedOutput(SimQueue<PTOInstRef>* queue);

  PTOEngineKind Kind() const;
  std::size_t Capacity() const;
  std::size_t Count() const;

  void Build() override;
  void Reset() override;
  void Work() override;
  void Xfer() override;

protected:
  static std::size_t TileElementCount(const PTOTileReg& reg);
  static std::size_t TileByteSizeCeil(const PTOTileReg& reg);
  static std::size_t CeilDiv(std::size_t num, std::size_t den);
  static std::size_t DTypeBits(PTODType dtype);

  virtual std::size_t LookupLatency(const PTOInst& inst) const = 0;

  PTOEngineKind kind_ = PTOEngineKind::Scalar;
  std::size_t engine_count_ = 0;
  std::uint64_t engine_id_ = 0;
  SimQueue<PTOInstRef>* issued_in_ = nullptr;
  SimQueue<PTOInstRef>* completed_out_ = nullptr;
  SimQueue<PTOInstRef> execute_q_;
};

}  // namespace davincioo::backend
