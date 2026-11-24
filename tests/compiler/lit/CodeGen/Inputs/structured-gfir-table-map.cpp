#include "pycircuit_system.hpp"
#include <cstdlib>
void require(bool ok) { if(!ok)std::abort(); }
template<unsigned W> auto bits(unsigned v,unsigned k,unsigned z=0) {
  using B=gfsim::Bits<W>;
  return gfsim::wire<B>::fromPacked(gfsim::FourState<W>::fromMasks(B{v},B{k},B{z}));
}
template<unsigned W> void expect(const gfsim::wire<gfsim::Bits<W>> &w,unsigned v,unsigned k,unsigned z=0) {
  auto p=w.packed(); using B=gfsim::Bits<W>;
  require(p.knownMask()==B{k} && p.zMask()==B{z} && (p.value()&p.knownMask())==B{v&k});
}
int main() {
  pyc_root root("table_map");
  root.base=bits<8>(0xa0,255);root.data=bits<8>(0xa5,255);
  for(unsigned index: {0u,1u,2u,3u,8u}) {
    root.index=bits<4>(index,15);root.Work();
    expect(root.row0,index==0?0xa5:0xa0,255);
    expect(root.row1,index==1?0xa5:0xa0,255);
    expect(root.row2,index==2?0xa5:0xa0,255);
    expect(root.selected,index<3?0xa5:0,index<3?255:0);
    expect(root.in_range,index<3,1);
  }
  for(unsigned z:{0u,1u}) {
    // 000X / 000Z can match rows 0 and 1, but cannot match row 2.
    root.index=bits<4>(0,14,z);root.Work();
    expect(root.row0,0xa0,0xfa);expect(root.row1,0xa0,0xfa);
    expect(root.row2,0xa0,255);expect(root.selected,0,0);expect(root.in_range,0,0);
  }
  // Equal all-Z branches preserve Z through unknown selection.
  root.base=bits<8>(0,0,255);root.data=bits<8>(0,0,255);root.Work();
  expect(root.row0,0,0,255);expect(root.row1,0,0,255);expect(root.row2,0,0,255);
}
