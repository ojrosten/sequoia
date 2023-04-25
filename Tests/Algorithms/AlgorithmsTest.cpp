////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */


#include "AlgorithmsTest.hpp"

#include "sequoia/Algorithms/Algorithms.hpp"
#include "sequoia/Core/Object/FaithfulWrapper.hpp"
#include "sequoia/Maths/Graph/Edge.hpp"
#include "sequoia/Core/Object/Handlers.hpp"

#include <array>

namespace sequoia::testing
{
  namespace
  {
    struct unstable{};
    struct stable{};

    template<class T, std::size_t N, class Comparer = std::ranges::less>
    constexpr std::array<T, N> sort(unstable, std::array<T, N> a, Comparer comp = Comparer{})
    {
      sequoia::sort(std::begin(a), std::end(a), comp);
      return a;
    }

    template<class T, std::size_t N, class Comparer = std::ranges::less>
    constexpr std::array<T, N> sort(stable, std::array<T, N> a, Comparer comp = Comparer{})
    {
      sequoia::stable_sort(std::begin(a), std::end(a), comp);
      return a;
    }

    template<class T, std::size_t N, class Comparer = std::ranges::equal_to>
    constexpr std::array<T, N> cluster(std::array<T, N> a, Comparer comp = Comparer{})
    {
      sequoia::cluster(std::begin(a), std::end(a), comp);
      return a;
    }
  }

  [[nodiscard]]
  std::filesystem::path algorithms_test::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void algorithms_test::run_tests()
  {
    sort_basic_type(unstable{});
    sort_basic_type(stable{});
    sort_faithful_wrapper();
    sort_partial_edge();

    cluster_basic_type();
  }
   
  template<class Stability>
  void algorithms_test::sort_basic_type(Stability stability)
  {
    {
      constexpr std::array<int, 0> a{};
      constexpr auto b = sort(stability, a);
      check(equality, report_line("Sort an empty array"), b, a);
    }

    {
      constexpr std::array<int, 1> a{1};
      constexpr auto b = sort(stability, a);
      check(equality, report_line("Sort an array of one element"), b, a);
    }

    {
      constexpr std::array<int, 2> s{1,2};
      constexpr auto b = sort(stability, s);
      check(equality, report_line("Sort a sorted array of two elements"), b, s);

      constexpr std::array<int, 2> u{2,1};
      constexpr auto c = sort(stability, u);
      check(equality, report_line("Sort an unsorted array of two elements"), c, s);

      constexpr std::array<int, 2> t{1,1};
      constexpr auto d = sort(stability, t);
      check(equality, report_line("Sort an array of two identical elements"), d, t);
    }

    {
      constexpr std::array<int, 10> a{5,4,7,8,6,1,2,0,9,3};
      constexpr auto b = sort(stability, a);
      check(equality, report_line("Sort digits from 0--9"), b, {0,1,2,3,4,5,6,7,8,9});

      constexpr auto c = sort(stability, a, std::greater<int>{});
      check(equality, report_line("Reverse sort digits from 0--9"), c, {9,8,7,6,5,4,3,2,1,0});
    }

    {
      constexpr std::array<int, 11> a{5,4,7,8,6,1,10,2,0,9,3};
      constexpr auto b = sort(stability, a);
      check(equality, report_line("Sort digits from 0--10"), b, {0,1,2,3,4,5,6,7,8,9,10});

      constexpr auto c = sort(stability, a, std::greater<int>{});
      check(equality, report_line("Reverse sort digits from 0--10"), c, {10,9,8,7,6,5,4,3,2,1,0});
    }
  }

  void algorithms_test::sort_faithful_wrapper()
  {
    using wrapper = object::faithful_wrapper<int>;
    constexpr std::array<wrapper, 4> a{wrapper{3}, wrapper{2}, wrapper{4}, wrapper{1}};
    constexpr auto b = sort(unstable{}, a);
    for(int i{}; i < 4; ++i)
      check(equality, report_line("Check array of wrapped ints, element " + std::to_string(i)), b[i].get(), i + 1);
  }

  void algorithms_test::sort_partial_edge()
  {
    struct null_type{};
    using edge = maths::partial_edge<object::by_value<object::faithful_wrapper<null_type>>>;
    constexpr std::array<edge, 3> a{edge{1}, edge{2}, edge{0}};
    constexpr auto b = sort(unstable{}, a, [](const edge& lhs, const edge& rhs) { return lhs.target_node() < rhs.target_node(); });

    for(std::size_t i{}; i < 3; ++i)
      check(equality, report_line("Check array of partial edges, element " + std::to_string(i)), b[i].target_node(), i);
  }

  void algorithms_test::cluster_basic_type()
  {
    {
      constexpr std::array<int, 9> a{1,2,2,1,3,1,2,2,1};
      constexpr auto b = cluster(a);
      check(equality, report_line("Cluster 9 digits"), b, {1,1,1,1,3,2,2,2,2});
    }
  }
}
