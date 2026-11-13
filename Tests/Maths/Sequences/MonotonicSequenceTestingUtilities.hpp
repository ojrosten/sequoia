////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Maths/Sequences/MonotonicSequence.hpp"

#include <ranges>
#include <vector>

namespace sequoia::testing
{
  namespace impl
  {
    template<class Sequence>
    struct monotonic_sequence_tester
    {
      using type = Sequence;

      template<test_mode Mode>
      static void test(equality_check_t,
                       test_logger<Mode>& logger,
                       const Sequence& sequence,
                       const Sequence& prediction)
      {
        check(equality, "Emptiness incorrect", logger, sequence.empty(), prediction.empty());

        if(check(equality, "Size incorrect", logger, sequence.size(), prediction.size()))
        {
          if(!prediction.empty())
          {
            check(equality, "Back element wrong", logger, sequence.back(), prediction.back());
            check(equality, "Front element wrong", logger, sequence.front(), prediction.front());
          }

          check(equality,
                "iterator range wrong",
                logger,
                sequence.begin(), sequence.end(),
                prediction.begin(), prediction.end());
          check(equality,
                "citerator range wrong",
                logger,
                sequence.cbegin(), sequence.cend(),
                prediction.cbegin(), prediction.cend());
          check(equality,
                "riterator range wrong",
                logger,
                sequence.rbegin(), sequence.rend(),
                prediction.rbegin(), prediction.rend());
          check(equality,
                "criterator range wrong",
                logger,
                sequence.crbegin(), sequence.crend(),
                prediction.crbegin(), prediction.crend());

          auto elementAt{[&sequence](const auto i) { return sequence[i]; }};
          const auto elementsBySubscript{
              std::views::iota(typename Sequence::size_type{}, sequence.size())
            | std::views::transform(elementAt)
            | std::ranges::to<std::vector>()
          };

          check(equality,
                "operator[] wrong",
                logger,
                elementsBySubscript.begin(), elementsBySubscript.end(),
                prediction.begin(), prediction.end());
        }
      }

      template<test_mode Mode>
      static void test(equivalence_check_t,
                       test_logger<Mode>& logger,
                       const Sequence& sequence,
                       std::initializer_list<typename Sequence::value_type> prediction)
      {
        check(equality, "", logger, sequence.begin(), sequence.end(), prediction.begin(), prediction.end());
      }
    };
  }

  template<class T, class Compare, class C>
  struct value_tester<maths::monotonic_sequence<T, Compare, C>>
    : impl::monotonic_sequence_tester<maths::monotonic_sequence<T, Compare, C>>
  {};

  template<class T, std::size_t N, class Compare>
  struct value_tester<maths::static_monotonic_sequence<T, N, Compare>>
    : impl::monotonic_sequence_tester<maths::static_monotonic_sequence<T, N, Compare>>
  {};
}
