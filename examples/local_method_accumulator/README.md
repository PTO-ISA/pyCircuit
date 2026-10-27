# Local Method accumulator

The System adds three on each permitted cycle, stopping at nine. `has_space`
and `increment` are Value Methods; `count_sample` and `finish` are Action
Methods; `add` is an ActionValue Method that returns its computed sum and calls
`count_sample`. All calls belong to the explicitly registered `receive` Rule.

Method parameters are ordinary values. Reassigning `increment`'s parameter
does not change the caller's `amount`: the two calls return three and five,
while `amount` remains two. State reads always observe committed old values.
The `receive` Rule's log contains both the old total and the explicitly returned
new sum, even though the state assignment precedes the log.

`finish` calls the guarded `require_permit` after `add` has proposed its updates
and `receive` has proposed its log. A false guard cancels the whole transaction, including
both state updates and both logs. The toggle and observer remain independent.

The first eight cycles begin with `(total, accepted, permit)` equal to
`(0, 0, 0)`, `(0, 0, 1)`, `(3, 1, 0)`, `(3, 1, 1)`, `(6, 2, 0)`,
`(6, 2, 1)`, `(9, 3, 0)` and `(9, 3, 1)`. Each is observed at both low and
high sampling epochs. Only cycles one, three and five publish receive logs.
The framework acceptance test uses these independent expectations for generated
C++ with one and two workers and for RTL.

With an installed toolchain:

```sh
pycircuit run examples/local_method_accumulator --cycles 8
pycircuit run examples/local_method_accumulator --target verilog --cycles 8
```
