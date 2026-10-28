////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PriorityQueueRegularTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <list>
#include <memory>

namespace sequoia::testing
{
  namespace
  {
    using queue_t = data_structures::priority_queue<int>;

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

    struct pointee_less
    {
      [[nodiscard]]
      constexpr bool operator()(const std::shared_ptr<int>& lhs, const std::shared_ptr<int>& rhs) const
      {
        return *lhs < *rhs;
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
    test_semantics();
    test_transitions();
    test_pop_destroys_the_element();
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
    using min_queue_t = data_structures::priority_queue<int, std::vector<int>, std::ranges::greater>;

    STATIC_CHECK(elements_in_pop_order(make_by_pushing<queue_t>({5, 3, 8, 1, 9, 2, 7, 4, 6, 0}))
                   == std::vector{9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
    STATIC_CHECK(elements_in_pop_order(make_by_pushing<min_queue_t>({5, 3, 8, 1, 9, 2, 7, 4, 6, 0}))
                   == std::vector{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    STATIC_CHECK(elements_in_pop_order(make_by_pushing<queue_t>({2, 2, 1, 2, 1, 1, 2}))
                   == std::vector{2, 2, 2, 2, 1, 1, 1});

    // std::ranges::pop_heap makes at most 2 log2(15) comparisons on a heap of
    // fifteen, which is at most 7. Rebuilding a heap of the fourteen left
    // would make at least 13.
    STATIC_CHECK(comparisons_in_one_pop() <= 7);
  }

  void priority_queue_regular_test::test_semantics()
  {
    const observed_priority_queue empty{};
    const auto one{make_by_pushing<observed_priority_queue>({1})},
               oneTwo{make_by_pushing<observed_priority_queue>({1, 2})},
               oneTwoThree{make_by_pushing<observed_priority_queue>({1, 2, 3})},
               threeTwoOne{make_by_pushing<observed_priority_queue>({3, 2, 1})};

    check(equivalence, "Empty", empty, std::vector<int>{});
    check(equivalence, "One element", one, std::vector<int>{1});
    check(equivalence, "Two elements, greatest first", oneTwo, std::vector<int>{2, 1});
    check(equivalence, "Three elements, greatest first", oneTwoThree, std::vector<int>{3, 2, 1});
    check(equality, "The order of the pushes is not observable", oneTwoThree, threeTwoOne);

    check_semantics("Empty against one element", empty, one);
    check_semantics("One element against two", one, oneTwo);
    check_semantics("Two elements against three", oneTwo, threeTwoOne);
  }

  void priority_queue_regular_test::test_transitions()
  {
    using transition_checker_t = transition_checker<observed_priority_queue>;
    using graph_t              = transition_checker_t::transition_graph;
    using edge_t               = graph_t::edge_type;

    enum node { empty, one, two, one_two, one_three, two_three, one_two_three, one_two_three_three };

    const graph_t g{
      { { edge_t{one, "Push 1 onto the empty queue", [](observed_priority_queue q) { q.push(1); return q; }},
          edge_t{two, "Push 2 onto the empty queue", [](observed_priority_queue q) { q.push(2); return q; }},
          edge_t{empty,
                 "Popping the empty queue throws, leaving it empty",
                 [this](observed_priority_queue q) {
                   check_exception_thrown<std::logic_error>("Popping the empty queue", [&q]() { q.pop(); });
                   return q;
                 }},
          edge_t{empty,
                 "Accessing the top of the empty queue throws",
                 [this](observed_priority_queue q) {
                   check_exception_thrown<std::logic_error>("Accessing the top of the empty queue",
                                                            [&q]() { return q.top(); });
                   return q;
                 }}
        },

        { edge_t{empty,     "Pop the only element", [](observed_priority_queue q) { q.pop(); return q; }},
          edge_t{one_two,   "Push 2 onto 1",        [](observed_priority_queue q) { q.push(2); return q; }},
          edge_t{one_three, "Push 3 onto 1",        [](observed_priority_queue q) { q.push(3); return q; }}
        },

        { edge_t{empty,     "Pop the only element", [](observed_priority_queue q) { q.pop(); return q; }},
          edge_t{one_two,   "Push 1 onto 2",        [](observed_priority_queue q) { q.push(1); return q; }},
          edge_t{two_three, "Push 3 onto 2",        [](observed_priority_queue q) { q.push(3); return q; }}
        },

        { edge_t{one,           "Pop 2, the greatest", [](observed_priority_queue q) { q.pop(); return q; }},
          edge_t{one_two_three, "Push 3 onto 1 and 2", [](observed_priority_queue q) { q.push(3); return q; }}
        },

        { edge_t{one,           "Pop 3, the greatest", [](observed_priority_queue q) { q.pop(); return q; }},
          edge_t{one_two_three, "Push 2 onto 1 and 3", [](observed_priority_queue q) { q.push(2); return q; }}
        },

        { edge_t{two,           "Pop 3, the greatest", [](observed_priority_queue q) { q.pop(); return q; }},
          edge_t{one_two_three, "Push 1 onto 2 and 3", [](observed_priority_queue q) { q.push(1); return q; }}
        },

        { edge_t{one_two,             "Pop 3, the greatest", [](observed_priority_queue q) { q.pop(); return q; }},
          edge_t{one_two_three_three, "Push a second 3",     [](observed_priority_queue q) { q.push(3); return q; }}
        },

        { edge_t{one_two_three, "Pop one of the two 3s", [](observed_priority_queue q) { q.pop(); return q; }},
          edge_t{one_two,       "Pop both 3s",           [](observed_priority_queue q) { q.pop(); q.pop(); return q; }}
        }
      },
      {observed_priority_queue{},
       make_by_pushing<observed_priority_queue>({1}),
       make_by_pushing<observed_priority_queue>({2}),
       make_by_pushing<observed_priority_queue>({1, 2}),
       make_by_pushing<observed_priority_queue>({1, 3}),
       make_by_pushing<observed_priority_queue>({2, 3}),
       make_by_pushing<observed_priority_queue>({1, 2, 3}),
       make_by_pushing<observed_priority_queue>({1, 2, 3, 3})}
    };

    auto checkerFn{
      [this](std::string description,
             const observed_priority_queue& obtained,
             const observed_priority_queue& prediction) {
        check(equality, description, obtained, prediction);
      }
    };

    transition_checker_t::check(report("Priority queue transitions"), g, checkerFn);
  }

  void priority_queue_regular_test::test_pop_destroys_the_element()
  {
    const auto element{std::make_shared<int>(1)};

    data_structures::priority_queue<std::shared_ptr<int>, std::vector<std::shared_ptr<int>>, pointee_less> q{};
    q.push(std::make_shared<int>(0));
    q.push(element);
    q.pop();

    check(equality, "Popping destroys the priority queue's copy of the element", element.use_count(), 1L);
  }
}
