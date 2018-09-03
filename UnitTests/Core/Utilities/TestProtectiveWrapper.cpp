#include "TestProtectiveWrapper.hpp"
#include "ProtectiveWrapper.hpp"

#include <complex>
#include <vector>

namespace sequoia
{
  namespace unit_testing
  {
    void test_protective_wrapper::run_tests()
    {
      using namespace utilities;

      using std::complex;

      protective_wrapper<double> x(1.0);
      check_equality<double>(1.0, x.get(), "Value is 1.0");
      x.set(2.0);
      check_equality<double>(2.0, x.get(), "Value is 2.0");
      constexpr protective_wrapper<int> i(10);
      check_equality<int>(10, i.get(), "Integer value is 10");

      constexpr protective_wrapper<int> j{invert(i)};
      check_equality<int>(-10, j.get(), "Integer value is -10");
      
      protective_wrapper<complex<float>>
        z(2.0f),
        w(0.5f, 1.0f);

      check_equality<complex<float>>(complex<float>(0.5, 1.0), w.get(), "Value is (0.5,1.0i)");
      check_equality<complex<float>>(complex<float>(2.0), z.get(), "Value is (2.0, 0i)");

      check(z != w, "Complex values diffent");

      swap(z, w);

      check_equality<complex<float>>(complex<float>(2.0), w.get(), "Values swapped so first value is now (2.0, 0i)");
      check_equality<complex<float>>(complex<float>(0.5, 1.0), z.get(), "Values swapped so second value is now (0.5, 1.0i)");

      w.set(0.5f, 1.0f);

      check(z == w, "Complex weights now identical");

      complex<float> lval{-1.0f, -2.4f};
      w.set(lval);

      check_equality<complex<float>>(lval, w.get(), "w is now (-1.0, -2.4i)");

      protective_wrapper<complex<float>> u(z);

      check_equality<complex<float>>(complex<float>(0.5, 1.0), u.get(), "New variable set to (0.5, 1.0i)");

      swap(u, w);

      check_equality<complex<float>>(complex<float>(-1.0f, -2.4f), u.get(), "New variable swapped with old, so value is (-1.0, -2.4i)");
      check_equality<complex<float>>(complex<float>(0.5f, 1.0f), w.get(), "Old variable swapped with new so value is (0.5, 1.0i)");

      u = z;

      check_equality<complex<float>>(complex<float>(0.5, 1.0), u.get(), "variable u set to z");

      u.mutate([](auto& c) { c.imag(2.0); });
      check_equality<complex<float>>(complex<float>(0.5, 2.0), u.get(), "imaginary part of u set to 2.0");

      constexpr protective_wrapper<complex<int>> p{10,-5};
      check_equality(complex<int>(10, -5), p.get(), "p has value (10, -5)");

      protective_wrapper<std::vector<double>> a{1.0, 2.0};
      check_equality(std::vector<double>{1,2}, a.get());

      protective_wrapper<std::vector<double>> b{10.0,4.0};
      check_equality(std::vector<double>{10,4}, b.get());

      swap(a,b);

      check_equality(std::vector<double>{1,2}, b.get());
      check_equality(std::vector<double>{10,4}, a.get());

      constexpr protective_wrapper<point<double>> pt{-1.0, 1.0};
      check_equality<double>(-1, pt.get().x());
      check_equality<double>(1, pt.get().y());

      constexpr protective_wrapper<point<double>> pt2{scale(pt, 2.0)};
      check_equality<double>(-2, pt2.get().x());
      check_equality<double>(2, pt2.get().y());
    }
  }
}
