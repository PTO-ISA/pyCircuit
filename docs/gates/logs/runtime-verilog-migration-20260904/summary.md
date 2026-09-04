# Runtime Verilog/crawler integration gate

This run validates the migration into `pyCircuit-PTO-ISA` on branch
`feat/runtime-verilog-crawler-integration`.

- The packaged catalog contains 132 accepted runtime components. Vendoring
  checked 449 closure files, including 25 license files, with no failures.
- Six representative packaged wrappers passed Verilator and Yosys. The
  functional priority-encoder oracle also passed.
- The runtime Python test suite passed 85/85 tests. The priority encoder demo
  passed for both the single configuration and the `w8_lo_to_hi` and
  `w16_hi_to_lo` parameter configurations through Python → ACIR → PYC IR →
  gfsim C++ → Verilog → Verilator/Yosys.
- The repository-local crawler scan completed with 13 accepted entries and no
  errors when using the explicit 1024-file exploratory bound. The default
  release-safe bound is intentionally smaller; the integrated runtime source
  currently contains 435 RTL files, so broad scans must choose the bound
  explicitly.
- The WSL toolchain rebuilt successfully with Ninja `-j1`. No CTest tests are
  registered in that build tree. The local API-hygiene pre-commit hook passed;
  the full pre-commit bootstrap was not completed because its first-time Node
  environment initialization did not finish during the bounded run.

Generated build, crawl, and demo artifacts remain under `.pycircuit_out/` and
are intentionally not part of the commit.
