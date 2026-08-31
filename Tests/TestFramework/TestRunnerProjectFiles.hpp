////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

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
#include <memory>
#include <optional>
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

#include "sequoia/PlatformSpecific/Macros.hpp"

import sequoia.test_framework;

/** \file */


namespace sequoia::testing
{
  /** \brief Checks the IDE project files a created project's build system generates.

      This is the one genuinely platform-dependent thing the test runner produces: a
      `.vcxproj` exists only under the Visual Studio generator. It lives here, alone,
      rather than inside the end-to-end test, so that the far larger and more valuable
      test of project creation and incremental building stays free of a summary
      discriminator - see the note in run_tests.
   */
  class test_runner_project_files final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    /** Only this test's own check count varies by platform, and it is one check. */
    [[nodiscard]]
    std::string summary_discriminator() const
    {
      return with_msvc_v ? "msvc" : std::string{};
    }

    void run_tests();
  private:
    void test_project_files();

    [[nodiscard]]
    std::filesystem::path generated_project() const;
  };
}
