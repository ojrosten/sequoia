////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for recording information from test failures.
 */

#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <vector>

namespace sequoia::testing
{
  struct failure_info
  {
    std::size_t check_index{};
    std::string message{};

    [[nodiscard]]
    friend auto operator<=>(const failure_info&, const failure_info&) noexcept = default;
    
    friend std::ostream& operator<<(std::ostream& s, const failure_info& info);

    friend std::istream& operator>>(std::istream& s, failure_info& info);
  };

  using failure_output = std::vector<failure_info>;

  std::ostream& operator<<(std::ostream& s, const failure_output& output);

  std::istream& operator>>(std::istream& s, failure_output& output);

  [[nodiscard]]
  std::string instability_analysis(const std::filesystem::path& root, const std::size_t trials);
}
