////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TypeAlgorithmsFreeTest.hpp"
#include "sequoia/Core/Meta/TypeAlgorithms.hpp"

namespace sequoia::testing
{
  namespace
  {
    template<class T, class U>
    struct comparator : std::bool_constant<sizeof(T) < sizeof(U)> {};

    
    template<class T>
    struct is_int : std::is_same<T, int> {};
  }
  
  using namespace meta;
  
  [[nodiscard]]
  std::filesystem::path type_algorithms_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void type_algorithms_free_test::run_tests()
  {    
    test_type_comparator();

    test_lower_bound<std::tuple>();
    test_lower_bound<std::variant>();

    test_filter<std::tuple>();
    test_filter<std::variant>();
 
    test_drop<std::tuple>();
    test_drop<std::variant>();

    test_keep<std::tuple>();
    test_keep<std::variant>();

    test_merge<std::tuple>();
    test_merge<std::variant>();
 
    test_stable_sort<std::tuple>();
    test_stable_sort<std::variant>();

    test_find<std::tuple>();
    test_find<std::variant>();

    test_find_if<std::tuple>();
    test_find_if<std::variant>();

    test_contains<std::tuple>();
    test_contains<std::variant>();

    test_erase<std::tuple>();
    test_erase<std::variant>();

    test_insert<std::tuple>();
    test_insert<std::variant>();

    test_flatten<std::tuple>();
    test_flatten<std::variant>();

    test_concat<std::tuple>();
    test_concat<std::variant>();

    test_reverse<std::tuple>();
    test_reverse<std::variant>();
  }

  void type_algorithms_free_test::test_type_comparator()
  {
    STATIC_CHECK((type_comparator_v<char, void>));
    STATIC_CHECK((!type_comparator_v<int, char>));
  }
  
  template<template<class...> class TT>
  void type_algorithms_free_test::test_lower_bound()
  {
    STATIC_CHECK((lower_bound_v<TT<>,                                   int,    comparator> == 0));
    STATIC_CHECK((lower_bound_v<TT<int>,                                int,    comparator> == 0));
    STATIC_CHECK((lower_bound_v<TT<char>,                               int,    comparator> == 1));
    STATIC_CHECK((lower_bound_v<TT<char, short, int>,                   char,   comparator> == 0));
    STATIC_CHECK((lower_bound_v<TT<char, short, int>,                   short,  comparator> == 1));
    STATIC_CHECK((lower_bound_v<TT<char, short, int>,                   int,    comparator> == 2));
    STATIC_CHECK((lower_bound_v<TT<char, short, int>,                   double, comparator> == 3));
    STATIC_CHECK((lower_bound_v<TT<char, char, short, short, int, int>, char,   comparator> == 0));
    STATIC_CHECK((lower_bound_v<TT<char, char, short, short, int, int>, short,  comparator> == 2));
    STATIC_CHECK((lower_bound_v<TT<char, char, short, short, int, int>, int,    comparator> == 4));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_filter()
  {   
    STATIC_CHECK((std::is_same_v<filter_t<TT<>,           std::index_sequence<>>,     TT<>>));
    STATIC_CHECK((std::is_same_v<filter_t<TT<int>,        std::index_sequence<>>,     TT<>>));
    STATIC_CHECK((std::is_same_v<filter_t<TT<int>,        std::index_sequence<0>>,    TT<int>>));
    STATIC_CHECK((std::is_same_v<filter_t<TT<int, float>, std::index_sequence<0>>,    TT<int>>));
    STATIC_CHECK((std::is_same_v<filter_t<TT<int, float>, std::index_sequence<1>>,    TT<float>>));
    STATIC_CHECK((std::is_same_v<filter_t<TT<int, float>, std::index_sequence<0, 1>>, TT<int, float>>));
    
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<TT<>,          is_int>, TT<>>));
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<TT<float>,     is_int>, TT<>>));
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<TT<int>,       is_int>, TT<int>>));
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<TT<char, int>, is_int>, TT<int>>));
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<TT<int, char>, is_int>, TT<int>>));
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<TT<int, int>,  is_int>, TT<int, int>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_drop()
  {
    STATIC_CHECK((std::is_same_v<drop_t<TT<>, 0>,          TT<>>));
    STATIC_CHECK((std::is_same_v<drop_t<TT<int>, 0>,       TT<int>>));
    STATIC_CHECK((std::is_same_v<drop_t<TT<int>, 1>,       TT<>>));
    STATIC_CHECK((std::is_same_v<drop_t<TT<char, int>, 1>, TT<int>>));
    STATIC_CHECK((std::is_same_v<drop_t<TT<char, int>, 2>, TT<>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_keep()
  {
    STATIC_CHECK((std::is_same_v<keep_t<TT<>, 0>,          TT<>>));
    STATIC_CHECK((std::is_same_v<keep_t<TT<int>, 0>,       TT<>>));
    STATIC_CHECK((std::is_same_v<keep_t<TT<int>, 1>,       TT<int>>));
    STATIC_CHECK((std::is_same_v<keep_t<TT<char, int>, 0>, TT<>>));
    STATIC_CHECK((std::is_same_v<keep_t<TT<char, int>, 1>, TT<char>>));
    STATIC_CHECK((std::is_same_v<keep_t<TT<char, int>, 2>, TT<char, int>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_merge()
  {
    STATIC_CHECK((std::is_same_v<merge_t<TT<>,           TT<>,                    comparator>, TT<>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<>,           TT<int>,                 comparator>, TT<int>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<int>,        TT<>,                    comparator>, TT<int>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<int>,        TT<int>,                 comparator>, TT<int, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<char>,       TT<int>,                 comparator>, TT<char, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<int>,        TT<char>,                comparator>, TT<char, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<char>,       TT<char, int>,           comparator>, TT<char, char, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<short>,      TT<char, int>,           comparator>, TT<char, short, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<double>,     TT<char, int>,           comparator>, TT<char, int, double>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<char, int>,  TT<char>,                comparator>, TT<char, char, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<char, int>,  TT<char, short>,         comparator>, TT<char, char, short, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<TT<short, int>, TT<char, short, double>, comparator>, TT<char, short, short, int, double>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_stable_sort()
  {
    STATIC_CHECK((std::is_same_v<stable_sort_t<TT<>,                         comparator>, TT<>>));
    STATIC_CHECK((std::is_same_v<stable_sort_t<TT<int>,                      comparator>, TT<int>>));
    STATIC_CHECK((std::is_same_v<stable_sort_t<TT<char, int>,                comparator>, TT<char, int>>));
    STATIC_CHECK((std::is_same_v<stable_sort_t<TT<int, char>,                comparator>, TT<char, int>>));
    STATIC_CHECK((std::is_same_v<stable_sort_t<TT<int, char, double, short>, comparator>, TT<char, short, int, double>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_find()
  {
    STATIC_CHECK((find_v<TT<>,                    int>    == 0));
    STATIC_CHECK((find_v<TT<int>,                 int>    == 0));
    STATIC_CHECK((find_v<TT<int>,                 float>  == 1));
    STATIC_CHECK((find_v<TT<int, int>,            int>    == 0));
    STATIC_CHECK((find_v<TT<int, int>,            float>  == 2));
    STATIC_CHECK((find_v<TT<int, float>,          float>  == 1));
    STATIC_CHECK((find_v<TT<int, double, float>,  int>    == 0));
    STATIC_CHECK((find_v<TT<int, double, float>,  double> == 1));
    STATIC_CHECK((find_v<TT<int, double, float>,  float>  == 2));
    STATIC_CHECK((find_v<TT<int, double, float>,  char>   == 3));
    STATIC_CHECK((find_v<TT<int, double, int>,    int>    == 0));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_find_if()
  {
    STATIC_CHECK((find_if_v<TT<>,                    std::is_integral>       == 0));
    STATIC_CHECK((find_if_v<TT<int>,                 std::is_integral>       == 0));
    STATIC_CHECK((find_if_v<TT<int>,                 std::is_floating_point> == 1));
    STATIC_CHECK((find_if_v<TT<int, int>,            std::is_integral>       == 0));
    STATIC_CHECK((find_if_v<TT<int, int>,            std::is_floating_point> == 2));
    STATIC_CHECK((find_if_v<TT<int, float>,          std::is_floating_point> == 1));
    STATIC_CHECK((find_if_v<TT<int, double, float>,  std::is_integral>       == 0));
    STATIC_CHECK((find_if_v<TT<int, double, float>,  std::is_floating_point> == 1));
    STATIC_CHECK((find_if_v<TT<float, double, int>,  std::is_integral>       == 2));
    STATIC_CHECK((find_if_v<TT<int, double, float>,  std::is_pointer>        == 3));
    STATIC_CHECK((find_if_v<TT<int, double, int>,    std::is_integral>       == 0));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_contains()
  {
    STATIC_CHECK(!contains_v<TT<>,                    int>    );
    STATIC_CHECK( contains_v<TT<int>,                 int>    );
    STATIC_CHECK(!contains_v<TT<int>,                 float>  );
    STATIC_CHECK( contains_v<TT<int, int>,            int>    );
    STATIC_CHECK(!contains_v<TT<int, int>,            float>  );
    STATIC_CHECK( contains_v<TT<int, float>,          float>  );
    STATIC_CHECK( contains_v<TT<int, double, float>,  int>    );
    STATIC_CHECK( contains_v<TT<int, double, float>,  double> );
    STATIC_CHECK( contains_v<TT<int, double, float>,  float>  );
    STATIC_CHECK(!contains_v<TT<int, double, float>,  char>   );
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_erase()
  {
    STATIC_CHECK((std::is_same_v<erase_t<TT<int>, 0>,                TT<>>));
    STATIC_CHECK((std::is_same_v<erase_t<TT<int, float>, 0>,         TT<float>>));
    STATIC_CHECK((std::is_same_v<erase_t<TT<int, float>, 1>,         TT<int>>));
    STATIC_CHECK((std::is_same_v<erase_t<TT<int, float, double>, 0>, TT<float, double>>));
    STATIC_CHECK((std::is_same_v<erase_t<TT<int, float, double>, 1>, TT<int, double>>));
    STATIC_CHECK((std::is_same_v<erase_t<TT<int, float, double>, 2>, TT<int, float>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_insert()
  {
    STATIC_CHECK((std::is_same_v<insert_t<TT<>, int, 0>,            TT<int>>));
    STATIC_CHECK((std::is_same_v<insert_t<TT<int>, int, 0>,         TT<int, int>>));
    STATIC_CHECK((std::is_same_v<insert_t<TT<int>, int, 1>,         TT<int, int>>));
    STATIC_CHECK((std::is_same_v<insert_t<TT<int>, float, 0>,       TT<float, int>>));
    STATIC_CHECK((std::is_same_v<insert_t<TT<int>, float, 1>,       TT<int, float>>));
    STATIC_CHECK((std::is_same_v<insert_t<TT<int, float>, char, 0>, TT<char, int, float>>));
    STATIC_CHECK((std::is_same_v<insert_t<TT<int, float>, char, 1>, TT<int, char, float>>));
    STATIC_CHECK((std::is_same_v<insert_t<TT<int, float>, char, 2>, TT<int, float, char>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_flatten()
  {
    STATIC_CHECK(std::is_same_v<flatten_t<TT<>>,    TT<>>);
    STATIC_CHECK(std::is_same_v<flatten_t<TT<int>>, TT<int>>);

    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<>>>,    TT<>>);
    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<int>>>, TT<int>>);

    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<>,       TT<>>>,    TT<>>);
    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<int>,    TT<>>>,    TT<int>>);
    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<>,       TT<int>>>, TT<int>>);
    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<double>, TT<int>>>, TT<double, int>>);

    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<>,       TT<>, TT<>>>,               TT<>>);
    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<int>,    TT<>, TT<>>>,               TT<int>>);
    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<>,       TT<int>, TT<>>>,            TT<int>>);
    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<>,       TT<>, TT<int>>>,            TT<int>>);
    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<float>,  TT<double>, TT<int, int>>>, TT<float, double, int, int>>);

    STATIC_CHECK(std::is_same_v<flatten_t<TT<TT<TT<>>>>, TT<>>);
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_concat()
  {
    STATIC_CHECK(std::is_same_v<concat_t<TT<>,    TT<>>,     TT<>>);
    STATIC_CHECK(std::is_same_v<concat_t<TT<int>, TT<>>,     TT<int>>);
    STATIC_CHECK(std::is_same_v<concat_t<TT<>,    TT<int>>,  TT<int>>);
    STATIC_CHECK(std::is_same_v<concat_t<TT<int>, TT<char>>, TT<int, char>>);
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_reverse()
  {
    STATIC_CHECK(std::is_same_v<reverse_t<TT<>>,                        TT<>>);
    STATIC_CHECK(std::is_same_v<reverse_t<TT<int>>,                     TT<int>>);
    STATIC_CHECK(std::is_same_v<reverse_t<TT<int, char>>,               TT<char, int>>);
    STATIC_CHECK(std::is_same_v<reverse_t<TT<int, char, float>>,        TT<float, char, int>>);
    STATIC_CHECK(std::is_same_v<reverse_t<TT<int, char, float, short>>, TT<short, float, char, int>>);
  }
}
