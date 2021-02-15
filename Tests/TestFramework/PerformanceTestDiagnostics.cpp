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
    const static auto delta_t{calibrate(std::chrono::milliseconds{5})};

    void wait(const std::size_t multiplier)
    {
      std::this_thread::sleep_for(delta_t * multiplier);
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
                               []() { wait(1); },
                               []() { wait(1); }, 2.0, 2.0);

    check_relative_performance(LINE("Performance Test for which fast task is too slow [1, (2.0, 3.0)"),
                               []() { wait(1); },
                               []() { wait(1); }, 2.0, 3.0);

    check_relative_performance(LINE("Performance Test for which fast task is too fast [4, (2.0, 2.5)]"),
                               []() { wait(1); },
                               []() { wait(4); }, 2.0, 2.5);
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
    check_relative_performance(LINE("Performance Test which should pass"), []() { wait(1); }, []() { wait(2); }, 1.8, 2.1, 5);
    check_relative_performance(LINE("Performance Test which should pass"), []() { wait(1); }, []() { wait(4); }, 3.4, 4.1, 5);
  }

  [[nodiscard]]
  std::string_view performance_utilities_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void performance_utilities_test::run_tests()
  {
    test_postprocessing();
  }

  void performance_utilities_test::test_postprocessing()
  {
    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011\n"};

      check_equality(LINE(""), postprocess(latest, reference), reference);
    }

    {
      std::string_view latest{"foo Task duration: 1.45e-3s +- 3 * 0.0014\n"};
      std::string_view reference{"bar Task duration: 1.47e-3s +- 3 * 0.0011\n"};

      check_equality(LINE(""), postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"foo Task duration: 1.45e-3s +- 3 * 0.0014\n"
                              "bar Task duration: 1.50e-3s +- 3 * 0.0019\n"};
      std::string_view reference{"foo Task duration: 1.47e-3s +- 3 * 0.0011\n"
                                 "bar Task duration: 1.51e-3s +- 3 * 0.0016\n"};

      check_equality(LINE(""), postprocess(latest, reference), reference);
    }

    {
      std::string_view latest{"foo Task duration: 1.45e-3s +- 3 * 0.0014\n"
                              "bar Task duration: 1.50e-3s +- 3 * 0.0019\n"};
      std::string_view reference{"foo Task duration: 1.47e-3s +- 3 * 0.0011\n"
                                 "baz Task duration: 1.51e-3s +- 3 * 0.0016\n"};

      check_equality(LINE(""), postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014\n"};
      std::string_view reference{""};

      check_equality(LINE(""), postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{""};
      std::string_view reference{"Task duration: 1.45e-3s +- 3 * 0.0014\n"};

      check_equality(LINE(""), postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014\n"
                              "bar Task duration: 1.50e-3s +- 3 * 0.0019\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011\n"};

      check_equality(LINE(""), postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011\n"      
                                 "bar Task duration: 1.50e-3s +- 3 * 0.0019\n"};

      check_equality(LINE(""), postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014 [3.4; (2.9, 4.1))]\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011 [3.4; (2.9, 4.1))]\n"};

      check_equality(LINE(""), postprocess(latest, reference), reference);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014 [3.4; (2.9, 4.1))]\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.001 [3.4; (2.9, 4.1))]\n"};

      check_equality(LINE(""), postprocess(latest, reference), reference);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014 [3.4; (2.8, 4.1))]\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011 [3.4; (2.9, 4.1))]\n"};

      check_equality(LINE(""), postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014 [3.4; (2.8, 4.1))]\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011 [3.4; (2.9, 4.0))]\n"};

      check_equality(LINE(""), postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 4 * 0.0011\n"};

      check_equality(LINE(""), postprocess(latest, reference), latest);
    }
  }
}
