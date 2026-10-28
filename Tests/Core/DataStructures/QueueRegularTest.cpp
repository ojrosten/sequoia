////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "QueueRegularTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <memory>

namespace sequoia::testing
{
  namespace
  {
    using queue_t = data_structures::queue<int>;

    [[nodiscard]]
    constexpr queue_t make_two_three_by_popping()
    {
      auto q{make_by_pushing<queue_t>({1, 2, 3})};
      q.pop();
      return q;
    }
  }

  [[nodiscard]]
  std::filesystem::path queue_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void queue_regular_test::run_tests()
  {
    test_constant_evaluation();
    test_semantics();
    test_transitions();
    test_pop_destroys_the_element();
  }

  void queue_regular_test::test_constant_evaluation()
  {
    STATIC_CHECK(elements_in_pop_order(make_by_pushing<queue_t>({1, 2, 3})) == std::vector{1, 2, 3});
    STATIC_CHECK(elements_in_pop_order(make_two_three_by_popping()) == std::vector{2, 3});
    STATIC_CHECK(make_by_pushing<queue_t>({1, 2}).back() == 2);
  }

  void queue_regular_test::test_semantics()
  {
    const queue_t empty{};
    const auto one{make_by_pushing<queue_t>({1})},
               oneTwo{make_by_pushing<queue_t>({1, 2})},
               oneTwoThree{make_by_pushing<queue_t>({1, 2, 3})};
    const auto twoThree{make_two_three_by_popping()};

    check(equivalence, "Empty", empty, std::vector<int>{});
    check(equivalence, "One element", one, std::vector<int>{1});
    check(equivalence, "Two elements, front first", oneTwo, std::vector<int>{1, 2});
    check(equivalence, "Three elements, front first", oneTwoThree, std::vector<int>{1, 2, 3});
    check(equivalence, "Two elements, reached by popping", twoThree, std::vector<int>{2, 3});

    check_semantics("Empty against one element", empty, one);
    check_semantics("One element against two", one, oneTwo);
    check_semantics("Two elements against three", oneTwo, oneTwoThree);
    check_semantics("Reached by popping against three elements", twoThree, oneTwoThree);
  }

  void queue_regular_test::test_transitions()
  {
    using transition_checker_t = transition_checker<queue_t>;
    using graph_t              = transition_checker_t::transition_graph;
    using edge_t               = graph_t::edge_type;

    enum node { empty, one, one_two, one_two_three, two_three, two_three_four, three_four, three };

    // make_two_three_by_popping generates two_three, so that its out-edges act
    // on a queue held differently from one made by pushing alone
    const graph_t g{
      { { edge_t{one, "Push 1 onto the empty queue", [](queue_t q) { q.push(1); return q; }},
          edge_t{empty,
                 "Popping the empty queue throws, leaving it empty",
                 [this](queue_t q) {
                   check_exception_thrown<std::logic_error>("Popping the empty queue", [&q]() { q.pop(); });
                   return q;
                 }},
          edge_t{empty,
                 "Accessing the front of the empty queue throws",
                 [this](queue_t q) {
                   check_exception_thrown<std::logic_error>("Accessing the front of the empty queue",
                                                            [&q]() { return q.front(); });
                   return q;
                 }},
          edge_t{empty,
                 "Accessing the back of the empty queue throws",
                 [this](queue_t q) {
                   check_exception_thrown<std::logic_error>("Accessing the back of the empty queue",
                                                            [&q]() { return q.back(); });
                   return q;
                 }}
        },

        { edge_t{empty,   "Pop the only element", [](queue_t q) { q.pop(); return q; }},
          edge_t{one_two, "Push 2 behind 1",      [](queue_t q) { q.push(2); return q; }}
        },

        { edge_t{one_two_three, "Push 3 behind 2", [](queue_t q) { q.push(3); return q; }},
          edge_t{empty,         "Pop both",        [](queue_t q) { q.pop(); q.pop(); return q; }}
        },

        { edge_t{two_three, "Pop 1, leaving 2 and 3", [](queue_t q) { q.pop(); return q; }} },

        { edge_t{two_three_four, "Push 4 behind 3", [](queue_t q) { q.push(4); return q; }},
          edge_t{three,          "Pop 2, leaving 3", [](queue_t q) { q.pop(); return q; }},
          edge_t{empty,          "Pop both",         [](queue_t q) { q.pop(); q.pop(); return q; }}
        },

        { edge_t{three_four, "Pop 2, leaving 3 and 4", [](queue_t q) { q.pop(); return q; }} },

        { edge_t{empty, "Pop both", [](queue_t q) { q.pop(); q.pop(); return q; }} },

        { edge_t{empty, "Pop the only element", [](queue_t q) { q.pop(); return q; }} }
      },
      {queue_t{},
       make_by_pushing<queue_t>({1}),
       make_by_pushing<queue_t>({1, 2}),
       make_by_pushing<queue_t>({1, 2, 3}),
       make_two_three_by_popping(),
       make_by_pushing<queue_t>({2, 3, 4}),
       make_by_pushing<queue_t>({3, 4}),
       make_by_pushing<queue_t>({3})}
    };

    auto checkerFn{
      [this](std::string description, const queue_t& obtained, const queue_t& prediction) {
        check(equality, description, obtained, prediction);
      }
    };

    transition_checker_t::check(report("Queue transitions"), g, checkerFn);
  }

  void queue_regular_test::test_pop_destroys_the_element()
  {
    const auto element{std::make_shared<int>(1)};

    data_structures::queue<std::shared_ptr<int>> q{};
    q.push(element);
    q.push(std::make_shared<int>(2));
    q.push(std::make_shared<int>(3));
    q.pop();

    check(equality, "Popping destroys the queue's copy of the element", element.use_count(), 1L);
  }
}
