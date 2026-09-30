# Collection API status

The former module-family, vector, map, and collection elaboration API is
retired. Dynamic collections and nonempty or shape-dependent static parameters
are outside the current approved source profile.

Unsupported collection shapes must fail closed; they do not select a legacy
builder or QueueGraph compiler. See the
[M5 migration guide](../development/m5-migration.md) for the explicit backlog.
