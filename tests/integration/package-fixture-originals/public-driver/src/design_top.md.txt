# DesignTop

This is a portless design module that instantiates two copies of `Counter`.
The input values are 3 and 10; parent-owned output registers start at 100 and
200. Each child observes its old output, reports its old private count, and
transfers that count to the parent output while capturing its own input. The
fixture contains no stimulus, testbench checker, clock loop, or model runner.
