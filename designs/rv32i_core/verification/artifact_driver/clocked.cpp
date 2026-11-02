
#include "pycircuit_system.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <stdexcept>
using Root=pyc_root;
auto bit(bool x){return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{x});}
template<class Wire> Wire decode(const std::string& text){
 if(text.size()!=Wire::width)throw std::runtime_error("input width");
 using Mask=typename Wire::packed_type::Mask;Mask value{0},known{0},z{0};
 for(unsigned i=0;i<Wire::width;++i){char c=text[Wire::width-1-i];auto b=uint64_t{1}<<(i%64);
  if(c=='1')value.setWord(i/64,value.word(i/64)|b);
  if(c=='0'||c=='1')known.setWord(i/64,known.word(i/64)|b);
  else if(c=='z')z.setWord(i/64,z.word(i/64)|b);
  else if(c!='x')throw std::runtime_error("input symbol");}
 return Wire::fromPacked(Wire::packed_type::fromMasks(value,known,z));}
template<class Wire> std::string bits(const Wire& w){auto p=w.packed();std::string s;
 for(unsigned n=Wire::width;n;--n){unsigned i=n-1;auto b=uint64_t{1}<<(i%64);
  s+=p.knownMask().word(i/64)&b?(p.value().word(i/64)&b?'1':'0'):(p.zMask().word(i/64)&b?'z':'x');}return s;}
template<class Wire> bool same(const Wire& a,const Wire& b){
 if(!a.isFullyKnown()||!b.isFullyKnown())return false;
 auto x=a.packed(),y=b.packed();for(unsigned i=0;i<(Wire::width+63)/64;++i)
  if(x.value().word(i)!=y.value().word(i))return false;return true;}
class System final:public gfsim::SimSystem{Root& r;gfsim::SimFailureInfo f{};
 bool Precheck()noexcept override{return r.pyc_validate_checks(f);}
 public:explicit System(Root& x):r(x){} };
int main(){try{
 Root root("clocked");root.bus=decode<decltype(root.bus)>(std::string(decltype(root.bus)::width,'0'));
 root.pyc_7079635f636c6b=bit(false);root.pyc_7079635f727374=bit(true);System system(root);system.AddModule(root);system.Build();system.Reset();
 root.pyc_7079635f727374=bit(false);root.Work();root.DiscardNext();
 std::string command,raw;
 while(std::cin>>command){
  if(command=="S"){root.Work();std::cout<<bits(root.result_1)<<std::endl;root.DiscardNext();continue;}
  if(!(std::cin>>raw))throw std::runtime_error("input record missing");
  root.bus=decode<decltype(root.bus)>(raw);
  root.pyc_7079635f636c6b=bit(true);root.pyc_7079635f727374=bit(command=="R");
  if(command=="Q"){
   root.Work();gfsim::SimFailureInfo f{};
   if(!root.pyc_validate_checks(f)){root.DiscardNext();throw std::runtime_error("query precheck failed");}
   std::cout<<bits(root.result_0)<<std::endl;root.DiscardNext();continue;
  }
  auto before=root.result_1;
  bool failed=false;
  try{failed=system.Step()!=gfsim::SimStepResult::Running;}
  catch(const std::exception&){failed=true;}
  if(failed){
   root.DiscardNext();root.bus=decode<decltype(root.bus)>(std::string(decltype(root.bus)::width,'0'));
   root.pyc_7079635f636c6b=bit(false);root.pyc_7079635f727374=bit(false);root.Work();
   std::cout<<"FAIL "<<bits(root.result_1)<<std::endl;root.DiscardNext();continue;
  }
  auto result=root.result_0;
  root.pyc_7079635f636c6b=bit(false);
  if(system.Step()!=gfsim::SimStepResult::Running)throw std::runtime_error("low sample failed");
  std::cout<<bits(result)<<' '<<(same(before,root.result_1)?1:0)<<std::endl;
 }
 return 0;
}catch(const std::exception& e){std::cerr<<"clocked bridge: "<<e.what()<<'\n';return 1;}}
