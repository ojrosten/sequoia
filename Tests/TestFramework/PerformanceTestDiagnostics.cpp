////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceTestDiagnostics.hpp"

namespace sequoia::testing
{
  namespace
  {
    void wait(std::size_t millisecs)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
    }
  }

  [[nodiscard]]
  std::string_view performance_false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void performance_false_positive_diagnostics::run_tests()
  {
    test_relative_performance();
  }

  void performance_false_positive_diagnostics::test_relative_performance()
  {
    check_relative_performance(LINE("Performance Test for which fast task is too slow, [1, (2.0, 2.0)"),
                               []() { return wait(5); },
                               []() { return wait(5); }, 2.0, 2.0);
      
    check_relative_performance(LINE("Performance Test for which fast task is too slow [1, (2.0, 3.0)"),
                               []() { return wait(5); },
                               []() { return wait(5); }, 2.0, 3.0);
      
    check_relative_performance(LINE("Performance Test for which fast task is too fast [4, (2.0, 2.5)]"),
                               []() { return wait(5); },
                               []() { return wait(20); }, 2.0, 2.5);
  }

  [[nodiscard]]
  std::string_view performance_false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void performance_false_negative_diagnostics::run_tests()
  {
    test_relative_performance();
  }

  void performance_false_negative_diagnostics::test_relative_performance()
  {
    auto fast{[]() { return wait(5); } };

    check_relative_performance(LINE("Performance Test which should pass"), fast, []() { return wait(10); }, 1.8, 2.1, 5);
    check_relative_performance(LINE("Performance Test which should pass"), fast, []() { return wait(20); }, 3.6, 4.1, 5);
  }
}
