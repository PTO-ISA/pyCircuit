# Agentic Circuit design record status

This page formerly tracked open designs and implementation status for the
Agentic Circuit and QueueGraph route. That route is retired from the active
product workflow. The old status statements and evidence references are
historical records, not claims about the current compiler candidate.

The active product contract is the bounded source-unit profile recorded in
[Decision 0283](../rfcs/pyc6-decisions.md#decision-0283-approved-source-unit-hardware-cutover-for-the-scalar-profile).
M5 verification is in progress; do not infer that retirement work is complete
from the historical issue resolutions recorded here.

Queues, buffer libraries, full `@system`/EXPECT behavior, memory/CDC,
multi-clock, four-state values, and external typed ports remain capability
backlog with independent contract and oracle requirements. See the
[M5 migration guide](m5-migration.md) for the current scope.
