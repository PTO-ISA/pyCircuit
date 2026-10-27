# Calls between sibling Module instances

`Receiver` owns a count initially equal to five. Its `accept` Method adds the
argument while the old count is below seven. The Module computes `has_room`
from committed state, and the Method guard captures that value.
`Sender` receives an existing
Module through its `peer: ac.module` argument and calls `peer.accept` from its
explicitly registered `send` Rule. The compiler derives the required Method
signature from the call; the Sender source does not import Receiver.

`Top` creates two Receivers and one Sender. Only the first Receiver is passed
to Sender. Passing that reference retains the existing instance and its state.
The unused Receiver remains at five.

Sender proposes its own count update and log before calling `accept`. Both
Modules' updates and logs belong to the same transaction. Once Receiver reaches
seven, its Method guard cancels Sender's earlier proposal and log too. The
independent observers continue to log the old committed counts.

For the first four cycles, Sender's count is `0, 1, 2, 2`, the receiving count
is `5, 6, 7, 7`, and the unused count is `5, 5, 5, 5`. Each appears at both
low and high sampling epochs. Only the first two cycles publish send/accept logs.
The framework test compiles the three files independently, hides provider bodies
while compiling the consumer, links the explicit closure, and checks generated
C++ with one and two workers and RTL against these expectations.

With an installed toolchain:

```sh
pycircuit run examples/sibling_method_calls --cycles 4
pycircuit run examples/sibling_method_calls --target verilog --cycles 4
```
