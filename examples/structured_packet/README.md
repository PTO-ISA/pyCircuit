# Structured packet processing

`Packet` carries a nested `Header`, nominal `Mode` and an 80-bit data field.
`Processor.process` constructs a new packet with its opcode incremented and mode
set to `DONE`, retaining the packet in module State and incrementing its accepted
count. The data field passes through unchanged. The Method returns its explicitly
constructed result; reading retained State still sees the old committed packet.

`Stage` connects a typed Packet Module input and output through one Rule that
calls the existing Processor instance. The System produces opcodes 0–3. The
delayed consumer receives opcodes 1–4 in order. Default depth-2 input and output Queues belong to the connecting System.
When the output fills, input consumption, packet retention and the accepted
counter all hold together. Queue admission uses occupancy before the edge.

The focused `transactional-structured-packet.test` independently models every
observation epoch. Its lanes cover C++ workers 1/2, RTL, reset with pending packets,
and an 80-bit X/Z payload alongside known header control through C++ and Icarus.
Reset tests adapt only physical reset stimulus in the generated harness; they
compare the old-state Work observations during reset and the complete replay
after reset commits. Sources compile separately using provider interfaces without provider bodies. A distinct
record with identical layout must be rejected at the nominal interface boundary.
This new example does not claim migration of historical structured examples.

With an installed toolchain:

```sh
pycircuit run examples/structured_packet
pycircuit run examples/structured_packet --target verilog
```
