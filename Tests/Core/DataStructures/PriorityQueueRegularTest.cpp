////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PriorityQueueRegularTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <concepts>
#include <deque>
#include <format>
#include <functional>
#include <list>
#include <string>
#include <string_view>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    /** Compares two `int`s with `<`, adding one to `*count` each time, so
        that every copy of the comparator adds to the same count.
     */
    struct counting_less
    {
      int* count{};

      [[nodiscard]]
      constexpr bool operator()(const int lhs, const int rhs) const
      {
        ++*count;
        return lhs < rhs;
      }
    };

    struct vector_with_nothing_deleted : std::vector<int> {};

    struct vector_without_default_construction : std::vector<int>
    {
      vector_without_default_construction() = delete;
    };

    struct vector_without_lvalue_push_back : std::vector<int>
    {
      using std::vector<int>::push_back;

      void push_back(const int&) = delete;
    };

    struct vector_without_rvalue_push_back : std::vector<int>
    {
      using std::vector<int>::push_back;

      void push_back(int&&) = delete;
    };

    struct vector_without_pop_back : std::vector<int>
    {
      void pop_back() = delete;
    };

    struct vector_without_empty : std::vector<int>
    {
      bool empty() const = delete;
    };

    struct vector_without_size : std::vector<int>
    {
      size_type size() const = delete;
    };

    struct less_without_default_construction
    {
      constexpr explicit less_without_default_construction(int) {}

      [[nodiscard]]
      constexpr bool operator()(const int lhs, const int rhs) const { return lhs < rhs; }
    };

    template<class... Args>
    inline constexpr bool priority_queue_admits_v{requires { typename data_structures::priority_queue<Args...>; }};

    [[nodiscard]]
    constexpr int comparisons_in_one_pop()
    {
      int count{};
      data_structures::priority_queue<int, std::vector<int>, counting_less> q{counting_less{&count}};
      for(int i{}; i < 15; ++i)
      {
        q.push((i * 7) % 15);
      }

      count = 0;
      q.pop();

      return count;
    }

    template<class Compare, class T>
      requires std::same_as<Compare, std::ranges::less> || std::same_as<Compare, std::ranges::greater>
    [[nodiscard]]
    constexpr T choose_for(const T underLess, const T underGreater)
    {
      return std::same_as<Compare, std::ranges::less> ? underLess : underGreater;
    }
  }

  [[nodiscard]]
  std::filesystem::path priority_queue_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void priority_queue_regular_test::run_tests()
  {
    test_constraints();
    test_constant_evaluation();
    test_transitions<std::ranges::less>();
    test_transitions<std::ranges::greater>();
  }

  void priority_queue_regular_test::test_constraints()
  {
    STATIC_CHECK( priority_queue_admits_v<int>);
    STATIC_CHECK( priority_queue_admits_v<int, std::vector<int>, std::ranges::greater>);
    STATIC_CHECK( priority_queue_admits_v<bool, std::deque<bool>>);
    STATIC_CHECK( priority_queue_admits_v<int, vector_with_nothing_deleted>);

    // The element type differs from the container's
    STATIC_CHECK(!priority_queue_admits_v<int, std::vector<long>>);
    STATIC_CHECK(!priority_queue_admits_v<long, std::vector<int>>);

    // The container is not random access
    STATIC_CHECK(!priority_queue_admits_v<int, std::list<int>>);

    // The comparator does not compare
    STATIC_CHECK(!priority_queue_admits_v<int, std::vector<int>, int>);

    // The container returns no reference to its elements
    STATIC_CHECK(!priority_queue_admits_v<bool>);

    // The container lacks a member which priority_queue uses
    STATIC_CHECK(!priority_queue_admits_v<int, vector_without_default_construction>);
    STATIC_CHECK(!priority_queue_admits_v<int, vector_without_lvalue_push_back>);
    STATIC_CHECK(!priority_queue_admits_v<int, vector_without_rvalue_push_back>);
    STATIC_CHECK(!priority_queue_admits_v<int, vector_without_pop_back>);
    STATIC_CHECK(!priority_queue_admits_v<int, vector_without_empty>);
    STATIC_CHECK(!priority_queue_admits_v<int, vector_without_size>);

    // A caller must supply a comparator which has no default constructor
    using queue_without_default_comparator_t = data_structures::priority_queue<int, std::vector<int>, less_without_default_construction>;
    STATIC_CHECK(!std::default_initializable<queue_without_default_comparator_t>);
    STATIC_CHECK( std::constructible_from<queue_without_default_comparator_t, less_without_default_construction>);
  }

  void priority_queue_regular_test::test_constant_evaluation()
  {
    using int_queue_t     = data_structures::priority_queue<int>;
    using int_min_queue_t = data_structures::priority_queue<int, std::vector<int>, std::ranges::greater>;

    STATIC_CHECK(elements_in_pop_order(make_by_pushing<int_queue_t>({5, 3, 8, 1, 9, 2, 7, 4, 6, 0}))
                   == std::vector{9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
    STATIC_CHECK(elements_in_pop_order(make_by_pushing<int_min_queue_t>({5, 3, 8, 1, 9, 2, 7, 4, 6, 0}))
                   == std::vector{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    STATIC_CHECK(elements_in_pop_order(make_by_pushing<int_queue_t>({2, 2, 1, 2, 1, 1, 2}))
                   == std::vector{2, 2, 2, 2, 1, 1, 1});

    // std::ranges::pop_heap makes at most 2 log2(15) comparisons on a heap of
    // fifteen, which is at most 7. Rebuilding a heap of the fourteen left
    // would make at least 13.
    STATIC_CHECK(comparisons_in_one_pop() <= 7);
  }

  template<class Compare>
  void priority_queue_regular_test::test_transitions()
  {
    using queue_t              = observed_priority_queue<element_with_lineage, Compare>;
    using transition_checker_t = transition_checker<queue_t>;
    using graph_t              = transition_checker_t::transition_graph;
    using edge_t               = graph_t::edge_type;

    enum node {
      empty,
      one,
      two,
      three,
      one_two,
      one_three,
      two_three,
      three_three,
      one_two_three,
      two_three_three,
      one_two_three_three
    };

    struct outcome_of_pop
    {
      std::size_t target{};
      std::string_view description{};
    };

    struct outcomes_by_comparator
    {
      outcome_of_pop under_less{},
                     under_greater{};
    };

    const auto edgeOfPops{
      [this](const std::size_t pops, const outcomes_by_comparator& outcomes) {
        const auto [target, description]{choose_for<Compare>(outcomes.under_less, outcomes.under_greater)};
        return edge_t{target, std::string{description}, transition_of_pops<queue_t>(*this, pops)};
      }
    };

    const graph_t g{
      { { edge_t{one,   "Push 1 onto the empty queue", [](queue_t q) { q.push(1); return q; }},
          edge_t{two,   "Push 2 onto the empty queue", [](queue_t q) { q.push(2); return q; }},
          edge_t{three, "Push 3 onto the empty queue", [](queue_t q) { q.push(3); return q; }},
          edge_t{empty,
                 "Popping the empty queue throws, leaving it empty",
                 [this](queue_t q) {
                   check_exception_thrown<std::logic_error>("Popping the empty queue", [&q]() { q.pop(); });
                   return q;
                 }},
          edge_t{empty,
                 "Accessing the top of the empty queue throws",
                 [this](queue_t q) {
                   check_exception_thrown<std::logic_error>("Accessing the top of the empty queue",
                                                            [&q]() { return q.top(); });
                   return q;
                 }}
        },

        { edge_t{empty,     "Pop the only element", transition_of_pops<queue_t>(*this, 1)},
          edge_t{one_two,   "Push 2 onto 1",        [](queue_t q) { q.push(2); return q; }},
          edge_t{one_three, "Push 3 onto 1",        [](queue_t q) { q.push(3); return q; }}
        },

        { edge_t{empty,     "Pop the only element", transition_of_pops<queue_t>(*this, 1)},
          edge_t{one_two,   "Push 1 onto 2",        [](queue_t q) { q.push(1); return q; }},
          edge_t{two_three, "Push 3 onto 2",        [](queue_t q) { q.push(3); return q; }}
        },

        { edge_t{empty,       "Pop the only element", transition_of_pops<queue_t>(*this, 1)},
          edge_t{one_three,   "Push 1 onto 3",        [](queue_t q) { q.push(1); return q; }},
          edge_t{two_three,   "Push 2 onto 3",        [](queue_t q) { q.push(2); return q; }},
          edge_t{three_three, "Push a second 3",      [](queue_t q) { q.push(3); return q; }}
        },

        { edgeOfPops(1, {.under_less   {one, "Pop 2, the greatest"},
                         .under_greater{two, "Pop 1, the least"}}),
          edge_t{one_two_three, "Push 3 onto 1 and 2", [](queue_t q) { q.push(3); return q; }}
        },

        { edgeOfPops(1, {.under_less   {one,   "Pop 3, the greatest"},
                         .under_greater{three, "Pop 1, the least"}}),
          edge_t{one_two_three, "Push 2 onto 1 and 3", [](queue_t q) { q.push(2); return q; }}
        },

        { edgeOfPops(1, {.under_less   {two,   "Pop 3, the greatest"},
                         .under_greater{three, "Pop 2, the least"}}),
          edge_t{one_two_three,   "Push 1 onto 2 and 3", [](queue_t q) { q.push(1); return q; }},
          edge_t{two_three_three, "Push a second 3",     [](queue_t q) { q.push(3); return q; }}
        },

        { edge_t{three, "Pop one of the two 3s", transition_of_pops<queue_t>(*this, 1)} },

        { edgeOfPops(1, {.under_less   {one_two,   "Pop 3, the greatest"},
                         .under_greater{two_three, "Pop 1, the least"}}),
          edge_t{one_two_three_three, "Push a second 3", [](queue_t q) { q.push(3); return q; }}
        },

        { edgeOfPops(1, {.under_less   {two_three,   "Pop one of the two 3s"},
                         .under_greater{three_three, "Pop 2, the least"}}) },

        { edgeOfPops(1, {.under_less   {one_two_three,   "Pop one of the two 3s"},
                         .under_greater{two_three_three, "Pop 1, the least"}}),
          edgeOfPops(2, {.under_less   {one_two,     "Pop both 3s"},
                         .under_greater{three_three, "Pop 1 and 2"}})
        }
      },
      {queue_t{},
       make_by_pushing<queue_t>({1}),
       make_by_pushing<queue_t>({2}),
       make_by_pushing<queue_t>({3}),
       make_by_pushing<queue_t>({1, 2}),
       make_by_pushing<queue_t>({1, 3}),
       make_by_pushing<queue_t>({2, 3}),
       make_by_pushing<queue_t>({3, 3}),
       make_by_pushing<queue_t>({1, 2, 3}),
       make_by_pushing<queue_t>({2, 3, 3}),
       make_by_pushing<queue_t>({1, 2, 3, 3})}
    };

    const auto description{std::format("Priority queue transitions, ordered by {}", demangle<Compare>())};
    transition_checker_t::check(report(description), g, equality_and_semantics_check_fn<queue_t>(*this));
  }
}
