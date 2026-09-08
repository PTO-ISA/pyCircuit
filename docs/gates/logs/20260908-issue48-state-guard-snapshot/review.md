# Issue #48 state-guard snapshot repair

The first concrete I2 token test accepted an operand response but never emitted
its execute request. The generated policy evaluated `not execute_outstanding`
against the selected branch's proposed `True` value, making output presence
permanently false, even though the rule declared and should have read the
committed state owner.

The frontend now keeps persistent state names in blocking, effect, and output
presence guards bound to the tick-start committed snapshot. Assignments in the
selected branch still create the new state proposal and may feed later local
expressions, but cannot rewrite the predicate that selected the transaction.
The repair is generic and contains no DavinciOO-specific path or symbol.

A minimal source regression proves that the rule condition uses `ac.var.read`
while `ac.var.assign` uses the proposed value. The existing executable optional
output/backpressure fixture confirms the repaired IR still preserves
candidate/output separation through QueueGraph and generated gfsim C++.
