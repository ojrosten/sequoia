////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Utilities for summarizing results of tests.
*/

#include "TestLogger.hpp"

#include <optional>

namespace sequoia::testing
{
  template<class Iter> void pad_right(Iter begin, Iter end, std::string_view suffix)
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
      
  template<class Iter> void pad_left(Iter begin, Iter end, const std::size_t minChars)
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
    
  [[nodiscard]]
  std::string stringify(const log_summary::duration& d);

  [[nodiscard]]
  std::string report_time(const log_summary& log, const opt_duration duration);

  [[nodiscard]]
  std::string summarize(const log_summary& log, const opt_duration duration, std::string_view indent, const log_verbosity suppression);

  [[nodiscard]]
  std::string summarize(const log_summary& log, std::string_view indent, const log_verbosity suppression);
}
