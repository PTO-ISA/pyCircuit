# Cycle balancing status

Automatic CycleAwareSignal pipeline balancing is a retired feature. It is not
part of the current authoring contract and must not be used in examples, build
instructions, or support claims.

The active profile has one default clock and scalar current/next state. Rules
read current values during an epoch, propose next values, and the system Xfer
stage commits successful proposals. Authors do not select logical cycles or
ask the compiler to insert balancing registers.

Multiple clocks, CDC, explicit cycle-domain authoring, and cross-domain
balancing remain outside the approved scalar profile. See the
[language reference](../reference/language.md) and
[M5 migration guide](../development/m5-migration.md).
