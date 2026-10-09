# Canonical C++ runtime and Verilog coding style

This directory contains hardware primitives and simulation callbacks. These
rules adapt the supplied coding standard to the current gfsim/Verilog scope.
They do not restore CycleAwareSignal, Structural, Agentic Circuit, PYC or pycc
compilation routes. New concepts or public interfaces require user approval.
The component contracts and C++/RTL mapping are recorded in
[gfsim/README.md](gfsim/README.md) and
[verilog/README.md](verilog/README.md). Verification status belongs to the exact
candidate's gate results; this style guide does not establish compilation,
simulation or acceptance by itself.

## Standard module organization

- Follow the [module organization](MODULE_TEMPLATE.md) in both targets.
  Use four regions: interface; internal declarations and child connections;
  combinational rules; owned register transfer.
- Keep parameters, port names, directions and bit widths identical across the
  C++ and RTL module. T selects a bits or struct payload; address width and
  depth are configurable compile-time parameters. Derive data width from T,
  never from host integer size or struct padding.
- Declare all rule intermediates in the internal declaration region. Neither
  target's design rules contain local declarations, allocation or owned storage.
  Rules read input/current and compute output/next using combinational logic.
- RTL design ports and rule results are wire driven by continuous assign.
  Standard design modules contain no always blocks. reg belongs to procedural
  storage inside DFF/DFFE primitives. Memory behavioral libraries may retain
  procedural reads/writes, as explicitly approved by the user.
- The transfer region instantiates standard dff/dffe/memory; posedge always
  stays in the library. C++ Xfer calls the matching owned component's Xfer.
  Do not also update the same Q through inline sequential RTL or native C++.
- Define each child module in its own file. Declare owning C++ child pointers
  and named connections before rule definitions; construct children before
  Work and call their hooks through ->. Do not place child class/module
  definitions in the parent source. System invokes only top-level modules.
  Parents call their nested children's hooks
  recursively, and children are not registered again in the system. Parent Work
  order must satisfy combinational dependencies.

## Ownership and hardware semantics

- Keep one owning implementation of each primitive. No prefixed aliases,
  compatibility wrappers or a second simulator/IR pipeline.
- C++ and Verilog must describe the same clock edge, reset priority, enable,
  hold, read latency, write strobe and read-during-write behavior.
- Primitive names and behavior must not depend on a consumer design, source
  filename, selected fixture, statement layout or fixed field/node count.
- Put sequential RTL in primitives. Design modules instantiate those primitives;
  technology-specific cells belong in an explicitly approved mapping.
- Expose semantic mismatches immediately. Do not hide X, discard transactions
  silently or repair source/IR meaning inside one emitter.

## C++ / gfsim

- Headers live in include/gfsim; use namespace gfsim and no pyc prefix.
- Use LLVM C++ formatting: two-space indentation, explicit standard headers,
  one member per declaration and no new formatting dependency.
- Rules are combinational subfunctions of Work, not module instances or
  lifecycle objects. Work() invokes them and samples standard-component input
  nets, has no epoch argument and does not advance current storage.
- System calls top-level Work(); each parent calls its children's Work().
  System Xfer performs complete precheck/discard before any commit, then calls
  top-level Xfer(); parents recursively call child Xfer(). Failure recursively
  discards the complete tree through the same ownership hierarchy.
- A module commits only its owned storage. Child references do not allocate
  copies of parent registers or memories.
- Current reads remain stable during Work. Xfer performs the update; enable
  false holds Q. Reset priority and init/unknown behavior are explicit.
- Callback hooks are lifecycle methods, not a runtime rule-object hierarchy.
  Do not introduce another scheduler, transaction context or semantic graph.
- Avoid allocator/exception work in commit; noexcept promises must be supported
  by the contained value's operations. Report invalid enabled inputs rather than
  silently truncating, overwriting a pending write or discarding accepted work.
- Hardware ports uniformly use wire<T>; the signal owns the packed FourState
  value, known and Z planes. Do not split binary payload from optional
  verification snapshots. Ordinary integer zero is not a representation of X.

## Verilog

- Sources live in include/verilog, with lowercase primitive names and no pyc
  prefix. Files and comments are ASCII; source filename preservation elsewhere
  does not permit non-ASCII generated RTL contents.
- Use four-space indentation. Declare one signal per line, use aligned ANSI
  ports, explicit direction/type/width and deterministic declaration order.
- Size datapath constants explicitly. Integer elaboration parameters/loop
  indices are structural arithmetic, not implicit datapath width conversions.
- Connect instances through named intermediate nets. Do not place arithmetic,
  Boolean expressions, concatenations or conditional expressions in port bindings.
- Use continuous assign for design rules; declare each result as wire and
  assign a complete expression. Use nonblocking assignments for clocked
  storage inside canonical primitives. Primitive combinational procedures,
  where required (such as byte memory), use blocking assignments and assign
  every output on every path.
- DFFE naturally holds when en is false. Do not add a Q recirculation mux while
  keeping enable high merely to model stall.
- Preserve true priority. Use AND-OR one-hot selection only with a proven
  exclusivity condition and matching onehot0 verification.
- Primitive procedural logic is permitted; ordinary generated design modules
  should instantiate the canonical sequential/memory primitives.
- Parameter checks and unknown-input diagnostics are simulation-only and must
  not invent hardware initialization or synthesis behavior.

## Registers and memory

- Specify reset polarity/synchrony/priority and power-on behavior. A no-reset
  register has unknown power-on Q, not implicit zero.
- Preserve wire<T> masks across connections and storage. Bits and value()
  alone contain binary payload; do not consume an invalid memory Q
  through that view or use binary arithmetic as implicit four-state arithmetic.
- Synchronous SRAM Q is stateful and has an explicit lifetime. Initial Q is X
  in behavioral simulation and expires at the second consecutive idle rising
  edge. A read restarts that lifetime; a write does not. Reset invalidates Q.
  Controls and enabled addresses/write data/strobes must be known. With no read
  enable, a continuous read address must always be known on evaluation.
- Memory reset and storage initialization differ: reset must not implicitly
  erase persistent SRAM contents.
- Capture a synchronous SRAM result using the delayed read-fire indication.
  At the capture edge, consumers see pre-NBA Q; use the live/captured choice
  appropriate to that cycle and retain the correct bank/way provenance.
- A C++ Work reads old memory before Xfer writes it, matching Verilog old-data
  behavior for simultaneous same-address read/write.
- Address handling, invalid lanes and partial final byte lanes must be stated
  and identical across targets. Check the complete address before narrowing an
  index: out-of-range reads return zero and writes are ignored. Check byte lanes
  independently without address-addition overflow. Do not conceal a difference
  with different tests.
- Output validity must clear when the declared interface says idle is invalid.
  Do not retain stale-valid output just because an enable register holds data.

## Handshake, timing and future verification

- ready && valid means the transaction was actually accepted. Audit stall,
  conflict, reset/flush, route and downstream-ready crosses without silent loss.
- Shared-FSM validity and data enables must stay aligned. Independent resource
  completion must neither require accidental simultaneous availability nor
  duplicate an already completed access while waiting for another resource.
- Drive stimulus before sampling edges. Stable DFF results are normally sampled
  after NBA; a one-cycle result being cleared at that edge may require active-edge
  sampling. Do not mechanically impose one sampling phase on every signal.
- Gate assertions by actual unit validity. Case equality is not a general fix for
  unknown-data assertions. Post-SRAM timing starts when SRAM Q physically exists.
- When verification is authorized, use the smallest relevant primitive/behavior
  reproducer and common C++/RTL stimulus. Check old-Q, reset, hold, discard,
  simultaneous access, X, transaction loss/duplication and deterministic output.
- Stress uses recorded deterministic seeds and meaningful transaction accounting.
  Do not freeze incidental SSA names, formatter layout or a design recipe in tests.
- Save reviewable RTL diffs and accurately report missing parity, stress, lint
  or implementation coverage. Style compliance alone is not semantic acceptance.
