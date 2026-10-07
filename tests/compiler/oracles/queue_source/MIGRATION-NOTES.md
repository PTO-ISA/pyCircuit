# Queue fixture migration notes

Moved from DUT module docstrings without changing the recorded contracts. These are historical migration notes, not fresh acceptance evidence. Product sources contain the hardware; reference models and checks stay test-only.

## pyc_credit_pipeline

A06 ``credit`` window (``credits=2``), built only from ``ac.queue`` + one ``@ac.rule``.

Historical root: ``examples/agentic-circuit/pipelines/pyc_credit_pipeline.py`` at
baseline ``8887e6dec7b4cc530a9967c860dc6a224d79a4ab`` (20 lines, 24-bit
``CreditToken``, ``@ac.system`` with the implicit chain
``ac.source(depth=4, latency=1)`` -> ``issued.credit(cost=lambda item: item.cycles,
credits=2, depth=4, latency=1)`` -> ``ac.sink``).  Source SHA-256
``2630d0783ddf1fada8ae72e15f2e220e22b20f0ae48b275beed03e60aa879855``
(``A06.md:10``, re-hashed from the baseline blob).

Frozen contract: ``docs/gates/logs/20261007-remaining-migration-orchestration/A06/
oracle.md`` (49 key assertions, ``mapping.json``) plus the ratified design
``docs/work-items/p-credit-queue-credit-design.md`` (907 lines, sha256
``b98a1a2d8b1da337e377414c21adf413c73e894d23282a3b7460a8880c8feca2``).

Contract constants -- these are history, not defaults (design sect. 3.7, T-06):

* ``credits = 2`` credit window slots, expanded as ``CreditState.slot0`` /
  ``CreditState.slot1``.  The historical generator expands slot-by-slot too
  (``QueueGraphPyc.cpp:4390``); this is the ``credits=2`` parameter unrolled,
  not a special case keyed on an example name, width or initial value.
* ``issued`` and ``completed`` are two *separate* ``ac.queue[CreditToken]`` of
  ``depth=4``, ``latency=1``, ``ready_policy="local_occupancy"`` each.  The queue
  depth is NOT the credit window (``QueueGraphPlan.cpp:3791-3800`` attaches
  ``depth``/``latency`` to the *result queue*).
* ``cost`` is ``CreditToken.cycles`` only (``u4``); ``sequence``/``value`` are
  opaque and pass through bit-for-bit.

Two ``ready`` meanings, which must never be confused (P-CREDIT F4 / sect. 4.4,
trap T-03):  the module output ``ready`` is the QUEUE CAPACITY ``issued.in_ready``
(``count < 4``), while the internal admission condition is
``anyFree & ~(out_valid & cost == 0)``.  Writing ``admit = available & ready``
accepts the ``cost == 0`` token into a slot (it would then finish immediately);
this fixture does not do that, and ``A06-T`` must keep them apart.

Named migration differences (V02: each one is named here, never hidden in golden
data).  The full port / Struct / queue tables are repeated below so that the
``CD-ENTRY`` appendix can be written from this docstring alone.

``CD-ENTRY``
    Entry form.  The historical ``@ac.system`` entry had no scalar ports at all
    (``-> None``) and an implicit ``ac.source`` / ``ac.credit`` / ``ac.sink``
    chain.  It is replaced by ``@ac.module def CreditPipeline`` with explicit
    typed ports and an explicit handshake, returning the named nominal Struct
    ``CreditResult``.

    Port table (historical -> this source):

    ============================  ==================  ==========================
    historical                    this source         semantics
    ============================  ==================  ==========================
    (implicit testbench push)     ``valid: ac.u1``    push request into ``issued``
    ``issued`` in_data            ``data: CreditToken``  24-bit payload
    (implicit sink pop)           ``take: ac.u1``     pop request out of ``completed``
    ``completed`` out_data        ``-> CreditResult`` named Struct return
    (no scalar outputs)           --                  the only observable is ``head``
    ============================  ==================  ==========================

    Return Struct field table (declaration order == packed order, MSB first):

    =============  =============  =============================================
    field          type / width   driven by
    =============  =============  =============================================
    ``ready``      ``ac.u1``      ``issued.in_ready`` == queue capacity ``count < 4``
    ``available``  ``ac.u1``      ``completed.out_valid`` (payload is eligible)
    ``head``       ``CreditToken`` / 24   ``completed.out_data`` (packed zero when not valid)
    =============  =============  =============================================

    ``CreditResult`` packs to ``1 + 1 + 24 = 26`` bits and is returned as one
    nominal Struct, so the single generated RTL port is ``result[25:0]`` -- the
    shared driver takes its ``#else`` branch (``queue-source.cpp:91-94``) and
    reads ``output.result``.  No ``Q4_MAPPING`` define and no driver edit.

    Queue table (each queue's parameters and their derivation):

    =============  =====  =======  ===================  ==========================================
    queue          depth  latency  ready_policy         derivation
    =============  =====  =======  ===================  ==========================================
    ``issued``     4      1        ``local_occupancy``  ``A06-O-IF-03``: the historical source
                                                       queue is depth 4 / latency 1 / rate 1;
                                                       ``local_occupancy`` == the historical
                                                       ``canProposePush()`` (0 additional pops,
                                                       ``queue.h:60-63``) -- a slot released on
                                                       edge E is not reused on E.
    ``completed``  4      1        ``local_occupancy``  ``A06-O-IF-04``: the historical credit
                                                       result queue is depth 4 / latency 1 /
                                                       rate 1; same ``canProposePush()`` rule
                                                       (``queue_blocks.h:811``).
    =============  =====  =======  ===================  ==========================================

    ``latency=1`` is "no extra pipeline delay": a token pushed on this edge is
    visible to the consumer from the next epoch's Work (``queue.h:290-297``), so
    the end-to-end distance is ``admission + cost + 2`` epochs (A06-O sect. 3.3).

``IF-1`` (interface difference)
    ``pycircuit.__all__`` has no ``ac.source`` / ``ac.credit`` / ``ac.sink``, so
    the historical implicit boundaries cannot be reproduced literally; explicit
    ports plus two explicit ``ac.queue`` allocations are the only feasible form.
    The credit window itself is *not* a primitive either -- it is the module-scope
    ``CreditState`` register plus the single ``@ac.rule`` below.

``CD-2``
    Not applicable here.  ``CD-2`` (A27) names a *zero* ``@ac.rule`` combinational
    shape; this root uses exactly one ``@ac.rule``, which is the form ratified by
    P-CREDIT Q-D4(a) and by the design's own skeleton (design sect. 11.3).  There
    is no rule-count or expression-form deviation to register.

``C-2``
    Failure face, frozen by PM (LEDGER sect. 39.1 Q-D3).  The historical root
    rejects a head token with ``cycles == 0`` by raising
    ``credit_nonpositive_cost`` and terminating the run at that epoch's commit
    (``A06-O-BD-01``/``BD-03``).  The current language has no failure-report or
    run-terminating channel (``A06-O`` sect. 9 / UNK-03), so the migration is
    **fail-closed permanent stall with no failure report**: the offending token
    stays at the ``issued`` head forever, nothing after it is ever admitted, and
    the tokens already in flight still finish and are consumed (``BD-02`` kept).
    Hooked to ``C-CHECK``.  Explicitly forbidden and not done here: no clamp
    (``cost = 0`` as 1), no wrap/modulo, no silent drop, no skipping past the head,
    no lowering of ``ready``, and no ``assert`` / ``ac.expect``.

``Q-D1(b)`` downgrade
    PM adopted ``ready = issued.in_ready`` (candidate (a)) because it is
    value-for-value consistent with ``A06-O-OUT-11`` ("epoch 6 reaches 4 pending
    tokens").  The canonical-PYG alternative
    ``ready = anyFree & safe`` (``QueueGraphPyc.cpp:5676``) is therefore
    **demoted to an internal admission condition** and is kept, named, in the
    ``admit`` expression below -- it is never the module ``ready`` output.

Residual named limitation (non-blocking, ``G-1``): only ``credits == 2`` is
implemented.  Rules contain no loops, so any other window size needs its own
slot-by-slot expansion; the historical generator expands as well.

## pyc_route_merge_pipeline

A27 route + strict-priority merge, built only from ``ac.queue`` and combinational logic.

Historical root: ``examples/agentic-circuit/pipelines/pyc_route_merge_pipeline.py``
at baseline ``8887e6dec7b4cc530a9967c860dc6a224d79a4ab`` (16 lines, ``@ac.system``
with implicit ``ac.source`` / ``ac.route`` / ``ac.apply`` / ``ac.merge`` / ``ac.sink``).

Frozen contract (A27-O ``oracle.md`` + P-ROUTE ``p-route-queue-arbitration-design.md``):

* Payload is a single ``ac.u64``; that one field is BOTH the route selector and the
  ``+10``/``+20`` operand (``A27-O-IF-04``/``IF-05``).  Declared domain is
  ``[0, 2**64)`` with wrapping arithmetic; the successfully handled selector domain
  is exactly ``{0, 1}``, expressed as two exact equality decodes -- not as a width.
* Six queues, every one ``latency=1`` and explicitly
  ``ready_policy="local_occupancy"``: ``input_queue`` depth 2, ``left`` depth 2,
  ``right`` depth 2, ``left_done`` depth 1, ``right_done`` depth 1, ``merged``
  depth 2.  A slot freed on edge E is first refillable on edge E+1 (no same-cycle
  reuse), so the two depth-1 transform result queues are half-rate; this is
  preserved history, not a migration defect.
* Route selection reads the ``input_queue`` head only: head-of-line blocking, no
  re-route, no skip.  An out-of-domain selector matches neither decode, so nothing
  is pushed, nothing is popped and the token stays at the head forever --
  fail-closed permanent stall, with no clamp / wrap / re-route / drop and no
  lowering of ``in_ready``.
* Merge is strict priority on ``inputs[0] == left_done`` (the ``+10`` branch) with
  no aging, no round-robin and no arbiter state: ``left_done_take = merged_ready``
  and ``right_done_take = merged_ready and not left_done_valid``, which makes
  same-edge uniqueness structural.  Global source order is NOT promised.

Only *future queue results* may be referenced ahead of their allocation; a plain
local may not.  So ``route_take`` / ``left_take`` / ``right_take`` from the design
skeleton are inlined at their single use sites (``left_take``/``right_take`` are
written directly as ``left_done_ready``/``right_done_ready``).  The dataflow is
unchanged: no width, depth, latency, policy or expression result differs.  Verbatim
attempt with the plain locals recorded
``pycircuit compile: native source compiler rejected the source:
loc("pyc_route_merge_pipeline.py":78:9): error: unknown hardware value 'route_take'``.

Named migration differences registered at this root (V02 requires each to be named
rather than hidden in golden data):

``CD-ENTRY``
    Entry form.  Historical ``@ac.system`` with implicit boundaries is replaced by
    ``@ac.module`` with explicitly typed ports, an explicit typed handshake
    (``valid``/``data``/``take``) and a named nominal Struct result
    (``RouteMergeResult``), per
    ``docs/work-items/c-entry-system-replacement-contract.md``.  The output channel
    is the explicit ``merged`` queue.  ``@ac.system`` is named-rejected by the
    current product (``compiler/lib/Compiler/PythonImportRecords.cpp:1971-1974``).

``CD-2``
    Expression form.  Zero ``@ac.rule``: pure combinational expressions plus six
    ``ac.queue`` allocations -- the same shape as the accepted precedents
    ``queue_pipeline`` / ``conditional_pipeline`` / ``broadcast_pipeline`` /
    ``latency``.  All state lives in the six queues, so adding a rule would only
    import old-Q / owner-enable transaction semantics that the historical atomic
    blocks never had.

``IF-1`` (interface difference)
    The front end has no ``ac.source`` / ``ac.sink`` (``pycircuit.__all__``), so the
    historical implicit boundaries cannot be reproduced literally; the explicit
    module form is the only feasible one.

``C-1``
    Out-of-range selector handling.  Historical behaviour is failure code
    ``route_selector_out_of_range`` plus no fire; the migration is a fail-closed
    permanent stall with no failure report.  The data path is identical (no pop, no
    push, no mutation); only diagnosability differs.  Hooked to ``C-CHECK``.

## pyc_reorder_pipeline

A25 ``reorder``: monotone key release from ``ac.table`` + one ``@ac.rule`` + two ``ac.queue``.

Historical root: ``examples/agentic-circuit/pipelines/pyc_reorder_pipeline.py`` at
baseline ``8887e6dec7b4cc530a9967c860dc6a224d79a4ab`` (18 lines, sha256
``1c28700ad166bdfcad2a600654abba13d9817a2af51dbc46f4e526e791b7cc68``; ``@ac.system``
with the implicit chain ``ac.source(Token, depth=8, latency=1)`` ->
``completed.reorder(key=lambda item: item.sequence, capacity=16, start=0,
depth=4, latency=1)`` -> ``ac.sink(retired)``, returning ``None``).

Frozen contract: ``docs/gates/logs/20261007-remaining-migration-orchestration/
A25/oracle.md`` + ``A25/mapping.json`` as corrected by the PM dispatch appendix
``A25/PRE-S-APPENDIX.md`` (correction 1: the block implementation exists;
correction 2: ``capacity`` is not a key window; correction 3: ``local_occupancy``),
the ratified design ``docs/work-items/p-reorder-sequence-reorder-design.md``, the
frozen block ``simulator/gfsim/include/gfsim/queue_blocks.h:400-503``
(``QueueReorder`` / ``Reorder``, sha256 ``e7db1c3357442e14…`` of the baseline blob)
and the frozen MLIR fixture ``tests/mlir/agentic-circuit/ACIR/reorder.mlir``
(sha256 ``00bfef898f079072…``).

Contract constants -- these are history, not defaults (``A25-O`` RT-02/RT-09):

* ``Token{sequence: ac.u32, value: ac.u32}``; both fields travel unchanged
  (``RT-01``/``BD-07``); the key is ``item.sequence`` (``RT-03``).
* ``capacity = 16`` is the EXTENT of ``ac.table[16, Entry]`` -- an OCCUPANCY bound
  (``queue_blocks.h:414`` ``entries_.size() < capacity_``), **not** a key window:
  the frozen block addresses entries by the full key (``:485``
  ``std::map<uint64_t, T> entries_;``), so ``key >= 16`` is legal and is merely
  released late.  ``key % capacity`` is never used (design sect. 4.5/8.4).
* ``start = 0`` is the initial/reset value of ``next_key`` (``:474``
  ``nextKey_ = start_``); ``stale = incoming_key < next_key`` (``:426``).
* The two queues are the historical source queue and the reorder OUTPUT queue:
  input ``depth=8, latency=1``, output ``depth=4, latency=1``, both
  ``ready_policy="local_occupancy"`` (``CD-DEPEND-3`` below).  ``latency=1`` means
  no extra pipeline stage: a token pushed on edge E is first visible on E+1, and
  the end-to-end distance is accept@E -> retire@E+2 -> sink-visible@E+3
  (input queue 1 + the reorder's own 1 + output queue 1), which is the frozen
  block's own cadence (``:439`` reads the pre-Xfer table while ``:453`` inserts,
  so an entry admitted on E can never retire on E).
* ``next_key`` is a real register: reset returns it to ``start = 0``.

``key`` width ruling (A25-S; the appendix left this to S and PM does not pre-judge)
    Two planes are in evidence.  The historical AUTHORING surface declares the key
    expression as ``Token.sequence: ac.u32``, so every key value is in
    ``[0, 2**32)`` -- that is the key's declared domain and it is not widened here
    (``RT-01`` forbids changing the payload).  The frozen SEMANTIC surface stores,
    compares and counts keys in a 64-bit domain: ``queue_blocks.h:425``
    ``const uint64_t key = static_cast<uint64_t>(rawKey);``, ``:483``
    ``uint64_t nextKey_;``, ``:485`` ``std::map<uint64_t, T> entries_;`` and
    ``:469`` ``uint64_t nextKey() const``.  This source therefore stores
    ``Entry.key: ac.u64`` (zero extended) and keeps ``next_key: ac.u64``, so
    ``next_key`` is exactly as wide as the ruled key domain, as the appendix
    requires.  The reason for the wider domain is the frozen one: a 32-bit
    ``next_key`` would wrap on the 2**32nd retirement and silently turn every
    already-retired key back into a legal one (``key < next_key`` would stop
    meaning "stale").  This is a declaration-width choice, not a behaviour change:
    every reachable key is < 2**32 and ``next_key`` can never exceed the largest
    in-flight key + 1 < 2**32, so a u32 and a u64 implementation agree on every
    reachable stimulus.

    Named difference recorded with it: the frozen MLIR fixture ``reorder.mlir``
    declares ``!ac.queue<i64>`` with an IDENTITY key lambda -- a SCALAR i64
    payload at the MLIR level, a different plane from the historical authoring
    surface (struct payload, key = ``sequence``).  Its ``i64`` is not adopted as
    the payload type (that would break ``RT-01``); it corroborates, but does not
    decide, the 64-bit key domain.

Named migration differences (V02: each one named here, never hidden in golden
data).  The full port / Struct / queue tables are repeated below so that the
``CD-ENTRY`` appendix can be written from this docstring alone.

``CD-ENTRY``
    Entry form.  Historical ``@ac.system`` with implicit boundaries and a
    ``-> None`` return, replaced by ``@ac.module def ReorderPipeline`` with
    explicitly typed ports, an explicit typed handshake and a named nominal Struct
    result (``ReorderResult``), per
    ``docs/work-items/c-entry-system-replacement-contract.md``.

    Port table (historical -> this source):

    ============================  ==================  ==========================
    historical                    this source         semantics
    ============================  ==================  ==========================
    (implicit testbench push)     ``valid: ac.u1``    push request into ``completed``
    ``completed`` in_data         ``data: Token``     64-bit payload
    (implicit sink pop)           ``take: ac.u1``     pop request out of ``retired``
    ``retired`` out_data          ``-> ReorderResult`` named Struct return
    ============================  ==================  ==========================

    Return Struct field table (declaration order == packed order, MSB first):

    =============  ==============  ============================================
    field          type / width    driven by
    =============  ==============  ============================================
    ``ready``      ``ac.u1``       ``completed.in_ready`` == queue capacity
                                   (``count < 8``), never the key state
    ``available``  ``ac.u1``       ``retired.out_valid``
    ``head``       ``Token`` / 64  ``retired.out_data`` (packed zero when invalid)
    ``next_key``   ``ac.u64``      the rule's pre-edge ``next_key``, mirroring the
                                   frozen ``nextKey()`` (``queue_blocks.h:469``)
    ``fault``      ``ac.u1``       ``in_valid and free_valid and (stale or dup)``
                                   == the exact edge on which the frozen block
                                   would ``setRuntimeFailureCode`` (``:414-433``)
    =============  ==============  ============================================

    ``ReorderResult`` packs to ``1 + 1 + 64 + 64 + 1 = 131`` bits and is returned
    as one nominal Struct, so the single generated RTL port is ``result[130:0]``:
    the shared driver takes its ``#else`` branch (``queue-source.cpp:91-94``) and
    reads ``output.result``.  No ``Q4_MAPPING`` define and no driver edit.

    Queue table (each queue's parameters and their derivation):

    =============  =====  =======  ===================  ==========================================
    queue          depth  latency  ready_policy         derivation
    =============  =====  =======  ===================  ==========================================
    ``completed``  8      1        ``local_occupancy``  ``A25-O-IF-02``/``RT-02``: the historical
                                                        source queue is depth 8 / latency 1 and
                                                        the frozen block pops it with
                                                        ``input_.canProposePop()`` (``:415``).
    ``retired``    4      1        ``local_occupancy``  ``A25-O-IF-03``: the frozen graph attaches
                                                        the reorder's own ``depth=4``/``latency=1``
                                                        to its OUTPUT queue
                                                        (``QueueGraphPlan.cpp:3733-3744``), and
                                                        the block pushes it with
                                                        ``output_.canProposePush()`` (``:440``).
    =============  =====  =======  ===================  ==========================================

    ``local_occupancy`` is exactly the frozen ``canProposePush()`` /
    ``canProposePop()`` reading: 0 additional pops (``queue.h:60-68``), so a slot
    freed on edge E is first refillable on edge E+1.  The reorder's own table
    capacity gate reads the OLD occupancy as well (``:414`` vs ``:448``).

``IF-1`` (interface difference)
    ``pycircuit.__all__`` has no ``ac.source`` / ``ac.sink`` / ``reorder``, so the
    historical implicit boundaries cannot be reproduced literally.  Explicit ports
    plus two explicit ``ac.queue`` allocations are the only feasible form; the
    ordering function itself is not a primitive either -- it is the module-scope
    ``ac.table[16, Entry]`` plus the single ``@ac.rule`` below.  No ``ac.reorder``
    op, type, attribute or front-end branch is requested or used.

``CD-2``
    Not applicable here.  ``CD-2`` (A27) names a *zero* ``@ac.rule`` combinational
    shape; this root uses exactly one ``@ac.rule``, which is the form ratified by
    the design (sect. 8.2) and required by the table/key search.

``CD-DEPEND-3``
    Ready policy.  The historical ``.reorder`` path used ``canProposePush()``
    (0 additional pops), but the frozen queue's own admission arithmetic is
    order-dependent same-epoch reuse: ``canProposePushWithAdditionalPops(count,
    additionalPops)`` subtracts ``min(committed_.size(), popProposalCount_ +
    additionalPops)``, so a pop already proposed in the SAME epoch counts.
    The current language expresses either 0-additional-pops
    (``local_occupancy``) or unconditional same-edge reuse (``downstream_pop``),
    each covering only half of that behaviour, so both queues take
    ``local_occupancy`` -- conservative, and never inventing a reuse history may
    not have.  Observable consequence: a full-but-draining output queue does not
    accept a replacement push on the edge it pops, so a "consumer-first" schedule
    that history would have allowed does not occur in the migration.  Hooked to
    ``C-CHECK``.

``C-R1``
    Illegal-key observability.  The frozen block reports
    ``reorder_negative_key`` / ``reorder_stale_key`` / ``reorder_duplicate_key``
    through ``setRuntimeFailureCode`` and then does not pop and does not push
    (``:422/:427/:431``); the current language has no source-side failure channel
    (``ac.expect`` is rejected by both emitters; ``__all__`` has no ``expect``).
    The migration is therefore a FAIL-CLOSED PERMANENT STALL with no failure
    report -- the offending head token stays at the input queue head, nothing is
    popped, stored, overwritten or dropped, and the next edge re-derives the same
    verdict.  As a named, PM-adopted difference the module additionally exposes
    two PURE OBSERVATION outputs that never gate the data path: ``next_key``
    (mirrors the frozen ``nextKey()``, ``:469``) and ``fault``
    (``in_valid and free_valid and (stale or dup)`` == the exact edge on which the
    frozen block sets a failure code; ``free_valid`` is part of the term because
    the frozen block short-circuits and never inspects a key when the table is
    full, ``:414``).  Hooked to ``C-CHECK``.

    Illegal-key policy, per the PM appendix sect. 5 (all of it is what the frozen
    block does, modulo the missing failure report):

    * missing key (``key > next_key``): WAIT, never skip -- only a key equal to
      the committed ``next_key`` is ever released, and it is found by the
      ``occupied and key == next_key`` search, not by a queue position.
    * duplicate key and already-retired key (``key < next_key``): fail-closed
      permanent stall; no accept, no overwrite, head unchanged.
    * ``key >= start + capacity``: LEGAL (the appendix's correction 2): the
      table is key-addressed and ``capacity`` counts occupied entries.
    * negative key: structurally inexpressible (``sequence: ac.u32`` has no
      negative values; the frozen block's negative branch is compiled out for an
      unsigned key, ``:420-424``); it is NOT claimed to be testable.
    * negative ``start``: not exercisable here -- this source exposes no ``start``
      parameter at all, the historical constant ``0`` is realised as
      ``next_key``'s initial/reset value, so there is nothing that could be
      negative.  ``A25-O``'s ``NG-04`` is therefore N/A for this source.
    * X/Z key or selector: a different expectation class, routed through the
      existing unknown-control channel, not through this policy.

    Explicitly forbidden and not done here: no clamp, no wrap, no modulo index,
    no re-route, no silent drop, no silent overwrite, no lowering of ``in_ready``
    on an illegal head (``ready`` stays pure queue capacity), no ``assert`` and no
    ``ac.expect``.

Source-structure note (design sect. 8.1 / A27-S landing lesson)
    The ``@ac.rule`` call comes FIRST and its arguments forward-reference the two
    future ``ac.queue`` results (``language.md:442-443``).  The reverse order is
    rejected (P-REORDER probe ``s3``: ``error: unknown hardware value 'move'``),
    and a plain local may not be forward-referenced either (A27-S:
    ``error: unknown hardware value 'route_take'``).  Inside the rule every table
    query precedes every table write, because queries read the values at the call
    site while earlier writes are visible to later reads
    (``language.md:408-409``); writing the retirement before querying the free
    slot would introduce same-cycle slot reuse and change the drain cadence
    (design sect. 6.2).

