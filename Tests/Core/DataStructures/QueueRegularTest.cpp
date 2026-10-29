////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "QueueRegularTest.hpp"

#include "sequoia/TestFramework/ExceptionSafetyTestUtilities.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <string>
#include <string_view>
#include <type_traits>

namespace sequoia::testing
{
  namespace
  {
    template<class T>
    inline constexpr bool queue_admits_v{requires { typename data_structures::queue<T>; }};

    // A std::vector admits an incomplete element type, so a queue must admit
    // one too
    struct with_queue_of_itself
    {
      data_structures::queue<with_queue_of_itself> nested{};
    };

    using queue_t = data_structures::queue<element_with_lineage>;

    template<class Queue>
    [[nodiscard]]
    constexpr Queue make_two_three_by_popping()
    {
      auto q{make_by_pushing<Queue>({1, 2, 3})};
      q.pop();
      return q;
    }

    /** \brief An element whose copy is a fallible step, and whose move is
               not declared `noexcept`.
     */
    struct element_with_fallible_copy
    {
      int value{};
      fallible_copy copy{};

      element_with_fallible_copy(const int initialValue)
        : value{initialValue}
      {}

      element_with_fallible_copy(const element_with_fallible_copy&)                       = default;
      element_with_fallible_copy(element_with_fallible_copy&&) noexcept(false)            = default;
      element_with_fallible_copy& operator=(const element_with_fallible_copy&)            = default;
      element_with_fallible_copy& operator=(element_with_fallible_copy&&) noexcept(false) = default;

      [[nodiscard]]
      friend bool operator==(const element_with_fallible_copy&, const element_with_fallible_copy&) = default;
    };
  }

  template<>
  struct serializer<element_with_fallible_copy>
  {
    [[nodiscard]]
    static std::string make(const element_with_fallible_copy& x) { return std::to_string(x.value); }
  };

  [[nodiscard]]
  std::filesystem::path queue_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void queue_regular_test::run_tests()
  {
    test_constraints();
    test_constant_evaluation();
    test_transitions();
    test_exception_safety_of_pop();
  }

  void queue_regular_test::test_constraints()
  {
    STATIC_CHECK( queue_admits_v<int>);
    STATIC_CHECK(std::is_default_constructible_v<with_queue_of_itself>);

    // A const std::vector<bool> returns no reference to its elements
    STATIC_CHECK(!queue_admits_v<bool>);
  }

  void queue_regular_test::test_constant_evaluation()
  {
    using int_queue_t = data_structures::queue<int>;

    STATIC_CHECK(elements_in_pop_order(make_by_pushing<int_queue_t>({1, 2, 3})) == std::vector{1, 2, 3});
    STATIC_CHECK(elements_in_pop_order(make_two_three_by_popping<int_queue_t>()) == std::vector{2, 3});
    STATIC_CHECK(make_by_pushing<int_queue_t>({1, 2}).back() == 2);
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

        { edge_t{empty,   "Pop the only element", transition_of_pops<queue_t>(*this, 1)},
          edge_t{one_two, "Push 2 behind 1",      [](queue_t q) { q.push(2); return q; }}
        },

        { edge_t{one_two_three, "Push 3 behind 2", [](queue_t q) { q.push(3); return q; }},
          edge_t{empty,         "Pop both",        transition_of_pops<queue_t>(*this, 2)}
        },

        { edge_t{two_three, "Pop 1, leaving 2 and 3", transition_of_pops<queue_t>(*this, 1)} },

        { edge_t{two_three_four, "Push 4 behind 3",  [](queue_t q) { q.push(4); return q; }},
          edge_t{three,          "Pop 2, leaving 3", transition_of_pops<queue_t>(*this, 1)},
          edge_t{empty,          "Pop both",         transition_of_pops<queue_t>(*this, 2)}
        },

        { edge_t{three_four, "Pop 2, leaving 3 and 4", transition_of_pops<queue_t>(*this, 1)} },

        { edge_t{empty, "Pop both", transition_of_pops<queue_t>(*this, 2)} },

        { edge_t{empty, "Pop the only element", transition_of_pops<queue_t>(*this, 1)} }
      },
      {queue_t{},
       make_by_pushing<queue_t>({1}),
       make_by_pushing<queue_t>({1, 2}),
       make_by_pushing<queue_t>({1, 2, 3}),
       make_two_three_by_popping<queue_t>(),
       make_by_pushing<queue_t>({2, 3, 4}),
       make_by_pushing<queue_t>({3, 4}),
       make_by_pushing<queue_t>({3})}
    };

    transition_checker_t::check(report("Queue transitions"), g, equality_and_semantics_check_fn<queue_t>(*this));
  }

  void queue_regular_test::test_exception_safety_of_pop()
  {
    using queue_with_fallible_copies_t = data_structures::queue<element_with_fallible_copy>;
    using transition_checker_t         = transition_checker<queue_with_fallible_copies_t>;
    using graph_t                      = transition_checker_t::transition_graph;

    enum node { one_two_three, two_three };

    graph_t g{};
    g.add_node(make_by_pushing<queue_with_fallible_copies_t>({1, 2, 3}));
    g.add_node(make_by_pushing<queue_with_fallible_copies_t>({2, 3}));

    // The element's move is not noexcept, so the queue refills by copying;
    // this pop is the first, so it copies all three elements
    const std::string_view context{"Queue of elements with fallible copies"};
    add_fallible_transition<queue_with_fallible_copies_t>(
      *this, g, context, one_two_three, two_three, "Pop 1, leaving 2 and 3", 3,
      [](queue_with_fallible_copies_t& q) { q.pop(); }
    );

    transition_checker_t::check(report(context),
                                g,
                                equality_and_semantics_check_fn<queue_with_fallible_copies_t>(*this));
  }
}
