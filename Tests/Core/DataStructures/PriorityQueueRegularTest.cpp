////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PriorityQueueRegularTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <concepts>
#include <format>
#include <functional>
#include <list>

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

    // The element type differs from the container's
    STATIC_CHECK(!priority_queue_admits_v<int, std::vector<long>>);
    STATIC_CHECK(!priority_queue_admits_v<long, std::vector<int>>);

    // The container is not random access
    STATIC_CHECK(!priority_queue_admits_v<int, std::list<int>>);

    // The comparator does not compare
    STATIC_CHECK(!priority_queue_admits_v<int, std::vector<int>, int>);
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

        { edge_t{empty,
                 "Pop the only element",
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this, "Popping 1 from {1}", q);
                   return q;
                 }},
          edge_t{one_two,   "Push 2 onto 1", [](queue_t q) { q.push(2); return q; }},
          edge_t{one_three, "Push 3 onto 1", [](queue_t q) { q.push(3); return q; }}
        },

        { edge_t{empty,
                 "Pop the only element",
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this, "Popping 2 from {2}", q);
                   return q;
                 }},
          edge_t{one_two,   "Push 1 onto 2", [](queue_t q) { q.push(1); return q; }},
          edge_t{two_three, "Push 3 onto 2", [](queue_t q) { q.push(3); return q; }}
        },

        { edge_t{empty,
                 "Pop the only element",
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this, "Popping 3 from {3}", q);
                   return q;
                 }},
          edge_t{one_three,   "Push 1 onto 3",   [](queue_t q) { q.push(1); return q; }},
          edge_t{two_three,   "Push 2 onto 3",   [](queue_t q) { q.push(2); return q; }},
          edge_t{three_three, "Push a second 3", [](queue_t q) { q.push(3); return q; }}
        },

        { edge_t{choose_for<Compare>(one,                   two),
                 choose_for<Compare>("Pop 2, the greatest", "Pop 1, the least"),
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this,
                                                     choose_for<Compare>("Popping 2 from {1, 2}",
                                                                         "Popping 1 from {1, 2}"),
                                                     q);
                   return q;
                 }},
          edge_t{one_two_three, "Push 3 onto 1 and 2", [](queue_t q) { q.push(3); return q; }}
        },

        { edge_t{choose_for<Compare>(one,                   three),
                 choose_for<Compare>("Pop 3, the greatest", "Pop 1, the least"),
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this,
                                                     choose_for<Compare>("Popping 3 from {1, 3}",
                                                                         "Popping 1 from {1, 3}"),
                                                     q);
                   return q;
                 }},
          edge_t{one_two_three, "Push 2 onto 1 and 3", [](queue_t q) { q.push(2); return q; }}
        },

        { edge_t{choose_for<Compare>(two,                   three),
                 choose_for<Compare>("Pop 3, the greatest", "Pop 2, the least"),
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this,
                                                     choose_for<Compare>("Popping 3 from {2, 3}",
                                                                         "Popping 2 from {2, 3}"),
                                                     q);
                   return q;
                 }},
          edge_t{one_two_three,   "Push 1 onto 2 and 3", [](queue_t q) { q.push(1); return q; }},
          edge_t{two_three_three, "Push a second 3",     [](queue_t q) { q.push(3); return q; }}
        },

        { edge_t{three,
                 "Pop one of the two 3s",
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this, "Popping 3 from {3, 3}", q);
                   return q;
                 }} },

        { edge_t{choose_for<Compare>(one_two,               two_three),
                 choose_for<Compare>("Pop 3, the greatest", "Pop 1, the least"),
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this,
                                                     choose_for<Compare>("Popping 3 from {1, 2, 3}",
                                                                         "Popping 1 from {1, 2, 3}"),
                                                     q);
                   return q;
                 }},
          edge_t{one_two_three_three, "Push a second 3", [](queue_t q) { q.push(3); return q; }}
        },

        { edge_t{choose_for<Compare>(two_three,               three_three),
                 choose_for<Compare>("Pop one of the two 3s", "Pop 2, the least"),
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this,
                                                     choose_for<Compare>("Popping 3 from {2, 3, 3}",
                                                                         "Popping 2 from {2, 3, 3}"),
                                                     q);
                   return q;
                 }} },

        { edge_t{choose_for<Compare>(one_two_three,           two_three_three),
                 choose_for<Compare>("Pop one of the two 3s", "Pop 1, the least"),
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this,
                                                     choose_for<Compare>("Popping 3 from {1, 2, 3, 3}",
                                                                         "Popping 1 from {1, 2, 3, 3}"),
                                                     q);
                   return q;
                 }},
          edge_t{choose_for<Compare>(one_two,       three_three),
                 choose_for<Compare>("Pop both 3s", "Pop 1 and 2"),
                 [this](queue_t q) {
                   pop_and_check_the_element_is_gone(*this,
                                                     choose_for<Compare>("Popping 3 from {1, 2, 3, 3}",
                                                                         "Popping 1 from {1, 2, 3, 3}"),
                                                     q);
                   pop_and_check_the_element_is_gone(*this,
                                                     choose_for<Compare>("Popping 3 from {1, 2, 3}",
                                                                         "Popping 2 from {2, 3, 3}"),
                                                     q);
                   return q;
                 }}
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

    auto checkerFn{
      [this](std::string_view description,
             const queue_t& obtained,
             const queue_t& prediction,
             const queue_t& parent,
             const std::size_t host,
             const std::size_t target) {
        check(equality, {description, no_source_location}, obtained, prediction);
        if(host != target) check_semantics({description, no_source_location}, prediction, parent);
      }
    };

    const auto description{std::format("Priority queue transitions, ordered by {}", demangle<Compare>())};
    transition_checker_t::check(report(description), g, checkerFn);
  }
}
