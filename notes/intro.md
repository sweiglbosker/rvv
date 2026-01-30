# RISC-V Vector Quick Intro

<https://blog.timhutt.co.uk/riscv-vector/>

## Registers

- 32 vector registers `v0` to `v31`
- `v0` can optionally be used as a mask register

- each register is `VLEN` bits wide
- `VLEN` is an implementation-defined constant, cant be changed at runtime
- `VLEN` most be a power of two and <= 65536

a typical vlen value will be 128 or 512 bits

- the readonly csr `vlenb` that contains vlen in bytes

## Elements

The way vector registers can be grouped and divided into elements is flexible.

- the maximum element width is `ELEN` bits, and `VLEN` >= `ELEN` >= `8`
- the maximum elen is `64`

integer support is mandatory (8, 16, 32, 64, unsigned and signed)
fp support depends on core isa extensions (F, D, Zfh)

Element width is global state, called *selected element width* (SEW).

SEW is set via the `vsetvl` instruction. (this instruction also sets other things)

## Register Grouping (LMUL)

The vector engine can be set (via `vsetvl`) to use a group of registers instead of a single register. A similiar solution to ARM SIMD ISAs.

A group may contain 2, 4, or 8 registers; the group size is called `LMUL`. The combination of `LMUL` and `VLEN` yields `VLENMAX`.

When `LMUL` > 1, then a program is allowed to use vector

# The RISC-V Vector tutorial

<https://riscv.org/wp-content/uploads/2024/12/15.20-15.55-18.05.06.VEXT-bcn-v1.pdf>

32 vregs
- each can either hold a scalar, a vector, or a matrix (shape)
- each *can optionally have* an associated type (polymorphic encoding)
- *Variable* number of registers (dynamically changeable)

semantics
- all instructions controlled by Vector Length (VL/VLEN) register
- all instructions can be executed under mask
- intuitive memory ordering model
- precise exceptions supported

isa
- all instrucions present in base ISA are present in vector ISA
- Vector memory instructions supporting linear, strided & gather/scatter access patterns
- Optional fixed-point set
- Optional transcedental set

## New architectural state

- vl (xlen)
- vregmax (8b)
- vemaxw (3b)
- vtypeen (1b)
- vxrm (2b)
- vxcm (1b)
- fcsr.vxsat (1b)

## Adding two vector registers


# RVV llvm

LLVM uses scalable vector types represented in the form `<vscale x n x ty>`.

In the RISC-V context, `n` and `ty` coorespond to `LMUL` (register group size) and `SEW` (element width)

LLVM only supports ELEN=32 or ELEN=64, so `vscale` is defined as `VLEN/64`. Note that this means VLEN must be at least 64, so `VLEN=32` isn't currently supported.

## Mask vector types

Mask vectors are physically2 represented using a layout of densely packed bits into a vector register. They are mapped to the following llvm IR types:

- `<vscale x 1 x i1>`
- `<vscale x 2 x i1>`
- `<vscale x 4 x i1>`
- `<vscale x 8 x i1>`
- `<vscale x 16 x i1>`
- `<vscale x 32 x i1>`
- `<vscale x 64 x i1>`

Two types with the same `SEW`/`LMUL` ratio will have the same related mask type.

## Representation in LLVM IR

1. Regular instructions on both scalable and fixed-length vector types
    ```llvm
    %c = add <vscale x 4 x i32> %a, %b
    %f = add <4 x i32> %d, %e
    ```
2. RISC-V vector intrinsics, which mirror the C intrinsics. There are both masked and unmasked variants
    ```llvm
    %c = call @llvm.riscv.vadd.nxv4i32.nxv4i32(
        <vscale x 4 x i32> %passthru,
        <vscale x 4 x i32> %a,
        <vscale x 4 x i32> %b,
        i64 %avl
    )
    %d = call @llvm.riscv.vadd.mask.nxv4i32.nxv4i32(
        <vscale x 4 x i32> %passthru,
        <vscale x 4 x i32> %a,
        <vscale x 4 x i32> %b,
        <vscale x 4 x i1> %mask,
        i64 %avl,
        i64 0 ; policy (must be an immediate)
    )
    ```

4. Vector Predication (VP) instrinsics
    ```llvm
    %c = call @llvm.vp.add.nxv4i32(
        <vscale x 4 x i32> %a,
        <vscale x 4 x i32> %b,
        <vscale x 4 x i1> %m
        i32 %evl
    )
    ```

Unlike RISC-V intrinsics, VP intrinsics are target agnostic so they can be emitted from other optimisation passes in the middle-end (like the loop vectorizer). They also support fixed-length vector types.

VP intrinsics also don’t have passthru operands, but tail/mask undisturbed behaviour can be emulated by using the output in a @llvm.vp.merge. It will get lowered as a vmerge, but will be merged back into the underlying instruction’s mask via RISCVDAGToDAGISel::performCombineVMergeAndVOps.


