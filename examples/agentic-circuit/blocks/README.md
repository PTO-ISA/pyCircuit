# Parameterized blocks

This directory contains small examples of reusable, statically specialized
building blocks. `multirate_compute.py` demonstrates a compute block with a
compile-time rate and latency contract.

- `bit_primitives.py` exercises priority encoding, population count, and
  leading/trailing zero count through one typed pipeline.
- `popcount.py` focuses on population count.
- `count_leading_zeros.py` covers both zero-count directions.
