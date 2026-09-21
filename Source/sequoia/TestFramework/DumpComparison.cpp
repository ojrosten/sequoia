////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/DumpComparison.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <ranges>

namespace sequoia::testing
{
  namespace
  {
    // A dump is what the logger wrote: each check's message and a newline, then two blank lines.
    // A message may end in a newline of its own, so a check is taken without its surrounding ones
    constexpr std::string_view check_terminator{"\n\n\n"};

    [[nodiscard]]
    std::string trimmed_of_newlines(std::string_view text)
    {
      const auto first{text.find_first_not_of('\n')};
      if(first == std::string_view::npos)
        return {};

      const auto last{text.find_last_not_of('\n')};
      return std::string{text.substr(first, last - first + 1)};
    }

    // The dump opens each check with its location, "<file>, Line <number>", then its description
    constexpr std::string_view line_number_prefix{", Line "};

    [[nodiscard]]
    std::string_view first_line(std::string_view text)
    {
      return text.substr(0, text.find('\n'));
    }

    /** \brief A check's identity: its text with the line number of its location removed. */
    [[nodiscard]]
    std::string identity_of(std::string_view check)
    {
      const auto location{first_line(check)};
      const auto lineNumber{location.rfind(line_number_prefix)};

      return std::string{location.substr(0, lineNumber)}.append(check.substr(location.size()));
    }

    /** \brief A check's location and description, joined by ": " */
    [[nodiscard]]
    std::string location_and_description(std::string_view check)
    {
      const auto location{first_line(check)};
      if(location.size() == check.size())
        return std::string{check};

      return std::format("{}: {}", location, first_line(check.substr(location.size() + 1)));
    }

    /** \brief Each check of the dump keyed by its identity, with how many times it occurs. */
    [[nodiscard]]
    std::map<std::string, std::size_t> occurrences(const std::vector<std::string>& checks)
    {
      std::map<std::string, std::size_t> counts{};
      for(const auto& check : checks)
      {
        ++counts[identity_of(check)];
      }

      return counts;
    }

    /** \brief The checks of `checks` whose identity occurs more often there than in `other`,
        each as many times as the excess, in the order they appear in `checks`.
     */
    [[nodiscard]]
    std::vector<std::string> surplus(const std::vector<std::string>& checks, const std::vector<std::string>& other)
    {
      auto excess{occurrences(checks)};
      for(const auto& [identity, count] : occurrences(other))
      {
        if(const auto found{excess.find(identity)}; found != excess.end())
          found->second -= std::ranges::min(count, found->second);
      }

      // Checks are taken in reverse, so that the excess is attributed to the last occurrences
      std::vector<std::string> taken{};
      for(const auto& check : checks | std::views::reverse)
      {
        if(auto& remaining{excess[identity_of(check)]}; remaining)
        {
          --remaining;
          taken.push_back(check);
        }
      }

      return taken | std::views::reverse | std::ranges::to<std::vector>();
    }
  }

  [[nodiscard]]
  std::vector<std::string> read_dump(const std::filesystem::path& dump)
  {
    const auto text{read_to_string(dump, std::ios_base::in)};
    if(!text)
      throw std::runtime_error{std::format("Unable to read the dump {}", dump.generic_string())};

    return   *text
           | std::views::split(check_terminator)
           | std::views::transform([](const auto& check){ return trimmed_of_newlines(std::string_view{check}); })
           | std::views::filter([](const std::string& check){ return !check.empty(); })
           | std::ranges::to<std::vector>();
  }

  [[nodiscard]]
  dump_comparison compare_dumps(const std::filesystem::path& baseline, const std::filesystem::path& dump)
  {
    const auto before{read_dump(baseline)}, after{read_dump(dump)};

    return {.missing{surplus(before, after)}, .added{surplus(after, before)}};
  }

  [[nodiscard]]
  std::string to_string(const dump_comparison& comparison, std::string_view baselineName)
  {
    auto count{[](std::size_t n){ return std::format("{} check{}", n, (n == 1) ? "" : "s"); }};

    auto report{
      std::format("Dump compared with '{}': {} missing, {} added\n",
                  baselineName,
                  count(comparison.missing.size()),
                  count(comparison.added.size()))
    };

    auto section{
      [&report](std::string_view heading, const std::vector<std::string>& checks) {
        if(checks.empty())
          return;

        report += std::format("\n{}:\n", heading);
        for(const auto& check : checks)
        {
          report += std::format("  {}\n", location_and_description(check));
        }
      }
    };

    section("Missing", comparison.missing);
    section("Added", comparison.added);

    return report;
  }
}
