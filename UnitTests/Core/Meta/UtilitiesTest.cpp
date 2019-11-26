////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UtilitiesTest.hpp"

#include "Utilities.hpp"

namespace sequoia::unit_testing
{
  
  template<class T>
  struct identity { using type = T;};

  double f(int) { return 1.0; }
  double g(int) noexcept { return 1.0; }

  void utilities_test::run_tests()
  {
    test_filtered_sequence();
  }

  void utilities_test::test_filtered_sequence()
  {
    check(LINE("One element which survives"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<void, identity, int>, std::index_sequence<0>>);
    
        return true;
      }()
    );

    check(LINE("One element which is filtered"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, identity, int>, std::index_sequence<>>);
    
        return true;
      }()
    );
    
    check(LINE("Two elements, both of which survive"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<void, identity, int, double>, std::index_sequence<0, 1>>);
    
        return true;
      }()
    );

    check(LINE("Two elements, the zeroth of which survives"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<double, identity, int, double>, std::index_sequence<0>>);
    
        return true;
      }()
    );

    check(LINE("Two elements, the first of which survives"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, identity, int, double>, std::index_sequence<1>>);
    
        return true;
      }()
    );

    check(LINE("Two elements, both of which are filtered"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, identity, int, int>, std::index_sequence<>>);
    
        return true;
      }()
    );
  }

  void utilities_test::test_function_signature()
  {
    check(LINE("Signature of lambda operator()"), []() {
        auto l{[](int) -> double { return 1.0; }};
        using clo = decltype(l);
        using sig = function_signature<decltype(&clo::operator())>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);
    
        return true;
      }()
    );

    check(LINE("Signature of struct operator() noexcept"), []() {
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

    check(LINE("Signature of struct operator() noexcept"), []() {
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

    check(LINE("Signature of struct operator() const noexcept"), []() {
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

    check(LINE("Signature of struct static function noexcept"), []() {
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

    check(LINE("Signature of function"), []() {
        using sig = function_signature<decltype(&f)>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);
    
        return true;
      }()
    );

    check(LINE("Signature of noexcept function"), []() {
        using sig = function_signature<decltype(&g)>;
        static_assert(std::is_same_v<sig::arg, int>);
        static_assert(std::is_same_v<sig::ret, double>);
    
        return true;
      }()
    );
  }
}
