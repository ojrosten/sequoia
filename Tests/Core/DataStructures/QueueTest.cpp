////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "QueueTest.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  using namespace data_structures;

  namespace
  {
    /// The states of a queue of ints and the transitions between them, as a function of a logger
    template<test_mode Mode>
    constexpr void walk_queue(test_logger<Mode>& logger)
    {
      using queue_t = queue<int>;
      using checker = transition_checker<queue_t, check_ordering::no>;
      using graph_t = checker::transition_graph;
      using edge_t  = graph_t::edge_type;

      enum node { empty, one, one_two, two, three };

      // The transitions compose, so that a queue reached by popping - its head beyond the start
      // of its elements - is popped from, pushed onto and emptied in turn
      const graph_t g{
        { { edge_t{one, "Push 1 onto the empty queue", [](const queue_t& q) { auto r{q}; r.push(1); return r; }} },

          { edge_t{empty,   "Pop the only element", [](const queue_t& q) { auto r{q}; r.pop(); return r; }},
            edge_t{one_two, "Push 2 behind 1",      [](const queue_t& q) { auto r{q}; r.push(2); return r; }}
          },

          { edge_t{two,   "Pop 1, leaving 2 at the front", [](const queue_t& q) { auto r{q}; r.pop(); return r; }},
            edge_t{empty, "Pop both",                      [](const queue_t& q) { auto r{q}; r.pop(); r.pop(); return r; }}
          },

          { edge_t{empty, "Pop 2, emptying a queue which was popped before", [](const queue_t& q) { auto r{q}; r.pop(); return r; }},
            edge_t{three, "Pop 2 and push 3 onto the emptied queue",         [](const queue_t& q) { auto r{q}; r.pop(); r.push(3); return r; }}
          },

          { edge_t{one_two, "Push 1 and 2 behind 3, then pop 3", [](const queue_t& q) { auto r{q}; r.push(1); r.push(2); r.pop(); return r; }} }
        },
        {queue_t{}, make_queue({1}), make_queue({1, 2}), make_queue({2}), make_queue({3})}
      };

      const auto checkerFn{
        [&logger](std::string_view description, const queue_t& obtained, const queue_t& prediction) {
          check(equality, std::string{description}, logger, obtained, prediction);
        }
      };

      checker::check("Queue transitions", g, checkerFn);
    }
  }

  [[nodiscard]]
  std::filesystem::path queue_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void queue_test::run_tests()
  {
    test_semantics();
    test_transitions();
  }

  void queue_test::test_semantics()
  {
    const queue<int> empty{};
    const auto one{make_queue({1})}, oneTwo{make_queue({1, 2})}, oneTwoThree{make_queue({1, 2, 3})};

    check(equivalence, "Empty", empty, std::vector<int>{});
    check(equivalence, "One element", one, std::vector<int>{1});
    check(equivalence, "Two elements, front first", oneTwo, std::vector<int>{1, 2});
    check(equivalence, "Three elements, front first", oneTwoThree, std::vector<int>{1, 2, 3});

    check_semantics("Empty against one element", empty, one);
    check_semantics("One element against two", one, oneTwo);
    check_semantics("Two elements against three", oneTwo, oneTwoThree);

    // A queue which reached a state by popping equals one built in that state: the head is
    // hidden state
    auto popped{make_queue({1, 2, 3})};
    popped.pop();
    check(equality, "Popped to the same state as built", popped, make_queue({2, 3}));
    check_semantics("Popped against built with more", popped, oneTwoThree);

    popped.pop();
    popped.pop();
    check(equality, "Popped to empty", popped, empty);
    popped.push(4);
    check(equivalence, "Pushed after being emptied", popped, std::vector<int>{4});
  }

  void queue_test::test_transitions()
  {
    EVALUATE_STATICALLY_AND_DYNAMICALLY([](auto& logger){ walk_queue(logger); });
  }
}
