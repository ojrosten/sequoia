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
          const auto [first2, last2] {find_sandwiched_text(mess, "CMade/", "/", first)};
          if((first2 != npos) && (last2 != npos))
          {
            mess.replace(first2, last2 - first2, "...");
          }
        }

        return mess;
      }

      std::string_view pattern;
    };
  }

  [[nodiscard]]
  std::filesystem::path commands_free_test::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void commands_free_test::run_tests()
  {
    test_exceptions();
  }

  void commands_free_test::test_exceptions()
  {
    const auto root{working_materials()};
    check_exception_thrown<std::runtime_error>(report_line("No cache file"),
      [&root]() { return cmake_cmd(std::nullopt, build_paths{root, {root / "NoCacheFile/CMakeLists.txt" , ""}}, ""); },
      postprocessor{" in "});

    check_exception_thrown<std::runtime_error>(report_line("'Parent' with no cache file"),
      [&root]() { return cmake_cmd(build_paths{root, {root / "NoCacheFile/CMakeLists.txt" , ""}}, build_paths{root, {root / "NoCacheFile/CMakeLists.txt" , ""}}, ""); },
      postprocessor{" in "});

    check_exception_thrown<std::runtime_error>(report_line("Empty cache file"),
      [&root]() { return cmake_cmd(std::nullopt, build_paths{root, {root / "EmptyCacheFile/CMakeLists.txt" , ""}}, ""); },
      postprocessor{" from "});

    check_exception_thrown<std::runtime_error>(report_line("No CXX compiler when Unix Makefiles specified"),
      [&root]() { return cmake_cmd(std::nullopt, build_paths{root, {root / "NoCXXCompiler/CMakeLists.txt" , ""}}, ""); },
      postprocessor{" from "});
  }
}
