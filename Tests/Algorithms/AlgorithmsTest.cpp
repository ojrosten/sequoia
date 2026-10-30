////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////


#include "AlgorithmsTest.hpp"
#include "../Maths/Graph/Components/Edges/EdgeTestingUtilities.hpp"

import std;
import sequoia.algorithms;
import sequoia.core.object;
import sequoia.maths.graph;
import sequoia.test_framework;

namespace sequoia::testing
{
  namespace
  {
    struct range_form{};
    struct iterator_sentinel_form{};

    template<class T, std::size_t N>
    constexpr auto counted_iterator_over(std::array<T, N>& a)
    {
      return std::counted_iterator{a.begin(), std::ranges::ssize(a)};
    }

    template<class T, std::size_t N, class Comp, class Proj>
    constexpr std::array<T, N> make_heap(range_form, std::array<T, N> a, Comp comp, Proj proj)
    {
      sequoia::make_heap(a, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp, class Proj>
    constexpr std::array<T, N> make_heap(iterator_sentinel_form, std::array<T, N> a, Comp comp, Proj proj)
    {
      sequoia::make_heap(counted_iterator_over(a), std::default_sentinel, comp, proj);
      return a;
    }

    template<class T, std::size_t N>
    struct heap_removals
    {
      std::array<T, N> removed_elements{};
      bool             heap_after_every_removal{true};

      [[nodiscard]]
      friend constexpr bool operator==(const heap_removals&, const heap_removals&) noexcept = default;
    };

    template<class T, std::size_t N, class Comp, class Proj>
    constexpr heap_removals<T, N> removals_by_bubble_down(std::array<T, N> a, Comp comp, Proj proj)
    {
      sequoia::make_heap(a, comp, proj);

      heap_removals<T, N> removals{};
      auto end{a.end()};
      for(auto out{removals.removed_elements.begin()}; end != a.begin(); ++out)
      {
        *out = *a.begin();
        std::ranges::iter_swap(a.begin(), end - 1);
        --end;
        sequoia::bubble_down(a.begin(), a.begin(), end, comp, proj);
        removals.heap_after_every_removal = removals.heap_after_every_removal && std::ranges::is_heap(a.begin(), end, comp, proj);
      }

      return removals;
    }

    template<class T, std::size_t N, class Comp = std::ranges::less, class Proj = std::identity>
    constexpr std::array<T, N> stably_sorted(range_form, std::array<T, N> a, Comp comp = {}, Proj proj = {})
    {
      sequoia::stable_sort(a, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp = std::ranges::less, class Proj = std::identity>
    constexpr std::array<T, N> stably_sorted(iterator_sentinel_form,
                                             std::array<T, N> a,
                                             Comp comp = {},
                                             Proj proj = {})
    {
      sequoia::stable_sort(counted_iterator_over(a), std::default_sentinel, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp, class Proj>
    constexpr std::array<T, N> buffered_stable_sort(range_form, std::array<T, N> a, Comp comp, Proj proj)
    {
      std::array<T, N> buffer{};
      sequoia::stable_sort(a, buffer.begin(), comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp, class Proj>
    constexpr std::array<T, N> buffered_stable_sort(iterator_sentinel_form, std::array<T, N> a, Comp comp, Proj proj)
    {
      std::array<T, N> buffer{};
      sequoia::stable_sort(counted_iterator_over(a), std::default_sentinel, buffer.begin(), comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp = std::ranges::equal_to, class Proj = std::identity>
    constexpr std::array<T, N> cluster(range_form, std::array<T, N> a, Comp comp = {}, Proj proj = {})
    {
      sequoia::cluster(a, comp, proj);
      return a;
    }

    template<class T, std::size_t N, class Comp = std::ranges::equal_to, class Proj = std::identity>
    constexpr std::array<T, N> cluster(iterator_sentinel_form, std::array<T, N> a, Comp comp = {}, Proj proj = {})
    {
      sequoia::cluster(counted_iterator_over(a), std::default_sentinel, comp, proj);
      return a;
    }

    template<class T, std::size_t N>
    constexpr std::array<T, N> permute(std::array<T, N> a, std::vector<std::size_t> destinations)
    {
      sequoia::permute(a, std::move(destinations));
      return a;
    }

    struct swap_counter
    {
      std::size_t* total{};

      friend constexpr void swap(swap_counter& lhs, swap_counter& rhs) noexcept
      {
        ++*lhs.total;
        std::ranges::swap(lhs.total, rhs.total);
      }
    };

    [[nodiscard]]
    constexpr std::size_t swaps_made_by_permute(std::vector<std::size_t> destinations)
    {
      std::size_t swaps{};
      std::vector<swap_counter> elements(destinations.size(), swap_counter{&swaps});
      sequoia::permute(elements, std::move(destinations));
      return swaps;
    }

    template<class Size>
    struct range_with_size_type
    {
      std::array<int, 3> elements{};

      [[nodiscard]]
      constexpr int* begin() noexcept { return elements.data(); }

      [[nodiscard]]
      constexpr int* end() noexcept { return elements.data() + elements.size(); }

      [[nodiscard]]
      constexpr Size size() const noexcept { return static_cast<Size>(elements.size()); }
    };

    template<class... Args>
    concept make_heap_accepts
      =  requires(Args&&... args) { sequoia::make_heap(std::forward<Args>(args)...); };

    template<class... Args>
    concept bubble_up_accepts
      =  requires(Args&&... args) { sequoia::bubble_up(std::forward<Args>(args)...); };

    template<class... Args>
    concept bubble_down_accepts
      =  requires(Args&&... args) { sequoia::bubble_down(std::forward<Args>(args)...); };

    template<class... Args>
    concept stable_sort_accepts
      =  requires(Args&&... args) { sequoia::stable_sort(std::forward<Args>(args)...); };

    template<class... Args>
    concept cluster_accepts
      =  requires(Args&&... args) { sequoia::cluster(std::forward<Args>(args)...); };

    template<class... Args>
    concept permute_accepts
      =  requires(Args&&... args) { sequoia::permute(std::forward<Args>(args)...); };

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
    permute_basic_type();
    permute_swap_count();
    permute_leaves_destinations_unchanged();
    permute_allocation_count();

    make_heap_by_second_element(range_form{});
    make_heap_by_second_element(iterator_sentinel_form{});

    bubble_down_after_removing_the_top();

    stable_sort_basic_type(range_form{});
    stable_sort_basic_type(iterator_sentinel_form{});

    stable_sort_partial_edge(range_form{});
    stable_sort_partial_edge(iterator_sentinel_form{});

    stable_sort_by_second_element(range_form{});
    stable_sort_by_second_element(iterator_sentinel_form{});

    stable_sort_stability(range_form{});
    stable_sort_stability(iterator_sentinel_form{});

    stable_sort_with_buffer(range_form{});
    stable_sort_with_buffer(iterator_sentinel_form{});

    cluster_basic_type(range_form{});
    cluster_basic_type(iterator_sentinel_form{});

    cluster_partial_edge(range_form{});
    cluster_partial_edge(iterator_sentinel_form{});

    cluster_by_first_element(range_form{});
    cluster_by_first_element(iterator_sentinel_form{});

    permute_refuses_non_permutations();
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

    STATIC_CHECK( bubble_up_accepts<                int*,                int*, less_t>);
    STATIC_CHECK(!bubble_up_accepts<          const int*,          const int*, less_t>);
    STATIC_CHECK(!bubble_up_accepts<     list_iterator_t,     list_iterator_t, less_t>);
    STATIC_CHECK(!bubble_up_accepts<                int*,                int*,    int>);

    STATIC_CHECK( bubble_down_accepts<              int*,               int*,               int*, less_t>);
    STATIC_CHECK(!bubble_down_accepts<        const int*,         const int*,         const int*, less_t>);
    STATIC_CHECK(!bubble_down_accepts<   list_iterator_t,    list_iterator_t,    list_iterator_t, less_t>);
    STATIC_CHECK(!bubble_down_accepts<              int*,               int*,               int*,    int>);

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

    using char_sized_range_t                = range_with_size_type<char>;

    using destinations_t                    = std::vector<std::size_t>;
    using signed_destinations_t             = std::vector<int>;
    using boolean_destinations_t            = std::vector<bool>;
    using character_destinations_t          = std::vector<char8_t>;

    using array_destinations_t              = std::array<std::size_t, 3>;
    using span_destinations_t               = std::span<std::size_t>;
    using iota_destinations_t               = std::ranges::iota_view<std::size_t, std::size_t>;
    using list_destinations_t               = std::list<std::size_t>;
    using forward_list_destinations_t       = std::forward_list<std::size_t>;
    // A mutable lambda, so the view can be iterated only when non-const
    using non_const_iterable_destinations_t = decltype(iota_destinations_t{} | std::views::transform([offset{0uz}](std::size_t i) mutable { return i + offset; }));

    STATIC_CHECK( permute_accepts<           array_t&,                           destinations_t>);
    STATIC_CHECK( permute_accepts<           array_t&,                          destinations_t&>);
    STATIC_CHECK( permute_accepts<           array_t&,                    const destinations_t&>);
    STATIC_CHECK( permute_accepts<           array_t&,                    array_destinations_t&>);
    STATIC_CHECK( permute_accepts<           array_t&,                      span_destinations_t>);
    STATIC_CHECK( permute_accepts<           array_t&,                      iota_destinations_t>);
    STATIC_CHECK( permute_accepts<         subrange_t,                           destinations_t>);
    STATIC_CHECK( permute_accepts<           array_t&,                    signed_destinations_t>);
    STATIC_CHECK(!permute_accepts<     const array_t&,                           destinations_t>);
    STATIC_CHECK(!permute_accepts<            list_t&,                           destinations_t>);
    STATIC_CHECK(!permute_accepts<char_sized_range_t&,                           destinations_t>);
    STATIC_CHECK( permute_accepts<           array_t&,                      list_destinations_t>);
    STATIC_CHECK( permute_accepts<           array_t&,        non_const_iterable_destinations_t>);
    STATIC_CHECK(!permute_accepts<           array_t&, const non_const_iterable_destinations_t&>);
    STATIC_CHECK(!permute_accepts<           array_t&,              forward_list_destinations_t>);
    STATIC_CHECK(!permute_accepts<           array_t&,                   boolean_destinations_t>);
    STATIC_CHECK(!permute_accepts<           array_t&,                 character_destinations_t>);
  }

  void algorithms_test::permute_basic_type()
  {
    constexpr std::array<int, 4> a{10, 11, 12, 13};

    STATIC_CHECK(permute(std::array<int, 0>{}, {}) == std::array<int, 0>{});
    STATIC_CHECK(permute(a, {0, 1, 2, 3}) == a);
    STATIC_CHECK(permute(a, {2, 0, 3, 1}) == std::array{11, 13, 10, 12});
    STATIC_CHECK(permute(a, {1, 0, 3, 2}) == std::array{11, 10, 13, 12});
    STATIC_CHECK(permute(a, {3, 2, 1, 0}) == std::array{13, 12, 11, 10});

    STATIC_CHECK(
      []() {
        std::vector<std::size_t> v{2, 0, 3, 1};
        sequoia::permute(v, v);
        return v == std::vector<std::size_t>{0, 1, 2, 3};
      }()
    );

    STATIC_CHECK(
      []() {
        range_with_size_type<std::ptrdiff_t> elements{{10, 11, 12}};
        sequoia::permute(elements, std::vector<std::size_t>{2, 0, 1});
        return elements.elements == std::array{11, 12, 10};
      }()
    );

    STATIC_CHECK(
      []() {
        std::array elements{10, 11, 12};
        sequoia::permute(elements, std::views::iota(0uz, 3uz) | std::views::transform([offset{2uz}](std::size_t i) mutable { return (i + offset) % 3; }));
        return elements == std::array{11, 12, 10};
      }()
    );

    {
      std::array elements{10, 11, 12};
      sequoia::permute(elements, std::list<std::size_t>{2, 0, 1});
      check(equality, "Permuting by destinations in a list", elements, std::array{11, 12, 10});
    }
  }

  void algorithms_test::permute_swap_count()
  {
    STATIC_CHECK(swaps_made_by_permute({})                       == 0);
    STATIC_CHECK(swaps_made_by_permute({0, 1, 2, 3, 4, 5, 6, 7}) == 0);
    STATIC_CHECK(swaps_made_by_permute({1, 2, 3, 4, 5, 6, 7, 0}) == 7);
    STATIC_CHECK(swaps_made_by_permute({7, 0, 1, 2, 3, 4, 5, 6}) == 7);
    STATIC_CHECK(swaps_made_by_permute({1, 0, 3, 2, 5, 4, 7, 6}) == 4);
    STATIC_CHECK(swaps_made_by_permute({1, 2, 0, 4, 3, 5, 6, 7}) == 3);
  }

  void algorithms_test::permute_leaves_destinations_unchanged()
  {
    STATIC_CHECK(
      []() {
        std::array a{10, 11, 12};
        std::vector<std::size_t> destinations{2, 0, 1};
        sequoia::permute(a, destinations);
        return (a == std::array{11, 12, 10}) && (destinations == std::vector<std::size_t>{2, 0, 1});
      }()
    );

    STATIC_CHECK(
      []() {
        std::array a{10, 11, 12};
        std::array<std::size_t, 3> destinations{2, 0, 1};
        sequoia::permute(a, std::span{destinations});
        return (a == std::array{11, 12, 10}) && (destinations == std::array<std::size_t, 3>{2, 0, 1});
      }()
    );
  }

  void algorithms_test::permute_allocation_count()
  {
    using allocator_t    = shared_counting_allocator<std::size_t>;
    using destinations_t = std::vector<std::size_t, allocator_t>;

    allocator_t allocator{};

    const int emptyVectorAllocations{allocations_made_by_empty_construction<destinations_t>(allocator)};

    destinations_t destinations({2, 0, 1}, allocator);
    std::array a{10, 11, 12};

    check(equality,
          "Permuting by named destinations copies them",
          allocations_made_by(allocator, [&]() { sequoia::permute(a, destinations); }),
          emptyVectorAllocations + 1);

    check(equality,
          "Permuting by moved destinations does not copy them",
          allocations_made_by(allocator, [&]() { sequoia::permute(a, std::move(destinations)); }),
          emptyVectorAllocations);

    check(equality, "Moved destinations permute as named ones do", a, std::array{12, 10, 11});
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

  void algorithms_test::bubble_down_after_removing_the_top()
  {
    // Removing from ten elements passes through even sizes, where the last parent has a left child only
    constexpr std::array<int, 10> a{5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
    STATIC_CHECK(removals_by_bubble_down(a, std::ranges::less{}, std::identity{})
                 == heap_removals<int, 10>{{9, 8, 7, 6, 5, 4, 3, 2, 1, 0}, true});
    STATIC_CHECK(removals_by_bubble_down(a, std::ranges::greater{}, std::identity{})
                 == heap_removals<int, 10>{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, true});

    constexpr std::array<int, 7> duplicates{2, 2, 1, 2, 1, 1, 2};
    STATIC_CHECK(removals_by_bubble_down(duplicates, std::ranges::less{}, std::identity{})
                 == heap_removals<int, 7>{{2, 2, 2, 2, 1, 1, 1}, true});

    using pair_t = std::pair<int, int>;
    constexpr std::array<pair_t, 6> pairs{pair_t{0,3}, {1,5}, {2,0}, {3,4}, {4,1}, {5,2}};
    constexpr auto removals{removals_by_bubble_down(pairs, std::ranges::greater{}, &pair_t::second)};
    STATIC_CHECK(removals.heap_after_every_removal);
    STATIC_CHECK(removals.removed_elements == std::array<pair_t, 6>{pair_t{2,0}, {4,1}, {5,2}, {0,3}, {3,4}, {1,5}});

    STATIC_CHECK(removals_by_bubble_down(std::array{1}, std::ranges::less{}, std::identity{})
                 == heap_removals<int, 1>{{1}, true});
    STATIC_CHECK(removals_by_bubble_down(std::array<int, 0>{}, std::ranges::less{}, std::identity{})
                 == heap_removals<int, 0>{});
  }

  template<class Form>
  void algorithms_test::stable_sort_basic_type(Form form)
  {
    {
      constexpr std::array<int, 0> a{};
      constexpr auto b = stably_sorted(form, a);
      check(equality, "Sort an empty array", b, a);
    }

    {
      constexpr std::array<int, 1> a{1};
      constexpr auto b = stably_sorted(form, a);
      check(equality, "Sort an array of one element", b, a);
    }

    {
      constexpr std::array<int, 2> s{1,2};
      constexpr auto b = stably_sorted(form, s);
      check(equality, "Sort a sorted array of two elements", b, s);

      constexpr std::array<int, 2> u{2,1};
      constexpr auto c = stably_sorted(form, u);
      check(equality, "Sort an unsorted array of two elements", c, s);

      constexpr std::array<int, 2> t{1,1};
      constexpr auto d = stably_sorted(form, t);
      check(equality, "Sort an array of two identical elements", d, t);
    }

    {
      constexpr std::array<int, 10> a{5,4,7,8,6,1,2,0,9,3};
      constexpr auto b = stably_sorted(form, a);
      check(equality, "Sort digits from 0--9", b, {0,1,2,3,4,5,6,7,8,9});

      constexpr auto c = stably_sorted(form, a, std::greater<int>{});
      check(equality, "Reverse sort digits from 0--9", c, {9,8,7,6,5,4,3,2,1,0});
    }

    {
      constexpr std::array<int, 11> a{5,4,7,8,6,1,10,2,0,9,3};
      constexpr auto b = stably_sorted(form, a);
      check(equality, "Sort digits from 0--10", b, {0,1,2,3,4,5,6,7,8,9,10});

      constexpr auto c = stably_sorted(form, a, std::greater<int>{});
      check(equality, "Reverse sort digits from 0--10", c, {10,9,8,7,6,5,4,3,2,1,0});
    }
  }

  template<class Form>
  void algorithms_test::stable_sort_partial_edge(Form form)
  {
    struct null_type{};
    using edge = maths::partial_edge<object::by_value<null_type>, maths::null_meta_data>;
    constexpr std::array<edge, 3> a{edge{1}, edge{2}, edge{0}}, prediction{edge{0}, edge{1}, edge{2}};

    {
      constexpr auto b = stably_sorted(
                           form,
                           a,
                           [](const edge& lhs, const edge& rhs) { return lhs.target_node() < rhs.target_node(); }
                         );
      check(equality, "", b, prediction);
    }

    {
      constexpr auto b = stably_sorted(form, a, std::ranges::less{}, [](const edge& e) { return e.target_node(); });
      check(equality, "", b, prediction);
    }
  }

  template<class Form>
  void algorithms_test::stable_sort_by_second_element(Form form)
  {
    using pair_t = std::pair<int, int>;
    constexpr std::array<pair_t, 3> a{pair_t{1,2}, {3,0}, {2,1}}, prediction{pair_t{3,0}, {2,1}, {1,2}};

    constexpr auto b = stably_sorted(form, a, std::ranges::less{}, &pair_t::second);
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
      constexpr auto b = stably_sorted(
                           form,
                           a,
                           [](const pair_t& lhs, const pair_t& rhs){ return lhs.first < rhs.first; }
                         );
      check(equality, "Stable sort", b, prediction);
    }

    {
      constexpr auto b = stably_sorted(form, a, std::ranges::less{}, [](const pair_t& p){ return p.first; });
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

    constexpr auto b = buffered_stable_sort(form, a, std::ranges::less{}, &pair_t::first);
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

  void algorithms_test::permute_refuses_non_permutations()
  {
    check_exception_thrown<std::invalid_argument>(
      "Fewer destinations than elements are not a permutation",
      [](){ return permute(std::array{10, 11, 12}, {1, 0}); }
    );

    check_exception_thrown<std::invalid_argument>(
      "More destinations than elements are not a permutation",
      [](){ return permute(std::array{10, 11}, {1, 0, 2}); }
    );

    check_exception_thrown<std::invalid_argument>(
      "A destination shared by two elements is not a permutation",
      [](){ return permute(std::array{10, 11, 12}, {1, 1, 0}); }
    );

    check_exception_thrown<std::invalid_argument>(
      "A destination shared by two elements, found only after a swap, is not a permutation",
      [](){ return permute(std::array{10, 11, 12}, {1, 2, 2}); }
    );

    check_exception_thrown<std::invalid_argument>(
      "A destination shared with an element already in place is not a permutation",
      [](){ return permute(std::array{10, 11}, {0, 0}); }
    );

    check_exception_thrown<std::invalid_argument>(
      "A destination beyond the range is not a permutation",
      [](){ return permute(std::array{10, 11, 12}, {1, 3, 0}); }
    );

    check_exception_thrown<std::invalid_argument>(
      "A negative destination is not a permutation",
      [](){
        std::array a{10, 11, 12};
        sequoia::permute(a, std::vector{1, -1, 0});
        return a;
      }
    );
  }
}
