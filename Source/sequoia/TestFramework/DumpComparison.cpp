////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/DumpComparison.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <map>
#include <ranges>

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    std::string trimmed_of_newlines(std::string_view text)
    {
      const auto first{text.find_first_not_of('\n')};
      if(first == std::string_view::npos)
        return {};

      const auto last{text.find_last_not_of('\n')};
      return std::string{text.substr(first, last - first + 1)};
    }

    // A location is a line "<file>, Line <number>"; a check opens with one, and a check nested in
    // it may add another
    constexpr std::string_view line_number_prefix{", Line "};

    [[nodiscard]]
    bool is_location(std::string_view line)
    {
      const auto prefix{line.rfind(line_number_prefix)};
      if(prefix == std::string_view::npos)
        return false;

      const auto number{line.substr(prefix + line_number_prefix.size())};
      return !number.empty() && std::ranges::all_of(number, [](char c){ return std::isdigit(static_cast<unsigned char>(c)); });
    }

    [[nodiscard]]
    std::string_view without_line_number(std::string_view location)
    {
      return location.substr(0, location.rfind(line_number_prefix));
    }

    [[nodiscard]]
    auto lines_of(std::string_view text)
    {
      return text | std::views::split('\n') | std::views::transform([](const auto& line){ return std::string_view{line}; });
    }

    /** \brief A check's identity: its text with the line number removed from every location in it. */
    [[nodiscard]]
    std::string identity_of(std::string_view check)
    {
      auto anonymised{[](std::string_view line){ return is_location(line) ? without_line_number(line) : line; }};

      return   lines_of(check)
             | std::views::transform(anonymised)
             | std::views::join_with('\n')
             | std::ranges::to<std::string>();
    }

    /** \brief A check's location and the first line after it which says anything, joined by ": ";
        the location alone if nothing does.
     */
    [[nodiscard]]
    std::string location_and_description(std::string_view check)
    {
      auto lines{lines_of(check)};
      const std::string_view location{*lines.begin()};

      auto rest{lines | std::views::drop(1)};
      const auto description{std::ranges::find_if(rest, [](std::string_view line){ return !line.empty(); })};

      return (description == rest.end()) ? std::string{location} : std::format("{}: {}", location, *description);
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

      // The excess is attributed to the last occurrences: a lost instantiation is reported where
      // the template's checks end, and a check found again earlier in the dump is not the one lost
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
           | std::views::split(dump_format::check_separator)
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
    auto report{
      std::format("Dump compared with '{}': {} missing, {} added\n",
                  baselineName,
                  with_count("check", comparison.missing.size()),
                  with_count("check", comparison.added.size()))
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
