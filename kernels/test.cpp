#include <array>
#include <iostream>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <random>

static std::mt19937 mt(time(nullptr));

extern "C" {
void vaddx32_ref(int32_t *a, int32_t s, size_t vlen);
void vaddx32(int32_t *a, int32_t s, size_t vlen);
}

void vaddx32_ref(int32_t *a, int32_t s, size_t vlen) {
  for (int i = 0; i < vlen; i++) {
    a[i] += s;
  }
}

bool test_vaddx32(size_t vlen) {
  std::unique_ptr<int32_t[]> ref_buf{new int32_t[vlen]};
  std::unique_ptr<int32_t[]> buf{new int32_t[vlen]};

  int32_t x = mt();

  for (int i = 0; i < vlen; i++) {
    int32_t v = mt();
    ref_buf[i] = v;
    buf[i] = v;
  }

  vaddx32_ref(ref_buf.get(), x, vlen);
  vaddx32(buf.get(), x, vlen);

  for (int i = 0; i < vlen; i++) {
    if (buf[i] != ref_buf[i]) {
      std::cout << "\033[31merror\033[0m: expected " << ref_buf[i]
                << " but got " << buf[i] << "\n";
      std::cout << "at i=" << i << ", with x=" << x << "\n";
      return false;
    }
  }
  return true;
}

int main(void) {
  constexpr std::array<int, 5> interesting_vlens{ 3, 64, 127, 1024, 321};
  for (int l : interesting_vlens) {
    if (!test_vaddx32(l)) {
      std::cout << "\033[31mFAILURE\033[0m: vaddx32 with vlen=" << l << "\n";
    }
  }
}
