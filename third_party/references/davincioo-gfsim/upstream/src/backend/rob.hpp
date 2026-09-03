#pragma once

#include <string>
#include <vector>

#include "davincioo/model/config.hpp"
#include "davincioo/model/framework.hpp"
#include "davincioo/model/pto_inst.hpp"
#include "davincioo/model/rob.hpp"

namespace davincioo::backend {

class ROB : public SimObject {
public:
  explicit ROB(ROBConfig config);

  void BindInstInput(SimQueue<PTOInstRef>* queue);
  void BindCompletionInput(SimQueue<PTOInstRef>* queue);
  void BindRenameOutput(SimQueue<PTOInstRef>* queue);
  void BindRetireOutput(SimQueue<PTOInstRef>* queue);

  const std::vector<PTOInst>& Processed() const;
  const std::vector<ROBEntry>& Entries() const;
  std::string DumpState() const;
  std::size_t Capacity() const;
  std::size_t Count() const;
  bool Full() const;
  bool Empty() const;

  void Build() override;
  void Reset() override;
  void Work() override;
  void Xfer() override;

  void SetAllocWidth(std::size_t width);
  void SetIssueWidth(std::size_t width);
  std::size_t AllocWidth() const noexcept { return alloc_width_; }
  std::size_t IssueWidth() const noexcept { return issue_width_; }

private:
  bool CanAlloc() const;
  void Allocate(PTOInstRef inst);
  void ConsumeCompletion(const PTOInstRef& inst);
  void AllocateMultipleEntries(std::size_t max_count);
  void IssueMultipleEntries(std::size_t max_count);
  void RetireHead();

  ROBConfig config_;
  SimQueue<PTOInstRef>* inst_in_ = nullptr;
  SimQueue<PTOInstRef>* done_in_ = nullptr;
  SimQueue<PTOInstRef>* rename_out_ = nullptr;
  SimQueue<PTOInstRef>* retire_out_ = nullptr;
  std::vector<ROBEntry> entries_;
  std::vector<PTOInst> processed_;
  std::size_t head_ = 0;
  std::size_t tail_ = 0;
  std::size_t count_ = 0;
  std::uint64_t next_rid_ = 0;
  std::size_t alloc_width_ = 1;
  std::size_t issue_width_ = 1;
};

}  // namespace davincioo::backend
