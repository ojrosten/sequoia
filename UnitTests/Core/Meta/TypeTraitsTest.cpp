////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TypeTraitsTest.hpp"

#include "TypeTraits.hpp"

#include <complex>

namespace sequoia::unit_testing
{  
  void type_traits_test::run_tests()
  {
    test_variadic_traits();
    test_base_of_head();
    test_resolve_to_copy_constructor();
    test_is_const_pointer();
    test_is_const_reference();
    test_is_orderable();
    test_is_equal_to_comparable();
    test_is_not_equal_to_comparable();
    test_is_container();
    test_is_allocator();
    test_has_default_constructor();
    test_has_allocator_type();
  }

  void type_traits_test::test_variadic_traits()
  {
    {
      using traits = variadic_traits<>;

      check(LINE(""), []() {
          static_assert(!traits::size());
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::head, void>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::tail, void>);
          return true;
        }()
      );
    }

    {
      using traits = variadic_traits<int>;

      check(LINE(""), []() {
          static_assert(traits::size() == 1);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::head, int>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::tail, void>);
          return true;
        }()
      );
    }

    {
      using traits = variadic_traits<int, double>;
      
      check(LINE(""), []() {
          static_assert(traits::size() == 2);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::head, int>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::tail::head, double>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::tail::tail, void>);
          return true;
        }()
      );
    }

    {
      using traits = variadic_traits<int, double, char>;

      check(LINE(""), []() {
          static_assert(traits::size() == 3);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::head, int>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::tail::head, double>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::tail::tail::head, char>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<traits::tail::tail::tail, void>);
          return true;
        }()
      );
    }
  }

  void type_traits_test::test_base_of_head()
  {
    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_base_of_head_t<int, double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_base_of_head_v<int, double>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_base_of_head_t<std::basic_iostream<char>, std::stringstream>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_base_of_head_v<std::basic_iostream<char>, std::stringstream>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_base_of_head_t<std::basic_iostream<char>, std::stringstream, double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_base_of_head_v<std::basic_iostream<char>, std::stringstream, double>);
        return true;
      }()
    );    

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_base_of_head_t<std::basic_iostream<char>, double, std::stringstream>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_base_of_head_v<std::basic_iostream<char>, double, std::stringstream>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_base_of_head_t<std::stringstream, std::basic_iostream<char>>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_base_of_head_v<std::stringstream, std::basic_iostream<char>>);
        return true;
      }()
    );
  }
  
  void type_traits_test::test_resolve_to_copy_constructor()
  {
    {
      using d = resolve_to_copy_constructor<int>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_constructor_t<int>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(!resolve_to_copy_constructor_v<int>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy_constructor<int, int>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_constructor_t<int, int>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(resolve_to_copy_constructor_v<int, int>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy_constructor<int&, int>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_constructor_t<int&, int>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(resolve_to_copy_constructor_v<int&, int>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy_constructor<int, int&>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_constructor_t<int, int&>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(resolve_to_copy_constructor_v<int, int&>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy_constructor<const int&, volatile int&>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_constructor_t<const int&, volatile int&>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(resolve_to_copy_constructor_v<const int&, volatile int&>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy_constructor<int, double>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_constructor_t<int, double>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(!resolve_to_copy_constructor_v<int, double>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy_constructor<int, int, int>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_constructor_t<int, int, double>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(!resolve_to_copy_constructor_v<int, int, int>);
          return true;
        }()
      );
    }
  }

  

  void type_traits_test::test_is_const_pointer()
  {

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_const_pointer_t<const int*>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_const_pointer_v<const int*>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_const_pointer_t<int*>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_const_pointer_v<int*>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_const_pointer_t<int* const>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_const_pointer_v<int* const>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_const_pointer_t<const int* const>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_const_pointer_v<const int* const>);
        return true;
      }()
    );
  }

  void type_traits_test::test_is_const_reference()
  {
    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_const_reference_t<const int&>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_const_reference_v<const int&>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_const_reference_t<const volatile int&>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_const_reference_v<const volatile int&>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_const_reference_t<int&>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_const_reference_v<int&>);
        return true;
      }()
    );
  }

  void type_traits_test::test_is_orderable()
  {
    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_orderable_t<double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_orderable_v<double>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_orderable_t<std::set<double>>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_orderable_v<std::set<double>>);
        return true;
      }()
    );
    
    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_orderable_t<std::complex<double>>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_orderable_v<std::complex<double>>);
        return true;
      }()
    );
  }

  void type_traits_test::test_is_equal_to_comparable()
  {
    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_equal_to_comparable_t<double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_equal_to_comparable_v<double>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_equal_to_comparable_t<std::vector<double>>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_equal_to_comparable_v<std::vector<double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_equal_to_comparable_t<std::thread>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_equal_to_comparable_v<std::thread>);
        return true;
      }()
    );
  }

  void type_traits_test::test_is_not_equal_to_comparable()
  {
    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_not_equal_to_comparable_t<double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_not_equal_to_comparable_v<double>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_not_equal_to_comparable_t<std::vector<double>>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_not_equal_to_comparable_v<std::vector<double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_not_equal_to_comparable_t<std::thread>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_not_equal_to_comparable_v<std::thread>);
        return true;
      }()
    );
  }

  void type_traits_test::test_is_container()
  {
    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_container_t<std::vector<double>>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_container_v<std::vector<double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_container_t<double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_container_v<double>);
        return true;
      }()
    );
  }

  void type_traits_test::test_is_allocator()
  {
    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, is_allocator_t<int>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!is_allocator_v<int>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, is_allocator_t<std::allocator<int>>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_allocator_v<std::allocator<int>>);
        return true;
      }()
    );
  }  

  void type_traits_test::test_has_default_constructor()
  {
    struct protected_destructor
    {
      protected_destructor() = default;
    protected:
      ~protected_destructor() = default;
    };

    struct no_default_constructor
    {
      no_default_constructor(int) {}
    };
    
    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, has_default_constructor_t<std::vector<double>>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(has_default_constructor_v<std::vector<double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, has_default_constructor_t<protected_destructor>>);
        static_assert(std::is_same_v<std::false_type, typename std::is_constructible<protected_destructor>::type>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(has_default_constructor_v<protected_destructor>);
        static_assert(!std::is_constructible_v<protected_destructor>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, has_default_constructor_t<no_default_constructor>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!has_default_constructor_v<no_default_constructor>);
        return true;
      }()
    );
  }  

  void type_traits_test::test_has_allocator_type()
  {
    check(LINE(""), []() {
        static_assert(std::is_same_v<std::true_type, has_allocator_type_t<std::vector<double>>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(has_allocator_type_v<std::vector<double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(std::is_same_v<std::false_type, has_allocator_type_t<double>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(!has_allocator_type_v<double>);
        return true;
      }()
    );
  }
}
