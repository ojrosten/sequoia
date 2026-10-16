////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A test run's outcome, and its encoding as a runner's exit status.
*/

#include "sequoia/Core/Logic/Bitmask.hpp"

#include <format>
#include <string>
#include <string_view>

namespace sequoia::testing
{
  /** \brief A bitmask to record the outcome of a test run.

      The flags occupy consecutive bits, from the lowest, and each needs a row in
      `return_code_names` (ReturnCodes.cpp); no check catches a missing row for the highest flag.
   */
  enum class return_code : unsigned {
    success                = 0,
    versioned_output_diffs = 1 << 0,
    soft_failures          = 1 << 1,
    critical_failures      = 1 << 2,
    incomplete_run         = 1 << 3,
    post_run_failures      = 1 << 4
  };

  [[nodiscard]]
  std::string to_string(return_code code);

  /** \brief Encodes `code` as a runner's exit status.

      \returns
      -# 0, if `code` is `return_code::success`;
      -# 80 plus the value of `code`, if every bit of `code` is a flag;
      -# Otherwise, 80 plus the value of the flags of `code`, with `incomplete_run` set.
   */
  [[nodiscard]]
  int to_exit_code(return_code code) noexcept;

  /** \brief The `return_code` which `exitStatus` carries, as encoded by
             `to_exit_code`.

      On Windows, a tool can exit with a Win32 error code, such as 87, 110 or 111, which `to_exit_code`
      also returns. This function reads that status as a runner's.

      \throws std::runtime_error if `exitStatus` is not a status which `to_exit_code` returns. The
              message begins with `childDescription`.
   */
  [[nodiscard]]
  return_code child_return_code(int exitStatus, std::string_view childDescription);
}

NAMESPACE_SEQUOIA_AS_BITMASK
{
  template<>
  struct as_bitmask<sequoia::testing::return_code> : std::true_type {};
}

namespace std
{
  template<>
  struct formatter<sequoia::testing::return_code>
  {
    constexpr auto parse(auto& ctx) { return ctx.begin(); }

    auto format(sequoia::testing::return_code code, auto& ctx) const -> decltype(ctx.out())
    {
      return std::format_to(ctx.out(), "{}", sequoia::testing::to_string(code));
    }
  };
}
