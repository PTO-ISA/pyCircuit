#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

auto require(bool ok) { if (!ok) std::abort(); }
template<unsigned W> auto value(const std::string &text) {
  using Bits = gfsim::Bits<W>;
  using Four = gfsim::FourState<W>;
  return gfsim::wire<Bits>::fromPacked(
      text[0]=='x' ? Four::unknown() : text[0]=='z' ? Four::highImpedance()
      : Four::known(Bits{std::stoul(text, nullptr, 16)}));
}
void expect(const gfsim::wire<gfsim::Bits<16>> &wire, const std::string &text) {
  const auto actual=wire.packed(), expected=value<16>(text).packed();
  require(actual.knownMask()==expected.knownMask());
  require(actual.zMask()==expected.zMask());
  require((actual.value() & actual.knownMask()) ==
          (expected.value() & expected.knownMask()));
}
int main(int argc, char **argv) {
  require(argc==2);
  std::ifstream rows(argv[1]); require(rows.good());
  gfsim::WorkExecutor pool(2);
  pyc_root root("raw_memories", &pool);
  root.pyc_7079635f636c6b=value<1>("0");
  root.pyc_7079635f727374=value<1>("0");
  root.byte_addr=value<2>("0");
  root.Reset(); root.Xfer();
  char action;
  std::string clock,reset,ren,address,other,byteAddress,write,writeAddress,
              data,mask,q,qother,bytes;
  unsigned count=0;
  while (rows>>action>>clock>>reset>>ren>>address>>other>>byteAddress
             >>write>>writeAddress>>data>>mask>>q>>qother>>bytes) {
    root.pyc_7079635f636c6b=value<1>(clock);
    root.pyc_7079635f727374=value<1>(reset);
    root.ren=value<1>(ren); root.raddr=value<2>(address);
    root.other_addr=value<2>(other); root.byte_addr=value<2>(byteAddress);
    root.write=value<1>(write); root.waddr=value<2>(writeAddress);
    root.data=value<16>(data); root.mask=value<2>(mask);
    bool failed=false;
    try { root.Work(); } catch (const gfsim::FourStateViolation &) { failed=true; }
    require(failed==(action=='F'));
    if (!failed) {
      expect(root.single,q); expect(root.dual0,q);
      expect(root.dual1,qother); expect(root.bytes,bytes);
    }
    if (action!='C') root.DiscardNext();
    root.Xfer();
    ++count;
  }
  require(rows.eof() && count==31);
}
