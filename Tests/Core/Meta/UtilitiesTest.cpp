////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UtilitiesTest.hpp"

import std;
import sequoia.core.meta;

namespace sequoia::testing
{
  namespace
  {
    double plain_function(int) { return 1.0; }
    double noexcept_function(int) noexcept { return 1.0; }

    struct fn_ob {
      int i{};
      double x{};

      void operator()(int val)    { i = val; }
      void operator()(double val) { x = val; }

      friend bool operator==(const fn_ob&, const fn_ob&) noexcept = default;
    };
  }

  [[nodiscard]]
  std::filesystem::path utilities_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void utilities_test::run_tests()
  {
    test_function_signature();
    test_for_each();
  }

  void utilities_test::test_function_signature()
  {
    {
      auto l{[](int) -> double { return 1.0; }};
      using clo = decltype(l);
      using sig = function_signature<decltype(&clo::operator())>;
      STATIC_CHECK(std::is_same_v<sig::arg, int>);
      STATIC_CHECK(std::is_same_v<sig::ret, double>);
    }

    {
      struct foo
      {
        double operator()(int) { return 1.0; }
      };

      using sig = function_signature<decltype(&foo::operator())>;
      STATIC_CHECK(std::is_same_v<sig::arg, int>);
      STATIC_CHECK(std::is_same_v<sig::ret, double>);
    }

    {
      struct foo
      {
        double operator()(int) noexcept { return 1.0; }
      };

      using sig = function_signature<decltype(&foo::operator())>;
      STATIC_CHECK(std::is_same_v<sig::arg, int>);
      STATIC_CHECK(std::is_same_v<sig::ret, double>);
    }

    {
      struct foo
      {
        double operator()(int) const noexcept { return 1.0; }
      };

      using sig = function_signature<decltype(&foo::operator())>;
      STATIC_CHECK(std::is_same_v<sig::arg, int>);
      STATIC_CHECK(std::is_same_v<sig::ret, double>);
    }

    {
      struct foo
      {
        static double bar(int) noexcept { return 1.0; }
      };

      using sig = function_signature<decltype(&foo::bar)>;
      STATIC_CHECK(std::is_same_v<sig::arg, int>);
      STATIC_CHECK(std::is_same_v<sig::ret, double>);
    }

    {
      using sig = function_signature<decltype(&plain_function)>;
      STATIC_CHECK(std::is_same_v<sig::arg, int>);
      STATIC_CHECK(std::is_same_v<sig::ret, double>);
    }

    {
      using sig = function_signature<decltype(&noexcept_function)>;
      STATIC_CHECK(std::is_same_v<sig::arg, int>);
      STATIC_CHECK(std::is_same_v<sig::ret, double>);
    }
  }

  void utilities_test::test_for_each()
  {
    {
      fn_ob f{};  
      meta::for_each(std::tuple<int>{42}, f);
      check("", f == fn_ob{42, 0});
    }

    {
      fn_ob f{};  
      meta::for_each(std::tuple<double>{3.14}, f);
      check("", f == fn_ob{0, 3.14});
    }
    
    {
      fn_ob f{};  
      meta::for_each(std::tuple<int, double>{42, 3.14}, f);
      check("", f == fn_ob{42, 3.14});
    }
  }
}
