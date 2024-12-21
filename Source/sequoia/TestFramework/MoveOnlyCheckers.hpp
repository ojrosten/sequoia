////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
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

    \anchor move_only_definition
*/

#include "sequoia/TestFramework/FreeCheckers.hpp"
#include "sequoia/TestFramework/MoveOnlyCheckersDetails.hpp"

namespace sequoia::testing
{
  /*! Preconditions:

      x != y
   */
  template<test_mode Mode, moveonly T, class S>
  bool check_semantics(std::string description,
                       test_logger<Mode>& logger,
                       T&& x,
                       T&& y,
                       const S& equivalent)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description)).append("\n")};

    return impl::check_semantics(
             logger,
             impl::auxiliary_data<T>{},
             std::forward<T>(x),
             std::forward<T>(y),
             equivalent,
             impl::null_mutator{}
           );
  }
  
  /*! Preconditions:

      x == xClone
      y == yClone
      x != y
   */
  template<test_mode Mode, moveonly T>
  bool check_semantics(std::string description,
                       test_logger<Mode>& logger,
                       T&& x,
                       T&& y,
                       const T& xClone,
                       const T& yClone,
                       opt_moved_from_ref<T> movedFromPostConstruction,
                       opt_moved_from_ref<T> movedFromPostAssignment)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description)).append("\n")};

    return impl::check_semantics(
             logger,
             impl::auxiliary_data<T>{},
             std::forward<T>(x),
             std::forward<T>(y),
             xClone,
             yClone,
             movedFromPostConstruction,
             movedFromPostAssignment,
             impl::null_mutator{}
           );
  }

  /*! Preconditions:

      x != y
   */
  template<test_mode Mode, moveonly T, class S>
  bool check_semantics(std::string description,
                       test_logger<Mode>& logger,
                       T&& x,
                       T&& y,
                       const S& equivalent,
                       std::weak_ordering order)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description)).append("\n")};

    return impl::check_semantics(
             logger,
             impl::auxiliary_data<T>{order},
             std::forward<T>(x),
             std::forward<T>(y),
             equivalent,
             impl::null_mutator{}
           );
  }
  
  /*! Preconditions:

      x == xClone
      y == yClone
      x != y
   */
  template<test_mode Mode, moveonly T>
    requires std::totally_ordered<T>
  bool check_semantics(std::string description,
                       test_logger<Mode>& logger,
                       T&& x,
                       T&& y,
                       const T& xClone,
                       const T& yClone,
                       opt_moved_from_ref<T> movedFromPostConstruction,
                       opt_moved_from_ref<T> movedFromPostAssignment,
                       std::weak_ordering order)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description)).append("\n")};

    return impl::check_semantics(
             logger,
             impl::auxiliary_data<T>{order},
             std::forward<T>(x),
             std::forward<T>(y),
             xClone,
             yClone,
             movedFromPostConstruction,
             movedFromPostAssignment,
             impl::null_mutator{}
           );
  }
}
