# M1 runner default sink clarification

Approved by the user in this thread on 2026-09-30:

> 默认静默；显式 --events 才输出（推荐）

For `pycircuit_system --config config.json` without `--events`, the runner is
silent: neither ordinary Events nor a terminal Result is delivered to stdout.
The model still executes, updates statistics, and returns its status by exit code.
With explicit `--events -` or a new file path, the approved stream contains
committed Events followed by exactly one terminal Result (subject to sink failure).
No config field, record field, lifecycle state or CLI option is added.

This resolves the otherwise unspecified Result destination between M1-C's silent
default sink and one-Result clauses. The independently consulted Astra xhigh
architecture instance recommended this interpretation; user selected it exactly.
The original [M1-C](../c2-m1-module-system.md) bytes remain frozen.
