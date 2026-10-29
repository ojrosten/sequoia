////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "ElementTestingUtilities.hpp"

#include "sequoia/Core/ContainerUtilities/ContainerAdaptorUtilities.hpp"
#include "sequoia/TestFramework/RegularTestCore.hpp"

#include <concepts>
#include <cstddef>
#include <format>
#include <initializer_list>
#include <ranges>
#include <string_view>
#include <vector>

namespace sequoia::testing
{
  /// Pushes `elements`, first to last, into an empty `Adaptor`, and returns it
  template<class Adaptor>
  [[nodiscard]]
  constexpr Adaptor make_by_pushing(std::initializer_list<typename Adaptor::value_type> elements)
  {
    Adaptor adaptor{};
    for(const auto& element : elements)
    {
      adaptor.push(element);
    }

    return adaptor;
  }

  template<class Adaptor>
  inline constexpr bool has_back_v{requires(const Adaptor& adaptor) { adaptor.back(); }};

  /// Returns the elements of `adaptor`, in the order pops remove them
  template<class Adaptor>
  [[nodiscard]]
  constexpr std::vector<typename Adaptor::value_type> elements_in_pop_order(Adaptor adaptor)
  {
    std::vector<typename Adaptor::value_type> elements{};
    while(!adaptor.empty())
    {
      elements.push_back(utilities::element_popped_next(adaptor));
      adaptor.pop();
    }

    return elements;
  }

  /** \brief Pops `adaptor`, and checks that the pop neither copies the element
             it removes nor leaves any object in that element's lineage.
   */
  template<class Adaptor>
    requires std::same_as<typename Adaptor::value_type, element_with_lineage>
  void pop_and_check_the_element_is_gone(regular_test& test, Adaptor& adaptor)
  {
    const auto observer{utilities::element_popped_next(adaptor).lineage.observer()};

    // Listing the elements copies them, so it precedes reading the count of
    // copies
    const auto values{
        elements_in_pop_order(adaptor)
      | std::views::transform(&element_with_lineage::value)
      | std::ranges::to<std::vector>()
    };

    const auto copies{utilities::element_popped_next(adaptor).lineage.copies()};
    const std::size_t copiesBefore{*copies};
    adaptor.pop();

    const auto popping{std::format("Popping from {}, in pop order", values)};
    test.check(equality, {append_lines(popping, "The pop makes no copy of the element"), no_source_location}, *copies, copiesBefore);
    test.check({append_lines(popping, "The pop leaves no object in the element's lineage"), no_source_location}, observer.expired());
  }

  /** \brief Returns a transition which takes an adaptor and pops it `pops`
             times, checking each pop with `pop_and_check_the_element_is_gone`.
   */
  template<class Adaptor>
    requires std::same_as<typename Adaptor::value_type, element_with_lineage>
  [[nodiscard]]
  auto transition_of_pops(regular_test& test, const std::size_t pops)
  {
    return [&test, pops](Adaptor adaptor) {
      for(std::size_t i{}; i < pops; ++i)
      {
        pop_and_check_the_element_is_gone(test, adaptor);
      }

      return adaptor;
    };
  }

  /** \brief Returns a CheckFn for a transition graph, checking equality on
             every edge, and `check_semantics` on every edge which is not a
             self-loop.
   */
  template<class T>
  [[nodiscard]]
  auto equality_and_semantics_check_fn(regular_test& test)
  {
    return [&test](std::string_view description,
                   const T& obtained,
                   const T& prediction,
                   const T& parent,
                   const std::size_t host,
                   const std::size_t target) {
      test.check(equality, {description, no_source_location}, obtained, prediction);
      if(host != target) test.check_semantics({description, no_source_location}, prediction, parent);
    };
  }

  /** \brief A `value_tester` for a stack, a queue or a priority queue.

      Equivalence is with the elements, in the order pops remove them.
   */
  template<class Adaptor>
  struct container_adaptor_value_tester
  {
    using type       = Adaptor;
    using value_type = Adaptor::value_type;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      test(equivalence, logger, actual, elements_in_pop_order(prediction));
    }

    template<test_mode Mode>
    static void test(equivalence_check_t,
                     test_logger<Mode>& logger,
                     const type& actual,
                     const std::vector<value_type>& prediction)
    {
      check(equality, "Emptiness incorrect", logger, actual.empty(), prediction.empty());
      check(equality, "Size incorrect", logger, actual.size(), prediction.size());
      check(equality, "Elements incorrect, in the order popped", logger, elements_in_pop_order(actual), prediction);

      if constexpr(has_back_v<Adaptor>)
      {
        if(!actual.empty() && !prediction.empty())
          check(equality, "Back element incorrect", logger, actual.back(), prediction.back());
      }
    }
  };
}
