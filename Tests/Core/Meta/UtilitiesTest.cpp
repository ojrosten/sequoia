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

  template<class T>
  struct identity { using type = T;};

  double f(int) { return 1.0; }
  double g(int) noexcept { return 1.0; }

  [[nodiscard]]
  std::filesystem::path utilities_test::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void utilities_test::run_tests()
  {
    test_filtered_sequence();
  }

  void utilities_test::test_filtered_sequence()
  {
    check(report_line("One element which survives"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<void, identity, int>, std::index_sequence<0>>);

        return true;
      }()
    );

    check(report_line("One element which is filtered"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, identity, int>, std::index_sequence<>>);

        return true;
      }()
    );

    check(report_line("Two elements, both of which survive"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<void, identity, int, double>, std::index_sequence<0, 1>>);

        return true;
      }()
    );

    check(report_line("Two elements, the zeroth of which survives"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<double, identity, int, double>, std::index_sequence<0>>);

        return true;
      }()
    );

    check(report_line("Two elements, the first of which survives"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, identity, int, double>, std::index_sequence<1>>);

        return true;
      }()
    );

    check(report_line("Two elements, both of which are filtered"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, identity, int, int>, std::index_sequence<>>);

        return true;
      }()
    );
  }

  void utilities_test::test_function_signature()
  {
    check(report_line("Signature of lambda operator()"), []() {
        auto l{[](int) -> double { return 1.0; }};
        using clo = decltype(l);
        using sig = function_signature<decltype(&clo::operator())>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);

        return true;
      }()
    );

    check(report_line("Signature of struct operator() noexcept"), []() {
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

    check(report_line("Signature of struct operator() noexcept"), []() {
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

    check(report_line("Signature of struct operator() const noexcept"), []() {
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

    check(report_line("Signature of struct static function noexcept"), []() {
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

    check(report_line("Signature of function"), []() {
        using sig = function_signature<decltype(&f)>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);

        return true;
      }()
    );

    check(report_line("Signature of noexcept function"), []() {
        using sig = function_signature<decltype(&g)>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);

        return true;
      }()
    );
  }
}
