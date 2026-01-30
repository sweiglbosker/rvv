Vector Memory Instructions
==========================

- vector load:vlb, vlbu, vlh, vlhu, vlw, vlwu, vld, vflh, vflw, vfld
- vector load, strided: vlsb, vlsbu, vlsh, vlshu, vlsw, vlswu, vlsd, vflsh, vflsw, vflsd
- vector load, indexed (gather): vlxb, vlxbu, vlxh, vlxhu, vlxw, vlxwu, vlxd, vflxh, vflxw, vflxd
- vector store: vsb, vs, vsw, vsd
- vector store strided: vssb, vssh, vssw, vssd
- vector store indexed (scatter): vsxb, vsxh, vsxw, vsxd
- vector store, indexed, unordered: vsxub, vsxuh, vsxuw, vsxud

Vector Integer Instructions
===========================

- add: vadd,    vaddi, vaddw, vaddiw
- subtract:     vsub, vsubw
- multiply:     vmul, vmulh, vmulhsu, vmulhu
- widening mul: vmulwdn
- divide:       vdiv, vdivu, vrem, vremu
- shift:        vsll, vslli, vsra, vsrai, vsrl, vsrli
- logical:      vand, vandi, vor, vori, vxor, vxori
- compare:      vseq, vslt, vsltu
- fixed point:  vclipb, vclipbu, vcliph, vcliphu, vclipw, vclipwu

Vector Data Movement
====================

- insert gpr or fp into vector: `vins vd, src, index` (index : gpr, src : gpr or fp)
- extract: `vext rd, vs, index`
- vector-{vector,gpr,fp} merge: `vmerge vd, dst, vs2, vm` (mask picks src)
- vector register gather: `vrgather vd, vs1, vs2, vm`
- gpr splat/bcast: `vsplatx vd, rs1`
