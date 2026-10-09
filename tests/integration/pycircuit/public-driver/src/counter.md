# Counter module

Each instance owns one eight-bit `count` register. Its stateless rule logs the
incoming value of the parent's output register, reports the old private count,
returns that count in `CounterResult`, and proposes the input as the next count.
The parent writes its own output register from the returned old count. Two
actual module instances exercise source-owned reuse with distinct instance
state and observation paths.
