# "V" Standard Extension for Vector Operations, Programing model

## Constants

- `ELEN`: the maximum element size in bits. must be power of two >= 8
- `VLEN`: the number of bits in a single vector register. must be power of two >= `ELEN` and <= 2^16

## Registers and CSRs

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

When >1, `LMUL` represents the default number of vector registers that are combined to form a vector register group.
Implementations **must** support `LMUL` integers values

LMUL can also be a fractional value, reducing the number of bits used in a single vector register.
Fractional LMUL is used to increase the number of effectively usable register groups when operating on mixed-width values.

LMUL is set by the signed `vlmul` field in `vtype`
LMUL = 2^`vlmul[2:0]`

The derived value `VLMAX=LMUL*VLEN/SEW` represents the maximum number of elements that can be operated on with a single vector instruction given the current SEW and LMUL settings.

When LMUL=2, the vector register group conains vector register `v<n>` and `v<n+1>`, providing twice the vector length in bits.

> Instructions specifying an LMUL=8 vector register group using vector register numbers that are not multiples of eight are reserved.

> Mask registers are always contained in a single vector register, regardless of LMUL

### `vta` and `vma`

These two bits modify the behavior of destination tail elements, and destination inactive masked-off elements

The tail and inactive sets contain element positions that are not recieving new results during a vector operation

glossary:
- undisturbed: the corresponding set of destination elements retain their value.
- agnostic: each tail element can either retain their previous value, or be overwritten with 1s.

if `vta` = 0, tail elements are undisturbed
if `vta` = 1, tail elements are agnostic 

same for vma, but for inactive elements (masked off)

> note: The agnostic policy was added to accommodate machines with vector register renaming. With an undisturbed policy, all elements would have to be read from the old physical destination vector register to be copied into the new physical destination vector register. This causes an inefficiency when these inactive or tail values are not required for subsequent calculations.

syntactically, two mandatory flags are added to the vsetvli instruction:


```asm
ta   // Tail agnostic
tu   // Tail undisturbed
ma   // Mask agnostic
mu   // Mask undisturbed

// Example: tail agnostic, mask agnostic
vsetvli t0, a0, e32, m4, ta, ma
```


### `vill`

The `vill` bit is used to encode that a previous vset{i}vl{i} instruction attempted to write

## Vector length register, `vl`

The `vl` register holds an unsigned integer specifying the number of elements to be updated with the results from a vector instruction

XLEN-bit-wide, read-only CSR `vl` can only be updated via vset{i}vl{i} instructions, and the *fault-only-first* vector load instruction variants.

> note to self: read-only only means you cant write to in with csrw, but it can still be written to via the above instructions

(will be elaborated on later in spec)

## Vector byte length, `vlenb`

Read-only CSR that holds the value VLEN/8. 

(therefore, it is also a design-time implementation defined constant)

## Vector start index, `vstart` register

Specifies the index of the first element to be executed by a vector instruction.

Normally, `vstart` is only written to by hardware on a trap on a vector instruction, with the `vstart` value representing the element on which a trap was taken. (either a synchronous exception or an asynchronous interrupt), at which execution should resume after a resumable trap is handled.

All vector instructions (including vset{i}vl{i}) reset `vstart` to 0.

All vector instructions are defined to begin execution with the element number given in the `vstart` CSR, leaving earlier elements in the destination vector undisturbed, and to reset the `vstart` CSR at the end of execution.

The `vstart` CSR is writable by unprivledged code, but non-zero `vstart` values may cause vector instructions to run substantially slower on some implementations, so `vstart` should not be used in application code.

also some vector instructions my raise an illegal instruction exception on nonzero vstart.

## `vxrm`, Vector fixed-point rounding mode register

`vxrm[1:0]` holds a two-bit read-write rounding-mode field. (least significant bits)

The upper bits `vxrm[XLEN-1:2]` should be written as zeros
The vector fixed-point rounding-mode is given a separate CSR address to allow independent access, but is also reflected as a field in vcsr.

> Note: A new rounding mode can be set while saving the original rounding mode using a single `csrwi` instruction

The fixed-point rounding algorithm is specified as follows:

Suppose the pre rounding result is `v`, and `d` bits of that result are to be rounded off. Then the rounded result is `(v >> d) + r`, where `r` depends on the rounding mode.

Rounding modes:
- `rnu`: "round-to-nearest-up" (add +0.5 LSB), `r=v[d-1]`
- `rne`: "round-to-nearest-even", `v[d-1] & (v[d-2:0]≠0 | v[d])`
- `rdn`: "round-down" (truncate), `0`
- `rod`: "round-to-odd" (OR bits in LSB, aka "JAM") `!v[d] & v[d-1:0]≠0`

The rounding functions

```
roundoff_unsigned(v, d) = (unsigned(v) >> d) + r
roundoff_signed(v, d) = (signed(v) >> d) + r
```

are used to represent this operation in the instruction descriptions below

> note to self: so roundoff signed is just an ashr, and unsigned is lshr?

## Vector Fixed-Point Saturation Flag (`vxsat`)

`vxsat[0]` indicates if a fixed-point instruction has to saturate an output value to fit into a destination format.

`vxsat` is mirrored in vcsr.

## Vector CSR `vcsr`

As mentioned, `vxrm` and `vxsat` can also be accesed via field in the XLEN-wide `vcsr`.

- `vcsr[0]` = `vxsat`
- `vcsr[2:1]` = `vxrm[1:0]`

the rest is reserved

## State of Vector Extension at reset

The vector extension *must* have a consistent state at reset.

In particular, `vtype` and `vl` must have values that can be read and then restored via a single `vsetvl` instruction.

> Recommended by spec: set `vtype.vill` at reset, zero remaining bits in `vtype`, and set `vl` to 0.

`vstart`, `vxrm`, `vxsat` can have abritary values at reset.

> Most uses of the vector unit will require an initial `vset{i}vl{i}`, which will reset `vstart`. The `vxrm` and `vxsat` fields should be reset explicitly in software before use.

The vector registers can have arbitrary values at reset.

# Mapping of Vector Elements to Vector Register State

different width elements are packed into the bytes of a vector register depending on the current `SEW` and `LMUL` settings, as well as implementation `VLEN`. Elements are packed into each vector register with the lsb in the lowest-numbered bits.

> The mapping was chosen to provide the simplest and most portable model for software, but might appear to incur large wiring cost for wider vector datapaths on certain operations. The vector instruction set was expressly designed to support implementations that internally rearrange vector data for different SEW to reduce datapath wiring costs, while externally preserving the simple software model.

## LMUL=1

When LMUL=1, elements are simply packed in order from the least-significant to the most-significant bits of the vector register.

```
LMUL=1 examples.

VLEN=32b

 Byte         3 2 1 0

 SEW=8b       3 2 1 0
 SEW=16b        1   0
 SEW=32b            0

 VLEN=64b

 Byte        7 6 5 4 3 2 1 0

 SEW=8b      7 6 5 4 3 2 1 0
 SEW=16b       3   2   1   0
 SEW=32b           1       0
 SEW=64b                   0

 VLEN=128b

 Byte        F E D C B A 9 8 7 6 5 4 3 2 1 0

 SEW=8b      F E D C B A 9 8 7 6 5 4 3 2 1 0
 SEW=16b       7   6   5   4   3   2   1   0
 SEW=32b           3       2       1       0
 SEW=64b                   1               0

 VLEN=256b

 Byte     1F1E1D1C1B1A19181716151413121110 F E D C B A 9 8 7 6 5 4 3 2 1 0

 SEW=8b   1F1E1D1C1B1A19181716151413121110 F E D C B A 9 8 7 6 5 4 3 2 1 0
 SEW=16b     F   E   D   C   B   A   9   8   7   6   5   4   3   2   1   0
 SEW=32b         7       6       5       4       3       2       1       0
 SEW=64b                 3               2               1               0
```


## LMUL<1

When LMUL<1, only the first LMUL*VLEN/SEW elements in a vector register are used.

The remaining space in the vector register is treated as part of the tail, and hence must obey the `vta` setting

```
 Example, VLEN=128b, LMUL=1/4

 Byte        F E D C B A 9 8 7 6 5 4 3 2 1 0

 SEW=8b      - - - - - - - - - - - - 3 2 1 0
 SEW=16b       -   -   -   -   -   -   1   0
 SEW=32b           -       -       -       0
```

## LMUL>1

When vector registers are grouped, the elements of the vector register group are packed contiguously in element order beginning with the lowest-numbered vector register and moving to the next-highest-numbered vector register in the group once each vector register is filled.


```
LMUL > 1 examples

 VLEN=32b, SEW=8b, LMUL=2

 Byte         3 2 1 0
 v2*n         3 2 1 0
 v2*n+1       7 6 5 4

 VLEN=32b, SEW=16b, LMUL=2

 Byte         3 2 1 0
 v2*n           1   0
 v2*n+1         3   2

 VLEN=32b, SEW=16b, LMUL=4

 Byte         3 2 1 0
 v4*n           1   0
 v4*n+1         3   2
 v4*n+2         5   4
 v4*n+3         7   6

 VLEN=32b, SEW=32b, LMUL=4

 Byte         3 2 1 0
 v4*n               0
 v4*n+1             1
 v4*n+2             2
 v4*n+3             3

 VLEN=64b, SEW=32b, LMUL=2

 Byte         7 6 5 4 3 2 1 0
 v2*n               1       0
 v2*n+1             3       2

 VLEN=64b, SEW=32b, LMUL=4

 Byte         7 6 5 4 3 2 1 0
 v4*n               1       0
 v4*n+1             3       2
 v4*n+2             5       4
 v4*n+3             7       6

 VLEN=128b, SEW=32b, LMUL=2

 Byte        F E D C B A 9 8 7 6 5 4 3 2 1 0
 v2*n              3       2       1       0
 v2*n+1            7       6       5       4

 VLEN=128b, SEW=32b, LMUL=4

 Byte          F E D C B A 9 8 7 6 5 4 3 2 1 0
 v4*n                3       2       1       0
 v4*n+1              7       6       5       4
 v4*n+2              B       A       9       8
 v4*n+3              F       E       D       CLMUL > 1 examples

 VLEN=32b, SEW=8b, LMUL=2

 Byte         3 2 1 0
 v2*n         3 2 1 0
 v2*n+1       7 6 5 4

 VLEN=32b, SEW=16b, LMUL=2

 Byte         3 2 1 0
 v2*n           1   0
 v2*n+1         3   2

 VLEN=32b, SEW=16b, LMUL=4

 Byte         3 2 1 0
 v4*n           1   0
 v4*n+1         3   2
 v4*n+2         5   4
 v4*n+3         7   6

 VLEN=32b, SEW=32b, LMUL=4

 Byte         3 2 1 0
 v4*n               0
 v4*n+1             1
 v4*n+2             2
 v4*n+3             3

 VLEN=64b, SEW=32b, LMUL=2

 Byte         7 6 5 4 3 2 1 0
 v2*n               1       0
 v2*n+1             3       2

 VLEN=64b, SEW=32b, LMUL=4

 Byte         7 6 5 4 3 2 1 0
 v4*n               1       0
 v4*n+1             3       2
 v4*n+2             5       4
 v4*n+3             7       6

 VLEN=128b, SEW=32b, LMUL=2

 Byte        F E D C B A 9 8 7 6 5 4 3 2 1 0
 v2*n              3       2       1       0
 v2*n+1            7       6       5       4

 VLEN=128b, SEW=32b, LMUL=4

 Byte          F E D C B A 9 8 7 6 5 4 3 2 1 0
 v4*n                3       2       1       0
 v4*n+1              7       6       5       4
 v4*n+2              B       A       9       8
 v4*n+3              F       E       D       C
```

## Mapping across Mixed-Width Operations

The vector ISA is designed to support mixed-width operations without requiring additional explicit rearrangement instructions.

Recommended software strategy: modify `vtype` dynamically to keep `SEW/LMUL` constant (and hence `VLMAX`)

Example, changing `LMUL` to keep vlmax constant

```
Example VLEN=128b, with SEW/LMUL=16

Byte      F E D C B A 9 8 7 6 5 4 3 2 1 0
vn        - - - - - - - - 7 6 5 4 3 2 1 0  SEW=8b, LMUL=1/2

vn          7   6   5   4   3   2   1   0  SEW=16b, LMUL=1

v2*n            3       2       1       0  SEW=32b, LMUL=2
v2*n+1          7       6       5       4

v4*n                    1               0  SEW=64b, LMUL=4
v4*n+1                  3               2
v4*n+2                  5               4
v4*n+3                  7               6
```

Larger LMUL settings can also used to simply increase vector length to reduce instruction fetch and dispatch overheads in cases where fewer vector register groups are needed.

## Mask Register Layout

Always occupies a single vector register.

Each element is allocated a single mask bit in a mask vector register. The mask bit for element *i* is located in bit *i* of the mask register.

# Vector instruction formats

The instructions fit under two existing major opcodes (`LOAD-FP` and `STORE-FP`), and one new major opcode (`OP-V`).

Vector loads and stores are encoded within the scalar floating-point load and store major opcodes.

The vector load and store encodings repurpose a portion of the standard scalar floating-point load/store 12-bit immediate field to provide further vector instruction encoding, with bit 25 holding the standard vector mask bit.

Vector instructions can have scalar or vector source operands and produce scalar or vector results, and most vector instructions can be performed either unconditionally or conditionally under a mask


## Scalar Operands

Scalar operands can be immediates or taken from the `x` registers, the `f` registers, or element 0 of a `v` register.

Any vector register can be used to hold a scalar regardless of current LMUL setting.

> note: also compatible with Zfinx

## Vector Operands

Each vector operand has an *effective element width* (EEW) and an *effective* LMUL (EMUL) that is used to determine the size and location of all the elements within a vector register group. By default, for most operands of most instructions, EEW=SEW and EMUL=LMUL

Some instructions have src and dst vector operands with the same number of elements but different widths, so that EEW and EMUL differ from SEW and LMUL respectively but EEW/LMUL = SEW/LMUL.

A vector register cant be used to provide source operands with more than one EEW for a single instruction. A mask source register is considered to have EEW=1 for this constraint.

A destination vector register group can overlap a source vector register group iff one of the following holds.
- The destination EEW equals the source EEW
- The destination EEW is smaller than the source EEW and the overlap is in the lowest-numbered part of the source register group (e.g., when LMUL=1, vnsrl.wi v0, v0, 3 is legal, but a destination of v1 is not
- The destination EEW is greater than the source EEW, the source EMUL is at least 1, and the overlap is in the highest-numbered part of the destination register group (e.g., when LMUL=8, vzext.vf4 v0, v6 is legal, but a source of v0, v2, or v4 is not).

For the purpose of determining register group overlap constraints, mask elements have EEW=1.

> The overlap constraints are designed to support resumable exceptions in machines without register renaming.

The largest vector register group used by an instruction cannot be greater than 8 vector registers.

Widened scalar values, e.g. input and output to a widening reduction operation, are held in the first element of a vector register and have EMUL=1

## Vector Masking

Masking is supported on many vector instructions. 

Element operations that are masked off (inactive) never generate exceptions.

The destination vector register elements corresponding to masked-off elements are handled based on `vma` (as described earlier)

The mask value used to control execution of a masked vector instruction is always supplied by vector register `v0`.

The destination vector register group for a masked vector instruction cant overlap the source mask register (`v0`), unless the destination vector register is being written with a mask value (e.g., compares), or the scalar result of a reduction.

> in other words, you cant clobber the mask register while using it

Also, writing a mask value to `v0` is always `vta`

### Mask encoding

Where available, masking is encoded in a single-bit `vm` field in the instruction (`inst[25]`)

- `vm=0` means the instruction is masked. So vector result only where `v0.mask[i]=1`
- `vm=1` means unmasked

in assemply, vector maskig is represented as another vector operand, with .t indicating that the op is masked. If no masking operand is specified, unmasked execution is assumed.

```asm
vop.v*  v1, v2, v3, v0.t    # masked
vop.v*  v1, v2, v3          # unmasked
```

## Prestart, Active, Inactive, Body, and Tail Element Definitions

The destination element indicies operated on during a vector instruction can be divided into three disjoint subsets.

- prestart: before `vstart`
- body: element index >= `vstart`, < `vl`. The body can be split into two disjoint subsets
    - *active* elements: where the current mask is enabled at that position
    - *inactive* elements
- tail: the elements past the current `vl`. When LMUL < 1, the tail includes the elements past `VLMAX` held in the same vector register
    - dont raise exceptions.
    - dont update any destination register group unless `vta`


# Configuration Setting Instructions (`vsetvli`/`vestivli`/`vsetvl`)

One of the common approaches to handling a large number of elements is "stripmining", where each iteration of a loop handles some number of elements, and the iterations continue until all elements have been processed.

The application specifies the total number of elements to be processed (the application vector length or AVL) as a candidate value for `vl`, and the hardware responds via a general purpose register with the (usually smaller), and the hardware responds via a general-purpose register with the number of elements that the hardware will handle per iteration (stored in `vl`), based on the microarchitectural implementation of the `vtype` setting.

Stripmining example:


```asm
# Example: Load 16-bit values, widen multiply to 32b, shift 32b result
# right by 3, store 32b values.
# On entry:
#  a0 holds the total number of elements to process
#  a1 holds the address of the source array
#  a2 holds the address of the destination array
loop:
    vsetvli a3, a0, e16, m4, ta, ma  # vtype = 16-bit integer vectors;
                                     # also update a3 with vl (# of elements this iteration)
    vle16.v v4, (a1)        # Get 16b vector
    slli t1, a3, 1          # Multiply # elements this iteration by 2 bytes/source element
    add a1, a1, t1          # Bump pointer
    vwmul.vx v8, v4, x10    # Widening multiply into 32b in <v8--v15>

    vsetvli x0, x0, e32, m8, ta, ma  # Operate on 32b values
    vsrl.vi v8, v8, 3
    vse32.v v8, (a2)        # Store vector of 32b elements
    slli t1, a3, 2          # Multiply # elements this iteration by 4 bytes/destination element
    add a2, a2, t1          # Bump pointer
    sub a0, a0, a3          # Decrement count by vl
    bnez a0, loop           # Any more?
```


A set of instructions is provided to allow rapid configuration of the values in `vl` and `vtype` to match their application needs.

```asm
vsetvli  rd, rs1, vtypei    # rd = new vl, rs1 = AVL, vtypei = new vtype setting
vsetivli rd, uimm, vtypei   # rd = new vl, uimm = AVL, vtypei = new vtype setting
vsetvl   rd, rs1, rs2       # rd = new vl, rs1 = AVL, rs2 = new vtype value
```


## Vtype encoding

the vtype immediate has the following syntax:

```asm
e8   // SEW=8b
e16  // SEW=16b
e32  // SEW=32b
e64  // SEW=64b

mf8  // LMUL=1/8
mf4  // LMUL=1/4
mf2  // LMUL=1/2
m1   // LMUL=1
m2   // LMUL=2
m4   // LMUL=4
m8   // LMUL=8

// Examples:

vsetvli t0, a0, e8, m1, ta, ma      # SEW= 8, LMUL=1
vsetvli t0, a0, e8, m2, ta, ma      # SEW= 8, LMUL=2
vsetvli t0, a0, e32, mf2, ta, ma    # SEW=32, LMUL=1/2
```

## AVL encoding

The new vector length setting is based on AVL, which for `vsetvli` and `vsetvl` is encoded in the rs1 and rd field as follows:

When rs1 is not x0, the AVL is an unsigned integer held in the x register specified by rs1, and the new vl value is also written to the x register specified by rd.

When rs1=x0 but rd≠x0, the maximum unsigned integer value (~0) is used as the AVL, and the resulting VLMAX is written to vl and also to the x register specified by rd.

When rs1=x0 and rd=x0, the instruction operates as if the current vector length in vl is used as the AVL, and the resulting value is written to vl, but not to a destination register. This form can only be used when VLMAX and hence vl is not actually changed by the new SEW/LMUL ratio. Use of the instruction with a new SEW/LMUL ratio that would result in a change of VLMAX is reserved. Use of the instruction is also reserved if vill was 1 beforehand. Implementations may set vill in either case.

## Constraints on setting `vl`

The vset{i}vl{i} instructions first set VLMAX according to their `vtype` argument, then set `vl` obeying the following contraints:

1. vl = AVL if AVL ≤ VLMAX
2. ceil(AVL / 2) ≤ vl ≤ VLMAX if AVL < (2 * VLMAX)
3. vl = VLMAX if AVL ≥ (2 * VLMAX)
4. Deterministic on any given implementation for same input AVL and VLMAX values
5. These specific properties follow from the prior rules:
    - vl = 0 if AVL = 0
    - vl > 0 if AVL > 0
    - ≤ VLMAX
    - ≤ AVL
    - a value read from vl when used as the AVL argument to vset{i}vl{i} results in the same value in vl, provided the resultant VLMAX equals the value of VLMAX at the time that vl was read


# Vector loads and stores

## Vector Load/Store encoding

repurposes fp load/store major opcodes.

Vector memory unit-stride and constant-stride operations directly encode EEW of the data to be transferred statically in the instruction to reduce the number of vtype changes when accessing memory in a mixed-width routine.
Indexed operations use the explicit EEW encoding in the instruction to set the size of the indices used, and use SEW/LMUL to specify the data width.

## Vector Load/Store addressing modes

Supports unit-stride, strided, and indexed (scatter/gather) addressing modes.

Vector load/store base registers and strides are taken from the GPR registers.

The base effective address for all vector accesses is given by the register named in `rs1`

Vector unit-stride operations access elements stored contiguously in memory starting from the base adderss (`base + i`)

> note: im guessing unit-stride addressed ops are the best to codegen?

Vector constant-strided operations access the first memory element at the base effective address, and then access subsequent elements at address incremements given by the *byte offset* contained in the gpr named in `rs2`.

> note: e.g want to access the first column of each row of matrix

Vector indexed operations add the contents of each element of the vector offset operand specified by `vs2` to the base effective address to give the effective address of each element.

The data vector register group has EEW=SEW, EMUL=LMUL, while the offset vector register group2 has EEW encoded in the instruction EMUL=(EEW/SEW)*LMUL

(these are also byte offsets)

> note: if im understanding this correctly, this would also imply there are different instructions for different offset types.
> also, the EMUL=(EEW/SEW)*LMUL part just means that that the number of elements is preserved (makes sense)

> The indexed operations can also be used to access fields within a vector of objects, where the vs2 vector holds pointers to the base of the objects and the scalar x register holds the offset of the member field in each object. Supporting this case is why the indexed operations were not defined to scale the element indices by the data EEW.

The `mop[1:0]` field encodes the addressing modes.

| mop | Desc.            | Opcodes      | 
|-----|------------------|--------------|
| 00  | unit-stride      | `VLE<EEW>`   |
| 01  | index-unordered  | `VLUXEI<EEW>`|
| 10  | strided          | `VLSE<EEW>`  |
| 11  | indexed-ordered  | `VLOXEI<EEW>`|

same idea for stores
