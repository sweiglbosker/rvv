Glossary
========

- a `vm` argument represents an optional mask. It can be `v0.t`, or not present for unmasked

Vector Load/Store
=================

- `vlm.v vd, (rs1)`: Load mask: loads byte vector of length ceil(vl/8) 
- `vsm.v vs3, (rs1)`: Store mask

## Unit Strided

- `vle64.v vd, (rs1), vm`: 64-bit unit-strided load
- `vse32.v vs3, (rs1), vm`: 32-bit unit-strided store

- `vle64ff.v vd, (rs1), vm`: fault-only-first variant

## Strided

- `vlse16.v vd, (rs1), rs2, vm`: 16-bit strided load, rs2=byte stride
- `vsse64.v vs3, (rs1), rs2, vm `: 64-bit strided store

## Indexed

> Note: the bitlen suffix here is for the indexes, not the elements (those obey SEW)

- `vluxei64.v vd, (rs1), vs2, vm`: Index unordered load of SEW data, vs2=byte offsets
- `vloxei32.v vd, (rs1), vs2, vm`: Index ordered load
- `vsuxei16.v vs3, (rs1), vs2, vm`:  Index unordered store
- `vsoxei8.v vs3, (rs1), vs2, vm`:  Index ordered store

