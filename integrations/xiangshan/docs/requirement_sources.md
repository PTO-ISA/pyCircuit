# Requirement Sources — XiangShan-pyc

Document index and parameter inventory for the PyCircuit V6 reimplementation.

## Documentation index

### Frontend

| Document | Path | Key content |
|----------|------|------------|
| Frontend overview | `XiangShan-doc/docs/frontend/overview.md` | Architecture, data flow |
| Branch prediction | `XiangShan-doc/docs/frontend/bp.md` | BPU pipeline (uBTB, TAGE, SC, ITTAGE, RAS) |
| FTQ | `XiangShan-doc/docs/frontend/ftq.md` | Fetch Target Queue |
| IFU | `XiangShan-doc/docs/frontend/ifu.md` | Instruction Fetch Unit, pre-decode |
| ICache | `XiangShan-doc/docs/frontend/icache.md` | Instruction cache |
| Decode | `XiangShan-doc/docs/frontend/decode.md` | Decode unit |

### Backend

| Document | Path | Key content |
|----------|------|------------|
| Backend overview | `XiangShan-doc/docs/backend/overview.md` | CtrlBlock, IntBlock, FloatBlock |
| Rename | `XiangShan-doc/docs/backend/rename.md` | RAT, freelist |
| Dispatch | `XiangShan-doc/docs/backend/dispatch.md` | Dispatch queues |
| Issue / Scheduler | `XiangShan-doc/docs/backend/issue.md`, `scheduler.md` | Issue queue, wakeup |
| ROB | `XiangShan-doc/docs/backend/rob.md` | Reorder buffer |
| ExeUnits | `XiangShan-doc/docs/backend/exu.md`, `exu_int.md`, `exu_fp.md` | Execution units |

### Memory subsystem

| Document | Path | Key content |
|----------|------|------------|
| MemBlock overview | `XiangShan-doc/docs/memory/overview.md` | Load/Store pipelines, LSQ |
| Mechanism | `XiangShan-doc/docs/memory/mechanism.md` | Memory dependency, forwarding |
| Load pipeline | `XiangShan-doc/docs/memory/fu/load_pipeline.md` | Load stages |
| Store pipeline | `XiangShan-doc/docs/memory/fu/store_pipeline.md` | Store stages |
| Atom | `XiangShan-doc/docs/memory/fu/atom.md` | Atomic operations |
| Load Queue | `XiangShan-doc/docs/memory/lsq/load_queue.md` | Load queue |
| Store Queue | `XiangShan-doc/docs/memory/lsq/store_queue.md` | Store queue |
| DCache | `XiangShan-doc/docs/memory/dcache/dcache.md` | Data cache overview |
| DCache main pipe | `XiangShan-doc/docs/memory/dcache/main_pipe.md` | Main pipeline |
| DCache load pipe | `XiangShan-doc/docs/memory/dcache/load_pipeline.md` | Load pipeline |
| Miss Queue | `XiangShan-doc/docs/memory/dcache/miss_queue.md` | MSHR |
| MMU overview | `XiangShan-doc/docs/memory/mmu/mmu.md` | MMU |
| TLB | `XiangShan-doc/docs/memory/mmu/tlb.md` | Translation lookaside buffer |
| L2 TLB | `XiangShan-doc/docs/memory/mmu/l2tlb.md` | L2 TLB / PTW |

### Cache hierarchy

| Document | Path | Key content |
|----------|------|------------|
| HuanCun overview | `XiangShan-doc/docs/huancun/overview.md` | L2/L3 cache |
| Channels | `XiangShan-doc/docs/huancun/channels.md` | TileLink channel controllers |
| Directory | `XiangShan-doc/docs/huancun/directory.md` | Cache directory |
| MSHR | `XiangShan-doc/docs/huancun/mshr.md` | Miss status holding registers |

### Reference implementation (key files)

| File | Path | Used for |
|------|------|----------|
| Core parameters | `XiangShan/src/main/scala/xiangshan/Parameters.scala` | Default config values |
| Top configs | `XiangShan/src/main/scala/top/Configs.scala` | Config composition |
| Frontend params | `XiangShan/src/main/scala/xiangshan/frontend/FrontendParameters.scala` | Frontend parameters |
| Backend params | `XiangShan/src/main/scala/xiangshan/backend/BackendParams.scala` | Backend parameters |

## Parameter inventory (KunMingHu defaults)

See `top/parameters.py` for the full Python extraction.  Key values:

| Parameter | Value | Source |
|-----------|-------|--------|
| XLEN | 64 | `Parameters.scala` |
| VLEN | 128 | `Parameters.scala` |
| FetchBlockSize | 64 bytes | `FrontendParameters` |
| DecodeWidth | 8 | `Parameters.scala` |
| RenameWidth | 8 | `Parameters.scala` |
| CommitWidth | 8 | `Parameters.scala` |
| RobSize | 352 | `Parameters.scala` |
| IntPhysRegs | 224 | `intPreg.numEntries` |
| FpPhysRegs | 256 | `fpPreg.numEntries` |
| VfPhysRegs | 128 | `vfPreg.numEntries` |
| IssueQueueSize | 20 | `Parameters.scala` |
| LoadPipelineWidth | 3 | `Parameters.scala` |
| StorePipelineWidth | 2 | `Parameters.scala` |
| StoreBufferSize | 16 | `Parameters.scala` |
| VirtualLoadQueueSize | 72 | `Parameters.scala` |
| StoreQueueSize | 56 | `Parameters.scala` |
| ICache | 256 sets x 4 ways x 64B line = 64KB | `ICacheParameters` default |
| DCache | 256 sets x 8 ways x 64B line = 128KB | `DCacheParameters` default |
| L2 Cache | 1024 sets x 8 ways = 512KB | `L2Param` default |
| ITLB ways | 48 | `itlbParameters` |
| LDTLB ways | 48 | `ldtlbParameters` |
