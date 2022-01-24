////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TypeTraitsTest.hpp"

#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <array>
#include <complex>
#include <map>
#include <set>
#include <vector>

namespace sequoia::testing
{
  struct foo {};

  template<class T>
  inline constexpr bool has_value_type{requires { typename T::value_type; }};

  template<class T>
  struct deep_equality_comparable;

  template<class T, std::size_t... I>
  inline constexpr bool heterogeneous_deep_equality_v{
    requires(T & t, std::index_sequence<I...>) {
      requires (deep_equality_comparable<std::remove_cvref_t<decltype(std::get<I>(t))>>::value && ...);
    }
  };

  template<class T, std::size_t...I>
  constexpr bool has_heterogeneous_deep_equality(std::index_sequence<I...>)
  {
    return heterogeneous_deep_equality_v<T, I...>;
  }

  template<class T>
  struct heterogeneous_deep_equality;

  template<template<class...> class T, class... Ts>
  struct heterogeneous_deep_equality<T<Ts...>>
    : std::bool_constant<has_heterogeneous_deep_equality<T<Ts...>>(std::make_index_sequence<sizeof...(Ts)>{}) >
  {};

  template<class T>
  struct deep_equality_comparable : std::bool_constant<std::equality_comparable<T>>
  {};

  template<class T>
    requires has_value_type<T>
  struct deep_equality_comparable<T> : std::bool_constant<std::equality_comparable<T> && deep_equality_comparable<typename T::value_type>::value>
  {};

  template<template<class...> class T, class... Ts>
    requires (!has_value_type<T<Ts...>>)
  struct deep_equality_comparable<T<Ts...>> : std::bool_constant<std::equality_comparable<T<Ts...>> && heterogeneous_deep_equality<T<Ts...>>::value>
  {};

  template<class T>
  inline constexpr bool deep_equality_comparable_v{deep_equality_comparable<T>::value};


  static_assert(heterogeneous_deep_equality<std::variant<int, double>>::value);
  static_assert(heterogeneous_deep_equality<std::tuple<int, double>>::value);
  static_assert(heterogeneous_deep_equality<std::pair<int, double>>::value);
  static_assert(!heterogeneous_deep_equality<std::variant<int, foo>>::value);

  static_assert(deep_equality_comparable_v<int>);
  static_assert(deep_equality_comparable_v<std::vector<int>>);
  static_assert(deep_equality_comparable_v<std::array<int, 3>>);
  static_assert(deep_equality_comparable_v<std::tuple<int>>);
  static_assert(deep_equality_comparable_v<std::tuple<int, double>>);
  static_assert(deep_equality_comparable_v<std::variant<int, float>>);
  static_assert(deep_equality_comparable_v<std::map<int, double>>);
  static_assert(deep_equality_comparable_v<std::tuple<std::vector<int>, std::array<std::pair<int, float>, 2>>>);

  static_assert(!deep_equality_comparable_v<foo>);
  static_assert(!deep_equality_comparable_v<std::vector<foo>>);
  static_assert(!deep_equality_comparable_v<std::array<foo, 3>>);
  static_assert(!deep_equality_comparable_v<std::tuple<foo>>);
  static_assert(!deep_equality_comparable_v<std::tuple<foo, double>>);
  static_assert(!deep_equality_comparable_v<std::variant<int, foo>>);
  static_assert(!deep_equality_comparable_v<std::map<int, foo>>);
  static_assert(!deep_equality_comparable_v<std::tuple<std::vector<foo>, std::array<std::pair<int, float>, 2>>>);
  static_assert(!deep_equality_comparable_v<std::tuple<std::vector<int>, std::array<std::pair<foo, float>, 2>>>);
  static_assert(!deep_equality_comparable_v<std::tuple<std::vector<int>, std::array<std::pair<int, foo>, 2>>>);


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
