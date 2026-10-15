# 实际执行指令覆盖

| 扩展 | 指令 | 执行次数 | 状态 |
| --- | --- | ---: | --- |
| I | `lui` | 487 | normal |
| I | `auipc` | 29 | normal |
| I | `jal` | 30 | normal |
| I | `jalr` | 36 | normal |
| I | `beq` | 56 | normal |
| I | `bne` | 517 | normal |
| I | `blt` | 41 | normal |
| I | `bge` | 31 | normal |
| I | `bltu` | 2 | normal |
| I | `bgeu` | 18 | normal |
| I | `lb` | 13 | normal |
| I | `lh` | 13 | normal |
| I | `lw` | 221 | normal |
| I | `lbu` | 61 | normal |
| I | `lhu` | 13 | normal |
| I | `sb` | 22 | normal |
| I | `sh` | 6 | normal |
| I | `sw` | 255 | normal |
| I | `addi` | 1880 | normal |
| I | `slti` | 16 | normal |
| I | `sltiu` | 16 | normal |
| I | `xori` | 5 | normal |
| I | `ori` | 8 | normal |
| I | `andi` | 7 | normal |
| I | `slli` | 80 | normal |
| I | `srli` | 15 | normal |
| I | `srai` | 19 | normal |
| I | `add` | 199 | normal |
| I | `sub` | 48 | normal |
| I | `sll` | 22 | normal |
| I | `slt` | 19 | normal |
| I | `sltu` | 18 | normal |
| I | `xor` | 16 | normal |
| I | `srl` | 23 | normal |
| I | `sra` | 23 | normal |
| I | `or` | 16 | normal |
| I | `and` | 12 | normal |
| I | `fence` | 1 | normal |
| I | `ecall` | 59 | normal |
| I | `ebreak` | 1 | confirmed-reference-defect |
| M | `mul` | 90 | normal |
| M | `mulh` | 30 | normal |
| M | `mulhsu` | 30 | normal |
| M | `mulhu` | 30 | normal |
| M | `div` | 24 | normal |
| M | `divu` | 24 | normal |
| M | `rem` | 24 | normal |
| M | `remu` | 45 | normal |
| C | `c.addi4spn` | 1 | normal; enclosing rvc program has separate fetch defect |
| C | `c.lw` | 39 | normal |
| C | `c.sw` | 11 | normal |
| C | `c.nop` | 16 | normal |
| C | `c.addi` | 78 | normal |
| C | `c.jal` | 2 | normal |
| C | `c.li` | 47 | normal |
| C | `c.addi16sp` | 2 | normal; enclosing rvc program has separate fetch defect |
| C | `c.lui` | 4 | normal |
| C | `c.srli` | 1 | normal; enclosing rvc program has separate fetch defect |
| C | `c.srai` | 1 | normal; enclosing rvc program has separate fetch defect |
| C | `c.andi` | 1 | normal; enclosing rvc program has separate fetch defect |
| C | `c.sub` | 1 | normal; enclosing rvc program has separate fetch defect |
| C | `c.xor` | 1 | normal; enclosing rvc program has separate fetch defect |
| C | `c.or` | 1 | normal; enclosing rvc program has separate fetch defect |
| C | `c.and` | 1 | normal; enclosing rvc program has separate fetch defect |
| C | `c.j` | 22 | normal |
| C | `c.beqz` | 5 | normal |
| C | `c.bnez` | 13 | normal |
| C | `c.slli` | 1 | normal; enclosing rvc program has separate fetch defect |
| C | `c.lwsp` | 5 | normal |
| C | `c.jr` | 7 | normal |
| C | `c.mv` | 38 | normal |
| C | `c.ebreak` | 1 | reference-unsupported |
| C | `c.jalr` | 1 | normal; enclosing rvc program has separate fetch defect |
| C | `c.add` | 38 | normal |
| C | `c.swsp` | 4 | normal |
