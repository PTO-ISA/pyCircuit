#include "model_top/core.hpp"

#include <algorithm>
#include <sstream>

namespace davincioo::model_top {

namespace {

std::size_t CountTotalEngines(const CoreConfig& config) {
  return config.scalar.count + config.vec.count + config.cube.count + config.tma.count;
}

template <class EngineT, class CostConfigT>
void BuildEngineArray(
    std::vector<std::unique_ptr<backend::Engine>>& engines,
    EngineConfig config,
    CostConfigT cost_config,
    const std::string& prefix) {
  engines.clear();
  engines.reserve(config.count);
  for (std::size_t index = 0; index < config.count; ++index) {
    engines.push_back(std::make_unique<EngineT>(config, cost_config, prefix + std::to_string(index)));
  }
}

void BuildPerEngineQueues(
    std::vector<SimQueue<PTOInstRef>>& queues,
    std::size_t count,
    const std::string& prefix,
    std::uint32_t latency = 0) {
  queues.clear();
  queues.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    queues.emplace_back(8, latency, prefix + std::to_string(index));
  }
}

void AppendIssueQueues(
    std::vector<std::unique_ptr<backend::IssueQueue>>& issue_queues,
    IssueQueueConfig config,
    std::size_t count,
    PTOEngineKind kind,
    const std::string& prefix) {
  for (std::size_t index = 0; index < count; ++index) {
    issue_queues.push_back(std::make_unique<backend::IssueQueue>(config, kind, prefix + std::to_string(index)));
  }
}

void MergeChildProgress(SimObject& parent, SimObject& child) {
  const std::size_t amount = child.ConsumeProgress();
  if (amount != 0) {
    parent.MarkProgress(amount);
  }
}

}  // namespace

Core::Core(CoreConfig config)
    : SimObject("core"),
      frontend_(),
      rob_(config.rob),
      rename_(config.rename),
      dispatch_(config.scalar.count, config.vec.count, config.cube.count, config.tma.count),
      ready_table_(config.scalar.count + config.vec.count + config.cube.count + config.tma.count),
      rob_input_q_(std::max<std::size_t>(1, config.frontend_width.fetch_width), 0, "rob_input_q"),
      rob_to_rename_q_(std::max<std::size_t>(1, config.frontend_width.rob_issue_width), 0, "rob_to_rename_q"),
      rob_to_rename_retire_q_(config.rob.entries, 0, "rob_to_rename_retire_q"),
      rename_to_dispatch_q_(std::max<std::size_t>(1, config.frontend_width.rename_width), 0, "rename_to_dispatch_q"),
      wakeup_q_(CountTotalEngines(config), 0, "wakeup_q"),
      rob_done_q_(CountTotalEngines(config), 0, "rob_done_q") {
  frontend_.SetFetchWidth(config.frontend_width.fetch_width);
  rob_.SetAllocWidth(config.frontend_width.rob_alloc_width);
  rob_.SetIssueWidth(config.frontend_width.rob_issue_width);
  rename_.SetRenameWidth(config.frontend_width.rename_width);
  dispatch_.SetDispatchWidth(config.frontend_width.dispatch_width);
  const std::size_t total_engines = CountTotalEngines(config);
  BuildPerEngineQueues(dispatch_to_issue_qs_, total_engines, "dispatch_to_issue_q");
  BuildPerEngineQueues(ready_to_issue_qs_, total_engines, "ready_to_issue_q");
  BuildPerEngineQueues(issue_wakeup_qs_, total_engines, "issue_wakeup_q");
  BuildPerEngineQueues(issue_to_engine_qs_, total_engines, "issue_to_engine_q", 1);
  BuildEngineArray<backend::Scalar>(scalar_engines_, config.scalar, config.scalar_cost, "scalar_engine");
  BuildEngineArray<backend::Vector>(vec_engines_, config.vec, config.vec_cost, "vec_engine");
  BuildEngineArray<backend::Cube>(cube_engines_, config.cube, config.cube_cost, "cube_engine");
  BuildEngineArray<backend::Tma>(tma_engines_, config.tma, config.tma_cost, "tma_engine");
  issue_queues_.clear();
  issue_queues_.reserve(total_engines);
  AppendIssueQueues(issue_queues_, config.issue_queue, config.scalar.count, PTOEngineKind::Scalar, "scalar_iq");
  AppendIssueQueues(issue_queues_, config.issue_queue, config.vec.count, PTOEngineKind::Vec, "vec_iq");
  AppendIssueQueues(issue_queues_, config.issue_queue, config.cube.count, PTOEngineKind::Cube, "cube_iq");
  AppendIssueQueues(issue_queues_, config.issue_queue, config.tma.count, PTOEngineKind::Tma, "tma_iq");
}

void Core::LoadTrace(std::vector<PTOInst> insts) {
  frontend_.LoadTrace(std::move(insts));
}

std::size_t Core::SourceCount() const {
  return frontend_.SourceCount();
}

bool Core::Done() const {
  const bool engines_empty =
      std::all_of(scalar_engines_.begin(), scalar_engines_.end(), [](const auto& engine) { return engine->Count() == 0; }) &&
      std::all_of(vec_engines_.begin(), vec_engines_.end(), [](const auto& engine) { return engine->Count() == 0; }) &&
      std::all_of(cube_engines_.begin(), cube_engines_.end(), [](const auto& engine) { return engine->Count() == 0; }) &&
      std::all_of(tma_engines_.begin(), tma_engines_.end(), [](const auto& engine) { return engine->Count() == 0; });
  const bool issue_queues_empty =
      std::all_of(issue_queues_.begin(), issue_queues_.end(), [](const auto& issue_queue) { return issue_queue->Count() == 0; });
  return frontend_.Done() && rob_input_q_.Empty() && rob_to_rename_q_.Empty() && rob_to_rename_retire_q_.Empty() &&
         rename_to_dispatch_q_.Empty() && wakeup_q_.Empty() && rob_done_q_.Empty() &&
         std::all_of(dispatch_to_issue_qs_.begin(), dispatch_to_issue_qs_.end(), [](const auto& q) { return q.Empty(); }) &&
         std::all_of(ready_to_issue_qs_.begin(), ready_to_issue_qs_.end(), [](const auto& q) { return q.Empty(); }) &&
         std::all_of(issue_wakeup_qs_.begin(), issue_wakeup_qs_.end(), [](const auto& q) { return q.Empty(); }) &&
         std::all_of(issue_to_engine_qs_.begin(), issue_to_engine_qs_.end(), [](const auto& q) { return q.Empty(); }) &&
         rob_.Empty() && issue_queues_empty && engines_empty;
}

const std::vector<PTOInst>& Core::Processed() const {
  return rob_.Processed();
}

std::size_t Core::RobCapacity() const {
  return rob_.Capacity();
}

std::size_t Core::RobCount() const {
  return rob_.Count();
}

std::size_t Core::ScalarEngineCount() const {
  return scalar_engines_.size();
}

std::size_t Core::VecEngineCount() const {
  return vec_engines_.size();
}

std::size_t Core::CubeEngineCount() const {
  return cube_engines_.size();
}

std::size_t Core::TmaEngineCount() const {
  return tma_engines_.size();
}

std::string Core::DumpRobState() const {
  return rob_.DumpState();
}

std::string Core::DumpSchedulerState() const {
  std::ostringstream stream;
  stream << ready_table_.DumpState();
  for (const auto& issue_queue : issue_queues_) {
    stream << "\n" << issue_queue->DumpState();
  }
  return stream.str();
}

void Core::Build() {
  if (built_) {
    return;
  }
  const std::size_t total_engines =
      scalar_engines_.size() + vec_engines_.size() + cube_engines_.size() + tma_engines_.size();
  GFSIM_ASSERT(total_engines > 0);
  GFSIM_ASSERT(dispatch_to_issue_qs_.size() == total_engines);
  GFSIM_ASSERT(ready_to_issue_qs_.size() == total_engines);
  GFSIM_ASSERT(issue_wakeup_qs_.size() == total_engines);
  GFSIM_ASSERT(issue_to_engine_qs_.size() == total_engines);
  GFSIM_ASSERT(issue_queues_.size() == total_engines);

  frontend_.BindOutput(0, &rob_input_q_);
  rob_.BindInstInput(&rob_input_q_);
  rob_.BindCompletionInput(&rob_done_q_);
  rob_.BindRenameOutput(&rob_to_rename_q_);
  rob_.BindRetireOutput(&rob_to_rename_retire_q_);
  rename_.BindInput(0, &rob_to_rename_q_);
  rename_.BindInput(1, &rob_to_rename_retire_q_);
  rename_.BindOutput(0, &rename_to_dispatch_q_);
  dispatch_.BindInput(0, &rename_to_dispatch_q_);

  std::size_t output_index = 0;
  for (std::size_t index = 0; index < dispatch_to_issue_qs_.size(); ++index) {
    dispatch_.BindOutput(output_index++, &dispatch_to_issue_qs_[index]);
    ready_table_.BindDispatchInput(index, &dispatch_to_issue_qs_[index]);
    ready_table_.BindIssueEnqueueOutput(index, &ready_to_issue_qs_[index]);
    ready_table_.BindIssueWakeupOutput(index, &issue_wakeup_qs_[index]);
    issue_queues_[index]->BindEnqueueInput(&ready_to_issue_qs_[index]);
    issue_queues_[index]->BindWakeupInput(&issue_wakeup_qs_[index]);
    issue_queues_[index]->BindIssuedOutput(&issue_to_engine_qs_[index]);
    issue_queues_[index]->BindReadyChecker([this](std::uint64_t tag) { return ready_table_.IsTagReady(tag); });
  }
  GFSIM_ASSERT(output_index == total_engines);
  ready_table_.BindWakeupInput(&wakeup_q_);
  ready_table_.BindRobCompletionOutput(&rob_done_q_);

  std::size_t engine_index = 0;
  for (std::size_t index = 0; index < scalar_engines_.size(); ++index, ++engine_index) {
    scalar_engines_[index]->BindIssuedInput(&issue_to_engine_qs_[engine_index]);
    scalar_engines_[index]->BindCompletedOutput(&wakeup_q_);
  }
  for (std::size_t index = 0; index < vec_engines_.size(); ++index, ++engine_index) {
    vec_engines_[index]->BindIssuedInput(&issue_to_engine_qs_[engine_index]);
    vec_engines_[index]->BindCompletedOutput(&wakeup_q_);
  }
  for (std::size_t index = 0; index < cube_engines_.size(); ++index, ++engine_index) {
    cube_engines_[index]->BindIssuedInput(&issue_to_engine_qs_[engine_index]);
    cube_engines_[index]->BindCompletedOutput(&wakeup_q_);
  }
  for (std::size_t index = 0; index < tma_engines_.size(); ++index, ++engine_index) {
    tma_engines_[index]->BindIssuedInput(&issue_to_engine_qs_[engine_index]);
    tma_engines_[index]->BindCompletedOutput(&wakeup_q_);
  }
  GFSIM_ASSERT(engine_index == total_engines);

  frontend_.Build();
  rob_.Build();
  rename_.Build();
  dispatch_.Build();
  ready_table_.Build();
  for (auto& issue_queue : issue_queues_) {
    issue_queue->Build();
  }
  for (auto& engine : scalar_engines_) {
    engine->Build();
  }
  for (auto& engine : vec_engines_) {
    engine->Build();
  }
  for (auto& engine : cube_engines_) {
    engine->Build();
  }
  for (auto& engine : tma_engines_) {
    engine->Build();
  }
  built_ = true;
}

void Core::Reset() {
  rob_input_q_.Reset();
  rob_to_rename_q_.Reset();
  rob_to_rename_retire_q_.Reset();
  rename_to_dispatch_q_.Reset();
  wakeup_q_.Reset();
  rob_done_q_.Reset();
  for (auto& queue : dispatch_to_issue_qs_) {
    queue.Reset();
  }
  for (auto& queue : ready_to_issue_qs_) {
    queue.Reset();
  }
  for (auto& queue : issue_wakeup_qs_) {
    queue.Reset();
  }
  for (auto& queue : issue_to_engine_qs_) {
    queue.Reset();
  }
  frontend_.Reset();
  rob_.Reset();
  rename_.Reset();
  dispatch_.Reset();
  ready_table_.Reset();
  for (auto& issue_queue : issue_queues_) {
    issue_queue->Reset();
  }
  for (auto& engine : scalar_engines_) {
    engine->Reset();
  }
  for (auto& engine : vec_engines_) {
    engine->Reset();
  }
  for (auto& engine : cube_engines_) {
    engine->Reset();
  }
  for (auto& engine : tma_engines_) {
    engine->Reset();
  }
}

void Core::Report() {
  frontend_.Report();
  rob_.Report();
  rename_.Report();
  dispatch_.Report();
  ready_table_.Report();
  rob_input_q_.Report();
  rob_to_rename_q_.Report();
  rob_to_rename_retire_q_.Report();
  rename_to_dispatch_q_.Report();
  wakeup_q_.Report();
  rob_done_q_.Report();
  for (auto& queue : dispatch_to_issue_qs_) {
    queue.Report();
  }
  for (auto& queue : ready_to_issue_qs_) {
    queue.Report();
  }
  for (auto& queue : issue_wakeup_qs_) {
    queue.Report();
  }
  for (auto& queue : issue_to_engine_qs_) {
    queue.Report();
  }
  for (auto& issue_queue : issue_queues_) {
    issue_queue->Report();
  }
  for (auto& engine : scalar_engines_) {
    engine->Report();
  }
  for (auto& engine : vec_engines_) {
    engine->Report();
  }
  for (auto& engine : cube_engines_) {
    engine->Report();
  }
  for (auto& engine : tma_engines_) {
    engine->Report();
  }
}

void Core::Work() {
  frontend_.SetCurrentCycle(CurrentCycle());
  rob_.SetCurrentCycle(CurrentCycle());
  rename_.SetCurrentCycle(CurrentCycle());
  dispatch_.SetCurrentCycle(CurrentCycle());
  rob_input_q_.SetCurrentCycle(CurrentCycle());
  rob_to_rename_q_.SetCurrentCycle(CurrentCycle());
  rob_to_rename_retire_q_.SetCurrentCycle(CurrentCycle());
  rename_to_dispatch_q_.SetCurrentCycle(CurrentCycle());
  wakeup_q_.SetCurrentCycle(CurrentCycle());
  rob_done_q_.SetCurrentCycle(CurrentCycle());
  ready_table_.SetCurrentCycle(CurrentCycle());
  for (auto& queue : dispatch_to_issue_qs_) {
    queue.SetCurrentCycle(CurrentCycle());
  }
  for (auto& queue : ready_to_issue_qs_) {
    queue.SetCurrentCycle(CurrentCycle());
  }
  for (auto& queue : issue_wakeup_qs_) {
    queue.SetCurrentCycle(CurrentCycle());
  }
  for (auto& queue : issue_to_engine_qs_) {
    queue.SetCurrentCycle(CurrentCycle());
  }
  for (auto& issue_queue : issue_queues_) {
    issue_queue->SetCurrentCycle(CurrentCycle());
  }
  for (auto& engine : scalar_engines_) {
    engine->SetCurrentCycle(CurrentCycle());
  }
  for (auto& engine : vec_engines_) {
    engine->SetCurrentCycle(CurrentCycle());
  }
  for (auto& engine : cube_engines_) {
    engine->SetCurrentCycle(CurrentCycle());
  }
  for (auto& engine : tma_engines_) {
    engine->SetCurrentCycle(CurrentCycle());
  }

  if (!rob_.Full()) {
    frontend_.Work();
    MergeChildProgress(*this, frontend_);
  }
  rob_input_q_.Work();
  MergeChildProgress(*this, rob_input_q_);
  rob_.Work();
  MergeChildProgress(*this, rob_);
  rob_to_rename_q_.Work();
  MergeChildProgress(*this, rob_to_rename_q_);
  rob_to_rename_retire_q_.Work();
  MergeChildProgress(*this, rob_to_rename_retire_q_);
  rename_.Work();
  MergeChildProgress(*this, rename_);
  rename_to_dispatch_q_.Work();
  MergeChildProgress(*this, rename_to_dispatch_q_);
  dispatch_.Work();
  MergeChildProgress(*this, dispatch_);
  for (auto& queue : dispatch_to_issue_qs_) {
    queue.Work();
    MergeChildProgress(*this, queue);
  }
  wakeup_q_.Work();
  MergeChildProgress(*this, wakeup_q_);
  ready_table_.Work();
  MergeChildProgress(*this, ready_table_);
  for (auto& queue : ready_to_issue_qs_) {
    queue.Work();
    MergeChildProgress(*this, queue);
  }
  for (auto& queue : issue_wakeup_qs_) {
    queue.Work();
    MergeChildProgress(*this, queue);
  }
  rob_done_q_.Work();
  MergeChildProgress(*this, rob_done_q_);
  for (auto& issue_queue : issue_queues_) {
    issue_queue->Work();
    MergeChildProgress(*this, *issue_queue);
  }
  for (auto& queue : issue_to_engine_qs_) {
    queue.Work();
    MergeChildProgress(*this, queue);
  }
  for (auto& engine : scalar_engines_) {
    engine->Work();
    MergeChildProgress(*this, *engine);
  }
  for (auto& engine : vec_engines_) {
    engine->Work();
    MergeChildProgress(*this, *engine);
  }
  for (auto& engine : cube_engines_) {
    engine->Work();
    MergeChildProgress(*this, *engine);
  }
  for (auto& engine : tma_engines_) {
    engine->Work();
    MergeChildProgress(*this, *engine);
  }
}

void Core::Xfer() {
  frontend_.Xfer();
  rob_.Xfer();
  rename_.Xfer();
  dispatch_.Xfer();
  ready_table_.Xfer();
  rob_input_q_.Xfer();
  rob_to_rename_q_.Xfer();
  rob_to_rename_retire_q_.Xfer();
  rename_to_dispatch_q_.Xfer();
  wakeup_q_.Xfer();
  rob_done_q_.Xfer();
  for (auto& queue : dispatch_to_issue_qs_) {
    queue.Xfer();
  }
  for (auto& queue : ready_to_issue_qs_) {
    queue.Xfer();
  }
  for (auto& queue : issue_wakeup_qs_) {
    queue.Xfer();
  }
  for (auto& queue : issue_to_engine_qs_) {
    queue.Xfer();
  }
  for (auto& issue_queue : issue_queues_) {
    issue_queue->Xfer();
  }
  for (auto& engine : scalar_engines_) {
    engine->Xfer();
  }
  for (auto& engine : vec_engines_) {
    engine->Xfer();
  }
  for (auto& engine : cube_engines_) {
    engine->Xfer();
  }
  for (auto& engine : tma_engines_) {
    engine->Xfer();
  }
}

void Core::PrintPipeView(std::ostream& os) const {
  os << "rob_count=" << rob_.Count()
     << " rob_capacity=" << rob_.Capacity()
     << " frontend_done=" << (frontend_.Done() ? 1 : 0)
     << " q_rob_in=" << rob_input_q_.Occupancy()
     << " q_rob_rename=" << rob_to_rename_q_.Occupancy()
     << " q_rename_retire=" << rob_to_rename_retire_q_.Occupancy()
     << " q_rename_dispatch=" << rename_to_dispatch_q_.Occupancy()
     << " q_wakeup=" << wakeup_q_.Occupancy()
     << " q_rob_done=" << rob_done_q_.Occupancy()
     << "\n";
}

}  // namespace davincioo::model_top
