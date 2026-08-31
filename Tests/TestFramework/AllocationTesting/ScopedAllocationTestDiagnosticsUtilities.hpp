////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */


#include "../RegularTestDiagnosticsUtilities.hpp"

#include <vector>
#include <string>

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
#include <memory>
#include <optional>
#include <scoped_allocator>
#include <source_location>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

import sequoia.test_framework;

namespace sequoia::testing
{
  template<template<class...> class OuterType, class InnerType>
  struct scoped_beast_builder
  {
    using inner_type = InnerType;

    using inner_allocator_type = InnerType::allocator_type;

    using outer_allocator_type = inner_allocator_type::template rebind<inner_type>::other;

    using allocator_type = std::scoped_allocator_adaptor<outer_allocator_type, inner_allocator_type>;

    using beast = OuterType<inner_type, allocator_type>;
  };
}
