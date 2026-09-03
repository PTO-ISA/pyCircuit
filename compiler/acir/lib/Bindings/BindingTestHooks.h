#ifndef ACIR_LIB_BINDINGS_BINDINGTESTHOOKS_H
#define ACIR_LIB_BINDINGS_BINDINGTESTHOOKS_H

namespace acir::bindings::detail {

inline thread_local bool failBindingPublish = false;
inline thread_local bool failCanonicalEmission = false;

class ScopedCanonicalEmissionFailure {
public:
  ScopedCanonicalEmissionFailure() : previous(failCanonicalEmission) {
    failCanonicalEmission = true;
  }

  ~ScopedCanonicalEmissionFailure() { failCanonicalEmission = previous; }

  ScopedCanonicalEmissionFailure(const ScopedCanonicalEmissionFailure &) =
      delete;
  ScopedCanonicalEmissionFailure &
  operator=(const ScopedCanonicalEmissionFailure &) = delete;

private:
  bool previous;
};

class ScopedBindingPublishFailure {
public:
  ScopedBindingPublishFailure() : previous(failBindingPublish) {
    failBindingPublish = true;
  }

  ~ScopedBindingPublishFailure() { failBindingPublish = previous; }

  ScopedBindingPublishFailure(const ScopedBindingPublishFailure &) = delete;
  ScopedBindingPublishFailure &
  operator=(const ScopedBindingPublishFailure &) = delete;

private:
  bool previous;
};

inline bool shouldFailBindingPublish() { return failBindingPublish; }
inline bool shouldFailCanonicalEmission() { return failCanonicalEmission; }

} // namespace acir::bindings::detail

#endif // ACIR_LIB_BINDINGS_BINDINGTESTHOOKS_H
