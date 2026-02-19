# Control of the programming model

The instrinsics allow control over the fields in `vtype`, as well as `vxrm` and `frm`.

## `EEW` and `LMUL`

The intrinsics' data types are strongly typed. 

They encode the `EEW` (effective element width) and `EMUL` (effective LMUL) of the destination vector register in the suffix of the function name.

Eg, this intrinsic will produce the semantic of a `vadd.vv` instruction, with source and result vector operands of `EEW=32` and `EMUL=1`

```c
vint32m1_t __riscv_vadd_vv_i32m1(vint32m1_t vs2, vint32m1_t vs1, size_t vl);
```

And this one will have source `EEW=16`, `EMUL=1/2`, and result `EEW=32`, `EMUL=1`.

```c
vint32m1_t __riscv_vwadd_vv_i32m1(vint16mf2_t vs2, vint16mf2_t vs1, size_t vl);
```

## Number of elements to be processed

The vector length control register is not directly exposed.
The `size_t vl` argument specifies the application vector length. 

## Control of vector masking

The instructions that are available for masking have masked variant instructions.

The intrinsics fuse the control of `vm`, `vta`, and `vma` in the same suffix.

Given the general assumption that target audience of the intrinsics are high performance cores, and
an "undisturbed" policy will generally slow down an out-of-order core, the intrinsics have a default
policy scheme of tail-agnostic and mask-agnostic (that is, vta=1 and vma=1).

## Fixed-point rounding mode

For the fixed-point intrinsics that are affected by rounding mode, there is a `vxrm` argument

```c
enum __RISCV_VXRM {
  __RISCV_VXRM_RNU = 0,
  __RISCV_VXRM_RNE = 1,
  __RISCV_VXRM_RDN = 2,
  __RISCV_VXRM_ROD = 3,
};
```
