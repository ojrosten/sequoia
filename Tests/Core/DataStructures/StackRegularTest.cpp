////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StackRegularTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <type_traits>

namespace sequoia::testing
{
  namespace
  {
    template<class T>
    inline constexpr bool stack_admits_v{requires { typename data_structures::stack<T>; }};

    // A std::vector admits an incomplete element type, so a stack must admit
    // one too. An element type holding a stack of itself must be able to
    // default its ==.
    struct with_stack_of_itself
    {
      data_structures::stack<with_stack_of_itself> nested{};

      [[nodiscard]]
      friend constexpr bool operator==(const with_stack_of_itself&, const with_stack_of_itself&) = default;
    };

    using stack_t = data_structures::stack<element_with_lineage>;
  }

  [[nodiscard]]
  std::filesystem::path stack_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void stack_regular_test::run_tests()
  {
    test_constraints();
    test_constant_evaluation();
    test_transitions();
  }

  void stack_regular_test::test_constraints()
  {
    STATIC_CHECK( stack_admits_v<int>);
    STATIC_CHECK(std::is_default_constructible_v<with_stack_of_itself>);
    STATIC_CHECK(with_stack_of_itself{} == with_stack_of_itself{});

    // A const std::vector<bool> returns no reference to its elements
    STATIC_CHECK(!stack_admits_v<bool>);
  }

  void stack_regular_test::test_constant_evaluation()
  {
    using int_stack_t = data_structures::stack<int>;

    STATIC_CHECK(elements_in_pop_order(make_by_pushing<int_stack_t>({1, 2, 3})) == std::vector{3, 2, 1});
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

        { edge_t{empty,   "Pop the only element", transition_of_pops<stack_t>(*this, 1)},
          edge_t{one_two, "Push 2 onto 1",        [](stack_t s) { s.push(2); return s; }}
        },

        { edge_t{one, "Pop 2, leaving 1", transition_of_pops<stack_t>(*this, 1)},
          edge_t{one_three,
                 "Pop 2 and push 3 in its place",
                 [this](stack_t s) {
                   pop_and_check_the_element_is_gone(*this, s);
                   s.push(3);
                   return s;
                 }},
          edge_t{empty, "Pop both", transition_of_pops<stack_t>(*this, 2)}
        },

        { edge_t{one, "Pop 3, leaving 1", transition_of_pops<stack_t>(*this, 1)} }
      },
      {stack_t{}, make_by_pushing<stack_t>({1}), make_by_pushing<stack_t>({1, 2}), make_by_pushing<stack_t>({1, 3})}
    };

    transition_checker_t::check(report("Stack transitions"), g, equality_and_semantics_check_fn<stack_t>(*this));
  }
}
