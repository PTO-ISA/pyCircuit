# Structured-spec API status

The former `pycircuit.spec` structures, signature builders, port bundles, and
wiring helpers are retired. The current source profile has no public typed
external-port schema; module arguments are only the approved connection forms
and static arguments are empty.

Do not use these historical builders in new source. See the
[language reference](language.md) for current module connections and the
[M5 migration guide](../development/m5-migration.md) for deferred interface
capabilities.
