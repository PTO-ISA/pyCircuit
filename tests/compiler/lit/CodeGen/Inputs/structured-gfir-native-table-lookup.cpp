#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <string>

using Word = std::string;
void require(bool ok) { if (!ok) std::abort(); }
// Independent per-bit truth tables. No DUT/runtime arithmetic forms the oracle.
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
Word invert(Word word) {
  for (char &bit : word) bit = bit == '0' ? '1' : bit == '1' ? '0' : 'x';
  return word;
}
Word encode(Word word) {
  word = invert(logic(word, "01011010", '^'));
  word = logic(logic(word, "11001100", '&'), "00110011", '|');
  word = invert(logic(word, "10100101", '^'));
  return logic(logic(word, "11110000", '&'), "00001111", '^');
}
template <unsigned W> auto bits(const Word &word) {
  require(word.size() == W);
  unsigned value = 0, known = 0, z = 0;
  for (char bit : word) {
    value = value * 2 + unsigned(bit == '1');
    known = known * 2 + unsigned(bit == '0' || bit == '1');
    z = z * 2 + unsigned(bit == 'z');
  }
  using B = gfsim::Bits<W>;
  return gfsim::wire<B>::fromPacked(gfsim::FourState<W>::fromMasks(B{value}, B{known}, B{z}));
}
template <class T> void expect(const T &actual, const Word &expected) {
  const auto reference = bits<T::width>(expected).packed(), observed = actual.packed();
  require(observed.knownMask() == reference.knownMask());
  require(observed.zMask() == reference.zMask());
  require((observed.value() & reference.knownMask()) == reference.value());
}
int main() {
  const std::array<std::array<Word, 3>, 6> cases{{
      {"10100101", "xz10zx01", "0"}, // Unselected X/Z must not affect the known row.
      {"xz10zx01", "01011010", "1"},
      {"10xz0101", "11111111", "0"}, // Selected mixed row takes the cold path.
      {"00000000", "zzzzzzzz", "1"},
      {"10100101", "10100101", "x"}, // Unknown indices do not merge equal rows.
      {"00110011", "11001100", "z"},
  }};
  pyc_root root("native-table-lookup");
  root.Reset(); root.Xfer();
  for (const auto &row : cases) {
    const auto &[first, second, index] = row;
    const bool knownIndex = index == "0" || index == "1";
    const Word selected = knownIndex ? (index == "0" ? first : second) : Word(8, 'x');
    const Word range = knownIndex ? "1" : "x";
    const Word joined = logic(selected, encode(selected), '^');
    root.rows.element(0) = bits<8>(first); root.rows.element(1) = bits<8>(second);
    root.index = bits<1>(index);
    root.Work();
    expect(root.joined, joined); expect(root.selected, selected); expect(root.in_range, range);
    root.Xfer();
    // RTL receives expected values from the oracle, never sampled DUT values.
    std::cout << first << second << ' ' << index << ' ' << joined << ' '
              << selected << ' ' << range << '\n';
  }
}
