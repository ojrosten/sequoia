////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

module sequoia.test_framework;

import std;

import sequoia.runtime;

namespace sequoia::testing
{
  namespace
  {
    constexpr auto return_code_names{
      std::to_array<std::pair<return_code, std::string_view>>({
        {return_code::versioned_output_diffs, "versioned_output_diffs"},
        {return_code::soft_failures,          "soft_failures"         },
        {return_code::critical_failures,      "critical_failures"     },
        {return_code::incomplete_run,         "incomplete_run"        },
        {return_code::post_run_failures,      "post_run_failures"     }
      })
    };

    constexpr return_code dirty_return_codes{
      std::ranges::fold_left(
        return_code_names,
        return_code::success,
        [](return_code acc, const auto& entry) { return acc | entry.first; }
      )
    };

    static_assert(std::has_single_bit(std::to_underlying(dirty_return_codes) + 1),
                  "The flags with rows in return_code_names do not occupy consecutive bits from the lowest; "
                  "give every return_code flag a row, and keep the flags consecutive");

    /** \brief The number a runner adds to its `return_code` to give its exit
               status.

        If the code is `return_code::success`, the exit status is 0 instead.

        A runner therefore exits either with 0 or with a status from
        `runner_exit_offset + 1` to `max_runner_exit_status`. That range is
        clear of these statuses, which a process gives when it fails for
        reasons of its own:
        -# 1 and 2, generically;
        -# 23, from LeakSanitizer;
        -# 64 to 78, from BSD's sysexits;
        -# 66 and 77, from ThreadSanitizer and MemorySanitizer;
        -# 126 and above, from a shell.

        ReturnCodes.cppm states this value in the contract of `to_exit_code`.
     */
    constexpr int runner_exit_offset{80};

    /** \brief The highest exit status a runner gives: the one which carries every flag. */
    constexpr int max_runner_exit_status{
      runner_exit_offset + static_cast<int>(std::to_underlying(dirty_return_codes))
    };

    static_assert(runner_exit_offset > 78, "The runner's exit statuses would overlap BSD's sysexits");

    // A POSIX exit status is 8 bits, and a shell's own statuses begin at 126.
    static_assert(max_runner_exit_status <= 125,
                  "The runner's exit statuses no longer fit below the shell's; report the return_code "
                  "to a parent process through a file it names, rather than in the exit status");
  }

  std::string to_string(const return_code code)
  {
    if((code & ~dirty_return_codes) != return_code::success)
      throw std::logic_error{std::format("Unrecognized bits in return_code: {}", std::to_underlying(code))};

    if(code == return_code::success) return "success";

    return std::ranges::fold_left(
             return_code_names,
             std::string{},
             [code](std::string name, const auto& entry) {
               const auto& [bit, text] {entry};

               if((code & bit) == bit)
               {
                 if(!name.empty()) name.append("|");
                 name.append(text);
               }

               return name;
             }
           );
  }

  [[nodiscard]]
  int to_exit_code(const return_code code) noexcept
  {
    if(code == return_code::success)
      return 0;

    const auto carried{(code & ~dirty_return_codes) == return_code::success
                         ? code
                         : (code & dirty_return_codes) | return_code::incomplete_run};

    return runner_exit_offset + static_cast<int>(std::to_underlying(carried));
  }

  [[nodiscard]]
  return_code child_return_code(const int exitStatus, std::string_view childDescription)
  {
    if(exitStatus == 0)
      return return_code::success;

    // The range is checked before the offset is subtracted. A Windows status of 0x80000000 or more
    // is negative, and near INT_MIN the subtraction would overflow.
    if((exitStatus <= runner_exit_offset) || (exitStatus > max_runner_exit_status))
    {
      throw std::runtime_error{
        std::format("{} {}.\nThat is not one of a test runner's exit statuses, which are 0 and {} to {}, so "
                    "it did not complete a test run: it may not have been built, may be misconfigured, "
                    "or may have crashed.\n",
                    childDescription,
                    runtime::describe_exit_status(exitStatus),
                    runner_exit_offset + 1,
                    max_runner_exit_status)
      };
    }

    return static_cast<return_code>(exitStatus - runner_exit_offset);
  }
}
