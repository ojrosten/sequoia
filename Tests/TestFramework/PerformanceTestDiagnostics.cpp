////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceTestDiagnostics.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <thread>

namespace sequoia::testing
{
  namespace
  {
    void wait(std::chrono::milliseconds t)
    {
      std::this_thread::sleep_for(t);
    }
  }

  [[nodiscard]]
  std::filesystem::path performance_false_negative_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_false_negative_diagnostics::run_tests()
  {
    test_relative_performance();
  }

  void performance_false_negative_diagnostics::test_relative_performance()
  {
    constexpr std::chrono::milliseconds deltaT{5};

    check_relative_performance("Performance Test for which fast task is too slow, [1, (2.0, 2.0)",
                               [deltaT]() { wait(deltaT); },
                               [deltaT]() { wait(deltaT); }, 2.0, 2.0);

    check_relative_performance("Performance Test for which fast task is too slow [1, (2.0, 3.0)",
                               [deltaT]() { wait(deltaT); },
                               [deltaT]() { wait(deltaT); }, 2.0, 3.0);

    check_relative_performance("Performance Test for which fast task is too fast [4, (2.0, 2.5)]",
                               [deltaT]() { wait(deltaT); },
                               [deltaT]() { wait(4 * deltaT); }, 2.0, 2.5);
  }

  [[nodiscard]]
  std::filesystem::path performance_false_positive_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_false_positive_diagnostics::run_tests()
  {
    test_relative_performance();
  }

  void performance_false_positive_diagnostics::test_relative_performance()
  {
    constexpr std::chrono::milliseconds deltaT{5};

    check_relative_performance("Performance Test which should pass",
                               [deltaT]() { wait(deltaT); },
                               [deltaT]() { wait(2 * deltaT); }, 1.8, 2.1, 5);

    check_relative_performance("Performance Test which should pass",
                               [deltaT]() { wait(deltaT); },
                               [deltaT]() { wait(4 * deltaT); }, 3.4, 4.1, 5);
  }

  [[nodiscard]]
  std::filesystem::path performance_utilities_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_utilities_test::run_tests()
  {
    test_postprocessing();
    test_coarse_sleep();
  }

  void performance_utilities_test::test_postprocessing()
  {
    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest{"foo Task duration: 1.45e-3s +- 3 * 0.0014\n"};
      std::string_view reference{"bar Task duration: 1.47e-3s +- 3 * 0.0011\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"foo Task duration: 1.45e-3s +- 3 * 0.0014\n"
                              "bar Task duration: 1.50e-3s +- 3 * 0.0019\n"};
      std::string_view reference{"foo Task duration: 1.47e-3s +- 3 * 0.0011\n"
                                 "bar Task duration: 1.51e-3s +- 3 * 0.0016\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest{"foo Task duration: 1.45e-3s +- 3 * 0.0014\n"
                              "bar Task duration: 1.50e-3s +- 3 * 0.0019\n"};
      std::string_view reference{"foo Task duration: 1.47e-3s +- 3 * 0.0011\n"
                                 "baz Task duration: 1.51e-3s +- 3 * 0.0016\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014\n"};
      std::string_view reference{""};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{""};
      std::string_view reference{"Task duration: 1.45e-3s +- 3 * 0.0014\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014\n"
                              "bar Task duration: 1.50e-3s +- 3 * 0.0019\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011\n"
                                 "bar Task duration: 1.50e-3s +- 3 * 0.0019\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014 [3.4; (2.9, 4.1))]\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011 [3.4; (2.9, 4.1))]\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014 [3.4; (2.9, 4.1))]\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.001 [3.4; (2.9, 4.1))]\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014 [3.4; (2.8, 4.1))]\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011 [3.4; (2.9, 4.1))]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014 [3.4; (2.8, 4.1))]\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 3 * 0.0011 [3.4; (2.9, 4.0))]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest{"Task duration: 1.45e-3s +- 3 * 0.0014\n"};
      std::string_view reference{"Task duration: 1.47e-3s +- 4 * 0.0011\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }
  }

  void performance_utilities_test::test_coarse_sleep()
  {
    using fractional_milliseconds = std::chrono::duration<double, std::milli>;

    constexpr fractional_milliseconds target{5.0};

    check("Rounded up to Windows' default tick", is_coarse_sleep(fractional_milliseconds{15.2}, target));
    check("Exactly twice the target", is_coarse_sleep(fractional_milliseconds{10.0}, target));
    check("Just under twice the target", !is_coarse_sleep(fractional_milliseconds{9.9}, target));
    check("A 1 ms timer resolution in effect", !is_coarse_sleep(fractional_milliseconds{5.4}, target));

    write_to_file(working_materials() /= "CoarseSleepMessage.txt",
                  coarse_sleep_message(fractional_milliseconds{15.2}, target),
                  std::ios_base::out);

    check(equivalence,
          "Message",
          working_materials() /= "CoarseSleepMessage.txt",
          predictive_materials() /= "CoarseSleepMessage.txt");
  }
}
