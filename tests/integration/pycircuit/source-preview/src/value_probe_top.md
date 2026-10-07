# ValueProbeTop

This portless, no-write probe emits separate one-value observations for a
finite integer state initialized to 7 and a boolean state initialized to true,
plus a literal-only print event. The system test mutates a saved valid final
snapshot to exercise signed narrow integer value formatting while retaining
the boolean type. The Python source does not claim signed-range syntax support.
