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

    test_erase<std::tuple>();
    test_erase<std::variant>();

    test_insert<std::tuple>();
    test_insert<std::variant>();
  }

  void type_algorithms_free_test::test_type_comparator()
  {
    STATIC_CHECK((type_comparator_v<char, void>));
    STATIC_CHECK((!type_comparator_v<int, char>));
  }
  
  template<template<class...> class TT>
  void type_algorithms_free_test::test_lower_bound()
  {
    STATIC_CHECK((lower_bound_v<TT<>,     int, comparator> == 0));
    STATIC_CHECK((lower_bound_v<TT<int>,  int, comparator> == 0));
    STATIC_CHECK((lower_bound_v<TT<char>, int, comparator> == 1));
    STATIC_CHECK((lower_bound_v<TT<char, short, int>, char,   comparator> == 0));
    STATIC_CHECK((lower_bound_v<TT<char, short, int>, short,  comparator> == 1));
    STATIC_CHECK((lower_bound_v<TT<char, short, int>, int,    comparator> == 2));
    STATIC_CHECK((lower_bound_v<TT<char, short, int>, double, comparator> == 3));
    STATIC_CHECK((lower_bound_v<TT<char, char, short, short, int, int>, char,  comparator> == 0));
    STATIC_CHECK((lower_bound_v<TT<char, char, short, short, int, int>, short, comparator> == 2));
    STATIC_CHECK((lower_bound_v<TT<char, char, short, short, int, int>, int,   comparator> == 4));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_filter()
  {   
    STATIC_CHECK((std::is_same_v<filter_t<std::tuple<>, std::index_sequence<>>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<filter_t<std::tuple<int>, std::index_sequence<>>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<filter_t<std::tuple<int>, std::index_sequence<0>>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<filter_t<std::tuple<int, float>, std::index_sequence<0>>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<filter_t<std::tuple<int, float>, std::index_sequence<1>>, std::tuple<float>>));
    STATIC_CHECK((std::is_same_v<filter_t<std::tuple<int, float>, std::index_sequence<0, 1>>, std::tuple<int, float>>));
    
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<std::tuple<>, is_int>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<std::tuple<float>, is_int>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<std::tuple<int>, is_int>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<std::tuple<char, int>, is_int>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<std::tuple<int, char>, is_int>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<filter_by_trait_t<std::tuple<int, int>, is_int>, std::tuple<int, int>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_drop()
  {
    STATIC_CHECK((std::is_same_v<drop_t<std::tuple<>, 0>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<drop_t<std::tuple<int>, 0>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<drop_t<std::tuple<int>, 1>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<drop_t<std::tuple<char, int>, 1>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<drop_t<std::tuple<char, int>, 2>, std::tuple<>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_keep()
  {
    STATIC_CHECK((std::is_same_v<keep_t<std::tuple<>, 0>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<keep_t<std::tuple<int>, 0>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<keep_t<std::tuple<int>, 1>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<keep_t<std::tuple<char, int>, 0>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<keep_t<std::tuple<char, int>, 1>, std::tuple<char>>));
    STATIC_CHECK((std::is_same_v<keep_t<std::tuple<char, int>, 2>, std::tuple<char, int>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_merge()
  {
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<>, std::tuple<>, comparator>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<>, std::tuple<int>, comparator>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<int>, std::tuple<>, comparator>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<int>, std::tuple<int>,  comparator>, std::tuple<int, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<char>, std::tuple<int>, comparator>, std::tuple<char, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<int>, std::tuple<char>, comparator>, std::tuple<char, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<char>, std::tuple<char, int>, comparator>, std::tuple<char, char, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<short>, std::tuple<char, int>, comparator>, std::tuple<char, short, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<double>, std::tuple<char, int>, comparator>, std::tuple<char, int, double>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<char, int>, std::tuple<char>, comparator>, std::tuple<char, char, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<char, int>, std::tuple<char, short>, comparator>, std::tuple<char, char, short, int>>));
    STATIC_CHECK((std::is_same_v<merge_t<std::tuple<short, int>, std::tuple<char, short, double>, comparator>, std::tuple<char, short, short, int, double>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_stable_sort()
  {
    STATIC_CHECK((std::is_same_v<stable_sort_t<std::tuple<>, comparator>, std::tuple<>>));
    STATIC_CHECK((std::is_same_v<stable_sort_t<std::tuple<int>, comparator>, std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<stable_sort_t<std::tuple<char, int>, comparator>, std::tuple<char, int>>));
    STATIC_CHECK((std::is_same_v<stable_sort_t<std::tuple<int, char>, comparator>, std::tuple<char, int>>));
    STATIC_CHECK((std::is_same_v<stable_sort_t<std::tuple<int, char, double, short>, comparator>, std::tuple<char, short, int, double>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_find()
  {
    STATIC_CHECK((find_v<std::tuple<>, int>                      == 0));
    STATIC_CHECK((find_v<std::tuple<int>, int>                   == 0));
    STATIC_CHECK((find_v<std::tuple<int>, float>                 == 1));
    STATIC_CHECK((find_v<std::tuple<int, int>, int>              == 0));
    STATIC_CHECK((find_v<std::tuple<int, int>, float>            == 2));
    STATIC_CHECK((find_v<std::tuple<int, float>, float>          == 1));
    STATIC_CHECK((find_v<std::tuple<int, double, float>, int>    == 0));
    STATIC_CHECK((find_v<std::tuple<int, double, float>, double> == 1));
    STATIC_CHECK((find_v<std::tuple<int, double, float>, float>  == 2));
    STATIC_CHECK((find_v<std::tuple<int, double, float>, char>   == 3));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_erase()
  {
    STATIC_CHECK((std::is_same_v<erase_t<std::tuple<int>, 0>,                std::tuple<>>));
    STATIC_CHECK((std::is_same_v<erase_t<std::tuple<int, float>, 0>,         std::tuple<float>>));
    STATIC_CHECK((std::is_same_v<erase_t<std::tuple<int, float>, 1>,         std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<erase_t<std::tuple<int, float, double>, 0>, std::tuple<float, double>>));
    STATIC_CHECK((std::is_same_v<erase_t<std::tuple<int, float, double>, 1>, std::tuple<int, double>>));
    STATIC_CHECK((std::is_same_v<erase_t<std::tuple<int, float, double>, 2>, std::tuple<int, float>>));
  }

  template<template<class...> class TT>
  void type_algorithms_free_test::test_insert()
  {
    STATIC_CHECK((std::is_same_v<insert_t<std::tuple<>, int, 0>,            std::tuple<int>>));
    STATIC_CHECK((std::is_same_v<insert_t<std::tuple<int>, int, 0>,         std::tuple<int, int>>));
    STATIC_CHECK((std::is_same_v<insert_t<std::tuple<int>, int, 1>,         std::tuple<int, int>>));
    STATIC_CHECK((std::is_same_v<insert_t<std::tuple<int>, float, 0>,       std::tuple<float, int>>));
    STATIC_CHECK((std::is_same_v<insert_t<std::tuple<int>, float, 1>,       std::tuple<int, float>>));
    STATIC_CHECK((std::is_same_v<insert_t<std::tuple<int, float>, char, 0>, std::tuple<char, int, float>>));
    STATIC_CHECK((std::is_same_v<insert_t<std::tuple<int, float>, char, 1>, std::tuple<int, char, float>>));
    STATIC_CHECK((std::is_same_v<insert_t<std::tuple<int, float>, char, 2>, std::tuple<int, float, char>>));
  }
}
