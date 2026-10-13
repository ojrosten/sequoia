////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file
    \brief Definitions for ExecutionRecordTestingUtilities.hpp
 */

#include "ExecutionRecordTestingUtilities.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <ranges>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

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

    constexpr auto label_of{
      [](std::string_view recordLine) { return std::string{recordLine.substr(0, recordLine.rfind(' '))}; }
    };
  }

  [[nodiscard]]
  std::vector<std::string> execution_record_lines(const fs::path& record)
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
  std::vector<std::string> execution_record_labels(const fs::path& record)
  {
    return   execution_record_lines(record)
           | std::views::transform(label_of)
           | std::ranges::to<std::vector>();
  }

  [[nodiscard]]
  std::string execution_record_value(const fs::path& record, std::string_view label)
  {
    auto givesValue{
      [label](std::string_view recordLine) {
        return (label_of(recordLine) == label) && (recordLine.size() > label.size());
      }
    };

    const auto lines{execution_record_lines(record)};
    if(std::ranges::count_if(lines, givesValue) != 1)
      throw std::runtime_error{std::format("Expected exactly one {} in: {}", label, record.generic_string())};

    return std::ranges::find_if(lines, givesValue)->substr(label.size() + 1);
  }
}
