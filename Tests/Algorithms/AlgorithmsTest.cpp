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

#include <algorithm>
#include <any>
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

    struct as_range{};
    struct as_iterator_and_sentinel{};

    template<class T, std::size_t N>
    constexpr auto counted_iterator_over(std::array<T, N>& a)
    {
      return std::counted_iterator{a.begin(), std::ranges::ssize(a)};
    }

    template<class T, std::size_t N, class Comp, class Proj>
    constexpr std::array<T, N> make_heap(as_range, std::array<T, N> a, Comp comp, Proj proj)
    {
      sequoia::make_heap(a, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp, class Proj>
    constexpr std::array<T, N> make_heap(as_iterator_and_sentinel, std::array<T, N> a, Comp comp, Proj proj)
    {
      sequoia::make_heap(counted_iterator_over(a), std::default_sentinel, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp = std::ranges::less, class Proj = std::identity>
    constexpr std::array<T, N> sort(unstable, as_range, std::array<T, N> a, Comp comp = {}, Proj proj = {})
    {
      sequoia::sort(a, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp = std::ranges::less, class Proj = std::identity>
    constexpr std::array<T, N> sort(unstable,
                                    as_iterator_and_sentinel,
                                    std::array<T, N> a,
                                    Comp comp = {},
                                    Proj proj = {})
    {
      sequoia::sort(counted_iterator_over(a), std::default_sentinel, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp = std::ranges::less, class Proj = std::identity>
    constexpr std::array<T, N> sort(stable, as_range, std::array<T, N> a, Comp comp = {}, Proj proj = {})
    {
      sequoia::stable_sort(a, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp = std::ranges::less, class Proj = std::identity>
    constexpr std::array<T, N> sort(stable,
                                    as_iterator_and_sentinel,
                                    std::array<T, N> a,
                                    Comp comp = {},
                                    Proj proj = {})
    {
      sequoia::stable_sort(counted_iterator_over(a), std::default_sentinel, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp, class Proj>
    constexpr std::array<T, N> sort_with_buffer(as_range, std::array<T, N> a, Comp comp, Proj proj)
    {
      std::array<T, N> buffer{};
      sequoia::stable_sort(a, buffer.begin(), comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp, class Proj>
    constexpr std::array<T, N> sort_with_buffer(as_iterator_and_sentinel, std::array<T, N> a, Comp comp, Proj proj)
    {
      std::array<T, N> buffer{};
      sequoia::stable_sort(counted_iterator_over(a), std::default_sentinel, buffer.begin(), comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp = std::ranges::equal_to, class Proj = std::identity>
    constexpr std::array<T, N> cluster(as_range, std::array<T, N> a, Comp comp = {}, Proj proj = {})
    {
      sequoia::cluster(a, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp = std::ranges::equal_to, class Proj = std::identity>
    constexpr std::array<T, N> cluster(as_iterator_and_sentinel, std::array<T, N> a, Comp comp = {}, Proj proj = {})
    {
      sequoia::cluster(counted_iterator_over(a), std::default_sentinel, comp, proj);
      return a;
    }

    template<class... Args>
    concept make_heap_accepts
      =  requires(Args&&... args) { sequoia::make_heap(std::forward<Args>(args)...); };

    template<class... Args>
    concept sort_accepts
      =  requires(Args&&... args) { sequoia::sort(std::forward<Args>(args)...); };

    template<class... Args>
    concept stable_sort_accepts
      =  requires(Args&&... args) { sequoia::stable_sort(std::forward<Args>(args)...); };

    template<class... Args>
    concept cluster_accepts
      =  requires(Args&&... args) { sequoia::cluster(std::forward<Args>(args)...); };

    struct move_only_equal_to
    {
      move_only_equal_to() = default;
      move_only_equal_to(move_only_equal_to&&) = default;
      move_only_equal_to(const move_only_equal_to&) = delete;

      [[nodiscard]]
      constexpr bool operator()(int lhs, int rhs) const { return lhs == rhs; }
    };

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
    algorithm_constraints();

    make_heap_by_second_element(as_range{});
    make_heap_by_second_element(as_iterator_and_sentinel{});

    sort_basic_type(unstable{}, as_range{});
    sort_basic_type(unstable{}, as_iterator_and_sentinel{});
    sort_basic_type(stable{},   as_range{});
    sort_basic_type(stable{},   as_iterator_and_sentinel{});

    sort_partial_edge(unstable{}, as_range{});
    sort_partial_edge(unstable{}, as_iterator_and_sentinel{});
    sort_partial_edge(stable{},   as_range{});
    sort_partial_edge(stable{},   as_iterator_and_sentinel{});

    sort_by_data_member(unstable{}, as_range{});
    sort_by_data_member(unstable{}, as_iterator_and_sentinel{});
    sort_by_data_member(stable{},   as_range{});
    sort_by_data_member(stable{},   as_iterator_and_sentinel{});

    stable_sort_stability(as_range{});
    stable_sort_stability(as_iterator_and_sentinel{});

    stable_sort_with_buffer(as_range{});
    stable_sort_with_buffer(as_iterator_and_sentinel{});

    cluster_basic_type(as_range{});
    cluster_basic_type(as_iterator_and_sentinel{});

    cluster_partial_edge(as_range{});
    cluster_partial_edge(as_iterator_and_sentinel{});

    cluster_by_first_element(as_range{});
    cluster_by_first_element(as_iterator_and_sentinel{});
  }

  void algorithms_test::algorithm_constraints()
  {
    using array_t                 = std::array<int, 3>;
    using subrange_t              = std::ranges::subrange<int*>;
    using list_t                  = std::list<int>;
    using istream_view_t          = std::ranges::istream_view<int>;
    using counted_iterator_t      = std::counted_iterator<int*>;
    using default_sentinel_t      = std::default_sentinel_t;
    using list_iterator_t         = list_t::iterator;
    using istream_view_iterator_t = std::ranges::iterator_t<istream_view_t>;
    using less_t                  = std::ranges::less;
    using equal_to_t              = std::ranges::equal_to;

    STATIC_CHECK( make_heap_accepts<       array_t&, less_t>);
    STATIC_CHECK( make_heap_accepts<     subrange_t, less_t>);
    STATIC_CHECK(!make_heap_accepts< const array_t&, less_t>);
    STATIC_CHECK(!make_heap_accepts<        list_t&, less_t>);
    STATIC_CHECK(!make_heap_accepts<       array_t&,    int>);

    STATIC_CHECK( make_heap_accepts<              int*,               int*, less_t>);
    STATIC_CHECK( make_heap_accepts<counted_iterator_t, default_sentinel_t, less_t>);
    STATIC_CHECK(!make_heap_accepts<        const int*,         const int*, less_t>);
    STATIC_CHECK(!make_heap_accepts<   list_iterator_t,    list_iterator_t, less_t>);
    STATIC_CHECK(!make_heap_accepts<              int*,            double*, less_t>);
    STATIC_CHECK(!make_heap_accepts<              int*,               int*,    int>);

    STATIC_CHECK( sort_accepts<       array_t&, less_t>);
    STATIC_CHECK( sort_accepts<     subrange_t, less_t>);
    STATIC_CHECK(!sort_accepts< const array_t&, less_t>);
    STATIC_CHECK(!sort_accepts<        list_t&, less_t>);
    STATIC_CHECK(!sort_accepts<       array_t&,    int>);

    STATIC_CHECK( sort_accepts<              int*,               int*, less_t>);
    STATIC_CHECK( sort_accepts<counted_iterator_t, default_sentinel_t, less_t>);
    STATIC_CHECK(!sort_accepts<        const int*,         const int*, less_t>);
    STATIC_CHECK(!sort_accepts<   list_iterator_t,    list_iterator_t, less_t>);
    STATIC_CHECK(!sort_accepts<              int*,            double*, less_t>);
    STATIC_CHECK(!sort_accepts<              int*,               int*,    int>);

    STATIC_CHECK( stable_sort_accepts<       array_t&, less_t>);
    STATIC_CHECK( stable_sort_accepts<     subrange_t, less_t>);
    STATIC_CHECK(!stable_sort_accepts< const array_t&, less_t>);
    STATIC_CHECK(!stable_sort_accepts<istream_view_t&, less_t>);
    STATIC_CHECK(!stable_sort_accepts<       array_t&,    int>);

    STATIC_CHECK( stable_sort_accepts<                   int*,               int*, less_t>);
    STATIC_CHECK( stable_sort_accepts<     counted_iterator_t, default_sentinel_t, less_t>);
    STATIC_CHECK(!stable_sort_accepts<             const int*,         const int*, less_t>);
    STATIC_CHECK(!stable_sort_accepts<istream_view_iterator_t, default_sentinel_t, less_t>);
    STATIC_CHECK(!stable_sort_accepts<                   int*,            double*, less_t>);
    STATIC_CHECK(!stable_sort_accepts<                   int*,               int*,    int>);

    STATIC_CHECK( std::input_iterator<single_pass_iterator>);
    STATIC_CHECK(!std::forward_iterator<single_pass_iterator>);
    STATIC_CHECK( stable_sort_accepts<array_t&,                 int*>);
    STATIC_CHECK(!stable_sort_accepts<array_t&, single_pass_iterator>);
    STATIC_CHECK(!stable_sort_accepts<array_t&,           const int*>);
    STATIC_CHECK(!stable_sort_accepts<array_t&,            std::any*>);

    STATIC_CHECK( stable_sort_accepts<int*,    int*,                 int*>);
    STATIC_CHECK(!stable_sort_accepts<int*, double*,                 int*>);
    STATIC_CHECK(!stable_sort_accepts<int*,    int*, single_pass_iterator>);
    STATIC_CHECK(!stable_sort_accepts<int*,    int*,           const int*>);
    STATIC_CHECK(!stable_sort_accepts<int*,    int*,            std::any*>);

    // None of these calls is ambiguous
    STATIC_CHECK(stable_sort_accepts<    int*, int*, int*, less_t, std::identity>);
    STATIC_CHECK(stable_sort_accepts<    int*, int*,       less_t, std::identity>);
    STATIC_CHECK(stable_sort_accepts<array_t&,       int*, less_t, std::identity>);
    STATIC_CHECK(stable_sort_accepts<array_t&,             less_t, std::identity>);

    STATIC_CHECK( cluster_accepts<       array_t&, equal_to_t>);
    STATIC_CHECK( cluster_accepts<     subrange_t, equal_to_t>);
    STATIC_CHECK(!cluster_accepts< const array_t&, equal_to_t>);
    STATIC_CHECK(!cluster_accepts<istream_view_t&, equal_to_t>);
    STATIC_CHECK(!cluster_accepts<       array_t&,        int>);

    STATIC_CHECK( cluster_accepts<                   int*,               int*, equal_to_t>);
    STATIC_CHECK( cluster_accepts<     counted_iterator_t, default_sentinel_t, equal_to_t>);
    STATIC_CHECK(!cluster_accepts<             const int*,         const int*, equal_to_t>);
    STATIC_CHECK(!cluster_accepts<istream_view_iterator_t, default_sentinel_t, equal_to_t>);
    STATIC_CHECK(!cluster_accepts<                   int*,            double*, equal_to_t>);
    STATIC_CHECK(!cluster_accepts<                   int*,               int*,        int>);

    STATIC_CHECK( clusterable<int*, equal_to_t,         std::identity>);
    STATIC_CHECK(!clusterable<int*, move_only_equal_to, std::identity>);
  }

  template<class Form>
  void algorithms_test::make_heap_by_second_element(Form form)
  {
    using pair_t = std::pair<int, int>;
    constexpr std::array<pair_t, 6> a{pair_t{0,3}, {1,5}, {2,0}, {3,4}, {4,1}, {5,2}};

    constexpr auto b = make_heap(form, a, std::ranges::greater{}, &pair_t::second);
    check("A heap under greater, by second element", std::ranges::is_heap(b, std::ranges::greater{}, &pair_t::second));
    check("The heap holds the original elements", std::ranges::is_permutation(b, a));
  }

  template<class Stability, class Form>
  void algorithms_test::sort_basic_type(Stability stability, Form form)
  {
    {
      constexpr std::array<int, 0> a{};
      constexpr auto b = sort(stability, form, a);
      check(equality, "Sort an empty array", b, a);
    }

    {
      constexpr std::array<int, 1> a{1};
      constexpr auto b = sort(stability, form, a);
      check(equality, "Sort an array of one element", b, a);
    }

    {
      constexpr std::array<int, 2> s{1,2};
      constexpr auto b = sort(stability, form, s);
      check(equality, "Sort a sorted array of two elements", b, s);

      constexpr std::array<int, 2> u{2,1};
      constexpr auto c = sort(stability, form, u);
      check(equality, "Sort an unsorted array of two elements", c, s);

      constexpr std::array<int, 2> t{1,1};
      constexpr auto d = sort(stability, form, t);
      check(equality, "Sort an array of two identical elements", d, t);
    }

    {
      constexpr std::array<int, 10> a{5,4,7,8,6,1,2,0,9,3};
      constexpr auto b = sort(stability, form, a);
      check(equality, "Sort digits from 0--9", b, {0,1,2,3,4,5,6,7,8,9});

      constexpr auto c = sort(stability, form, a, std::greater<int>{});
      check(equality, "Reverse sort digits from 0--9", c, {9,8,7,6,5,4,3,2,1,0});
    }

    {
      constexpr std::array<int, 11> a{5,4,7,8,6,1,10,2,0,9,3};
      constexpr auto b = sort(stability, form, a);
      check(equality, "Sort digits from 0--10", b, {0,1,2,3,4,5,6,7,8,9,10});

      constexpr auto c = sort(stability, form, a, std::greater<int>{});
      check(equality, "Reverse sort digits from 0--10", c, {10,9,8,7,6,5,4,3,2,1,0});
    }
  }

  template<class Stability, class Form>
  void algorithms_test::sort_partial_edge(Stability stability, Form form)
  {
    struct null_type{};
    using edge = maths::partial_edge<object::by_value<null_type>, maths::null_meta_data>;
    constexpr std::array<edge, 3> a{edge{1}, edge{2}, edge{0}}, prediction{edge{0}, edge{1}, edge{2}};

    {
      constexpr auto b = sort(
                           stability,
                           form,
                           a,
                           [](const edge& lhs, const edge& rhs) { return lhs.target_node() < rhs.target_node(); }
                         );
      check(equality, "", b, prediction);
    }

    {
      constexpr auto b = sort(stability, form, a, std::ranges::less{}, [](const edge& e) { return e.target_node(); });
      check(equality, "", b, prediction);
    }
  }

  template<class Stability, class Form>
  void algorithms_test::sort_by_data_member(Stability stability, Form form)
  {
    using pair_t = std::pair<int, int>;
    constexpr std::array<pair_t, 3> a{pair_t{1,2}, {3,0}, {2,1}}, prediction{pair_t{3,0}, {2,1}, {1,2}};

    constexpr auto b = sort(stability, form, a, std::ranges::less{}, &pair_t::second);
    check(equality, "Sort by a pointer to a data member", b, prediction);
  }

  template<class Form>
  void algorithms_test::stable_sort_stability(Form form)
  {
    using pair_t = std::pair<int, int>;
    constexpr std::array<pair_t, 10>
      a{pair_t{5,1}, {5,2}, {4,1}, {4,0}, {6, -1}, {6,-2}, {6, 0}, {2,0}, {2,-1}, {2,2}},
      prediction{pair_t{2,0}, {2,-1}, {2,2}, {4,1}, {4,0}, {5,1}, {5,2}, {6, -1}, {6,-2},{6, 0}};

    {
      constexpr auto b = sort(
                           stable{},
                           form,
                           a,
                           [](const pair_t& lhs, const pair_t& rhs){ return lhs.first < rhs.first; }
                         );
      check(equality, "Stable sort", b, prediction);
    }

    {
      constexpr auto b = sort(stable{}, form, a, std::ranges::less{}, [](const pair_t& p){ return p.first; });
      check(equality, "Stable sort", b, prediction);
    }
  }

  template<class Form>
  void algorithms_test::stable_sort_with_buffer(Form form)
  {
    using pair_t = std::pair<int, int>;
    constexpr std::array<pair_t, 10>
      a{pair_t{5,1}, {5,2}, {4,1}, {4,0}, {6, -1}, {6,-2}, {6, 0}, {2,0}, {2,-1}, {2,2}},
      prediction{pair_t{2,0}, {2,-1}, {2,2}, {4,1}, {4,0}, {5,1}, {5,2}, {6, -1}, {6,-2},{6, 0}};

    constexpr auto b = sort_with_buffer(form, a, std::ranges::less{}, &pair_t::first);
    check(equality, "Stable sort, with a buffer supplied", b, prediction);
  }

  template<class Form>
  void algorithms_test::cluster_basic_type(Form form)
  {
    {
      constexpr std::array<int, 9> a{1,2,2,1,3,1,2,2,1};
      constexpr auto b = cluster(form, a);
      check(equality, "Cluster 9 digits", b, {1,1,1,1,3,2,2,2,2});
    }
  }

  template<class Form>
  void algorithms_test::cluster_partial_edge(Form form)
  {
    struct null_type{};
    using edge = maths::partial_edge<object::by_value<null_type>, maths::null_meta_data>;
    constexpr std::array<edge, 6>
      a{edge{1}, edge{2}, edge{2}, edge{0}, edge{1}, edge{0}},
      prediction{edge{1}, edge{1}, edge{2}, edge{2}, edge{0}, edge{0}};

    {
      constexpr auto b = cluster(
                           form,
                           a,
                           [](const edge& lhs, const edge& rhs) { return lhs.target_node() == rhs.target_node(); }
                         );
      check(equality, "", b, prediction);
    }

    {
      constexpr auto b = cluster(form, a, std::ranges::equal_to{}, [](const edge& e) { return e.target_node(); });
      check(equality, "", b, prediction);
    }
  }

  template<class Form>
  void algorithms_test::cluster_by_first_element(Form form)
  {
    using pair_t = std::pair<int, int>;
    constexpr std::array<pair_t, 5>
      a{pair_t{1,0}, {2,0}, {1,1}, {3,0}, {2,1}},
      prediction{pair_t{1,0}, {1,1}, {2,0}, {2,1}, {3,0}};

    {
      constexpr auto b = cluster(form, a, [](const pair_t& lhs, const pair_t& rhs){ return lhs.first == rhs.first; });
      check(equality, "Cluster by a comparison of first elements", b, prediction);
    }

    {
      constexpr auto b = cluster(form, a, std::ranges::equal_to{}, [](const pair_t& p){ return p.first; });
      check(equality, "Cluster by a projection onto first elements", b, prediction);
    }

    {
      constexpr auto b = cluster(form, a, std::ranges::equal_to{}, &pair_t::first);
      check(equality, "Cluster by a pointer to the first element", b, prediction);
    }
  }
}
