////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <compare>
#include <concepts>
#include <execution>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <optional>
#include <scoped_allocator>
#include <source_location>
#include <span>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module sequoia.test_framework:MoveOnlyCheckers;

import :Advice;
import :BinaryRelationships;
import :CoreInfrastructure;
import :FailureInfo;
import :FreeCheckers;
import :MoveOnlyCheckersDetails;
import :Output;
import :ProjectPaths;
import :SemanticsCheckersDetails;
import :TestLogger;
import :TestMode;
export import sequoia.core.meta;
export import sequoia.platform_specific;
export import sequoia.text_processing;

/** \file
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


export namespace sequoia::testing
{
  /** Prerequisites:
      x != y
      x equivalent to xEquivalent
      y equivalent to yEquivalent
   */
  template<test_mode Mode, moveonly T, class U, class V>
    requires checkable_against_for_semantics<Mode, T, U> && checkable_against_for_semantics<Mode, T, V>
  bool check_semantics(std::string description,
                       test_logger<Mode>& logger,
                       T&& x,
                       T&& y,
                       const U& xEquivalent,
                       const U& yEquivalent,
                       optional_ref<const V> movedFromPostConstruction,
                       optional_ref<const V> movedFromPostAssignment)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description)).append("\n")};

    if constexpr(equivalence_checkable_for_semantics<Mode, T, U>)
    {
      impl::check_best_equivalence(logger, x, y, xEquivalent, yEquivalent);
    }

    return impl::check_semantics(
             logger,
             impl::auxiliary_data<T>{},
             std::forward<T>(x),
             std::forward<T>(y),
             xEquivalent,
             yEquivalent,
             movedFromPostConstruction,
             movedFromPostAssignment,
             impl::null_mutator{}
           );
  }
  
  /** Prerequisites:
      x != y
      x equivalent to xEquivalent
      y equivalent to yEquivalent
   */
  template<test_mode Mode, moveonly T, class U, class V>
    requires deep_totally_ordered<T> && checkable_against_for_semantics<Mode, T, U> && checkable_against_for_semantics<Mode, T, V>
  bool check_semantics(std::string description,
                       test_logger<Mode>& logger,
                       T&& x,
                       T&& y,
                       const U& xEquivalent,
                       const U& yEquivalent,
                       optional_ref<const V> movedFromPostConstruction,
                       optional_ref<const V> movedFromPostAssignment,
                       std::weak_ordering order)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description)).append("\n")};

    if constexpr(equivalence_checkable_for_semantics<Mode, T, U>)
    {
      impl::check_best_equivalence(logger, x, y, xEquivalent, yEquivalent);
    }

    return impl::check_semantics(
             logger,
             impl::auxiliary_data<T>{order},
             std::forward<T>(x),
             std::forward<T>(y),
             xEquivalent,
             yEquivalent,
             movedFromPostConstruction,
             movedFromPostAssignment,
             impl::null_mutator{}
           );
  }
}
