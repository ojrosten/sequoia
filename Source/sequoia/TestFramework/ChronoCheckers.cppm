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

export module sequoia.test_framework:ChronoCheckers;

import :Advice;
import :BinaryRelationships;
import :CoreInfrastructure;
import :FailureInfo;
import :FreeCheckers;
import :Output;
import :ProjectPaths;
import :TestLogger;
import :TestMode;
export import sequoia.core.meta;
export import sequoia.platform_specific;
export import sequoia.text_processing;

/** \file
    \brief Specializations of the class template value_tester for `std::chrono::time_point`.

    See ConcreteTypeCheckers.hpp for how nested checks dispatch, and for a single
    header pulling in every specialization at once.
 */



export namespace sequoia::testing
{
  /** \brief Compares instance of `std::chrono::time_point`

      For the advice to be invoked, `tutor` must be constructed by a function object
      with an overload `operator()(std::chrono::nanoseconds, std::chrono::nanoseconds)`.
   */

  template<class Clock, class Duration>
  struct value_tester<std::chrono::time_point<Clock, Duration>>
  {
    using type = std::chrono::time_point<Clock, Duration>;

    template<test_mode Mode, class Advisor>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor)
    {
      using ns = std::chrono::nanoseconds;
      check(equality,
            "Time since epoch",
            logger,
            std::chrono::duration_cast<ns>(obtained.time_since_epoch()),
            std::chrono::duration_cast<ns>(prediction.time_since_epoch()), advisor);
    }
  };
}
