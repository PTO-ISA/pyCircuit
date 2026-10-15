#pragma once
// Embedded verbatim in generated standalone projects.
inline constexpr const char *cppRuntime = R"ACIR(#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <tuple>
#include <limits>
namespace acir_sim {
// Dispatch aliases without copying resources or losing component identity.
template<size_t I=0,class Tuple,class F> decltype(auto) visit_resource(Tuple &&resources,size_t index,F &&f) {
  if (index==I) return f(std::get<I>(resources));
  if constexpr(I + 1 < std::tuple_size_v<std::remove_reference_t<Tuple>>)
    return visit_resource<I+1>(resources,index,std::forward<F>(f));
  else throw std::out_of_range("resource index");
}
inline bool check(bool value) {
  if (!value) throw std::runtime_error("model assertion failed");
  return true;
}
template<class T> constexpr unsigned width = std::is_same_v<T,bool> ? 1 : sizeof(T)*8;
template<class T> constexpr uint64_t bits(T x) { return uint64_t(x); }
template<class T> constexpr bool negative(T x) { return (bits(x) >> (width<T>-1)) != 0; }
template<class T> constexpr T narrow(uint64_t x) {
  if constexpr(std::is_same_v<T,bool>) return (x & 1) != 0;
  else return T(x);
}
template<class T> constexpr T neg(T x) { return narrow<T>(uint64_t(0)-bits(x)); }
template<class T> constexpr bool less_signed(T x,T y) {
  return negative(x) != negative(y) ? negative(x) : x < y;
}
template<class To,class From> constexpr To sext(From x) {
  if constexpr (width<From> == 64) return To(x);
  else return To(negative(x) ? bits(x) | (~uint64_t(0) << width<From>) : bits(x));
}
template<class T> T shift(T x,T y,int kind) {
  auto n=bits(y);
  if(n>=width<T>) throw std::runtime_error("shift amount exceeds MLIR bit width");
  if(kind==0) return narrow<T>(bits(x)<<n);
  if(kind==1 || !negative(x) || n==0) return T(bits(x)>>n);
  return T((bits(x)>>n) | (~uint64_t(0) << (width<T>-n)));
}
template<class T> T divide(T x,T y,bool sign,bool remainder,bool floor=false) {
  if(!y) throw std::runtime_error("division by zero");
  // Magnitudes and modular negation avoid signed MIN/-1 and overflow UB.
  bool nx=sign && negative(x), ny=sign && negative(y);
  uint64_t a=bits(nx?neg(x):x), b=bits(ny?neg(y):y);
  T q=T(a/b), r=T(a%b);
  if(remainder) return nx?neg(r):r;
  if(nx!=ny) q=neg(q);
  if(floor && nx!=ny && r) q=T(bits(q)-1);
  return q;
}
template<class T> struct Cell {
  T value{}, next{};
  void begin() { next=value; }
  T read() const { return value; }
  void write(T v) { next=v; }
  void commit() { value=next; }
};
template<class T> struct Signal {
  T value{}; bool valid=false;
  void begin() { value={}; valid=false; }
  T read() const { return value; }
  void write(T v) { value=v; valid=true; }
  void commit() {}
};
template<class T,size_t P> struct EHR {
  T value{}; std::array<std::optional<T>,P> writes{};
  void begin() { for(auto &w:writes) w.reset(); }
  T read(size_t port) const {
    T result=value;
    for(size_t p=0;p<port;++p) if(writes.at(p)) result=*writes[p];
    return result;
  }
  void write(size_t port,T v) { writes.at(port)=v; }
  void commit() { for(auto &w:writes) if(w) value=*w; }
};
template<class T,size_t N> struct Queue {
  std::array<T,N> data{}, pushes{};
  size_t count=0, pops=0, pushed=0;
  void begin() { pops=0; pushed=0; }
  bool empty() const { return count==0; }
  bool full() const { return count-pops>=N; }
  uint32_t size() const { return uint32_t(count); }
  T read() const { return count?data[0]:T{}; }
  void pop() { if(pops>=count) throw std::runtime_error("Queue underflow"); ++pops; }
  void push(T v) { pushes.at(pushed++)=v; }
  void commit() {
    if(count-pops+pushed>N) throw std::runtime_error("Queue overflow");
    for(size_t i=pops;i<count;++i) data[i-pops]=data[i];
    count-=pops;
    for(size_t i=0;i<pushed;++i) data[count++]=pushes[i];
  }
};
template<class T> void full_mask(T &v) {
  if constexpr(std::is_same_v<T,bool>) v=true;
  else for(auto &x:v) full_mask(x);
}
template<class T> T intersect(T a,const T &b) {
  if constexpr(std::is_same_v<T,bool>) return a && b;
  else { for(size_t i=0;i<a.size();++i) a[i]=intersect(a[i],b[i]); return a; }
}
// Overloads are declared before recursion, including generated structures' ADL.
template<class T> requires std::is_integral_v<T> void json(std::ostream &o,T v);
template<class T,size_t N> void json(std::ostream &o,const std::array<T,N> &v);
template<class... T> void json(std::ostream &o,const std::tuple<T...> &v);
template<class T> requires std::is_integral_v<T> void json(std::ostream &o,T v) {
  if constexpr(std::is_same_v<T,bool>) o<<(v?"true":"false"); else o<<uint64_t(v);
}
template<class T,size_t N> void json(std::ostream &o,const std::array<T,N> &v) {
  o<<'['; for(size_t i=0;i<N;++i) { if(i)o<<','; json(o,v[i]); } o<<']';
}
template<class... T> void json(std::ostream &o,const std::tuple<T...> &v) {
  o<<'['; size_t i=0; std::apply([&](const auto &...x){ ((o<<(i++?",":""),json(o,x)),...); },v); o<<']';
}
template<class T,size_t N> void json(std::ostream &o,const Queue<T,N> &v) {
  o<<'['; for(size_t i=0;i<v.count;++i) { if(i)o<<','; json(o,v.data[i]); } o<<']';
}
} // namespace acir_sim
)ACIR";
