////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Core/DataStructures/Queue.hpp"

#include <initializer_list>
#include <vector>

namespace sequoia::testing
{
  /// A queue holding the elements, front first, since the type is built only by pushing
  template<class T>
  [[nodiscard]]
  constexpr data_structures::queue<T> make_queue(std::initializer_list<T> elements)
  {
    data_structures::queue<T> q{};
    for(const auto& element : elements)
    {
      q.push(element);
    }

    return q;
  }

  template<class T>
  struct value_tester<data_structures::queue<T>>
  {
    using type = data_structures::queue<T>;

    template<test_mode Mode>
    constexpr static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Emptiness incorrect", logger, actual.empty(), prediction.empty());
      check(equality, "Size incorrect", logger, actual.size(), prediction.size());

      if(!actual.empty() && !prediction.empty())
      {
        check(equality, "Front element incorrect", logger, actual.front(), prediction.front());
        check(equality, "Back element incorrect", logger, actual.back(), prediction.back());
      }

      check("Hidden state incorrect", logger, actual == prediction);
      check("Hidden state, symmetry of operator== incorrect", logger, prediction == actual);
    }

    /// Equivalent to the elements it would yield, front first
    template<test_mode Mode>
    constexpr static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::vector<T>& prediction)
    {
      check(equality, "Emptiness incorrect", logger, actual.empty(), prediction.empty());
      check(equality, "Size incorrect", logger, actual.size(), prediction.size());

      if(!actual.empty() && !prediction.empty())
      {
        check(equality, "Back element incorrect", logger, actual.back(), prediction.back());
      }

      auto remaining{actual};
      for(const auto& element : prediction)
      {
        if(remaining.empty())
          break;

        check(equality, "Element incorrect", logger, remaining.front(), element);
        remaining.pop();
      }
    }
  };
}
