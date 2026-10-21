# Sibling calls on one Counter

`Counter` owns one persistent count, initially 5. Its `increment` Rule adds one
while the count is below 7. The callable `read` Rule declares
`after=(increment,)`.

`Writer(counter: Counter)` and `Reader(counter: Counter)` receive the same existing
Module instance. Their nested Rules call that instance directly. Passing a
Module reference creates no copy of its state. The compiler routes the calls and
propagates the callable order to the two outer transactions.

On four rising edges the reader observes **6, 7, 7, 7**. Its first observation
includes the writer's candidate. Once increment stops firing, read continues
with the committed count. Source declaration and instance construction order do
not provide priority.

The three files compile independently; imported consumers use published
interfaces. The closed System uses compiler-generated C++ and Verilog runners.
API tests in `tests/system/test_sibling_calls_and_order.py` own the additional
alias, nested reference, rollback, saved-anchor and four-state checks.
