////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ProjectPathsFreeTest.hpp"

import std;
import sequoia.test_framework;

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
    test_build_configuration();
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
    check(equality,
          "Default-constructed build paths give no configuration",
          build_paths{}.configuration(),
          std::string{});
  }
}
