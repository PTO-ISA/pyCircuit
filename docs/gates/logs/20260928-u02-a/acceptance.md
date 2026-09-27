# U02-A source module acceptance

Accepted isolated candidate: `946cd022ba70c6160d58b5a3234bcde6e56a29c6`.
The independent review and test rounds ran against its 38-file pre-commit tree at
`03625a3dc3b368be9dbac4bfff7ffdb1d3b54466`. Before committing, PM checked
that every reviewed file retained its recorded SHA-256; the reviewed manifest
itself is `c4be7e586f8cca49c11d2e358c5ca996cd1d4c5fe943e12f19cfcb830013ba92`.
The commit contains exactly those 38 files and the isolated checkout is clean.

The actual Packet → AccumulatorProbe → ProbeRoot producers compile independently.
ProbeRoot compiles with provider Python sources and bodies hidden, using only
their explicit owning interfaces. This exercises module import, source DFFE,
owned reset, rule/instance shape, registered effects, and header authority.

Independent validation: 19 U02-A native cases, 64 preserved native cases, and
76 system cases passed, for **159/159 with zero skips**. CTest passed **4/4**;
clang-format, Ruff format/check, and diff checks passed. Independent Sol high
source/test review found no remaining U02-A issue. See [test results](test-b/results.md),
[commands](test-b/commands.txt), and [review](review-b/summary.md).

This acceptance does not cover the original Accumulator/Core mathematics and
control flow, header/body link, final IR closure, C++ or Verilog execution,
installed SDK, or complete C2-N1 categories. The candidate remains isolated;
it is not the product-route hard break.
