////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "UtilitiesTest.hpp"

#include "sequoia/Core/Meta/Utilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    double f(int) { return 1.0; }
    double g(int) noexcept { return 1.0; }

    struct fn_ob {
      int i{};
      double x{};

      void operator()(int val)    { i = val; }
      void operator()(double val) { x = val; }

      friend bool operator==(const fn_ob&, const fn_ob&) noexcept = default;
    };
  }

  [[nodiscard]]
  std::filesystem::path utilities_test::source_file() const
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
    check("Signature of lambda operator()", []() {
        auto l{[](int) -> double { return 1.0; }};
        using clo = decltype(l);
        using sig = function_signature<decltype(&clo::operator())>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);

        return true;
      }()
    );

    check("Signature of struct operator() noexcept", []() {
        struct foo
        {
          double operator()(int) { return 1.0; }
        };

        using sig = function_signature<decltype(&foo::operator())>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);

        return true;
      }()
    );

    check("Signature of struct operator() noexcept", []() {
        struct foo
        {
          double operator()(int) noexcept { return 1.0; }
        };

        using sig = function_signature<decltype(&foo::operator())>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);

        return true;
      }()
    );

    check("Signature of struct operator() const noexcept", []() {
        struct foo
        {
          double operator()(int) const noexcept { return 1.0; }
        };

        using sig = function_signature<decltype(&foo::operator())>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);

        return true;
      }()
    );

    check("Signature of struct static function noexcept", []() {
        struct foo
        {
          static double bar(int) noexcept { return 1.0; }
        };

        using sig = function_signature<decltype(&foo::bar)>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);

        return true;
      }()
    );

    check("Signature of function", []() {
        using sig = function_signature<decltype(&f)>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);

        return true;
      }()
    );

    check("Signature of noexcept function", []() {
        using sig = function_signature<decltype(&g)>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);

        return true;
      }()
    );
  }

  void utilities_test::test_for_each()
  {
    {
      fn_ob f{};  
      for_each(std::tuple<int>{42}, f);
      check("", f == fn_ob{42, 0});
    }

    {
      fn_ob f{};  
      for_each(std::tuple<double>{3.14}, f);
      check("", f == fn_ob{0, 3.14});
    }
    
    {
      fn_ob f{};  
      for_each(std::tuple<int, double>{42, 3.14}, f);
      check("", f == fn_ob{42, 3.14});
    }
  }
}
