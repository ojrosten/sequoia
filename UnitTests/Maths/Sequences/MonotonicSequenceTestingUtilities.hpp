////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

#include "MonotonicSequence.hpp"

namespace sequoia::unit_testing
{
  template<class T, class C, class Compare>
  struct details_checker<maths::monotonic_sequence<T, C, Compare>>
  {
    using type = maths::monotonic_sequence<T, C, Compare>;
    
    template<class Logger>
    static void check(Logger& logger, const type& reference, const type& actual, std::string_view description="")
    {
      if(check_equality(logger, reference.size(), actual.size(), impl::concat_messages(description, "Size incorrect")))
      {
        auto i_ref{reference.begin()}, i_act{actual.begin()};
        auto ci_ref{reference.cbegin()}, ci_act{actual.cbegin()};
        auto ri_ref{reference.rbegin()}, ri_act{actual.rbegin()};
        auto cri_ref{reference.crbegin()}, cri_act{actual.crbegin()};

        for(;i_ref != reference.end(); ++i_ref, ++i_act, ++ci_ref, ++ci_act, ++ri_ref, ++ri_act, ++cri_ref, ++cri_act)
        {          
          using std::distance;
          const auto i{distance(reference.begin(), i_ref)};
          const std::string mess{" for index " + std::to_string(i)};
          
          check_equality(logger, *i_ref, *i_act, impl::concat_messages(description, "Dereferenced iterator wrong" + mess));
          check_equality(logger, *ci_ref, *ci_act, impl::concat_messages(description, "Dereferenced citerator wrong" + mess));

          check_equality(logger, reference[i], actual[i], impl::concat_messages(description, "operator[] wrong" + mess));  

          const auto shift{static_cast<int64_t>(reference.size()) - i - 1};
          check_equality(logger, *(ri_ref + shift), *(ri_act + shift), impl::concat_messages(description, "Dereferenced riterator wrong" + mess));
          check_equality(logger, *(cri_ref + shift), *(cri_act + shift), impl::concat_messages(description, "Dereferenced criterator wrong" + mess));
        }
          
        unit_testing::check(logger, i_ref == reference.end(), "iterator location wrong");
        unit_testing::check(logger, ci_ref == reference.cend(), "citerator location wrong");
        unit_testing::check(logger, ri_ref == reference.rend(), "riterator location wrong");
        unit_testing::check(logger, cri_ref == reference.crend(), "criterator location wrong");
      }
    }
  };

  template<class T, class C, class Compare>
  struct equivalence_checker<maths::monotonic_sequence<T, C, Compare>>
  {
    using type = maths::monotonic_sequence<T, C, Compare>;
    
    template<class Logger>
    static void check(Logger& logger, const type& sequence, std::initializer_list<T> refVals, std::string_view description="")
    {
      check_range(logger, sequence.begin(), sequence.end(), refVals.begin(), refVals.end(), description);            
    }
  };

  template<class T, std::size_t N, class Compare>
  struct equivalence_checker<maths::static_monotonic_sequence<T, N, Compare>>
  {
    using type = maths::static_monotonic_sequence<T, N, Compare>;
    
    template<class Logger>
    static void check(Logger& logger, const type& sequence, std::initializer_list<T> refVals, std::string_view description="")
    {
      check_range(logger, sequence.begin(), sequence.end(), refVals.begin(), refVals.end(), description);            
    }
  };
}
