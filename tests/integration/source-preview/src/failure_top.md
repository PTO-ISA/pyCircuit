# FailureTop

This negative fixture has a real design invariant: `count` must remain zero at
the start of each registered tick. The first tick commits; the second tick
fails. Its pre-check log/report probe verifies that observations from the
failed epoch are discarded. This is a failure-path design fixture, not the
normal `DesignTop` and not a testbench embedded in that design.
