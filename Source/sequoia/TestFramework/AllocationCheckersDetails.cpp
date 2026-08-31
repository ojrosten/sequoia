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

module sequoia.test_framework;

/** \file
    \brief Definitions for AllocationCheckersDetails.hpp
*/


namespace sequoia::testing::impl
{
  [[nodiscard]]
  std::string allocation_advice::operator()(int count, int) const
  {
    return (count < 0)
      ? "A negative allocation count generally indicates an allocator propagting when it shouldn't or not propagating when it should.\n"
        "Alternatively, for scoped allocator adaptors, it may be that the predicted number of (inner) containers is incorrect."
      : "";
  }
}
