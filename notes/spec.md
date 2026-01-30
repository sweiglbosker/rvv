# "V" Standard Extension for Vector Operations

## Constants

- `ELEN`: the maximum element size in bits. must be power of two >= 8
- `VLEN`: the number of bits in a single vector register. must be power of two >= `ELEN` and <= 2^16

## Programmers Model

Adds 32 vector registers, and seven unprivledged csrs to a base scalar riscv ISA.

New CSRs
- `vstart`: vector start element index
- `vxsat`: fixed-point saturate flag
- `vxrm`: fixed-point rounding mode
- `vcsr`: vector control and status register
- `vl`: vector length
- `vtype`: vector data type register
- `vlenb`: VLEN/8: vector register length in bytes


