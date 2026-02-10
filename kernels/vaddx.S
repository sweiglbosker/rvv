// void vaddx32_ref(int32_t *a, int32_t s, size_t vlen) {
// a: a0, s: a1, vlen: a2
.globl vaddx32
.type vaddx32, @function
vaddx32:
  beqz a2, .Ldone
.Lloop:
  # avl = a2, sew=32, LMUL = 1
  vsetvli t0, a2, e32, m1, ta, ma
  # updates t0 with numelts to proccess
  vle32.v v4, (a0)
  vadd.vx v4, v4, a1
  vse32.v v4, (a0)

  slli t1, t0, 2 # multiply numelts this iteration by 4 so we can bump ptr
  add a0, a0, t1
  sub a2, a2, t0
  bnez a2, .Lloop
.Ldone:
  ret
