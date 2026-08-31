////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

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
#include <future>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <ranges>
#include <scoped_allocator>
#include <set>
#include <source_location>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

import sequoia.test_framework;

/** \file */


namespace sequoia::testing
{
  template<>
  struct value_tester<template_spec>
  {
    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const template_spec& obtained, const template_spec& prediction)
    {
      check(equality, "Species", logger, obtained.species, prediction.species);
      check(equality, "Symbol",  logger, obtained.symbol,  prediction.symbol);
    }
  };

  class bar_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests()
    {
      check(equality, {"Phoney equality check"}, 1, 1);
      throw std::runtime_error{"Throw after check"};
    }
  };
}
