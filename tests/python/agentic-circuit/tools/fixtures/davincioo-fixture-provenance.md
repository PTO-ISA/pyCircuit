# DavinciOO trace fixture provenance

`davincioo-valid.jsonl` contains the first two records of
`model/tests/fixtures/traces/examples_beginner_matmul.pto.trace` from DavinciOO
commit `e73633301cabed0d871ea5ff66e76a91df870aeb`. That checkout pins PTO-ISA
commit `f6d0567c1cae2d6a7b0ebaf7ad0e3b93f8a39da3`.

`davincioo-valid.pto-trace.json` is the deterministic output of:

```text
python tools/import-davincioo-pto-trace.py \
  tests/python/agentic-circuit/tools/fixtures/davincioo-valid.jsonl \
  tests/python/agentic-circuit/tools/fixtures/davincioo-valid.pto-trace.json \
  --source-program examples/beginner_matmul
```
