# H3 contributor checklist

All 240 source candidates are accounted for. Hardware H1/H2/H3 is inside NDF L2; NDF L0 and L1 are intent and behavior. Checkbox completion requires the accepted disposition and its implementation/mapping evidence, not a generated file.

`declared` means a port is explicitly present in the cited source, not verified at this new location. `proposed` is a suggested contract; `unresolved` requires a decision. Detailed cards contain full ports, state, evidence and open questions.

## H1 SPE

### H2 BCTRL

- [ ] [DAV-SPE-BCTRL-BFU-0001 — Block Fetch Unit](spe/bctrl/bfu.md) · review; IN: prediction: PredictionSidecar (unresolved); redirect: RecoveryRedirect (unresolved); cancel: FetchCancel (unresolved); OUT: block_fetch: BlockFetchRequest (unresolved)
- [ ] [DAV-SPE-BCTRL-BIFU-0001 — Block Instruction Fetch Unit](spe/bctrl/bifu.md) · alias; IN: block_fetch: BlockFetchRequest (proposed); OUT: frontend_txn: BlockFrontendTxn (proposed)
- [ ] [DAV-SPE-BCTRL-BIQ-0001 — Block Issue Queue](spe/bctrl/biq.md) · leaf; IN: engine_issue: EngineIssuePacket (proposed); engine_accept: EngineIssueAck (proposed); OUT: engine_request: EngineIssuePacket (proposed)
- [ ] [DAV-SPE-BCTRL-BISQ-0001 — Block Issue Scheduler Queue](spe/bctrl/bisq.md) · leaf; IN: bisq_req: CommandPacket (proposed); tmu_rename_ack: TileRenameAck (proposed); readiness: OperandReadyUpdate (proposed); recovery: Recovery …; OUT: no_effect_resolve: NoEffectResolve (proposed)
- [ ] [DAV-SPE-BCTRL-BMDB-0001 — Block Memory Disambiguation Buffer](spe/bctrl/bmdb.md) · leaf; IN: block_memory_observation: BlockMemoryObservation (proposed); brob_query_result: BrobLiveness (proposed); OUT: block_violation: BlockMemoryViolation (proposed)
- [ ] [DAV-SPE-BCTRL-BROB-0001 — Block Reorder Buffer](spe/bctrl/brob.md) · leaf; IN: block_alloc_req: BlockAllocRequest (proposed); scalar_handoff_req: HandoffReq (proposed); engine_resolve: EngineResolve (proposed); tile_pu…; OUT: block_close: BlockCloseResult (proposed)
- [ ] [DAV-SPE-BCTRL-CBRG-0001 — Command IQ to BISQ Bridge](spe/bctrl/cbrg.md) · state_schema; IN: cmdp_command: CommandPacket (proposed); OUT: bisq_command: CommandPacket (proposed)
- [ ] [DAV-SPE-BCTRL-F5-0001 — Block Frontend Stage 5](spe/bctrl/f5.md) · alias; IN: block_frontend: BlockFrontendTxn (proposed); OUT: registered_block_frontend: BlockFrontendTxn (proposed)

### H2 DTU

- [ ] [DAV-SPE-DTU-CTR-0001 — Counter](spe/dtu/ctr.md) · leaf; IN: event: NamedEvent (proposed); counter_read: CounterRead (proposed); OUT: counter_result: CounterValue (proposed)
- [ ] [DAV-SPE-DTU-DBG-0001 — Debug](spe/dtu/dbg.md) · review; IN: debug_request: DebugRequest (unresolved); drain_ack: DebugDrainAck (unresolved); debug_control: DebugControl (unresolved); OUT: debug_response: DebugResponse (unresolved)
- [ ] [DAV-SPE-DTU-LTP-0001 — Local Trace Probe](spe/dtu/ltp.md) · interface; IN: probe_source: ProbeSample (proposed); OUT: probe_event: DiagnosticEvent (proposed)
- [ ] [DAV-SPE-DTU-PMU-0001 — Performance Monitoring Unit](spe/dtu/pmu.md) · leaf; IN: named_event: NamedEvent (proposed); pmu_config: PmuConfig (proposed); OUT: pmu_sample: PmuSample (proposed)
- [ ] [DAV-SPE-DTU-TMON-0001 — Trace Monitor](spe/dtu/tmon.md) · leaf; IN: architectural_commit: ArchitecturalCommit (proposed); architectural_memory_event: ArchitecturalMemoryEvent (proposed); precise_fault: Preci…; OUT: comparison_event: ComparisonEvent (proposed)
- [ ] [DAV-SPE-DTU-XCHK-0001 — Cross-check](spe/dtu/xchk.md) · review; IN: dut_event: ComparisonEvent (unresolved); reference_event: ComparisonEvent (unresolved); OUT: divergence: DivergenceReport (unresolved)

### H2 IEX

- [ ] [DAV-SPE-IEX-AGU-0001 — Address Generation Unit](spe/iex/agu.md) · leaf; IN: execute_request: ExecutePacket (proposed); lsu_request: LoadStoreAddressRequest (proposed); OUT: terminal_result: TerminalResult (proposed)
- [ ] [DAV-SPE-IEX-ALU-0001 — Arithmetic Logic Unit](spe/iex/alu.md) · leaf; IN: request: AluPacket (declared); OUT: result: AluPacket (declared)
- [ ] [DAV-SPE-IEX-BRU-0001 — Branch Unit](spe/iex/bru.md) · leaf; IN: execute_request: ExecutePacket (proposed); terminal_result: TerminalResult (proposed); OUT: branch_resolve: BranchResolve (proposed)
- [ ] [DAV-SPE-IEX-CMDIQ-0001 — Command Issue Queue](spe/iex/cmdiq.md) · leaf; IN: command_publish: CommandPacket (proposed); cmdp_ack: CommandPipeAck (proposed); OUT: cmdp_req: CommandPacket (proposed)
- [ ] [DAV-SPE-IEX-CMDP-0001 — Command Pipe](spe/iex/cmdp.md) · leaf; IN: cmdp_req: CommandPacket (proposed); cmdp_ack: CommandPipeAck (proposed); OUT: cbrg_command: CommandPacket (proposed)
- [ ] [DAV-SPE-IEX-DIV-0001 — Divider](spe/iex/div.md) · leaf; IN: execute_request: ExecutePacket (proposed); cancel: IssueCancel (proposed); OUT: terminal_result: TerminalResult (proposed)
- [ ] [DAV-SPE-IEX-E1-0001 — Execute Stage 1](spe/iex/e1.md) · state_schema; IN: execute_request: ExecutePacket (proposed); OUT: stage_result: ExecuteStagePacket (proposed)
- [ ] [DAV-SPE-IEX-E2-0001 — Execute Stage 2](spe/iex/e2.md) · state_schema; IN: stage_input: ExecuteStagePacket (proposed); OUT: stage_result: ExecuteStagePacket (proposed)
- [ ] [DAV-SPE-IEX-E3-0001 — Execute Stage 3](spe/iex/e3.md) · state_schema; IN: stage_input: ExecuteStagePacket (proposed); OUT: stage_result: ExecuteStagePacket (proposed)
- [ ] [DAV-SPE-IEX-E4-0001 — Execute Stage 4](spe/iex/e4.md) · state_schema; IN: stage_input: ExecuteStagePacket (proposed); OUT: terminal_result: TerminalResult (proposed)
- [ ] [DAV-SPE-IEX-E5-0001 — Execute Stage 5](spe/iex/e5.md) · leaf; IN: load_result: LoadMemoryResult (proposed); terminal_result: TerminalResult (proposed); OUT: replay: LoadReplay (proposed)
- [ ] [DAV-SPE-IEX-E6-0001 — Execute Stage 6](spe/iex/e6.md) · state_schema; IN: load_result: TerminalResult (proposed); OUT: aged_result: TerminalResult (proposed)
- [ ] [DAV-SPE-IEX-FSU-0001 — Floating-point and SIMD Unit](spe/iex/fsu.md) · review; IN: execute_request: ExecutePacket (unresolved); OUT: terminal_result: TerminalResult (unresolved)
- [ ] [DAV-SPE-IEX-GPR-0001 — General-purpose Register](spe/iex/gpr.md) · leaf; IN: maintenance: GprMaintenanceRequest (declared); read_request: OperandReadRequest (declared); write_request: GprWriteRequest (declared); proj…; OUT: maintenance_ack: GprMaintenanceAck (declared); read_result: GprReadResult (declared); write_ack: GprWriteAck (declared); projection_result:…
- [ ] [DAV-SPE-IEX-I1-0001 — Issue Stage 1](spe/iex/i1.md) · review; IN: selected: IssueAttempt (declared); cancel: IssueCancel (declared); read_decision: OperandReadDecision (declared); OUT: read_request: OperandReadRequest (declared); read_decision_ack: OperandReadDecisionAck (declared); cancel_ack: IssueCancelAck (declared)
- [ ] [DAV-SPE-IEX-I2-0001 — Issue Stage 2](spe/iex/i2.md) · review; IN: operand_response: OperandReadResponse (declared); load_dependency: LoadDependencyResolution (declared); sink_decision: ExecutionSinkDecisio…; OUT: execute_request: ExecutePacket (declared); operand_ack: OperandResponseAck (declared); dependency_ack: LoadDependencyAck (declared); sink_d…
- [ ] [DAV-SPE-IEX-P0-0001 — Pick Preselection Stage 0](spe/iex/p0.md) · state_schema; IN: eligible_set: IssueCandidates (proposed); OUT: preselected: IssueCandidates (proposed)
- [ ] [DAV-SPE-IEX-P1-0001 — Pick Stage 1](spe/iex/p1.md) · leaf; IN: eligible_set: IssueCandidates (proposed); claim_ack: IssueClaimAck (proposed); OUT: selected: IssueAttempt (proposed)
- [ ] [DAV-SPE-IEX-S2-0001 — Schedule Stage 2](spe/iex/s2.md) · state_schema; IN: publication: RenamePublication (proposed); isq_ack: IssueAdmissionAck (proposed); OUT: isq_request: IssueEntry (proposed)
- [ ] [DAV-SPE-IEX-STD-0001 — Store Data Unit](spe/iex/std.md) · leaf; IN: execute_request: ExecutePacket (proposed); store_data: StoreData (proposed); OUT: terminal_result: TerminalResult (proposed)
- [ ] [DAV-SPE-IEX-W1-0001 — Writeback Stage 1](spe/iex/w1.md) · state_schema; IN: terminal_result: TerminalResult (proposed); OUT: aged_result: TerminalResult (proposed)
- [ ] [DAV-SPE-IEX-W2-0001 — Writeback Stage 2](spe/iex/w2.md) · review; IN: commit: WritebackCommit (declared); cancel: IssueCancel (declared); gpr_write_ack: GprWriteAck (declared); drain_request: W2DrainRequest (d…; OUT: gpr_write: GprWriteRequest (declared); wakeup: WakeupPublication (declared); rob_completion: RobCompletion (declared); brob_resolve: BrobRe…
- [ ] [DAV-SPE-IEX-W3-0001 — Writeback Stage 3](spe/iex/w3.md) · state_schema; IN: gpr_write: GprWriteRequest (proposed); OUT: gpr_write_ack: GprWriteAck (proposed)
- [ ] [DAV-SPE-IEX-WBA-0001 — Writeback Arbiter](spe/iex/wba.md) · leaf; IN: alu_result: TerminalResult (declared); bru_result: TerminalResult (declared); lsu_result: TerminalResult (declared); other_result: Terminal…; OUT: commit: WritebackCommit (declared); ack_result: WritebackAckResult (declared); cancel_ack: WritebackCancelAck (declared); drain_ack: WbaDra…

### H2 IFU

- [ ] [DAV-SPE-IFU-BF0-0001 — B-side Stage 0](spe/ifu/bf0.md) · leaf; IN: prediction_request: PredictionRequest (proposed); correction: PredictionCorrection (proposed); recovery: Recovery (proposed); OUT: prediction_sidecar: PredictionSidecar (proposed)
- [ ] [DAV-SPE-IFU-BF1-0001 — B-side Stage 1](spe/ifu/bf1.md) · leaf; IN: prediction_request: PredictionRequest (proposed); correction: PredictionCorrection (proposed); recovery: Recovery (proposed); OUT: prediction_sidecar: PredictionSidecar (proposed)
- [ ] [DAV-SPE-IFU-BF2-0001 — B-side Stage 2](spe/ifu/bf2.md) · leaf; IN: prediction_request: PredictionRequest (proposed); correction: PredictionCorrection (proposed); recovery: Recovery (proposed); OUT: prediction_sidecar: PredictionSidecar (proposed)
- [ ] [DAV-SPE-IFU-BF3-0001 — B-side Stage 3](spe/ifu/bf3.md) · leaf; IN: prediction_request: PredictionRequest (proposed); correction: PredictionCorrection (proposed); recovery: Recovery (proposed); OUT: prediction_sidecar: PredictionSidecar (proposed)
- [ ] [DAV-SPE-IFU-BF4-0001 — B-side Stage 4](spe/ifu/bf4.md) · leaf; IN: prediction_request: PredictionRequest (proposed); correction: PredictionCorrection (proposed); recovery: Recovery (proposed); OUT: prediction_sidecar: PredictionSidecar (proposed)
- [ ] [DAV-SPE-IFU-BIM-0001 — Bimodal Predictor](spe/ifu/bim.md) · leaf; IN: lookup: PredictionLookup (proposed); train: PredictionTraining (proposed); OUT: prediction: PredictionCandidate (proposed)
- [ ] [DAV-SPE-IFU-BPFIFO-0001 — Branch Prediction FIFO](spe/ifu/bpfifo.md) · state_schema; IN: enqueue: PredictionSidecar (proposed); OUT: dequeue: PredictionSidecar (proposed)
- [ ] [DAV-SPE-IFU-BPU-0001 — Branch Prediction Unit](spe/ifu/bpu.md) · alias; IN: prediction_request: PredictionRequest (proposed); correction: PredictionCorrection (proposed); recovery: Recovery (proposed); OUT: prediction_sidecar: PredictionSidecar (proposed)
- [ ] [DAV-SPE-IFU-BRQ-0001 — Branch Prediction Queue](spe/ifu/brq.md) · leaf; IN: prediction_event: PredictionEvent (proposed); OUT: checkpoint_result: PredictionCheckpoint (proposed)
- [ ] [DAV-SPE-IFU-CBTB-0001 — Context Branch Target Buffer](spe/ifu/cbtb.md) · leaf; IN: lookup: PredictionLookup (proposed); train: PredictionTraining (proposed); OUT: prediction: PredictionCandidate (proposed)
- [ ] [DAV-SPE-IFU-F0-0001 — I-side Fetch Stage 0](spe/ifu/f0.md) · leaf; IN: fetch_grant: FetchGrant (proposed); pc_reply: PcReply (proposed); context_reply: ContextReply (proposed); recovery: Recovery (proposed); OUT: lookup_launch: FetchLookup (proposed); prediction_launch: PredictionRequest (proposed); pc_advance: PcAdvance (proposed); launch_ack: Fetch…
- [ ] [DAV-SPE-IFU-F1-0001 — I-side Fetch Stage 1](spe/ifu/f1.md) · leaf; IN: lookup_launch: FetchLookup (proposed); OUT: itlb_request: TranslationRequest (proposed); itag_request: ITagLookup (proposed); idata_request: IDataLookup (proposed); launch_status: Loo…
- [ ] [DAV-SPE-IFU-F2-0001 — I-side Fetch Stage 2](spe/ifu/f2.md) · leaf; IN: translation_result: TranslationResult (proposed); itag_result: ITagResult (proposed); idata_result: IDataResult (proposed); recovery: Recov…; OUT: assembled_fragment: FetchFragment (proposed); fill_request: IFillRequest (proposed); replay_request: FetchReplay (proposed); fetch_fault: F…
- [ ] [DAV-SPE-IFU-F3-0001 — I-side Fetch Stage 3](spe/ifu/f3.md) · leaf; IN: assembled_fragment: FetchFragment (proposed); recovery: Recovery (proposed); OUT: instruction_slots: InstructionSlots (proposed); successor_fetch_request: FetchLookup (proposed); fetch_fault: FetchFault (proposed)
- [ ] [DAV-SPE-IFU-F4-0001 — I-side Fetch Stage 4](spe/ifu/f4.md) · leaf; IN: bank_window: WindowSnapshot (proposed); prediction_sidecar: PredictionSidecar (proposed); decode_selection: DecodeSelection (proposed); dec…; OUT: decode_candidate: DecodeCandidate (proposed); decode_window: DecodeWindow (proposed); ib_accept: DecodeAccept (proposed); selection_ack: Se…
- [ ] [DAV-SPE-IFU-FQ-0001 — Fill Queue](spe/ifu/fq.md) · leaf; IN: fill_request: IFillRequest (proposed); memory_response: MemoryResponse (proposed); waiter_cancel: FetchCancel (proposed); OUT: memory_request: MemoryRequest (proposed); fill_fragment: FetchFragment (proposed); waiter_terminal: FillTerminal (proposed)
- [ ] [DAV-SPE-IFU-GHR-0001 — Global History Register](spe/ifu/ghr.md) · state_schema; IN: prediction_event: PredictionEvent (proposed); OUT: checkpoint_result: PredictionCheckpoint (proposed)
- [ ] [DAV-SPE-IFU-GHRQ-0001 — Global History Queue](spe/ifu/ghrq.md) · state_schema; IN: prediction_event: PredictionEvent (proposed); OUT: checkpoint_result: PredictionCheckpoint (proposed)
- [ ] [DAV-SPE-IFU-IB-0001 — Instruction Buffer](spe/ifu/ib.md) · leaf; IN: instruction_slots: InstructionSlots (proposed); decode_accept: DecodeAccept (proposed); recovery: Recovery (proposed); OUT: bank_window: WindowSnapshot (proposed); window_release_ack: WindowReleaseAck (proposed)
- [ ] [DAV-SPE-IFU-IBTB-0001 — Indirect Branch Target Buffer](spe/ifu/ibtb.md) · leaf; IN: lookup: PredictionLookup (proposed); train: PredictionTraining (proposed); OUT: prediction: PredictionCandidate (proposed)
- [ ] [DAV-SPE-IFU-IDATA-0001 — Level-1 Instruction Cache Data Array](spe/ifu/idata.md) · leaf; IN: byte_lookup: IDataLookup (proposed); fill_write: IDataFillWrite (proposed); invalidate_reservation: LineReservationCancel (proposed); OUT: byte_result: IDataResult (proposed); fill_write_ack: IDataFillAck (proposed)
- [ ] [DAV-SPE-IFU-IMMQ-0001 — Instruction Memory Miss Queue](spe/ifu/immq.md) · state_schema; IN: fill_request: IFillRequest (proposed); memory_response: MemoryResponse (proposed); waiter_cancel: FetchCancel (proposed); memory_request: M…; OUT: waiter_terminal: FillTerminal (proposed)
- [ ] [DAV-SPE-IFU-ITAG-0001 — Level-1 Instruction Cache Tag Array](spe/ifu/itag.md) · leaf; IN: tag_lookup: ITagLookup (proposed); fill_install: ITagFillInstall (proposed); invalidate: ICacheInvalidate (proposed); OUT: tag_result: ITagResult (proposed); install_ack: ITagInstallAck (proposed); invalidate_ack: InvalidateAck (proposed)
- [ ] [DAV-SPE-IFU-ITLB-0001 — Level-1 Instruction Translation Lookaside Buffer](spe/ifu/itlb.md) · leaf; IN: translation_request: TranslationRequest (proposed); walker_result: WalkerResult (proposed); translation_invalidate: TranslationInvalidate (…; OUT: translation_result: TranslationResult (proposed); walker_request: WalkerRequest (proposed); invalidate_ack: InvalidateAck (proposed)
- [ ] [DAV-SPE-IFU-LBUF-0001 — Loop Buffer](spe/ifu/lbuf.md) · review; IN: loop_fill: LoopBufferFill (unresolved); loop_lookup: LoopBufferLookup (unresolved); invalidate: ICacheInvalidate (unresolved); OUT: loop_result: LoopBufferResult (unresolved)
- [ ] [DAV-SPE-IFU-LOOP-0001 — Loop Predictor](spe/ifu/loop.md) · review; IN: lookup: PredictionLookup (unresolved); train: PredictionTraining (unresolved); OUT: prediction: PredictionCandidate (unresolved)
- [ ] [DAV-SPE-IFU-MBTB-0001 — Main Branch Target Buffer](spe/ifu/mbtb.md) · leaf; IN: lookup: PredictionLookup (proposed); train: PredictionTraining (proposed); OUT: prediction: PredictionCandidate (proposed)
- [ ] [DAV-SPE-IFU-NLP-0001 — Next-line Predictor](spe/ifu/nlp.md) · leaf; IN: lookup: PredictionLookup (proposed); train: PredictionTraining (proposed); OUT: prediction: PredictionCandidate (proposed)
- [ ] [DAV-SPE-IFU-PBTB-0001 — Previous Branch Target Buffer](spe/ifu/pbtb.md) · review; IN: lookup: PredictionLookup (unresolved); train: PredictionTraining (unresolved); OUT: prediction: PredictionCandidate (unresolved)
- [ ] [DAV-SPE-IFU-RAFE-0001 — Run-ahead Fetch Engine](spe/ifu/rafe.md) · review; IN: runahead_request: RunaheadRequest (unresolved); recovery: Recovery (unresolved); runahead_fetch: FetchLookup (unresolved); OUT: runahead_status: RunaheadStatus (unresolved)
- [ ] [DAV-SPE-IFU-RAHQ-0001 — Run-ahead Queue](spe/ifu/rahq.md) · review; IN: runahead_request: RunaheadRequest (unresolved); recovery: Recovery (unresolved); runahead_fetch: FetchLookup (unresolved); OUT: runahead_status: RunaheadStatus (unresolved)
- [ ] [DAV-SPE-IFU-RAS-0001 — Return Address Stack](spe/ifu/ras.md) · leaf; IN: prediction_event: PredictionEvent (proposed); OUT: checkpoint_result: PredictionCheckpoint (proposed)
- [ ] [DAV-SPE-IFU-SC-0001 — Statistical Corrector](spe/ifu/sc.md) · review; IN: lookup: PredictionLookup (unresolved); train: PredictionTraining (unresolved); OUT: prediction: PredictionCandidate (unresolved)
- [ ] [DAV-SPE-IFU-TAGE-0001 — Tagged Geometric History Length Predictor](spe/ifu/tage.md) · review; IN: lookup: PredictionLookup (unresolved); train: PredictionTraining (unresolved); OUT: prediction: PredictionCandidate (unresolved)
- [ ] [DAV-SPE-IFU-TGTFIFO-0001 — Branch Target FIFO](spe/ifu/tgtfifo.md) · state_schema; IN: enqueue: PredictionSidecar (proposed); OUT: dequeue: PredictionSidecar (proposed)
- [ ] [DAV-SPE-IFU-UBTB-0001 — Micro Branch Target Buffer](spe/ifu/ubtb.md) · leaf; IN: lookup: PredictionLookup (proposed); train: PredictionTraining (proposed); OUT: prediction: PredictionCandidate (proposed)
- [ ] [DAV-SPE-IFU-UCACHE-0001 — Micro Instruction Cache](spe/ifu/ucache.md) · review; IN: uop_lookup: UopCacheLookup (unresolved); uop_fill: UopCacheFill (unresolved); invalidate: ICacheInvalidate (unresolved); OUT: uop_result: UopCacheResult (unresolved)
- [ ] [DAV-SPE-IFU-UTLB-0001 — Micro Instruction Translation Lookaside Buffer](spe/ifu/utlb.md) · leaf; IN: translation_request: TranslationRequest (proposed); walker_result: WalkerResult (proposed); translation_invalidate: TranslationInvalidate (…; OUT: invalidate_ack: InvalidateAck (proposed)

### H2 LSU

- [ ] [DAV-SPE-LSU-FWD-0001 — Forwarding](spe/lsu/fwd.md) · leaf; IN: forward_request: ForwardRequest (proposed); OUT: forward_result: ForwardResult (proposed)
- [ ] [DAV-SPE-LSU-L1D-0001 — Level-1 Data Cache](spe/lsu/l1d.md) · leaf; IN: load_request: DataCacheRead (proposed); store_request: DataCacheWrite (proposed); fill_install: DataFillInstall (proposed); invalidate: Dat…; OUT: fill_ack: DataFillAck (proposed)
- [ ] [DAV-SPE-LSU-LFB-0001 — Line Fill Buffer](spe/lsu/lfb.md) · leaf; IN: miss_request: DataFillRequest (proposed); memory_response: MemoryResponse (proposed); waiter_cancel: MemoryCancel (proposed); memory_reques…; OUT: waiter_terminal: DataFillTerminal (proposed)
- [ ] [DAV-SPE-LSU-LHQ-0001 — Load Hit Queue](spe/lsu/lhq.md) · leaf; IN: load_resolved: LoadResolved (proposed); ordering_query: MemoryOrderingQuery (proposed); OUT: ordering_result: MemoryOrderingResult (proposed)
- [ ] [DAV-SPE-LSU-LIQ-0001 — Load Inflight Queue](spe/lsu/liq.md) · leaf; IN: load_issue: LoadIssue (proposed); memory_result: LoadMemoryResult (proposed); recovery: Recovery (proposed); load_terminal: TerminalResult …; OUT: dependency_resolution: LoadDependencyResolution (proposed)
- [ ] [DAV-SPE-LSU-MDB-0001 — Memory Disambiguation Buffer](spe/lsu/mdb.md) · leaf; IN: conflict_observation: MemoryConflict (proposed); recovery: Recovery (proposed); delay_prediction: MemoryDelayPrediction (proposed); OUT: violation: MemoryViolation (proposed)
- [ ] [DAV-SPE-LSU-RPL-0001 — Replay](spe/lsu/rpl.md) · leaf; IN: replay_request: LoadReplay (proposed); wakeup: ReplayWakeup (proposed); recovery: Recovery (proposed); OUT: reissue: LoadIssue (proposed)
- [ ] [DAV-SPE-LSU-SCB-0001 — Store Coalescing Buffer](spe/lsu/scb.md) · leaf; IN: committed_store: CommittedStore (proposed); cache_ack: StoreCacheAck (proposed); cache_write: CacheWrite (proposed); OUT: drain_status: StoreDrainStatus (proposed)
- [ ] [DAV-SPE-LSU-STQ-0001 — Store Queue](spe/lsu/stq.md) · leaf; IN: store_address: StoreAddress (proposed); store_data: StoreData (proposed); scalar_store_commit: ScalarStoreCommit (proposed); recovery: Reco…; OUT: store_status: StoreStatus (proposed)
- [ ] [DAV-SPE-LSU-TLB-0001 — Translation Lookaside Buffer](spe/lsu/tlb.md) · leaf; IN: translation_request: TranslationRequest (proposed); walker_result: WalkerResult (proposed); invalidate: TranslationInvalidate (proposed); t…; OUT: walker_request: WalkerRequest (proposed)

### H2 OOO

- [ ] [DAV-SPE-OOO-CMAP-0001 — Committed Map](spe/ooo/cmap.md) · leaf; IN: architectural_commit: MapRecord (declared); lookup_request: MapLookupRequest (declared); OUT: commit_ack: MapRecord (declared); lookup_response: MapLookupResponse (declared)
- [ ] [DAV-SPE-OOO-CMT-0001 — Commit](spe/ooo/cmt.md) · leaf; IN: microcommit: RobEvent (declared); mpq_ack: MpqHandoffAck (declared); brob_ack: BrobHandoffAck (declared); OUT: mpq_request: RobEvent (declared); brob_request: RobEvent (declared); rob_handoff: RobHandoff (declared); diagnostic: HandoffDiagnostic (dec…
- [ ] [DAV-SPE-OOO-D1-0001 — Decode Stage 1](spe/ooo/d1.md) · leaf; IN: decode_window: DecodeWindow (proposed); decode_accept: DecodeAccept (proposed); decoded_group: DecodedGroup (proposed); OUT: decode_fault: DecodedFault (proposed)
- [ ] [DAV-SPE-OOO-D2-0001 — Decode Stage 2](spe/ooo/d2.md) · leaf; IN: decoded_group: DecodedGroup (proposed); map_reply: MapLookupResponse (proposed); resource_reply: ResourceReply (proposed); OUT: d2_plan: D2Plan (proposed)
- [ ] [DAV-SPE-OOO-D3-0001 — Decode Stage 3](spe/ooo/d3.md) · leaf; IN: d2_plan: D2Plan (proposed); owner_ack: ReservationAck (proposed); d3_reservation: D3Reservation (proposed); OUT: compensation: ReservationCancel (proposed)
- [ ] [DAV-SPE-OOO-DSAQ-0001 — Domain-specific Accelerator Queue](spe/ooo/dsaq.md) · review; IN: accelerator_request: AcceleratorRequest (unresolved); OUT: accelerator_result: TerminalResult (unresolved)
- [ ] [DAV-SPE-OOO-DSP-0001 — Dispatch](spe/ooo/dsp.md) · leaf; IN: reserve_req: DispatchReserveRequest (proposed); release: IssueRelease (proposed); OUT: reserve_ack: DispatchReservation (proposed)
- [ ] [DAV-SPE-OOO-EXC-0001 — Exception](spe/ooo/exc.md) · state_schema; IN: fault_update: FaultPublication (proposed); fault_query: FaultQuery (proposed); OUT: fault_result: FaultRecord (proposed)
- [ ] [DAV-SPE-OOO-FLS-0001 — Flush](spe/ooo/fls.md) · leaf; IN: fault_or_redirect: RecoveryCause (proposed); participant_ack: RecoveryAck (proposed); recovery_request: Recovery (proposed); OUT: restart: RecoveryRestart (proposed)
- [ ] [DAV-SPE-OOO-IQ-0001 — Issue Queue](spe/ooo/iq.md) · alias; IN: issue_publish: IssueEntry (proposed); OUT: issued: IssueAttempt (proposed)
- [ ] [DAV-SPE-OOO-ISQ-0001 — Issue Scheduler Queue](spe/ooo/isq.md) · leaf; IN: request: IssueEntry (declared); wakeup: WakeupEvent (declared); recovery: RecoveryEvent (declared); OUT: issued: IssueEntry (declared)
- [ ] [DAV-SPE-OOO-MPQ-0001 — Map Queue](spe/ooo/mpq.md) · leaf; IN: rename_history: ScalarRenameHistory (declared); holder_acquire: MpqHolderAcquireRequest (declared); microcommit: RobEvent (declared); lease…; OUT: history_ack: ScalarRenameHistory (declared); holder_ack: MpqHolderAcquireAck (declared); handoff_ack: MpqHandoffAck (declared); lease_ack: …
- [ ] [DAV-SPE-OOO-PCB-0001 — Program Counter Buffer](spe/ooo/pcb.md) · leaf; IN: pc_record: PcRecord (proposed); pc_query: PcQuery (proposed); OUT: pc_result: PcRecord (proposed)
- [ ] [DAV-SPE-OOO-R0-0001 — Recovery Stage 0](spe/ooo/r0.md) · state_schema; IN: resolve: TerminalResult (proposed); OUT: retire_window: RetireWindow (proposed)
- [ ] [DAV-SPE-OOO-R1-0001 — Recovery Stage 1](spe/ooo/r1.md) · state_schema; IN: retire_window: RetireWindow (proposed); OUT: commit_decision: CommitDecision (proposed)
- [ ] [DAV-SPE-OOO-R2-0001 — Recovery Stage 2](spe/ooo/r2.md) · state_schema; IN: commit_decision: CommitDecision (proposed); handoff_req: HandoffReq (proposed); OUT: recovery_cause: RecoveryCause (proposed)
- [ ] [DAV-SPE-OOO-R3-0001 — Recovery Stage 3](spe/ooo/r3.md) · state_schema; IN: recovery_request: Recovery (proposed); OUT: participant_ack: RecoveryAck (proposed)
- [ ] [DAV-SPE-OOO-R4-0001 — Recovery Stage 4](spe/ooo/r4.md) · state_schema; IN: restart_decision: RecoveryRestart (proposed); OUT: redirect: RecoveryRedirect (proposed)
- [ ] [DAV-SPE-OOO-REN-0001 — Rename](spe/ooo/ren.md) · leaf; IN: maintenance: RenameMaintenanceRequest (declared); preview_request: RenameUopRequest (declared); prepare_request: RenamePrepareRequest (decl…; OUT: maintenance_ack: RenameMaintenanceAck (declared); preview_result: RenamePreviewResult (declared); prepare_bundle: RenamePrepareBundle (decl…
- [ ] [DAV-SPE-OOO-ROB-0001 — Reorder Buffer](spe/ooo/rob.md) · leaf; IN: flush_request: RobEvent (declared); allocate_request: RobEvent (declared); completion: RobEvent (declared); handoff_ack: RobHandoff (declar…; OUT: allocated: RobEvent (declared); committed: RobEvent (declared)
- [ ] [DAV-SPE-OOO-S1-0001 — Schedule Stage 1](spe/ooo/s1.md) · leaf; IN: plan: S1PublicationPlan (declared); ren_publication: S1RenResponse (declared); cancel_request: S1CancelRequest (declared); OUT: admitted: S1AdmissionAck (declared); advance: S1Advance (declared); publication_done: S1PublicationDone (declared)
- [ ] [DAV-SPE-OOO-S3-0001 — Issue Residency Stage 3](spe/ooo/s3.md) · state_schema; IN: resident_entry: IssueEntry (proposed); wakeup: WakeupEvent (proposed); OUT: eligible_entry: IssueEntry (proposed)
- [ ] [DAV-SPE-OOO-SMAP-0001 — Speculative Map](spe/ooo/smap.md) · state_schema; IN: rename_lookup: MapLookupRequest (proposed); rename_update: MapRecord (proposed); OUT: map_result: MapLookupResponse (proposed)
- [ ] [DAV-SPE-OOO-SRF-0001 — System Register File](spe/ooo/srf.md) · leaf; IN: system_request: SystemRegisterRequest (proposed); architectural_commit: ArchitecturalCommit (proposed); OUT: system_response: SystemRegisterResponse (proposed)

## H1 SMT

### H2 ARB

- [ ] [DAV-SMT-ARB-CSEL-0001 — Commit Selection](smt/arb/csel.md) · leaf; IN: publish_candidate: PublishCandidate[flow] (proposed); publish_cancel: PublishCancel (proposed); publish_ack: PublishAck (proposed); OUT: publish_grant: PublishGrant (proposed)
- [ ] [DAV-SMT-ARB-DSEL-0001 — Dispatch Selection](smt/arb/dsel.md) · leaf; IN: decode_candidate: DecodeCandidate[flow] (proposed); window_cancel: WindowCancel (proposed); selection_ack: SelectionAck (proposed); OUT: decode_selection: DecodeSelection (proposed)
- [ ] [DAV-SMT-ARB-FSEL-0001 — Fetch Selection](smt/arb/fsel.md) · leaf; IN: fetch_candidate: FetchCandidate[flow] (proposed); eligibility_cancel: EligibilityCancel (proposed); grant_ack: FetchGrantAck (proposed); OUT: fetch_grant: FetchGrant (proposed)
- [ ] [DAV-SMT-ARB-QOS-0001 — Quality of Service](smt/arb/qos.md) · review; IN: candidate: TBD (unresolved); ack: TBD (unresolved); cancel: EpochCancel (proposed); OUT: grant: TBD (unresolved)
- [ ] [DAV-SMT-ARB-RR-0001 — Round Robin](smt/arb/rr.md) · state_schema; IN: candidate: TBD (unresolved); ack: TBD (unresolved); cancel: EpochCancel (proposed); OUT: grant: TBD (unresolved)

### H2 PRD

- [ ] [DAV-SMT-PRD-FFR-0001 — First Fault Register](smt/prd/ffr.md) · review; IN: predicate_event: TBD (unresolved); OUT: predicate_result: TBD (unresolved)
- [ ] [DAV-SMT-PRD-MASK-0001 — Predicate Mask](smt/prd/mask.md) · interface; IN: predicate_event: TBD (unresolved); OUT: predicate_result: TBD (unresolved)
- [ ] [DAV-SMT-PRD-PMAP-0001 — Predicate Map](smt/prd/pmap.md) · state_schema; IN: predicate_event: TBD (unresolved); OUT: predicate_result: TBD (unresolved)
- [ ] [DAV-SMT-PRD-PREG-0001 — Predicate Register](smt/prd/preg.md) · leaf; IN: predicate_bind: PredicateBind (proposed); predicate_read: PredicateRead (proposed); predicate_writeback: PredicateWriteback (proposed); pre…; OUT: predicate_bind_ack: PredicateBindAck (proposed); predicate_read_result: PredicateReadResult (proposed); predicate_writeback_ack: PredicateW…
- [ ] [DAV-SMT-PRD-RECON-0001 — Reconvergence](smt/prd/recon.md) · review; IN: predicate_event: TBD (unresolved); OUT: predicate_result: TBD (unresolved)

### H2 RSC

- [ ] [DAV-SMT-RSC-PART-0001 — Partition](smt/rsc/part.md) · leaf; IN: resource_request: ResourceRequest (proposed); resource_release: ResourceRelease (proposed); recovery_release: RecoveryRelease (proposed); OUT: resource_grant: ResourceGrant (proposed); release_ack: ReleaseAck (proposed); pressure_event: PressureEvent (proposed)
- [ ] [DAV-SMT-RSC-QUOTA-0001 — Quota](smt/rsc/quota.md) · state_schema; IN: resource_event: TBD (unresolved); OUT: resource_status: TBD (unresolved)
- [ ] [DAV-SMT-RSC-SHARE-0001 — Sharing](smt/rsc/share.md) · state_schema; IN: resource_event: TBD (unresolved); OUT: resource_status: TBD (unresolved)
- [ ] [DAV-SMT-RSC-THRSH-0001 — Threshold](smt/rsc/thrsh.md) · state_schema; IN: resource_event: TBD (unresolved); OUT: resource_status: TBD (unresolved)

### H2 THR

- [ ] [DAV-SMT-THR-CTX-0001 — Context](smt/thr/ctx.md) · leaf; IN: run_command: RunCommand (proposed); context_query: ContextQuery (proposed); flow_fence: FlowFence (proposed); epoch_install: EpochInstall (…; OUT: run_ack: RunAck (proposed); context_reply: ContextReply (proposed); context_control_ack: ContextControlAck (proposed)
- [ ] [DAV-SMT-THR-MODE-0001 — Thread Mode](smt/thr/mode.md) · state_schema; IN: request: TBD (unresolved); OUT: response: TBD (unresolved)
- [ ] [DAV-SMT-THR-SWI-0001 — Thread Switch](smt/thr/swi.md) · review; IN: request: TBD (unresolved); OUT: response: TBD (unresolved)
- [ ] [DAV-SMT-THR-TID-0001 — Thread Identifier](smt/thr/tid.md) · interface; IN: request: TBD (unresolved); OUT: response: TBD (unresolved)
- [ ] [DAV-SMT-THR-TPC-0001 — Thread Program Counter](smt/thr/tpc.md) · leaf; IN: pc_read: PcRequest (proposed); sequential_advance: PcAdvance (proposed); redirect: PcRedirect (proposed); start: PcStart (proposed); OUT: pc_reply: PcReply (proposed); pc_update_ack: PcUpdateAck (proposed)
- [ ] [DAV-SMT-THR-TSB-0001 — Thread Status Block](smt/thr/tsb.md) · state_schema; IN: request: TBD (unresolved); OUT: response: TBD (unresolved)

### H2 XTD

- [ ] [DAV-SMT-XTD-CEXT-0001 — Consumer Extension](smt/xtd/cext.md) · review; IN: extension_event: TBD (unresolved); OUT: extension_result: TBD (unresolved)
- [ ] [DAV-SMT-XTD-LIFE-0001 — Lifetime](smt/xtd/life.md) · review; IN: extension_event: TBD (unresolved); OUT: extension_result: TBD (unresolved)
- [ ] [DAV-SMT-XTD-PEXT-0001 — Producer Extension](smt/xtd/pext.md) · review; IN: extension_event: TBD (unresolved); OUT: extension_result: TBD (unresolved)
- [ ] [DAV-SMT-XTD-VID-0001 — Vector Identifier](smt/xtd/vid.md) · interface; IN: extension_event: TBD (unresolved); OUT: extension_result: TBD (unresolved)
- [ ] [DAV-SMT-XTD-VTB-0001 — Vector Thread Buffer](smt/xtd/vtb.md) · review; IN: extension_event: TBD (unresolved); OUT: extension_result: TBD (unresolved)
- [ ] [DAV-SMT-XTD-WAKE-0001 — Wakeup](smt/xtd/wake.md) · review; IN: extension_event: TBD (unresolved); OUT: extension_result: TBD (unresolved)

## H1 TMU

### H2 BGF

- [ ] [DAV-TMU-BGF-ARB-0001 — Arbiter](tmu/bgf/arb.md) · leaf; IN: bank_grant_requests[]: BankGrantReq (proposed); grant_cancel: GrantCancelReq (proposed); OUT: client_grants[]: BankGrant (proposed); grant_to_xbar: BankGrant (proposed); grant_cancel_ack: GrantCancelAck (proposed)
- [ ] [DAV-TMU-BGF-MAP-0001 — Mapping](tmu/bgf/map.md) · leaf; IN: cell_map_req: CellMapReq (proposed); OUT: cell_map_resp: CellMapResp (proposed)
- [ ] [DAV-TMU-BGF-RQ-0001 — Read Queue](tmu/bgf/rq.md) · state_schema; IN: read_enqueue: CellReadReq (proposed); OUT: read_head: CellReadReq (proposed)
- [ ] [DAV-TMU-BGF-WQ-0001 — Write Queue](tmu/bgf/wq.md) · state_schema; IN: write_enqueue: CellWriteReq (proposed); OUT: write_head: CellWriteReq (proposed)
- [ ] [DAV-TMU-BGF-XBAR-0001 — Crossbar](tmu/bgf/xbar.md) · leaf; IN: ingress_0: BGFPacket (declared); ingress_1: BGFPacket (declared); ingress_2: BGFPacket (declared); ingress_3: BGFPacket (declared); OUT: delivered: BGFPacket (declared)

### H2 TRF

- [ ] [DAV-TMU-TRF-ALC-0001 — Allocation](tmu/trf/alc.md) · interface; IN: capacity_query: TrfCapacityQuery (proposed); OUT: capacity_response: TrfCapacityResponse (proposed)
- [ ] [DAV-TMU-TRF-BANK-0001 — Register Bank](tmu/trf/bank.md) · leaf; IN: cell_read_req: CellReadReq (proposed); cell_write_req: CellWriteReq (proposed); bank_invalidate_req: BankInvalidateReq (proposed); OUT: cell_read_resp: CellReadResp (proposed); cell_write_ack: CellWriteAck (proposed); bank_invalidate_ack: BankInvalidateAck (proposed)
- [ ] [DAV-TMU-TRF-CELL-0001 — Tile Cell](tmu/trf/cell.md) · state_schema; IN: Containing-owner interface; no independent port; OUT: Containing-owner interface; no independent port
- [ ] [DAV-TMU-TRF-RDY-0001 — Readiness](tmu/trf/rdy.md) · state_schema; IN: status_snapshot: StatusQueryResp (proposed); bank_completion: CellWriteAck|CellReadResp (proposed); OUT: readiness: TileReadiness (proposed)
- [ ] [DAV-TMU-TRF-REF-0001 — Reference Count](tmu/trf/ref.md) · leaf; IN: lease_acquire_req: LeaseAcquireReq (proposed); lease_transfer_req: LeaseTransferReq (proposed); tile_lease_release_req: TileLeaseReleaseReq…; OUT: lease_acquire_ack: LeaseAcquireAck (proposed); lease_transfer_ack: LeaseTransferAck (proposed); tile_lease_release_ack: TileLeaseReleaseAck…

### H2 TRN

- [ ] [DAV-TMU-TRN-CHK-0001 — Checkpoint](tmu/trn/chk.md) · review; IN: checkpoint_req: TileCheckpointReq (proposed); unwind_req: TileUnwindReq (proposed); OUT: checkpoint_ack: TileCheckpointAck (proposed); unwind_ack: TileUnwindAck (proposed)
- [ ] [DAV-TMU-TRN-FRE-0001 — Free-list](tmu/trn/fre.md) · leaf; IN: alloc_req: AllocReq (proposed); commit_reservation_req: CommitReservationReq (proposed); cancel_reservation_req: CancelReservationReq (prop…; OUT: alloc_ack: AllocAck (proposed); commit_reservation_ack: CommitReservationAck (proposed); cancel_reservation_ack: CancelReservationAck (prop…
- [ ] [DAV-TMU-TRN-LRM-0001 — Local Rename Map](tmu/trn/lrm.md) · alias; IN: local_map_request: RenameLookupReq|MapSwapReq|MapPublishReq|MapRollbackReq (proposed); OUT: local_map_response: RenameLookupResp|MapSwapAck|MapPublishAck|MapRollbackAck (proposed)
- [ ] [DAV-TMU-TRN-LTR-0001 — Logical Tile Register](tmu/trn/ltr.md) · leaf; IN: tile_rename_req: TileRenameReq (proposed); tile_publish_req: TilePublishReq (proposed); recovery_req: TileRecoveryReq (proposed); OUT: tile_rename_ack: TileRenameAck (proposed); tile_publish_ack: TilePublishAck (proposed); recovery_ack: TileRecoveryAck (proposed)
- [ ] [DAV-TMU-TRN-RAT-0001 — Register Alias Table](tmu/trn/rat.md) · leaf; IN: rename_lookup_req: RenameLookupReq (proposed); map_swap_req: MapSwapReq (proposed); map_publish_req: MapPublishReq (proposed); map_rollback…; OUT: rename_lookup_resp: RenameLookupResp (proposed); map_swap_ack: MapSwapAck (proposed); map_publish_ack: MapPublishAck (proposed); map_rollba…
- [ ] [DAV-TMU-TRN-STS-0001 — Tile Status](tmu/trn/sts.md) · leaf; IN: status_reserve_req: StatusReserveReq (proposed); status_write_req: StatusWriteReq (proposed); status_publish_req: StatusPublishReq (propose…; OUT: status_reserve_ack: StatusReserveAck (proposed); status_write_ack: StatusWriteAck (proposed); status_publish_ack: StatusPublishAck (propose…

### H2 TUL

- [ ] [DAV-TMU-TUL-FLS-0001 — Flush](tmu/tul/fls.md) · alias; IN: Containing-owner interface; no independent port; OUT: Containing-owner interface; no independent port
- [ ] [DAV-TMU-TUL-LBA-0001 — Late Binding Allocation](tmu/tul/lba.md) · alias; IN: Containing-owner interface; no independent port; OUT: Containing-owner interface; no independent port
- [ ] [DAV-TMU-TUL-RCM-0001 — Recommit](tmu/tul/rcm.md) · alias; IN: Containing-owner interface; no independent port; OUT: Containing-owner interface; no independent port
- [ ] [DAV-TMU-TUL-REN-0001 — Rename](tmu/tul/ren.md) · alias; IN: Containing-owner interface; no independent port; OUT: Containing-owner interface; no independent port
- [ ] [DAV-TMU-TUL-RET-0001 — Retirement](tmu/tul/ret.md) · alias; IN: Containing-owner interface; no independent port; OUT: Containing-owner interface; no independent port

## H1 VEC

### H2 SFU

- [ ] [DAV-VEC-SFU-DIV-0001 — Division](vec/sfu/div.md) · leaf; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: result_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-SFU-EXP-0001 — Exponential](vec/sfu/exp.md) · leaf; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: result_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-SFU-LOG-0001 — Logarithm](vec/sfu/log.md) · leaf; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: result_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-SFU-RECIP-0001 — Reciprocal](vec/sfu/recip.md) · leaf; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: result_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-SFU-RSQRT-0001 — Reciprocal Square Root](vec/sfu/rsqrt.md) · leaf; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: result_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-SFU-SQRT-0001 — Square Root](vec/sfu/sqrt.md) · leaf; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: result_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)

### H2 SHU

- [ ] [DAV-VEC-SHU-GTH-0001 — Gather](vec/shu/gth.md) · alias; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: rearranged_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-SHU-PERM-0001 — Permutation](vec/shu/perm.md) · leaf; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: rearranged_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-SHU-SCT-0001 — Scatter](vec/shu/sct.md) · alias; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: rearranged_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-SHU-SHF-0001 — Shuffle](vec/shu/shf.md) · alias; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: rearranged_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-SHU-SORT-0001 — Sort](vec/shu/sort.md) · review; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: rearranged_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-SHU-TRANS-0001 — Transpose](vec/shu/trans.md) · alias; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: rearranged_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)

### H2 TLOP

- [ ] [DAV-VEC-TLOP-DEC-0001 — Decode](vec/tlop/dec.md) · leaf; IN: engine_command_in: EngineCommand (proposed); child_resolve_in: EngineResolve (proposed); cancel_in: EpochCancel (proposed); OUT: child_uop_out: EngineCommand (proposed); engine_resolve_out: EngineResolve (proposed)
- [ ] [DAV-VEC-TLOP-FSM-0001 — Finite State Machine](vec/tlop/fsm.md) · leaf; IN: engine_command_in: EngineCommand (proposed); child_resolve_in: EngineResolve (proposed); cancel_in: EpochCancel (proposed); OUT: child_uop_out: EngineCommand (proposed); engine_resolve_out: EngineResolve (proposed)
- [ ] [DAV-VEC-TLOP-INJ-0001 — Injection](vec/tlop/inj.md) · interface; IN: engine_command_in: EngineCommand (proposed); child_resolve_in: EngineResolve (proposed); cancel_in: EpochCancel (proposed); OUT: child_uop_out: EngineCommand (proposed); engine_resolve_out: EngineResolve (proposed)
- [ ] [DAV-VEC-TLOP-UROM-0001 — Microcode ROM](vec/tlop/urom.md) · state_schema; IN: engine_command_in: EngineCommand (proposed); child_resolve_in: EngineResolve (proposed); cancel_in: EpochCancel (proposed); OUT: child_uop_out: EngineCommand (proposed); engine_resolve_out: EngineResolve (proposed)

### H2 VEX

- [ ] [DAV-VEC-VEX-ACC-0001 — Accumulation](vec/vex/acc.md) · state_schema; IN: operand_or_control: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: result_or_status: TBD (unresolved)
- [ ] [DAV-VEC-VEX-ALU-0001 — Arithmetic Logic Unit](vec/vex/alu.md) · leaf; IN: operand_window_in: OperandWindow (proposed); cancel_in: EpochCancel (proposed); OUT: result_fragment_out: ResultFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-VEX-DBUF-0001 — Destination Buffer](vec/vex/dbuf.md) · leaf; IN: fragment_in: ResultFragment (proposed); tile_write_ack_in: CellWriteAck (proposed); cancel_in: EpochCancel (proposed); OUT: tile_write_req_out: CellWriteReq (proposed); vec_resolve_out: EngineResolve (proposed)
- [ ] [DAV-VEC-VEX-EXP-0001 — Expansion](vec/vex/exp.md) · alias; IN: operand_or_control: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: result_or_status: TBD (unresolved)
- [ ] [DAV-VEC-VEX-RED-0001 — Reduction](vec/vex/red.md) · leaf; IN: fragment_in: ResultFragment (proposed); cancel_in: EpochCancel (proposed); OUT: reduced_result_out: ReducedResult (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-VEC-VEX-SBUF-0001 — Source Buffer](vec/vex/sbuf.md) · leaf; IN: command_in: EngineCommand (proposed); cell_read_resp_in: CellReadResp (proposed); lease_release_ack_in: TileLeaseReleaseAck (proposed); can…; OUT: cell_read_req_out: CellReadReq (proposed); operand_window_out: OperandWindow (proposed); lease_release_req_out: TileLeaseReleaseReq (propos…
- [ ] [DAV-VEC-VEX-WB-0001 — Writeback](vec/vex/wb.md) · alias; IN: operand_or_control: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: result_or_status: TBD (unresolved)

## H1 CUBE

### H2 ACC

- [ ] [DAV-CUBE-ACC-CVT-0001 — Conversion](cube/acc/cvt.md) · leaf; IN: partial_in: MacPartial (proposed); cancel_in: EpochCancel (proposed); OUT: converted_fragment_out: ConvertedFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-CUBE-ACC-IACC-0001 — Internal Accumulator](cube/acc/iacc.md) · leaf; IN: acc_alloc_in: AccAllocate (proposed); ordered_partial_in: MacPartial (proposed); acc_read_req_in: AccReadReq (proposed); cancel_in: EpochCa…; OUT: acc_complete_out: AccComplete (proposed); acc_read_resp_out: AccReadResp (proposed)
- [ ] [DAV-CUBE-ACC-RND-0001 — Rounding](cube/acc/rnd.md) · review; IN: partial_in: MacPartial (proposed); cancel_in: EpochCancel (proposed); OUT: converted_fragment_out: ConvertedFragment (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-CUBE-ACC-SAT-0001 — Saturation](cube/acc/sat.md) · review; IN: partial_in: MacPartial (proposed); cancel_in: EpochCancel (proposed); OUT: converted_fragment_out: ConvertedFragment (proposed); fault_out: EngineFault (proposed)

### H2 DFL

- [ ] [DAV-CUBE-DFL-ABUF-0001 — A Operand Buffer](cube/dfl/abuf.md) · state_schema; IN: command_or_fragment_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: operand_or_status_out: TBD (unresolved)
- [ ] [DAV-CUBE-DFL-BBUF-0001 — B Operand Buffer](cube/dfl/bbuf.md) · state_schema; IN: command_or_fragment_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: operand_or_status_out: TBD (unresolved)
- [ ] [DAV-CUBE-DFL-CELL-0001 — Compute Cell](cube/dfl/cell.md) · state_schema; IN: command_or_fragment_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: operand_or_status_out: TBD (unresolved)
- [ ] [DAV-CUBE-DFL-CSCL-0001 — Cube Scale](cube/dfl/cscl.md) · leaf; IN: command_or_fragment_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: operand_or_status_out: TBD (unresolved)
- [ ] [DAV-CUBE-DFL-LAY-0001 — Layout](cube/dfl/lay.md) · leaf; IN: command_or_fragment_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: operand_or_status_out: TBD (unresolved)
- [ ] [DAV-CUBE-DFL-RDB-0001 — Read Buffer](cube/dfl/rdb.md) · leaf; IN: command_in: EngineCommand (proposed); tile_read_resp_in: CellReadResp (proposed); lease_release_ack_in: TileLeaseReleaseAck (proposed); can…; OUT: tile_read_req_out: CellReadReq (proposed); operand_tile_out: OperandTile (proposed); fault_out: EngineFault (proposed); lease_release_req_o…
- [ ] [DAV-CUBE-DFL-XPOS-0001 — Cross-positioning](cube/dfl/xpos.md) · alias; IN: command_or_fragment_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: operand_or_status_out: TBD (unresolved)

### H2 MMA

- [ ] [DAV-CUBE-MMA-KACC-0001 — K-dimension Accumulator](cube/mma/kacc.md) · leaf; IN: operand_or_partial_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: partial_or_status_out: TBD (unresolved)
- [ ] [DAV-CUBE-MMA-MAC-0001 — Multiply-Accumulate](cube/mma/mac.md) · leaf; IN: positioned_a_in: PositionedOperand (proposed); positioned_b_in: PositionedOperand (proposed); cancel_in: EpochCancel (proposed); OUT: mac_partial_out: MacPartial (proposed); fault_out: EngineFault (proposed)
- [ ] [DAV-CUBE-MMA-PIPE-0001 — Pipeline](cube/mma/pipe.md) · assembly; IN: operand_or_partial_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: partial_or_status_out: TBD (unresolved)
- [ ] [DAV-CUBE-MMA-SYST-0001 — Systolic Array](cube/mma/syst.md) · alias; IN: operand_or_partial_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: partial_or_status_out: TBD (unresolved)

### H2 WBK

- [ ] [DAV-CUBE-WBK-FMT-0001 — Format](cube/wbk/fmt.md) · leaf; IN: fragment_or_ack_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: write_or_resolve_out: TBD (unresolved)
- [ ] [DAV-CUBE-WBK-WBF-0001 — Writeback Buffer](cube/wbk/wbf.md) · leaf; IN: converted_fragment_in: ConvertedFragment (proposed); tile_write_ack_in: CellWriteAck (proposed); cancel_in: EpochCancel (proposed); OUT: tile_write_req_out: CellWriteReq (proposed); cube_resolve_out: EngineResolve (proposed)
- [ ] [DAV-CUBE-WBK-WQ-0001 — Write Queue](cube/wbk/wq.md) · state_schema; IN: fragment_or_ack_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: write_or_resolve_out: TBD (unresolved)

## H1 MEM

### H2 COH

- [ ] [DAV-MEM-COH-FLU-0001 — Flush](mem/coh/flu.md) · review; IN: flush_req: MaintenanceFlushReq (unresolved); owner_ack[]: MaintenanceOwnerAck (unresolved); OUT: flush_work[]: MaintenanceWork (unresolved); flush_ack: MaintenanceAck (unresolved)
- [ ] [DAV-MEM-COH-INV-0001 — Invalidate](mem/coh/inv.md) · review; IN: invalidate_req: MaintenanceInvalidateReq (unresolved); owner_ack[]: MaintenanceOwnerAck (unresolved); OUT: invalidate_work[]: MaintenanceWork (unresolved); invalidate_ack: MaintenanceAck (unresolved)
- [ ] [DAV-MEM-COH-SNP-0001 — Snoop](mem/coh/snp.md) · review; IN: snoop_req: SnoopReq (unresolved); snoop_data_resp: SnoopDataResp (unresolved); OUT: cache_probe: CacheProbeReq (unresolved); snoop_resp: SnoopResp (unresolved)
- [ ] [DAV-MEM-COH-WB-0001 — Writeback](mem/coh/wb.md) · review; IN: writeback_req: WritebackReq (unresolved); writeback_terminal: TxnTerminalResp (unresolved); OUT: writeback_txn: TxnFragmentReq (unresolved); writeback_ack: WritebackAck (unresolved)

### H2 L2C

- [ ] [DAV-MEM-L2C-ARB-0001 — Arbiter](mem/l2c/arb.md) · leaf; IN: l2_requests[]: L2ArbReq (proposed); OUT: l2_grants[]: L2ArbGrant (proposed)
- [ ] [DAV-MEM-L2C-BHU-0001 — Bank Hazard Unit](mem/l2c/bhu.md) · leaf; IN: bhu_req: BhuReq (proposed); downstream_resp: DownstreamMemResp (proposed); bhu_cancel_req: BhuCancelReq (proposed); OUT: downstream_req: DownstreamMemReq (proposed); bhu_resp: BhuResp (proposed); bhu_cancel_ack: BhuCancelAck (proposed)
- [ ] [DAV-MEM-L2C-DATA-0001 — Data Array](mem/l2c/data.md) · leaf; IN: data_read_req: DataReadReq (proposed); data_write_req: DataWriteReq (proposed); data_lease_req: DataLeaseReq (proposed); data_invalidate_re…; OUT: data_read_resp: DataReadResp (proposed); data_write_ack: DataWriteAck (proposed); data_lease_ack: DataLeaseAck (proposed); data_invalidate_…
- [ ] [DAV-MEM-L2C-MROB-0001 — Memory Reorder Buffer](mem/l2c/mrob.md) · leaf; IN: mrob_alloc_req: MrobAllocReq (proposed); data_ready_req: DataReadyReq (proposed); return_map_req: ReturnMapReq (proposed); mrob_to_rwdb_ack…; OUT: mrob_alloc_ack: MrobAllocAck (proposed); data_ready_ack: DataReadyAck (proposed); return_map_ack: ReturnMapAck (proposed); mrob_to_rwdb_req…
- [ ] [DAV-MEM-L2C-RET-0001 — Return Path](mem/l2c/ret.md) · leaf; IN: tag_hit_req: TagHitReturnReq (proposed); data_read_resp: DataReadResp (proposed); bypass_ack: BypassAck (proposed); OUT: data_read_req: DataReadReq (proposed); hit_to_rwdb_req: HitToRwdbReq (proposed); terminal_return: TerminalReturn (proposed); hit_beat_compl…
- [ ] [DAV-MEM-L2C-RWDB-0001 — Read Write Data Buffer](mem/l2c/rwdb.md) · leaf; IN: mrob_to_rwdb_req: MrobToRwdbReq (proposed); hit_to_rwdb_req: HitToRwdbReq (proposed); tile_read_resp: TileReadResp (proposed); tile_write_a…; OUT: mrob_to_rwdb_ack: MrobToRwdbAck (proposed); hit_to_rwdb_ack: HitToRwdbAck (proposed); tile_read_req: TileReadReq (proposed); tile_write_req…
- [ ] [DAV-MEM-L2C-TAG-0001 — Tag Array](mem/l2c/tag.md) · leaf; IN: tag_lookup_req: TagLookupReq (proposed); tag_install_req: TagInstallReq (proposed); tag_invalidate_req: TagInvalidateReq (proposed); OUT: tag_lookup_resp: TagLookupResp (proposed); tag_install_ack: TagInstallAck (proposed); tag_invalidate_ack: TagInvalidateAck (proposed)

### H2 MIF

- [ ] [DAV-MEM-MIF-ATOM-0001 — Atomic Operation](mem/mif/atom.md) · review; IN: atomic_req: AtomicReq (unresolved); effect_permit_ack: EffectPermitAck (unresolved); atomic_mem_resp: AtomicMemResp (unresolved); OUT: effect_permit_req: EffectPermitReq (unresolved); atomic_mem_req: AtomicMemReq (unresolved); atomic_resp: AtomicResp (unresolved)
- [ ] [DAV-MEM-MIF-GM-0001 — Global Memory](mem/mif/gm.md) · interface; IN: global_mem_req: GlobalMemReq (proposed); OUT: global_mem_resp: GlobalMemResp (proposed)
- [ ] [DAV-MEM-MIF-ORD-0001 — Ordering](mem/mif/ord.md) · leaf; IN: order_reserve_req: OrderReserveReq (proposed); effect_permit_req: EffectPermitReq (proposed); effect_complete_req: EffectCompleteReq (propo…; OUT: order_reserve_ack: OrderReserveAck (proposed); effect_permit_ack: EffectPermitAck (proposed); effect_complete_ack: EffectCompleteAck (propo…
- [ ] [DAV-MEM-MIF-REQ-0001 — Request](mem/mif/req.md) · leaf; IN: request_ingress[]: ScalarReq|TileReq|IFillReq|WritebackReq|MaintenanceReq (proposed); translate_resp: TranslateResp (proposed); txn_fragmen…; OUT: translate_req: TranslateReq (proposed); txn_fragment_req: TxnFragmentReq (proposed); req_accept_ack: ReqAcceptAck (proposed); req_fault: Re…
- [ ] [DAV-MEM-MIF-RSP-0001 — Response](mem/mif/rsp.md) · leaf; IN: terminal_resp: TxnTerminalResp (proposed); OUT: scalar_result: ScalarMemResult (proposed); cache_fill: CachelineFill (proposed); tile_fragment: TileMemoryFragment (proposed); store_or_wri…
- [ ] [DAV-MEM-MIF-TXN-0001 — Transaction](mem/mif/txn.md) · leaf; IN: txn_fragment_req: TxnFragmentReq (proposed); l2_resp: L2Resp (proposed); bhu_resp: BhuResp (proposed); txn_cancel_req: TxnCancelReq (propos…; OUT: txn_fragment_ack: TxnFragmentAck (proposed); l2_req: L2Req (proposed); bhu_req: BhuReq (proposed); txn_terminal_resp: TxnTerminalResp (prop…

### H2 NOC

- [ ] [DAV-MEM-NOC-ARB-0001 — Arbiter](mem/noc/arb.md) · leaf; IN: route_requests[]: NoCGrantReq (proposed); OUT: route_grants[]: NoCGrant (proposed)
- [ ] [DAV-MEM-NOC-BUF-0001 — Buffer](mem/noc/buf.md) · state_schema; IN: buffer_enqueue: NoCFlit|NoCPacket (unresolved); OUT: buffer_head: NoCFlit|NoCPacket (unresolved)
- [ ] [DAV-MEM-NOC-RTR-0001 — Router](mem/noc/rtr.md) · review; IN: router_ingress[]: NoCFlit|NoCPacket (unresolved); credit_or_accept[]: NoCFlowControl (unresolved); OUT: router_egress[]: NoCFlit|NoCPacket (unresolved); transport_status: NoCTransportStatus (unresolved)
- [ ] [DAV-MEM-NOC-VC-0001 — Virtual Channel](mem/noc/vc.md) · review; IN: vc_alloc_req: VcAllocReq (unresolved); credit_return: VcCredit (unresolved); vc_release_req: VcReleaseReq (unresolved); OUT: vc_alloc_ack: VcAllocAck (unresolved); vc_release_ack: VcReleaseAck (unresolved)
- [ ] [DAV-MEM-NOC-XBAR-0001 — Crossbar](mem/noc/xbar.md) · leaf; IN: ingress_0: NoCPacket (declared); ingress_1: NoCPacket (declared); ingress_2: NoCPacket (declared); ingress_3: NoCPacket (declared); OUT: delivered: NoCPacket (declared)

## H1 GPE

### H2 GCU

- [ ] [DAV-GPE-GCU-CGI-0001 — Current Group Instruction](gpe/gcu/cgi.md) · alias; IN: group_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_event_out: TBD (unresolved)
- [ ] [DAV-GPE-GCU-CNT-0001 — Counter](gpe/gcu/cnt.md) · state_schema; IN: group_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_event_out: TBD (unresolved)
- [ ] [DAV-GPE-GCU-GGC-0001 — Global Group Counter](gpe/gcu/ggc.md) · leaf; IN: claim_in: GroupClaim (proposed); local_ready_in: GroupLocalReady (proposed); group_release_ack_in: GroupReleaseAck (proposed); cancel_in: E…; OUT: wake_out: GroupWake[pe] (proposed); group_outcome_out: GroupOutcome (proposed)
- [ ] [DAV-GPE-GCU-LCMT-0001 — Local Commit](gpe/gcu/lcmt.md) · state_schema; IN: group_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_event_out: TBD (unresolved)
- [ ] [DAV-GPE-GCU-PGI-0001 — Previous Group Instruction](gpe/gcu/pgi.md) · alias; IN: group_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_event_out: TBD (unresolved)
- [ ] [DAV-GPE-GCU-WAKE-0001 — Wakeup](gpe/gcu/wake.md) · interface; IN: group_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_event_out: TBD (unresolved)

### H2 GMM

- [ ] [DAV-GPE-GMM-GLD-0001 — Group Load](gpe/gmm/gld.md) · leaf; IN: group_matrix_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_matrix_event_out: TBD (unresolved)
- [ ] [DAV-GPE-GMM-GMV-0001 — Group Matrix Move](gpe/gmm/gmv.md) · alias; IN: group_matrix_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_matrix_event_out: TBD (unresolved)
- [ ] [DAV-GPE-GMM-ISS-0001 — Issue](gpe/gmm/iss.md) · leaf; IN: group_wake_in: GroupWake[pe] (proposed); local_operand_ready_in: OperandReady[pe] (proposed); shared_operand_ready_in: OperandReady[pe] (pr…; OUT: cube_issue_out: EngineCommand[pe] (proposed); group_issue_done_out: GroupIssueDone (proposed)
- [ ] [DAV-GPE-GMM-LCMT-0001 — Local Commit](gpe/gmm/lcmt.md) · interface; IN: group_matrix_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_matrix_event_out: TBD (unresolved)
- [ ] [DAV-GPE-GMM-STGB-0001 — Stage Buffer B](gpe/gmm/stgb.md) · leaf; IN: shared_write_in: CellBeat (proposed); cube_read_req_in: SharedReadReq[pe] (proposed); lease_release_ack_in: TileLeaseReleaseAck (proposed);…; OUT: cube_read_rsp_out: SharedReadResp[pe] (proposed); lease_release_req_out: TileLeaseReleaseReq (proposed)

### H2 GMV

- [ ] [DAV-GPE-GMV-DRDY-0001 — Data Ready](gpe/gmv/drdy.md) · review; IN: group_move_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_move_event_out: TBD (unresolved)
- [ ] [DAV-GPE-GMV-GBUF-0001 — Group Buffer](gpe/gmv/gbuf.md) · leaf; IN: reserve_in: GroupMoveReserve (proposed); producer_data_in: CellBeat (proposed); fabric_rsp_in: GPEPacket (proposed); lease_release_ack_in: …; OUT: fabric_req_out: GPEPacket (proposed); consumer_data_out: CellBeat[pe] (proposed); gpe_resolve_out: EngineResolve (proposed); lease_release_…
- [ ] [DAV-GPE-GMV-REQ-0001 — Request](gpe/gmv/req.md) · interface; IN: group_move_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_move_event_out: TBD (unresolved)
- [ ] [DAV-GPE-GMV-RSP-0001 — Response](gpe/gmv/rsp.md) · interface; IN: group_move_event_in: TBD (unresolved); cancel_in: EpochCancel (proposed); OUT: group_move_event_out: TBD (unresolved)

### H2 IPF

- [ ] [DAV-GPE-IPF-ARB-0001 — Arbiter](gpe/ipf/arb.md) · leaf; IN: packet_in: GPEPacket (proposed); OUT: packet_out: GPEPacket (proposed)
- [ ] [DAV-GPE-IPF-REQ-0001 — Request](gpe/ipf/req.md) · interface; IN: packet_in: GPEPacket (proposed); OUT: packet_out: GPEPacket (proposed)
- [ ] [DAV-GPE-IPF-RSP-0001 — Response](gpe/ipf/rsp.md) · interface; IN: packet_in: GPEPacket (proposed); OUT: packet_out: GPEPacket (proposed)
- [ ] [DAV-GPE-IPF-RTR-0001 — Router](gpe/ipf/rtr.md) · leaf; IN: packet_in: GPEPacket (proposed); OUT: packet_out: GPEPacket (proposed)
- [ ] [DAV-GPE-IPF-XBAR-0001 — Crossbar](gpe/ipf/xbar.md) · leaf; IN: ingress_0: GPEPacket (declared); ingress_1: GPEPacket (declared); ingress_2: GPEPacket (declared); ingress_3: GPEPacket (declared); OUT: delivered: GPEPacket (declared)
