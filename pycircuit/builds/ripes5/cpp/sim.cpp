#include "model.hpp"
#include <charconv>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
int main(int argc,char **argv) {
  try {
    uint64_t cycles=0; std::string path;
    for(int i=1;i<argc;++i) {
      std::string_view option=argv[i];
      if(option=="--cycles" && i+1<argc) {
        std::string_view value=argv[++i];
        auto [end,ec]=std::from_chars(value.data(),value.data()+value.size(),cycles);
        if(ec!=std::errc{} || end!=value.data()+value.size()) throw std::runtime_error("invalid cycle count");
      } else if(option=="--trace" && i+1<argc) path=argv[++i];
      else throw std::runtime_error("usage: sim --cycles N [--trace FILE]");
    }
    std::ofstream trace;
    if(!path.empty()) { trace.open(path); if(!trace) throw std::runtime_error("cannot open trace"); trace<<"{\"trace\":["; }
    auto model=std::make_unique<acir_generated::Model>();
    for(uint64_t i=0;i<cycles;++i) { model->tick(); if(trace.is_open()) { if(i)trace<<','; model->trace(trace,i); } }
    if(trace.is_open()) { trace<<"]}\n"; trace.flush(); if(!trace) throw std::runtime_error("cannot write trace"); }
    return 0;
  } catch(const std::exception &e) { std::cerr<<"sim: "<<e.what()<<'\n'; return 1; }
}
