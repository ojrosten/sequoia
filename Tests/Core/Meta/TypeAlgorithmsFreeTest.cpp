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
  }
  
  using namespace meta;
  
  [[nodiscard]]
  std::filesystem::path type_algorithms_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void type_algorithms_free_test::run_tests()
  {
    test_lower_bound();
    test_filter();
    test_drop();
    test_keep();
    test_merge();
    test_stable_sort();
  }

  void type_algorithms_free_test::test_lower_bound()
  {
    static_assert(lower_bound_v<std::tuple<>,     int, comparator> == 0);
    static_assert(lower_bound_v<std::tuple<int>,  int, comparator> == 0);
    static_assert(lower_bound_v<std::tuple<char>, int, comparator> == 1);
    static_assert(lower_bound_v<std::tuple<char, short, int>, char,   comparator> == 0);
    static_assert(lower_bound_v<std::tuple<char, short, int>, short,  comparator> == 1);
    static_assert(lower_bound_v<std::tuple<char, short, int>, int,    comparator> == 2);
    static_assert(lower_bound_v<std::tuple<char, short, int>, double, comparator> == 3);
    static_assert(lower_bound_v<std::tuple<char, char, short, short, int, int>, char,  comparator> == 0);
    static_assert(lower_bound_v<std::tuple<char, char, short, short, int, int>, short, comparator> == 2);
    static_assert(lower_bound_v<std::tuple<char, char, short, short, int, int>, int,   comparator> == 4);
  }

  void type_algorithms_free_test::test_filter()
  {   
    static_assert(std::is_same_v<filter_t<std::tuple<>, std::index_sequence<>>, std::tuple<>>);
    static_assert(std::is_same_v<filter_t<std::tuple<int>, std::index_sequence<>>, std::tuple<>>);
    static_assert(std::is_same_v<filter_t<std::tuple<int>, std::index_sequence<0>>, std::tuple<int>>);
    static_assert(std::is_same_v<filter_t<std::tuple<int, float>, std::index_sequence<0>>, std::tuple<int>>);
    static_assert(std::is_same_v<filter_t<std::tuple<int, float>, std::index_sequence<1>>, std::tuple<float>>);
    static_assert(std::is_same_v<filter_t<std::tuple<int, float>, std::index_sequence<0, 1>>, std::tuple<int, float>>);
  }

  void type_algorithms_free_test::test_drop()
  {
    static_assert(std::is_same_v<drop_t<std::tuple<>, 0>, std::tuple<>>);
    static_assert(std::is_same_v<drop_t<std::tuple<int>, 0>, std::tuple<int>>);
    static_assert(std::is_same_v<drop_t<std::tuple<int>, 1>, std::tuple<>>);
    static_assert(std::is_same_v<drop_t<std::tuple<char, int>, 1>, std::tuple<int>>);
    static_assert(std::is_same_v<drop_t<std::tuple<char, int>, 2>, std::tuple<>>);
  }

  void type_algorithms_free_test::test_keep()
  {
    static_assert(std::is_same_v<keep_t<std::tuple<>, 0>, std::tuple<>>);
    static_assert(std::is_same_v<keep_t<std::tuple<int>, 0>, std::tuple<>>);
    static_assert(std::is_same_v<keep_t<std::tuple<int>, 1>, std::tuple<int>>);
    static_assert(std::is_same_v<keep_t<std::tuple<char, int>, 0>, std::tuple<>>);
    static_assert(std::is_same_v<keep_t<std::tuple<char, int>, 1>, std::tuple<char>>);
    static_assert(std::is_same_v<keep_t<std::tuple<char, int>, 2>, std::tuple<char, int>>);
  }

  void type_algorithms_free_test::test_merge()
  {
    static_assert(std::is_same_v<merge_t<std::tuple<>, std::tuple<>, comparator>, std::tuple<>>);
    static_assert(std::is_same_v<merge_t<std::tuple<>, std::tuple<int>, comparator>, std::tuple<int>>);
    static_assert(std::is_same_v<merge_t<std::tuple<int>, std::tuple<>, comparator>, std::tuple<int>>);
    static_assert(std::is_same_v<merge_t<std::tuple<int>, std::tuple<int>,  comparator>, std::tuple<int, int>>);
    static_assert(std::is_same_v<merge_t<std::tuple<char>, std::tuple<int>, comparator>, std::tuple<char, int>>);
    static_assert(std::is_same_v<merge_t<std::tuple<int>, std::tuple<char>, comparator>, std::tuple<char, int>>);
    static_assert(std::is_same_v<merge_t<std::tuple<char>, std::tuple<char, int>, comparator>, std::tuple<char, char, int>>);
    static_assert(std::is_same_v<merge_t<std::tuple<short>, std::tuple<char, int>, comparator>, std::tuple<char, short, int>>);
    static_assert(std::is_same_v<merge_t<std::tuple<double>, std::tuple<char, int>, comparator>, std::tuple<char, int, double>>);
    static_assert(std::is_same_v<merge_t<std::tuple<char, int>, std::tuple<char>, comparator>, std::tuple<char, char, int>>);
    static_assert(std::is_same_v<merge_t<std::tuple<char, int>, std::tuple<char, short>, comparator>, std::tuple<char, char, short, int>>);
    static_assert(std::is_same_v<merge_t<std::tuple<short, int>, std::tuple<char, short, double>, comparator>, std::tuple<char, short, short, int, double>>);
  }

  void type_algorithms_free_test::test_stable_sort()
  {
    static_assert(std::is_same_v<stable_sort_t<std::tuple<>, comparator>, std::tuple<>>);
    static_assert(std::is_same_v<stable_sort_t<std::tuple<int>, comparator>, std::tuple<int>>);
    static_assert(std::is_same_v<stable_sort_t<std::tuple<char, int>, comparator>, std::tuple<char, int>>);
    static_assert(std::is_same_v<stable_sort_t<std::tuple<int, char>, comparator>, std::tuple<char, int>>);
    static_assert(std::is_same_v<stable_sort_t<std::tuple<int, char, double, short>, comparator>, std::tuple<char, short, int, double>>);
  }
}
