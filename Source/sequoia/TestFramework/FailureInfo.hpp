////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for recording information from test failures.
 */

#include "sequoia/Core/Functional/ErasedFunction.hpp"
#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <string>
#include <string_view>
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

  [[nodiscard]]
  std::string to_string(const failure_info& info);

  using failure_output = std::vector<failure_info>;

  [[nodiscard]]
  std::string to_string(const failure_output& output);

  std::ostream& operator<<(std::ostream& s, const failure_output& output);

  std::istream& operator>>(std::istream& s, failure_output& output);

  /** \brief A map from a failure's message to the form in which it is compared
             with the same failure from other runs.
   */
  using message_projection = erased_function<std::string(std::string_view) const>;

  /** \brief Reports each test whose runs, `trials` of them, failed
             differently.

      Each failure's message passes through `projection` before the runs are
      compared, so that what the projection discards, such as measurements, is
      not a difference.
   */
  [[nodiscard]]
  std::string instability_analysis(const std::filesystem::path& root,
                                   const std::size_t trials,
                                   const message_projection& projection);
}
