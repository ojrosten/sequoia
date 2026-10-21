////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerConcurrencyTestingUtilities.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace sequoia::testing
{
  sleep_interval::sleep_interval(time_point start, time_point end)
    : m_Start{start}
    , m_End{end}
  {
    if(end < start)
      throw std::invalid_argument{"A sleep interval cannot end before it starts"};
  }

  [[nodiscard]]
  std::size_t peak_overlap(std::span<const sleep_interval> intervals)
  {
    using change_t = std::pair<sleep_interval::time_point, std::ptrdiff_t>;
    auto endpoints{
      [](const sleep_interval& interval){
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

  void write_peak_overlap(std::ostream& stream, std::size_t peak)
  {
    stream << std::format("[Peak Overlap: {}]\n", peak);
  }
}
