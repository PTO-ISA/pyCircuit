// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/four_state.h"

#include <cstdint>
#include <exception>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace pycircuit::transactional_gfsim {

class Context;
class System;

// These interfaces are a generated-code lifecycle, not a design interpreter.
// System invokes every registered object once; hooks must not recurse into
// other registered objects' lifecycle hooks. Registered objects outlive System.
class Resource {
public:
  Resource() = default;
  Resource(const Resource &) = delete;
  Resource &operator=(const Resource &) = delete;
  virtual ~Resource() = default;

  virtual void Begin() noexcept = 0;
  virtual void Check(Context &) = 0;
  virtual void Xfer() noexcept = 0;
  virtual void Discard() noexcept = 0;
  virtual void Reset() noexcept = 0;

protected:
  void RequireWork(const Context &) const;

private:
  friend class System;
  System *owner_ = nullptr;
};

class Module {
public:
  Module() = default;
  Module(const Module &) = delete;
  Module &operator=(const Module &) = delete;
  virtual ~Module() = default;

  virtual void CombWork(Context &) {}
  virtual void Work(Context &) = 0;
  virtual void Check(Context &) {}

private:
  friend class System;
  System *owner_ = nullptr;
};

// The only Context is owned by its System and is never retained by generated
// code. Payload reads and readiness queries do not modify this context.
class Context final {
public:
  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;

  void Fail(std::string message);
  void Assert(bool condition, std::string_view message);
  void Assert(const ::gfsim::FourState<1> &condition, std::string_view message);
  void Log(std::string message);
  bool Failed() const noexcept { return failed_; }

private:
  friend class System;
  friend class Resource;
  enum class Phase { Idle, Begin, Comb, Work, Check, Xfer, Publish };
  explicit Context(System &owner) : owner_(owner) {}
  void RequireEvaluation() const;

  System &owner_;
  Phase phase_ = Phase::Idle;
  bool failed_ = false;
  std::string failure_;
  std::exception_ptr exception_;
  std::vector<std::string> logs_;
};

// Strict controls accept exactly one known bit. This helper has no effects;
// System catches a reached X/Z control before any resource can commit.
bool CheckedControl(const ::gfsim::FourState<1> &value);
inline bool CheckedControl(const ::gfsim::Bits<1> &value) noexcept {
  return value.toBool();
}
inline bool CheckedControl(bool value) noexcept { return value; }

class System final {
public:
  using LogSink = std::function<void(std::string_view)>;

  System();
  ~System();
  System(const System &) = delete;
  System &operator=(const System &) = delete;

  void Register(Resource &resource);
  void Register(Module &module);
  void SetLogSink(LogSink sink);

  // One successful call is one complete cycle. All precommit exceptions or
  // failed checks discard every proposal and unpublished log, returning false.
  // A sink runs only after commit: its exception stops publication, is exposed
  // separately, and never changes a successful Step into a rollback result.
  bool Step();
  // Evaluate one observation sample. Only a transfer sample updates storage;
  // sampled reset takes priority over updates. Successful samples publish their
  // old-Q observations, including samples without a transfer.
  bool Sample(bool transfer, bool reset = false);
  void Reset();
  std::uint64_t Cycle() const noexcept { return cycle_; }
  bool Failed() const noexcept { return context_.failed_; }
  const std::string &Failure() const noexcept { return context_.failure_; }
  std::exception_ptr Exception() const noexcept { return context_.exception_; }
  std::exception_ptr PublishException() const noexcept {
    return publishException_;
  }

private:
  void RequireIdle() const;
  void Discard() noexcept;

  Context context_;
  std::vector<Resource *> resources_;
  std::vector<Module *> modules_;
  LogSink sink_;
  std::uint64_t cycle_ = 0;
  std::exception_ptr publishException_;
};

// A logical payload must support an allocation-free, nonthrowing commit and
// reset. Fixed Bits/FourState values and aggregates of them satisfy this.
template <typename T> class State final : public Resource {
public:
  static_assert(std::is_nothrow_copy_assignable_v<T>,
                "State payload assignment must not throw");
  static_assert(std::is_nothrow_destructible_v<T>);

  explicit State(const T &initial) : initial_(initial), q_(initial), next_(initial) {}
  State() requires requires { T::unknown(); } : State(T::unknown()) {}

  const T &Q() const noexcept { return q_; }
  bool Propose(Context &context, const T &value) {
    RequireWork(context);
    if (pending_) {
      conflict_ = true;
      context.Fail("multiple proposals to one State");
      return false;
    }
    next_ = value;
    pending_ = true;
    return true;
  }

  void Begin() noexcept override { Discard(); }
  void Check(Context &context) override {
    if (conflict_)
      context.Fail("multiple proposals to one State");
  }
  void Xfer() noexcept override {
    if (pending_)
      q_ = next_;
    Discard();
  }
  void Discard() noexcept override {
    pending_ = false;
    conflict_ = false;
  }
  void Reset() noexcept override {
    q_ = initial_;
    Discard();
  }

private:
  const T initial_;
  T q_;
  T next_;
  bool pending_ = false;
  bool conflict_ = false;
};

} // namespace pycircuit::transactional_gfsim
