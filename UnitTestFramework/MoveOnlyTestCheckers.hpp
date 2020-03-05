////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestCheckers.hpp
    \brief Utilities for performing checks within the unit testing framework.
*/

#include "FreeTestCheckers.hpp"
#include "MoveOnlyTestCheckersDetails.hpp"

namespace sequoia::unit_testing
{
  template<class Logger, class T>
  void check_semantics(std::string_view description, Logger& logger, T&& x, T&& y, const T& xClone, const T& yClone)
  {
    impl::check_semantics(description, logger, impl::default_actions{}, std::forward<T>(x), std::forward<T>(y), xClone, yClone, impl::null_mutator{});
  }
}
