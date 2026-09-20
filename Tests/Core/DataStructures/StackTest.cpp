////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "StackTest.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  using namespace data_structures;

  namespace
  {
    /// The states of a stack of ints and the transitions between them, as a function of a logger
    template<test_mode Mode>
    constexpr void walk_stack(test_logger<Mode>& logger)
    {
      using stack_t = stack<int>;
      using checker = transition_checker<stack_t, check_ordering::no>;
      using graph_t = checker::transition_graph;
      using edge_t  = graph_t::edge_type;

      enum node { empty, one, one_two, one_three };

      const graph_t g{
        { { edge_t{one, "Push 1 onto the empty stack", [](const stack_t& s) { auto r{s}; r.push(1); return r; }} },

          { edge_t{empty,   "Pop the only element", [](const stack_t& s) { auto r{s}; r.pop(); return r; }},
            edge_t{one_two, "Push 2 onto 1",        [](const stack_t& s) { auto r{s}; r.push(2); return r; }}
          },

          { edge_t{one,       "Pop 2, leaving 1",              [](const stack_t& s) { auto r{s}; r.pop(); return r; }},
            edge_t{one_three, "Pop 2 and push 3 in its place", [](const stack_t& s) { auto r{s}; r.pop(); r.push(3); return r; }},
            edge_t{empty,     "Pop both",                      [](const stack_t& s) { auto r{s}; r.pop(); r.pop(); return r; }}
          },

          { edge_t{one, "Pop 3, leaving 1", [](const stack_t& s) { auto r{s}; r.pop(); return r; }} }
        },
        {stack_t{}, make_stack({1}), make_stack({1, 2}), make_stack({1, 3})}
      };

      const auto checkerFn{
        [&logger](std::string_view description, const stack_t& obtained, const stack_t& prediction) {
          check(equality, std::string{description}, logger, obtained, prediction);
        }
      };

      checker::check("Stack transitions", g, checkerFn);
    }
  }

  [[nodiscard]]
  std::filesystem::path stack_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void stack_test::run_tests()
  {
    test_semantics();
    test_transitions();
  }

  void stack_test::test_semantics()
  {
    const stack<int> empty{};
    const auto one{make_stack({1})}, oneTwo{make_stack({1, 2})}, oneTwoThree{make_stack({1, 2, 3})};

    check(equivalence, "Empty", empty, std::vector<int>{});
    check(equivalence, "One element", one, std::vector<int>{1});
    check(equivalence, "Two elements, top first", oneTwo, std::vector<int>{2, 1});
    check(equivalence, "Three elements, top first", oneTwoThree, std::vector<int>{3, 2, 1});

    check_semantics("Empty against one element", empty, one);
    check_semantics("One element against two", one, oneTwo);
    check_semantics("Two elements against three", oneTwo, oneTwoThree);
  }

  void stack_test::test_transitions()
  {
    EVALUATE_STATICALLY_AND_DYNAMICALLY([](auto& logger){ walk_stack(logger); });
  }
}
