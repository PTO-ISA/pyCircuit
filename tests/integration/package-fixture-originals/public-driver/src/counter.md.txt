# Counter module

Each instance owns one `count` register. On a registered tick, it reports the
current count and proposes the previous count on the parent-owned output, then
captures the current input for the next cycle. The log observes the output's
old value. Two instances exercise source-owned module reuse with distinct
instance state.
