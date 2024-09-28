////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "CommandsFreeTest.hpp"
#include "sequoia/TestFramework/Commands.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"

namespace sequoia::testing
{
  using namespace runtime;

  namespace
  {
    struct postprocessor
    {
      [[nodiscard]]
      std::string operator()(std::string mess) const
      {
        constexpr auto npos{std::string::npos};
        const auto [first, last] {find_sandwiched_text(mess, pattern, "output")};
        if((first != npos) && (last != npos))
        {
          mess.erase(first, last - first);
        }

        return mess;
      }

      std::string_view pattern;
    };
  }

  [[nodiscard]]
  std::filesystem::path commands_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void commands_free_test::run_tests()
  {
    test_exceptions();
  }

  void commands_free_test::test_exceptions()
  {
    const auto root{working_materials()}, buildSubdir{root / "build/CMade"};
    check_exception_thrown<std::runtime_error>(report("No cache file"),
      [&]() { return cmake_cmd(std::nullopt, build_paths{root, "", buildSubdir / "NoCacheFile/CMakeCache.txt"}, ""); },
      postprocessor{" in "});

    check_exception_thrown<std::runtime_error>(report("Empty cache file"),
      [&]() { return cmake_cmd(std::nullopt, build_paths{root, "", buildSubdir / "EmptyCacheFile/CMakeCache.txt"}, ""); },
      postprocessor{" from "});

    check_exception_thrown<std::runtime_error>(report("No CXX compiler when Unix Makefiles specified"),
      [&]() { return cmake_cmd(std::nullopt, build_paths{root, "", buildSubdir / "NoCXXCompiler/CMakeCache.txt"}, ""); },
      postprocessor{" from "});
  }
}
