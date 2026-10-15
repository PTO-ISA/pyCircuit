__attribute__((noinline)) unsigned fact(unsigned n) { return n<2 ? 1 : n*fact(n-1); }
unsigned algorithm(void) { return fact(6); }