#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <string>

void require(bool ok) { if (!ok) std::abort(); }
using Word = std::string;
// Test-owned character truth tables, independent of gfsim operations/masks.
Word logic(const Word &a, const Word &b, char op) {
  Word out(8, 'x');
  for (unsigned i = 0; i < 8; ++i) {
    const char x = a[i], y = b[i];
    const bool known = (x == '0' || x == '1') && (y == '0' || y == '1');
    if (op == '&' && (x == '0' || y == '0')) out[i] = '0';
    else if (op == '|' && (x == '1' || y == '1')) out[i] = '1';
    else if (known) out[i] = op == '&' ? (x == '1' && y == '1' ? '1' : '0')
                          : op == '|' ? (x == '1' || y == '1' ? '1' : '0')
                                      : (x != y ? '1' : '0');
  }
  return out;
}
Word invert(Word a) {
  for (char &bit : a) bit = bit == '0' ? '1' : bit == '1' ? '0' : 'x';
  return a;
}
Word shift(const Word &a, const Word &amount, bool left) {
  if (amount.find_first_not_of("01") != Word::npos) return Word(8, 'x');
  unsigned count = 0;
  for (char bit : amount) count = count * 2 + unsigned(bit == '1');
  Word out(8, '0');
  if (count < 8)
    for (unsigned i = 0; i < 8 - count; ++i)
      if (left) out[i] = a[i + count]; else out[i + count] = a[i];
  return out;
}
Word select(char selector, const Word &a, const Word &b) {
  if (selector == '0') return b;
  if (selector == '1') return a;
  Word out(8, 'x');
  for (unsigned i = 0; i < 8; ++i) if (a[i] == b[i]) out[i] = a[i];
  return out;
}
template <unsigned W> auto bits(const Word &s) {
  require(s.size() == W);
  unsigned value = 0, known = 0, z = 0;
  for (char bit : s) {
    value = value * 2 + unsigned(bit == '1');
    known = known * 2 + unsigned(bit == '0' || bit == '1');
    z = z * 2 + unsigned(bit == 'z');
  }
  using B = gfsim::Bits<W>;
  return gfsim::wire<B>::fromPacked(gfsim::FourState<W>::fromMasks(B{value}, B{known}, B{z}));
}
template <class T> void expect(const T &actual, const Word &expected) {
  const auto reference = bits<8>(expected).packed(), observed = actual.packed();
  require(observed.knownMask() == reference.knownMask());
  require(observed.zMask() == reference.zMask());
  require((observed.value() & reference.knownMask()) == reference.value());
}
int main() {
  // Six known samples select both branches and cover zero, boundary and large
  // counts. Mixed samples independently exercise data, selector and count X/Z.
  const std::array<std::array<Word, 4>, 13> rows{{
      {"10100101", "00111100", "00000000", "0"},
      {"11000011", "10100101", "00000001", "1"},
      {"00011000", "11110000", "00000111", "0"},
      {"11111111", "01010101", "00001000", "1"},
      {"01010101", "10011001", "01000000", "0"},
      {"01100110", "00001111", "11111111", "1"},
      {"10xz0101", "0z1x1010", "00000001", "1"},
      {"z01x1100", "10101010", "00000010", "0"},
      {"00000000", "00000000", "00000001", "x"},
      {"10100101", "00111100", "00000010", "z"},
      {"01010101", "10011001", "0000000x", "1"},
      {"zzzzzzzz", "00000000", "0000000z", "z"},
      {"x0z10101", "11110000", "00001000", "1"},
  }};
  pyc_root root("native-output-group");
  root.Reset(); root.Xfer();
  for (const auto &row : rows) {
    const auto &[a, b, count, selector] = row;
    const auto x = logic(a, "01011010", '^');
    const auto z = logic(x, logic(b, "11110000", '&'), '|');
    const auto left = shift(invert(z), count, true), right = shift(x, count, false);
    const auto picked = select(selector[0], logic(logic(left, right, '^'), "11001100", '&'), z);
    const auto mirror = logic(picked, a, '^');
    const auto joined = logic(picked, mirror, '^');
    root.a = bits<8>(a); root.b = bits<8>(b);
    root.count = bits<8>(count); root.selector = bits<1>(selector);
    root.Work();
    expect(root.picked, joined); expect(root.left, left); expect(root.right, right);
    expect(root.echo, a); expect(root.mirror, mirror);
    root.Xfer();
    // Icarus consumes oracle expectations, never values read from the DUT.
    std::cout << a << ' ' << b << ' ' << count << ' ' << selector << ' '
              << joined << ' ' << left << ' ' << right << ' ' << a << ' ' << mirror << '\n';
  }
}
