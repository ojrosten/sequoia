////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ExceptionSafetyTestUtilitiesFreeDiagnostics.hpp"
#include "sequoia/TestFramework/ExceptionSafetyTestUtilities.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <cstddef>
#include <initializer_list>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    /** \brief An int whose copy construction and copy assignment each take a
               fallible step.

        `copy` precedes `value`, so a failed copy assignment leaves the value
        unchanged.
     */
    struct fallible_int
    {
      fallible_copy copy{};
      int value{};

      [[nodiscard]]
      friend bool operator==(const fallible_int&, const fallible_int&) = default;
    };

    using fallible_ints_t      = std::vector<fallible_int>;
    using transition_checker_t = transition_checker<fallible_ints_t>;
    using graph_t              = transition_checker_t::transition_graph;

    [[nodiscard]]
    fallible_ints_t make_fallible_ints(std::initializer_list<int> values)
    {
      const auto toFallibleInt{[](const int v) { return fallible_int{.value{v}}; }};
      return values | std::views::transform(toFallibleInt) | std::ranges::to<std::vector>();
    }
  }

  template<>
  struct serializer<fallible_int>
  {
    [[nodiscard]]
    static std::string make(const fallible_int& x) { return std::to_string(x.value); }
  };

  [[nodiscard]]
  std::filesystem::path exception_safety_test_utilities_false_negative_free_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void exception_safety_test_utilities_false_negative_free_diagnostics::run_tests()
  {
    graph_t g{};
    g.add_node(make_fallible_ints({1, 2}));
    g.add_node(make_fallible_ints({2, 3}));

    // The mutation takes two steps against a prediction of three, and
    // swallows the failure at each, so every loop finds no exception and a
    // changed state
    add_fallible_transition<fallible_ints_t>(
      *this, g, "A mutation swallowing failures, its steps mispredicted", 0, 1, "Increment each value by copying", 3,
      [](fallible_ints_t& ints) {
        for(auto& element : ints)
        {
          try
          {
            const fallible_int incremented{.value{element.value + 1}};
            element = incremented;
          }
          catch(const injected_failure&)
          {}
        }
      }
    );

    // Only the loops' states are compared: the mutation succeeds when no
    // step fails, and a false-negative test admits no passing check
    transition_checker_t::check(
      report(""),
      g,
      [this](std::string_view description,
             const fallible_ints_t& obtained,
             const fallible_ints_t& prediction,
             const fallible_ints_t&,
             const std::size_t host,
             const std::size_t target) {
        if(host == target)
          check(equality, {description, no_source_location}, obtained, prediction);
      }
    );
  }

  [[nodiscard]]
  std::filesystem::path exception_safety_test_utilities_false_positive_free_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void exception_safety_test_utilities_false_positive_free_diagnostics::run_tests()
  {
    graph_t g{};
    g.add_node(make_fallible_ints({1, 2}));
    g.add_node(make_fallible_ints({1, 2, 1}));

    add_fallible_transition<fallible_ints_t>(
      *this, g, "A mutation with the strong guarantee", 0, 1, "Push a copy of the front", 1,
      [](fallible_ints_t& ints) { ints.push_back(ints.front()); }
    );

    transition_checker_t::check(
      report(""),
      g,
      [this](std::string_view description, const fallible_ints_t& obtained, const fallible_ints_t& prediction) {
        check(equality, {description, no_source_location}, obtained, prediction);
      }
    );
  }
}
