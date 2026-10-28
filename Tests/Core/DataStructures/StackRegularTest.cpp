////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StackRegularTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <memory>

namespace sequoia::testing
{
  namespace
  {
    using stack_t = data_structures::stack<int>;
  }

  [[nodiscard]]
  std::filesystem::path stack_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void stack_regular_test::run_tests()
  {
    test_constant_evaluation();
    test_semantics();
    test_transitions();
    test_pop_destroys_the_element();
  }

  void stack_regular_test::test_constant_evaluation()
  {
    STATIC_CHECK(elements_in_pop_order(make_by_pushing<stack_t>({1, 2, 3})) == std::vector{3, 2, 1});
  }

  void stack_regular_test::test_semantics()
  {
    const stack_t empty{};
    const auto one{make_by_pushing<stack_t>({1})},
               oneTwo{make_by_pushing<stack_t>({1, 2})},
               oneTwoThree{make_by_pushing<stack_t>({1, 2, 3})};

    check(equivalence, "Empty", empty, std::vector<int>{});
    check(equivalence, "One element", one, std::vector<int>{1});
    check(equivalence, "Two elements, top first", oneTwo, std::vector<int>{2, 1});
    check(equivalence, "Three elements, top first", oneTwoThree, std::vector<int>{3, 2, 1});

    check_semantics("Empty against one element", empty, one);
    check_semantics("One element against two", one, oneTwo);
    check_semantics("Two elements against three", oneTwo, oneTwoThree);
  }

  void stack_regular_test::test_transitions()
  {
    using transition_checker_t = transition_checker<stack_t>;
    using graph_t              = transition_checker_t::transition_graph;
    using edge_t               = graph_t::edge_type;

    enum node { empty, one, one_two, one_three };

    const graph_t g{
      { { edge_t{one,   "Push 1 onto the empty stack", [](stack_t s) { s.push(1); return s; }},
          edge_t{empty,
                 "Popping the empty stack throws, leaving it empty",
                 [this](stack_t s) {
                   check_exception_thrown<std::logic_error>("Popping the empty stack", [&s]() { s.pop(); });
                   return s;
                 }},
          edge_t{empty,
                 "Accessing the top of the empty stack throws",
                 [this](stack_t s) {
                   check_exception_thrown<std::logic_error>("Accessing the top of the empty stack",
                                                            [&s]() { return s.top(); });
                   return s;
                 }}
        },

        { edge_t{empty,   "Pop the only element", [](stack_t s) { s.pop(); return s; }},
          edge_t{one_two, "Push 2 onto 1",        [](stack_t s) { s.push(2); return s; }}
        },

        { edge_t{one,       "Pop 2, leaving 1",              [](stack_t s) { s.pop(); return s; }},
          edge_t{one_three, "Pop 2 and push 3 in its place", [](stack_t s) { s.pop(); s.push(3); return s; }},
          edge_t{empty,     "Pop both",                      [](stack_t s) { s.pop(); s.pop(); return s; }}
        },

        { edge_t{one, "Pop 3, leaving 1", [](stack_t s) { s.pop(); return s; }} }
      },
      {stack_t{}, make_by_pushing<stack_t>({1}), make_by_pushing<stack_t>({1, 2}), make_by_pushing<stack_t>({1, 3})}
    };

    auto checkerFn{
      [this](std::string description, const stack_t& obtained, const stack_t& prediction) {
        check(equality, description, obtained, prediction);
      }
    };

    transition_checker_t::check(report("Stack transitions"), g, checkerFn);
  }

  void stack_regular_test::test_pop_destroys_the_element()
  {
    const auto element{std::make_shared<int>(1)};

    data_structures::stack<std::shared_ptr<int>> s{};
    s.push(std::make_shared<int>(0));
    s.push(element);
    s.pop();

    check(equality, "Popping destroys the stack's copy of the element", element.use_count(), 1L);
  }
}
