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
        if(const auto start{mess.find(pattern)}; start != npos)
        {
          const auto pos{start + pattern.size()};
          if(const auto end{mess.find("output", pos)}; end != npos)
          {
            mess.erase(pos, end - pos);
          }

          const auto [first, last] {find_sandwiched_text(mess, "CMade/", "/", pos)};
          mess.replace(first, last-first, "...");
        }

        return mess;
      }

      std::string_view pattern;
    };
  }

  [[nodiscard]]
  std::string_view commands_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void commands_free_test::run_tests()
  {
    test_exceptions();
  }

  void commands_free_test::test_exceptions()
  {
    const auto& root{working_materials()};
    check_exception_thrown<std::runtime_error>(LINE("No cache file"),
      [&root]() { return cmake_cmd(std::nullopt, build_paths{root, {root / "NoCacheFile/CMakeLists.txt" , ""}}, ""); },
      postprocessor{" in "});

    check_exception_thrown<std::runtime_error>(LINE("'Parent' with no cache file"),
      [&root]() { return cmake_cmd(build_paths{root, {root / "NoCacheFile/CMakeLists.txt" , ""}}, build_paths{root, {root / "NoCacheFile/CMakeLists.txt" , ""}}, ""); },
      postprocessor{" in "});

    check_exception_thrown<std::runtime_error>(LINE("Empty cache file"),
      [&root]() { return cmake_cmd(std::nullopt, build_paths{root, {root / "EmptyCacheFile/CMakeLists.txt" , ""}}, ""); },
      postprocessor{" from "});

    check_exception_thrown<std::runtime_error>(LINE("No CXX compiler when Unix Makefiles specified"),
      [&root]() { return cmake_cmd(std::nullopt, build_paths{root, {root / "NoCXXCompiler/CMakeLists.txt" , ""}}, ""); },
      postprocessor{" from "});
  }
}
