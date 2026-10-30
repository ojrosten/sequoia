////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "WorkloadTests.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <ranges>

namespace workload
{
  namespace fs = std::filesystem;

  namespace
  {
    constexpr std::size_t files_per_test{64};
  }

  [[nodiscard]]
  fs::path workload_test_base::source_file()
  {
    return std::source_location::current().file_name();
  }

  void workload_test_base::run_tests()
  {
    const auto dir{scratchpad_materials() / "Written"};
    fs::create_directory(dir);
    std::ranges::for_each(std::views::iota(0uz, files_per_test),
                          [&dir](std::size_t i) { std::ofstream{dir / std::format("{}.txt", i)} << i << '\n'; });

    check("A file is written", fs::exists(dir / "0.txt"));
  }
}
