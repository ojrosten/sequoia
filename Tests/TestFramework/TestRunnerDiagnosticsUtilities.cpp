////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerDiagnosticsUtilities.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <fstream>
#include <ranges>
#include <stdexcept>

namespace sequoia::testing
{
  namespace
  {
    /** \brief A line of text that `>>` reads whole.

        So `std::views::istream<line>` yields lines, where
        `std::views::istream<std::string>` would yield words.
     */
    struct line
    {
      std::string text{};

      friend std::istream& operator>>(std::istream& stream, line& toRead)
      {
        return std::getline(stream, toRead.text);
      }
    };
  }

  [[nodiscard]]
  std::vector<std::string> execution_record_lines(const std::filesystem::path& record)
  {
    if(std::ifstream file{record})
    {
      return   std::views::istream<line>(file)
             | std::views::transform(&line::text)
             | std::ranges::to<std::vector>();
    }

    throw std::runtime_error{report_failed_read(record)};
  }

  [[nodiscard]]
  std::filesystem::path bar_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }
}
