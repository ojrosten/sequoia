////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include <initializer_list>
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
  inline constexpr bool has_front_v{requires(const Adaptor& adaptor) { adaptor.front(); }};

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
      if constexpr(has_front_v<Adaptor>)
        elements.push_back(adaptor.front());
      else
        elements.push_back(adaptor.top());

      adaptor.pop();
    }

    return elements;
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
