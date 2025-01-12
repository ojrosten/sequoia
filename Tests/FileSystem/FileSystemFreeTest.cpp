////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FileSystemFreeTest.hpp"
#include "sequoia/FileSystem/FileSystem.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::filesystem::path file_system_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void file_system_free_test::run_tests()
  {
    check_exception_thrown<std::runtime_error>("",  [](){ return back(""); });
    check(equality, "", back("A"), fs::path{"A"});
    check(equality, "", back("A/B"), fs::path{"B"});

    {
      fs::path p{"A/B/C"};
      check("", rfind(p, "B") == std::ranges::next(p.begin(), 1, p.end()));
    }

    {
      fs::path p{"A/B/C/B"};
      check("", rfind(p, "B") == std::ranges::next(p.begin(), 3, p.end()));
    }

    {
      const fs::path p{"A/B/C"};
      check("", rfind(p, "B") == std::ranges::next(p.begin(), 1, p.end()));
    }
  }
}
