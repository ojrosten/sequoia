
#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <compare>
#include <complex>
#include <concepts>
#include <execution>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <numbers>
#include <numeric>
#include <optional>
#include <ranges>
#include <ratio>
#include <scoped_allocator>
#include <source_location>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

import sequoia.maths.geometry;
import sequoia.test_framework;
////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */


namespace sequoia::testing
{
  template<maths::weak_commutative_ring T>
  struct value_tester<maths::coordinate_bounds<T>>
  {
    using bounds_type = maths::coordinate_bounds<T>;
    
    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const bounds_type& actual, const bounds_type& prediction)
    {
      check(equality, "Lower", logger, actual.lower, prediction.lower);
      check(equality, "Upper", logger, actual.upper, prediction.upper);
    }
  };
}
