// SPDX-License-Identifier: BSD-3-Clause
// Reused hardware value representation from the existing primitive library.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#define GFSIM_SIMD_NEON 1
#endif

namespace gfsim {

// ---------------------------------------------------------------------------
// NEON helpers (compile to nothing on non-ARM)
// ---------------------------------------------------------------------------
namespace simd {

#if GFSIM_SIMD_NEON
inline void bitwise_and(std::uint64_t *dst, const std::uint64_t *a,
                        const std::uint64_t *b, unsigned nWords) {
  unsigned i = 0;
  for (; i + 2 <= nWords; i += 2) {
    uint64x2_t va = vld1q_u64(a + i);
    uint64x2_t vb = vld1q_u64(b + i);
    vst1q_u64(dst + i, vandq_u64(va, vb));
  }
  for (; i < nWords; i++)
    dst[i] = a[i] & b[i];
}

inline void bitwise_or(std::uint64_t *dst, const std::uint64_t *a,
                       const std::uint64_t *b, unsigned nWords) {
  unsigned i = 0;
  for (; i + 2 <= nWords; i += 2) {
    uint64x2_t va = vld1q_u64(a + i);
    uint64x2_t vb = vld1q_u64(b + i);
    vst1q_u64(dst + i, vorrq_u64(va, vb));
  }
  for (; i < nWords; i++)
    dst[i] = a[i] | b[i];
}

inline void bitwise_xor(std::uint64_t *dst, const std::uint64_t *a,
                        const std::uint64_t *b, unsigned nWords) {
  unsigned i = 0;
  for (; i + 2 <= nWords; i += 2) {
    uint64x2_t va = vld1q_u64(a + i);
    uint64x2_t vb = vld1q_u64(b + i);
    vst1q_u64(dst + i, veorq_u64(va, vb));
  }
  for (; i < nWords; i++)
    dst[i] = a[i] ^ b[i];
}

inline void bitwise_not(std::uint64_t *dst, const std::uint64_t *a,
                        unsigned nWords) {
  unsigned i = 0;
  for (; i + 2 <= nWords; i += 2) {
    uint64x2_t va = vld1q_u64(a + i);
    vst1q_u64(dst + i,
              vreinterpretq_u64_u8(vmvnq_u8(vreinterpretq_u8_u64(va))));
  }
  for (; i < nWords; i++)
    dst[i] = ~a[i];
}

inline bool bitwise_eq(const std::uint64_t *a, const std::uint64_t *b,
                       unsigned nWords) {
  unsigned i = 0;
  for (; i + 2 <= nWords; i += 2) {
    uint64x2_t va = vld1q_u64(a + i);
    uint64x2_t vb = vld1q_u64(b + i);
    uint64x2_t cmp = vceqq_u64(va, vb);
    if (vgetq_lane_u64(cmp, 0) != ~std::uint64_t{0} ||
        vgetq_lane_u64(cmp, 1) != ~std::uint64_t{0})
      return false;
  }
  for (; i < nWords; i++)
    if (a[i] != b[i])
      return false;
  return true;
}

// Bitwise select: dst[i] = mask[i] ? a[i] : b[i]  (per-bit)
inline void bitwise_sel(std::uint64_t *dst, const std::uint64_t *mask,
                        const std::uint64_t *a, const std::uint64_t *b,
                        unsigned nWords) {
  unsigned i = 0;
  for (; i + 2 <= nWords; i += 2) {
    uint64x2_t vm = vld1q_u64(mask + i);
    uint64x2_t va = vld1q_u64(a + i);
    uint64x2_t vb = vld1q_u64(b + i);
    vst1q_u64(dst + i, vbslq_u64(vreinterpretq_u64_u8(vreinterpretq_u8_u64(vm)),
                                 va, vb));
  }
  for (; i < nWords; i++)
    dst[i] = (a[i] & mask[i]) | (b[i] & ~mask[i]);
}
#endif

} // namespace simd

template <unsigned Width> class Bits {
public:
  static_assert(Width > 0, "gfsim::Bits requires Width > 0");

  using word_type = std::uint64_t;
  static constexpr unsigned kWidth = Width;
  static constexpr unsigned kWordBits = 64;
  static constexpr unsigned kWords = 1 + (Width - 1) / kWordBits;

  constexpr Bits() = default;

  constexpr explicit Bits(word_type low) {
    words_.fill(0);
    words_[0] = low;
    maskTop();
  }

  explicit Bits(std::initializer_list<word_type> words) {
    words_.fill(0);
    unsigned i = 0;
    for (word_type w : words) {
      if (i >= kWords)
        break;
      words_[i++] = w;
    }
    maskTop();
  }

  constexpr word_type value() const { return words_[0]; }
  constexpr bool toBool() const { return (words_[0] & 1u) != 0; }

  constexpr word_type word(unsigned i) const {
    return (i < kWords) ? words_[i] : word_type{0};
  }
  constexpr void setWord(unsigned i, word_type w) {
    if (i >= kWords)
      return;
    if (i + 1u == kWords) {
      words_[i] = w & topMask();
      return;
    }
    words_[i] = w;
  }

  constexpr bool bit(unsigned i) const {
    if (i >= Width)
      return false;
    unsigned wi = i / kWordBits;
    unsigned bi = i % kWordBits;
    return ((word(wi) >> bi) & 1u) != 0;
  }

  word_type *data() { return words_.data(); }
  const word_type *data() const { return words_.data(); }

  static constexpr word_type mask() {
    if constexpr (Width >= 64)
      return ~word_type{0};
    return (word_type{1} << Width) - 1;
  }

  static constexpr Bits ones() {
    Bits out;
    for (unsigned i = 0; i < kWords; i++)
      out.words_[i] = ~word_type{0};
    out.maskTop();
    return out;
  }

  // MSVC has no unsigned __int128, so every multi-word operation below is
  // expressed with 64-bit wraparound arithmetic. The carry and borrow results
  // are identical to the previous 128-bit formulation.
  friend constexpr Bits operator+(Bits a, Bits b) {
    Bits out;
    word_type carry = 0;
    for (unsigned i = 0; i < kWords; i++) {
      word_type sum = static_cast<word_type>(a.words_[i] + b.words_[i]);
      word_type carryLow = (sum < a.words_[i]) ? word_type{1} : word_type{0};
      word_type withCarry = static_cast<word_type>(sum + carry);
      word_type carryHigh = (withCarry < sum) ? word_type{1} : word_type{0};
      out.words_[i] = withCarry;
      carry = static_cast<word_type>(carryLow | carryHigh);
    }
    out.maskTop();
    return out;
  }

  friend constexpr Bits operator-(Bits a, Bits b) {
    Bits out;
    word_type borrow = 0;
    for (unsigned i = 0; i < kWords; i++) {
      word_type diff = static_cast<word_type>(a.words_[i] - b.words_[i]);
      word_type borrowLow =
          (a.words_[i] < b.words_[i]) ? word_type{1} : word_type{0};
      word_type withBorrow = static_cast<word_type>(diff - borrow);
      word_type borrowHigh = (diff < borrow) ? word_type{1} : word_type{0};
      out.words_[i] = withBorrow;
      borrow = static_cast<word_type>(borrowLow | borrowHigh);
    }
    out.maskTop();
    return out;
  }

  // Portable 64x64 -> 128 product built from 32-bit limbs.
  static constexpr void mulWord(word_type a, word_type b, word_type &hi,
                                word_type &lo) {
    const word_type aLow = static_cast<word_type>(a & 0xFFFFFFFFu);
    const word_type aHigh = static_cast<word_type>(a >> 32);
    const word_type bLow = static_cast<word_type>(b & 0xFFFFFFFFu);
    const word_type bHigh = static_cast<word_type>(b >> 32);
    const word_type lowLow = static_cast<word_type>(aLow * bLow);
    const word_type lowHigh = static_cast<word_type>(aLow * bHigh);
    const word_type highLow = static_cast<word_type>(aHigh * bLow);
    const word_type highHigh = static_cast<word_type>(aHigh * bHigh);
    const word_type mid = static_cast<word_type>(
        (lowLow >> 32) + (lowHigh & 0xFFFFFFFFu) + (highLow & 0xFFFFFFFFu));
    lo = static_cast<word_type>((lowLow & 0xFFFFFFFFu) | (mid << 32));
    hi = static_cast<word_type>(highHigh + (lowHigh >> 32) + (highLow >> 32) +
                                (mid >> 32));
  }

  friend inline Bits operator*(Bits a, Bits b) {
    Bits out;
    out.words_.fill(0);
    for (unsigned i = 0; i < kWords; i++) {
      word_type carry = 0;
      for (unsigned j = 0; j + i < kWords; j++) {
        unsigned idx = i + j;
        word_type hi = 0;
        word_type lo = 0;
        mulWord(a.words_[i], b.words_[j], hi, lo);
        word_type cur = out.words_[idx];
        word_type sum = static_cast<word_type>(cur + lo);
        word_type carryLow = (sum < cur) ? word_type{1} : word_type{0};
        word_type withCarry = static_cast<word_type>(sum + carry);
        word_type carryHigh = (withCarry < sum) ? word_type{1} : word_type{0};
        out.words_[idx] = withCarry;
        carry = static_cast<word_type>(hi + carryLow + carryHigh);
      }
    }
    out.maskTop();
    return out;
  }

  friend Bits operator&(Bits a, Bits b) {
    Bits out;
#if GFSIM_SIMD_NEON
    if constexpr (kWords >= 2) {
      simd::bitwise_and(out.words_.data(), a.words_.data(), b.words_.data(),
                        kWords);
      out.maskTop();
      return out;
    }
#endif
    for (unsigned i = 0; i < kWords; i++)
      out.words_[i] = a.words_[i] & b.words_[i];
    out.maskTop();
    return out;
  }

  friend Bits operator|(Bits a, Bits b) {
    Bits out;
#if GFSIM_SIMD_NEON
    if constexpr (kWords >= 2) {
      simd::bitwise_or(out.words_.data(), a.words_.data(), b.words_.data(),
                       kWords);
      out.maskTop();
      return out;
    }
#endif
    for (unsigned i = 0; i < kWords; i++)
      out.words_[i] = a.words_[i] | b.words_[i];
    out.maskTop();
    return out;
  }

  friend Bits operator^(Bits a, Bits b) {
    Bits out;
#if GFSIM_SIMD_NEON
    if constexpr (kWords >= 2) {
      simd::bitwise_xor(out.words_.data(), a.words_.data(), b.words_.data(),
                        kWords);
      out.maskTop();
      return out;
    }
#endif
    for (unsigned i = 0; i < kWords; i++)
      out.words_[i] = a.words_[i] ^ b.words_[i];
    out.maskTop();
    return out;
  }

  friend Bits operator~(Bits a) {
    Bits out;
#if GFSIM_SIMD_NEON
    if constexpr (kWords >= 2) {
      simd::bitwise_not(out.words_.data(), a.words_.data(), kWords);
      out.maskTop();
      return out;
    }
#endif
    for (unsigned i = 0; i < kWords; i++)
      out.words_[i] = ~a.words_[i];
    out.maskTop();
    return out;
  }

  friend bool operator==(Bits a, Bits b) {
#if GFSIM_SIMD_NEON
    if constexpr (kWords >= 2)
      return simd::bitwise_eq(a.words_.data(), b.words_.data(), kWords);
#endif
    for (unsigned i = 0; i < kWords; i++) {
      if (a.words_[i] != b.words_[i])
        return false;
    }
    return true;
  }

  friend bool operator!=(Bits a, Bits b) { return !(a == b); }

  friend constexpr bool operator<(Bits a, Bits b) {
    for (unsigned i = 0; i < kWords; i++) {
      unsigned idx = (kWords - 1u) - i;
      if (a.words_[idx] < b.words_[idx])
        return true;
      if (a.words_[idx] > b.words_[idx])
        return false;
    }
    return false;
  }

  friend constexpr bool operator>(Bits a, Bits b) { return b < a; }
  friend constexpr bool operator<=(Bits a, Bits b) { return !(b < a); }
  friend constexpr bool operator>=(Bits a, Bits b) { return !(a < b); }

private:
  static constexpr word_type topMask() {
    if constexpr ((Width % kWordBits) == 0)
      return ~word_type{0};
    return (word_type{1} << (Width % kWordBits)) - 1;
  }

  constexpr void maskTop() { words_[kWords - 1] &= topMask(); }

  std::array<word_type, kWords> words_{};
};

// SIMD-accelerated MUX: returns sel ? a : b (branch-free for wide wires)
template <unsigned Width>
inline Bits<Width> mux(Bits<1> sel, Bits<Width> a, Bits<Width> b) {
#if GFSIM_SIMD_NEON
  if constexpr (Bits<Width>::kWords >= 2) {
    Bits<Width> out;
    // Broadcast sel to all bits: 0 or all-ones mask
    std::uint64_t smask = sel.toBool() ? ~std::uint64_t{0} : std::uint64_t{0};
    uint64x2_t vm = vdupq_n_u64(smask);
    const auto *pa = a.data();
    const auto *pb = b.data();
    auto *po = out.data();
    unsigned i = 0;
    for (; i + 2 <= Bits<Width>::kWords; i += 2) {
      uint64x2_t va = vld1q_u64(pa + i);
      uint64x2_t vb = vld1q_u64(pb + i);
      vst1q_u64(po + i, vbslq_u64(vm, va, vb));
    }
    for (; i < Bits<Width>::kWords; i++)
      po[i] = sel.toBool() ? pa[i] : pb[i];
    return out;
  }
#endif
  return sel.toBool() ? a : b;
}

template <unsigned Width, std::size_t N>
inline void appendPackedBitsWords(std::array<std::uint64_t, N> &dst,
                                  std::size_t &offset, Bits<Width> v) {
  for (unsigned i = 0; i < Bits<Width>::kWords; ++i)
    dst[offset++] = v.word(i);
}

template <unsigned Width> constexpr std::int64_t asSigned(Bits<Width> v) {
  static_assert(Width > 0 && Width <= 64, "asSigned supports widths 1..64");
  if constexpr (Width == 64) {
    return static_cast<std::int64_t>(v.value());
  } else {
    std::uint64_t x = v.value();
    std::uint64_t signBit = std::uint64_t{1} << (Width - 1);
    if (x & signBit)
      x |= (~std::uint64_t{0}) << Width;
    return static_cast<std::int64_t>(x);
  }
}

template <unsigned Width>
constexpr Bits<Width> shl(Bits<Width> v, unsigned amount) {
  if (amount == 0)
    return v;
  if (amount >= Width)
    return Bits<Width>(0);

  Bits<Width> out;
  unsigned wordShift = amount / Bits<Width>::kWordBits;
  unsigned bitShift = amount % Bits<Width>::kWordBits;
  for (unsigned i = 0; i < Bits<Width>::kWords; i++) {
    unsigned dst = (Bits<Width>::kWords - 1u) - i;
    std::uint64_t w = 0;
    if (dst >= wordShift) {
      unsigned src = dst - wordShift;
      w = v.word(src) << bitShift;
      if (bitShift != 0 && src > 0)
        w |= (v.word(src - 1) >> (64u - bitShift));
    }
    out.setWord(dst, w);
  }
  return out;
}

template <unsigned Width>
constexpr Bits<Width> lshr(Bits<Width> v, unsigned amount) {
  if (amount == 0)
    return v;
  if (amount >= Width)
    return Bits<Width>(0);

  Bits<Width> out;
  unsigned wordShift = amount / Bits<Width>::kWordBits;
  unsigned bitShift = amount % Bits<Width>::kWordBits;
  for (unsigned dst = 0; dst < Bits<Width>::kWords; dst++) {
    std::uint64_t w = 0;
    unsigned src = dst + wordShift;
    if (src < Bits<Width>::kWords) {
      w = v.word(src) >> bitShift;
      if (bitShift != 0 && (src + 1) < Bits<Width>::kWords)
        w |= (v.word(src + 1) << (64u - bitShift));
    }
    out.setWord(dst, w);
  }
  return out;
}

template <unsigned Width>
constexpr Bits<Width> ashr(Bits<Width> v, unsigned amount) {
  if (amount == 0)
    return v;
  if (amount >= Width)
    return v.bit(Width - 1) ? Bits<Width>::ones() : Bits<Width>(0);

  Bits<Width> out = lshr<Width>(v, amount);
  if (!v.bit(Width - 1))
    return out;
  // Negative: fill high bits with 1.
  unsigned fillFrom = Width - amount;
  Bits<Width> fill = shl<Width>(Bits<Width>::ones(), fillFrom);
  return out | fill;
}

namespace detail {

template <unsigned Width> constexpr bool isZero(Bits<Width> v) {
  for (unsigned i = 0; i < Bits<Width>::kWords; i++) {
    if (v.word(i) != 0u)
      return false;
  }
  return true;
}

template <unsigned Width> constexpr bool isAllOnes(Bits<Width> v) {
  for (unsigned i = 0; i < Width; i++) {
    if (!v.bit(i))
      return false;
  }
  return true;
}

template <unsigned Width> constexpr bool isMinSigned(Bits<Width> v) {
  if (!v.bit(Width - 1))
    return false;
  for (unsigned i = 0; i + 1 < Width; i++) {
    if (v.bit(i))
      return false;
  }
  return true;
}

template <unsigned Width> constexpr Bits<Width> negate(Bits<Width> v) {
  return (~v) + Bits<Width>(1);
}

template <unsigned Width>
constexpr Bits<Width> setBit(Bits<Width> v, unsigned idx) {
  if (idx >= Width)
    return v;
  return v | shl<Width>(Bits<Width>(1), idx);
}

template <unsigned Width>
constexpr void udivrem(Bits<Width> numer, Bits<Width> denom, Bits<Width> &quot,
                       Bits<Width> &rem) {
  quot = Bits<Width>(0);
  rem = Bits<Width>(0);
  if (isZero(denom))
    return;

  for (unsigned i = Width; i > 0; --i) {
    unsigned bit = i - 1;
    rem = shl<Width>(rem, 1);
    if (numer.bit(bit))
      rem = rem | Bits<Width>(1);
    if (rem >= denom) {
      rem = rem - denom;
      quot = setBit(quot, bit);
    }
  }
}

} // namespace detail

template <unsigned Width>
constexpr Bits<Width> udiv(Bits<Width> a, Bits<Width> b) {
  if (detail::isZero(b))
    return Bits<Width>(0);
  if constexpr (Width <= 64)
    return Bits<Width>(a.value() / b.value());
  Bits<Width> q;
  Bits<Width> r;
  detail::udivrem<Width>(a, b, q, r);
  return q;
}

template <unsigned Width>
constexpr Bits<Width> urem(Bits<Width> a, Bits<Width> b) {
  if (detail::isZero(b))
    return Bits<Width>(0);
  if constexpr (Width <= 64)
    return Bits<Width>(a.value() % b.value());
  Bits<Width> q;
  Bits<Width> r;
  detail::udivrem<Width>(a, b, q, r);
  return r;
}

template <unsigned Width>
constexpr Bits<Width> sdiv(Bits<Width> a, Bits<Width> b) {
  if (detail::isZero(b))
    return Bits<Width>(0);
  // Two's-complement overflow: MIN_INT / -1 -> MIN_INT.
  if (detail::isMinSigned(a) && detail::isAllOnes(b))
    return a;
  if constexpr (Width <= 64) {
    std::int64_t sa = asSigned<Width>(a);
    std::int64_t sb = asSigned<Width>(b);
    return Bits<Width>(static_cast<std::uint64_t>(sa / sb));
  }

  bool aNeg = a.bit(Width - 1);
  bool bNeg = b.bit(Width - 1);
  Bits<Width> ua = aNeg ? detail::negate(a) : a;
  Bits<Width> ub = bNeg ? detail::negate(b) : b;

  Bits<Width> q;
  Bits<Width> r;
  detail::udivrem<Width>(ua, ub, q, r);
  if (aNeg ^ bNeg)
    q = detail::negate(q);
  return q;
}

template <unsigned Width>
constexpr Bits<Width> srem(Bits<Width> a, Bits<Width> b) {
  if (detail::isZero(b))
    return Bits<Width>(0);
  if (detail::isMinSigned(a) && detail::isAllOnes(b))
    return Bits<Width>(0);
  if constexpr (Width <= 64) {
    std::int64_t sa = asSigned<Width>(a);
    std::int64_t sb = asSigned<Width>(b);
    return Bits<Width>(static_cast<std::uint64_t>(sa % sb));
  }

  bool aNeg = a.bit(Width - 1);
  bool bNeg = b.bit(Width - 1);
  Bits<Width> ua = aNeg ? detail::negate(a) : a;
  Bits<Width> ub = bNeg ? detail::negate(b) : b;

  Bits<Width> q;
  Bits<Width> r;
  detail::udivrem<Width>(ua, ub, q, r);
  if (aNeg)
    r = detail::negate(r);
  return r;
}

namespace detail {

template <unsigned... Ws> struct SumWidth;

template <> struct SumWidth<> {
  static constexpr unsigned value = 0;
};

template <unsigned W0, unsigned... Rest> struct SumWidth<W0, Rest...> {
  static constexpr unsigned value = W0 + SumWidth<Rest...>::value;
};

} // namespace detail

template <unsigned OutWidth, unsigned InWidth>
constexpr Bits<OutWidth> trunc(Bits<InWidth> v) {
  static_assert(OutWidth <= InWidth, "trunc requires OutWidth <= InWidth");
  Bits<OutWidth> out;
  for (unsigned i = 0; i < Bits<OutWidth>::kWords; i++)
    out.setWord(i, v.word(i));
  return out;
}

template <unsigned OutWidth, unsigned InWidth>
constexpr Bits<OutWidth> zext(Bits<InWidth> v) {
  static_assert(OutWidth >= InWidth, "zext requires OutWidth >= InWidth");
  Bits<OutWidth> out;
  for (unsigned i = 0; i < Bits<OutWidth>::kWords; i++)
    out.setWord(i, v.word(i));
  return out;
}

template <unsigned OutWidth, unsigned InWidth>
constexpr Bits<OutWidth> sext(Bits<InWidth> v) {
  static_assert(OutWidth >= InWidth, "sext requires OutWidth >= InWidth");

  Bits<OutWidth> out;
  for (unsigned i = 0; i < Bits<OutWidth>::kWords; i++)
    out.setWord(i, v.word(i));

  if (!v.bit(InWidth - 1))
    return out;

  // Fill above the sign bit with 1s.
  unsigned signWord = (InWidth - 1) / Bits<OutWidth>::kWordBits;
  unsigned signBit = (InWidth - 1) % Bits<OutWidth>::kWordBits;
  std::uint64_t topFill = ~std::uint64_t{0};
  if (signBit < 63)
    topFill = (~std::uint64_t{0}) << (signBit + 1u);
  out.setWord(signWord, out.word(signWord) | topFill);
  for (unsigned i = signWord + 1; i < Bits<OutWidth>::kWords; i++)
    out.setWord(i, ~std::uint64_t{0});
  return out;
}

template <unsigned OutWidth, unsigned InWidth>
constexpr Bits<OutWidth> extract(Bits<InWidth> v, unsigned lsb) {
  return trunc<OutWidth, InWidth>(lshr<InWidth>(v, lsb));
}

template <unsigned A> constexpr Bits<A> concat(Bits<A> a) { return a; }

template <unsigned A, unsigned B>
constexpr Bits<A + B> concat(Bits<A> a, Bits<B> b) {
  static_assert(A > 0 && B > 0, "concat inputs must be non-zero width");
  Bits<A + B> aa = zext<A + B, A>(a);
  Bits<A + B> bb = zext<A + B, B>(b);
  return shl<A + B>(aa, B) | bb;
}

template <unsigned A, unsigned B, unsigned C, unsigned... Rest>
constexpr Bits<A + B + C + detail::SumWidth<Rest...>::value>
concat(Bits<A> a, Bits<B> b, Bits<C> c, Bits<Rest>... rest) {
  return concat(a, concat(b, c, rest...));
}

template <unsigned Width> constexpr bool slt(Bits<Width> a, Bits<Width> b) {
  // Signed-compare via sign-bit flip (two's complement ordering).
  Bits<Width> sign = shl<Width>(Bits<Width>(1), Width - 1u);
  return (a ^ sign) < (b ^ sign);
}

template <unsigned Width> constexpr Bits<1> eq(Bits<Width> a, Bits<Width> b) {
  return Bits<1>((a == b) ? 1u : 0u);
}

template <unsigned Width> constexpr Bits<1> ult(Bits<Width> a, Bits<Width> b) {
  return Bits<1>((a < b) ? 1u : 0u);
}

} // namespace gfsim
