////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for summarizing results of tests.
*/

#include "sequoia/Core/Logic/Bitmask.hpp"
#include "sequoia/TestFramework/TestLogger.hpp"

#include <optional>

namespace sequoia::testing
{
  /*! bit mask for the level of detail */
  enum class summary_detail { none=0, absent_checks=1, failure_messages=2, timings=4};
}

namespace sequoia
{
  template<>
  struct as_bitmask<testing::summary_detail> : std::true_type
  {};
}

namespace sequoia::testing
{
  template<std::forward_iterator Iter> void pad_right(Iter begin, Iter end, std::string_view suffix)
  {
    auto maxIter{
      std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
          return lhs.size() < rhs.size();
        }
      )
    };

    const auto maxChars{maxIter->size()};

    for(; begin != end; ++begin)
    {
      auto& s{*begin};
      s += std::string(maxChars - s.size(), ' ') += std::string{suffix};
    }
  }

  template<std::forward_iterator Iter> void pad_left(Iter begin, Iter end, const std::size_t minChars)
  {
    auto maxIter{std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
          return lhs.size() < rhs.size();
        }
      )
    };

    const auto maxChars{std::max(maxIter->size(), minChars)};

    for(; begin != end; ++begin)
    {
      auto& s{*begin};
      s = std::string(maxChars - s.size(), ' ') + s;
    }
  }

  using opt_duration = std::optional<log_summary::duration>;

  struct stringified_duration
  {
    std::string time, unit;
  };

  [[nodiscard]]
  stringified_duration stringify(const log_summary::duration& d);

  [[nodiscard]]
  std::string report_time(const log_summary& log, opt_duration duration);

  [[nodiscard]]
  std::string summarize(const log_summary& log, std::string_view namesuffix, opt_duration duration, const summary_detail verbosity, indentation ind_0, indentation ind_1);

  [[nodiscard]]
  std::string summarize(const log_summary& log, std::string_view namesuffix, summary_detail verbosity, indentation ind_0, indentation ind_1);
}
