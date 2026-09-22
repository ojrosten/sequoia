////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CMakeCacheFreeTest.hpp"

import std;

namespace sequoia::testing
{
  using namespace std::string_literals;

  [[nodiscard]]
  std::filesystem::path cmake_cache_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void cmake_cache_free_test::run_tests()
  {
    test_absent_cache();
    test_variables();
    test_generator_families();
  }

  [[nodiscard]]
  build_paths cmake_cache_free_test::tree(std::string_view name) const
  {
    const auto root{working_materials()};
    return {root, root / name, root / name};
  }

  void cmake_cache_free_test::test_absent_cache()
  {
    check_exception_thrown<std::runtime_error>("No cache file", [this]() { return cmake_cache{tree("NoCache")}; });
  }

  void cmake_cache_free_test::test_variables()
  {
    const cmake_cache cache{tree("Ninja")};

    check(equality, "Value with colons",           cache.variable("CMAKE_COMMAND"),            std::optional{"C:/Program Files/CMake/bin/cmake.exe"s});
    check(equality, "Empty value",                 cache.variable("CMAKE_CXX_FLAGS"),          std::optional{""s});
    check(equality, "Prefix of a longer name",     cache.variable("CMAKE_GENERATOR"),          std::optional{"Ninja"s});
    check(equality, "Longer name",                 cache.variable("CMAKE_GENERATOR_INSTANCE"), std::optional{""s});
    check(equality, "Absent",                      cache.variable("CMAKE_BUILD_TYPE"),         std::optional<std::string>{});
    check(equality, "Comment carrying = and :",    cache.variable("// Rebuilding"),            std::optional<std::string>{});
    check(equality, "Name only, no type or value", cache.variable("STRAY"),                    std::optional<std::string>{});
  }

  void cmake_cache_free_test::test_generator_families()
  {
    check("Ninja",          cmake_cache{tree("Ninja")}.generator_family()        == cmake_generator_family::ninja);
    check("Visual Studio",  cmake_cache{tree("VisualStudio")}.generator_family() == cmake_generator_family::visual_studio);
    check("Unix Makefiles", cmake_cache{tree("Makefiles")}.generator_family()    == cmake_generator_family::other);

    check_exception_thrown<std::runtime_error>("No generator recorded", [this]() { return cmake_cache{tree("NoGenerator")}.generator_family(); });
  }
}
