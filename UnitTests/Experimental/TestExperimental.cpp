#include "TestExperimental.hpp"
#include "Algorithms.hpp"

#include <iomanip>
#include <cfenv>
#include <array>

namespace sequoia
{
  namespace unit_testing
  {
    constexpr int foo(float x) {
        int a{}; int* pa = &a;
        frexp(x, pa);
        return a;
    }
    
    void test_experimental::run_tests()
    {      
      /*constexpr double y{0};
      constexpr double* const py = &y;

      constexpr double z{modf(y, py)};

      check_equality<double>(1,z);
      */

      //constexpr auto z{foo(1.0f)};
      
      /*

      std::fesetround(FE_DOWNWARD);
      //std::fesetround(FE_UPWARD);
      constexpr double u{1.0/10.0};
      double v{1.0};
      v /= 10.0;
      const double w{1.0/10.0};
      constexpr double t{divby10(1.0)};
      const double s{divby10B(1.0)};
      std::cout << "\nu-v: " << u - v << '\n';
      std::cout << "\nu-w: " << u - w << '\n';
      std::cout << "\nu-t: " << u - t << '\n';
      std::cout << "\nu-s: " << u - s << '\n';
      std::cout << "\nw-v: " << w - v << '\n';
      */
      
      /*
      std::cout.precision(20);

      std::fesetround(FE_UPWARD);

      constexpr double foo{1.0/3.0};
      const double foo1{1.0/3.0};
      double foo2{1.0};
      foo2 /= 10.0;
      std::cout << "\nfoo: " << foo << '\n';
      std::cout << "\nfoo1: " << foo1 << '\n';
      std::cout << "\nfoo2: " << foo2 << '\n';
      
      std::fesetround(FE_DOWNWARD);

      constexpr double bar{1.0/3.0};
      const double bar1{1.0/3.0};
      double bar2{1.0};
      bar2/= 10.0;
      std::cout << "foo: " << foo << '\n';
      std::cout << "bar: " << bar << '\n';
      std::cout << "bar1: " << bar1 << '\n';
      std::cout << "\nbar2: " << bar2 << '\n';
   
      std::cout << "foo1 - bar1: " << foo1 - bar1 << '\n';
      std::cout << "foo2 - bar2: " << foo2 - bar2 << '\n';
      
      std::fesetround(FE_UPWARD);
      std::cout << "foo: " << foo << '\n';
      std::cout << "bar: " << bar << '\n';
      */
      
      
        /*
      constexpr double foo{std::numeric_limits<double>::max()+1};
      constexpr double rnd{1.0/3.0};
      constexpr int rnd2{1/3};
      //constexpr int bar{std::numeric_limits<int>::max()+1};
      //constexpr double a{std::exp(3.0)};

      constexpr double b{std::numeric_limits<double>::epsilon()*0.1};
      */

      /*
      constexpr size_t bar{std::numeric_limits<size_t>::max()+1};
      check_equality<size_t>(std::numeric_limits<size_t>::max(), bar);

      constexpr double foo{std::numeric_limits<double>::max()+1};
      check_equality<double>(std::numeric_limits<double>::max(), foo);
      */

      /*
      constexpr float small{std::numeric_limits<float>::min()};
      constexpr float tiny{small/10.0};
      constexpr float tootiny{small/1e7};
      std::cout << "small " << small << '\n';
      std::cout << "tiny " << tiny << '\n';
      std::cout << "too tiny " << tootiny << '\n';
      std::cout << "tiniest " << std::numeric_limits<float>::denorm_min() << '\n';
      */

      static_assert(std::is_same_v<thing<int, component<int>>, thing<int, alias<int>>>);
      //static_assert(std::is_same_v<thing2<int, component>, thing2<int, alias>>);

      int x{5};
      wrapper<int> w{x}; // fine
      //thing3<int, wrapper<int>> foo{x}; // fails due to exception spec of move assignment operator not matching the calculated one

      
    }
  }
}
