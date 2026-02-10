#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <print>
#include <random>

static std::mt19937 mt(time(nullptr));

extern "C" {
void vaddx32(int32_t* a, int32_t s, size_t vlen);
void saxpyf32(float *res, const float* x, const float* y, float s, size_t vlen);
}

void vaddx32_ref(int32_t* a, int32_t s, size_t vlen) {
  for (int i = 0; i < vlen; i++)
    a[i] += s;
}

void saxpyf32_ref(float* res, const float* x, const float* y, float s, size_t vlen) {
  for (int i = 0; i < vlen; i++)
    res[i] = s * x[i] + y[i];
}

template <typename T>
bool checkbuf(T* buf, T* ref, size_t len) {
  for (int i = 0; i < len; i++) {
    if (buf[i] != ref[i]) {
      std::cout << "\033[31merror\033[0m: expected " << ref[i] << " but got "
                << buf[i];
      std::cout << " (i=" << i << ")\n";
      return false;
    }
  }
  return true;
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

  if (checkbuf(buf.get(), ref_buf.get(), vlen)) {
    std::println("\033[32mSUCCESS\033[0m: {} with vlen={}", __FUNCTION__, vlen);
    return true;
  } else {
    std::println("\033[31mFAILURE\033[0m: {} with vlen={}", __FUNCTION__, vlen);
    return false;
  }
}

bool test_saxpyf32(size_t vlen) {
  std::unique_ptr<float[]> ref_res{new float[vlen]};
  std::unique_ptr<float[]> res{new float[vlen]};
  std::unique_ptr<float[]> x{new float[vlen]};
  std::unique_ptr<float[]> y{new float[vlen]};

  float s = mt();

  for (int i = 0; i < vlen; i++) {
    x[i] = mt();
    y[i] = mt();
  }

  saxpyf32_ref(ref_res.get(), x.get(), y.get(), s, vlen);
  saxpyf32(res.get(), x.get(), y.get(), s, vlen);

  if (checkbuf(res.get(), ref_res.get(), vlen)) {
    std::println("\033[32mSUCCESS\033[0m: {} with vlen={}", __FUNCTION__, vlen);
    return true;
  } else {
    std::println("\033[31mFAILURE\033[0m: {} with vlen={}", __FUNCTION__, vlen);
    return false;
  }
}

int main(void) {
  std::vector<size_t> vlens(30);
  for (auto& vl : vlens)
    vl = mt() % 4096;

  for (size_t l : vlens)
    test_vaddx32(l);
  std::cout << "\n";

  for (size_t vl : vlens)
    test_saxpyf32(vl);
}
