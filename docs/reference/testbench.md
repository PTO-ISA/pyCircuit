# Testbench API status

The former Python testbench and `Tb` API are retired. A full public `@system`
role, drive, expectation, and external-port contract is not approved for the
current scalar profile.

Current generated runner output is controlled by a finite configuration and an
optional explicit `--events` sink. This tooling path does not imply external
DUT ports or a testbench language. See [runtime behavior](../architecture/simulation.md)
and the [M5 migration guide](../development/m5-migration.md).
