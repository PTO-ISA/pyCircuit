#ifndef ACIR_TEST_EXTENSION_PROVIDER_H
#define ACIR_TEST_EXTENSION_PROVIDER_H

#include "gfsim/components.h"

#include <cstdint>
#include <string>
#include <utility>

namespace ac_test {

class Counter final : public gfsim::SimObject {
public:
  static constexpr std::string_view contractName = "ac.test.Counter";
  static constexpr gfsim::ObjectKind componentKind = gfsim::ObjectKind::Compute;

  Counter(std::string name, gfsim::ObjectId id, gfsim::SimObject *parent)
      : SimObject(componentKind, std::move(name), id, parent) {}

  void doWork(gfsim::Epoch) override { proposed_ = committed_ + 1; }
  void doXfer(gfsim::Epoch) override { committed_ = proposed_; }
  bool hasPendingCommit() const override { return proposed_ != committed_; }
  bool validate() const { return id() != gfsim::kInvalidObjectId; }
  void reset() override { committed_ = proposed_ = 0; }

private:
  uint64_t committed_ = 0;
  uint64_t proposed_ = 0;
};

static_assert(gfsim::Component<Counter>);

} // namespace ac_test

#endif // ACIR_TEST_EXTENSION_PROVIDER_H
