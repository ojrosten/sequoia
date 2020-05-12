////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

#include "MonotonicSequence.hpp"

namespace sequoia::unit_testing
{
  namespace impl
  {
    template<test_mode Mode, class T>
    void check(std::string_view description, test_logger<Mode>& logger, const T& sequence, const T& prediction)
    {
      if(check_equality(combine_messages(description, "Size incorrect"), logger, sequence.size(), prediction.size()))
      {
        if(!prediction.empty())
        {
          check_equality(combine_messages(description, "Back element wrong"), logger, sequence.back(), prediction.back());
          check_equality(combine_messages(description, "Front element wrong"), logger, sequence.front(), prediction.front());
        }
       
        auto i_prediction{prediction.begin()}, i{sequence.begin()};
        auto ci_prediction{prediction.cbegin()}, ci{sequence.cbegin()};
        auto ri_prediction{prediction.rbegin()}, ri{sequence.rbegin()};
        auto cri_prediction{prediction.crbegin()}, cri{sequence.crbegin()};

        for(;i_prediction != prediction.end(); ++i_prediction, ++i, ++ci_prediction, ++ci, ++ri_prediction, ++ri, ++cri_prediction, ++cri)
        {          
          using std::distance;
          const auto d{distance(prediction.begin(), i_prediction)};
          const std::string mess{" for index " + std::to_string(d)};
          
          check_equality(combine_messages(description, "Dereferenced iterator wrong" + mess), logger, *i, *i_prediction);
          check_equality(combine_messages(description, "Dereferenced citerator wrong" + mess), logger, *ci, *ci_prediction);

          check_equality(combine_messages(description, "operator[] wrong" + mess), logger, sequence[d], prediction[d]);  

          const auto shift{static_cast<int64_t>(prediction.size()) - d - 1};
          check_equality(combine_messages(description, "Dereferenced riterator wrong" + mess), logger, *(ri + shift), *(ri_prediction + shift));
          check_equality(combine_messages(description, "Dereferenced criterator wrong" + mess), logger, *(cri + shift), *(cri_prediction + shift));
        }
          
        unit_testing::check(combine_messages(description, "iterator location wrong"), logger, i_prediction == prediction.end());
        unit_testing::check(combine_messages(description, "citerator location wrong"), logger, ci_prediction == prediction.cend());
        unit_testing::check(combine_messages(description, "riterator location wrong"), logger, ri_prediction == prediction.rend());
        unit_testing::check(combine_messages(description, "criterator location wrong"), logger, cri_prediction == prediction.crend());
      }
    }
  }
  
  template<class T, class C, class Compare>
  struct detailed_equality_checker<maths::monotonic_sequence<T, C, Compare>>
  {
    using type = maths::monotonic_sequence<T, C, Compare>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& sequence, const type& prediction)
    {
      impl::check(description, logger, sequence, prediction);
    }
  };

  template<class T, class C, class Compare>
  struct equivalence_checker<maths::monotonic_sequence<T, C, Compare>>
  {
    using type = maths::monotonic_sequence<T, C, Compare>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& sequence, std::initializer_list<T> prediction)
    {
      check_range(description, logger, sequence.begin(), sequence.end(), prediction.begin(), prediction.end());            
    }
  };

  template<class T, std::size_t N, class Compare>
  struct detailed_equality_checker<maths::static_monotonic_sequence<T, N, Compare>>
  {
    using type = maths::static_monotonic_sequence<T, N, Compare>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& sequence, const type& prediction)
    {
      impl::check(description, logger, sequence, prediction);
    }
  };

  template<class T, std::size_t N, class Compare>
  struct equivalence_checker<maths::static_monotonic_sequence<T, N, Compare>>
  {
    using type = maths::static_monotonic_sequence<T, N, Compare>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& sequence, std::initializer_list<T> prediction)
    {
      check_range(description, logger, sequence.begin(), sequence.end(), prediction.begin(), prediction.end());            
    }
  };
}
