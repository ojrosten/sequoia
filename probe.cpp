// What OutputFreeTest's line 525 does to -inf, and the classification steps
// it relies on, printed so that a native run and a run under valgrind can be
// compared line by line.
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <format>
#include <limits>
#include <string>

namespace
{
  // As Output.cpp's from_bit_pattern: x87 -inf's ten bytes, little-endian,
  // zero-padded to sizeof(long double), then bit_cast.
  long double from_bytes()
  {
    std::array<std::byte, sizeof(long double)> bytes{};
    const unsigned char pattern[10]{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff};
    for(std::size_t i{}; i < 10; ++i) bytes[i] = std::byte{pattern[i]};
    return std::bit_cast<long double>(bytes);
  }

  std::string hex(long double v)
  {
    const auto bytes{std::bit_cast<std::array<unsigned char, sizeof(long double)>>(v)};
    std::string s;
    for(std::size_t i{10}; i-- > 0;) s += std::format("{:02x}", bytes[i]);
    return s;
  }
}

int main()
{
  const long double x{from_bytes()};
  volatile long double maxSource{std::numeric_limits<long double>::max()};
  const long double ldblMax{maxSource};
  volatile long double infSource{std::numeric_limits<long double>::infinity()};
  const long double inf{infSource};

  std::printf("bits of x:                     %s\n", hex(x).c_str());
  std::printf("std::isinf(x):                 %d\n", std::isinf(x) ? 1 : 0);
  std::printf("std::isfinite(x):              %d\n", std::isfinite(x) ? 1 : 0);
  std::printf("std::fpclassify(x)==FP_INFINITE: %d\n", std::fpclassify(x) == FP_INFINITE ? 1 : 0);
  std::printf("std::fabs(x) > LDBL_MAX:       %d\n", std::fabs(x) > ldblMax ? 1 : 0);
  std::printf("LDBL_MAX == inf:               %d\n", ldblMax == inf ? 1 : 0);
  std::printf("bits of LDBL_MAX:              %s\n", hex(ldblMax).c_str());
  std::printf("format {:.6f} of x:            %s\n", std::format("{:.6f}", x).substr(0, 40).c_str());
  std::printf("format {} of x:                %s\n", std::format("{}", x).substr(0, 40).c_str());
  std::printf("format {:.6f} of 1.5L:         %s\n", std::format("{:.6f}", 1.5L).c_str());
  std::printf("format {:.6f} of LDBL_MAX:     %s\n", std::format("{:.6f}", ldblMax).substr(0, 40).c_str());
  std::printf("printf %%.6Lf of x:             %.6Lf\n", x);
}
