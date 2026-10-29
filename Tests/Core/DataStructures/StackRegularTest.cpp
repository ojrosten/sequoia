////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StackRegularTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    using stack_t = data_structures::stack<element_with_lineage>;
  }

  [[nodiscard]]
  std::filesystem::path stack_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void stack_regular_test::run_tests()
  {
    test_constant_evaluation();
    test_transitions();
  }

  void stack_regular_test::test_constant_evaluation()
  {
    STATIC_CHECK(elements_in_pop_order(make_by_pushing<data_structures::stack<int>>({1, 2, 3})) == std::vector{3, 2, 1});
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

        { edge_t{empty,
                 "Pop the only element",
                 [this](stack_t s) { pop_and_check_the_element_is_gone(*this, "Popping 1, the only element", s); return s; }},
          edge_t{one_two, "Push 2 onto 1", [](stack_t s) { s.push(2); return s; }}
        },

        { edge_t{one,
                 "Pop 2, leaving 1",
                 [this](stack_t s) { pop_and_check_the_element_is_gone(*this, "Popping 2 off 1", s); return s; }},
          edge_t{one_three,
                 "Pop 2 and push 3 in its place",
                 [this](stack_t s) {
                   pop_and_check_the_element_is_gone(*this, "Popping 2 off 1", s);
                   s.push(3);
                   return s;
                 }},
          edge_t{empty,
                 "Pop both",
                 [this](stack_t s) {
                   pop_and_check_the_element_is_gone(*this, "Popping 2 off 1", s);
                   pop_and_check_the_element_is_gone(*this, "Popping 1, the only element", s);
                   return s;
                 }}
        },

        { edge_t{one,
                 "Pop 3, leaving 1",
                 [this](stack_t s) { pop_and_check_the_element_is_gone(*this, "Popping 3 off 1", s); return s; }} }
      },
      {stack_t{}, make_by_pushing<stack_t>({1}), make_by_pushing<stack_t>({1, 2}), make_by_pushing<stack_t>({1, 3})}
    };

    auto checkerFn{
      [this](std::string_view description,
             const stack_t& obtained,
             const stack_t& prediction,
             const stack_t& parent,
             const std::size_t host,
             const std::size_t target) {
        check(equality, {description, no_source_location}, obtained, prediction);
        if(host != target) check_semantics({description, no_source_location}, prediction, parent);
      }
    };

    transition_checker_t::check(report("Stack transitions"), g, checkerFn);
  }
}
