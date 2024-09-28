////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */


#include "AlgorithmsTest.hpp"
#include "../Maths/Graph/Components/Edges/EdgeTestingUtilities.hpp"

#include "sequoia/Algorithms/Algorithms.hpp"
#include "sequoia/Maths/Graph/Edge.hpp"
#include "sequoia/Core/Object/Handlers.hpp"

#include <array>

namespace sequoia::testing
{
  namespace
  {
    struct unstable{};
    struct stable{};

    template<class T, std::size_t N, class Comparer = std::ranges::less, class Proj = std::identity>
    constexpr std::array<T, N> sort(unstable, std::array<T, N> a, Comparer comp = {}, Proj proj = {})
    {
      sequoia::sort(std::begin(a), std::end(a), comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comparer = std::ranges::less, class Proj = std::identity>
    constexpr std::array<T, N> sort(stable, std::array<T, N> a, Comparer comp = {}, Proj proj = {})
    {
      sequoia::stable_sort(std::begin(a), std::end(a), comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comparer = std::ranges::equal_to, class Proj = std::identity>
    constexpr std::array<T, N> cluster(std::array<T, N> a, Comparer comp = {}, Proj proj = {})
    {
      sequoia::cluster(std::begin(a), std::end(a), comp, proj);
      return a;
    }
  }

  [[nodiscard]]
  std::filesystem::path algorithms_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void algorithms_test::run_tests()
  {
    sort_basic_type(unstable{});
    sort_basic_type(stable{});
    sort_partial_edge(unstable{});
    sort_partial_edge(stable{});

    stable_sort_stability();

    cluster_basic_type();
    cluster_partial_edge();
  }
   
  template<class Stability>
  void algorithms_test::sort_basic_type(Stability stability)
  {
    {
      constexpr std::array<int, 0> a{};
      constexpr auto b = sort(stability, a);
      check(equality, report("Sort an empty array"), b, a);
    }

    {
      constexpr std::array<int, 1> a{1};
      constexpr auto b = sort(stability, a);
      check(equality, report("Sort an array of one element"), b, a);
    }

    {
      constexpr std::array<int, 2> s{1,2};
      constexpr auto b = sort(stability, s);
      check(equality, report("Sort a sorted array of two elements"), b, s);

      constexpr std::array<int, 2> u{2,1};
      constexpr auto c = sort(stability, u);
      check(equality, report("Sort an unsorted array of two elements"), c, s);

      constexpr std::array<int, 2> t{1,1};
      constexpr auto d = sort(stability, t);
      check(equality, report("Sort an array of two identical elements"), d, t);
    }

    {
      constexpr std::array<int, 10> a{5,4,7,8,6,1,2,0,9,3};
      constexpr auto b = sort(stability, a);
      check(equality, report("Sort digits from 0--9"), b, {0,1,2,3,4,5,6,7,8,9});

      constexpr auto c = sort(stability, a, std::greater<int>{});
      check(equality, report("Reverse sort digits from 0--9"), c, {9,8,7,6,5,4,3,2,1,0});
    }

    {
      constexpr std::array<int, 11> a{5,4,7,8,6,1,10,2,0,9,3};
      constexpr auto b = sort(stability, a);
      check(equality, report("Sort digits from 0--10"), b, {0,1,2,3,4,5,6,7,8,9,10});

      constexpr auto c = sort(stability, a, std::greater<int>{});
      check(equality, report("Reverse sort digits from 0--10"), c, {10,9,8,7,6,5,4,3,2,1,0});
    }
  }

  template<class Stability>
  void algorithms_test::sort_partial_edge(Stability stability)
  {
    struct null_type{};
    using edge = maths::partial_edge<object::by_value<null_type>, maths::null_meta_data>;
    constexpr std::array<edge, 3> a{edge{1}, edge{2}, edge{0}}, prediction{edge{0}, edge{1}, edge{2}};

    {
      constexpr auto b = sort(stability, a, [](const edge& lhs, const edge& rhs) { return lhs.target_node() < rhs.target_node(); });
      check(equality, report(""), b, prediction);
    }

    {
      constexpr auto b = sort(stability, a, std::ranges::less{}, [](const edge& e) { return e.target_node(); });
      check(equality, report(""), b, prediction);
    }
  }

  void algorithms_test::stable_sort_stability()
  {
    using pair_t = std::pair<int, int>;
    constexpr std::array<pair_t, 10>
      a{pair_t{5,1}, {5,2}, {4,1}, {4,0}, {6, -1}, {6,-2}, {6, 0}, {2,0}, {2,-1}, {2,2}},
      prediction{pair_t{2,0}, {2,-1}, {2,2}, {4,1}, {4,0}, {5,1}, {5,2}, {6, -1}, {6,-2},{6, 0}};

    {
      constexpr auto b = sort(stable{}, a, [](const pair_t& lhs, const pair_t& rhs){ return lhs.first < rhs.first; });
      check(equality, report("Stable sort"), b, prediction);
    }

    {
      constexpr auto b = sort(stable{}, a, std::ranges::less{}, [](const pair_t& p){ return p.first; });
      check(equality, report("Stable sort"), b, prediction);
    }
  }

  void algorithms_test::cluster_basic_type()
  {
    {
      constexpr std::array<int, 9> a{1,2,2,1,3,1,2,2,1};
      constexpr auto b = cluster(a);
      check(equality, report("Cluster 9 digits"), b, {1,1,1,1,3,2,2,2,2});
    }
  }

  void algorithms_test::cluster_partial_edge()
  {
    struct null_type{};
    using edge = maths::partial_edge<object::by_value<null_type>, maths::null_meta_data>;
    constexpr std::array<edge, 6>
      a{edge{1}, edge{2}, edge{2}, edge{0}, edge{1}, edge{0}},
      prediction{edge{1}, edge{1}, edge{2}, edge{2}, edge{0}, edge{0}};

    {
      constexpr auto b = cluster(a, [](const edge& lhs, const edge& rhs) { return lhs.target_node() == rhs.target_node(); });
      check(equality, report(""), b, prediction);
    }

    {
      constexpr auto b = cluster(a, std::ranges::equal_to{}, [](const edge& e) { return e.target_node(); });
      check(equality, report(""), b, prediction);
    }
  }
}
