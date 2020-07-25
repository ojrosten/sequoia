////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

#include "LinearSequence.hpp"

namespace sequoia::testing
{
  template<class T, integral Index>
  struct detailed_equality_checker<maths::linear_sequence<T, Index>>
  {
    using type = maths::linear_sequence<T, Index>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& sequence, const type& prediction)
    {
      sentinel<Mode> s{logger, description};
      
      check_equality(s.generate_message("Start"), logger, sequence.start(), prediction.start());
      check_equality(s.generate_message("Step"), logger, sequence.step(), prediction.step());
    }
  };

  template<class T, integral Index>
  struct equivalence_checker<maths::linear_sequence<T, Index>>
  {
    using type = maths::linear_sequence<T, Index>;
    
    template<test_mode Mode>
    static void check( std::string_view description, test_logger<Mode>& logger, const type& sequence, const T& start, const T& step)
    {
      sentinel<Mode> s{logger, description};
      
      check_equality(s.generate_message("Start wrong"), logger, sequence.start(), start);
      check_equality(s.generate_message("Step wrong"), logger, sequence.step(), step);
    }
  };
}
