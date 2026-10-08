# DesignTop

This is a compiler-executed `@system` that instantiates two copies of `Counter`.
The input values are 3 and 10; parent-owned output registers start at 100 and
200. Each child observes the old parent output and reports its old private
count while capturing its input. A global stateless rule writes both parent
outputs from the returned old counts. Three regular-clock cycles therefore log
100/200, 0/0 and 3/10, and report 0/0, 3/10 and 3/10. The generated runner samples
each old-Q view at both low and high clock phases. The fixture contains no
authored clock, reset, clock loop or runner.
