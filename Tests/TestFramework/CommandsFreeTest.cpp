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

    check_exception_thrown<std::runtime_error>("No cache file",
      [&]() { return cmake_cmd(build_paths{root, buildSubdir, ""}, ""); });
  }
}
