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
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <scoped_allocator>
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
  class threading_models_performance_test final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:

    void test_waiting_task(const std::chrono::milliseconds millisecs);
    void test_waiting_task_return(const std::chrono::milliseconds millisecs);
  };

  class wait
  {
    std::chrono::milliseconds m_Wait;
  public:
    wait(const std::chrono::milliseconds millisecs) : m_Wait{millisecs} {}

    void operator()() const
    {
      std::this_thread::sleep_for(m_Wait);
    }
  };
}
