#ifndef ACIR_TEST_FULL_GFSIM_FIFO_HPP
#define ACIR_TEST_FULL_GFSIM_FIFO_HPP

#include "gfsim/components.h"
#include "gfsim/process.h"

#include <string>
#include <utility>

namespace gfsim {

class TestEndpoint {
public:
  void bind(TestEndpoint &other) { peer_ = &other; }

private:
  TestEndpoint *peer_ = nullptr;
};

class Fifo final : public SimObject {
public:
  static constexpr std::string_view contractName = "ac.test.Fifo";
  static constexpr ObjectKind componentKind = ObjectKind::Queue;

  Fifo(std::string name, ObjectId id, SimObject *parent)
      : SimObject(componentKind, std::move(name), id, parent) {}

  bool invoke(bool value) { return value; }
  ProcessWake invoke() const { return {ProcessWakeKind::Condition, id()}; }

  TestEndpoint &output() { return output_; }
  const TestEndpoint &output() const { return output_; }
  TestEndpoint &input() { return input_; }
  const TestEndpoint &input() const { return input_; }
  TestEndpoint &initiator() { return initiator_; }
  const TestEndpoint &initiator() const { return initiator_; }
  TestEndpoint &target() { return target_; }
  const TestEndpoint &target() const { return target_; }

private:
  TestEndpoint output_;
  TestEndpoint input_;
  TestEndpoint initiator_;
  TestEndpoint target_;
};

template <typename T>
concept StatefulModel = Component<T>;

} // namespace gfsim

#endif // ACIR_TEST_FULL_GFSIM_FIFO_HPP
