////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Functions for checking regular semantics.

    Types exhibiting regular semantics are pleasant to reason about. Within this
    library, a type is regular if it exhibits the following:

    copy constructor
    move constructor
    copy assignment
    move assignment
    swap
    operator==
    operator!=

    Note that a default constructor is not a strict requirement. To distinguish this
    Concept from std::regular, the Concept pseudoregular is used. Types additionally
    possessing the remaining comparison operators will be referred to as being
    std::totally_ordered.

    This file adds functions to the check_semantics overload set: they are
    appropriate for testing the behaviour of types with regular/std::totally_ordered semantics. Inside the
    functions, consistency of the operators listed above will be checked. One of the overloads
    also accepts a mutator. This will modify a copy of y, checking both that the copy is
    indeed changed and also that y is left alone. The reason for this is to check that classes
    which e.g. wrap a pointer have faithful copy semantics. For example, suppose that
    the copy constructor of vector were incorrectly implemented with the pointer, rather
    than what it points to, being copied. The mutation check would catch this. There is a
    similar check for copy assignment.

    \anchor regular_semantics_definition
 */

#include "sequoia/TestFramework/FreeCheckers.hpp"
#include "sequoia/TestFramework/RegularCheckersDetails.hpp"

namespace sequoia::testing
{
  /// Prerequisite: x!=y
  template<test_mode Mode, pseudoregular T>
  void check_semantics(std::string description, test_logger<Mode>& logger, const T& x, const T& y)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description)).append("\n")};
    impl::check_semantics(logger, impl::auxiliary_data<T>{}, x, y, impl::null_mutator{});
  }

  /// Prerequisite: x!=y with values consistent with order
  template<test_mode Mode, pseudoregular T>
    requires std::totally_ordered<T>
  void check_semantics(std::string description, test_logger<Mode>& logger, const T& x, const T& y, std::weak_ordering order)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description)).append("\n")};
    impl::check_semantics(logger, impl::auxiliary_data<T>{order}, x, y, impl::null_mutator{});
  }

  /// Prerequisite: x!=y
  template<test_mode Mode, pseudoregular T, std::invocable<T&> Mutator>
  void check_semantics(std::string description, test_logger<Mode>& logger, const T& x, const T& y, Mutator yMutator)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description)).append("\n")};
    impl::check_semantics(logger, impl::auxiliary_data<T>{}, x, y, yMutator);
  }

  /// Prerequisite: x!=y, with values consistent with order
  template<test_mode Mode, pseudoregular T, std::invocable<T&> Mutator>
    requires std::totally_ordered<T>
  void check_semantics(std::string description, test_logger<Mode>& logger, const T& x, const T& y, std::weak_ordering order, Mutator yMutator)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description)).append("\n")};
    impl::check_semantics(logger, impl::auxiliary_data<T>{order}, x, y, order, yMutator);
  }
}
