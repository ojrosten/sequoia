////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerConcurrencyTestingUtilities.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace sequoia::testing
{
  steady_clock_interval::steady_clock_interval(time_point start, time_point end)
    : m_Start{start}
    , m_End{end}
  {
    if(end < start)
      throw std::invalid_argument{"A steady clock interval cannot end before it starts"};
  }

  [[nodiscard]]
  std::size_t peak_overlap(std::span<const steady_clock_interval> intervals)
  {
    using change_t = std::pair<steady_clock_interval::time_point, std::ptrdiff_t>;
    auto endpoints{
      [](const steady_clock_interval& interval){
        return std::array{change_t{interval.start_time(), 1}, change_t{interval.end_time(), -1}};
      }
    };

    auto changes{
        intervals
      | std::views::transform(endpoints)
      | std::views::join
      | std::ranges::to<std::vector>()
    };

    // At one instant an end, -1, sorts before a start, +1
    std::ranges::sort(changes);

    // The overlap just after each change is the sum of the changes up to it
    auto overlaps{changes | std::views::values | std::ranges::to<std::vector>()};
    std::inclusive_scan(overlaps.begin(), overlaps.end(), overlaps.begin());

    return static_cast<std::size_t>(std::ranges::fold_left(overlaps, std::ptrdiff_t{}, std::ranges::max));
  }

  void print_peak_overlap(std::ostream& stream, std::size_t peak)
  {
    std::println(stream, "[Peak Overlap: {}]", peak);
  }

  void scratch_report_run(std::string_view label, std::span<const steady_clock_interval> intervals)
  {
    using namespace std::chrono;
    auto earliest{steady_clock::time_point::max()};
    for(const auto& i : intervals) earliest = std::min(earliest, i.start_time());

    std::string line{std::format("SCRATCH-PEAK {} peak {} sleep_ms {} starts_us", label, peak_overlap(intervals), sleep_duration_for_overlap_checks.count())};
    for(const auto& i : intervals) line += std::format(" {}", duration_cast<microseconds>(i.start_time() - earliest).count());
    line += " durs_us";
    for(const auto& i : intervals) line += std::format(" {}", duration_cast<microseconds>(i.end_time() - i.start_time()).count());

    std::cerr << line << std::endl;
    if(const char* ws{std::getenv("GITHUB_WORKSPACE")})
    {
      const std::filesystem::path dir{std::filesystem::path{ws} / "run-log"};
      std::error_code ec{};
      std::filesystem::create_directories(dir, ec);
      std::ofstream file{dir / "peaks.txt", std::ios_base::app};
      file << line << '\n';
    }
  }
}
