# Sibling calls on one Counter

`Counter` owns one persistent count, initially 5. Its `increment` Method adds
one while the count is below 7; `read` returns the current count.

`Writer(counter: ac.module)` and `Reader(counter: ac.module)` receive the same existing
Module instance. Each explicitly registers one Rule and calls that instance's
Methods. Passing a Module reference creates no copy of its state. The compiler
routes the declared reference interface and keeps both outer transactions atomic.

On four rising edges the reader observes **5, 6, 7, 7**. Both callbacks start
from committed state, so the writer's current proposal does not change the
reader's observation. Once increment stops firing, read continues with the
committed count. Source declaration and instance construction order provide no
priority.

The three files compile independently; imported consumers use published
interfaces. The closed System uses compiler-generated C++ and Verilog runners.
The older API fixtures in `tests/system/test_sibling_calls_and_order.py` retain
additional migration oracles; their legacy syntax is not current passing evidence.
