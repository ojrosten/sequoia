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
    test_resolve_to_copy_constructor();
  }

  void type_traits_test::test_variadic_traits()
  {
    {
      using traits = variadic_traits<>;

      check([]() {
          static_assert(!traits::size());
          return true;
        }()
      );

      check([]() {
          static_assert(std::is_same_v<traits::head, void>);
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

  void type_traits_test::test_resolve_to_copy_constructor()
  {
    {
      using d = resolve_to_copy_constructor<int>;
      
      check([]() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_constructor_t<int>>);
          return true;
        }
      );

      check([]() {
          static_assert(!resolve_to_copy_constructor_v<int>);
          return true;
        }
      );
    }

    {
      using d = resolve_to_copy_constructor<int, int>;
      
      check([]() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_constructor_t<int, int>>);
          return true;
        }
      );

      check([]() {
          static_assert(resolve_to_copy_constructor_v<int, int>);
          return true;
        }
      );
    }

    {
      using d = resolve_to_copy_constructor<int&, int>;
      
      check([]() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_constructor_t<int&, int>>);
          return true;
        }
      );

      check([]() {
          static_assert(resolve_to_copy_constructor_v<int&, int>);
          return true;
        }
      );
    }

    {
      using d = resolve_to_copy_constructor<int, int&>;
      
      check([]() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_constructor_t<int, int&>>);
          return true;
        }
      );

      check([]() {
          static_assert(resolve_to_copy_constructor_v<int, int&>);
          return true;
        }
      );
    }

    {
      using d = resolve_to_copy_constructor<const int&, volatile int&>;
      
      check([]() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_constructor_t<const int&, volatile int&>>);
          return true;
        }
      );

      check([]() {
          static_assert(resolve_to_copy_constructor_v<const int&, volatile int&>);
          return true;
        }
      );
    }

    {
      using d = resolve_to_copy_constructor<int, double>;
      
      check([]() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_constructor_t<int, double>>);
          return true;
        }
      );

      check([]() {
          static_assert(!resolve_to_copy_constructor_v<int, double>);
          return true;
        }
      );
    }

    {
      using d = resolve_to_copy_constructor<int, int, int>;
      
      check([]() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }
      );

      check([]() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_constructor_t<int, int, double>>);
          return true;
        }
      );

      check([]() {
          static_assert(!resolve_to_copy_constructor_v<int, int, int>);
          return true;
        }
      );
    }
  }
}
