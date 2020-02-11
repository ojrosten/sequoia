////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceTestDiagnostics.hpp"

namespace sequoia::unit_testing
{  
  [[nodiscard]]
  std::string_view performance_false_positive_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void performance_false_positive_diagnostics::run_tests()
  {
    test_relative_performance();
  }

  void performance_false_positive_diagnostics::test_relative_performance()
  {
    auto wait{[](const size_t millisecs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
      }
    };    
    
    check_relative_performance(LINE("Performance Test for which fast task is too slow, [1, (2.0, 2.0)"),
                               [wait]() { return wait(1); },
                               [wait]() { return wait(1); }, 2.0, 2.0);
      
    check_relative_performance(LINE("Performance Test for which fast task is too slow [1, (2.0, 3.0)"),
                               [wait]() { return wait(1); },
                               [wait]() { return wait(1); }, 2.0, 3.0);
      
    check_relative_performance(LINE("Performance Test for which fast task is too fast [4, (2.0, 2.5)]"),
                               [wait]() { return wait(1); },
                               [wait]() { return wait(4); }, 2.0, 2.5);      
  }

  [[nodiscard]]
  std::string_view performance_false_negative_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void performance_false_negative_diagnostics::run_tests()
  {
    test_relative_performance();
  }

  void performance_false_negative_diagnostics::test_relative_performance()
  {
      
    auto wait{[](const size_t millisecs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(millisecs));
      }
    };

    auto fast{[wait]() {
        return wait(1);
      }
    };      

    check_relative_performance(LINE("Performance Test which should pass"), fast, [wait]() { return wait(2); }, 1.8, 2.1, 5);
    check_relative_performance(LINE("Performance Test which should pass"), fast, [wait]() { return wait(4); }, 3.6, 4.1, 5);
  }
}
