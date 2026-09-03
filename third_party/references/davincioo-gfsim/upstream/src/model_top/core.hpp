#pragma once

#include <iosfwd>
#include <memory>
#include <vector>

#include "backend/dispatch.hpp"
#include "backend/cube.hpp"
#include "backend/issue_queue.hpp"
#include "backend/ready_table.hpp"
#include "backend/rename.hpp"
#include "backend/rob.hpp"
#include "backend/scalar.hpp"
#include "backend/tma.hpp"
#include "backend/vector.hpp"
#include "davincioo/model/config.hpp"
#include "davincioo/model/framework.hpp"
#include "davincioo/model/pto_inst.hpp"
#include "frontend/trace_source_module.hpp"

namespace davincioo::model_top {

class Core : public SimObject {
public:
  explicit Core(CoreConfig config);

  void LoadTrace(std::vector<PTOInst> insts);
  std::size_t SourceCount() const;
  bool Done() const;
  const std::vector<PTOInst>& Processed() const;
  std::size_t RobCapacity() const;
  std::size_t RobCount() const;
  std::size_t ScalarEngineCount() const;
  std::size_t VecEngineCount() const;
  std::size_t CubeEngineCount() const;
  std::size_t TmaEngineCount() const;
  std::string DumpRobState() const;
  std::string DumpSchedulerState() const;
  void PrintPipeView(std::ostream& os) const;

  void Build() override;
  void Reset() override;
  void Report() override;
  void Work() override;
  void Xfer() override;

private:
  frontend::TraceSourceModule frontend_;
  backend::ROB rob_;
  backend::Rename rename_;
  backend::Dispatch dispatch_;
  backend::ReadyTable ready_table_;
  SimQueue<PTOInstRef> rob_input_q_;
  SimQueue<PTOInstRef> rob_to_rename_q_;
  SimQueue<PTOInstRef> rob_to_rename_retire_q_;
  SimQueue<PTOInstRef> rename_to_dispatch_q_;
  SimQueue<PTOInstRef> wakeup_q_;
  SimQueue<PTOInstRef> rob_done_q_;
  std::vector<SimQueue<PTOInstRef>> dispatch_to_issue_qs_;
  std::vector<SimQueue<PTOInstRef>> ready_to_issue_qs_;
  std::vector<SimQueue<PTOInstRef>> issue_wakeup_qs_;
  std::vector<SimQueue<PTOInstRef>> issue_to_engine_qs_;
  std::vector<std::unique_ptr<backend::IssueQueue>> issue_queues_;
  std::vector<std::unique_ptr<backend::Engine>> scalar_engines_;
  std::vector<std::unique_ptr<backend::Engine>> vec_engines_;
  std::vector<std::unique_ptr<backend::Engine>> cube_engines_;
  std::vector<std::unique_ptr<backend::Engine>> tma_engines_;
  bool built_ = false;
};

}  // namespace davincioo::model_top
