////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file RegularCheckers.hpp
    \brief Functions for checking regular semantics.
*/

#include "FreeCheckers.hpp"
#include "RegularCheckersDetails.hpp"

namespace sequoia::unit_testing
{
  template<test_mode Mode, class T, class Mutator>
  void check_semantics(std::string_view description, unit_test_logger<Mode>& logger, const T& x, const T& y, Mutator yMutator)
  {
    impl::check_semantics(description, logger, impl::default_actions{}, x, y, yMutator);
  }
    
  template<test_mode Mode, class T>
  void check_semantics(std::string_view description, unit_test_logger<Mode>& logger, const T& x, const T& y)
  {
    impl::check_semantics(description, logger, impl::default_actions{}, x, y, impl::null_mutator{});
  }
}
