#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "pyc_bits.hpp"

namespace pyc::cpp {

template <unsigned W>
inline Wire<W> &deref_connector(const std::shared_ptr<Wire<W>> &ptr, const char *ctx) {
  if (!ptr)
    throw std::runtime_error(std::string("null pyc connector pointer") + (ctx ? std::string(" in ") + ctx : ""));
  return *ptr;
}

template <unsigned W>
struct Connector {
  std::shared_ptr<Wire<W>> ptr;

  Connector() : ptr(std::make_shared<Wire<W>>()) {}
  explicit Connector(std::shared_ptr<Wire<W>> p) : ptr(std::move(p)) {
    if (!ptr)
      ptr = std::make_shared<Wire<W>>();
  }

  Wire<W> &ref() { return deref_connector<W>(ptr, "Connector::ref"); }
  const Wire<W> &ref() const { return deref_connector<W>(ptr, "Connector::ref const"); }
};

template <unsigned W>
inline Connector<W> make_connector() {
  return Connector<W>(std::make_shared<Wire<W>>());
}

template <unsigned W>
inline Connector<W> share_connector(const std::shared_ptr<Wire<W>> &ptr) {
  return Connector<W>(ptr);
}

} // namespace pyc::cpp
