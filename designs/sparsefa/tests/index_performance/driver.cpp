// Public-port adapter: supplies memory contents only, never DUT results/state.
#include "pycircuit_system.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
using Command=fa2::fa2_scheduler::TensorCommand;
using Reply=fa2::fa2_interfaces::MemoryReply;
static void require(bool x,const char* s){if(!x) throw std::runtime_error(s);}
template<unsigned N> auto known(uint64_t x){return gfsim::wire<gfsim::Bits<N>>::known(gfsim::Bits<N>{x});}
template<unsigned N,class T> uint64_t field(const T& x,unsigned low){require(x.isFullyKnown(),"unknown public output");return gfsim::extract<N>(x.packed(),low).value().value();}
int main(int argc,char** argv){try{
 unsigned n=argc>1?std::stoul(argv[1]):128, delay=argc>2?std::stoul(argv[2]):1;
 std::string mode=argc>3?argv[3]:"valid"; unsigned workers=argc>4?std::stoul(argv[4]):1;
 require(n<=2048&&delay>=1,"invalid arguments");
 const uint64_t Q=0x10000,K=uint64_t{1}<<36,V=uint64_t{1}<<40,O=0x40000,C=0x50000,I=0x60000;
 std::vector<uint32_t> tokens; uint32_t key_length=n;
 if(argc>5){std::ifstream f(argv[5]);require(bool(f),"input prefix file");f>>key_length;uint64_t x;while(f>>x){require(x<=0xffffffff,"u32 token");tokens.push_back(x);}require(tokens.size()==n,"prefix count");}
 else for(unsigned i=0;i<n;i++)tokens.push_back(n-1-i);
 const std::string original_mode=mode; int cancel_offset=argc>6?std::stoi(argv[6]):-1;
 bool restarted=false;long second_response=-1,cancel_latency=-1;
 Command c{}; c.valid=decltype(c.valid){1};c.tag=decltype(c.tag){73};c.batches=decltype(c.batches){1};c.heads=decltype(c.heads){1};c.queries=decltype(c.queries){1};c.keys=decltype(c.keys){key_length};c.dimensions=decltype(c.dimensions){1};c.scale=decltype(c.scale){0x3f800000};c.indexed=decltype(c.indexed){1};
 c.q_base=decltype(c.q_base){Q};c.k_base=decltype(c.k_base){K};c.v_base=decltype(c.v_base){V};c.o_base=decltype(c.o_base){O};c.count_base=decltype(c.count_base){C};c.index_base=decltype(c.index_base){I};
 c.q_row_stride=decltype(c.q_row_stride){2};c.k_row_stride=decltype(c.k_row_stride){2};c.v_row_stride=decltype(c.v_row_stride){2};c.o_row_stride=decltype(c.o_row_stride){2};c.count_row_stride=decltype(c.count_row_stride){4};c.index_row_stride=decltype(c.index_row_stride){8192};
 pyc_dut dut(workers); pyc_dut::Inputs in{}; Reply m{};
 auto apply=[&](bool clock){in.command=decltype(in.command)::known(c);in.memory=decltype(in.memory)::known(m);in.completion_ready=known<1>(1);in.pyc_7079635f636c6b=known<1>(clock);in.pyc_7079635f727374=known<1>(0);dut.drive(in);};
 in.cancel=known<1>(0);apply(false);dut.system().Build();dut.system().Reset();
 bool sent=false,pending=false,cancelled=false; uint64_t due=0,reply=0;bool error=false;
 long accepted=-1,firstq=-1,firstkv=-1; unsigned indices=0,keys=0,values=0,writes=0;
 bool stalled=false;uint64_t held_address=0,held_data=0,held_size=0,held_write=0;
 auto started=std::chrono::steady_clock::now();
 for(uint64_t cycle=0;cycle<12000000;cycle++){
   bool cancel_now=mode=="cancel" && !cancelled && (cancel_offset<0 ? (pending&&indices==2) : (second_response>=0 && cycle==uint64_t(second_response+cancel_offset)));
   if(cancel_now)cancelled=true;
   in.cancel=known<1>(cancel_now);c.valid=decltype(c.valid){!sent};
   bool ready=delay==1 || cycle%5!=2;
   m.request_ready=decltype(m.request_ready){ready};m.valid=decltype(m.valid){pending&&cycle>=due};m.data=decltype(m.data){reply};m.error=decltype(m.error){error};
   apply(false);require(dut.system().Step()==gfsim::SimStepResult::Running,"low step failure");
   auto output=dut.sample();auto req=output.result_0;auto status=output.result_1;
   if(stalled&&!cancel_now){require(field<1>(req,101)&&field<64>(req,36)==held_address&&field<32>(req,4)==held_data&&field<3>(req,1)==held_size&&field<1>(req,100)==held_write,"stalled request stability");}
   stalled=field<1>(req,101)&&!ready;
   if(stalled){held_address=field<64>(req,36);held_data=field<32>(req,4);held_size=field<3>(req,1);held_write=field<1>(req,100);}
   bool accept_command=!sent&&field<1>(status,43);
   bool take_response=bool(m.valid.value())&&field<1>(req,0);
   bool take_request=field<1>(req,101)&&ready;
   bool done=field<1>(status,42);auto code=field<4>(status,6);
   if(accept_command){sent=true;accepted=cycle;}
   if(take_response){pending=false;if(indices==2&&firstq<0&&second_response<0)second_response=cycle;}
   if(take_request){
     require(!pending,"multiple outstanding memory requests");uint64_t a=field<64>(req,36),size=field<3>(req,1);bool write=field<1>(req,100);reply=0;error=false;
     if(write){require(a==O&&size==2,"unexpected output write");require(field<32>(req,4)==(n?0x3c00:0),"independent zeroQK constantV oracle");++writes;}
     else if(a==C){require(size==4&&indices==0,"count request");reply=n;}
     else if(a>=I&&a<I+8192){require(a==I+4*indices&&size==4,"index request order/width");reply=tokens.at(indices);if(mode=="duplicate"&&indices==n-1)reply=tokens.front();if(mode=="duplicate-first"&&indices==1)reply=tokens.front();if((mode=="range"||mode=="memory-range")&&indices==n-1)reply=key_length;if((mode=="memory"||mode=="memory-range")&&indices==n-1)error=true;++indices;}
     else if(a==Q){require(indices==n&&size==2,"Q before full prefix validation");if(firstq<0)firstq=cycle;reply=0;}
     else if(a>=K&&a<K+2*uint64_t(key_length)){require(a==K+2*uint64_t(tokens.at(keys))&&size==2,"K order/width");if(firstkv<0)firstkv=cycle;++keys;reply=0;}
     else if(a>=V&&a<V+2*uint64_t(key_length)){require(a==V+2*uint64_t(tokens.at(values))&&size==2,"V order/width");++values;reply=0x3c00;}
     else throw std::runtime_error("unexpected memory address");
     pending=true;due=cycle+delay;
   }
   apply(true);require(dut.system().Step()==gfsim::SimStepResult::Running,"high step failure");
   if(done){
     unsigned expected=mode=="valid"?0:(mode=="duplicate"||mode=="duplicate-first")?4:mode=="range"?3:(mode=="memory"||mode=="memory-range")?5:6;
     require(code==expected&&field<32>(status,10)==73,"completion status/tag");require(!pending,"completion before drain");
     if(mode=="valid")require(indices==n&&keys==n&&values==n&&writes==1,"complete valid trace");
     else require(firstq<0&&keys==0&&values==0&&writes==0,"invalid prefix accessed tensor");
     if(mode=="duplicate-first")require(indices==2,"duplicate must terminate before later index requests");
     if(mode=="cancel"&&!restarted){cancel_latency=cycle-accepted;restarted=true;mode="valid";sent=false;accepted=-1;firstq=-1;firstkv=-1;indices=keys=values=writes=0;second_response=-1;continue;}
     auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();
     std::cout<<"{\"mode\":\""<<original_mode<<"\",\"n\":"<<n<<",\"response_latency\":"<<delay<<",\"workers\":"<<workers<<",\"accept_edge\":"<<accepted<<",\"first_q_latency\":"<<(firstq<0?-1:firstq-accepted)<<",\"first_kv_latency\":"<<(firstkv<0?-1:firstkv-accepted)<<",\"done_latency\":"<<(cycle-accepted)<<",\"index_reads\":"<<indices<<",\"output_writes\":"<<writes<<",\"cancel_latency\":"<<cancel_latency<<",\"restart_checked\":"<<restarted<<",\"host_ms\":"<<ms<<"}\n";return 0;
   }
 }
 throw std::runtime_error("cycle limit");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
