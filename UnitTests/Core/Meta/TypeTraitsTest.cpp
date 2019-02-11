////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TypeTraitsTest.hpp"

#include "TypeTraits.hpp"

namespace sequoia::unit_testing
{
  void type_traits_test::run_tests()
  {
    test_variadic_traits();
  }

  void type_traits_test::test_variadic_traits()
  {
    check([]() {
        static_assert(!variadic_traits<>::size());
        return true;
      }()
    );

    {
      using traits = variadic_traits<int>;
      check([]() {
          static_assert(traits::size() == 1);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<traits::head, int>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<traits::tail, void>);
          return true;
        }
      );
    }

    {
      using traits = variadic_traits<int, double>;
      check([]() {
          static_assert(traits::size() == 2);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<traits::head, int>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<traits::tail::head, double>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<traits::tail::tail, void>);
          return true;
        }
      );
    }

    {
      using traits = variadic_traits<int, double, char>;
      check([]() {
          static_assert(traits::size() == 3);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<traits::head, int>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<traits::tail::head, double>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<traits::tail::tail::head, char>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<traits::tail::tail::tail, void>);
          return true;
        }
      );
    }
  }
}
