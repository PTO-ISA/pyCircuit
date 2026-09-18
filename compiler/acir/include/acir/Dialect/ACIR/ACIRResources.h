#ifndef ACIR_DIALECT_ACIR_ACIRRESOURCES_H
#define ACIR_DIALECT_ACIR_ACIRRESOURCES_H

#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

#include <cassert>
#include <cstdint>

namespace mlir {
class Operation;
}

namespace acir::ac {

inline constexpr uint64_t kMaxTickScale = uint64_t{1} << 32;
/// ACIR capability bound for exact general mixed-interleave relations.
inline constexpr uint64_t kMaxGeneralSelectorIntersectionQueries = 256;

/// Wide half-open address endpoint type. A 64-bit address space needs the
/// representable endpoint 2^64 even though its largest address is 2^64-1.
///
/// MSVC has no `unsigned __int128`, so the handful of operations address
/// verification needs are implemented here over two 64-bit words. Division and
/// modulo are long division by a word-sized divisor, which is all the address
/// verifiers use (alignment, denominator, interleave cycle).
class WideAddress {
public:
  constexpr WideAddress() = default;
  constexpr WideAddress(uint64_t low) : low_(low) {} // NOLINT: implicit by design
  constexpr WideAddress(uint64_t high, uint64_t low) : low_(low), high_(high) {}

  constexpr uint64_t low() const { return low_; }
  constexpr uint64_t high() const { return high_; }

  friend constexpr bool operator==(WideAddress a, WideAddress b) {
    return a.low_ == b.low_ && a.high_ == b.high_;
  }
  friend constexpr bool operator!=(WideAddress a, WideAddress b) {
    return !(a == b);
  }
  friend constexpr bool operator<(WideAddress a, WideAddress b) {
    return a.high_ != b.high_ ? a.high_ < b.high_ : a.low_ < b.low_;
  }
  friend constexpr bool operator>(WideAddress a, WideAddress b) { return b < a; }
  friend constexpr bool operator<=(WideAddress a, WideAddress b) {
    return !(b < a);
  }
  friend constexpr bool operator>=(WideAddress a, WideAddress b) {
    return !(a < b);
  }

  friend constexpr WideAddress operator+(WideAddress a, WideAddress b) {
    const uint64_t low = a.low_ + b.low_;
    const uint64_t carry = low < a.low_ ? 1u : 0u;
    return WideAddress(a.high_ + b.high_ + carry, low);
  }
  friend constexpr WideAddress operator-(WideAddress a, WideAddress b) {
    const uint64_t low = a.low_ - b.low_;
    const uint64_t borrow = a.low_ < b.low_ ? 1u : 0u;
    return WideAddress(a.high_ - b.high_ - borrow, low);
  }
  friend constexpr WideAddress operator<<(WideAddress a, unsigned shift) {
    if (shift >= 128)
      return WideAddress(0);
    if (shift >= 64)
      return WideAddress(a.low_ << (shift - 64), 0);
    if (shift == 0)
      return a;
    return WideAddress((a.high_ << shift) | (a.low_ >> (64 - shift)),
                       a.low_ << shift);
  }
  friend constexpr WideAddress operator*(WideAddress a, uint64_t b) {
    uint64_t high = 0;
    uint64_t low = 0;
    multiplyWords(a.low_, b, high, low);
    return WideAddress(a.high_ * b + high, low);
  }
  friend WideAddress operator/(WideAddress a, WideAddress b) {
    uint64_t remainder = 0;
    return divideByWord(a, divisorWord(b), remainder);
  }
  friend WideAddress operator%(WideAddress a, WideAddress b) {
    uint64_t remainder = 0;
    (void)divideByWord(a, divisorWord(b), remainder);
    return WideAddress(remainder);
  }

private:
  /// Full 64x64 -> 128 product from 32-bit limbs.
  static constexpr void multiplyWords(uint64_t a, uint64_t b, uint64_t &high,
                                      uint64_t &low) {
    const uint64_t aLow = a & 0xFFFFFFFFu;
    const uint64_t aHigh = a >> 32;
    const uint64_t bLow = b & 0xFFFFFFFFu;
    const uint64_t bHigh = b >> 32;
    const uint64_t lowLow = aLow * bLow;
    const uint64_t lowHigh = aLow * bHigh;
    const uint64_t highLow = aHigh * bLow;
    const uint64_t highHigh = aHigh * bHigh;
    const uint64_t mid = (lowLow >> 32) + (lowHigh & 0xFFFFFFFFu) +
                         (highLow & 0xFFFFFFFFu);
    low = (lowLow & 0xFFFFFFFFu) | (mid << 32);
    high = highHigh + (lowHigh >> 32) + (highLow >> 32) + (mid >> 32);
  }

  static constexpr uint64_t divisorWord(WideAddress divisor) {
    assert(divisor.high_ == 0 && divisor.low_ != 0 &&
           "address verification divides only by word-sized values");
    return divisor.low_;
  }

  /// Long division by a word-sized divisor, returning the quotient and writing
  /// the remainder.
  static WideAddress divideByWord(WideAddress value, uint64_t divisor,
                                  uint64_t &remainder) {
    const uint64_t quotientHigh = value.high_ / divisor;
    uint64_t rem = value.high_ % divisor;
    uint64_t quotientLow = 0;
    for (int bit = 63; bit >= 0; --bit) {
      const uint64_t next = (value.low_ >> bit) & 1u;
      if (rem >> 63) {
        // Doubling overflows 64 bits, and `rem < divisor` guarantees that one
        // subtraction is exact; the wrap-around subtraction yields the true
        // remainder because it fits in 64 bits.
        rem = ((rem << 1) | next) - divisor;
        quotientLow |= uint64_t{1} << bit;
      } else {
        rem = (rem << 1) | next;
        if (rem >= divisor) {
          rem -= divisor;
          quotientLow |= uint64_t{1} << bit;
        }
      }
    }
    remainder = rem;
    return WideAddress(quotientHigh, quotientLow);
  }

  uint64_t low_ = 0;
  uint64_t high_ = 0;
};

struct AddressInterval {
  WideAddress begin;
  WideAddress end;
};

struct AddressMapOrderKey {
  uint64_t base;
  uint64_t size;
  bool hasPriority;
  uint64_t priority;
};

bool checkedAdd(uint64_t left, uint64_t right, uint64_t &result);
bool checkedMultiply(uint64_t left, uint64_t right, uint64_t &result);
bool intervalsOverlap(AddressInterval left, AddressInterval right);
int compareAddressMapOrder(AddressMapOrderKey left, AddressMapOrderKey right);

/// Computes the normative global tick phase + cycle * period. The result is
/// bounded by the signed i64 tick domain used by ACIR attributes.
bool checkedDomainTick(uint64_t phase, uint64_t period, uint64_t cycle,
                       uint64_t &tick);

/// Converts an exact rational duration to an integral number of global ticks.
/// The duration is numerator/denominator and the global quantum is
/// quantumNumerator/quantumDenominator. The reduced conversion scale (the
/// dimensionless multiplier between duration and quantum) is bounded by
/// kMaxTickScale. Returns false for invalid, inexact, or overflowing
/// conversions; no floating-point rounding is permitted.
bool normalizeRationalToTicks(uint64_t numerator, uint64_t denominator,
                              uint64_t quantumNumerator,
                              uint64_t quantumDenominator, uint64_t &ticks);

struct QueueStateResource
    : public mlir::SideEffects::Resource::Base<QueueStateResource> {
  llvm::StringRef getName() final { return "ac.queue.state"; }
};

struct EventQueueStateResource
    : public mlir::SideEffects::Resource::Base<EventQueueStateResource> {
  llvm::StringRef getName() final { return "ac.event_queue.state"; }
};

struct ReservationStateResource
    : public mlir::SideEffects::Resource::Base<ReservationStateResource> {
  llvm::StringRef getName() final { return "ac.resource.reservation"; }
};

struct ModuleStateResource
    : public mlir::SideEffects::Resource::Base<ModuleStateResource> {
  llvm::StringRef getName() final { return "ac.module.state"; }
};

struct StorageStateResource
    : public mlir::SideEffects::Resource::Base<StorageStateResource> {
  llvm::StringRef getName() final { return "ac.storage.state"; }
};

struct ProtocolStateResource
    : public mlir::SideEffects::Resource::Base<ProtocolStateResource> {
  llvm::StringRef getName() final { return "ac.protocol.state"; }
};

struct ExternalIOResource
    : public mlir::SideEffects::Resource::Base<ExternalIOResource> {
  llvm::StringRef getName() final { return "ac.external_io"; }
};

struct StatisticsResource
    : public mlir::SideEffects::Resource::Base<StatisticsResource> {
  llvm::StringRef getName() final { return "ac.statistics"; }
};

/// Resolves all Task7 cross-operation references from the ac.module producer
/// index and verifies address/time parent graphs exactly once per module.
mlir::LogicalResult verifyModuleResourceReferences(
    mlir::Operation *module,
    const llvm::StringMap<mlir::Operation *> &producerIndex);

/// Rewrites address-map set fields and entries into the unique ACIR
/// total order. This is the deterministic normalization phase used by the
/// mandatory public pipeline; operation verifiers remain mutation-free.
void normalizeAddressMaps(mlir::Operation *topLevel);

} // namespace acir::ac

#endif
