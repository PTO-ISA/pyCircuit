# Benchmarks

The old builder/QueueGraph performance drivers have been retired. Current
M5 evidence measures correctness of the serial source-owned route. Scale,
incremental compilation and parallel scheduling benchmarks belong to M6 and
must invoke the public compile/link/emit driver against a pinned candidate.
Historical benchmark implementations remain in Git, outside the active route.
