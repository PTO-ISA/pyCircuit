#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <fstream>
#include <string>
void require(bool ok) { if (!ok) std::abort(); }
template<unsigned W> auto bits(const std::string &s) {
  using F=gfsim::FourState<W>; using B=gfsim::Bits<W>;
  return gfsim::wire<B>::fromPacked(s=="x" ? F::unknown() : s=="z" ? F::highImpedance() : F::known(B{std::stoul(s,nullptr,16)}));
}
int main(int argc,char **argv) {
  require(argc==2); std::ifstream rows(argv[1]); require(rows.good());
  gfsim::WorkExecutor pool(2); pyc_root root("guards",&pool);
  root.pyc_7079635f636c6b=bits<1>("0"); root.pyc_7079635f727374=bits<1>("0");
  root.Reset();root.Xfer();
  std::string clk,rst,a,b,sel,value,sibling; unsigned ok,count=0;
  while(rows>>clk>>rst>>a>>b>>sel>>value>>sibling>>ok) {
    root.pyc_7079635f636c6b=bits<1>(clk);root.pyc_7079635f727374=bits<1>(rst);
    root.a=bits<1>(a);root.b=bits<1>(b);root.select=bits<1>(sel);
    root.Work();
    require(root.value.isFullyKnown() && root.sibling.isFullyKnown());
    require(root.value.packed().value()==bits<8>(value).packed().value());
    require(root.sibling.packed().value()==bits<8>(sibling).packed().value());
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure)==bool(ok));
    if(!ok)root.DiscardNext();
    root.Xfer();++count;
  }
  require(rows.eof() && count==16);
}
