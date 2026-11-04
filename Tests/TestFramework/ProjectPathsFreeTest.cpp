////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ProjectPathsFreeTest.hpp"
#include "sequoia/TestFramework/ProjectPaths.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  fs::path project_paths_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void project_paths_free_test::run_tests()
  {
    test_no_default_construction();
    test_build_configuration();
  }

  void project_paths_free_test::test_no_default_construction()
  {
    // A default would hold an empty directory, so each file within it would
    // resolve against the current directory
    STATIC_CHECK(!std::is_default_constructible_v<recovery_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<drift_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<prune_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<execution_record_paths>);

    // A default would name no project: each path it held would be empty
    STATIC_CHECK(!std::is_default_constructible_v<project_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<discoverable_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<main_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<source_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<build_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<auxiliary_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<output_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<dependencies_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<tests_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<test_materials_paths>);
    STATIC_CHECK(!std::is_default_constructible_v<build_system_paths>);
  }

  void project_paths_free_test::test_build_configuration()
  {
    const fs::path root{"/proj"}, tree{root / "build" / "TestAll" / "llvm-homebrew"};

    check(equality,
          "An executable beside the CMake cache gives no configuration",
          build_paths{root, tree, tree}.configuration(),
          std::string{});
    check(equality,
          "An executable in a directory directly within the build tree gives that directory's name",
          build_paths{root, tree / "Release", tree}.configuration(),
          std::string{"Release"});
    check(equality,
          "An executable directory two levels within the build tree gives no configuration",
          build_paths{root, tree / "bin" / "Release", tree}.configuration(),
          std::string{});
    check(equality,
          "An executable directory outside the build tree gives no configuration",
          build_paths{root, root / "Release", tree}.configuration(),
          std::string{});
  }
}
