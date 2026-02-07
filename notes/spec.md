# "V" Standard Extension for Vector Operations

## Constants

- `ELEN`: the maximum element size in bits. must be power of two >= 8
- `VLEN`: the number of bits in a single vector register. must be power of two >= `ELEN` and <= 2^16

## Programmers Model

Adds 32 vector registers, `v0`-`v31` and seven unprivledged csrs to a base scalar riscv ISA.
each register has a fixed `VLEN` bits of state.

New CSRs
- `vstart`: vector start element index
- `vxsat`: fixed-point saturate flag
- `vxrm`: fixed-point rounding mode
- `vcsr`: vector control and status register
- `vl`: vector length
- `vtype`: vector data type register
- `vlenb`: VLEN/8: vector register length in bytes

### mstatus changes

A vector context status field, `VS` is added to `mstatus[10:9]` and shadowed in `sstatus[10:9]`

When `mstatus.VS=off`, any attempts to execute vector instructions, access vector csrs, etc will raise and illegal-instruction exception.
when `mstatus.VS` is set to Initial or Clean, executing any isntruction that changes vector state, including the vector CSRs, will change `mstatus.VS` to dirty.

> accurate setting of `mstatus.VS` is an optimization. Software will typically use VS to reduce context-swap overhead.

## `vtype` (Vector type) register

The readonly `XLEN`-wide *vector type* CSR, `vtype` provides the default type used to interpret the contents of the vector register file

- Can only be updated via `vset{i}vl{i}` instructions.
- Determines the organization of elements in each vector register, and how multiple registers are grouped

five fields
- `vill`: illegal if set
- `vma`: vector mask agnostic
- `vta`: vector tail agnostic
- `vsew[2:0]`: selected element width (SEW) setting
- `vlmul[2:0]`: vector register group multiplier (LMUL) setting

> the motivation behind this csr and the instruction to update it is to allow the V isa to fit in 32 bit instruction encoding space.
A seperate vsetivli instruction can be used to set `vl` and/or `vtype` before the execution of a vector inst, and implementations may choose to fuse these into a single microop

### `vsew`

sets the dynamic selected element width (`SEW`)

vecs are `VLEN`/`SEW` elements by default

| vsew | SEW |
|------|-----|
| 000  |  8  |
| 001  | 16  |
| 010  | 32  |
| 011  | 64  |
| 1xx  | ?   |

Lets say vlen = 128.
Then SEW=64 means 2 elements per vector register, SEW=32 means 4, etc.

### `vlmul`

Multiple vector registers can be grouped together, such that a single vector instruction can operate on multiple vector registers.

The term *vector register group* is used to refer to one or more vector registers used as a single operand in a vector instruction.
