////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

#include "MonotonicSequence.hpp"

namespace sequoia::unit_testing
{
  namespace impl
  {
    template<class Logger, class T>
    void check(Logger& logger, const T& sequence, const T& prediction, std::string_view description)
    {
      if(check_equality(logger, sequence.size(), prediction.size(), combine_messages(description, "Size incorrect")))
      {
        if(!prediction.empty())
        {
          check_equality(logger, sequence.back(), prediction.back(), combine_messages(description, "Back element wrong"));
          check_equality(logger, sequence.front(), prediction.front(), combine_messages(description, "Front element wrong"));
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
          
          check_equality(logger, *i, *i_prediction, combine_messages(description, "Dereferenced iterator wrong" + mess));
          check_equality(logger, *ci, *ci_prediction, combine_messages(description, "Dereferenced citerator wrong" + mess));

          check_equality(logger, sequence[d], prediction[d], combine_messages(description, "operator[] wrong" + mess));  

          const auto shift{static_cast<int64_t>(prediction.size()) - d - 1};
          check_equality(logger, *(ri + shift), *(ri_prediction + shift), combine_messages(description, "Dereferenced riterator wrong" + mess));
          check_equality(logger, *(cri + shift), *(cri_prediction + shift), combine_messages(description, "Dereferenced criterator wrong" + mess));
        }
          
        unit_testing::check(logger, i_prediction == prediction.end(), "iterator location wrong");
        unit_testing::check(logger, ci_prediction == prediction.cend(), "citerator location wrong");
        unit_testing::check(logger, ri_prediction == prediction.rend(), "riterator location wrong");
        unit_testing::check(logger, cri_prediction == prediction.crend(), "criterator location wrong");
      }
    }
  }
  
  template<class T, class C, class Compare>
  struct detailed_equality_checker<maths::monotonic_sequence<T, C, Compare>>
  {
    using type = maths::monotonic_sequence<T, C, Compare>;
    
    template<class Logger>
    static void check(Logger& logger, const type& sequence, const type& prediction, std::string_view description)
    {
      impl::check(logger, sequence, prediction, description);
    }
  };

  template<class T, class C, class Compare>
  struct equivalence_checker<maths::monotonic_sequence<T, C, Compare>>
  {
    using type = maths::monotonic_sequence<T, C, Compare>;
    
    template<class Logger>
    static void check(Logger& logger, const type& sequence, std::initializer_list<T> prediction, std::string_view description)
    {
      check_range(logger, sequence.begin(), sequence.end(), prediction.begin(), prediction.end(), description);            
    }
  };

  template<class T, std::size_t N, class Compare>
  struct detailed_equality_checker<maths::static_monotonic_sequence<T, N, Compare>>
  {
    using type = maths::static_monotonic_sequence<T, N, Compare>;
    
    template<class Logger>
    static void check(Logger& logger, const type& sequence, const type& prediction, std::string_view description)
    {
      impl::check(logger, sequence, prediction, description);
    }
  };

  template<class T, std::size_t N, class Compare>
  struct equivalence_checker<maths::static_monotonic_sequence<T, N, Compare>>
  {
    using type = maths::static_monotonic_sequence<T, N, Compare>;
    
    template<class Logger>
    static void check(Logger& logger, const type& sequence, std::initializer_list<T> prediction, std::string_view description)
    {
      check_range(logger, sequence.begin(), sequence.end(), prediction.begin(), prediction.end(), description);            
    }
  };
}
