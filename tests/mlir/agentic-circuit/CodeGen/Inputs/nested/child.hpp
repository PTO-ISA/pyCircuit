#ifndef ACIR_TEST_NESTED_CHILD_HPP
#define ACIR_TEST_NESTED_CHILD_HPP

#include "gfsim/components.h"

#include <string>
#include <utility>

class Child final : public gfsim::SimObject {
public:
  static constexpr std::string_view contractName = "ac.test.Child";
  static constexpr gfsim::ObjectKind componentKind = gfsim::ObjectKind::Compute;

  Child(std::string name, gfsim::ObjectId id, gfsim::SimObject *parent)
      : SimObject(componentKind, std::move(name), id, parent) {}
};

template <typename T>
concept StatefulComponent = gfsim::Component<T>;

#endif // ACIR_TEST_NESTED_CHILD_HPP
