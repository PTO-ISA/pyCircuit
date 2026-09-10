# RegisterFile cycle-0 snapshot regression

- Decision: 0148
- Scope: `designs/blocks/RegisterFile/regfile.py` and
  `tests/unit/test_register_file.py`
- Status: pass
- Root cause: the next-state mux first read each `ForwardSignal` after
  `domain.next()`, so the Q operand was tagged at logical cycle 1 while write
  hit/data operands retained cycle 0. Automatic balancing inserted 2,560
  `_v6_bal_*` registers and delayed every write by one hardware cycle.
- Fix: save immutable cycle-0 `CycleAwareSignal` views for both banks before
  advancing the domain and use those views for read and next-state logic.
- Committed regression: `test_register_file_keeps_cycle_zero_state_snapshot`
  elaborates the default RegisterFile to canonical PYC and requires zero
  `_v6_bal_*` values and exactly 256 intended `pyc.reg` operations.
- Fresh dual-backend result: the current checkout built with `--target both`;
  Verilator reached `$finish` at 27ns and the manifest's C++ executable printed
  `OK` with return code 0.
- ForwardSignal result: all 15 focused unit tests passed.
- Targeted nightly result: `regfile` and the following `bypass_unit` case both
  passed with return code 0.
- Targeted pre-commit result: all applicable hooks passed after ruff corrected
  the new test's import grouping.
- Repository check: target and whole-worktree `git diff --check` passed.

Exact commands are in `commands.txt`. Raw focused outputs and return codes use
the matching `*.stdout`, `*.stderr`, and `*.rc` names. Nightly case output is
under `cases/run_sims_nightly/`; the machine-readable aggregate is
`summary.json`.
