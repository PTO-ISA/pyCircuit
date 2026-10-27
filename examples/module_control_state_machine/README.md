# Module control state machine

The System selects waiting, working, or done with ordinary `if/elif/else`.
Its Rules start work, finish it, and count the completion. `toggle_permit` and
`observe` are autorules: each registers once without a trailing call. The
ordinary Rules are registered explicitly in their selected branches.

Waiting initially selects `receive`, whose permit guard is false. The state and
completion count hold; the `else` branch does not run. The independent autorule changes the
permit for the next cycle. Every read observes committed state, including the
logs following assignments within a Rule.

The five cycles begin with `(state, permit, completed)` equal to
`(0, 0, 0)`, `(0, 1, 0)`, `(1, 0, 0)`, `(2, 1, 0)`, and `(0, 0, 1)`.
Each appears at both low and high sampling epochs. Only the selected, admitted
Rule logs its operation. The framework test checks these independently derived
observations through generated C++ with one and two workers and through RTL.

With an installed toolchain:

```sh
pycircuit run examples/module_control_state_machine --cycles 5
pycircuit run examples/module_control_state_machine --target verilog --cycles 5
```
