# Golden Test Vectors

Reference test vectors for functional verification of XiangShan-pyc modules.

## Files

| File | Description |
|------|-------------|
| `alu_vectors.py` | All ALU operations with golden Python results |
| `decode_vectors.py` | RISC-V instruction encoding → decoded fields |
| `riscv_encodings.py` | Instruction encoders for generating test programs |

## Usage

```python
from golden.alu_vectors import ALU_VECTORS_16BIT

for src1, src2, op, exp_result, exp_zero in ALU_VECTORS_16BIT:
    ...
```

## Sources

- RISC-V ISA Manual Vol. 1, v20191213
- XiangShan KunMingHu micro-architecture documentation
