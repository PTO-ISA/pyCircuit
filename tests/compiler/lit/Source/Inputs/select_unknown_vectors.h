#pragma once
#include <string_view>
struct UnknownRow {
  std::string_view inputs[2];
  std::string_view golden[2];
};
// Fixed oracles for physically compatible opaque imported output and small
// literal.
#ifdef SELECT_FOUR_STATE
const UnknownRow unknownRows[] = {
    {{"z0000011", "1"}, {"z0000011", "00000011"}},
    {{"x0000011", "0"}, {"00000011", "x0000011"}},
    {{"00000011", "x"}, {"00000011", "00000011"}},
    {{"11111111", "z"}, {"xxxxxx11", "xxxxxx11"}},
};
#else
const UnknownRow unknownRows[] = {
    {{"11111111", "1"}, {"11111111", "00000011"}},
    {{"00000000", "0"}, {"00000011", "00000000"}},
    {{"00000011", "1"}, {"00000011", "00000011"}},
    {{"10101010", "0"}, {"00000011", "10101010"}},
};
#endif
