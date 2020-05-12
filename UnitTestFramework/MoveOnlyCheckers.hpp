////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Functions for checking semantics of types lacking copy semantics but which are otherwise regular.

    Within this library, a type is defined to have move-only semantics if possesses the following:
        
    move constructor 
    move assignment
    swap
    operator==
    operator!=

    but lacks

    copy constructor
    copy assignment

    Note that a default constructor is not a strict requirement. 

    This file adds one function to the check_semantics overload set, inside which consistency
    of the first list of operators above will be checked. There is an important difference
    compared to the corresponding overloads for regular semantics. Whereas the latter takes just
    two instance of T, x and y, in the move-only case 4 are taken. This is because, by definition,
    we cannot copy x and y and so, to be able to compare to the original values (essential for
    the mechanics of testing), immutable clones must be supplied.
*/

#include "FreeCheckers.hpp"
#include "MoveOnlyCheckersDetails.hpp"

namespace sequoia::unit_testing
{
  template<test_mode Mode, class T>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, T&& x, T&& y, const T& xClone, const T& yClone)
  {
    impl::check_semantics(description, logger, impl::default_actions{}, std::forward<T>(x), std::forward<T>(y), xClone, yClone, impl::null_mutator{});
  }
}
