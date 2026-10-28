# Resource composition

`Top` sends 1, 2, 3, 4 through `Relay` to its child `SplitStage`. The relay
passes the existing shared `Counter` instance down to that child and reexports
two independent scalar Queue outputs. One Rule consumes an input, adds it to
the shared counter and produces `value + 10` and `value + 20` atomically.

The left consumer runs immediately; the right consumer starts at cycle 10.
Each connection has the default depth of two. When the right Queue fills,
backpressure holds the input, both output enqueues, the accepted count and the
shared counter update together. Admission uses old occupancy, so a full Queue
cannot borrow space from a same-edge pop. The resulting sequences are
11–14 and 21–24, and the shared counter finishes at 10.

A separate sibling `Observer` reads the shared counter through the same
instance reference. It also reads an independent `Counter` that remains zero,
demonstrating distinct state identity for instances of the same definition.
Observations see committed state, including during blocked cycles.

The CMake source list explicitly compiles each unit before linking the closed
System. Both generated targets consume the same verified artifact. The focused
resource-composition regression compares every observation epoch with an
independent old-occupancy model, including the delayed-consumer interval.
