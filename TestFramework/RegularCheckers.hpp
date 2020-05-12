////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Functions for checking regular semantics.

    Types exhibiting regular semantics are pleasant to reason about. Within this
    library, a type is regular if it possesses the following:
    
    copy constructor
    move constructor
    copy assignment
    move assignment
    swap
    operator==
    operator!=

    Note that a default constructor is not a strict requirement. Types additionally
    possessing the remaining comparison operators will be referred to as having
    ordered semantics.

    This file adds two functions to the check_semantics overload set; both functions are
    appropriate for testing the behaviour of types with regular semantics. Inside the
    functions, consistency of the operators listed above will be checked. One of the overloads
    also accepts a mutator. This will modify a copy of y, checking both that the copy is
    indeed changed and also that y is left alone. The reason for this is to check that classes
    which e.g. wrap a pointer have faithful copy semantics. For example, suppose that
    the copy constructor of vector were incorrectly implemented with the pointer, rather
    than what it points to, being copied. The mutation check would catch this. There is a
    similar check for copy assignment.
 */

#include "FreeCheckers.hpp"
#include "RegularCheckersDetails.hpp"

namespace sequoia::unit_testing
{
  /// Precondition: x!=y
  template<test_mode Mode, class T, class Mutator>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, const T& x, const T& y, Mutator yMutator)
  {
    static_assert(has_regular_semantics_v<T>);
    
    impl::check_semantics(description, logger, impl::default_actions{}, x, y, yMutator);
  }

  /// Precondition: x!=y
  template<test_mode Mode, class T>
  void check_semantics(std::string_view description, test_logger<Mode>& logger, const T& x, const T& y)
  {
    static_assert(has_regular_semantics_v<T>);

    impl::check_semantics(description, logger, impl::default_actions{}, x, y, impl::null_mutator{});
  }
}
