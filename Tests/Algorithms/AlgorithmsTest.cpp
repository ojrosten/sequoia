////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////


#include "AlgorithmsTest.hpp"
#include "../Maths/Graph/Components/Edges/EdgeTestingUtilities.hpp"

#include "sequoia/Algorithms/Algorithms.hpp"
#include "sequoia/Maths/Graph/Edge.hpp"
#include "sequoia/Core/Object/Handlers.hpp"

#include <array>
#include <istream>
#include <iterator>
#include <list>
#include <ranges>

namespace sequoia::testing
{
  namespace
  {
    struct unstable{};
    struct stable{};

    template<class T, std::size_t N, class Comparer = std::ranges::less, class Proj = std::identity>
    constexpr std::array<T, N> sort(unstable, std::array<T, N> a, Comparer comp = {}, Proj proj = {})
    {
      sequoia::sort(a, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comparer = std::ranges::less, class Proj = std::identity>
    constexpr std::array<T, N> sort(stable, std::array<T, N> a, Comparer comp = {}, Proj proj = {})
    {
      sequoia::stable_sort(a, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comparer = std::ranges::equal_to, class Proj = std::identity>
    constexpr std::array<T, N> cluster(std::array<T, N> a, Comparer comp = {}, Proj proj = {})
    {
      sequoia::cluster(a, comp, proj);
      return a;
    }

    template<class R, class Comp>
    concept make_heap_accepts
      =  requires(R&& r, Comp comp) { sequoia::make_heap(std::forward<R>(r), comp); };

    template<class R, class Comp>
    concept sort_accepts
      =  requires(R&& r, Comp comp) { sequoia::sort(std::forward<R>(r), comp); };

    template<class R, class Comp>
    concept stable_sort_accepts
      =  requires(R&& r, Comp comp) { sequoia::stable_sort(std::forward<R>(r), comp); };

    template<class R, class Comp>
    concept cluster_accepts
      =  requires(R&& r, Comp comp) { sequoia::cluster(std::forward<R>(r), comp); };

    template<class R, class OutIter>
    concept buffered_stable_sort_accepts
      =  requires(R&& r, OutIter out) { sequoia::stable_sort(std::forward<R>(r), out); };

    struct single_pass_iterator
    {
      using value_type       = int;
      using difference_type  = std::ptrdiff_t;
      using iterator_concept = std::input_iterator_tag;

      int* ptr{};

      [[nodiscard]]
      int& operator*() const { return *ptr; }

      single_pass_iterator& operator++()
      {
        ++ptr;
        return *this;
      }

      void operator++(int) { ++ptr; }
    };
  }

  [[nodiscard]]
  std::filesystem::path algorithms_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void algorithms_test::run_tests()
  {
    range_constraints();

    sort_basic_type(unstable{});
    sort_basic_type(stable{});
    sort_partial_edge(unstable{});
    sort_partial_edge(stable{});

    stable_sort_stability();

    cluster_basic_type();
    cluster_partial_edge();
    cluster_by_projection();
  }
   
  void algorithms_test::range_constraints()
  {
    using array_t    = std::array<int, 3>;
    using subrange_t = std::ranges::subrange<int*>;
    using list_t     = std::list<int>;
    using istream_t  = std::ranges::istream_view<int>;
    using less_t     = std::ranges::less;
    using equal_t    = std::ranges::equal_to;

    STATIC_CHECK( make_heap_accepts<      array_t&, less_t>);
    STATIC_CHECK( make_heap_accepts<    subrange_t, less_t>);
    STATIC_CHECK(!make_heap_accepts<const array_t&, less_t>);
    STATIC_CHECK(!make_heap_accepts<       list_t&, less_t>);
    STATIC_CHECK(!make_heap_accepts<      array_t&,    int>);

    STATIC_CHECK( sort_accepts<      array_t&, less_t>);
    STATIC_CHECK( sort_accepts<    subrange_t, less_t>);
    STATIC_CHECK(!sort_accepts<const array_t&, less_t>);
    STATIC_CHECK(!sort_accepts<       list_t&, less_t>);
    STATIC_CHECK(!sort_accepts<      array_t&,    int>);

    STATIC_CHECK( stable_sort_accepts<      array_t&, less_t>);
    STATIC_CHECK( stable_sort_accepts<    subrange_t, less_t>);
    STATIC_CHECK(!stable_sort_accepts<const array_t&, less_t>);
    STATIC_CHECK(!stable_sort_accepts<    istream_t&, less_t>);
    STATIC_CHECK(!stable_sort_accepts<      array_t&,    int>);

    STATIC_CHECK( std::input_iterator<single_pass_iterator>);
    STATIC_CHECK(!std::forward_iterator<single_pass_iterator>);
    STATIC_CHECK( buffered_stable_sort_accepts<array_t&,                 int*>);
    STATIC_CHECK(!buffered_stable_sort_accepts<array_t&, single_pass_iterator>);

    STATIC_CHECK( cluster_accepts<      array_t&, equal_t>);
    STATIC_CHECK( cluster_accepts<    subrange_t, equal_t>);
    STATIC_CHECK(!cluster_accepts<const array_t&, equal_t>);
    STATIC_CHECK(!cluster_accepts<    istream_t&, equal_t>);
    STATIC_CHECK(!cluster_accepts<      array_t&,     int>);
  }

  template<class Stability>
  void algorithms_test::sort_basic_type(Stability stability)
  {
    {
      constexpr std::array<int, 0> a{};
      constexpr auto b = sort(stability, a);
      check(equality, "Sort an empty array", b, a);
    }

    {
      constexpr std::array<int, 1> a{1};
      constexpr auto b = sort(stability, a);
      check(equality, "Sort an array of one element", b, a);
    }

    {
      constexpr std::array<int, 2> s{1,2};
      constexpr auto b = sort(stability, s);
      check(equality, "Sort a sorted array of two elements", b, s);

      constexpr std::array<int, 2> u{2,1};
      constexpr auto c = sort(stability, u);
      check(equality, "Sort an unsorted array of two elements", c, s);

      constexpr std::array<int, 2> t{1,1};
      constexpr auto d = sort(stability, t);
      check(equality, "Sort an array of two identical elements", d, t);
    }

    {
      constexpr std::array<int, 10> a{5,4,7,8,6,1,2,0,9,3};
      constexpr auto b = sort(stability, a);
      check(equality, "Sort digits from 0--9", b, {0,1,2,3,4,5,6,7,8,9});

      constexpr auto c = sort(stability, a, std::greater<int>{});
      check(equality, "Reverse sort digits from 0--9", c, {9,8,7,6,5,4,3,2,1,0});
    }

    {
      constexpr std::array<int, 11> a{5,4,7,8,6,1,10,2,0,9,3};
      constexpr auto b = sort(stability, a);
      check(equality, "Sort digits from 0--10", b, {0,1,2,3,4,5,6,7,8,9,10});

      constexpr auto c = sort(stability, a, std::greater<int>{});
      check(equality, "Reverse sort digits from 0--10", c, {10,9,8,7,6,5,4,3,2,1,0});
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
      check(equality, "", b, prediction);
    }

    {
      constexpr auto b = sort(stability, a, std::ranges::less{}, [](const edge& e) { return e.target_node(); });
      check(equality, "", b, prediction);
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
      check(equality, "Stable sort", b, prediction);
    }

    {
      constexpr auto b = sort(stable{}, a, std::ranges::less{}, [](const pair_t& p){ return p.first; });
      check(equality, "Stable sort", b, prediction);
    }
  }

  void algorithms_test::cluster_basic_type()
  {
    {
      constexpr std::array<int, 9> a{1,2,2,1,3,1,2,2,1};
      constexpr auto b = cluster(a);
      check(equality, "Cluster 9 digits", b, {1,1,1,1,3,2,2,2,2});
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
      check(equality, "", b, prediction);
    }

    {
      constexpr auto b = cluster(a, std::ranges::equal_to{}, [](const edge& e) { return e.target_node(); });
      check(equality, "", b, prediction);
    }
  }
  void algorithms_test::cluster_by_projection()
  {
    using pair_t = std::pair<int, int>;
    constexpr std::array<pair_t, 5>
      a{pair_t{1,0}, {2,0}, {1,1}, {3,0}, {2,1}},
      prediction{pair_t{1,0}, {1,1}, {2,0}, {2,1}, {3,0}};

    {
      constexpr auto b = cluster(a, [](const pair_t& lhs, const pair_t& rhs){ return lhs.first == rhs.first; });
      check(equality, "Cluster by first element", b, prediction);
    }

    {
      constexpr auto b = cluster(a, std::ranges::equal_to{}, [](const pair_t& p){ return p.first; });
      check(equality, "Cluster by first element", b, prediction);
    }
  }
}
