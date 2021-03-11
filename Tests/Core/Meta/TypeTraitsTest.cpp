////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TypeTraitsTest.hpp"

#include "sequoia/Core/Meta/TypeTraits.hpp"


#include <complex>
#include <set>
#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view type_traits_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void type_traits_test::run_tests()
  {
    test_variadic_traits();
    test_base_of_head();
    test_resolve_to_copy();
    test_is_const_pointer();
    test_is_const_reference();
    test_is_tuple();
  }

  void type_traits_test::test_variadic_traits()
  {
    {
      using traits = variadic_traits<>;

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
  
  void type_traits_test::test_resolve_to_copy()
  {
    {
      using d = resolve_to_copy<int>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_t<int>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(!resolve_to_copy_v<int>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<int, int>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_t<int, int>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(resolve_to_copy_v<int, int>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<int&, int>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_t<int&, int>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(resolve_to_copy_v<int&, int>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<int, int&>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_t<int, int&>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(resolve_to_copy_v<int, int&>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<const int&, volatile int&>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_t<const int&, volatile int&>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(resolve_to_copy_v<const int&, volatile int&>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<int, double>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_t<int, double>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(!resolve_to_copy_v<int, double>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<int, int, int>;
      
      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_t<int, int, double>>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(!resolve_to_copy_v<int, int, int>);
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

  void type_traits_test::test_is_tuple()
  {
    check(LINE(""), []() {
        static_assert(!is_tuple_v<int>);
        static_assert(std::is_same_v<std::false_type, is_tuple_t<int>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_tuple_v<std::tuple<>>);
        static_assert(std::is_same_v<std::true_type, is_tuple_t<std::tuple<>>>);
        return true;
      }()
    );

    check(LINE(""), []() {
        static_assert(is_tuple_v<std::tuple<int>>);
        static_assert(std::is_same_v<std::true_type, is_tuple_t<std::tuple<int>>>);
        return true;
      }()
    );
  }
}
