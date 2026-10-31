////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestLoggerFreeTest.hpp"
#include "sequoia/TestFramework/TestLogger.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <string>
#include <string_view>

namespace sequoia::testing
{
  namespace
  {
    /** Returns the summary, named `name`, of a logger in `Mode` after one
        check, in which the logger recorded a failure and a caught exception's
        message, each naming `name`.
     */
    template<test_mode Mode>
    [[nodiscard]]
    log_summary summary_with_named_texts(std::string_view name)
    {
      test_logger<Mode> logger{};
      {
        sentinel<Mode> sentry{logger, std::format("{}'s check", name)};
        sentry.log_check();
        sentry.log_failure(std::format("{}'s failure", name));
        sentry.log_caught_exception_message(std::format("{}'s caught exception", name));
      }

      return {name, logger, log_summary::duration{}};
    }
  }

  [[nodiscard]]
  std::filesystem::path test_logger_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_logger_free_test::run_tests()
  {
    test_texts_of_a_sum();
  }

  /** Each summand's texts name it, so a sum that lost, replaced or reordered
      a summand's text differs from the summands' texts joined in order. A
      failure goes to the failure messages in standard mode, and to the
      diagnostics output in false-negative mode.
   */
  void test_logger_free_test::test_texts_of_a_sum()
  {
    const auto first{summary_with_named_texts<test_mode::standard>("First")},
               second{summary_with_named_texts<test_mode::standard>("Second")},
               firstInFalseNegativeMode{summary_with_named_texts<test_mode::false_negative>("First")},
               secondInFalseNegativeMode{summary_with_named_texts<test_mode::false_negative>("Second")};

    const std::array summandTexts{
      first.failure_messages(),
      second.failure_messages(),
      first.caught_exceptions_output(),
      second.caught_exceptions_output(),
      firstInFalseNegativeMode.diagnostics_output(),
      secondInFalseNegativeMode.diagnostics_output()
    };

    check("Every summand's text is non-empty, so no check of a sum can pass vacuously",
          std::ranges::none_of(summandTexts, &std::string::empty));

    auto sum{first};
    sum += second;

    check(equality,
          "A sum's failure messages are its summands', in order",
          sum.failure_messages(),
          first.failure_messages() + second.failure_messages());

    check(equality,
          "A sum's caught exceptions are its summands', in order",
          sum.caught_exceptions_output(),
          first.caught_exceptions_output() + second.caught_exceptions_output());

    auto sumInFalseNegativeMode{firstInFalseNegativeMode};
    sumInFalseNegativeMode += secondInFalseNegativeMode;

    check(equality,
          "A sum's diagnostics output is its summands', in order",
          sumInFalseNegativeMode.diagnostics_output(),
          firstInFalseNegativeMode.diagnostics_output() + secondInFalseNegativeMode.diagnostics_output());
  }
}
