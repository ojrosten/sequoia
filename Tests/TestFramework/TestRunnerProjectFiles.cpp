////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

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

#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"
#include "TestRunnerProjectFiles.hpp"
#include "sequoia/PlatformSpecific/Macros.hpp"

import sequoia.test_framework;

namespace sequoia::testing
{
  using namespace runtime;
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::filesystem::path test_runner_project_files::source_file() const
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  std::filesystem::path test_runner_project_files::generated_project() const
  {
    return working_materials().parent_path() /= "GeneratedProject";
  }

  void test_runner_project_files::run_tests()
  {
    // A .vcxproj is a property of the Visual Studio generator, so there is nothing
    // for the other columns to compare against. Guarding here rather than omitting
    // the test keeps one registration for every platform, at the cost of a summary
    // that varies - which is the whole reason this is a test of its own.
    if constexpr(with_msvc_v)
    {
      test_project_files();
    }
  }

  void test_runner_project_files::test_project_files()
  {
    // --no-build because only the *generated* project files are under test, and cmake
    // generates those at configure time. Compiling the new project as well would cost
    // a great deal for nothing this test looks at; the end-to-end test pays that price
    // deliberately, for its own reasons.
    commandline_arguments args{{get_project_paths().discovered().executable().generic_string(),
                                "init",
                                "Oliver Jacob Rosten",
                                generated_project().string(),
                                "\t",
                                "--no-build",
                                "--to-files", "GenerationOutput.txt"}};

    std::stringstream outputStream{};

    const auto relativeMainCppPath{rebase_from(get_project_paths().main().file(), get_project_paths().project_root())};
    test_runner tr{args.size(),
                   args.get(),
                   "Oliver J. Rosten",
                   "  ",
                   {.main_cpp{relativeMainCppPath.generic_string()}, .common_includes{"TestCommon/TestIncludes.hpp"}},
                   outputStream};

    const auto build{make_new_build_paths(generated_project(), get_project_paths().build())};
    const main_paths main{generated_project() / main_paths::default_main_cpp_from_root()};

    invoke(cd_cmd(main.dir()) && cmake_cmd(build, generated_project() / "CMakeOutput.txt"));

    check("CMake cache existance", fs::exists(build.cmake_cache_dir() / "CMakeCache.txt"));

    const fs::path subdirs{"ProjectFiles" / back(get_project_paths().build().cmake_cache_dir())};
    fs::create_directories(working_materials() /= subdirs);
    fs::copy(build.cmake_cache_dir() / "TestAll.vcxproj", working_materials() /= subdirs);

    check(equivalence, report("Project files"), working_materials() /= subdirs, predictive_materials() /= subdirs);
  }
}
