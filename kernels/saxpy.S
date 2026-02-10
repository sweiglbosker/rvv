.global saxpyf32
.type saxpyf32, @function
// void saxpyf32_ref(float* res, const float* x, const float* y, float s, size_t vlen) {
//    res: a0
//    x: a1
//    y: a2
//    s: fa0
//    vlen: a3
saxpyf32:
  beqz a3, .Ldone
.Lloop:
  vsetvli t0, a3, e32, m1, ta, ma
  vle32.v v4, (a1)
  vle32.v v5, (a2)
  vfmacc.vf v5, fa0, v4

  vse32.v v5, (a0)

  slli t1, t0, 2
  add a0, a0, t1
  add a1, a1, t1
  add a2, a2, t1

  sub a3, a3, t0
  bnez a3, .Lloop
.Ldone:
  ret
