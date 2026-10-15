#include "model.hpp"
#include <chrono>
#include <iostream>
#include <memory>
int main(){
using Clock=std::chrono::steady_clock;
auto construction=Clock::now();
auto m=std::make_unique<acir_generated::Model>();
auto initialized=Clock::now();
for(unsigned i=0;i<WARMUP;++i)m->tick();
auto start=Clock::now();
for(unsigned i=0;i<MEASURED;++i)m->tick();
auto end=Clock::now();
std::cout<<"{\"construct_ns\":"<<std::chrono::duration_cast<std::chrono::nanoseconds>(initialized-construction).count()<<",\"run_ns\":"<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<",\"final_state\":";
m->trace(std::cout,WARMUP+MEASURED);std::cout<<"}\n";
}
