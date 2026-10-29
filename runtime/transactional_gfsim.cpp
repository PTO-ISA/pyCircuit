// SPDX-License-Identifier: BSD-3-Clause
#include "pycircuit/transactional_gfsim/runtime.h"

#include <limits>

namespace pycircuit::transactional_gfsim {

void Resource::RequireWork(const Context &context) const {
  if (context.phase_ != Context::Phase::Work || owner_ != &context.owner_)
    throw std::logic_error("resource proposal requires its owning System Work");
}

void Context::RequireEvaluation() const {
  if (phase_ != Phase::Comb && phase_ != Phase::Work && phase_ != Phase::Check)
    throw std::logic_error("transaction diagnostic outside evaluation");
}

void Context::Fail(std::string message) {
  RequireEvaluation();
  if (!failed_) {
    failed_ = true;
    failure_ = std::move(message);
  }
}

void Context::Assert(bool condition, std::string_view message) {
  RequireEvaluation();
  if (!condition)
    Fail(std::string(message));
}

void Context::Assert(const ::gfsim::FourState<1> &condition,
                     std::string_view message) {
  RequireEvaluation();
  if (!condition.isFullyKnown() || !condition.value().toBool())
    Fail(std::string(message));
}

void Context::Log(std::string message) {
  if (phase_ != Phase::Work)
    throw std::logic_error("transaction logs require Work");
  logs_.push_back(std::move(message));
}

bool CheckedControl(const ::gfsim::FourState<1> &value) {
  if (!value.isFullyKnown())
    throw ::gfsim::FourStateViolation("transaction control must be known 0 or 1");
  return value.value().toBool();
}

System::System() : context_(*this) {}

System::~System() {
  for (auto *resource : resources_)
    resource->owner_ = nullptr;
  for (auto *module : modules_)
    module->owner_ = nullptr;
}

void System::RequireIdle() const {
  if (context_.phase_ != Context::Phase::Idle)
    throw std::logic_error("System operation requires an idle System");
}

void System::Register(Resource &resource) {
  RequireIdle();
  if (resource.owner_)
    throw std::logic_error("resource already has a System owner");
  resources_.push_back(&resource);
  resource.owner_ = this;
}

void System::Register(Module &module) {
  RequireIdle();
  if (module.owner_)
    throw std::logic_error("module already has a System owner");
  modules_.push_back(&module);
  module.owner_ = this;
}

void System::SetLogSink(LogSink sink) {
  RequireIdle();
  sink_ = std::move(sink);
}

void System::Discard() noexcept {
  for (auto *resource : resources_)
    resource->Discard();
  context_.logs_.clear();
}

bool System::Step() { return Sample(true); }

bool System::Sample(bool transfer, bool reset) {
  RequireIdle();
  context_.failed_ = false;
  context_.failure_.clear();
  context_.exception_ = nullptr;
  context_.logs_.clear();
  publishException_ = nullptr;
  try {
    if (cycle_ == std::numeric_limits<std::uint64_t>::max())
      throw std::overflow_error("System cycle counter exhausted");
    context_.phase_ = Context::Phase::Begin;
    for (auto *resource : resources_)
      resource->Begin();
    context_.phase_ = Context::Phase::Comb;
    for (auto *module : modules_)
      module->CombWork(context_);
    context_.phase_ = Context::Phase::Work;
    for (auto *module : modules_)
      module->Work(context_);
    context_.phase_ = Context::Phase::Check;
    for (auto *module : modules_)
      module->Check(context_);
    for (auto *resource : resources_)
      resource->Check(context_);
  } catch (...) {
    context_.failed_ = true;
    context_.exception_ = std::current_exception();
  }
  if (context_.failed_) {
    Discard();
    context_.phase_ = Context::Phase::Idle;
    return false;
  }

  context_.phase_ = Context::Phase::Xfer;
  for (auto *resource : resources_) {
    if (!transfer)
      resource->Discard();
    else if (reset)
      resource->Reset();
    else
      resource->Xfer();
  }
  ++cycle_;
  context_.phase_ = Context::Phase::Publish;
  if (sink_) {
    try {
      for (const auto &message : context_.logs_)
        sink_(message);
    } catch (...) {
      publishException_ = std::current_exception();
    }
  }
  context_.logs_.clear();
  context_.phase_ = Context::Phase::Idle;
  return true;
}

void System::Reset() {
  RequireIdle();
  for (auto *resource : resources_)
    resource->Reset();
  context_.logs_.clear();
  context_.failed_ = false;
  context_.failure_.clear();
  context_.exception_ = nullptr;
  publishException_ = nullptr;
  cycle_ = 0;
}

} // namespace pycircuit::transactional_gfsim
