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
  namespace
  {
    struct foo { int x{}; };
  }
  
  [[nodiscard]]
  std::filesystem::path type_traits_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void type_traits_test::run_tests()
  {
    test_resolve_to_copy();
    test_is_const_pointer();
    test_is_const_reference();
    test_is_tuple();
    test_is_initializable();
    test_has_allocator_type();
    test_is_compatible();
    test_are_same();
  }

  void type_traits_test::test_resolve_to_copy()
  {
    {
      using d = resolve_to_copy<int>;

      check("", []() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }()
      );

      check("", []() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_t<int>>);
          return true;
        }()
      );

      check("", []() {
          static_assert(!resolve_to_copy_v<int>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<int, int>;

      check("", []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check("", []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_t<int, int>>);
          return true;
        }()
      );

      check("", []() {
          static_assert(resolve_to_copy_v<int, int>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<int&, int>;

      check("", []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check("", []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_t<int&, int>>);
          return true;
        }()
      );

      check("", []() {
          static_assert(resolve_to_copy_v<int&, int>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<int, int&>;

      check("", []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check("", []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_t<int, int&>>);
          return true;
        }()
      );

      check("", []() {
          static_assert(resolve_to_copy_v<int, int&>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<const int&, volatile int&>;

      check("", []() {
          static_assert(std::is_same_v<std::true_type, d::type>);
          return true;
        }()
      );

      check("", []() {
          static_assert(std::is_same_v<std::true_type, resolve_to_copy_t<const int&, volatile int&>>);
          return true;
        }()
      );

      check("", []() {
          static_assert(resolve_to_copy_v<const int&, volatile int&>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<int, double>;

      check("", []() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }()
      );

      check("", []() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_t<int, double>>);
          return true;
        }()
      );

      check("", []() {
          static_assert(!resolve_to_copy_v<int, double>);
          return true;
        }()
      );
    }

    {
      using d = resolve_to_copy<int, int, int>;

      check("", []() {
          static_assert(std::is_same_v<std::false_type, d::type>);
          return true;
        }()
      );

      check("", []() {
          static_assert(std::is_same_v<std::false_type, resolve_to_copy_t<int, int, double>>);
          return true;
        }()
      );

      check("", []() {
          static_assert(!resolve_to_copy_v<int, int, int>);
          return true;
        }()
      );
    }
  }



  void type_traits_test::test_is_const_pointer()
  {

    check("", []() {
        static_assert(std::is_same_v<std::true_type, is_const_pointer_t<const int*>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(is_const_pointer_v<const int*>);
        return true;
      }()
    );

    check("", []() {
        static_assert(std::is_same_v<std::false_type, is_const_pointer_t<int*>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(!is_const_pointer_v<int*>);
        return true;
      }()
    );

    check("", []() {
        static_assert(std::is_same_v<std::false_type, is_const_pointer_t<int* const>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(!is_const_pointer_v<int* const>);
        return true;
      }()
    );

    check("", []() {
        static_assert(std::is_same_v<std::true_type, is_const_pointer_t<const int* const>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(is_const_pointer_v<const int* const>);
        return true;
      }()
    );
  }

  void type_traits_test::test_is_const_reference()
  {
    check("", []() {
        static_assert(std::is_same_v<std::true_type, is_const_reference_t<const int&>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(is_const_reference_v<const int&>);
        return true;
      }()
    );

    check("", []() {
        static_assert(std::is_same_v<std::true_type, is_const_reference_t<const volatile int&>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(is_const_reference_v<const volatile int&>);
        return true;
      }()
    );

    check("", []() {
        static_assert(std::is_same_v<std::false_type, is_const_reference_t<int&>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(!is_const_reference_v<int&>);
        return true;
      }()
    );
  }

  void type_traits_test::test_is_initializable()
  {
    check("", []() {
        static_assert(std::is_same_v<std::false_type, is_initializable_t<foo, std::vector<int>>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(!is_initializable_v<foo, std::vector<int>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(std::is_same_v<std::true_type, is_initializable_t<foo, int>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(is_initializable_v<foo, int>);
        return true;
      }()
    );
  }

  void type_traits_test::test_is_tuple()
  {
    check("", []() {
        static_assert(!is_tuple_v<int>);
        static_assert(std::is_same_v<std::false_type, is_tuple_t<int>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(is_tuple_v<std::tuple<>>);
        static_assert(std::is_same_v<std::true_type, is_tuple_t<std::tuple<>>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(is_tuple_v<std::tuple<int>>);
        static_assert(std::is_same_v<std::true_type, is_tuple_t<std::tuple<int>>>);
        return true;
      }()
    );
  }

  void type_traits_test::test_has_allocator_type()
  {
    check("", []() {
        static_assert(has_allocator_type_v<std::vector<double>>);
        static_assert(std::is_same_v<std::true_type, has_allocator_type_t<std::vector<double>>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(!has_allocator_type_v<double>);
        static_assert(std::is_same_v<std::false_type, has_allocator_type_t<double>>);
        return true;
      }()
    );
  }

  void type_traits_test::test_is_compatible()
  {
    check("", []() {
        static_assert(is_compatible_v<double, float>);
        static_assert(std::is_same_v<std::true_type, is_compatible_t<double, float>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(is_compatible_v<double, double>);
        static_assert(std::is_same_v<std::true_type, is_compatible_t<double, double>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(is_compatible_v<double, int>);
        static_assert(std::is_same_v<std::true_type, is_compatible_t<double, int>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(!is_compatible_v<float, double>);
        static_assert(std::is_same_v<std::false_type, is_compatible_t<float, double>>);
        return true;
      }()
    );

    check("", []() {
        static_assert(is_compatible_v<float, float>);
        static_assert(std::is_same_v<std::true_type, is_compatible_t<float, float>>);
        return true;
      }()
    );

    check("", []() {
      static_assert(is_compatible_v<float, int>);
      static_assert(std::is_same_v<std::true_type, is_compatible_t<float, int>>);
      return true;
      }()
    );

    check("", []() {
      static_assert(has_allocator_type_v<std::vector<double>>);
      static_assert(std::is_same_v<std::true_type, has_allocator_type_t<std::vector<double>>>);
      return true;
      }()
    );

    check("", []() {
      static_assert(!has_allocator_type_v<double>);
      static_assert(std::is_same_v<std::false_type, has_allocator_type_t<double>>);
      return true;
      }()
    );
  }

  void type_traits_test::test_are_same()
  {
    STATIC_CHECK(are_same_v<int>);
    STATIC_CHECK(std::same_as<are_same_t<int>, std::true_type>);

    STATIC_CHECK(are_same_v<float>);
    STATIC_CHECK(are_same_v<int, int>);
    STATIC_CHECK(std::same_as<are_same_t<int, int>, std::true_type>);

    STATIC_CHECK(are_same_v<float, float>);
    STATIC_CHECK(!are_same_v<int, float>);
    STATIC_CHECK(std::same_as<are_same_t<int, float>, std::false_type>);
  }
}
