////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TypeTraitsTest.hpp"

#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <array>
#include <complex>
#include <map>
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
    test_type_list();
    test_base_of_head();
    test_resolve_to_copy();
    test_is_const_pointer();
    test_is_const_reference();
    test_is_tuple();
    test_has_allocator_type();
  }

  void type_traits_test::test_type_list()
  {
    {
      using typeList = type_list<>;

      check(LINE(""), []() {
          static_assert(std::is_same_v<head_of_t<>, void>);
          static_assert(std::is_same_v<head_of_t<typeList>, void>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<tail_of_t<>,         type_list<>>);
          static_assert(std::is_same_v<tail_of_t<typeList>, type_list<>>);
          return true;
        }()
      );
    }

    {
      using typeList = type_list<int>;

      check(LINE(""), []() {
          static_assert(std::is_same_v<head_of_t<int>, int>);
          static_assert(std::is_same_v<head_of_t<typeList>, int>);
          return true;
        }()
      );

      check(LINE(""), []() {
        static_assert(std::is_same_v<tail_of_t<int>, type_list<>>);
          static_assert(std::is_same_v<tail_of_t<typeList>, type_list<>>);
          return true;
        }()
      );
    }

    {
      using typeList = type_list<int, double>;

      check(LINE(""), []() {
          static_assert(std::is_same_v<head_of_t<int, double>, int>);
          static_assert(std::is_same_v<head_of_t<typeList>, int>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<tail_of_t<int, double>, type_list<double>>);
          static_assert(std::is_same_v<tail_of_t<typeList>, type_list<double>>);
          return true;
        }()
      );
    }

    {
      using typeList = type_list<int, double, char>;

      check(LINE(""), []() {
          static_assert(std::is_same_v<head_of_t<int, double, char>, int>);
          static_assert(std::is_same_v<head_of_t<typeList>, int>);
          return true;
        }()
      );

      check(LINE(""), []() {
          static_assert(std::is_same_v<tail_of_t<int, double, char>, type_list<double, char>>);
          static_assert(std::is_same_v<tail_of_t<typeList>, type_list<double, char>>);
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

  void type_traits_test::test_has_allocator_type()
  {
    check(LINE(""), []() {
      static_assert(has_allocator_type_v<std::vector<double>>);
      static_assert(std::is_same_v<std::true_type, has_allocator_type_t<std::vector<double>>>);
      return true;
      }()
    );

    check(LINE(""), []() {
      static_assert(!has_allocator_type_v<double>);
      static_assert(std::is_same_v<std::false_type, has_allocator_type_t<double>>);
      return true;
      }()
    );
  }
}
