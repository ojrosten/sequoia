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
    sort_basic_type(stable{});
  }
   
  template<class Stability>
  void algorithms_test::sort_basic_type(Stability stability)
  {
    constexpr std::array<int, 10> a{5,4,7,8,6,1,2,0,9,3};
    // Bug appears to occur in here; commenting this line out fixes the problem
    constexpr auto b = sort(stability, a);
  }

  void algorithms_test::stable_sort_stability()
  {
    using pair_t = std::pair<int, int>;
    constexpr std::array<pair_t, 10>
      a{pair_t{5,1}, {5,2}, {4,1}, {4,0}, {6, -1}, {6,-2}, {6, 0}, {2,0}, {2,-1}, {2,2}},
      prediction{pair_t{2,0}, {2,-1}, {2,2}, {4,1}, {4,0}, {5,1}, {5,2}, {6, -1}, {6,-2},{6, 0}};

    {
      constexpr auto b = sort(stable{}, a, [](const pair_t& lhs, const pair_t& rhs){ return lhs.first < rhs.first; });
      // But commenting out this line...
      check(equality, report_line("Stable sort"), b, prediction);
    }

    {
      constexpr auto b = sort(stable{}, a, std::ranges::less{}, [](const pair_t& p){ return p.first; });
      // And this line stops the crash
      check(equality, report_line("Stable sort"), b, prediction);
    }
  }

  // Alternatively, commenting out this method stops the crash
  void algorithms_test::cluster_basic_type()
  {
    constexpr std::array<int, 9> a{1,2,2,1,3,1,2,2,1};
    constexpr auto b = cluster(a);
    check(equality, report_line("Cluster 9 digits"), b, {1,1,1,1,3,2,2,2,2});
  }
}
