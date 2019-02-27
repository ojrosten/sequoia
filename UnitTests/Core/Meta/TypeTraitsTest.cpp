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
    test_base_of_head();
    test_resolve_to_copy_constructor();
    test_is_const_pointer();
    test_is_const_reference();
    test_has_member_type();
    test_is_orderable();
    test_is_equal_to_comparable();
    test_is_not_equal_to_comparable();
    test_has_default_constructor();
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

  void type_traits_test::test_base_of_head()
  {
    check([]() {
        static_assert(std::is_same_v<std::false_type, is_base_of_head_t<int, double>>);
        return true;
      }
    );

    check([]() {
        static_assert(!is_base_of_head_v<int, double>);
        return true;
      }
    );

    check([]() {
        static_assert(std::is_same_v<std::true_type, is_base_of_head_t<std::basic_iostream<char>, std::stringstream>>);
        return true;
      }
    );

    check([]() {
        static_assert(is_base_of_head_v<std::basic_iostream<char>, std::stringstream>);
        return true;
      }
    );

    check([]() {
        static_assert(std::is_same_v<std::true_type, is_base_of_head_t<std::basic_iostream<char>, std::stringstream, double>>);
        return true;
      }
    );

    check([]() {
        static_assert(is_base_of_head_v<std::basic_iostream<char>, std::stringstream, double>);
        return true;
      }
    );    

    check([]() {
        static_assert(std::is_same_v<std::false_type, is_base_of_head_t<std::basic_iostream<char>, double, std::stringstream>>);
        return true;
      }
    );

    check([]() {
        static_assert(!is_base_of_head_v<std::basic_iostream<char>, double, std::stringstream>);
        return true;
      }
    );

    check([]() {
        static_assert(std::is_same_v<std::false_type, is_base_of_head_t<std::stringstream, std::basic_iostream<char>>>);
        return true;
      }
    );

    check([]() {
        static_assert(!is_base_of_head_v<std::stringstream, std::basic_iostream<char>>);
        return true;
      }
    );
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

  

  void type_traits_test::test_is_const_pointer()
  {

    check([]() {
        static_assert(std::is_same_v<std::true_type, is_const_pointer_t<const int*>>);
        return true;
      }
    );

    check([]() {
        static_assert(is_const_pointer_v<const int*>);
        return true;
      }
    );

    check([]() {
        static_assert(std::is_same_v<std::false_type, is_const_pointer_t<int*>>);
        return true;
      }
    );

    check([]() {
        static_assert(!is_const_pointer_v<int*>);
        return true;
      }
    );

    check([]() {
        static_assert(std::is_same_v<std::false_type, is_const_pointer_t<int* const>>);
        return true;
      }
    );

    check([]() {
        static_assert(!is_const_pointer_v<int* const>);
        return true;
      }
    );

    check([]() {
        static_assert(std::is_same_v<std::true_type, is_const_pointer_t<const int* const>>);
        return true;
      }
    );

    check([]() {
        static_assert(is_const_pointer_v<const int* const>);
        return true;
      }
    );
  }

  void type_traits_test::test_is_const_reference()
  {
  }

  void type_traits_test::test_has_member_type()
  {
  }

  void type_traits_test::test_is_orderable()
  {}

  void type_traits_test::test_is_equal_to_comparable()
  {}

  void type_traits_test::test_is_not_equal_to_comparable()
  {}

  void type_traits_test::test_has_default_constructor(){}
}
