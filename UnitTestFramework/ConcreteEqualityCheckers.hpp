////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "ConcreteEquivalenceCheckers.hpp"

namespace sequoia::unit_testing
{
  template<class T, class S>
  struct detailed_equality_checker<std::pair<T, S>> : equivalence_checker<std::pair<T, S>>
  {};

  template<class... T>
  struct detailed_equality_checker<std::tuple<T...>> : equivalence_checker<std::tuple<T...>>
  {};
}
